#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"

#include "uav_msgs/msg/uav_status.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/cluster_info.hpp"
#include "uav_msgs/msg/charge_decision.hpp"
#include "uav_msgs/msg/charge_request.hpp"
#include "uav_msgs/msg/weather_status.hpp"

#include "geometry_msgs/msg/pose.hpp"

#include "uav_msgs/srv/send_debug_text.hpp"
#include "uav_msgs/msg/failure_event.hpp"
#include "uav_msgs/msg/uav_deployment.hpp"
#include <unordered_set>
#include <random>
#include <algorithm>
#include <sstream>
#include <vector>




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
public:
  UavNode()
  : Node("uav_node"),
    msg_counter_(0),
    waiting_for_charge_response_(false),
    is_charging_(false),
    has_charge_slot_(false),
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
      "/uav_fleet/status", 10);
    traffic_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 10);
    delivered_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/network/traffic_delivered", 10);
    charge_request_pub_ = this->create_publisher<uav_msgs::msg::ChargeRequest>(
      "/uav_fleet/charge_requests", 10);
    failure_pub_ = this->create_publisher<uav_msgs::msg::FailureEvent>(
      "/uav_fleet/failure_events", 10);

    // ---- Subscribers ----
    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 10,
      std::bind(&UavNode::trafficCallback, this, std::placeholders::_1));

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

    auto hello_period = std::chrono::duration<double>(hello_period_sec_);
    hello_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(hello_period),
      std::bind(&UavNode::publishHello, this));
    neighbor_timeout_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(hello_period),
      std::bind(&UavNode::pruneNeighbors, this));

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

    service_radius_ = 400.0f;


    RCLCPP_INFO(this->get_logger(),
                "UAV %s ready. Role=%u, CH capacity flag=%s",
                uav_id_.c_str(), role_, (role_ == 1 ? "YES" : "NO"));
  }

private:
  struct NeighborInfo
  {
    std::string id;
    std::string role;
    double last_hello_time;
    double x;
    double y;
    double battery;
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
      msg.service_radius = service_radius_;
      msg.connected_users = 0;
      msg.traffic_load = 0.0f;
      msg.packet_loss_estimate = 0.0f;
      msg.energy_consumption_rate = 0.0f;
      msg.stamp = now;
      msg.backbone_active = backbone_active;

      status_pub_->publish(msg);
      return;
    }

    // ---- Normal behaviour AFTER deployment ----
    if (deployment_received_ && is_charging_) {
      // Charging: interpolate
      if (now >= charge_end_time_) {
        battery_energy_ = battery_capacity_;
        is_charging_ = false;
        RCLCPP_INFO(this->get_logger(),
                    "UAV %s: finished charging, battery full (%.1f).",
                    uav_id_.c_str(), battery_energy_);
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
    msg.service_radius = service_radius_;
    msg.connected_users = 0;
    msg.traffic_load = 0.0f;
    msg.packet_loss_estimate = 0.0f;
    msg.energy_consumption_rate = energy_consumption_rate;

    msg.stamp = now;
    msg.backbone_active = backbone_active;
    status_pub_->publish(msg);
  }


  void requestCharge(float battery_percent)
  {
    if (role_ == 0 && neighbor_table_.find(my_ch_id_) == neighbor_table_.end()) {
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
      msg.next_hop_id = resolveNextHop(ugv_id_);
    }

    // Optional control metadata to describe the control alert type.
    msg.control_type = "CHARGE_REQUEST";
    // For now payload is empty; UGV will look up status from /uav_fleet/status

    RCLCPP_INFO(this->get_logger(),
                "[TX CTRL] UAV %s sending CHARGE_REQUEST msg_id=%s dst=%s next_hop=%s",
                uav_id_.c_str(), msg.msg_id.c_str(),
                msg.dst_id.c_str(), msg.next_hop_id.c_str());

    traffic_pub_->publish(msg);
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
      msg.next_hop_id = resolveNextHop(default_dst_id_);
    }

    RCLCPP_INFO(this->get_logger(),
                "[TX CTRL] UAV %s sending FAILURE_EVENT msg_id=%s dst=%s next_hop=%s",
                uav_id_.c_str(), msg.msg_id.c_str(),
                msg.dst_id.c_str(), msg.next_hop_id.c_str());

    traffic_pub_->publish(msg);
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
      is_charging_ = true;
      energy_at_charge_start_ = battery_energy_;
      charge_start_time_ = now;
      charge_end_time_ = now + rclcpp::Duration::from_seconds(charging_duration_sec_);

      RCLCPP_INFO(this->get_logger(),
                  "UAV %s: received CHARGE_DECISION from %s (msg_id=%s). "
                  "Starting charging session now.",
                  uav_id_.c_str(), msg->src_id.c_str(), msg->msg_id.c_str());
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
      msg.next_hop_id = resolveNextHop(default_dst_id_);
    }

    traffic_pub_->publish(msg);
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
      // CH sends towards sink/backbone using next_hop_to_sink_
      msg.next_hop_id = next_hop_to_sink_;
    }

    msg.flow_type = 0;       // TEXT
    msg.creation_time = this->now();
    msg.hop_count = 0;

    RCLCPP_INFO(this->get_logger(),
                "[TX] msg_id=%s src=%s dst=%s next_hop=%s",
                msg.msg_id.c_str(), msg.src_id.c_str(),
                msg.dst_id.c_str(), msg.next_hop_id.c_str());

    traffic_pub_->publish(msg);
  }

  void publishHello()
  {
    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = uav_id_ + "_HELLO_" + std::to_string(msg_counter_++);
    msg.src_id = uav_id_;
    msg.dst_id = "broadcast";
    msg.next_hop_id = "";  // broadcast semantics

    msg.flow_type = 1;       // CONTROL
    msg.creation_time = this->now();
    msg.hop_count = 0;
    msg.ttl = 1;            // single-hop broadcast
    msg.control_type = "HELLO";

    std::ostringstream oss;
    oss << roleString(role_) << ","
        << pose_.position.x << ","
        << pose_.position.y << ","
        << battery_energy_;
    msg.payload = oss.str();

    traffic_pub_->publish(msg);
  }

  void handleHelloMessage(const uav_msgs::msg::TrafficMessage::SharedPtr & msg)
  {
    const auto now = this->now().seconds();
    NeighborInfo info;
    info.id = msg->src_id;
    info.role = "UNKNOWN";
    info.x = 0.0;
    info.y = 0.0;
    info.battery = 0.0;
    info.last_hello_time = now;

    auto parts = splitString(msg->payload, ',');
    if (parts.size() >= 4) {
      info.role = parts[0];
      info.x = std::stod(parts[1]);
      info.y = std::stod(parts[2]);
      info.battery = std::stod(parts[3]);
    }

    bool is_new = neighbor_table_.find(info.id) == neighbor_table_.end();
    neighbor_table_[info.id] = info;

    if (is_new) {
      RCLCPP_INFO(this->get_logger(),
                  "UAV %s: new neighbor detected via HELLO -> %s (%s)",
                  uav_id_.c_str(), info.id.c_str(), info.role.c_str());
    }

    if (role_ == 0 && msg->src_id == my_ch_id_) {
      updateChDeploymentReached(info);
    }
  }

  void pruneNeighbors()
  {
    if (hello_timeout_sec_ <= 0.0) {
      return;
    }

    const auto now = this->now().seconds();
    for (auto it = neighbor_table_.begin(); it != neighbor_table_.end(); ) {
      if (now - it->second.last_hello_time > hello_timeout_sec_) {
        RCLCPP_WARN(this->get_logger(),
                    "UAV %s: neighbor %s timed out (no HELLO for %.1f s)",
                    uav_id_.c_str(), it->first.c_str(), now - it->second.last_hello_time);

        dropRoutesThrough(it->first);
        it = neighbor_table_.erase(it);
      } else {
        ++it;
      }
    }
  }

  std::string statusStateString() const
  {
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

    std::string next_hop = resolveNextHop(default_dst_id_);
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

    traffic_pub_->publish(msg);
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
        // Otherwise, use normal backbone routing (sink/UGV/etc.)
        msg.next_hop_id = resolveNextHop(msg.dst_id);
      }
    }

    RCLCPP_INFO(this->get_logger(),
                "[DEBUG TX] msg_id=%s src=%s dst=%s next_hop=%s text=\"%s\"",
                msg.msg_id.c_str(), msg.src_id.c_str(), msg.dst_id.c_str(),
                msg.next_hop_id.c_str(), msg.payload.c_str());

    traffic_pub_->publish(msg);

    res->accepted = true;
    res->info = "sent";
  }


  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
      // Dead UAV: ignore all traffic
    if (battery_energy_ <= 0.0f) {
      return;
    }
    // Don't route anything while charging (you can relax this if you want later)
    if (is_charging_) {
      return;
    }

    if (msg->ttl != 0 && msg->hop_count >= msg->ttl) {
      RCLCPP_WARN(this->get_logger(),
                  "UAV %s: dropping msg_id=%s due to TTL expiry (hop=%u ttl=%u)",
                  uav_id_.c_str(), msg->msg_id.c_str(), msg->hop_count, msg->ttl);
      return;
    }

    if (msg->control_type == "HELLO") {
      handleHelloMessage(msg);
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

        delivered_pub_->publish(*msg);
        return;
      }

      // Otherwise: normal data delivery
      RCLCPP_INFO(this->get_logger(),
                  "[RX] msg_id=%s delivered to %s (from %s, hop=%u)",
                  msg->msg_id.c_str(), uav_id_.c_str(),
                  msg->src_id.c_str(), msg->hop_count);

      delivered_pub_->publish(*msg);
      return;
    }

    // I'm not final destination; if I'm a CH, I may forward
    if (role_ == 1) { // CH
      // Multi-hop DEPLOYMENT forwarding along CH backbone
      if (msg->flow_type == 1 &&
          (msg->control_type == "DEPLOYMENT" || msg->control_type == "DEPLOYMENT_CMD") &&
          msg->dst_id != uav_id_)
      {
          msg->hop_count++;

          if (msg->ttl != 0 && msg->hop_count >= msg->ttl) {
            RCLCPP_WARN(this->get_logger(),
                        "[FWD-DEPLOY] CH %s dropping %s due to TTL (hop=%u ttl=%u)",
                        uav_id_.c_str(), msg->msg_id.c_str(), msg->hop_count, msg->ttl);
            return;
          }

          // Use backbone routing
          std::string next_hop = resolveNextHop(msg->dst_id);

          if (next_hop.empty()) {
              RCLCPP_WARN(this->get_logger(),
                          "[FWD-DEPLOY] CH %s: no route to %s, dropping msg %s",
                          uav_id_.c_str(), msg->dst_id.c_str(), msg->msg_id.c_str());
              return;
          }

          msg->next_hop_id = next_hop;

          RCLCPP_INFO(this->get_logger(),
                      "[FWD-DEPLOY] CH %s forwarding DEPLOYMENT to %s via %s",
                      uav_id_.c_str(), msg->dst_id.c_str(), next_hop.c_str());

          traffic_pub_->publish(*msg);
          return;
      }

      uav_msgs::msg::TrafficMessage fwd = *msg;
      fwd.hop_count = msg->hop_count + 1;

      if (fwd.ttl != 0 && fwd.hop_count >= fwd.ttl) {
        RCLCPP_WARN(this->get_logger(),
                    "[FWD] CH %s dropping msg_id=%s due to TTL (hop=%u ttl=%u)",
                    uav_id_.c_str(), fwd.msg_id.c_str(), fwd.hop_count, fwd.ttl);
        return;
      }

      if (fwd.control_type == "START_MOBILITY" || fwd.control_type == "MOTION_START") {
        fwd.next_hop_id = msg->dst_id;
      } else if (cluster_members_.find(msg->dst_id) != cluster_members_.end()) {
        // If the destination is one of my cluster members, send directly down to it.
        fwd.next_hop_id = msg->dst_id;
      } else {
        // Otherwise, forward according to backbone routing (sink/UGV/other CHs)
        fwd.next_hop_id = resolveNextHop(msg->dst_id);
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

      traffic_pub_->publish(fwd);
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
  
    // 1) Every UAV learns the cluster parent of every UAV
    if (msg->role == 0) {
        // MEMBER
        cluster_parent_[msg->uav_id] = msg->ch_id;   // e.g., uav_3 -> uav_2
    } else if (msg->role == 1) {
        // CH
        cluster_parent_[msg->uav_id] = msg->uav_id;  // CH parent is itself
    }
    // If this deployment belongs to our cluster (we are the CH),
    //    record the member. This works even before we receive our own deployment.
    if (msg->role == 0 && msg->ch_id == uav_id_) {  // MEMBER assigned to this CH
      cluster_members_.insert(msg->uav_id);
      RCLCPP_INFO(this->get_logger(),
                  "UAV %s (CH): discovered member %s from deployment.",
                  uav_id_.c_str(), msg->uav_id.c_str());
    }

    // Store CH pose so members know their CH center later
    if (msg->role == 1) { // CH
      ch_poses_[msg->uav_id] = msg->target_pose;
    }
    // 2) If this deployment is not for us, we are done.
    if (msg->uav_id != uav_id_) {
      return;
    }
    // ignore direct deployment if flag is false
    if (!accept_direct_deployment_) {
      RCLCPP_INFO(this->get_logger(),
                  "UAV %s: ignoring direct deployment; using network DEPLOYMENT",
                  uav_id_.c_str());
      return;
    }
    deployment_ack_sent_ = false;
    start_mobility_received_ = false;
    ch_deployment_reached_ = false;
    // 3) This is our own deployment: apply role + cluster configuration
    role_ = msg->role;
    cluster_id_ = msg->cluster_id;

    if (role_ == 0) {
      // MEMBER: remember our CH
      my_ch_id_ = msg->ch_id;
    } else {
      // CH: its own CH id is itself
      my_ch_id_ = uav_id_;
      // When we become CH and receive a fresh deployment, reset the member set.
      cluster_members_.clear();
      RCLCPP_INFO(this->get_logger(),
                  "UAV %s: now CH of cluster %s. Member set cleared.",
                  uav_id_.c_str(), cluster_id_.c_str());
    }

    // Apply backbone routing info if provided
    if (!msg->next_hop_to_sink.empty()) {
      next_hop_to_sink_ = msg->next_hop_to_sink;
    }

    // If this UAV is a CH, also learn the path towards the UGV from the planner
    if (role_ == 1 && !msg->next_hop_to_ugv.empty()) {
      // Use the planner's next hop for the UGV destination
      routing_table_[ugv_id_] = msg->next_hop_to_ugv;
      RCLCPP_INFO(this->get_logger(),
                  "UAV %s (CH): routing to UGV '%s' via '%s' set from deployment.",
                  uav_id_.c_str(), ugv_id_.c_str(), msg->next_hop_to_ugv.c_str());
    }

    deployment_goal_pose_ = msg->target_pose;

    deployment_received_ = true;
    sendDeploymentAck();

    // Avoid a huge first dt() when the motion barrier is released.
    last_pose_time_ = this->now();
    last_pose_ = pose_;

    // Move toward deployment pose only after MOTION_START
    mobility_phase_ = MobilityPhase::GO_TO_DEPLOYMENT;
    if (!mobility_enabled_) {
      RCLCPP_WARN(this->get_logger(),
                  "UAV %s: mobility disabled, will wait for MOTION_START without teleporting.",
                  uav_id_.c_str());
    }

    RCLCPP_INFO(this->get_logger(),
                "UAV %s: deployment received. Target pose=(%.1f, %.1f, %.1f), "
                "role=%u, cluster=%s, ch=%s, next_hop_to_sink=%s",
                uav_id_.c_str(),
                deployment_goal_pose_.position.x,
                deployment_goal_pose_.position.y,
                deployment_goal_pose_.position.z,
                role_,
                cluster_id_.c_str(),
                my_ch_id_.c_str(),
                next_hop_to_sink_.c_str());
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

    bool held_by_ch = false;
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

    if (held_by_ch) {
      syncPoseToCh();
    }

    // Update speed for drain model
    double dx_all = pose_.position.x - last_pose_.position.x;
    double dy_all = pose_.position.y - last_pose_.position.y;
    double d_all = std::sqrt(dx_all * dx_all + dy_all * dy_all);
    current_speed_mps_ = static_cast<float>(d_all / dt);

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

  void updateChDeploymentReached(const NeighborInfo & info)
  {
    if (role_ != 0 || ch_deployment_reached_) {
      return;
    }

    auto it = ch_poses_.find(my_ch_id_);
    if (it == ch_poses_.end()) {
      return;
    }

    const auto & target = it->second.position;
    double dx = info.x - target.x;
    double dy = info.y - target.y;
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
    auto it = neighbor_table_.find(my_ch_id_);
    if (it == neighbor_table_.end()) {
      return;
    }

    pose_.position.x = it->second.x;
    pose_.position.y = it->second.y;
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

    // Reset handshake state for new deployments
    deployment_ack_sent_ = false;
    start_mobility_received_ = false;
    ch_deployment_reached_ = false;

    // 1) Store CH pose for later task mobility
    if (role_int == 1) {
      geometry_msgs::msg::Pose ch_pose;
      ch_pose.position.x = x;
      ch_pose.position.y = y;
      ch_pose.position.z = z;
      ch_pose.orientation.w = 1.0;
      ch_poses_[ch_id] = ch_pose;
    }

    // 2) If this deployment is not for us, we are done (we only forward in trafficCallback)
    if (msg->dst_id != uav_id_) {
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
    deployment_goal_pose_.position.x = x;
    deployment_goal_pose_.position.y = y;
    deployment_goal_pose_.position.z = z;
    deployment_goal_pose_.orientation.w = 1.0;

    deployment_received_ = true;
    sendDeploymentAck();

    // We start from whatever pose_ currently is (usually origin) and, after
    // receiving MOTION_START, fly to deployment_goal_pose_ in mobilityStep().
    mobility_phase_ = MobilityPhase::GO_TO_DEPLOYMENT;
    if (!mobility_enabled_) {
      RCLCPP_WARN(this->get_logger(),
                  "UAV %s: mobility disabled, will wait for MOTION_START without teleporting.",
                  uav_id_.c_str());
    }

    last_pose_      = pose_;
    last_pose_time_ = this->now();

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
    ack.dst_id = default_dst_id_;  // this is your sink id (usually "sink_gateway")

    // First hop into the network
    if (role_ == 0) {
      // MEMBER: go to its CH
      ack.next_hop_id = my_ch_id_;
    } else {
      // CH (or other roles): use routing towards sink if known
      if (!next_hop_to_sink_.empty()) {
        ack.next_hop_id = next_hop_to_sink_;
      } else {
        ack.next_hop_id = default_dst_id_;  // direct if in range
      }
    }

    ack.flow_type = 1;
    ack.creation_time = this->now();
    ack.hop_count = 0;

    ack.control_type = "DEPLOYMENT_ACK";
    ack.payload = "";  // not needed for now

    traffic_pub_->publish(ack);
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

  // Speed tracking
  geometry_msgs::msg::Pose last_pose_;
  rclcpp::Time last_pose_time_;
  float current_speed_mps_ = 0.0f;

  // === Extended weather ===
  float current_wind_speed_mps_ = 0.0f;
  float current_wind_dir_rad_   = 0.0f;
  float current_rain_intensity_ = 0.0f;

  // Drain model
  float drain_rate_member_;
  float drain_rate_ch_;
  float current_temperature_c_;

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
  rclcpp::Subscription<uav_msgs::msg::ClusterInfo>::SharedPtr   cluster_sub_;
  rclcpp::Subscription<uav_msgs::msg::ChargeDecision>::SharedPtr charge_decision_sub_;
  rclcpp::Subscription<uav_msgs::msg::WeatherStatus>::SharedPtr  weather_sub_;
  rclcpp::Subscription<uav_msgs::msg::UavDeployment>::SharedPtr deployment_sub_;

  rclcpp::TimerBase::SharedPtr status_timer_;
  rclcpp::TimerBase::SharedPtr heartbeat_timer_;
  rclcpp::TimerBase::SharedPtr traffic_timer_;
  rclcpp::TimerBase::SharedPtr hello_timer_;
  rclcpp::TimerBase::SharedPtr neighbor_timeout_timer_;
  rclcpp::TimerBase::SharedPtr ch_status_timer_;

  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_pub_;
  rclcpp::Service<uav_msgs::srv::SendDebugText>::SharedPtr debug_service_;
  rclcpp::Publisher<uav_msgs::msg::FailureEvent>::SharedPtr failure_pub_;

  std::unordered_map<std::string, NeighborInfo> neighbor_table_;
  double hello_period_sec_ = 1.0;
  double hello_timeout_sec_ = 3.0;
  double status_period_sec_ = 5.0;
  uint32_t status_ttl_ = 0;

};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UavNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
