#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"

#include "uav_msgs/msg/uav_status.hpp"
#include "uav_msgs/msg/heartbeat.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/cluster_info.hpp"
#include "uav_msgs/msg/charge_decision.hpp"
#include "uav_msgs/msg/charge_request.hpp"
#include "uav_msgs/msg/weather_status.hpp"

#include "geometry_msgs/msg/pose.hpp"

#include "uav_msgs/srv/send_debug_text.hpp"
#include "uav_msgs/msg/failure_event.hpp"
#include "uav_msgs/msg/uav_deployment.hpp"




using namespace std::chrono_literals;

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

    // Base drain rates (energy units per second)
    double drain_member = this->declare_parameter<double>("drain_rate_member", 0.5);
    double drain_ch     = this->declare_parameter<double>("drain_rate_ch", 0.5);
    drain_rate_member_ = static_cast<float>(drain_member);
    drain_rate_ch_ = static_cast<float>(drain_ch);

    current_temperature_c_ = 22.0f;  // default comfy temp

    RCLCPP_INFO(this->get_logger(),
                "Starting UAV node id='%s', role=%u, cluster=%s, dst='%s'. "
                "Batt capacity=%.1f, threshold=%.1f%%, charge_duration=%.1f s",
                uav_id_.c_str(), role_, cluster_id_.c_str(), default_dst_id_.c_str(),
                battery_capacity_, battery_threshold_percent_, charging_duration_sec_);

    // ---- Publishers ----
    status_pub_ = this->create_publisher<uav_msgs::msg::UavStatus>(
      "/uav_fleet/status", 10);
    heartbeat_pub_ = this->create_publisher<uav_msgs::msg::Heartbeat>(
      "/uav_fleet/heartbeat", 10);
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

    // Dummy pose
    pose_.position.x = 0.0;
    pose_.position.y = 0.0;
    pose_.position.z = 10.0;
    pose_.orientation.w = 1.0;

    service_radius_ = 100.0f;


    RCLCPP_INFO(this->get_logger(),
                "UAV %s ready. Role=%u, CH capacity flag=%s",
                uav_id_.c_str(), role_, (role_ == 1 ? "YES" : "NO"));
  }

private:
  // ---------------- Weather ----------------

  void weatherCallback(const uav_msgs::msg::WeatherStatus::SharedPtr msg)
  {
    current_temperature_c_ = msg->temperature_c;
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

    if (is_charging_) {
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
    } else {
      // Not charging: drain battery based on role and temperature
      float base_drain = (role_ == 1) ? drain_rate_ch_ : drain_rate_member_;
      float temp_factor = temperatureFactor(current_temperature_c_);
      float drain_rate = base_drain * temp_factor;

      battery_energy_ -= drain_rate;
      if (battery_energy_ < 0.0f) {
        battery_energy_ = 0.0f;
      }
    }

    // Percentage
    float battery_percent = 0.0f;
    if (battery_capacity_ > 0.0f) {
      battery_percent = (battery_energy_ / battery_capacity_) * 100.0f;
    }

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
    }

    // If low and not waiting or scheduled, request a charge slot
    if (!is_charging_ && !waiting_for_charge_response_ && !has_charge_slot_ &&
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
    msg.energy_consumption_rate = 0.0f;
    msg.stamp = now;

    status_pub_->publish(msg);
  }


  void requestCharge(float battery_percent)
  {
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
    msg.msg_type = 3;           // CONTROL_ALERT
    msg.priority = 2;
    msg.size_bytes = 50;
    msg.creation_time = now;
    msg.hop_count = 0;

    // Let routing decide, as for any other dst:
    // - members will rely on their CH
    // - CHs will use routing_rules / next_hop_to_sink_
    msg.next_hop_id = resolveNextHop(msg.dst_id);

    // Optional control metadata
    msg.control_type = "CHARGE_REQUEST";
    // For now payload is empty; UGV will look up status from /uav_fleet/status

    RCLCPP_INFO(this->get_logger(),
                "[TX CTRL] UAV %s sending CHARGE_REQUEST msg_id=%s dst=%s next_hop=%s",
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

    // Must be a CHARGE_DECISION control alert
    if (msg->msg_type != 3 || msg->control_type != "CHARGE_DECISION") {
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
    uav_msgs::msg::Heartbeat hb;
    hb.uav_id = uav_id_;
    hb.stamp = this->now();
    heartbeat_pub_->publish(hb);
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

    msg.msg_type = 0;       // TEXT
    msg.priority = 0;
    msg.size_bytes = 100;
    msg.creation_time = this->now();
    msg.hop_count = 0;

    RCLCPP_INFO(this->get_logger(),
                "[TX] msg_id=%s src=%s dst=%s next_hop=%s",
                msg.msg_id.c_str(), msg.src_id.c_str(),
                msg.dst_id.c_str(), msg.next_hop_id.c_str());

    traffic_pub_->publish(msg);
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
    msg.msg_type = 0; // TEXT
    msg.priority = 1;
    msg.size_bytes = static_cast<uint32_t>(req->text.size());
    msg.creation_time = this->now();
    msg.hop_count = 0;

    // 用 control_* 来存调试信息
    msg.control_payload = req->text;
    msg.control_type = "DEBUG_TEXT:" + uav_id_;  // 初始路径就是自己

    // 正常走路由
    if (role_ == 0) { // MEMBER
      msg.next_hop_id = my_ch_id_;
    } else { // CH 也可以发
      msg.next_hop_id = resolveNextHop(msg.dst_id);
    }

    RCLCPP_INFO(this->get_logger(),
                "[DEBUG TX] msg_id=%s src=%s dst=%s next_hop=%s text=\"%s\"",
                msg.msg_id.c_str(), msg.src_id.c_str(), msg.dst_id.c_str(),
                msg.next_hop_id.c_str(), msg.control_payload.c_str());

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

    // If I'm not the next hop, ignore
    if (msg->next_hop_id != uav_id_) {
      return;
    }

    // If I'm the final destination
    if (msg->dst_id == uav_id_) {

      // First, see if this is a control message for charging
      if (msg->msg_type == 3 && msg->control_type == "CHARGE_DECISION") {
        handleChargeDecisionFromNetwork(msg);
        return;
      }

      // 如果是 debug 文本消息
      if (msg->msg_type == 0 && msg->control_type.rfind("DEBUG_TEXT:", 0) == 0) {
        std::string path = msg->control_type.substr(std::string("DEBUG_TEXT:").size());
        std::string text = msg->control_payload;

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
      uav_msgs::msg::TrafficMessage fwd = *msg;
      fwd.hop_count = msg->hop_count + 1;
      fwd.next_hop_id = resolveNextHop(msg->dst_id);

      if (fwd.control_type.rfind("DEBUG_TEXT:", 0) == 0) { // 以 DEBUG_TEXT: 开头
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
    if (msg->uav_id != uav_id_) {
      return;
    }

    // Instant teleport for now – later this becomes a motion goal.
    pose_ = msg->target_pose;

    // Apply role + cluster information from the planner
    role_ = msg->role;
    cluster_id_ = msg->cluster_id;

    if (role_ == 0) {
      // MEMBER: remember our CH
      my_ch_id_ = msg->ch_id;
    } else {
      // CH: its own CH id is itself
      my_ch_id_ = uav_id_;
    }

    // Apply backbone routing info if provided
    if (!msg->next_hop_to_sink.empty()) {
      next_hop_to_sink_ = msg->next_hop_to_sink;
    }

    deployment_received_ = true;

    RCLCPP_INFO(this->get_logger(),
                "UAV %s: deployment received. New pose=(%.1f, %.1f, %.1f), "
                "role=%u, cluster=%s, ch=%s, next_hop_to_sink=%s",
                uav_id_.c_str(),
                pose_.position.x,
                pose_.position.y,
                pose_.position.z,
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
  std::string resolveNextHop(const std::string & dst) const
  {
    // If we have an explicit rule for this destination, use it
    auto it = routing_table_.find(dst);
    if (it != routing_table_.end()) {
      return it->second;
    }
    // Fallback: use generic "towards sink" direction
    return next_hop_to_sink_;
  }
  std::string ugv_id_;   // logical id of the UGV in the network

  geometry_msgs::msg::Pose pose_;
  float service_radius_;

  // Battery model
  float battery_capacity_;
  float battery_energy_;
  float battery_threshold_percent_;
  double charging_duration_sec_;
  bool reported_battery_dead_ = false;


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
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_cb_handle_;

  rclcpp::Publisher<uav_msgs::msg::UavStatus>::SharedPtr status_pub_;
  rclcpp::Publisher<uav_msgs::msg::Heartbeat>::SharedPtr heartbeat_pub_;
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

  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_pub_;
  rclcpp::Service<uav_msgs::srv::SendDebugText>::SharedPtr debug_service_;
  rclcpp::Publisher<uav_msgs::msg::FailureEvent>::SharedPtr failure_pub_;

};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UavNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
