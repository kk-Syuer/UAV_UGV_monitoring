#include <cmath>
#include <memory>
#include <string>
#include <deque>
#include <list>
#include <exception>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sstream>
#include <algorithm>
#include <utility>
#include <iomanip>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/charge_decision.hpp"
#include "uav_msgs/msg/uav_status.hpp"
#include "uav_msgs/msg/uav_deployment.hpp"
#include "uav_msgs/msg/routing_table.hpp"
#include "uav_msgs/msg/failure_event.hpp"
#include "std_msgs/msg/string.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "geometry_msgs/msg/twist.hpp"

using namespace std::chrono_literals;

// Compute number of charging docks to reach target utilization.
int compute_required_charging_spots(
    int    num_ch,
    int    num_members,
    double flight_time_ch_min,
    double charge_time_ch_min,
    double flight_time_mem_min,
    double charge_time_mem_min,
    double target_utilization)
{
  if (num_ch == 0 && num_members == 0) {
    return 0;
  }

  double load_ch = 0.0;
  if (flight_time_ch_min + charge_time_ch_min > 0.0) {
    load_ch = charge_time_ch_min / (flight_time_ch_min + charge_time_ch_min);
  }

  double load_mem = 0.0;
  if (flight_time_mem_min + charge_time_mem_min > 0.0) {
    load_mem = charge_time_mem_min / (flight_time_mem_min + charge_time_mem_min);
  }

  double total_load = num_ch * load_ch + num_members * load_mem;

  double effective_target = target_utilization <= 0.0 ? 0.9 : target_utilization;

  double raw_spots = total_load / effective_target;

  int num_spots = static_cast<int>(std::ceil(raw_spots));
  if (num_spots < 1 && total_load > 0.0) {
    num_spots = 1;
  }

  return num_spots;
}

// UGV charger node schedules charge sessions and routes control messages.
class UgvChargerNode : public rclcpp::Node
{
public:
  UgvChargerNode()
  : Node("ugv_charger_node"),
    max_parallel_spots_(1)
  {
    ugv_id_ = this->declare_parameter<std::string>("ugv_id", "ugv");

    charging_duration_sec_ =
      this->declare_parameter<double>("charging_duration_sec", 20.0);

    std::string policy_name =
      this->declare_parameter<std::string>("charging_policy", "fcfs");
    target_utilization_ = this->declare_parameter<double>("target_utilization", 0.8);
    flight_time_ch_min_ = this->declare_parameter<double>("flight_time_ch_min", 90.0);
    charge_time_ch_min_ = this->declare_parameter<double>("charge_time_ch_min", 30.0);
    flight_time_mem_min_ = this->declare_parameter<double>("flight_time_mem_min", 45.0);
    charge_time_mem_min_ = this->declare_parameter<double>("charge_time_mem_min", 20.0);
    // Track deployments to learn CH/member relations for routing.
    deployment_sub_ = this->create_subscription<uav_msgs::msg::UavDeployment>(
      "/coverage_planner/deployment", 10,
      std::bind(&UgvChargerNode::deploymentCallback, this, std::placeholders::_1));
    sink_id_ = this->declare_parameter<std::string>("sink_id", "sink_gateway");

    mobility_enabled_ = this->declare_parameter<bool>("mobility_enabled", true);
    mobility_dt_sec_  = this->declare_parameter<double>("mobility_dt_sec", 0.2);
    ugv_speed_mps_    = this->declare_parameter<double>("ugv_speed_mps", 15.3);
    comm_radius_m_    = this->declare_parameter<double>("comm_radius_m", 400.0);
    status_period_sec_ = this->declare_parameter<double>("status_period_sec", 1.0);
    neighbor_timeout_sec_ = this->declare_parameter<double>("neighbor_timeout_sec", 3.0);
    charge_decision_retry_sec_ = this->declare_parameter<double>("charge_decision_retry_sec", 2.0);
    charge_decision_max_retries_ = this->declare_parameter<int>("charge_decision_max_retries", 5);
    ack_retry_period_sec_ = this->declare_parameter<double>("ack_retry_period_sec", 0.5);
    control_dedup_cache_size_ = static_cast<size_t>(
      this->declare_parameter<int>("control_dedup_cache_size", 200));
    charge_request_battery_gate_percent_ =
      this->declare_parameter<double>("battery_threshold", 30.0);
    charge_request_status_stale_sec_ =
      this->declare_parameter<double>("charge_request_status_stale_sec", 3.0);

    if (policy_name == "role_priority") {
      policy_ = Policy::ROLE_PRIORITY;
    } else if (policy_name == "edf") {
      policy_ = Policy::EDF;
    } else if (policy_name == "dynamic") {
      policy_ = Policy::DYNAMIC_SCORE;
    } else {
      policy_ = Policy::FCFS;
      policy_name = "fcfs";
    }
    policy_name_ = policy_name;
    uplink_ch_id_ =
      this->declare_parameter<std::string>("uplink_ch_id", "uav_3");

    // Parameters for EDF: approximate drain of battery percentage per second
    drain_percent_member_ = this->declare_parameter<double>("drain_percent_member", 0.5); // %/s
    drain_percent_ch_     = this->declare_parameter<double>("drain_percent_ch", 0.25);    // %/s

    // Parameters for dynamic score
    w_role_ = this->declare_parameter<double>("w_role", 5.0);
    w_batt_ = this->declare_parameter<double>("w_batt", 1.0);
    w_wait_ = this->declare_parameter<double>("w_wait", 0.1);
    debug_scheduler_candidates_ =
      this->declare_parameter<bool>("debug_scheduler_candidates", false);

    // Charge decisions are published both directly and via routed control messages.
    charge_decision_pub_ = this->create_publisher<uav_msgs::msg::ChargeDecision>(
      "/ugv/charge_decisions", 10);
    control_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/fanet/network_bus_raw", 100);
    status_pub_ = this->create_publisher<uav_msgs::msg::UavStatus>(
      "/fanet/status", 50);
    routing_event_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/fanet/routing_event", 10);
    hello_period_sec_ = this->declare_parameter<double>("hello_period_sec", 1.0);
    RCLCPP_INFO(this->get_logger(),
                "UGV charger started. id='%s', policy='%s', charging_duration=%.1f s, uplink_ch_id='%s', charge_gate=%.1f%%",
                ugv_id_.c_str(), policy_name_.c_str(),
                charging_duration_sec_, uplink_ch_id_.c_str(),
                charge_request_battery_gate_percent_);
    delivered_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/fanet/delivered", 100);

    // NEW: subscribe to network control messages
    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/fanet/network_bus", 100,
      std::bind(&UgvChargerNode::trafficCallback, this, std::placeholders::_1));

    // NEW: subscribe to UAV status to know battery & role
    status_sub_ = this->create_subscription<uav_msgs::msg::UavStatus>(
      "/fanet/status", 100,
      std::bind(&UgvChargerNode::statusCallback, this, std::placeholders::_1));
    failure_sub_ = this->create_subscription<uav_msgs::msg::FailureEvent>(
      "/uav_fleet/failure_events", 100,
      std::bind(&UgvChargerNode::failureCallback, this, std::placeholders::_1));
    routing_table_sub_ = this->create_subscription<uav_msgs::msg::RoutingTable>(
      "/fanet/routing_table", 20,
      std::bind(&UgvChargerNode::routingTableCallback, this, std::placeholders::_1));
    queue_event_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/ugv/queue_events", 50);
    charging_snapshot_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/ugv/charging_snapshot", 10);

    // Scheduler loop assigns charging slots at a steady cadence.
    scheduler_timer_ = this->create_wall_timer(
      500ms, std::bind(&UgvChargerNode::schedulerLoop, this));

    // HELLO beacons removed; discovery happens via UavStatus on /fanet/status.
    auto status_period = std::chrono::duration<double>(status_period_sec_);
    status_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(status_period),
      std::bind(&UgvChargerNode::publishStatus, this));

    auto ack_retry = std::chrono::duration<double>(ack_retry_period_sec_);
    ack_retry_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(ack_retry),
      std::bind(&UgvChargerNode::resendPendingAcks, this));

    charging_snapshot_timer_ = this->create_wall_timer(
      1s, std::bind(&UgvChargerNode::publishChargingSnapshot, this));

    if (neighbor_timeout_sec_ > 0.0) {
      auto timeout_period = std::chrono::duration<double>(neighbor_timeout_sec_);
      neighbor_timeout_timer_ = this->create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(timeout_period),
        std::bind(&UgvChargerNode::pruneNeighbors, this));
    }

    ugv_pose_.position.x = 0.0;
    ugv_pose_.position.y = 0.0;
    ugv_pose_.position.z = 0.0;
    ugv_pose_.orientation.w = 1.0;
    deployment_goal_pose_ = ugv_pose_;
    last_pose_time_ = this->now();

    if (mobility_enabled_) {
      auto dt = std::chrono::duration<double>(mobility_dt_sec_);
      mobility_timer_ = this->create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(dt),
        std::bind(&UgvChargerNode::mobilityStep, this));
    }

    recomputeChargingSpots();

    RCLCPP_INFO(this->get_logger(),
                "UGV charger started. id='%s', policy='%s', charging_duration=%.1f s, charge_gate=%.1f%%",
                ugv_id_.c_str(), policy_name_.c_str(), charging_duration_sec_,
                charge_request_battery_gate_percent_);
  }

private:
  enum class Policy
  {
    FCFS,
    ROLE_PRIORITY,
    EDF,
    DYNAMIC_SCORE
  };

  struct QueueEntry
  {
    std::string uav_id;
    uint8_t role;         // 0=member,1=CH
    float battery_level;  // percent at request time
    rclcpp::Time request_time;
    std::string request_msg_id;
  };

  struct UavInfo
  {
    uint8_t role = 0;
    float battery_level = 0.0f;
    bool backbone_active = true; // whether this UAV is usable as backbone

    rclcpp::Time last_seen;
    geometry_msgs::msg::Pose pose;
    float comm_radius_m = 0.0f;

    uint8_t charging_state = 0;
    bool intent_to_leave = false;
    float eta_to_leave_sec = -1.0f;
  };

  struct PendingAck
  {
    uav_msgs::msg::TrafficMessage msg;
    rclcpp::Time last_send_time;
    int attempts = 0;
    int max_retries = 0;
    double retry_interval_sec = 1.0;
  };

  struct RejectedRequest
  {
    std::string uav_id;
    rclcpp::Time time;
    std::string reason;
  };

  struct LastRequestState
  {
    bool valid = false;
    std::string uav_id;
    std::string msg_id;
    rclcpp::Time time;
    uint8_t role = 0;
    float battery = 0.0f;
    std::string status;
  };

  struct LastDecisionState
  {
    bool valid = false;
    std::string uav_id;
    bool accepted = false;
    rclcpp::Time time;
    rclcpp::Time slot_start;
    rclcpp::Time slot_end;
    std::string policy;
    uint8_t priority = 0;
    std::string msg_id;
  };

  // ------------- Callbacks -------------

  // Cache latest UAV status for policy scoring and dock sizing.
  void statusCallback(const uav_msgs::msg::UavStatus::SharedPtr msg)
  {
    if (msg->uav_id == ugv_id_) {
      return;
    }

    auto now = this->now();

    auto & info = uav_status_[msg->uav_id];
    info.role = msg->role;
    info.battery_level = msg->battery_level;
    info.backbone_active = msg->backbone_active;
    info.charging_state = msg->charging_state;
    info.intent_to_leave = msg->intent_to_leave;
    info.eta_to_leave_sec = msg->eta_to_leave_sec;

    info.last_seen = now;
    info.pose = msg->pose;
    info.comm_radius_m = static_cast<float>(comm_radius_m_);
    recomputeChargingSpots();
  }

  void routingTableCallback(const uav_msgs::msg::RoutingTable::SharedPtr msg)
  {
    if (msg->node_id != ugv_id_) {
      return;
    }

    routing_table_.clear();
    const size_t count = std::min(msg->destinations.size(), msg->next_hops.size());
    for (size_t i = 0; i < count; ++i) {
      if (!msg->destinations[i].empty() && !msg->next_hops[i].empty()) {
        routing_table_[msg->destinations[i]] = msg->next_hops[i];
      }
    }

    RCLCPP_INFO(this->get_logger(),
                "UGV %s: routing table updated (%zu entries)",
                ugv_id_.c_str(), routing_table_.size());
  }

  void publishQueueEvent(const std::string & event,
                         const std::string & uav_id,
                         const std::string & reason,
                         size_t queue_size_after)
  {
    if (!queue_event_pub_) {
      return;
    }
    std_msgs::msg::String msg;
    std::ostringstream oss;
    oss << "event=" << event
        << " uav_id=" << uav_id
        << " reason=" << reason
        << " queue_size=" << queue_size_after
        << " stamp=" << this->now().seconds();
    msg.data = oss.str();
    queue_event_pub_->publish(msg);
  }

  // Remove stale charge requests when UAVs are declared dead.
  void failureCallback(const uav_msgs::msg::FailureEvent::SharedPtr msg)
  {
    if (msg->uav_id.empty()) {
      return;
    }

    if (msg->failure_type != 1) {
      return;
    }

    dead_uavs_.insert(msg->uav_id);

    size_t removed = 0;
    for (auto it = queue_.begin(); it != queue_.end(); ) {
      if (it->uav_id == msg->uav_id) {
        it = queue_.erase(it);
        ++removed;
      } else {
        ++it;
      }
    }

    if (removed > 0) {
      RCLCPP_INFO(this->get_logger(),
                  "UGV: cancelled %zu queued CHARGE_REQUEST(s) for %s due to BATTERY_DEAD",
                  removed, msg->uav_id.c_str());
      publishQueueEvent("QUEUE_CANCEL", msg->uav_id, "BATTERY_DEAD", queue_.size());
    }
  }

  std::string resolveNextHop(const std::string & dst) const
  {
    auto it = routing_table_.find(dst);
    if (it != routing_table_.end()) {
      return it->second;
    }
    return "";
  }

  void publishRoutingEvent(const std::string & reason,
                           const std::string & dst_id,
                           const std::string & next_hop) const
  {
    if (!routing_event_pub_) {
      return;
    }
    std_msgs::msg::String msg;
    std::ostringstream oss;
    oss << "node=" << ugv_id_
        << " dst=" << dst_id
        << " next_hop=" << (next_hop.empty() ? "UNREACHABLE" : next_hop)
        << " reason=" << reason;
    msg.data = oss.str();
    routing_event_pub_->publish(msg);
  }

  bool removeQueuedChargeRequest(const std::string & uav_id)
  {
    bool removed = false;
    for (auto it = queue_.begin(); it != queue_.end(); ) {
      if (it->uav_id == uav_id) {
        it = queue_.erase(it);
        removed = true;
      } else {
        ++it;
      }
    }
    return removed;
  }

  static std::string escapeJson(const std::string & input)
  {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
      switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out.push_back(c); break;
      }
    }
    return out;
  }

  void updateLastRequest(const std::string & uav_id,
                         const std::string & msg_id,
                         const rclcpp::Time & time,
                         uint8_t role,
                         float battery,
                         const std::string & status)
  {
    last_request_.valid = true;
    last_request_.uav_id = uav_id;
    last_request_.msg_id = msg_id;
    last_request_.time = time;
    last_request_.role = role;
    last_request_.battery = std::clamp(battery, 0.0f, 100.0f);
    last_request_.status = status;
  }

  void appendRejectedRequest(const std::string & uav_id,
                             const std::string & reason,
                             const rclcpp::Time & now)
  {
    rejected_history_.push_back(RejectedRequest{uav_id, now, reason});
    if (rejected_history_.size() > rejected_history_limit_) {
      rejected_history_.pop_front();
    }
  }

  void updateLastDecision(const std::string & uav_id,
                          bool accepted,
                          const rclcpp::Time & now,
                          const rclcpp::Time & slot_start,
                          const rclcpp::Time & slot_end,
                          uint8_t priority,
                          const std::string & msg_id)
  {
    last_decision_.valid = true;
    last_decision_.uav_id = uav_id;
    last_decision_.accepted = accepted;
    last_decision_.time = now;
    last_decision_.slot_start = slot_start;
    last_decision_.slot_end = slot_end;
    last_decision_.policy = policy_name_;
    last_decision_.priority = priority;
    last_decision_.msg_id = msg_id;
  }

  void publishChargingSnapshot()
  {
    if (!charging_snapshot_pub_) {
      return;
    }

    const auto now = this->now();
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);
    oss << "{"
        << "\"stamp\":" << now.seconds()
        << ",\"ugv_id\":\"" << escapeJson(ugv_id_) << "\""
        << ",\"policy\":\"" << escapeJson(policy_name_) << "\"";

    oss << ",\"active_sessions\":[";
    for (size_t i = 0; i < active_sessions_.size(); ++i) {
      const auto & session = active_sessions_[i];
      if (i > 0) {
        oss << ",";
      }
      oss << "{\"uav_id\":\"" << escapeJson(session.uav_id)
          << "\",\"start\":" << session.start_time.seconds()
          << ",\"end\":" << session.end_time.seconds() << "}";
    }
    oss << "]";

    oss << ",\"waiting_queue\":[";
    for (size_t i = 0; i < queue_.size(); ++i) {
      const auto & entry = queue_[i];
      auto [live_role, live_battery] = getLiveRoleAndBattery(entry);
      if (i > 0) {
        oss << ",";
      }
      oss << "{\"uav_id\":\"" << escapeJson(entry.uav_id)
          << "\",\"request_time\":" << entry.request_time.seconds()
          << ",\"live_role\":" << static_cast<int>(live_role)
          << ",\"live_battery\":" << std::clamp(live_battery, 0.0f, 100.0f)
          << "}";
    }
    oss << "]";

    oss << ",\"rejected\":[";
    size_t rej_idx = 0;
    for (const auto & rej : rejected_history_) {
      if (rej_idx++ > 0) {
        oss << ",";
      }
      oss << "{\"uav_id\":\"" << escapeJson(rej.uav_id)
          << "\",\"time\":" << rej.time.seconds()
          << ",\"reason\":\"" << escapeJson(rej.reason) << "\"}";
    }
    oss << "]";

    oss << ",\"last_request\":";
    if (last_request_.valid) {
      oss << "{\"uav_id\":\"" << escapeJson(last_request_.uav_id)
          << "\",\"msg_id\":\"" << escapeJson(last_request_.msg_id)
          << "\",\"time\":" << last_request_.time.seconds()
          << ",\"role\":" << static_cast<int>(last_request_.role)
          << ",\"battery\":" << std::clamp(last_request_.battery, 0.0f, 100.0f)
          << ",\"status\":\"" << escapeJson(last_request_.status) << "\"}";
    } else {
      oss << "{}";
    }

    oss << ",\"last_decision\":";
    if (last_decision_.valid) {
      oss << "{\"uav_id\":\"" << escapeJson(last_decision_.uav_id)
          << "\",\"accepted\":" << (last_decision_.accepted ? "true" : "false")
          << ",\"time\":" << last_decision_.time.seconds()
          << ",\"slot_start\":" << last_decision_.slot_start.seconds()
          << ",\"slot_end\":" << last_decision_.slot_end.seconds()
          << ",\"policy\":\"" << escapeJson(last_decision_.policy)
          << "\",\"priority\":" << static_cast<int>(last_decision_.priority)
          << ",\"msg_id\":\"" << escapeJson(last_decision_.msg_id) << "\"}";
    } else {
      oss << "{}";
    }
    oss << "}";

    std_msgs::msg::String out;
    out.data = oss.str();
    charging_snapshot_pub_->publish(out);
  }

  // Handle control messages routed through the mesh.
  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    // Only consider messages whose final destination is this UGV
    if (msg->dst_id != ugv_id_) {
      return;
    }
    if (!msg->next_hop_id.empty() && msg->next_hop_id != ugv_id_) {
      return;
    }

    auto now = this->now();
    if (msg->control_type == "ACK") {
      const std::string acked_id = extractAckedMsgId(*msg);
      if (!acked_id.empty()) {
        pending_acks_.erase(acked_id);
      }
      return;
    }

    // CHARGE_REQUEST retries are expected when the requester has not
    // observed a decision yet. We must process them idempotently instead of
    // dropping as duplicates, otherwise a lost CHARGE_DECISION can starve the
    // UAV until battery depletion.
    if (msg->requires_ack && msg->control_type != "ACK" &&
        msg->control_type != "CHARGE_REQUEST" &&
        isDuplicateControlMessage(*msg)) {
      maybePublishAck(*msg);
      return;
    }

    publishDelivered(*msg, now);
    maybePublishAck(*msg);

    if (msg->control_type == "MOTION_START") {
      motion_start_received_ = true;
      last_pose_time_ = now;
      RCLCPP_INFO(this->get_logger(),
                  "[MOB-START] %s received %s from %s", ugv_id_.c_str(),
                  msg->control_type.c_str(), msg->src_id.c_str());
      return;
    }

    if (msg->control_type == "CHARGE_REQUEST") {
      const std::string & uav_id = msg->src_id;

      if (dead_uavs_.find(uav_id) != dead_uavs_.end()) {
        updateLastRequest(uav_id, msg->msg_id, now, 0, 0.0f, "rejected");
        appendRejectedRequest(uav_id, "BATTERY_DEAD", now);
        RCLCPP_WARN(this->get_logger(),
                    "UGV: ignoring CHARGE_REQUEST from %s because UAV is marked BATTERY_DEAD.",
                    uav_id.c_str());
        publishQueueEvent("QUEUE_REJECT", uav_id, "BATTERY_DEAD", queue_.size());
        return;
      }

      // Lookup last known status for this UAV
      auto it = uav_status_.find(uav_id);
      if (it == uav_status_.end()) {
        updateLastRequest(uav_id, msg->msg_id, now, 0, 0.0f, "rejected");
        appendRejectedRequest(uav_id, "NO_STATUS", now);
        RCLCPP_WARN(this->get_logger(),
                    "UGV: received CHARGE_REQUEST from '%s' but no status known. Ignoring.",
                    uav_id.c_str());
        return;
      }

      const auto & info = it->second;
      const double status_age_sec = (now - info.last_seen).seconds();
      const bool status_fresh =
        charge_request_status_stale_sec_ <= 0.0 || status_age_sec <= charge_request_status_stale_sec_;

      if (status_fresh &&
          !info.intent_to_leave &&
          info.battery_level > static_cast<float>(charge_request_battery_gate_percent_)) {
        updateLastRequest(uav_id, msg->msg_id, now, info.role, info.battery_level, "rejected");
        appendRejectedRequest(uav_id, "BATTERY_ABOVE_GATE", now);
        bool removed = removeQueuedChargeRequest(uav_id);
        RCLCPP_INFO(this->get_logger(),
                    "UGV: ignoring CHARGE_REQUEST from %s, battery %.1f%% above UAV battery_threshold %.1f%% and intent_to_leave=false.",
                    uav_id.c_str(), info.battery_level, charge_request_battery_gate_percent_);
        if (removed) {
          publishQueueEvent("QUEUE_CANCEL", uav_id, "BATTERY_RECOVERED", queue_.size());
        }
        publishQueueEvent("QUEUE_REJECT", uav_id, "BATTERY_ABOVE_GATE", queue_.size());
        return;
      }

      auto existing = std::find_if(queue_.begin(), queue_.end(),
        [&](const QueueEntry & queued) {
          return queued.uav_id == uav_id || queued.request_msg_id == msg->msg_id;
        });

      if (existing != queue_.end()) {
        existing->role = info.role;
        existing->battery_level = info.battery_level;
        existing->request_time = now;
        existing->request_msg_id = msg->msg_id;
        updateLastRequest(uav_id, msg->msg_id, now, info.role, info.battery_level, "updated");
        RCLCPP_INFO(this->get_logger(),
                    "UGV: updated CHARGE_REQUEST from %s (role=%u, batt=%.1f%%). "
                    "Queue size remains: %zu",
                    existing->uav_id.c_str(), existing->role,
                    existing->battery_level, queue_.size());
        return;
      }

      QueueEntry entry;
      entry.uav_id = uav_id;
      entry.role = info.role;
      entry.battery_level = info.battery_level;
      entry.request_time = now;
      entry.request_msg_id = msg->msg_id;

      queue_.push_back(entry);
      updateLastRequest(uav_id, msg->msg_id, now, info.role, info.battery_level, "enqueued");

      RCLCPP_INFO(this->get_logger(),
                  "UGV: enqueued CHARGE_REQUEST from %s (role=%u, batt=%.1f%%). "
                  "Queue size now: %zu",
                  entry.uav_id.c_str(), entry.role, entry.battery_level, queue_.size());
      return;
    }

    if (msg->control_type == "DEPLOYMENT_CMD" || msg->control_type == "DEPLOYMENT") {
      handleDeploymentFromNetwork(msg);
      return;
    }

    // Any other message destined to UGV counts as delivered.
  }

  // Update target pose and reset motion bookkeeping.
  void setDeploymentGoal(const geometry_msgs::msg::Pose & target)
  {
    deployment_goal_pose_ = target;
    has_deployment_goal_ = true;
    ugv_in_motion_ = true;
    deployment_received_ = true;
    motion_start_received_ = false;

    if (last_pose_time_.nanoseconds() == 0) {
      last_pose_time_ = this->now();
    }

    RCLCPP_INFO(this->get_logger(),
                "UGV %s: received deployment target -> (%.1f, %.1f, %.1f)",
                ugv_id_.c_str(),
                deployment_goal_pose_.position.x,
                deployment_goal_pose_.position.y,
                deployment_goal_pose_.position.z);
  }

  // Move the UGV toward its deployment goal at a fixed speed.
  void mobilityStep()
  {
    if (!mobility_enabled_ || !has_deployment_goal_ ||
        !deployment_received_ || !motion_start_received_) {
      ugv_velocity_.linear.x = 0.0;
      ugv_velocity_.linear.y = 0.0;
      ugv_velocity_.linear.z = 0.0;
      return;
    }

    auto now = this->now();
    double dt = mobility_dt_sec_;
    if (last_pose_time_.nanoseconds() > 0 &&
        last_pose_time_.get_clock_type() == now.get_clock_type())
    {
      dt = (now - last_pose_time_).seconds();
      if (dt <= 0.0) {
        dt = mobility_dt_sec_;
      }
    }
    geometry_msgs::msg::Pose prev_pose = ugv_pose_;
    last_pose_time_ = now;

    double dx = deployment_goal_pose_.position.x - ugv_pose_.position.x;
    double dy = deployment_goal_pose_.position.y - ugv_pose_.position.y;
    double dist = std::sqrt(dx * dx + dy * dy);
    double max_step = ugv_speed_mps_ * dt;

    if (dist <= 1e-3) {
      ugv_in_motion_ = false;
      ugv_velocity_.linear.x = 0.0;
      ugv_velocity_.linear.y = 0.0;
      ugv_velocity_.linear.z = 0.0;
      return;
    }

    if (dist <= max_step) {
      ugv_pose_ = deployment_goal_pose_;
      ugv_in_motion_ = false;
      RCLCPP_INFO(this->get_logger(),
                  "UGV %s reached deployment pose (%.1f, %.1f, %.1f)",
                  ugv_id_.c_str(),
                  ugv_pose_.position.x,
                  ugv_pose_.position.y,
                  ugv_pose_.position.z);
      if (dt > 0.0) {
        ugv_velocity_.linear.x = static_cast<float>(
          (ugv_pose_.position.x - prev_pose.position.x) / dt);
        ugv_velocity_.linear.y = static_cast<float>(
          (ugv_pose_.position.y - prev_pose.position.y) / dt);
        ugv_velocity_.linear.z = 0.0f;
      } else {
        ugv_velocity_.linear.x = 0.0f;
        ugv_velocity_.linear.y = 0.0f;
        ugv_velocity_.linear.z = 0.0f;
      }
      return;
    }

    double ux = dx / dist;
    double uy = dy / dist;
    ugv_pose_.position.x += ux * max_step;
    ugv_pose_.position.y += uy * max_step;
    // keep current z (UGV stays on ground unless target dictates otherwise)

    if (dt > 0.0) {
      ugv_velocity_.linear.x = static_cast<float>(
        (ugv_pose_.position.x - prev_pose.position.x) / dt);
      ugv_velocity_.linear.y = static_cast<float>(
        (ugv_pose_.position.y - prev_pose.position.y) / dt);
      ugv_velocity_.linear.z = 0.0f;
    } else {
      ugv_velocity_.linear.x = 0.0f;
      ugv_velocity_.linear.y = 0.0f;
      ugv_velocity_.linear.z = 0.0f;
    }
  }

  // Observe deployments to learn topology and update UGV pose.
  void deploymentCallback(const uav_msgs::msg::UavDeployment::SharedPtr msg)
  {
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

  // Decode deployment commands injected through the network layer.
  void handleDeploymentFromNetwork(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    // msg->payload format:
    // "role,cluster_id,ch_id,x,y,z,next_sink,next_ugv"
    std::stringstream ss(msg->payload);
    std::string token;
    std::string cluster_id, ch_id;
    double x = 0.0, y = 0.0, z = 0.0;
    std::string next_sink;

    // role (used to update routing metadata)
    if (!std::getline(ss, token, ',')) {
      RCLCPP_WARN(this->get_logger(),
                  "UGV %s: bad DEPLOYMENT payload \"%s\"",
                  ugv_id_.c_str(), msg->payload.c_str());
      return;
    }
    int role = -1;
    try {
      role = std::stoi(token);
    } catch (const std::exception &) {
      RCLCPP_WARN(this->get_logger(),
                  "UGV %s: invalid role in DEPLOYMENT payload \"%s\"",
                  ugv_id_.c_str(), msg->payload.c_str());
      return;
    }

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

    // Update routing metadata even when deployments arrive over the network.
    if (role == 0) {
      if (!ch_id.empty()) {
        uav_to_ch_[msg->dst_id] = ch_id;
      }
    } else if (role == 1) {
      uav_to_ch_[msg->dst_id] = msg->dst_id;
      geometry_msgs::msg::Pose ch_pose;
      ch_pose.position.x = x;
      ch_pose.position.y = y;
      ch_pose.position.z = z;
      ch_pose.orientation.w = 1.0;
      ch_poses_[msg->dst_id] = ch_pose;
    }

    if (msg->dst_id != ugv_id_) {
      return;
    }

    deployment_ack_sent_ = false;

    geometry_msgs::msg::Pose goal;
    goal.position.x = x;
    goal.position.y = y;
    goal.position.z = z;
    goal.orientation.w = 1.0;
    setDeploymentGoal(goal);

    RCLCPP_INFO(this->get_logger(),
                "UGV %s: DEPLOYMENT via network -> pos=(%.1f,%.1f,%.1f) next_sink=%s",
                ugv_id_.c_str(),
                deployment_goal_pose_.position.x,
                deployment_goal_pose_.position.y,
                deployment_goal_pose_.position.z,
                next_sink.empty() ? "-" : next_sink.c_str());

    publishDelivered(*msg, this->now());
    sendDeploymentAck(next_sink);
  }

  // Ack deployment so the sink can release the motion barrier.
  void sendDeploymentAck(const std::string & suggested_next_hop = "")
  {
    if (deployment_ack_sent_) {
      return;
    }

    if (!control_pub_) {
      return;
    }

    uav_msgs::msg::TrafficMessage ack;
    ack.msg_id = "DEP_ACK_" + ugv_id_ + "_" + std::to_string(dep_ack_seq_++);
    ack.src_id = ugv_id_;
    ack.dst_id = sink_id_;

    (void)suggested_next_hop;
    ack.next_hop_id = resolveNextHop(sink_id_);
    if (ack.next_hop_id.empty() && !uplink_ch_id_.empty()) {
      ack.next_hop_id = uplink_ch_id_;
      RCLCPP_INFO(this->get_logger(),
                  "UGV %s: using uplink CH %s for DEPLOYMENT_ACK",
                  ugv_id_.c_str(), uplink_ch_id_.c_str());
    }

    ack.flow_type = 1;
    ack.creation_time = this->now();
    ack.hop_count = 0;

    ack.control_type = "DEPLOYMENT_ACK";
    ack.payload = "";

    if (ack.next_hop_id.empty()) {
      ack.next_hop_id = "ch0";
      RCLCPP_WARN(this->get_logger(),
                  "UGV %s: no route to sink for DEPLOYMENT_ACK, falling back to %s.",
                  ugv_id_.c_str(), ack.next_hop_id.c_str());
    } else if (!neighborReachable(ack.next_hop_id)) {
      ack.next_hop_id = "ch0";
      RCLCPP_WARN(this->get_logger(),
                  "UGV %s: DEPLOYMENT_ACK next hop unreachable, falling back to %s.",
                  ugv_id_.c_str(), ack.next_hop_id.c_str());
    }

    control_pub_->publish(ack);
    deployment_ack_sent_ = true;

    RCLCPP_INFO(this->get_logger(),
                "UGV %s sent DEPLOYMENT_ACK to sink via %s",
                ugv_id_.c_str(), ack.next_hop_id.c_str());
  }

  // Broadcast HELLO for discovery and timing tests.
  // Publish UGV status so it appears as a neighbor entity.
  void publishStatus()
  {
    if (!status_pub_) {
      return;
    }

    auto now = this->now();
    uav_msgs::msg::UavStatus status;
    status.uav_id = ugv_id_;
    status.role = 2;  // treat UGV as BACKUP_CH-equivalent for discovery
    status.cluster_id = "ugv";
    status.battery_level = 100.0f;
    status.battery_capacity = 0.0f;
    status.pose = ugv_pose_;
    status.service_radius = static_cast<float>(comm_radius_m_);
    status.connected_users = 0;
    status.traffic_load = 0.0f;
    status.packet_loss_estimate = 0.0f;
    status.energy_consumption_rate = 0.0f;
    status.stamp = now;
    status.backbone_active = true;

    status_pub_->publish(status);
  }

  void publishDelivered(const uav_msgs::msg::TrafficMessage & msg, const rclcpp::Time & now)
  {
    if (!delivered_pub_) {
      return;
    }
    uav_msgs::msg::TrafficMessage delivered = msg;
    delivered.next_hop_id.clear();
    delivered_pub_->publish(delivered);
  }

  std::string extractAckedMsgId(const uav_msgs::msg::TrafficMessage & msg) const
  {
    if (!msg.ref_msg_id.empty()) {
      return msg.ref_msg_id;
    }
    if (msg.payload.rfind("ref_msg_id=", 0) == 0) {
      return msg.payload.substr(std::string("ref_msg_id=").size());
    }
    return msg.payload;
  }

  bool isDuplicateControlMessage(const uav_msgs::msg::TrafficMessage & msg)
  {
    if (msg.flow_type != 1 || msg.control_type == "ACK" || !msg.requires_ack) {
      return false;
    }
    std::string key = msg.src_id + "|" + msg.msg_id;
    auto it = control_dedup_index_.find(key);
    if (it != control_dedup_index_.end()) {
      control_dedup_order_.splice(control_dedup_order_.end(), control_dedup_order_, it->second);
      return true;
    }
    control_dedup_order_.push_back(key);
    control_dedup_index_[key] = std::prev(control_dedup_order_.end());
    if (control_dedup_order_.size() > control_dedup_cache_size_) {
      const auto & oldest = control_dedup_order_.front();
      control_dedup_index_.erase(oldest);
      control_dedup_order_.pop_front();
    }
    return false;
  }

  void registerPendingAck(const uav_msgs::msg::TrafficMessage & msg,
                          int max_retries,
                          double retry_interval_sec)
  {
    if (msg.flow_type != 1 || !msg.requires_ack || msg.msg_id.empty()) {
      return;
    }
    PendingAck pending;
    pending.msg = msg;
    pending.last_send_time = this->now();
    pending.attempts = 1;
    pending.max_retries = max_retries;
    pending.retry_interval_sec = retry_interval_sec;
    pending_acks_[msg.msg_id] = pending;
  }

  void refreshPendingNextHop(PendingAck & pending)
  {
    if (pending.msg.control_type == "CHARGE_DECISION") {
      pending.msg.next_hop_id = resolveNextHop(pending.msg.dst_id);
    }
  }

  void resendPendingAcks()
  {
    if (pending_acks_.empty() || !control_pub_) {
      return;
    }
    auto now = this->now();
    for (auto it = pending_acks_.begin(); it != pending_acks_.end(); ) {
      auto & pending = it->second;
      if (pending.max_retries > 0 && pending.attempts >= pending.max_retries) {
        RCLCPP_ERROR(this->get_logger(),
                     "UGV %s: giving up on %s msg_id=%s after %d attempts.",
                     ugv_id_.c_str(),
                     pending.msg.control_type.c_str(),
                     pending.msg.msg_id.c_str(),
                     pending.attempts);
        it = pending_acks_.erase(it);
        continue;
      }
      double elapsed = (now - pending.last_send_time).seconds();
      if (elapsed >= pending.retry_interval_sec) {
        refreshPendingNextHop(pending);
        if (!pending.msg.next_hop_id.empty() && neighborReachable(pending.msg.next_hop_id)) {
          control_pub_->publish(pending.msg);
          pending.last_send_time = now;
          pending.attempts++;
        } else {
          pending.last_send_time = now;
          pending.attempts++;
        }
      }
      ++it;
    }
  }

  void maybePublishAck(const uav_msgs::msg::TrafficMessage & msg)
  {
    if (!msg.requires_ack || !control_pub_) {
      return;
    }

    uav_msgs::msg::TrafficMessage ack;
    ack.msg_id = ugv_id_ + "_ACK_" + msg.msg_id + "_" + std::to_string(msg_counter_++);
    ack.src_id = ugv_id_;
    ack.dst_id = msg.src_id;
    ack.ref_msg_id = msg.msg_id;
    ack.flow_type = 1;
    ack.control_type = "ACK";
    ack.payload = msg.msg_id;
    ack.creation_time = this->now();
    ack.hop_count = 0;
    ack.ttl = 6;
    ack.requires_ack = false;
    ack.next_hop_id = resolveNextHop(ack.dst_id);
    if (ack.next_hop_id.empty() || !neighborReachable(ack.next_hop_id)) {
      RCLCPP_WARN(this->get_logger(),
                  "UGV %s: cannot ACK msg_id=%s (no reachable next hop)",
                  ugv_id_.c_str(), msg.msg_id.c_str());
      publishRoutingEvent("NO_ROUTE_ACK", ack.dst_id, ack.next_hop_id);
      return;
    }

    control_pub_->publish(ack);
  }

  double distance3d(const geometry_msgs::msg::Point & a,
                    const geometry_msgs::msg::Point & b) const
  {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    double dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
  }

  bool neighborReachable(const std::string & id) const
  {
    if (id.empty()) {
      return false;
    }

    auto it = uav_status_.find(id);
    if (it == uav_status_.end()) {
      return false;
    }

    const auto & nb = it->second;

    if (neighbor_timeout_sec_ > 0.0) {
      if (nb.last_seen.nanoseconds() == 0) {
        return false;
      }
      double age = (this->now() - nb.last_seen).seconds();
      if (age > neighbor_timeout_sec_) {
        return false;
      }
    }

    double dist = distance3d(ugv_pose_.position, nb.pose.position);
    double range = std::min(comm_radius_m_, static_cast<double>(nb.comm_radius_m));
    if (range <= 0.0 || dist > range) {
      return false;
    }

    const double leave_soon_sec = 15.0;
    if (nb.charging_state != 0) {
      return false;
    }
    if (nb.intent_to_leave && nb.eta_to_leave_sec >= 0.0f && nb.eta_to_leave_sec < leave_soon_sec) {
      return false;
    }

    return nb.backbone_active;
  }

  void pruneNeighbors()
  {
    if (neighbor_timeout_sec_ <= 0.0) return;

    auto now = this->now();
    for (auto it = uav_status_.begin(); it != uav_status_.end(); ) {
      double age = it->second.last_seen.nanoseconds() == 0
        ? std::numeric_limits<double>::infinity()
        : (now - it->second.last_seen).seconds();
      if (age > neighbor_timeout_sec_) {
        it = uav_status_.erase(it);
      } else {
        ++it;
      }
    }
  }

  void publishDrop(const std::string & msg_id, const std::string & reason)
  {
    if (!control_pub_) {
      return;
    }
    uav_msgs::msg::TrafficMessage drop;
    drop.msg_id = msg_id + "_DROP_" + ugv_id_;
    drop.src_id = ugv_id_;
    drop.dst_id = sink_id_;
    drop.flow_type = 1;
    drop.control_type = "DROP";
    drop.payload = msg_id + "," + reason;
    drop.creation_time = this->now();
    drop.hop_count = 0;
    drop.ttl = 4;
    drop.next_hop_id = resolveNextHop(sink_id_);
    if (drop.next_hop_id.empty()) {
      publishRoutingEvent("NO_ROUTE_DROP", sink_id_, drop.next_hop_id);
    }
    control_pub_->publish(drop);
  }

  bool ensureReachableOrDrop(uav_msgs::msg::TrafficMessage & msg,
                             const std::string & drop_reason)
  {
    if (msg.next_hop_id.empty()) {
      publishDrop(msg.msg_id, drop_reason);
      publishRoutingEvent(drop_reason, msg.dst_id, msg.next_hop_id);
      return false;
    }

    if (neighborReachable(msg.next_hop_id)) {
      return true;
    }

    msg.next_hop_id.clear();
    publishDrop(msg.msg_id, drop_reason);
    publishRoutingEvent(drop_reason, msg.dst_id, msg.next_hop_id);
    return false;
  }

  // ------------- Scheduler -------------

  // Assign charging sessions based on the active policy.
  void schedulerLoop()
  {
    auto now = this->now();

    // Remove finished charging sessions
    auto it = active_sessions_.begin();
    while (it != active_sessions_.end()) {
      if (now >= it->end_time) {
        RCLCPP_INFO(this->get_logger(),
                    "Charging session completed for %s at t=%.1f",
                    it->uav_id.c_str(),
                    it->end_time.seconds());
        it = active_sessions_.erase(it);
      } else {
        ++it;
      }
    }

    // If no dock capacity, exit early
    if (max_parallel_spots_ == 0) {
      return;
    }

    // Dock is free; if no one waiting, nothing to do
    if (queue_.empty()) {
      return;
    }

    size_t preview_idx = chooseNextIndex(now);
    logSchedulerCandidates(now, preview_idx);

    size_t available_spots = 0;
    if (max_parallel_spots_ > static_cast<int>(active_sessions_.size())) {
      available_spots = static_cast<size_t>(max_parallel_spots_ - active_sessions_.size());
    }

    while (available_spots > 0 && !queue_.empty()) {
      size_t idx = chooseNextIndex(now);
      DecisionRationale rationale = buildDecisionRationale(idx, now);
      QueueEntry job = queue_[idx];
      auto [job_role, job_battery] = getLiveRoleAndBattery(job);
      queue_.erase(queue_.begin() + static_cast<long>(idx));

      rclcpp::Time slot_start_time = now;
      rclcpp::Time slot_end_time = slot_start_time +
                                   rclcpp::Duration::from_seconds(charging_duration_sec_);

      // Publish ChargeDecision (direct, not routed yet)
      uav_msgs::msg::ChargeDecision decision;
      decision.uav_id = job.uav_id;
      decision.accepted = true;
      decision.slot_start_time = slot_start_time;
      decision.priority = job_role;
      decision.policy = policy_name_;

      charge_decision_pub_->publish(decision);
      updateLastDecision(job.uav_id,
                         true,
                         now,
                         slot_start_time,
                         slot_end_time,
                         job_role,
                         job.request_msg_id);

      RCLCPP_INFO(this->get_logger(),
                  "UGV: assigned dock to %s (role=%u, batt=%.1f%%) with policy='%s'. "
                  "Session: [%.1f, %.1f], queue size now: %zu, active sessions: %zu/%d",
                  job.uav_id.c_str(), job_role, job_battery,
                  policy_name_.c_str(),
                  slot_start_time.seconds(), slot_end_time.seconds(),
                  queue_.size(), active_sessions_.size() + 1, max_parallel_spots_);
      // Also send the decision through the routed network as a control message
      sendDecisionControlMessage(job, now, rationale);

      active_sessions_.push_back({job.uav_id, slot_start_time, slot_end_time});

      if (max_parallel_spots_ > static_cast<int>(active_sessions_.size())) {
        available_spots = static_cast<size_t>(max_parallel_spots_ - active_sessions_.size());
      } else {
        available_spots = 0;
      }
    }

  }

  struct DecisionRationale
  {
    double tte_sec = -1.0;
    double score = -1.0;
    size_t rank_index = 0;
    size_t queue_size = 0;
  };

  std::pair<uint8_t, float> getLiveRoleAndBattery(const QueueEntry & entry) const
  {
    auto clamp_battery = [](float value) {
      return std::clamp(value, 0.0f, 100.0f);
    };

    auto it = uav_status_.find(entry.uav_id);
    if (it != uav_status_.end()) {
      return {it->second.role, clamp_battery(it->second.battery_level)};
    }
    return {entry.role, clamp_battery(entry.battery_level)};
  }

  double getWaitSec(const QueueEntry & entry, const rclcpp::Time & now) const
  {
    double wait_sec = (now - entry.request_time).seconds();
    if (wait_sec < 0.0) {
      wait_sec = 0.0;
    }
    return wait_sec;
  }

  double estimateTimeToEmptySec(const QueueEntry & entry) const
  {
    auto [live_role, live_battery] = getLiveRoleAndBattery(entry);
    double drain = (live_role == 1) ? drain_percent_ch_ : drain_percent_member_;
    if (drain <= 0.0) {
      return std::numeric_limits<double>::infinity();
    }
    return static_cast<double>(live_battery) / drain;
  }

  double computeDynamicScore(const QueueEntry & entry, const rclcpp::Time & now) const
  {
    auto [live_role, live_battery] = getLiveRoleAndBattery(entry);
    double role_term = (live_role == 1 ? 1.0 : 0.0);
    double batt_term = (100.0 - static_cast<double>(live_battery));
    double wait_sec = getWaitSec(entry, now);
    return w_role_ * role_term + w_batt_ * batt_term + w_wait_ * wait_sec;
  }

  DecisionRationale buildDecisionRationale(size_t idx, const rclcpp::Time & now) const
  {
    DecisionRationale rationale;
    rationale.rank_index = idx;
    rationale.queue_size = queue_.size();
    if (idx >= queue_.size()) {
      return rationale;
    }
    const auto & entry = queue_[idx];
    rationale.tte_sec = estimateTimeToEmptySec(entry);
    rationale.score = computeDynamicScore(entry, now);
    return rationale;
  }

  void logSchedulerCandidates(const rclcpp::Time & now, size_t selected_index) const
  {
    if (!debug_scheduler_candidates_ || queue_.empty()) {
      return;
    }

    struct Candidate
    {
      size_t idx;
      std::string uav_id;
      uint8_t role;
      float battery;
      rclcpp::Time request_time;
      double wait_sec;
      double tte;
      double score;
    };

    std::vector<Candidate> rows;
    rows.reserve(queue_.size());
    for (size_t i = 0; i < queue_.size(); ++i) {
      const auto & q = queue_[i];
      auto [live_role, live_battery] = getLiveRoleAndBattery(q);
      rows.push_back(Candidate{
        i,
        q.uav_id,
        live_role,
        live_battery,
        q.request_time,
        getWaitSec(q, now),
        estimateTimeToEmptySec(q),
        computeDynamicScore(q, now)
      });
    }

    constexpr double kEps = 1e-9;
    std::sort(rows.begin(), rows.end(), [&](const Candidate & a, const Candidate & b) {
      if (policy_ == Policy::ROLE_PRIORITY) {
        if (a.role != b.role) {
          return a.role > b.role;
        }
        if (a.request_time != b.request_time) {
          return a.request_time < b.request_time;
        }
        return a.uav_id < b.uav_id;
      }

      if (policy_ == Policy::EDF) {
        if (std::abs(a.tte - b.tte) > kEps) {
          return a.tte < b.tte;
        }
        if (a.role != b.role) {
          return a.role > b.role;
        }
        if (a.request_time != b.request_time) {
          return a.request_time < b.request_time;
        }
        return a.uav_id < b.uav_id;
      }

      if (policy_ == Policy::DYNAMIC_SCORE) {
        if (std::abs(a.score - b.score) > kEps) {
          return a.score > b.score;
        }
        if (a.request_time != b.request_time) {
          return a.request_time < b.request_time;
        }
        return a.uav_id < b.uav_id;
      }

      if (a.request_time != b.request_time) {
        return a.request_time < b.request_time;
      }
      return a.uav_id < b.uav_id;
    });

    std::ostringstream oss;
    oss << "[SCHED-CANDIDATES] preview_index=" << selected_index;
    const size_t count = std::min<size_t>(5, rows.size());
    for (size_t i = 0; i < count; ++i) {
      const auto & c = rows[i];
      oss << " | idx=" << c.idx
          << ",uav_id=" << c.uav_id
          << ",live_role=" << static_cast<int>(c.role)
          << ",live_battery=" << c.battery
          << ",wait_sec=" << c.wait_sec
          << ",EDF_TTE=" << c.tte
          << ",dynamic_score=" << c.score;
    }
    RCLCPP_DEBUG(this->get_logger(), "%s", oss.str().c_str());
  }

  // ------------- Policy-specific selection -------------

  // Select the next queue entry according to the configured policy.
  size_t chooseNextIndex(const rclcpp::Time & now)
  {
    constexpr double kEps = 1e-9;

    if (policy_ == Policy::FCFS) {
      return 0;
    }

    if (policy_ == Policy::ROLE_PRIORITY) {
      size_t best_idx = 0;
      auto [best_role, best_batt_unused] = getLiveRoleAndBattery(queue_[0]);
      (void)best_batt_unused;

      for (size_t i = 1; i < queue_.size(); ++i) {
        const auto & q = queue_[i];
        const auto & best_q = queue_[best_idx];
        auto [role, batt_unused] = getLiveRoleAndBattery(q);
        (void)batt_unused;

        if (role > best_role ||
            (role == best_role && q.request_time < best_q.request_time) ||
            (role == best_role && q.request_time == best_q.request_time && q.uav_id < best_q.uav_id)) {
          best_idx = i;
          best_role = role;
        }
      }
      return best_idx;
    }

    if (policy_ == Policy::EDF) {
      size_t best_idx = 0;
      double best_tte = estimateTimeToEmptySec(queue_[0]);
      auto [best_role, best_batt_unused] = getLiveRoleAndBattery(queue_[0]);
      (void)best_batt_unused;

      for (size_t i = 1; i < queue_.size(); ++i) {
        const auto & q = queue_[i];
        const auto & best_q = queue_[best_idx];
        double tte = estimateTimeToEmptySec(q);
        auto [role, batt_unused] = getLiveRoleAndBattery(q);
        (void)batt_unused;

        if (tte < best_tte ||
            (std::abs(tte - best_tte) <= kEps && role > best_role) ||
            (std::abs(tte - best_tte) <= kEps && role == best_role && q.request_time < best_q.request_time) ||
            (std::abs(tte - best_tte) <= kEps && role == best_role && q.request_time == best_q.request_time && q.uav_id < best_q.uav_id)) {
          best_idx = i;
          best_tte = tte;
          best_role = role;
        }
      }
      return best_idx;
    }

    // Policy::DYNAMIC_SCORE
    size_t best_idx = 0;
    double best_score = computeDynamicScore(queue_[0], now);

    for (size_t i = 1; i < queue_.size(); ++i) {
      const auto & q = queue_[i];
      const auto & best_q = queue_[best_idx];
      double score = computeDynamicScore(q, now);

      if (score > best_score ||
          (std::abs(score - best_score) <= kEps && q.request_time < best_q.request_time) ||
          (std::abs(score - best_score) <= kEps && q.request_time == best_q.request_time && q.uav_id < best_q.uav_id)) {
        best_idx = i;
        best_score = score;
      }
    }

    return best_idx;
  }

  // ------------- Members -------------

  std::string ugv_id_;
  std::string sink_id_;
  geometry_msgs::msg::Pose ugv_pose_;
  rclcpp::Subscription<uav_msgs::msg::UavDeployment>::SharedPtr deployment_sub_;

  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Subscription<uav_msgs::msg::UavStatus>::SharedPtr status_sub_;
  rclcpp::Subscription<uav_msgs::msg::FailureEvent>::SharedPtr failure_sub_;
  rclcpp::Subscription<uav_msgs::msg::RoutingTable>::SharedPtr routing_table_sub_;
  rclcpp::Publisher<uav_msgs::msg::ChargeDecision>::SharedPtr charge_decision_pub_;
  rclcpp::TimerBase::SharedPtr scheduler_timer_;
  rclcpp::TimerBase::SharedPtr status_timer_;
  rclcpp::TimerBase::SharedPtr ack_retry_timer_;
  rclcpp::TimerBase::SharedPtr mobility_timer_;
  rclcpp::TimerBase::SharedPtr neighbor_timeout_timer_;
  std::string uplink_ch_id_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr control_pub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_pub_;
  rclcpp::Publisher<uav_msgs::msg::UavStatus>::SharedPtr status_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr routing_event_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr queue_event_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr charging_snapshot_pub_;
  uint64_t msg_counter_ = 0;

  double charging_duration_sec_;
  double target_utilization_;
  double flight_time_ch_min_;
  double charge_time_ch_min_;
  double flight_time_mem_min_;
  double charge_time_mem_min_;
  bool mobility_enabled_ = true;
  double mobility_dt_sec_ = 0.2;
  double ugv_speed_mps_ = 15.3;
  double comm_radius_m_ = 400.0;
  double charge_decision_retry_sec_ = 2.0;
  int charge_decision_max_retries_ = 5;
  double ack_retry_period_sec_ = 0.5;
  size_t control_dedup_cache_size_ = 200;
  double charge_request_battery_gate_percent_ = 45.0;
  double charge_request_status_stale_sec_ = 3.0;
  Policy policy_;
  std::string policy_name_;
  double neighbor_timeout_sec_ = 0.0;

  // EDF parameters
  double drain_percent_member_;
  double drain_percent_ch_;

  // Dynamic-score parameters
  double w_role_;
  double w_batt_;
  double w_wait_;
  bool debug_scheduler_candidates_ = false;
  double hello_period_sec_ = 1.0;
  double status_period_sec_ = 1.0;
  geometry_msgs::msg::Pose deployment_goal_pose_;
  bool has_deployment_goal_ = false;
  bool ugv_in_motion_ = false;
  bool deployment_received_ = false;
  bool motion_start_received_ = false;
  rclcpp::Time last_pose_time_;
  geometry_msgs::msg::Twist ugv_velocity_;
  rclcpp::TimerBase::SharedPtr charging_snapshot_timer_;

  bool deployment_ack_sent_ = false;
  uint64_t dep_ack_seq_ = 0;

  std::deque<QueueEntry> queue_;
  int max_parallel_spots_;
  std::unordered_map<std::string, PendingAck> pending_acks_;
  std::list<std::string> control_dedup_order_;
  std::unordered_map<std::string, std::list<std::string>::iterator> control_dedup_index_;
  struct ChargingSession
  {
    std::string uav_id;
    rclcpp::Time start_time;
    rclcpp::Time end_time;
  };

  std::vector<ChargingSession> active_sessions_;
  std::deque<RejectedRequest> rejected_history_;
  size_t rejected_history_limit_ = 20;
  LastRequestState last_request_;
  LastDecisionState last_decision_;

  std::unordered_map<std::string, UavInfo> uav_status_;
  std::unordered_map<std::string, std::string> routing_table_;
  std::unordered_set<std::string> dead_uavs_;

  // Map each UAV to its CH, based on deployments
  std::unordered_map<std::string, std::string> uav_to_ch_;

  // Optional: store CH poses if we want geometric reasoning later
  std::unordered_map<std::string, geometry_msgs::msg::Pose> ch_poses_;
  // Send a routed control message so the UAV receives its decision.
  void sendDecisionControlMessage(const QueueEntry & job,
                                  const rclcpp::Time & now,
                                  const DecisionRationale & rationale)
  {
    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = ugv_id_ + "_charge_decision_" + job.uav_id + "_" +
                 std::to_string(now.nanoseconds());
    msg.src_id = ugv_id_;
    msg.dst_id = job.uav_id;
    msg.ref_msg_id = job.request_msg_id;

    msg.next_hop_id = resolveNextHop(job.uav_id);
    if (msg.next_hop_id.empty()) {
      RCLCPP_WARN(this->get_logger(),
                  "UGV %s: no route to %s for CHARGE_DECISION",
                  ugv_id_.c_str(), job.uav_id.c_str());
      publishRoutingEvent("NO_ROUTE_CHARGE_DECISION", job.uav_id, msg.next_hop_id);
      return;
    }

    msg.flow_type = 1;              // CONTROL_ALERT
    msg.creation_time = now;
    msg.hop_count = 0;
    msg.requires_ack = true;

    msg.control_type = "CHARGE_DECISION";
    std::ostringstream payload;
    payload << "policy=" << policy_name_
            << ";priority=" << static_cast<int>(job.role)
            << ";rank_index=" << rationale.rank_index
            << ";queue_size=" << rationale.queue_size
            << ";tte_sec=" << rationale.tte_sec
            << ";score=" << rationale.score;
    msg.payload = payload.str();

    if (!ensureReachableOrDrop(msg, "UNREACHABLE_CHARGE_DECISION_NEXT_HOP")) {
      RCLCPP_WARN(this->get_logger(),
                  "UGV %s: dropping CHARGE_DECISION msg_id=%s (next hop unreachable)",
                  ugv_id_.c_str(), msg.msg_id.c_str());
      return;
    }

    RCLCPP_INFO(this->get_logger(),
                "UGV: sending CHARGE_DECISION msg_id=%s to %s via %s",
                msg.msg_id.c_str(), msg.dst_id.c_str(), msg.next_hop_id.c_str());

    control_pub_->publish(msg);
    registerPendingAck(msg, charge_decision_max_retries_, charge_decision_retry_sec_);
  }

  // Recompute dock capacity based on fleet mix and utilization target.
  void recomputeChargingSpots()
  {
    int num_ch = 0;
    int num_members = 0;
    for (const auto & kv : uav_status_) {
      if (kv.second.role == 1) {
        ++num_ch;
      } else {
        ++num_members;
      }
    }

    double load_ch = 0.0;
    double load_mem = 0.0;
    if (flight_time_ch_min_ + charge_time_ch_min_ <= 0.0) {
      RCLCPP_WARN(this->get_logger(), "Invalid CH flight/charge times; skipping CH load contribution");
    } else {
      load_ch = charge_time_ch_min_ / (flight_time_ch_min_ + charge_time_ch_min_);
    }
    if (flight_time_mem_min_ + charge_time_mem_min_ <= 0.0) {
      RCLCPP_WARN(this->get_logger(), "Invalid member flight/charge times; skipping member load contribution");
    } else {
      load_mem = charge_time_mem_min_ / (flight_time_mem_min_ + charge_time_mem_min_);
    }

    double total_load = num_ch * load_ch + num_members * load_mem;
    double effective_target = target_utilization_ <= 0.0 ? 0.9 : target_utilization_;

    int required_spots = compute_required_charging_spots(
      num_ch,
      num_members,
      flight_time_ch_min_,
      charge_time_ch_min_,
      flight_time_mem_min_,
      charge_time_mem_min_,
      effective_target);

    if (required_spots != max_parallel_spots_) {
      RCLCPP_INFO(this->get_logger(),
                  "Computed required charging spots: %d (total_load=%.3f, target_utilization=%.2f, num_ch=%d, num_members=%d)",
                  required_spots, total_load, effective_target, num_ch, num_members);
    }
    max_parallel_spots_ = required_spots;
  }


};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UgvChargerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
