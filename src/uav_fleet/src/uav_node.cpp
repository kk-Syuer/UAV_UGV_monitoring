#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <deque>
#include <optional>
#include <cmath>
#include <limits>

#include "rclcpp/rclcpp.hpp"

#include "uav_msgs/msg/uav_status.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/cluster_info.hpp"
#include "uav_msgs/msg/charge_decision.hpp"
#include "uav_msgs/msg/charge_request.hpp"
#include "uav_msgs/msg/weather_status.hpp"

#include "geometry_msgs/msg/pose.hpp"
#include "geometry_msgs/msg/twist.hpp"

#include "uav_msgs/srv/send_debug_text.hpp"
#include "uav_msgs/msg/failure_event.hpp"
#include "uav_msgs/msg/uav_deployment.hpp"
#include <unordered_set>
#include <random>
#include <algorithm>
#include <sstream>
#include <vector>
#include <deque>
#include <functional>




using namespace std::chrono_literals;

// Lightweight CSV splitter for control payloads.
static std::vector<std::string> splitString(const std::string & s, char delim)
{
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    out.push_back(item);
  }
  return out;
}


class UavNode : public rclcpp::Node
{
  struct BufferEntry
  {
    uav_msgs::msg::TrafficMessage msg;
    rclcpp::Time enqueue_time;
    rclcpp::Time next_retry_time;
    rclcpp::Time expiry_time;
  };

  class BufferManager
  {
  public:
    BufferManager() = default;

    void configure(size_t max_msgs,
                   double ttl_sec,
                   double retry_period_sec)
    {
      max_msgs_ = max_msgs;
      ttl_sec_ = ttl_sec;
      retry_period_sec_ = retry_period_sec;
    }

    size_t size() const { return queue_.size(); }

    bool enqueue(const uav_msgs::msg::TrafficMessage & msg,
                 const rclcpp::Time & now,
                 std::function<void(const uav_msgs::msg::TrafficMessage &, const std::string &)> drop_cb)
    {
      if (queue_.size() >= max_msgs_) {
        // Drop oldest
        drop_cb(queue_.front().msg, "BUFFER_OVERFLOW");
        queue_.pop_front();
      }
      BufferEntry entry;
      entry.msg = msg;
      entry.enqueue_time = now;
      entry.next_retry_time = now + rclcpp::Duration::from_seconds(retry_period_sec_);
      entry.expiry_time = now + rclcpp::Duration::from_seconds(ttl_sec_);
      queue_.push_back(entry);
      return true;
    }

    void tick(const rclcpp::Time & now,
              std::function<bool(uav_msgs::msg::TrafficMessage &)> try_send,
              std::function<void(const uav_msgs::msg::TrafficMessage &, const std::string &)> drop_cb)
    {
      for (auto it = queue_.begin(); it != queue_.end(); ) {
        if (now >= it->expiry_time) {
          drop_cb(it->msg, "BUFFER_TTL_EXPIRED");
          it = queue_.erase(it);
          continue;
        }
        if (now < it->next_retry_time) {
          ++it;
          continue;
        }
        if (try_send(it->msg)) {
          it = queue_.erase(it);
        } else {
          it->next_retry_time = now + rclcpp::Duration::from_seconds(retry_period_sec_);
          ++it;
        }
      }
    }

  private:
    std::deque<BufferEntry> queue_;
    size_t max_msgs_ = 200;
    double ttl_sec_ = 60.0;
    double retry_period_sec_ = 1.0;
  };

public:
  UavNode()
  : Node("uav_node"),
    msg_counter_(0),
    waiting_for_charge_response_(false),
    is_charging_(false),
    has_charge_slot_(false),
    charge_state_(ChargeState::IDLE),
    deployment_received_(false)
  {
    // ---- Parameters ----
    uav_id_ = this->declare_parameter<std::string>("uav_id", "uav_1");
    int role_int = this->declare_parameter<int>("role", 0);  // 0=MEMBER,1=CH
    role_ = static_cast<uint8_t>(role_int);

    cluster_id_ = this->declare_parameter<std::string>("cluster_id", "cluster_1");
    default_dst_id_ = this->declare_parameter<std::string>("default_dst_id", "sink_gateway");
    my_ch_id_ = this->declare_parameter<std::string>("my_ch_id", "uav_1");
    next_hop_to_sink_ = this->declare_parameter<std::string>(
      "next_hop_to_sink", default_dst_id_);

    // NEW: id of the UGV in the network, used as dst_id for CHARGE_REQUEST
    ugv_id_ = this->declare_parameter<std::string>("ugv_id", "ugv");
    monitor_id_ = this->declare_parameter<std::string>("monitor_id", "network_monitor");

    // Optional per-destination routing rules: ["dst:next_hop", ...]
    std::vector<std::string> routing_rules =
      this->declare_parameter<std::vector<std::string>>(
        "routing_rules", std::vector<std::string>{});

    for (const auto & rule : routing_rules) {
      auto pos = rule.find(':');
      if (pos == std::string::npos || pos == 0 || pos == rule.size() - 1) {
        RCLCPP_WARN(this->get_logger(),
                    "UAV %s: invalid routing rule '%s' (expected 'dst:next_hop')",
                    uav_id_.c_str(), rule.c_str());
        continue;
      }
      std::string dst = rule.substr(0, pos);
      std::string nh = rule.substr(pos + 1);
      routing_table_[dst] = nh;
      RCLCPP_INFO(this->get_logger(),
                  "UAV %s: routing rule added dst='%s' -> next_hop='%s'",
                  uav_id_.c_str(), dst.c_str(), nh.c_str());
    }

    double cap_member = this->declare_parameter<double>("battery_capacity_member", 100.0);
    double cap_ch     = this->declare_parameter<double>("battery_capacity_ch", 200.0);
    battery_capacity_ = (role_ == 1) ? static_cast<float>(cap_ch)
                                     : static_cast<float>(cap_member);
    battery_energy_ = battery_capacity_;  // start full

    battery_threshold_percent_ =
      static_cast<float>(this->declare_parameter<double>("battery_threshold", 30.0));
    charging_duration_sec_ =
      this->declare_parameter<double>("charging_duration_sec", 20.0);

    accept_direct_deployment_ =
      this->declare_parameter<bool>("accept_direct_deployment", false);

    // Base drain rates (energy units per second)
    double drain_member = this->declare_parameter<double>("drain_rate_member", 0.5);
    double drain_ch     = this->declare_parameter<double>("drain_rate_ch", 0.5);
    drain_rate_member_ = static_cast<float>(drain_member);
    drain_rate_ch_ = static_cast<float>(drain_ch);

    current_temperature_c_ = 22.0f;  // default comfy temp

    // Mobility parameters governing simulated kinematics.
    mobility_enabled_ =
      this->declare_parameter<bool>("mobility_enabled", true);
    mobility_dt_sec_ =
      this->declare_parameter<double>("mobility_dt_sec", 0.2);
    // Default cruise speed ~55 km/h
    uav_speed_mps_ =
      this->declare_parameter<double>("uav_speed_mps", 15.3);
    tasks_per_round_ =
      this->declare_parameter<int>("tasks_per_round", 8);
    mobility_phase_ = MobilityPhase::IDLE;
    deployment_goal_pose_.position.x = 0.0;
    deployment_goal_pose_.position.y = 0.0;
    deployment_goal_pose_.position.z = 0.0;
    deployment_goal_pose_.orientation.w = 1.0;

    RCLCPP_INFO(this->get_logger(),
                "Starting UAV node id='%s', role=%u, cluster=%s, dst='%s'. "
                "Batt capacity=%.1f, threshold=%.1f%%, charge_duration=%.1f s",
                uav_id_.c_str(), role_, cluster_id_.c_str(), default_dst_id_.c_str(),
                battery_capacity_, battery_threshold_percent_, charging_duration_sec_);

    // ---- Publishers ----
    status_pub_ = this->create_publisher<uav_msgs::msg::UavStatus>(
      "/fanet/status", 10);
    traffic_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/fanet/network_bus", 10);
    delivered_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/fanet/delivered", 10);
    charge_request_pub_ = this->create_publisher<uav_msgs::msg::ChargeRequest>(
      "/uav_fleet/charge_requests", 10);
    failure_pub_ = this->create_publisher<uav_msgs::msg::FailureEvent>(
      "/uav_fleet/failure_events", 10);

    // ---- Subscribers ----
    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/fanet/network_bus", 10,
      std::bind(&UavNode::trafficCallback, this, std::placeholders::_1));
    neighbor_status_sub_ = this->create_subscription<uav_msgs::msg::UavStatus>(
      "/fanet/status", 50,
      std::bind(&UavNode::neighborStatusCallback, this, std::placeholders::_1));

    cluster_sub_ = this->create_subscription<uav_msgs::msg::ClusterInfo>(
      "/ch_manager/cluster_info", 10,
      std::bind(&UavNode::clusterInfoCallback, this, std::placeholders::_1));

    weather_sub_ = this->create_subscription<uav_msgs::msg::WeatherStatus>(
      "/environment/weather", 10,
      std::bind(&UavNode::weatherCallback, this, std::placeholders::_1));
    deployment_sub_ = this->create_subscription<uav_msgs::msg::UavDeployment>(
      "/coverage_planner/deployment", 10,
      std::bind(&UavNode::deploymentCallback, this, std::placeholders::_1));

    // ---- Timers ----
    status_timer_ = this->create_wall_timer(
      1s, std::bind(&UavNode::publishStatus, this));
    heartbeat_timer_ = this->create_wall_timer(
      1s, std::bind(&UavNode::publishHeartbeat, this));
    traffic_timer_ = this->create_wall_timer(
      2s, std::bind(&UavNode::publishTraffic, this));

    hello_period_sec_ = this->declare_parameter<double>("hello_period_sec", 1.0);
    hello_timeout_sec_ = this->declare_parameter<double>("hello_timeout_sec", 3.0);
    status_period_sec_ = this->declare_parameter<double>("status_ch_period_sec", 5.0);
    status_ttl_ = static_cast<uint32_t>(
      this->declare_parameter<int>("status_ch_ttl", 20));
    buffer_enable_ = this->declare_parameter<bool>("buffer_enable", true);
    ctrl_buffer_enable_ = this->declare_parameter<bool>("ctrl_buffer_enable", false);
    buffer_max_msgs_ = static_cast<size_t>(
      this->declare_parameter<int>("buffer_max_msgs", 200));
    buffer_ttl_sec_ = this->declare_parameter<double>("buffer_ttl_sec", 90.0);
    buffer_retry_period_sec_ = this->declare_parameter<double>("buffer_retry_period_sec", 1.0);
    max_recent_hops_ = static_cast<size_t>(
      this->declare_parameter<int>("max_recent_hops", 5));

    // Location-aided DTN routing parameters
    location_aided_routing_ =
      this->declare_parameter<bool>("ladtr_enabled", true);
    location_progress_threshold_m_ =
      this->declare_parameter<double>("ladtr_progress_threshold_m", 5.0);
    carry_buffer_limit_ = static_cast<size_t>(
      this->declare_parameter<int>("ladtr_buffer_limit", 200));
    carry_ttl_sec_ = this->declare_parameter<double>("ladtr_buffer_ttl_sec", 45.0);
    carry_retry_period_sec_ = this->declare_parameter<double>("ladtr_retry_period_sec", 1.0);

    auto hello_period = std::chrono::duration<double>(hello_period_sec_);
    neighbor_timeout_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(hello_period),
      std::bind(&UavNode::pruneNeighbors, this));
    buffer_retry_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::duration<double>(buffer_retry_period_sec_)),
      std::bind(&UavNode::bufferTick, this));
    buffer_manager_.configure(buffer_max_msgs_, buffer_ttl_sec_, buffer_retry_period_sec_);

    auto carry_retry = std::chrono::duration<double>(carry_retry_period_sec_);
    ladtr_retry_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(carry_retry),
      std::bind(&UavNode::flushBufferedMessagesTimer, this));

    auto status_period = std::chrono::duration<double>(status_period_sec_);
    ch_status_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(status_period),
      std::bind(&UavNode::publishStatusCh, this));

    if (mobility_enabled_) {
      auto dt = std::chrono::duration<double>(mobility_dt_sec_);
      mobility_timer_ = this->create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(dt),
        std::bind(&UavNode::mobilityStep, this));
    }

    // Debug service: send a one-shot text message from this UAV
    debug_service_ = this->create_service<uav_msgs::srv::SendDebugText>(
      "/uav_fleet/" + uav_id_ + "/send_debug_text",
      std::bind(&UavNode::handleDebugSendText, this,
                std::placeholders::_1, std::placeholders::_2));
    // ---- Auto traffic generation parameter ----
    auto_traffic_enabled_ =
      this->declare_parameter<bool>("auto_traffic_enabled", false);

    // Allow toggling at runtime via `ros2 param set`
    param_cb_handle_ = this->add_on_set_parameters_callback(
      [this](const std::vector<rclcpp::Parameter> & params)
      {
        for (const auto & p : params) {
          if (p.get_name() == "auto_traffic_enabled") {
            auto_traffic_enabled_ = p.as_bool();
            RCLCPP_INFO(this->get_logger(),
                        "UAV %s: auto_traffic_enabled set to %s",
                        uav_id_.c_str(),
                        auto_traffic_enabled_ ? "true" : "false");
          }
        }
        // Always accept parameter changes
        rcl_interfaces::msg::SetParametersResult result;
        result.successful = true;
        return result;
      });

    // Dummy pose: everyone starts co-located at the origin
    pose_.position.x = 0.0;
    pose_.position.y = 0.0;
    pose_.position.z = 0.0;
    pose_.orientation.w = 1.0;

    ugv_pose_known_ = false;
    ugv_pose_.orientation.w = 1.0;

    service_radius_ = 400.0f;


    RCLCPP_INFO(this->get_logger(),
                "UAV %s ready. Role=%u, CH capacity flag=%s",
                uav_id_.c_str(), role_, (role_ == 1 ? "YES" : "NO"));
  }

private:
  struct NeighborState
  {
    rclcpp::Time last_seen;
    geometry_msgs::msg::Pose pose;
    geometry_msgs::msg::Twist velocity;
    float battery = 0.0f;
    uint8_t role = 0;
    uint8_t charging_state = 0;
    bool intent_to_leave = false;
    float eta_to_leave_sec = -1.0f;
    float comm_radius_m = 0.0f;
  };

  enum class ChargeState {
    IDLE = 0,
    TO_UGV,
    CHARGING,
    RETURNING
  };

  struct BufferedMessage
  {
    uav_msgs::msg::TrafficMessage msg;
    rclcpp::Time buffered_time;
  };

  // ---------------- Weather ----------------

  void weatherCallback(const uav_msgs::msg::WeatherStatus::SharedPtr msg)
  {
    current_temperature_c_ = msg->temperature_c;

    // New fields
    current_wind_speed_mps_ = msg->wind_speed;
    // assuming wind_direction_deg exists in WeatherStatus
    current_wind_dir_rad_ = msg->wind_direction_deg * static_cast<float>(M_PI / 180.0f);
    current_rain_intensity_ = msg->rain_intensity;
  }


  float temperatureFactor(float temp_c) const
  {
    // Very simple model:
    //  - Cold (< 5C): 1.7x drain
    //  - Cool (5–15C): 1.7 -> 1.0
    //  - Comfy (15–30C): 1.0x
    //  - Hot (30–40C): 1.0 -> 1.3
    //  - Very hot (> 40C): 1.4x
    if (temp_c < 5.0f) {
      return 1.7f;
    } else if (temp_c < 15.0f) {
      float alpha = (temp_c - 5.0f) / 10.0f; // 0..1
      return 1.7f - 0.7f * alpha;
    } else if (temp_c < 30.0f) {
      return 1.0f;
    } else if (temp_c < 40.0f) {
      float alpha = (temp_c - 30.0f) / 10.0f;
      return 1.0f + 0.3f * alpha;
    } else {
      return 1.4f;
    }
  }

  // ---------------- Battery & charging ----------------
  void publishStatus()
  {
    auto now = this->now();
    float energy_consumption_rate = 0.0f;
    geometry_msgs::msg::Twist velocity_msg = current_velocity_;

    bool ready_for_battery =
      deployment_received_ &&
      start_mobility_received_ &&
      (role_ == 1 || ch_deployment_reached_);

    // If we haven't received deployment/motion yet, stay "idle":
    // - no drain
    // - no charging logic
    // - no charge requests
    if (!ready_for_battery && deployment_received_) {
      // Keep reporting status (and backbone activity for CHs),
      // but skip any drain/charge logic until motion is allowed.
      energy_consumption_rate = 0.0f;
    } else if (!deployment_received_) {
      float battery_percent = 0.0f;
      if (battery_capacity_ > 0.0f) {
        battery_percent = (battery_energy_ / battery_capacity_) * 100.0f;
      }
      bool backbone_active = false;
      uav_msgs::msg::UavStatus msg;
      msg.uav_id = uav_id_;
      msg.role = role_;
      msg.cluster_id = cluster_id_;
      msg.battery_level = battery_percent;
      msg.battery_capacity = battery_capacity_;
      msg.pose = pose_;              // dummy pose until deployment
      msg.velocity = velocity_msg;
      msg.service_radius = service_radius_;
      msg.connected_users = 0;
      msg.traffic_load = 0.0f;
      msg.packet_loss_estimate = 0.0f;
      msg.energy_consumption_rate = 0.0f;
      msg.charging_state = 0;
      msg.intent_to_leave = false;
      msg.eta_to_leave_sec = -1.0f;
      msg.comm_radius_m = service_radius_;
      msg.stamp = now;
      msg.backbone_active = backbone_active;

      status_pub_->publish(msg);
      return;
    }

    // ---- Normal behaviour AFTER deployment ----
    if (deployment_received_ && charge_state_ == ChargeState::CHARGING) {
      // Charging: interpolate
      if (now >= charge_end_time_) {
        battery_energy_ = battery_capacity_;
        is_charging_ = false;
        charge_state_ = ChargeState::RETURNING;
        RCLCPP_INFO(this->get_logger(),
                    "UAV %s: finished charging, battery full (%.1f). Returning to (%.1f, %.1f).",
                    uav_id_.c_str(), battery_energy_,
                    charge_departure_pose_.position.x,
                    charge_departure_pose_.position.y);
      } else {
        double total = (charge_end_time_ - charge_start_time_).seconds();
        double elapsed = (now - charge_start_time_).seconds();
        if (total > 0.0 && elapsed >= 0.0) {
          double frac = elapsed / total;
          if (frac < 0.0) frac = 0.0;
          if (frac > 1.0) frac = 1.0;
          battery_energy_ = energy_at_charge_start_ +
            static_cast<float>((battery_capacity_ - energy_at_charge_start_) * frac);
        }
      }
    } else if (deployment_received_ && ready_for_battery) {
      // Not charging: drain battery based on role and temperature
      float base_drain = (role_ == 1) ? drain_rate_ch_ : drain_rate_member_;
      float temp_factor = temperatureFactor(current_temperature_c_);
      float f_motion = motionFactor(current_speed_mps_);
      float f_wind   = windFactor(current_speed_mps_);
      float f_rain   = rainFactor();

      float drain_rate = base_drain * temp_factor * f_motion * f_wind * f_rain;

      battery_energy_ -= drain_rate;

      if (battery_energy_ < 0.0f) {
        battery_energy_ = 0.0f;
      }
      energy_consumption_rate = drain_rate;
    }

    // Percentage
    float battery_percent = 0.0f;
    if (battery_capacity_ > 0.0f) {
      battery_percent = (battery_energy_ / battery_capacity_) * 100.0f;
    }
    // backbone activity flag (CH, deployed, not charging, not dead)
    bool backbone_active =
      (role_ == 1) && deployment_received_ && (battery_energy_ > 0.0f) && !is_charging_;

    // If we just died from battery, publish a FailureEvent once
    if (battery_energy_ <= 0.0f && !reported_battery_dead_) {
      reported_battery_dead_ = true;

      uav_msgs::msg::FailureEvent fe;
      fe.uav_id = uav_id_;
      fe.failure_type = 1;  // 1 = BATTERY_DEAD
      fe.description = "Battery depleted (0%).";
      fe.stamp = now;

      RCLCPP_WARN(this->get_logger(),
                  "UAV %s: BATTERY_DEAD at t=%.3f", uav_id_.c_str(), now.seconds());

      failure_pub_->publish(fe);
      publishFailureTraffic(fe);
    }

    // If low and not waiting or scheduled, request a charge slot.
    if (ready_for_battery && !is_charging_ && !waiting_for_charge_response_ && !has_charge_slot_ &&
        battery_percent <= battery_threshold_percent_)
    {
      requestCharge(battery_percent);
    }

    // Publish status
    uav_msgs::msg::UavStatus msg;
    msg.uav_id = uav_id_;
    msg.role = role_;
    msg.cluster_id = cluster_id_;
    msg.battery_level = battery_percent;
    msg.battery_capacity = battery_capacity_;
    msg.pose = pose_;
    msg.velocity = velocity_msg;
    msg.service_radius = service_radius_;
    msg.connected_users = 0;
    msg.traffic_load = 0.0f;
    msg.packet_loss_estimate = 0.0f;
    msg.energy_consumption_rate = energy_consumption_rate;

    switch (charge_state_) {
      case ChargeState::TO_UGV:
        msg.charging_state = 1;
        break;
      case ChargeState::CHARGING:
        msg.charging_state = 2;
        break;
      case ChargeState::RETURNING:
        msg.charging_state = 3;
        break;
      case ChargeState::IDLE:
      default:
        msg.charging_state = 0;
        break;
    }

    bool below_threshold = battery_percent <= battery_threshold_percent_;
    msg.intent_to_leave = below_threshold || waiting_for_charge_response_ ||
      has_charge_slot_ || charge_state_ == ChargeState::TO_UGV;

    float eta_sec = -1.0f;
    if (energy_consumption_rate > 0.0f && battery_capacity_ > 0.0f && battery_energy_ > 0.0f) {
      float threshold_energy = battery_capacity_ * (battery_threshold_percent_ / 100.0f);
      float delta_energy = battery_energy_ - threshold_energy;
      if (delta_energy <= 0.0f) {
        eta_sec = 0.0f;
      } else {
        eta_sec = delta_energy / energy_consumption_rate;
      }
    }
    msg.eta_to_leave_sec = eta_sec;
    msg.comm_radius_m = service_radius_;

    msg.stamp = now;
    msg.backbone_active = backbone_active;
    status_pub_->publish(msg);
  }


  void requestCharge(float battery_percent)
  {
    if (role_ == 0 && neighbors_.find(my_ch_id_) == neighbors_.end()) {
      RCLCPP_WARN(this->get_logger(),
                  "UAV %s: cannot request charge, CH %s not reachable",
                  uav_id_.c_str(), my_ch_id_.c_str());
      return;
    }
    // Set flag to avoid duplicate requests while waiting for decision
    waiting_for_charge_response_ = true;

    auto now = this->now();

    RCLCPP_INFO(this->get_logger(),
                "UAV %s: requesting charge via network (battery=%.1f%%)",
                uav_id_.c_str(), battery_percent);

    // 1) Publish a ChargeRequest for monitoring (unchanged)
    uav_msgs::msg::ChargeRequest cr;
    cr.uav_id = uav_id_;
    cr.role = role_;
    cr.battery_level = battery_percent;
    cr.stamp = now;
    charge_request_pub_->publish(cr);

    // 2) Send a CONTROL_ALERT message through the network to the UGV
    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = uav_id_ + "_charge_req_" + std::to_string(msg_counter_++);
    msg.src_id = uav_id_;
    msg.dst_id = ugv_id_;       // final destination: UGV
    msg.flow_type = 1;           // CONTROL_ALERT
    msg.creation_time = now;
    msg.hop_count = 0;

    // Let routing decide, as for any other dst:
    // - members will rely on their CH
    // - CHs will use routing_rules / next_hop_to_sink_
    if (role_ == 0) {  // MEMBER
      msg.next_hop_id = my_ch_id_;
    } else {           // CH
      msg.next_hop_id = pickNextHop(ugv_id_, resolveNextHop(ugv_id_));
    }

    // Optional control metadata to describe the control alert type.
    msg.control_type = "CHARGE_REQUEST";
    // For now payload is empty; UGV will look up status from /fanet/status

    RCLCPP_INFO(this->get_logger(),
                "[TX CTRL] UAV %s sending CHARGE_REQUEST msg_id=%s dst=%s next_hop=%s",
                uav_id_.c_str(), msg.msg_id.c_str(),
                msg.dst_id.c_str(), msg.next_hop_id.c_str());

    publishToBus(msg);
  }

  void publishFailureTraffic(const uav_msgs::msg::FailureEvent & failure)
  {
    if (!traffic_pub_) {
      return;
    }

    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = uav_id_ + "_failure_" + std::to_string(msg_counter_++);
    msg.src_id = uav_id_;
    msg.dst_id = default_dst_id_;
    msg.flow_type = 1;  // CONTROL
    msg.creation_time = failure.stamp;
    msg.hop_count = 0;
    msg.control_type = "FAILURE_EVENT";

    std::ostringstream oss;
    oss << static_cast<int>(failure.failure_type) << ","
        << failure.stamp.sec << "." << failure.stamp.nanosec << ","
        << failure.description;
    msg.payload = oss.str();

    if (role_ == 0) {
      msg.next_hop_id = my_ch_id_;
    } else {
      msg.next_hop_id = pickNextHop(default_dst_id_, resolveNextHop(default_dst_id_));
    }

    RCLCPP_INFO(this->get_logger(),
                "[TX CTRL] UAV %s sending FAILURE_EVENT msg_id=%s dst=%s next_hop=%s",
                uav_id_.c_str(), msg.msg_id.c_str(),
                msg.dst_id.c_str(), msg.next_hop_id.c_str());

    publishToBus(msg);
  }

  void handleChargeDecisionFromNetwork(const uav_msgs::msg::TrafficMessage::SharedPtr & msg)
  {
    // Only act if this UAV is the final destination
    if (msg->dst_id != uav_id_) {
      return;
    }

    // Must be a CHARGE_DECISION control message
    if (msg->flow_type != 1 || msg->control_type != "CHARGE_DECISION") {
      return;
    }

    auto now = this->now();

    waiting_for_charge_response_ = false;
    has_charge_slot_ = false;  // we start charging immediately

    if (!is_charging_ && battery_energy_ > 0.0f) {
      charge_departure_pose_ = pose_;
      charge_state_ = ChargeState::TO_UGV;

      if (resolveUgvPose(charge_target_pose_)) {
        charge_target_pose_.position.z = pose_.position.z;
        RCLCPP_INFO(this->get_logger(),
                    "UAV %s: received CHARGE_DECISION from %s (msg_id=%s). "
                    "Navigating to UGV at (%.1f, %.1f) before charging.",
                    uav_id_.c_str(), msg->src_id.c_str(), msg->msg_id.c_str(),
                    charge_target_pose_.position.x, charge_target_pose_.position.y);
      } else {
        RCLCPP_WARN(this->get_logger(),
                    "UAV %s: received CHARGE_DECISION but UGV pose unknown. Charging in place.",
                    uav_id_.c_str());
        beginChargingSession(now);
      }
    } else {
      RCLCPP_INFO(this->get_logger(),
                  "UAV %s: received CHARGE_DECISION but is already charging or dead.",
                  uav_id_.c_str());
    }

    // Optional: mark control message as delivered for metrics
    if (delivered_pub_) {
      delivered_pub_->publish(*msg);
    }
  }


  // ---------------- Heartbeat & traffic ----------------

  void publishHeartbeat()
  {
    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = uav_id_ + "_HEARTBEAT_" + std::to_string(msg_counter_++);
    msg.src_id = uav_id_;
    msg.dst_id = default_dst_id_;
    msg.flow_type = 1;  // CONTROL
    msg.creation_time = this->now();
    msg.hop_count = 0;
    msg.control_type = "HEARTBEAT";
    msg.payload = "";

    if (role_ == 0) {
      msg.next_hop_id = my_ch_id_;
    } else {
      msg.next_hop_id = pickNextHop(default_dst_id_, resolveNextHop(default_dst_id_));
    }

    publishToBus(msg);
  }

  void publishTraffic()
  {
    // Do not generate application traffic if disabled,
    // while charging, or if we're "dead".
    if (!auto_traffic_enabled_ || is_charging_ || battery_energy_ <= 0.0f || !deployment_received_) {
      return;
    }

    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = uav_id_ + "_" + std::to_string(msg_counter_++);
    msg.src_id = uav_id_;

    // Final destination: by default, sink_gateway (or whatever default_dst_id_ is)
    msg.dst_id = default_dst_id_;

    if (role_ == 0) { // MEMBER
      // First hop is my CH
      msg.next_hop_id = my_ch_id_;
    } else { // CH
      // CH sends using LADTR when enabled, otherwise next_hop_to_sink_
      msg.next_hop_id = pickNextHop(default_dst_id_, next_hop_to_sink_);
    }

    msg.flow_type = 0;       // TEXT
    msg.creation_time = this->now();
    msg.hop_count = 0;

    RCLCPP_INFO(this->get_logger(),
                "[TX] msg_id=%s src=%s dst=%s next_hop=%s",
                msg.msg_id.c_str(), msg.src_id.c_str(),
                msg.dst_id.c_str(), msg.next_hop_id.c_str());

    publishToBus(msg);
  }

  void beginChargingSession(const rclcpp::Time & now)
  {
    if (is_charging_ || battery_energy_ <= 0.0f) {
      return;
    }

    is_charging_ = true;
    charge_state_ = ChargeState::CHARGING;
    energy_at_charge_start_ = battery_energy_;
    charge_start_time_ = now;
    charge_end_time_ = now + rclcpp::Duration::from_seconds(charging_duration_sec_);

    RCLCPP_INFO(this->get_logger(),
                "UAV %s: starting charging session. ETA %.1f s.",
                uav_id_.c_str(), charging_duration_sec_);
  }

  void handleHelloMessage(const uav_msgs::msg::TrafficMessage::SharedPtr & msg)
  {
    (void)msg;
    // Neighbor tables are now driven by /fanet/status.
  }

  bool shouldBuffer(const uav_msgs::msg::TrafficMessage & msg) const
  {
    if (msg.flow_type == 0) {
      return buffer_enable_;
    }
    return ctrl_buffer_enable_;
  }

  void bufferTick()
  {
    auto now = this->now();
    buffer_manager_.tick(
      now,
      [this](uav_msgs::msg::TrafficMessage & msg) {
        if (msg.next_hop_id.empty() || !neighborReachable(msg.next_hop_id)) {
          return false;
        }
        return forwardMessage(msg);
      },
      [this](const uav_msgs::msg::TrafficMessage & msg, const std::string & reason) {
        publishDrop(msg.msg_id, reason);
      });
  }

  void neighborStatusCallback(const uav_msgs::msg::UavStatus::SharedPtr msg)
  {
    if (msg->uav_id == uav_id_) {
      return;
    }

    NeighborState state;
    state.last_seen = this->now();
    state.pose = msg->pose;
    state.velocity = msg->velocity;
    state.battery = msg->battery_level;
    state.role = msg->role;
    state.charging_state = msg->charging_state;
    state.intent_to_leave = msg->intent_to_leave;
    state.eta_to_leave_sec = msg->eta_to_leave_sec;
    state.comm_radius_m = msg->comm_radius_m;

    neighbors_[msg->uav_id] = state;

    if (msg->uav_id == ugv_id_) {
      ugv_pose_known_ = true;
      ugv_pose_ = msg->pose;
    }

    if (role_ == 0 && msg->uav_id == my_ch_id_) {
      updateChDeploymentReached(state);
    }
  }

  void pruneNeighbors()
  {
    if (hello_timeout_sec_ <= 0.0) {
      return;
    }

    const auto now = this->now();
    const auto timeout = rclcpp::Duration::from_seconds(hello_timeout_sec_);
    for (auto it = neighbors_.begin(); it != neighbors_.end(); ) {
      if ((now - it->second.last_seen) > timeout) {
        RCLCPP_WARN(this->get_logger(),
                    "UAV %s: neighbor %s timed out (no status for %.1f s)",
                    uav_id_.c_str(), it->first.c_str(),
                    (now - it->second.last_seen).seconds());

        dropRoutesThrough(it->first);
        it = neighbors_.erase(it);
      } else {
        ++it;
      }
    }
  }

  std::string statusStateString() const
  {
    if (charge_state_ == ChargeState::TO_UGV) {
      return "FLYING_TO_UGV";
    }
    if (charge_state_ == ChargeState::RETURNING) {
      return "RETURNING_FROM_UGV";
    }
    if (is_charging_) {
      return "CHARGING";
    }
    if (waiting_for_charge_response_) {
      return "WAITING_CHARGE_DECISION";
    }
    return "ACTIVE";
  }

  void publishStatusCh()
  {
    if (role_ != 1) {
      return;
    }

    std::string next_hop = pickNextHop(default_dst_id_, resolveNextHop(default_dst_id_));
    if (next_hop.empty()) {
      RCLCPP_WARN(this->get_logger(),
                  "UAV %s: cannot send STATUS_CH (no next hop to %s)",
                  uav_id_.c_str(), default_dst_id_.c_str());
      return;
    }

    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = uav_id_ + "_STATUS_CH_" + std::to_string(msg_counter_++);
    msg.src_id = uav_id_;
    msg.dst_id = default_dst_id_;
    msg.next_hop_id = next_hop;
    msg.flow_type = 1;
    msg.creation_time = this->now();
    msg.hop_count = 0;
    msg.ttl = status_ttl_;
    msg.control_type = "STATUS_CH";

    std::ostringstream oss;
    oss << pose_.position.x << ","
        << pose_.position.y << ","
        << battery_energy_ << ","
        << statusStateString();
    msg.payload = oss.str();

    publishToBus(msg);
  }

  bool resolveUgvPose(geometry_msgs::msg::Pose & pose)
  {
    if (ugv_pose_known_) {
      pose = ugv_pose_;
      return true;
    }

    auto it = neighbors_.find(ugv_id_);
    if (it != neighbors_.end()) {
      pose = it->second.pose;
      pose.orientation.w = 1.0;
      return true;
    }

    return false;
  }

  std::string roleString(uint8_t role) const
  {
    return role == 1 ? "CH" : "MEMBER";
  }

  void dropRoutesThrough(const std::string & neighbor_id)
  {
    for (auto it = routing_table_.begin(); it != routing_table_.end();) {
      if (it->second == neighbor_id) {
        RCLCPP_WARN(this->get_logger(),
                    "UAV %s: dropping route dst=%s via stale neighbor %s",
                    uav_id_.c_str(), it->first.c_str(), neighbor_id.c_str());
        it = routing_table_.erase(it);
      } else {
        ++it;
      }
    }
  }

  void handleDebugSendText(
    const std::shared_ptr<uav_msgs::srv::SendDebugText::Request> req,
    std::shared_ptr<uav_msgs::srv::SendDebugText::Response> res)
  {
    if (battery_energy_ <= 0.0f) {
      res->accepted = false;
      res->info = "UAV is dead (battery=0).";
      RCLCPP_WARN(this->get_logger(),
                  "UAV %s: debug send requested but battery is 0.", uav_id_.c_str());
      return;
    }

    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = uav_id_ + "_dbg_" + std::to_string(msg_counter_++);
    msg.src_id = uav_id_;
    msg.dst_id = req->dst_id;
    msg.flow_type = 0; // TEXT
    msg.creation_time = this->now();
    msg.hop_count = 0;

    // Use control_* to carry debug info
    msg.payload = req->text;
    msg.control_type = "DEBUG_TEXT:" + uav_id_;  // initial path is myself

    // ---- NEW ROUTING LOGIC ----
    if (role_ == 0) {
      // MEMBER: always send to its CH first
      msg.next_hop_id = my_ch_id_;
    } else {
      // CH:
      // If destination is one of my cluster members, send directly down to it.
      if (cluster_members_.find(msg.dst_id) != cluster_members_.end()) {
        msg.next_hop_id = msg.dst_id;
      } else {
        // Otherwise, use LADTR when available (fallback to backbone routing).
        msg.next_hop_id = pickNextHop(msg.dst_id, resolveNextHop(msg.dst_id));
      }
    }

    RCLCPP_INFO(this->get_logger(),
                "[DEBUG TX] msg_id=%s src=%s dst=%s next_hop=%s text=\"%s\"",
                msg.msg_id.c_str(), msg.src_id.c_str(), msg.dst_id.c_str(),
                msg.next_hop_id.c_str(), msg.payload.c_str());

    publishToBus(msg);

    res->accepted = true;
    res->info = "sent";
  }


  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
      // Dead UAV: ignore all traffic
    if (battery_energy_ <= 0.0f) {
      return;
    }
    rclcpp::Time rx_time = this->now();

    if (std::find(msg->recent_hops.begin(), msg->recent_hops.end(), uav_id_) != msg->recent_hops.end()) {
      publishDrop(msg->msg_id, "LOOP_DETECTED");
      return;
    }

    if ((msg->control_type == "START_MOBILITY" || msg->control_type == "MOTION_START") &&
        msg->dst_id == "broadcast") {
      start_mobility_received_ = true;
      last_pose_time_ = this->now();
      last_pose_ = pose_;
      RCLCPP_INFO(this->get_logger(),
                  "[MOB-START] %s received broadcast %s from %s",
                  uav_id_.c_str(), msg->control_type.c_str(), msg->src_id.c_str());
      return;
    }

    // If I'm not the next hop, ignore.
    if (msg->next_hop_id != uav_id_) {
      return;
    }

    if (msg->flow_type == 1 &&
        (msg->control_type == "DEPLOYMENT" || msg->control_type == "DEPLOYMENT_CMD")) {
      DeploymentInfo info;
      if (parseDeploymentPayload(msg->payload, info)) {
        updateClusterMetadata(info, msg->dst_id);
      }
    }

    // If I'm the final destination
    if (msg->dst_id == uav_id_) {

      // First, see if this is a control message for charging
      if (msg->flow_type == 1 && msg->control_type == "CHARGE_DECISION") {
        handleChargeDecisionFromNetwork(msg);
        return;
      }

      // receive start mobility / motion start barrier
      if (msg->flow_type == 1 &&
          (msg->dst_id == uav_id_ || msg->dst_id == "broadcast") &&
          (msg->control_type == "START_MOBILITY" || msg->control_type == "MOTION_START")) {

        start_mobility_received_ = true;
        // Reset timing so the first mobility step after the barrier uses the
        // configured dt instead of a huge elapsed time since deployment.
        last_pose_time_ = this->now();
        last_pose_ = pose_;
        RCLCPP_INFO(this->get_logger(),
                    "[MOB-START] %s received %s from %s",
                    uav_id_.c_str(), msg->control_type.c_str(), msg->src_id.c_str());
        return;
      }

      // Deployment via network
      if (msg->flow_type == 1 &&
          (msg->control_type == "DEPLOYMENT" || msg->control_type == "DEPLOYMENT_CMD")) {
        handleDeploymentFromNetwork(msg);
        return;
      }
      // Debug text messages are routed with a control_type prefix.
      if (msg->flow_type == 0 && msg->control_type.rfind("DEBUG_TEXT:", 0) == 0) {
        std::string path = msg->control_type.substr(std::string("DEBUG_TEXT:").size());
        std::string text = msg->payload;

        RCLCPP_INFO(this->get_logger(),
                    "[DEBUG RX] msg_id=%s src=%s dst=%s hops=%u path=%s text=\"%s\"",
                    msg->msg_id.c_str(), msg->src_id.c_str(), msg->dst_id.c_str(),
                    msg->hop_count, path.c_str(), text.c_str());

        publishDelivered(*msg, rx_time);
        return;
      }

      // Otherwise: normal data delivery
      RCLCPP_INFO(this->get_logger(),
                  "[RX] msg_id=%s delivered to %s (from %s, hop=%u)",
                  msg->msg_id.c_str(), uav_id_.c_str(),
                  msg->src_id.c_str(), msg->hop_count);

      publishDelivered(*msg, rx_time);
      return;
    }

    // I'm not final destination; if I'm a CH, I may forward
    if (role_ == 1) { // CH
      // Multi-hop DEPLOYMENT forwarding along CH backbone
      if (msg->flow_type == 1 &&
          (msg->control_type == "DEPLOYMENT" || msg->control_type == "DEPLOYMENT_CMD") &&
          msg->dst_id != uav_id_)
      {
          // Use backbone routing
          std::string next_hop = pickNextHop(msg->dst_id, resolveNextHop(msg->dst_id),
                                             &msg->recent_hops, &rx_time);

          if (next_hop.empty()) {
              RCLCPP_WARN(this->get_logger(),
                          "[FWD-DEPLOY] CH %s: no route to %s, dropping msg %s",
                          uav_id_.c_str(), msg->dst_id.c_str(), msg->msg_id.c_str());
              publishDrop(msg->msg_id, "NO_REACHABLE_NEIGHBOR");
              return;
          }

          msg->next_hop_id = next_hop;

          RCLCPP_INFO(this->get_logger(),
                      "[FWD-DEPLOY] CH %s forwarding DEPLOYMENT to %s via %s",
                      uav_id_.c_str(), msg->dst_id.c_str(), next_hop.c_str());

          publishToBus(*msg, true);
          return;
      }

      uav_msgs::msg::TrafficMessage fwd = *msg;

      if (fwd.control_type == "START_MOBILITY" || fwd.control_type == "MOTION_START") {
        fwd.next_hop_id = msg->dst_id;
      } else if (cluster_members_.find(msg->dst_id) != cluster_members_.end()) {
        // If the destination is one of my cluster members, send directly down to it.
        fwd.next_hop_id = msg->dst_id;
      } else {
        // Otherwise, use LADTR first, then fall back to backbone routing.
        if (location_aided_routing_) {
          auto ladtr = selectLadtrNextHop(msg->dst_id);
          if (ladtr && !ladtr->empty()) {
            fwd.next_hop_id = *ladtr;
            RCLCPP_INFO(this->get_logger(),
                        "[LADTR] %s forwarding msg_id=%s via %s toward %s",
                        uav_id_.c_str(), fwd.msg_id.c_str(), fwd.next_hop_id.c_str(),
                        msg->dst_id.c_str());
            publishToBus(fwd);
            return;
          }
        }

        fwd.next_hop_id = pickNextHop(msg->dst_id, resolveNextHop(msg->dst_id),
                                      &msg->recent_hops, &rx_time);
        if (fwd.next_hop_id.empty() && location_aided_routing_) {
          if (tryLocationAidedForward(fwd)) {
            return;
          }
          publishToBus(fwd, true);
          return;
          } else if (fwd.next_hop_id.empty()) {
            RCLCPP_WARN(this->get_logger(),
                        "[FWD] CH %s dropping msg_id=%s: no route to %s",
                        uav_id_.c_str(), fwd.msg_id.c_str(), msg->dst_id.c_str());
            publishDrop(fwd.msg_id, "NO_REACHABLE_NEIGHBOR");
            return;
          }
      }

      if (fwd.control_type.rfind("DEBUG_TEXT:", 0) == 0) { // starts with DEBUG_TEXT:
        fwd.control_type += "->" + uav_id_;
      }

      RCLCPP_INFO(this->get_logger(),
                  "[FWD] CH %s forwarding msg_id=%s src=%s dst=%s next_hop=%s hop=%u",
                  uav_id_.c_str(),
                  fwd.msg_id.c_str(), fwd.src_id.c_str(),
                  fwd.dst_id.c_str(), fwd.next_hop_id.c_str(),
                  fwd.hop_count);

      publishToBus(fwd, true);
    }

  }



  void clusterInfoCallback(const uav_msgs::msg::ClusterInfo::SharedPtr msg)
  {
    for (const auto & id : msg->member_ids) {
      if (id == uav_id_) {
        my_ch_id_ = msg->ch_id;
        cluster_id_ = msg->cluster_id;
        RCLCPP_INFO(this->get_logger(),
                    "UAV %s: cluster=%s CH=%s",
                    uav_id_.c_str(), cluster_id_.c_str(), my_ch_id_.c_str());
        break;
      }
    }
  }

  void deploymentCallback(const uav_msgs::msg::UavDeployment::SharedPtr msg)
  {
    if (!accept_direct_deployment_) {
      RCLCPP_INFO(this->get_logger(),
                  "UAV %s: ignoring direct deployment; using network DEPLOYMENT",
                  uav_id_.c_str());
      return;
    }
    uav_msgs::msg::TrafficMessage tm;
    tm.msg_id = "DEPLOYMENT_DIRECT_" + msg->uav_id + "_" + std::to_string(msg_counter_++);
    tm.src_id = "coverage_planner";
    tm.dst_id = msg->uav_id;
    tm.flow_type = 1;
    tm.creation_time = this->now();
    tm.hop_count = 0;
    tm.control_type = "DEPLOYMENT";
    std::ostringstream oss;
    oss << static_cast<int>(msg->role) << ","
        << msg->cluster_id << ","
        << msg->ch_id << ","
        << msg->target_pose.position.x << ","
        << msg->target_pose.position.y << ","
        << msg->target_pose.position.z << ","
        << msg->next_hop_to_sink << ","
        << msg->next_hop_to_ugv;
    tm.payload = oss.str();
    handleDeploymentFromNetwork(std::make_shared<uav_msgs::msg::TrafficMessage>(tm));
  }



  // ---------------- Members ----------------

  std::string uav_id_;
  uint8_t role_;
  std::string cluster_id_;
  std::string default_dst_id_;
  std::string my_ch_id_;
  std::string next_hop_to_sink_;
  // New: simple per-destination routing table for CHs
  std::unordered_map<std::string, std::string> routing_table_;
  // Resolve a destination into the next hop using cluster membership and routing table.
  std::string resolveNextHop(const std::string & dst) const
  {
      // CASE 1: destination is known as member of a CH
      auto itc = cluster_parent_.find(dst);
      if (itc != cluster_parent_.end()) {
          const std::string & parent = itc->second;
 
          // if CH == me → direct downlink
          if (parent == uav_id_) {
              return dst; 
          }

          // otherwise → route toward that CH
          auto it2 = routing_table_.find(parent);
          if (it2 != routing_table_.end()) {
              return it2->second;
          }

          // fallback: send directly to the parent CH
          return parent;
      }

      // CASE 2: explicit route exists
      auto it = routing_table_.find(dst);
      if (it != routing_table_.end()) {
          return it->second;
      }

      // CASE 3: default route toward sink
      return next_hop_to_sink_;
  }

  std::optional<std::string> selectLadtrNextHop(const std::string & dst) const
  {
    if (!location_aided_routing_) {
      return std::nullopt;
    }

    auto dst_pos_opt = lookupNodePosition(dst);
    if (!dst_pos_opt) {
      return std::nullopt;
    }

    geometry_msgs::msg::Point dst_pos = *dst_pos_opt;
    double self_dist = distance2d(pose_.position, dst_pos);
    auto neighbor = selectProgressNeighbor(dst_pos, self_dist);
    if (!neighbor) {
      return std::nullopt;
    }

    return neighbor->first;
  }

  std::string pickNextHop(const std::string & dst, const std::string & fallback,
                          const std::vector<std::string> * recent_hops = nullptr,
                          const rclcpp::Time * now_override = nullptr) const
  {
    rclcpp::Time now = now_override ? *now_override : this->now();
    auto greedy = selectGreedyNextHop(dst, recent_hops, now);
    if (greedy) {
      return *greedy;
    }

    auto ladtr = selectLadtrNextHop(dst);
    if (ladtr) {
      return *ladtr;
    }
    if (!fallback.empty()) {
      return fallback;
    }
    return resolveNextHop(dst);
  }

  std::optional<geometry_msgs::msg::Point> lookupNodePosition(const std::string & id) const
  {
    if (id == uav_id_) {
      return pose_.position;
    }

    auto it_dep = deployment_positions_.find(id);
    if (it_dep != deployment_positions_.end()) {
      return it_dep->second.position;
    }

    auto it_ch = ch_poses_.find(id);
    if (it_ch != ch_poses_.end()) {
      return it_ch->second.position;
    }

    auto it_nb = neighbors_.find(id);
    if (it_nb != neighbors_.end()) {
      return it_nb->second.pose.position;
    }

    return std::nullopt;
  }

  double distance2d(const geometry_msgs::msg::Point & a,
                    const geometry_msgs::msg::Point & b) const
  {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
  }

  // Local greedy next-hop selection with link expiration and charging penalty.
  std::optional<std::string> selectGreedyNextHop(
    const std::string & dst,
    const std::vector<std::string> * recent_hops,
    const rclcpp::Time & now) const
  {
    auto dst_pos_opt = lookupNodePosition(dst);
    geometry_msgs::msg::Point dst_pos;
    if (dst_pos_opt) {
      dst_pos = *dst_pos_opt;
    } else {
      // Fallback: try the configured sink/CH direction if the destination pose is unknown.
      auto sink_it = deployment_positions_.find(default_dst_id_);
      if (sink_it != deployment_positions_.end()) {
        dst_pos = sink_it->second.position;
        dst_pos_opt = dst_pos;
      } else if (!next_hop_to_sink_.empty()) {
        auto nh_it = neighbors_.find(next_hop_to_sink_);
        if (nh_it != neighbors_.end()) {
          dst_pos = nh_it->second.pose.position;
          dst_pos_opt = dst_pos;
        }
      }
    }

    if (!dst_pos_opt) {
      return std::nullopt;
    }

    const double self_comm_radius = static_cast<double>(service_radius_);

    auto link_expiration = [](const geometry_msgs::msg::Pose & self_pose,
                              const geometry_msgs::msg::Twist & self_vel,
                              const geometry_msgs::msg::Pose & nb_pose,
                              const geometry_msgs::msg::Twist & nb_vel,
                              double range) -> double
    {
      double dx = nb_pose.position.x - self_pose.position.x;
      double dy = nb_pose.position.y - self_pose.position.y;
      double dist = std::sqrt(dx * dx + dy * dy);
      if (dist >= range) {
        return 0.0;
      }
      double rel_vx = nb_vel.linear.x - self_vel.linear.x;
      double rel_vy = nb_vel.linear.y - self_vel.linear.y;
      double rel_speed_along = (rel_vx * dx + rel_vy * dy) / (dist + 1e-6);
      double rel_speed_mag = std::sqrt(rel_vx * rel_vx + rel_vy * rel_vy);
      double closing_speed = rel_speed_mag - rel_speed_along; // heuristic
      if (closing_speed <= 1e-3) {
        return std::numeric_limits<double>::infinity();
      }
      double remaining = range - dist;
      if (remaining < 0.0) remaining = 0.0;
      return remaining / closing_speed;
    };

    const double w_progress = 1.0;
    const double w_let = 0.5;
    const double w_charge = 5.0;
    const double penalty_intent = 2.0;
    const double leave_soon_sec = 15.0;

    double best_score = -std::numeric_limits<double>::infinity();
    std::optional<std::string> best_id;

    const double self_to_dst = distance2d(pose_.position, dst_pos);

    for (const auto & kv : neighbors_) {
      const auto & nb_id = kv.first;
      const auto & nb = kv.second;

      if (recent_hops &&
          std::find(recent_hops->begin(), recent_hops->end(), nb_id) != recent_hops->end()) {
        continue;
      }

      if (hello_timeout_sec_ > 0.0 &&
          (now - nb.last_seen) > rclcpp::Duration::from_seconds(hello_timeout_sec_)) {
        continue;
      }

      double nb_range = static_cast<double>(nb.comm_radius_m > 0.0f ? nb.comm_radius_m : service_radius_);
      double max_range = std::min(self_comm_radius, nb_range);

      double dist_nb = distance2d(pose_.position, nb.pose.position);
      if (dist_nb > max_range) {
        continue;
      }

      double nb_to_dst = distance2d(nb.pose.position, dst_pos);
      double progress = self_to_dst - nb_to_dst;
      if (progress <= 0.0) {
        continue;
      }

      double let = link_expiration(pose_, current_velocity_, nb.pose, nb.velocity, max_range);
      double charging_penalty = 0.0;
      if (nb.charging_state != 0) {
        charging_penalty = 3.0;
      } else if (nb.intent_to_leave && (nb.eta_to_leave_sec >= 0.0f && nb.eta_to_leave_sec < leave_soon_sec)) {
        charging_penalty = penalty_intent;
      }

      double score = w_progress * progress + w_let * let - w_charge * charging_penalty;

      if (score > best_score) {
        best_score = score;
        best_id = nb_id;
      }
    }

    return best_id;
  }

  std::optional<std::pair<std::string, double>> selectProgressNeighbor(
    const geometry_msgs::msg::Point & dst_pos,
    double self_dist) const
  {
    std::optional<std::pair<std::string, double>> best;
    double best_dist = self_dist;

    for (const auto & kv : neighbors_) {
      geometry_msgs::msg::Point npos = kv.second.pose.position;

      double d = distance2d(npos, dst_pos);
      if (d + location_progress_threshold_m_ < self_dist && d < best_dist) {
        best = std::make_pair(kv.first, d);
        best_dist = d;
      }
    }

    return best;
  }

  bool neighborReachable(const std::string & id, const rclcpp::Time & now) const
  {
    auto it = neighbors_.find(id);
    if (it == neighbors_.end()) {
      return false;
    }
    if (hello_timeout_sec_ > 0.0 &&
        (now - it->second.last_seen) > rclcpp::Duration::from_seconds(hello_timeout_sec_)) {
      return false;
    }
    double nb_range = static_cast<double>(it->second.comm_radius_m > 0.0f ? it->second.comm_radius_m : service_radius_);
    double max_range = std::min(static_cast<double>(service_radius_), nb_range);
    double dist = distance2d(pose_.position, it->second.pose.position);
    return dist <= max_range;
  }

  void stampForSend(uav_msgs::msg::TrafficMessage & msg, const std::string & next_hop)
  {
    auto now = this->now();
    msg.last_hop_id = uav_id_;
    msg.last_tx_time = now;
    msg.last_rx_time = now;
    msg.next_hop_id = next_hop;
    if (std::find(msg.recent_hops.begin(), msg.recent_hops.end(), uav_id_) == msg.recent_hops.end()) {
      msg.recent_hops.push_back(uav_id_);
    }
    if (msg.recent_hops.size() > max_recent_hops_) {
      msg.recent_hops.erase(msg.recent_hops.begin(),
                            msg.recent_hops.begin() + (msg.recent_hops.size() - max_recent_hops_));
    }
  }

  bool forwardMessage(uav_msgs::msg::TrafficMessage & msg)
  {
    if (msg.ttl != 0) {
      msg.ttl -= 1;
      if (msg.ttl == 0) {
        publishDrop(msg.msg_id, "TTL_EXPIRED");
        return false;
      }
    }
    msg.hop_count += 1;

    auto now = this->now();
    msg.last_rx_time = now;
    stampForSend(msg, msg.next_hop_id);
    traffic_pub_->publish(msg);
    return true;
  }

  bool publishToBus(uav_msgs::msg::TrafficMessage msg, bool allow_buffer = false)
  {
    if (!msg.next_hop_id.empty() && !neighborReachable(msg.next_hop_id, this->now())) {
      if (allow_buffer && shouldBuffer(msg)) {
        buffer_manager_.enqueue(msg, this->now(),
          [this](const uav_msgs::msg::TrafficMessage & buffered, const std::string & reason) {
            publishDrop(buffered.msg_id, reason);
          });
        return false;
      }
      publishDrop(msg.msg_id, "UNREACHABLE_NEXT_HOP");
      return false;
    }
    return forwardMessage(msg);
  }

  void publishDrop(const std::string & msg_id, const std::string & reason)
  {
    uav_msgs::msg::TrafficMessage drop;
    drop.msg_id = msg_id + "_DROP_" + uav_id_;
    drop.src_id = uav_id_;
    drop.dst_id = monitor_id_;
    drop.flow_type = 1;
    drop.control_type = "DROP";
    drop.drop_reason = reason;
    drop.payload = msg_id + "," + reason;
    drop.creation_time = this->now();
    drop.hop_count = 0;
    drop.ttl = 4;
    drop.next_hop_id = pickNextHop(monitor_id_, resolveNextHop(monitor_id_));
    if (drop.next_hop_id.empty()) {
      RCLCPP_WARN(this->get_logger(),
                  "[DROP] %s could not forward drop report for msg=%s (reason=%s): no route",
                  uav_id_.c_str(), msg_id.c_str(), reason.c_str());
      return;
    }
    publishToBus(drop);
  }

  void publishDelivered(const uav_msgs::msg::TrafficMessage & msg, const rclcpp::Time & rx_time)
  {
    if (!delivered_pub_) {
      return;
    }
    uav_msgs::msg::TrafficMessage delivered = msg;
    delivered.last_rx_time = rx_time;
    delivered.last_hop_id = uav_id_;
    delivered.last_tx_time = rx_time;
    if (std::find(delivered.recent_hops.begin(), delivered.recent_hops.end(), uav_id_) == delivered.recent_hops.end()) {
      delivered.recent_hops.push_back(uav_id_);
    }
    delivered.next_hop_id = "";
    delivered_pub_->publish(delivered);
  }

  bool tryLocationAidedForward(uav_msgs::msg::TrafficMessage & msg)
  {
    auto ladtr = selectLadtrNextHop(msg.dst_id);
    if (!ladtr) {
      return false;
    }

    auto dst_pos_opt = lookupNodePosition(msg.dst_id);
    if (!dst_pos_opt) {
      return false;
    }
    double self_dist = distance2d(pose_.position, *dst_pos_opt);
    auto neighbor = selectProgressNeighbor(*dst_pos_opt, self_dist);
    msg.next_hop_id = *ladtr;
    RCLCPP_INFO(this->get_logger(),
                "[LADTR] %s forwarding msg_id=%s toward %s via %s (d_self=%.1f d_next=%.1f)",
                uav_id_.c_str(), msg.msg_id.c_str(), msg.dst_id.c_str(),
                msg.next_hop_id.c_str(), self_dist,
                neighbor ? neighbor->second : -1.0);

    publishToBus(msg);
    return true;
  }

  void bufferForCarry(const uav_msgs::msg::TrafficMessage & msg)
  {
    if (!location_aided_routing_ || carry_buffer_limit_ == 0) {
      return;
    }

    for (const auto & entry : carry_buffer_) {
      if (entry.msg.msg_id == msg.msg_id) {
        return;
      }
    }

    if (carry_buffer_.size() >= carry_buffer_limit_) {
      const auto & dropped = carry_buffer_.front();
      RCLCPP_WARN(this->get_logger(),
                  "[LADTR] buffer full (%zu) dropping oldest msg_id=%s",
                  carry_buffer_.size(), dropped.msg.msg_id.c_str());
      carry_buffer_.pop_front();
    }

    BufferedMessage bm;
    bm.msg = msg;
    bm.buffered_time = this->now();
    carry_buffer_.push_back(bm);

    RCLCPP_INFO(this->get_logger(),
                "[LADTR] buffering msg_id=%s (dst=%s) carry_buffer=%zu",
                msg.msg_id.c_str(), msg.dst_id.c_str(), carry_buffer_.size());
  }

  void flushBufferedMessagesTimer()
  {
    if (!location_aided_routing_ || carry_buffer_.empty()) {
      return;
    }

    auto now = this->now();
    for (auto it = carry_buffer_.begin(); it != carry_buffer_.end();) {
      double age = (now - it->buffered_time).seconds();
      if (age > carry_ttl_sec_) {
        RCLCPP_WARN(this->get_logger(),
                    "[LADTR] dropping msg_id=%s after %.1f s in buffer",
                    it->msg.msg_id.c_str(), age);
        it = carry_buffer_.erase(it);
        continue;
      }

      uav_msgs::msg::TrafficMessage attempt = it->msg;
      std::string nh = pickNextHop(attempt.dst_id, resolveNextHop(attempt.dst_id));
      if (!nh.empty()) {
        attempt.next_hop_id = nh;
        RCLCPP_INFO(this->get_logger(),
                    "[LADTR] replaying msg_id=%s via next_hop=%s",
                    attempt.msg_id.c_str(), nh.c_str());
        publishToBus(attempt);
        it = carry_buffer_.erase(it);
        continue;
      }

      if (tryLocationAidedForward(attempt)) {
        it = carry_buffer_.erase(it);
        continue;
      }

      ++it;
    }
  }
  void initTaskMobility(const geometry_msgs::msg::Pose & ch_pose)
  {
    // Generate first round of task points around this CH
    generateTaskRound(ch_pose);
    current_task_index_ = 0;

    // Switch to task mobility phase (for members)
    if (role_ == 0 && mobility_enabled_) {
      mobility_phase_ = MobilityPhase::TASK_MOBILITY;
    }

    RCLCPP_INFO(this->get_logger(),
                "UAV %s: task mobility initialized in cluster of CH=%s",
                uav_id_.c_str(),
                my_ch_id_.c_str());
  }


  void generateTaskRound(const geometry_msgs::msg::Pose & ch_pose)
  {
    task_points_.clear();
    task_points_.reserve(tasks_per_round_);

    double cx = ch_pose.position.x;
    double cy = ch_pose.position.y;

    // Radius: use service_radius_ already parsed from parameters
    double R = static_cast<double>(service_radius_);

    // Random generator
    std::mt19937 rng(static_cast<unsigned>(
        this->now().nanoseconds() ^ std::hash<std::string>{}(uav_id_)));
    std::uniform_real_distribution<double> dist_u(0.0, 1.0);
    std::uniform_real_distribution<double> dist_theta(0.0, 2 * M_PI);

    for (int i = 0; i < tasks_per_round_; ++i) {
      double u = dist_u(rng);
      double r = R * std::sqrt(u);
      double theta = dist_theta(rng);

      geometry_msgs::msg::Point p;
      p.x = cx + r * std::cos(theta);
      p.y = cy + r * std::sin(theta);
      p.z = pose_.position.z; // Keep altitude constant for now

      task_points_.push_back(p);
    }

    current_task_index_ = 0;

    RCLCPP_INFO(this->get_logger(),
                "UAV %s: generated %d task points",
                uav_id_.c_str(), tasks_per_round_);
  }

  void mobilityStep()
  {
    // Do nothing until we actually have a deployment
    if (!mobility_enabled_ || !deployment_received_ || !start_mobility_received_ )
      return;


    if (battery_energy_ <= 0.0f || is_charging_)
      return;

    // Time step: guard against mixed / uninitialised time sources
    rclcpp::Time now = this->now();
    double dt = mobility_dt_sec_;  // default fallback

    if (last_pose_time_.nanoseconds() > 0 &&
        last_pose_time_.get_clock_type() == now.get_clock_type())
    {
      dt = (now - last_pose_time_).seconds();
      if (dt <= 0.0) {
        dt = mobility_dt_sec_;
      }
    }

    // Returns true when the target is reached within one step.
    auto stepTowards2D = [&](double gx, double gy) -> bool {
      double dx = gx - pose_.position.x;
      double dy = gy - pose_.position.y;
      double dist = std::sqrt(dx * dx + dy * dy);
      double max_step = uav_speed_mps_ * dt;

      if (dist <= 1e-3) {
        return true;
      }
      if (dist <= max_step) {
        pose_.position.x = gx;
        pose_.position.y = gy;
        return true;
      }

      double ux = dx / dist;
      double uy = dy / dist;
      pose_.position.x += ux * max_step;
      pose_.position.y += uy * max_step;
      return false;
    };

    bool handled_charge_motion = false;
    if (charge_state_ == ChargeState::TO_UGV) {
      handled_charge_motion = true;
      bool reached = stepTowards2D(
        charge_target_pose_.position.x,
        charge_target_pose_.position.y);

      if (reached) {
        pose_.position.x = charge_target_pose_.position.x;
        pose_.position.y = charge_target_pose_.position.y;
        beginChargingSession(now);
      }
    } else if (charge_state_ == ChargeState::RETURNING) {
      handled_charge_motion = true;
      bool reached = stepTowards2D(
        charge_departure_pose_.position.x,
        charge_departure_pose_.position.y);

      if (reached) {
        pose_.position.x = charge_departure_pose_.position.x;
        pose_.position.y = charge_departure_pose_.position.y;
        charge_state_ = ChargeState::IDLE;
        RCLCPP_INFO(this->get_logger(),
                    "UAV %s: returned from charging to (%.1f, %.1f).",
                    uav_id_.c_str(),
                    pose_.position.x,
                    pose_.position.y);
      }
    }

    bool held_by_ch = false;
    if (!handled_charge_motion) {
      switch (mobility_phase_) {
        case MobilityPhase::IDLE:
          // nothing to do
          break;

        case MobilityPhase::GO_TO_DEPLOYMENT:
        {
          if (role_ == 0 && !ch_deployment_reached_) {
            held_by_ch = true;
            break;
          }
          bool reached = stepTowards2D(
            deployment_goal_pose_.position.x,
            deployment_goal_pose_.position.y);

          if (reached) {
            pose_.position.x = deployment_goal_pose_.position.x;
            pose_.position.y = deployment_goal_pose_.position.y;
            pose_.position.z = deployment_goal_pose_.position.z;

            RCLCPP_INFO(this->get_logger(),
                        "UAV %s: reached deployment pose (%.1f, %.1f, %.1f)",
                        uav_id_.c_str(),
                        pose_.position.x,
                        pose_.position.y,
                        pose_.position.z);

            if (role_ == 1) {
              mobility_phase_ = MobilityPhase::IDLE;
            } else {
              // Member: start task mobility inside cluster
              auto it = ch_poses_.find(my_ch_id_);
              if (it != ch_poses_.end()) {
                initTaskMobility(it->second);
              }
              mobility_phase_ = MobilityPhase::TASK_MOBILITY;
            }
          }
          break;
        }

        case MobilityPhase::TASK_MOBILITY:
        {
          // Only members have task mobility
          if (role_ != 0)
            break;
          if (task_points_.empty())
            break;

          geometry_msgs::msg::Point & target = task_points_[current_task_index_];

          bool reached = stepTowards2D(target.x, target.y);

          if (reached) {
            pose_.position.x = target.x;
            pose_.position.y = target.y;

            current_task_index_++;
            if (current_task_index_ >= task_points_.size()) {
              // Regenerate tasks around CH
              auto it = ch_poses_.find(my_ch_id_);
              if (it != ch_poses_.end()) {
                generateTaskRound(it->second);
              } else {
                current_task_index_ = 0;
              }
            }
          }
          break;
        }
      }
    }

    if (held_by_ch) {
      syncPoseToCh();
    }

    // Update speed for drain model
    double dx_all = pose_.position.x - last_pose_.position.x;
    double dy_all = pose_.position.y - last_pose_.position.y;
    double d_all = std::sqrt(dx_all * dx_all + dy_all * dy_all);
    current_speed_mps_ = static_cast<float>(d_all / dt);
    if (dt > 0.0) {
      current_velocity_.linear.x = static_cast<float>(dx_all / dt);
      current_velocity_.linear.y = static_cast<float>(dy_all / dt);
      current_velocity_.linear.z = 0.0f;
    } else {
      current_velocity_.linear.x = 0.0f;
      current_velocity_.linear.y = 0.0f;
      current_velocity_.linear.z = 0.0f;
    }

    last_pose_ = pose_;
    last_pose_time_ = now;
  }


  float motionFactor(float speed_mps) const
  {
    if (speed_mps < 0.1f)
      return 1.0f;

    const float v_ref = 5.0f;
    const float k_speed = 0.3f;

    return 1.0f + k_speed * (speed_mps / v_ref);
  }
    
  float windFactor(float speed_mps) const
  {
    if (speed_mps <= 0.1f || current_wind_speed_mps_ <= 0.1f)
      return 1.0f;

    // UAV movement direction
    double vx = pose_.position.x - last_pose_.position.x;
    double vy = pose_.position.y - last_pose_.position.y;
    double vnorm = std::sqrt(vx*vx + vy*vy);
    if (vnorm < 1e-3)
      return 1.0f;
    vx /= vnorm;
    vy /= vnorm;

    // Wind direction vector (direction the wind blows TOWARD)
    double wx = std::cos(current_wind_dir_rad_);
    double wy = std::sin(current_wind_dir_rad_);

    // Opposite = headwind direction
    double wx_op = -wx;
    double wy_op = -wy;

    double cos_theta = vx * wx_op + vy * wy_op; // 1 = full headwind
    if (cos_theta < 0.0) cos_theta = 0.0;
    if (cos_theta > 1.0) cos_theta = 1.0;

    const float k_wind = 0.5f;
    const float v_ref = 10.0f;

    return 1.0f + k_wind * cos_theta * (current_wind_speed_mps_ / v_ref);
  }

  float rainFactor() const
  {
    // current_rain_intensity_ is mm/h
    const float k_rain = 0.4f;  // max +40% drain under heavy rain
    float r_mm = std::max(0.0f, current_rain_intensity_);

    // Normalize: 0..1 for 0–20 mm/h, clamp above
    float r_norm = r_mm / 20.0f;
    if (r_norm > 1.0f) r_norm = 1.0f;

    return 1.0f + k_rain * r_norm;
  }

  struct DeploymentInfo
  {
    int role = 0;
    std::string cluster_id;
    std::string ch_id;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    std::string next_sink;
    std::string next_ugv;
  };

  bool parseDeploymentPayload(const std::string & payload, DeploymentInfo & out) const
  {
    std::stringstream ss(payload);
    std::string token;

    if (!std::getline(ss, token, ',')) {
      return false;
    }
    out.role = std::stoi(token);
    std::getline(ss, out.cluster_id, ',');
    std::getline(ss, out.ch_id, ',');

    std::getline(ss, token, ',');
    out.x = std::stod(token);
    std::getline(ss, token, ',');
    out.y = std::stod(token);
    std::getline(ss, token, ',');
    out.z = std::stod(token);

    std::getline(ss, out.next_sink, ',');
    std::getline(ss, out.next_ugv, ',');
    return true;
  }

  void updateClusterMetadata(const DeploymentInfo & info, const std::string & dst_id)
  {
    if (info.role == 0) {
      cluster_parent_[dst_id] = info.ch_id;
      if (info.ch_id == uav_id_) {
        cluster_members_.insert(dst_id);
      }
    } else if (info.role == 1) {
      cluster_parent_[dst_id] = dst_id;
      geometry_msgs::msg::Pose ch_pose;
      ch_pose.position.x = info.x;
      ch_pose.position.y = info.y;
      ch_pose.position.z = info.z;
      ch_pose.orientation.w = 1.0;
      ch_poses_[dst_id] = ch_pose;
    }
  }

  void updateChDeploymentReached(const NeighborState & info)
  {
    if (role_ != 0 || ch_deployment_reached_) {
      return;
    }

    auto it = ch_poses_.find(my_ch_id_);
    if (it == ch_poses_.end()) {
      return;
    }

    const auto & target = it->second.position;
    double dx = info.pose.position.x - target.x;
    double dy = info.pose.position.y - target.y;
    double dist = std::sqrt(dx * dx + dy * dy);

    if (dist <= 1.0) {
      ch_deployment_reached_ = true;
      RCLCPP_INFO(this->get_logger(),
                  "UAV %s: CH %s reached deployment target (dist=%.2f).",
                  uav_id_.c_str(), my_ch_id_.c_str(), dist);
    }
  }

  void syncPoseToCh()
  {
    auto it = neighbors_.find(my_ch_id_);
    if (it == neighbors_.end()) {
      return;
    }

    pose_.position.x = it->second.pose.position.x;
    pose_.position.y = it->second.pose.position.y;
  }

  void handleDeploymentFromNetwork(
    const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    // msg->payload format:
    // "role,cluster_id,ch_id,x,y,z,next_sink,next_ugv"
    const std::string & payload = msg->payload;

    std::stringstream ss(payload);
    std::string token;

    int role_int = 0;
    std::string cluster_id, ch_id;
    double x = 0.0, y = 0.0, z = 0.0;
    std::string next_sink, next_ugv;

    // role
    if (!std::getline(ss, token, ',')) {
      RCLCPP_WARN(this->get_logger(),
                  "UAV %s: bad DEPLOYMENT payload \"%s\"",
                  uav_id_.c_str(), payload.c_str());
      return;
    }
    role_int = std::stoi(token);

    // cluster_id
    std::getline(ss, cluster_id, ',');
    // ch_id
    std::getline(ss, ch_id, ',');

    // x
    std::getline(ss, token, ',');
    x = std::stod(token);
    // y
    std::getline(ss, token, ',');
    y = std::stod(token);
    // z
    std::getline(ss, token, ',');
    z = std::stod(token);

    // next_sink
    std::getline(ss, next_sink, ',');
    // next_ugv (possibly empty)
    std::getline(ss, next_ugv, ',');
    geometry_msgs::msg::Pose deployed_pose;
    deployed_pose.position.x = x;
    deployed_pose.position.y = y;
    deployed_pose.position.z = z;
    deployed_pose.orientation.w = 1.0;
    deployment_positions_[msg->dst_id] = deployed_pose;

    bool is_self_deployment = (msg->dst_id == uav_id_);
    bool is_duplicate = false;
    if (is_self_deployment && deployment_received_) {
      double dx = deployment_goal_pose_.position.x - x;
      double dy = deployment_goal_pose_.position.y - y;
      double dz = deployment_goal_pose_.position.z - z;
      double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
      std::string expected_ch = (role_ == 0) ? my_ch_id_ : uav_id_;
      if (role_int == role_ &&
          cluster_id == cluster_id_ &&
          ch_id == expected_ch &&
          dist <= 1e-3) {
        is_duplicate = true;
      }
    }

    // Reset handshake state for new deployments
    if (!is_duplicate) {
      deployment_ack_sent_ = false;
      start_mobility_received_ = false;
      ch_deployment_reached_ = false;
    }

    // 1) Store CH pose for later task mobility
    if (role_int == 1) {
      geometry_msgs::msg::Pose ch_pose;
      ch_pose.position.x = x;
      ch_pose.position.y = y;
      ch_pose.position.z = z;
      ch_pose.orientation.w = 1.0;
      ch_poses_[msg->dst_id] = ch_pose;
    }

    // 2) If this deployment is not for us, we are done (we only forward in trafficCallback)
    if (!is_self_deployment) {
      return;
    }

    // 3) This deployment is for this UAV: update role/cluster info
    role_       = static_cast<uint8_t>(role_int);
    cluster_id_ = cluster_id;
    my_ch_id_   = (role_ == 0) ? ch_id : uav_id_;

    // Backbone routing info
    if (!next_sink.empty() && next_sink != "-") {
      next_hop_to_sink_ = next_sink;
    }
    if (role_ == 1 && !next_ugv.empty() && next_ugv != "-") {
      routing_table_[ugv_id_] = next_ugv;
    }

    // 4) Set deployment goal pose and start boot movement
    if (!is_duplicate) {
      deployment_goal_pose_.position.x = x;
      deployment_goal_pose_.position.y = y;
      deployment_goal_pose_.position.z = z;
      deployment_goal_pose_.orientation.w = 1.0;
    }

    deployment_received_ = true;
    sendDeploymentAck();

    // We start from whatever pose_ currently is (usually origin) and, after
    // receiving MOTION_START, fly to deployment_goal_pose_ in mobilityStep().
    if (!is_duplicate) {
      mobility_phase_ = MobilityPhase::GO_TO_DEPLOYMENT;
      if (!mobility_enabled_) {
        RCLCPP_WARN(this->get_logger(),
                    "UAV %s: mobility disabled, will wait for MOTION_START without teleporting.",
                    uav_id_.c_str());
      }

      last_pose_      = pose_;
      last_pose_time_ = this->now();
    }

    RCLCPP_INFO(this->get_logger(),
                "UAV %s: DEPLOYMENT via network -> role=%d cluster=%s ch=%s "
                "pos=(%.1f,%.1f,%.1f) next_sink=%s next_ugv=%s",
                uav_id_.c_str(),
                role_,
                cluster_id_.c_str(),
                my_ch_id_.c_str(),
                deployment_goal_pose_.position.x,
                deployment_goal_pose_.position.y,
                deployment_goal_pose_.position.z,
                next_hop_to_sink_.empty() ? "-" : next_hop_to_sink_.c_str(),
                routing_table_.count(ugv_id_) ? routing_table_[ugv_id_].c_str() : "-");
  }

  void sendDeploymentAck()
  {
    if (deployment_ack_sent_) {
      return;
    }
    if (!traffic_pub_) {
      return;
    }

    uav_msgs::msg::TrafficMessage ack;
    ack.msg_id = "DEP_ACK_" + uav_id_ + "_" +
                std::to_string(dep_ack_seq_++);
    ack.src_id = uav_id_;
    ack.dst_id = default_dst_id_;  // expected sink id (usually "sink_gateway")

    // First hop into the network
    if (role_ == 0) {
      // MEMBER: prefer its CH as the first hop toward the sink
      if (!my_ch_id_.empty() && neighbors_.count(my_ch_id_) > 0) {
        ack.next_hop_id = my_ch_id_;
      } else {
        // Fallback: resolve next hop to sink using any available route
        ack.next_hop_id = pickNextHop(default_dst_id_, resolveNextHop(default_dst_id_));
        if (ack.next_hop_id.empty()) {
          RCLCPP_WARN(this->get_logger(),
                      "UAV %s: cannot send DEPLOYMENT_ACK, no route to sink.",
                      uav_id_.c_str());
          return;
        }
        RCLCPP_WARN(this->get_logger(),
                    "UAV %s: CH %s unreachable, sending DEPLOYMENT_ACK via %s",
                    uav_id_.c_str(), my_ch_id_.c_str(), ack.next_hop_id.c_str());
      }
    } else {
      // CH (or other roles): use LADTR first, then routing towards sink
      ack.next_hop_id = pickNextHop(default_dst_id_, next_hop_to_sink_);
    }

    ack.flow_type = 1;
    ack.creation_time = this->now();
    ack.hop_count = 0;

    ack.control_type = "DEPLOYMENT_ACK";
    ack.payload = "";  // not needed for now

    publishToBus(ack);
    deployment_ack_sent_ = true;

    RCLCPP_INFO(this->get_logger(),
                "[DEP-ACK] %s sent DEPLOYMENT_ACK to sink via %s",
                uav_id_.c_str(), ack.next_hop_id.c_str());
  }

  // --- Mobility state machine ---
  enum class MobilityPhase {
    IDLE = 0,              // no movement
    GO_TO_DEPLOYMENT = 1,  // flying from origin to deployment pose
    TASK_MOBILITY = 2      // moving between task points in cluster
  };
  // Deployment / mobility barrier
  bool deployment_ack_sent_ = false;
  bool start_mobility_received_ = false;
  bool ch_deployment_reached_ = false;
  uint64_t dep_ack_seq_ = 0;


  MobilityPhase mobility_phase_;
  geometry_msgs::msg::Pose deployment_goal_pose_;

  std::string ugv_id_;   // logical id of the UGV in the network
  std::string monitor_id_;
  // For CHs: set of member UAV IDs in this cluster
  std::unordered_set<std::string> cluster_members_;
  std::unordered_map<std::string, std::string> cluster_parent_;

  geometry_msgs::msg::Pose pose_;
  float service_radius_;

  // Battery model
  float battery_capacity_;
  float battery_energy_;
  float battery_threshold_percent_;
  double charging_duration_sec_;
  bool reported_battery_dead_ = false;

  // === Mobility ===
  bool mobility_enabled_;
  double mobility_dt_sec_;
  double uav_speed_mps_;
  int tasks_per_round_;

  rclcpp::TimerBase::SharedPtr mobility_timer_;

  std::vector<geometry_msgs::msg::Point> task_points_;
  size_t current_task_index_ = 0;

  // CH poses so members can know their CH center
  std::unordered_map<std::string, geometry_msgs::msg::Pose> ch_poses_;
  std::unordered_map<std::string, geometry_msgs::msg::Pose> deployment_positions_;

  // Speed tracking
  geometry_msgs::msg::Pose last_pose_;
  rclcpp::Time last_pose_time_;
  geometry_msgs::msg::Twist current_velocity_;
  float current_speed_mps_ = 0.0f;

  // === Extended weather ===
  float current_wind_speed_mps_ = 0.0f;
  float current_wind_dir_rad_   = 0.0f;
  float current_rain_intensity_ = 0.0f;

  // Drain model
  float drain_rate_member_;
  float drain_rate_ch_;
  float current_temperature_c_;

  ChargeState charge_state_;
  geometry_msgs::msg::Pose charge_departure_pose_;
  geometry_msgs::msg::Pose charge_target_pose_;
  geometry_msgs::msg::Pose ugv_pose_;
  bool ugv_pose_known_;

  bool waiting_for_charge_response_;
  bool is_charging_;
  bool has_charge_slot_;
  rclcpp::Time charge_start_time_;
  rclcpp::Time charge_end_time_;
  float energy_at_charge_start_;

  uint64_t msg_counter_;
  // Auto traffic generation control
  bool auto_traffic_enabled_;
  // wait for deployment
  bool deployment_received_;
  bool accept_direct_deployment_;

  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_cb_handle_;

  rclcpp::Publisher<uav_msgs::msg::UavStatus>::SharedPtr status_pub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_pub_;
  rclcpp::Publisher<uav_msgs::msg::ChargeRequest>::SharedPtr charge_request_pub_;

  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Subscription<uav_msgs::msg::UavStatus>::SharedPtr neighbor_status_sub_;
  rclcpp::Subscription<uav_msgs::msg::ClusterInfo>::SharedPtr   cluster_sub_;
  rclcpp::Subscription<uav_msgs::msg::ChargeDecision>::SharedPtr charge_decision_sub_;
  rclcpp::Subscription<uav_msgs::msg::WeatherStatus>::SharedPtr  weather_sub_;
  rclcpp::Subscription<uav_msgs::msg::UavDeployment>::SharedPtr deployment_sub_;

  rclcpp::TimerBase::SharedPtr status_timer_;
  rclcpp::TimerBase::SharedPtr heartbeat_timer_;
  rclcpp::TimerBase::SharedPtr traffic_timer_;
  rclcpp::TimerBase::SharedPtr neighbor_timeout_timer_;
  rclcpp::TimerBase::SharedPtr buffer_retry_timer_;
  rclcpp::TimerBase::SharedPtr ch_status_timer_;
  rclcpp::TimerBase::SharedPtr ladtr_retry_timer_;

  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_pub_;
  rclcpp::Service<uav_msgs::srv::SendDebugText>::SharedPtr debug_service_;
  rclcpp::Publisher<uav_msgs::msg::FailureEvent>::SharedPtr failure_pub_;

  std::unordered_map<std::string, NeighborState> neighbors_;
  double hello_period_sec_ = 1.0;
  double hello_timeout_sec_ = 3.0;
  double status_period_sec_ = 5.0;
  uint32_t status_ttl_ = 0;
  bool location_aided_routing_ = false;
  double location_progress_threshold_m_ = 0.0;
  size_t carry_buffer_limit_ = 0;
  double carry_ttl_sec_ = 0.0;
  double carry_retry_period_sec_ = 1.0;
  bool buffer_enable_ = true;
  bool ctrl_buffer_enable_ = false;
  size_t buffer_max_msgs_ = 200;
  double buffer_ttl_sec_ = 90.0;
  double buffer_retry_period_sec_ = 1.0;
  size_t max_recent_hops_ = 5;
  BufferManager buffer_manager_;
  std::deque<BufferedMessage> carry_buffer_;

};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UavNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
