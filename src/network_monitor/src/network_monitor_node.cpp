#include <algorithm>
#include <chrono>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <cmath>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/charge_request.hpp"
#include "uav_msgs/msg/charge_decision.hpp"
#include "uav_msgs/msg/routing_table.hpp"
#include "uav_msgs/msg/uav_status.hpp"
#include "uav_msgs/msg/weather_status.hpp"

using std::placeholders::_1;

struct MsgRecord {
  std::string msg_id;
  std::string ref_msg_id;

  uint8_t flow_type = 0;
  std::string control_type;
  std::string src_id;
  std::string dst_id;

  rclcpp::Time creation_time;
  rclcpp::Time first_seen_bus_time;
  rclcpp::Time delivered_time;
  rclcpp::Time ack_time;

  int forward_count = 0;
  int hop_count = -1;
  int ttl_hops = -1;

  bool delivered = false;
  bool dropped = false;
  std::string drop_reason;
  std::string dropper_id;
  size_t payload_bytes = 0;
  bool generated_counted = false;
};

struct RecoveryEvent
{
  std::string msg_id;
  std::string control_type;
  std::string src_id;
  std::string dst_id;
  int epoch = -1;
  std::string member_id;
  std::string ch_id;
  int task_count = 0;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  rclcpp::Time creation_time;
};

enum class ChargeOutcome {
  PENDING,
  ACCEPTED,
  REJECTED,
  DROPPED,
  TIMEOUT,
  STARTED,
  PREEMPTED,
  ENERGY_DEPLETED
};

struct ChargeRecord
{
  std::string request_msg_id;
  std::string uav_id;
  std::string ugv_id;

  rclcpp::Time request_time;
  rclcpp::Time decision_time;
  rclcpp::Time dock_start_time;
  rclcpp::Time charge_end_time;

  ChargeOutcome outcome = ChargeOutcome::PENDING;
  std::string failure_reason;

  uint8_t role = 0;
  bool role_known = false;
  std::string decision_policy;
  int decision_priority = -1;
  double decision_tte_sec = -1.0;
  double decision_score = -1.0;
  int decision_rank_index = -1;
  int decision_queue_size = -1;
  double decision_ctrl_pdr = -1.0;
  double decision_ctrl_delay_mean_ms = -1.0;
  double decision_ctrl_delay_p95_ms = -1.0;
  std::string decision_ctrl_drop_reasons;
  double request_battery = -1.0;
  double start_battery = -1.0;
  double end_battery = -1.0;
  bool charge_completed = false;
  bool preempted_flag = false;
  int preempt_count = 0;
};

struct UavState
{
  uint8_t charging_state = 0;
  double battery_level = 0.0;
  uint8_t role = 0;
  bool backbone_active = false;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double energy_consumption_rate = 0.0;
};

struct QosAggregate
{
  size_t generated = 0;
  size_t delivered = 0;
  size_t dropped = 0;
  double generated_bytes = 0.0;
  double delivered_bytes = 0.0;
  std::vector<std::pair<rclcpp::Time, double>> delays_ms;
  std::unordered_map<std::string, size_t> drop_reasons;
  rclcpp::Time first_delivered;
  rclcpp::Time last_delivered;
};

struct PreemptionEvent
{
  rclcpp::Time time;
  std::string victim_uav_id;
  std::string winner_uav_id;
  uint8_t victim_role = 0;
  uint8_t winner_role = 0;
  double victim_priority = 0.0;
  double winner_priority = 0.0;
  double delta_priority = 0.0;
  double victim_charge_time_s = 0.0;
  std::string policy;
  std::string ugv_id;
};

// Aggregates network telemetry for traffic, charging, and failures.
class NetworkMonitorNode : public rclcpp::Node
{
public:
  NetworkMonitorNode()
  : Node("network_monitor_node"),
    total_generated_(0),
    total_delivered_(0),
    avg_delay_sec_(0.0),
    total_charging_sessions_(0),
    avg_charge_wait_sec_(0.0)
  {
    run_id_ = this->declare_parameter<std::string>("run_id", "run0");
    output_dir_ = this->declare_parameter<std::string>("output_dir", "log");
    double csv_write_period_sec = this->declare_parameter<double>("csv_write_period_sec", 10.0);
    decision_timeout_sec_ = this->declare_parameter<double>("decision_timeout_sec", 30.0);
    status_sample_period_sec_ = this->declare_parameter<double>("status_sample_period_sec", 1.0);
    queue_stats_period_sec_ = this->declare_parameter<double>("queue_stats_period_sec", 1.0);
    ugv_dock_capacity_ = this->declare_parameter<int>("ugv_dock_capacity", 1);
    max_runtime_sec_ = this->declare_parameter<double>("max_runtime_sec", 0.0);
    stop_on_backbone_loss_ = this->declare_parameter<bool>("stop_on_backbone_loss", false);
    routing_table_empty_shutdown_sec_ =
      this->declare_parameter<double>("routing_table_empty_shutdown_sec", 10.0);
    rate_window_sec_ = this->declare_parameter<double>("network_stats_window_sec", 10.0);
    qos_target_pdr_ = this->declare_parameter<double>("qos_target_pdr", 0.95);
    qos_target_delay_ms_ = this->declare_parameter<double>("qos_target_delay_ms", 200.0);
    qos_target_jitter_ms_ = this->declare_parameter<double>("qos_target_jitter_ms", 50.0);
    qos_weight_pdr_ = this->declare_parameter<double>("qos_weight_pdr", 0.5);
    qos_weight_delay_ = this->declare_parameter<double>("qos_weight_delay", 0.3);
    qos_weight_jitter_ = this->declare_parameter<double>("qos_weight_jitter", 0.2);
    backbone_ids_ = this->declare_parameter<std::vector<std::string>>(
      "backbone_ids", std::vector<std::string>{});
    output_root_ = (std::filesystem::path(output_dir_) / run_id_).string();
    start_time_ = this->now();

    // Listen to traffic generation and delivery for latency metrics.
    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/fanet/network_bus", 100,
      std::bind(&NetworkMonitorNode::trafficCallback, this, _1));

    traffic_raw_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/fanet/network_bus_raw", 100,
      std::bind(&NetworkMonitorNode::trafficRawCallback, this, _1));

    delivered_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/fanet/delivered", 100,
      std::bind(&NetworkMonitorNode::deliveredCallback, this, _1));

    // Charging request/decision streams for queue statistics.
    charge_request_sub_ = this->create_subscription<uav_msgs::msg::ChargeRequest>(
      "/uav_fleet/charge_requests", 100,
      std::bind(&NetworkMonitorNode::chargeRequestCallback, this, _1));

    charge_decision_sub_ = this->create_subscription<uav_msgs::msg::ChargeDecision>(
      "/ugv/charge_decisions", 100,
      std::bind(&NetworkMonitorNode::chargeDecisionCallback, this, _1));

    status_sub_ = this->create_subscription<uav_msgs::msg::UavStatus>(
      "/fanet/status", 200,
      std::bind(&NetworkMonitorNode::statusCallback, this, _1));

    routing_table_sub_ = this->create_subscription<uav_msgs::msg::RoutingTable>(
      "/fanet/routing_table", 50,
      std::bind(&NetworkMonitorNode::routingTableCallback, this, _1));

    weather_sub_ = this->create_subscription<uav_msgs::msg::WeatherStatus>(
      "/environment/weather", 10,
      std::bind(&NetworkMonitorNode::weatherCallback, this, _1));

    queue_event_sub_ = this->create_subscription<std_msgs::msg::String>(
      "/ugv/queue_events", 100,
      std::bind(&NetworkMonitorNode::queueEventCallback, this, _1));

    csv_timer_ = this->create_wall_timer(
      std::chrono::duration<double>(csv_write_period_sec),
      [this]() { this->writeOutputs(false); });

    charge_timeout_timer_ = this->create_wall_timer(
      std::chrono::seconds(1),
      std::bind(&NetworkMonitorNode::checkChargeTimeouts, this));

    status_timeseries_timer_ = this->create_wall_timer(
      std::chrono::duration<double>(status_sample_period_sec_),
      std::bind(&NetworkMonitorNode::writeStatusTimeseriesRow, this));

    weather_timeseries_timer_ = this->create_wall_timer(
      std::chrono::duration<double>(status_sample_period_sec_),
      std::bind(&NetworkMonitorNode::writeWeatherTimeseriesRow, this));

    queue_timeseries_timer_ = this->create_wall_timer(
      std::chrono::duration<double>(queue_stats_period_sec_),
      std::bind(&NetworkMonitorNode::writeChargeQueueTimeseriesRow, this));

    stats_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/network_monitor/stats", 10);
    stats_timer_ = this->create_wall_timer(
      std::chrono::duration<double>(0.5),
      std::bind(&NetworkMonitorNode::publishNetworkStats, this));

    shutdown_check_timer_ = this->create_wall_timer(
      std::chrono::seconds(1),
      std::bind(&NetworkMonitorNode::checkShutdownConditions, this));

    rclcpp::on_shutdown([this]() {
      this->writeOutputs(true);
    });

    RCLCPP_INFO(this->get_logger(), "Network monitor started.");
  }

  ~NetworkMonitorNode()
  {
    writeOutputs(true);
  }

private:
  // ---- Traffic monitoring ----
  // Track first-seen messages to compute end-to-end delay.
  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    rclcpp::Time now = this->now();
    if (!msg->msg_id.empty() &&
        seen_control_msg_ids_.insert(msg->msg_id).second) {
      control_type_counts_[msg->control_type]++;
    }

    if (msg->control_type == "DROP") {
      if (!msg->ref_msg_id.empty()) {
        drop_by_ref_[msg->ref_msg_id] = {msg->drop_reason, msg->src_id};
        if (dropped_ids_.insert(msg->ref_msg_id).second) {
          drop_total_++;
          drop_reason_counts_[msg->drop_reason.empty() ? "UNKNOWN" : msg->drop_reason]++;
          last_drop_time_ = now;
          recordTimestamp(drop_timestamps_, now);
        }

        auto it_charge = charge_records_.find(msg->ref_msg_id);
        if (it_charge != charge_records_.end() &&
            !isTerminalOutcome(it_charge->second.outcome)) {
          it_charge->second.outcome = ChargeOutcome::DROPPED;
          it_charge->second.failure_reason = msg->drop_reason.empty()
            ? "UNKNOWN_DROP"
            : msg->drop_reason;
        }
      }
      return;
    }

    if (msg->control_type == "ACK") {
      if (!msg->ref_msg_id.empty()) {
        if (ack_by_ref_.insert({msg->ref_msg_id, now}).second) {
          ack_total_++;
        }
      }
      return;
    }

    if (msg->control_type == "CHARGE_REQUEST") {
      auto & rec = charge_records_[msg->msg_id];
      rec.request_msg_id = msg->msg_id;
      rec.uav_id = msg->src_id;
      rec.ugv_id = msg->dst_id;
      rec.request_time = now;
      auto role_it = latest_role_by_uav_.find(rec.uav_id);
      if (role_it != latest_role_by_uav_.end()) {
        rec.role = role_it->second;
        rec.role_known = true;
      } else {
        auto st_it = uav_states_.find(rec.uav_id);
        if (st_it != uav_states_.end()) {
          rec.role = st_it->second.role;
          rec.role_known = true;
        }
      }
      auto batt_it = latest_request_battery_by_uav_.find(rec.uav_id);
      if (batt_it != latest_request_battery_by_uav_.end()) {
        rec.request_battery = batt_it->second;
      }
      if (!isTerminalOutcome(rec.outcome) || rec.outcome == ChargeOutcome::PENDING) {
        rec.outcome = ChargeOutcome::PENDING;
      }
      latest_request_by_uav_[rec.uav_id] = msg->msg_id;
    }

    if (msg->flow_type == 1) {
      trackRecoveryEvent(*msg);
    }

    auto & rec = records_[msg->msg_id];
    if (rec.msg_id.empty()) {
      rec.msg_id = msg->msg_id;
      rec.ref_msg_id = msg->ref_msg_id;
      rec.flow_type = msg->flow_type;
      rec.control_type = msg->control_type;
      rec.src_id = msg->src_id;
      rec.dst_id = msg->dst_id;
      rec.creation_time = rclcpp::Time(msg->creation_time);
      rec.payload_bytes = msg->payload.size();
    }
    if (rec.first_seen_bus_time.nanoseconds() == 0) {
      rec.first_seen_bus_time = now;
    }
    rec.forward_count++;

    if (msg->flow_type == 1 && msg->control_type == "FAILURE_EVENT") {
      handleFailureFromTraffic(*msg);
    }
  }

  void trafficRawCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    if (msg->control_type == "DROP" || msg->control_type == "ACK") {
      return;
    }

    auto & rec = records_[msg->msg_id];
    if (rec.msg_id.empty()) {
      rec.msg_id = msg->msg_id;
      rec.ref_msg_id = msg->ref_msg_id;
      rec.flow_type = msg->flow_type;
      rec.control_type = msg->control_type;
      rec.src_id = msg->src_id;
      rec.dst_id = msg->dst_id;
      rec.creation_time = rclcpp::Time(msg->creation_time);
      rec.payload_bytes = msg->payload.size();
      rec.first_seen_bus_time = this->now();
      if (!rec.generated_counted) {
        total_generated_++;
        rec.generated_counted = true;
      }
      last_msg_time_ = this->now();
      recordTimestamp(generated_timestamps_, last_msg_time_);

      RCLCPP_INFO(this->get_logger(),
                  "[GEN] msg_id=%s src=%s dst=%s | total_generated=%zu",
                  msg->msg_id.c_str(), msg->src_id.c_str(), msg->dst_id.c_str(),
                  total_generated_);
    }
  }

  // Compute delivery delay when messages arrive at final destination.
  void deliveredCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    rclcpp::Time delivered_wall_time =
      (msg->last_rx_time.sec == 0 && msg->last_rx_time.nanosec == 0)
      ? this->now()
      : rclcpp::Time(msg->last_rx_time);

    if (msg->control_type == "CHARGE_DECISION" && !msg->ref_msg_id.empty()) {
      auto & charge_rec = charge_records_[msg->ref_msg_id];
      if (charge_rec.request_msg_id.empty()) {
        charge_rec.request_msg_id = msg->ref_msg_id;
      }
      if (charge_rec.uav_id.empty()) {
        charge_rec.uav_id = msg->dst_id;
      }
      charge_rec.ugv_id = msg->src_id;
      charge_rec.decision_time = delivered_wall_time;

      // Detect preemption: payload contains reason=PREEMPTED
      bool is_preemption = msg->payload.find("reason=PREEMPTED") != std::string::npos;
      bool accepted = !is_preemption && msg->payload.find("accepted=0") == std::string::npos;
      if (!isTerminalOutcome(charge_rec.outcome)) {
        if (is_preemption) {
          charge_rec.outcome = ChargeOutcome::PREEMPTED;
        } else {
          charge_rec.outcome = accepted ? ChargeOutcome::ACCEPTED : ChargeOutcome::REJECTED;
        }
      }
      if (!accepted) {
        charge_rec.failure_reason = is_preemption ? "PREEMPTED" : "REJECTED";
      }
      if (is_preemption) {
        charge_rec.preempted_flag = true;
      }
      parseDecisionRationale(msg->payload, charge_rec);
      fillDecisionNetworkContext("CHARGE_DECISION", charge_rec);
      latest_request_by_uav_[charge_rec.uav_id] = msg->ref_msg_id;

      // Record dedicated preemption event
      if (is_preemption) {
        recordPreemptionFromDecision(*msg, delivered_wall_time);
      }
    }

    auto & rec = records_[msg->msg_id];
    if (rec.msg_id.empty()) {
      rec.msg_id = msg->msg_id;
      rec.flow_type = msg->flow_type;
      rec.control_type = msg->control_type;
      rec.src_id = msg->src_id;
      rec.dst_id = msg->dst_id;
      rec.creation_time = rclcpp::Time(msg->creation_time);
      rec.payload_bytes = msg->payload.size();
    }

    if (!rec.generated_counted) {
      total_generated_++;
      rec.generated_counted = true;
    }

    if (rec.delivered) {
      return;
    }

    rec.delivered = true;
    rec.delivered_time = delivered_wall_time;

    rec.hop_count = msg->hop_count;
    rec.ttl_hops = msg->ttl;

    double delay_sec = (rec.delivered_time - rec.creation_time).seconds();
    total_delivered_++;
    avg_delay_sec_ += (delay_sec - avg_delay_sec_) / static_cast<double>(total_delivered_);
    last_delivered_time_ = delivered_wall_time;
    recordTimestamp(delivered_timestamps_, delivered_wall_time);

    if (msg->flow_type == 0 && msg->control_type == "SEARCH_TELEMETRY") {
      telemetry_delivered_++;
      telemetry_avg_delay_sec_ += (delay_sec - telemetry_avg_delay_sec_) / static_cast<double>(telemetry_delivered_);
    }

    delivered_by_flow_control_[msg->flow_type][msg->control_type]++;

    RCLCPP_INFO(this->get_logger(),
                "[DEL] msg_id=%s delay=%.4f s ttl=%u | delivered=%zu / generated=%zu | avg_delay=%.4f s",
                msg->msg_id.c_str(),
                delay_sec,
                msg->ttl,
                total_delivered_,
                total_generated_,
                avg_delay_sec_);
  }

  // ---- Charging monitoring ----

  // Record request time for charge wait calculations.
  void chargeRequestCallback(const uav_msgs::msg::ChargeRequest::SharedPtr msg)
  {
    rclcpp::Time t(msg->stamp);
    request_times_[msg->uav_id] = t;
    latest_role_by_uav_[msg->uav_id] = msg->role;
    latest_request_battery_by_uav_[msg->uav_id] = msg->battery_level;

    RCLCPP_INFO(this->get_logger(),
                "[CHG-REQ] uav=%s role=%u batt=%.1f%% at t=%.1f",
                msg->uav_id.c_str(), msg->role, msg->battery_level,
                t.seconds());
  }

  // Measure wait time between request and accepted slot.
  void chargeDecisionCallback(const uav_msgs::msg::ChargeDecision::SharedPtr msg)
  {
    if (!msg->accepted) {
      RCLCPP_WARN(this->get_logger(),
                  "[CHG-DEC] uav=%s received REJECT decision (policy=%s)",
                  msg->uav_id.c_str(), msg->policy.c_str());
      return;
    }

    auto it = request_times_.find(msg->uav_id);
    if (it == request_times_.end()) {
      // We didn't see the request (e.g. started before monitor), just log
      rclcpp::Time slot_start(msg->slot_start_time);
      RCLCPP_INFO(this->get_logger(),
                  "[CHG-DEC] uav=%s (policy=%s) start=%.1f (no req time to compute wait).",
                  msg->uav_id.c_str(), msg->policy.c_str(),
                  slot_start.seconds());
      return;
    }

    rclcpp::Time t_req = it->second;
    rclcpp::Time t_start(msg->slot_start_time);
    double wait_sec = (t_start - t_req).seconds();
    if (wait_sec < 0.0) wait_sec = 0.0;

    total_charging_sessions_++;
    avg_charge_wait_sec_ +=
      (wait_sec - avg_charge_wait_sec_) / static_cast<double>(total_charging_sessions_);

    RCLCPP_INFO(this->get_logger(),
                "[CHG] uav=%s policy=%s wait=%.2f s | sessions=%zu | avg_wait=%.2f s",
                msg->uav_id.c_str(),
                msg->policy.c_str(),
                wait_sec,
                total_charging_sessions_,
                avg_charge_wait_sec_);

    // Optionally erase so map doesn't grow forever
    request_times_.erase(it);
  }
  
  // Log failures, tracking battery-dead events separately.
  void handleFailureFromTraffic(const uav_msgs::msg::TrafficMessage & msg)
  {
    if (seen_failure_ids_.find(msg.msg_id) != seen_failure_ids_.end()) {
      return;
    }
    seen_failure_ids_.insert(msg.msg_id);

    int failure_type = 0;
    std::string description = msg.payload;
    rclcpp::Time t(msg.creation_time);

    if (!msg.payload.empty()) {
      auto first = msg.payload.find(',');
      auto second = msg.payload.find(',', first == std::string::npos ? first : first + 1);
      if (first != std::string::npos) {
        try {
          failure_type = std::stoi(msg.payload.substr(0, first));
        } catch (const std::exception & e) {
          RCLCPP_WARN(this->get_logger(),
                      "[FAIL] unable to parse failure type from payload='%s' (%s)",
                      msg.payload.c_str(), e.what());
        }
        if (second != std::string::npos) {
          description = msg.payload.substr(second + 1);
        }
      }
    }

    if (failure_type == 1) {  // BATTERY_DEAD
      battery_dead_count_++;
      dead_uavs_.insert(msg.src_id);
      markChargeFailureForUav(msg.src_id, ChargeOutcome::ENERGY_DEPLETED, "ENERGY_DEPLETED");
      RCLCPP_WARN(this->get_logger(),
                  "[FAIL] BATTERY_DEAD from %s at t=%.3f (total=%zu)",
                  msg.src_id.c_str(),
                  t.seconds(),
                  battery_dead_count_);
    } else {
      RCLCPP_WARN(this->get_logger(),
                  "[FAIL] failure from %s type=%u desc=%s",
                  msg.src_id.c_str(),
                  failure_type,
                  description.c_str());
    }
  }

  void statusCallback(const uav_msgs::msg::UavStatus::SharedPtr msg)
  {
    rclcpp::Time now = this->now();
    auto prev = uav_states_[msg->uav_id];
    UavState state;
    state.charging_state = msg->charging_state;
    state.battery_level = msg->battery_level;
    state.role = msg->role;
    state.backbone_active = msg->backbone_active;
    state.x = msg->pose.position.x;
    state.y = msg->pose.position.y;
    state.z = msg->pose.position.z;
    state.energy_consumption_rate = msg->energy_consumption_rate;
    uav_states_[msg->uav_id] = state;

    auto req_it = latest_request_by_uav_.find(msg->uav_id);
    if (req_it == latest_request_by_uav_.end()) {
      return;
    }
    auto rec_it = charge_records_.find(req_it->second);
    if (rec_it == charge_records_.end()) {
      return;
    }
    auto & rec = rec_it->second;
    rec.role = msg->role;
    rec.role_known = true;
    if (isTerminalOutcome(rec.outcome) && rec.outcome != ChargeOutcome::STARTED) {
      return;
    }

    if (rec.outcome == ChargeOutcome::ACCEPTED &&
        (msg->charging_state == 1 || msg->charging_state == 2)) {
      rec.outcome = ChargeOutcome::STARTED;
      rec.dock_start_time = now;
      rec.start_battery = msg->battery_level;
    }

    if (rec.outcome == ChargeOutcome::STARTED) {
      if (!rec.charge_completed &&
          prev.charging_state == 2 &&
          msg->charging_state != 2) {
        rec.charge_completed = true;
        rec.charge_end_time = now;
        rec.end_battery = msg->battery_level;
      }
    }

    if ((rec.outcome == ChargeOutcome::ACCEPTED || rec.outcome == ChargeOutcome::PENDING) &&
        msg->charging_state == 3) {
      rec.outcome = ChargeOutcome::PREEMPTED;
      rec.failure_reason = "RETURNED_BEFORE_DOCK";
    }
  }

  void checkChargeTimeouts()
  {
    rclcpp::Time now = this->now();
    for (auto & [id, rec] : charge_records_) {
      if ((rec.outcome == ChargeOutcome::PENDING || rec.outcome == ChargeOutcome::ACCEPTED) &&
          rec.request_time.nanoseconds() != 0 &&
          rec.decision_time.nanoseconds() == 0) {
        double wait_sec = (now - rec.request_time).seconds();
        if (wait_sec > decision_timeout_sec_) {
          rec.outcome = ChargeOutcome::TIMEOUT;
          rec.failure_reason = "NO_DECISION";
        }
      }
    }
  }

  // ---- Network stats publishing ----
  void recordTimestamp(std::deque<rclcpp::Time> & timestamps, const rclcpp::Time & now)
  {
    timestamps.push_back(now);
    pruneTimestamps(timestamps, now);
  }

  void pruneTimestamps(std::deque<rclcpp::Time> & timestamps, const rclcpp::Time & now)
  {
    rclcpp::Time cutoff = now - rclcpp::Duration::from_seconds(rate_window_sec_);
    while (!timestamps.empty() && timestamps.front() < cutoff) {
      timestamps.pop_front();
    }
  }

  double ageSeconds(const rclcpp::Time & stamp, const rclcpp::Time & now) const
  {
    if (stamp.nanoseconds() == 0) {
      return -1.0;
    }
    double age = (now - stamp).seconds();
    return age < 0.0 ? 0.0 : age;
  }

  void publishNetworkStats()
  {
    if (!stats_pub_) {
      return;
    }
    rclcpp::Time now = this->now();
    pruneTimestamps(generated_timestamps_, now);
    pruneTimestamps(drop_timestamps_, now);
    pruneTimestamps(delivered_timestamps_, now);

    double msg_rate = rate_window_sec_ > 0.0
      ? static_cast<double>(generated_timestamps_.size()) / rate_window_sec_
      : 0.0;
    double drop_rate = rate_window_sec_ > 0.0
      ? static_cast<double>(drop_timestamps_.size()) / rate_window_sec_
      : 0.0;
    double delivered_rate = rate_window_sec_ > 0.0
      ? static_cast<double>(delivered_timestamps_.size()) / rate_window_sec_
      : 0.0;

    std::ostringstream out;
    out << "{";
    out << "\"generated_total\":" << total_generated_ << ",";
    out << "\"delivered_total\":" << total_delivered_ << ",";
    out << "\"drop_total\":" << drop_total_ << ",";
    out << "\"ack_total\":" << ack_total_ << ",";
    out << "\"generated_rate\":" << msg_rate << ",";
    out << "\"delivered_rate\":" << delivered_rate << ",";
    out << "\"drop_rate\":" << drop_rate << ",";
    out << "\"window_sec\":" << rate_window_sec_ << ",";
    out << "\"last_msg_age\":" << ageSeconds(last_msg_time_, now) << ",";
    out << "\"last_drop_age\":" << ageSeconds(last_drop_time_, now) << ",";
    out << "\"last_delivered_age\":" << ageSeconds(last_delivered_time_, now) << ",";
    out << "\"control_type_counts\":{";
    bool first = true;
    for (const auto & pair : control_type_counts_) {
      if (!first) {
        out << ",";
      }
      first = false;
      out << "\"" << pair.first << "\":" << pair.second;
    }
    out << "},";
    out << "\"drop_reason_counts\":{";
    first = true;
    for (const auto & pair : drop_reason_counts_) {
      if (!first) {
        out << ",";
      }
      first = false;
      out << "\"" << pair.first << "\":" << pair.second;
    }
    out << "}";
    out << "}";

    std_msgs::msg::String msg;
    msg.data = out.str();
    stats_pub_->publish(msg);
  }

  void checkShutdownConditions()
  {
    if (max_runtime_sec_ > 0.0) {
      const double elapsed = (this->now() - start_time_).seconds();
      if (elapsed >= max_runtime_sec_) {
        RCLCPP_WARN(this->get_logger(),
                    "[SHUTDOWN] max_runtime_sec reached (%.1f >= %.1f).",
                    elapsed,
                    max_runtime_sec_);
        rclcpp::shutdown();
        return;
      }
    }

    if (stop_on_backbone_loss_ && !backbone_ids_.empty()) {
      bool all_dead = true;
      for (const auto & id : backbone_ids_) {
        if (dead_uavs_.find(id) == dead_uavs_.end()) {
          all_dead = false;
          break;
        }
      }
      if (all_dead) {
        RCLCPP_WARN(this->get_logger(),
                    "[SHUTDOWN] all backbone UAVs reported dead.");
        rclcpp::shutdown();
      }
    }

    if (routing_table_empty_shutdown_sec_ > 0.0 && routing_table_seen_) {
      if (routing_table_empty_since_.nanoseconds() > 0) {
        const double empty_duration = (this->now() - routing_table_empty_since_).seconds();
        if (empty_duration >= routing_table_empty_shutdown_sec_) {
          RCLCPP_WARN(this->get_logger(),
                      "[SHUTDOWN] routing table empty for %.1f sec (threshold %.1f).",
                      empty_duration,
                      routing_table_empty_shutdown_sec_);
          rclcpp::shutdown();
        }
      }
    }
  }

  bool isTerminalOutcome(ChargeOutcome outcome) const
  {
    return outcome == ChargeOutcome::STARTED ||
           outcome == ChargeOutcome::REJECTED ||
           outcome == ChargeOutcome::DROPPED ||
           outcome == ChargeOutcome::TIMEOUT ||
           outcome == ChargeOutcome::PREEMPTED ||
           outcome == ChargeOutcome::ENERGY_DEPLETED;
  }

  std::string chargeOutcomeToString(ChargeOutcome outcome) const
  {
    switch (outcome) {
      case ChargeOutcome::PENDING: return "PENDING";
      case ChargeOutcome::ACCEPTED: return "ACCEPTED";
      case ChargeOutcome::REJECTED: return "REJECTED";
      case ChargeOutcome::DROPPED: return "ROUTING_DROP";
      case ChargeOutcome::TIMEOUT: return "TIMEOUT";
      case ChargeOutcome::STARTED: return "STARTED";
      case ChargeOutcome::PREEMPTED: return "PREEMPTED";
      case ChargeOutcome::ENERGY_DEPLETED: return "ENERGY_DEPLETED";
      default: return "UNKNOWN";
    }
  }

  void markChargeFailureForUav(const std::string & uav_id, ChargeOutcome outcome, const std::string & reason)
  {
    auto it_req = latest_request_by_uav_.find(uav_id);
    if (it_req == latest_request_by_uav_.end()) {
      return;
    }
    auto it_rec = charge_records_.find(it_req->second);
    if (it_rec == charge_records_.end()) {
      return;
    }
    if (isTerminalOutcome(it_rec->second.outcome)) {
      return;
    }
    it_rec->second.outcome = outcome;
    it_rec->second.failure_reason = reason;
  }

  void routingTableCallback(const uav_msgs::msg::RoutingTable::SharedPtr msg)
  {
    routing_table_seen_ = true;
    if (!msg) {
      return;
    }
    if (!msg->destinations.empty()) {
      routing_table_empty_since_ = rclcpp::Time(0, 0, this->get_clock()->get_clock_type());
      return;
    }
    if (routing_table_empty_since_.nanoseconds() == 0) {
      routing_table_empty_since_ = this->now();
    }
  }

  void weatherCallback(const uav_msgs::msg::WeatherStatus::SharedPtr msg)
  {
    if (!msg) {
      return;
    }
    std::string new_regime = msg->regime;
    if (!new_regime.empty() && new_regime != current_weather_regime_ &&
        !current_weather_regime_.empty()) {
      RCLCPP_INFO(this->get_logger(),
                  "[WEATHER] regime transition: %s -> %s at t=%.3f",
                  current_weather_regime_.c_str(),
                  new_regime.c_str(),
                  this->now().seconds());
    }
    current_weather_regime_ = new_regime;
    current_weather_temp_c_ = msg->temperature_c;
    current_weather_wind_speed_ = msg->wind_speed;
    current_weather_wind_dir_ = msg->wind_direction_deg;
    current_weather_rain_ = msg->rain_intensity;
    weather_received_ = true;
  }

  void parseDecisionRationale(const std::string & payload, ChargeRecord & rec)
  {
    if (payload.empty()) {
      return;
    }

    std::stringstream ss(payload);
    std::string token;
    while (std::getline(ss, token, ';')) {
      if (token.empty()) {
        continue;
      }
      auto sep = token.find('=');
      if (sep == std::string::npos) {
        continue;
      }
      std::string key = token.substr(0, sep);
      std::string value = token.substr(sep + 1);
      if (key == "policy") {
        rec.decision_policy = value;
        continue;
      }
      try {
        if (key == "priority") {
          rec.decision_priority = std::stoi(value);
        } else if (key == "rank_index") {
          rec.decision_rank_index = std::stoi(value);
        } else if (key == "queue_size") {
          rec.decision_queue_size = std::stoi(value);
        } else if (key == "tte_sec") {
          rec.decision_tte_sec = std::stod(value);
        } else if (key == "score") {
          rec.decision_score = std::stod(value);
        }
      } catch (const std::exception &) {
        continue;
      }
    }
  }

  void fillDecisionNetworkContext(const std::string & control_type, ChargeRecord & rec)
  {
    auto qos_stats = buildQosStats();
    std::string key = "1:" + control_type;
    auto it = qos_stats.find(key);
    if (it == qos_stats.end()) {
      return;
    }
    QosMetrics metrics = finalizeQosStats(it->second, 1, control_type);
    rec.decision_ctrl_pdr = metrics.pdr;
    rec.decision_ctrl_delay_mean_ms = metrics.delay_mean_ms;
    rec.decision_ctrl_delay_p95_ms = metrics.delay_p95_ms;
    rec.decision_ctrl_drop_reasons = formatDropReasons(it->second.drop_reasons);
  }

  std::string formatDropReasons(const std::unordered_map<std::string, size_t> & reasons) const
  {
    if (reasons.empty()) {
      return "";
    }
    std::ostringstream out;
    bool first = true;
    for (const auto & pair : reasons) {
      if (!first) {
        out << "|";
      }
      first = false;
      out << pair.first << ":" << pair.second;
    }
    return out.str();
  }

  // Parse preemption event from queue_events topic
  void queueEventCallback(const std_msgs::msg::String::SharedPtr msg)
  {
    if (msg->data.find("event=PREEMPTION") == std::string::npos) {
      return;
    }
    PreemptionEvent pe;
    pe.time = this->now();

    // Parse key=value pairs from the event string
    std::istringstream iss(msg->data);
    std::string token;
    while (iss >> token) {
      auto sep = token.find('=');
      if (sep == std::string::npos) continue;
      std::string key = token.substr(0, sep);
      std::string value = token.substr(sep + 1);
      if (key == "victim_uav_id") pe.victim_uav_id = value;
      else if (key == "winner_uav_id") pe.winner_uav_id = value;
      else if (key == "victim_role") { try { pe.victim_role = static_cast<uint8_t>(std::stoi(value)); } catch(...) {} }
      else if (key == "winner_role") { try { pe.winner_role = static_cast<uint8_t>(std::stoi(value)); } catch(...) {} }
      else if (key == "victim_priority") { try { pe.victim_priority = std::stod(value); } catch(...) {} }
      else if (key == "winner_priority") { try { pe.winner_priority = std::stod(value); } catch(...) {} }
      else if (key == "delta_priority") { try { pe.delta_priority = std::stod(value); } catch(...) {} }
      else if (key == "victim_charge_time_s") { try { pe.victim_charge_time_s = std::stod(value); } catch(...) {} }
      else if (key == "policy") pe.policy = value;
      else if (key == "stamp") { /* use our own time */ }
    }

    preemption_events_.push_back(pe);
    RCLCPP_INFO(this->get_logger(),
                "[PREEMPT] victim=%s winner=%s delta=%.3f charge_time=%.1fs policy=%s",
                pe.victim_uav_id.c_str(), pe.winner_uav_id.c_str(),
                pe.delta_priority, pe.victim_charge_time_s, pe.policy.c_str());
  }

  // Record preemption from a delivered CHARGE_DECISION with reason=PREEMPTED
  void recordPreemptionFromDecision(const uav_msgs::msg::TrafficMessage & msg,
                                    const rclcpp::Time & delivered_time)
  {
    // The victim is the dst_id of the preemption decision
    std::string victim_id = msg.dst_id;

    // Update the latest charge record for this UAV
    auto req_it = latest_request_by_uav_.find(victim_id);
    if (req_it != latest_request_by_uav_.end()) {
      auto & rec = charge_records_[req_it->second];
      rec.preempted_flag = true;
      rec.preempt_count++;
      if (!isTerminalOutcome(rec.outcome) || rec.outcome == ChargeOutcome::ACCEPTED) {
        rec.outcome = ChargeOutcome::PREEMPTED;
        rec.failure_reason = "PREEMPTED";
      }
    }
  }

  void writeOutputs(bool final_flush)
  {
    reconcileCausality();
    writeMessagesCsv(final_flush);
    writeQosMetricsCsv(final_flush);
    writeChargeEventsCsv(final_flush);
    writeRecoveryEventsCsv(final_flush);
    writePreemptionEventsCsv(final_flush);
    writeChargeQueueTimeseriesRow();
    writeStatusTimeseriesRow();
    writeWeatherTimeseriesRow();
    writeSummaryJson();
  }

  void reconcileCausality()
  {
    for (auto & [msg_id, rec] : records_) {
      if (drop_by_ref_.count(msg_id)) {
        rec.dropped = true;
        rec.drop_reason = drop_by_ref_[msg_id].first;
        rec.dropper_id = drop_by_ref_[msg_id].second;
      }
      if (ack_by_ref_.count(msg_id)) {
        rec.ack_time = ack_by_ref_[msg_id];
      }
    }
  }

  void writeMessagesCsv(bool final_flush)
  {
    std::error_code ec;
    std::filesystem::create_directories(output_root_, ec);
    if (ec) {
      RCLCPP_WARN(this->get_logger(), "Failed to create output directory %s: %s",
                  output_root_.c_str(), ec.message().c_str());
      return;
    }

    auto path = std::filesystem::path(output_root_) / "messages.csv";
    bool need_header = !messages_file_initialized_;
    std::ofstream out(path,
                      messages_file_initialized_ ? std::ios::app : (std::ios::out | std::ios::trunc));
    if (!out.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open %s for writing", path.string().c_str());
      return;
    }
    messages_file_initialized_ = true;

    if (need_header) {
      out << "run_id,msg_id,flow_type,control_type,src_id,dst_id,"
          << "creation_time_s,delivered_time_s,delivered,e2e_delay_ms,forward_count,hop_count,ttl_hops,"
          << "payload_bytes,dropped,drop_reason,dropper_id,ack_time" << std::endl;
    }

    for (const auto & [msg_id, rec] : records_) {
      if (!final_flush && exported_messages_.count(msg_id)) {
        continue;
      }
      double delay_ms = rec.delivered
        ? (rec.delivered_time - rec.creation_time).seconds() * 1000.0
        : -1.0;
      double ack_time = rec.ack_time.nanoseconds() == 0 ? -1.0 : rec.ack_time.seconds();
      double delivered_time = rec.delivered ? rec.delivered_time.seconds() : -1.0;
      out << run_id_ << ','
          << msg_id << ','
          << static_cast<int>(rec.flow_type) << ','
          << rec.control_type << ','
          << rec.src_id << ','
          << rec.dst_id << ','
          << rec.creation_time.seconds() << ','
          << delivered_time << ','
          << (rec.delivered ? "true" : "false") << ','
          << delay_ms << ','
          << rec.forward_count << ','
          << rec.hop_count << ','
          << rec.ttl_hops << ','
          << rec.payload_bytes << ','
          << (rec.dropped ? "true" : "false") << ','
          << rec.drop_reason << ','
          << rec.dropper_id << ','
          << ack_time
          << std::endl;
      exported_messages_.insert(msg_id);
    }
  }

  void writeQosMetricsCsv(bool final_flush)
  {
    std::error_code ec;
    std::filesystem::create_directories(output_root_, ec);
    if (ec) {
      RCLCPP_WARN(this->get_logger(), "Failed to create output directory %s: %s",
                  output_root_.c_str(), ec.message().c_str());
      return;
    }

    auto path = std::filesystem::path(output_root_) / "qos_metrics.csv";
    bool need_header = !qos_metrics_file_initialized_;
    std::ofstream out(path,
                      qos_metrics_file_initialized_ ? std::ios::app : (std::ios::out | std::ios::trunc));
    if (!out.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open %s for writing", path.string().c_str());
      return;
    }
    qos_metrics_file_initialized_ = true;

    if (need_header) {
      out << "run_id,flow_type,control_type,generated,delivered,dropped,pdr,"
          << "delay_mean_ms,delay_p95_ms,jitter_ms,throughput_bps,generated_bps,qos_score"
          << std::endl;
    }

    auto qos_stats = buildQosStats();
    for (const auto & [key, stats] : qos_stats) {
      if (!final_flush && exported_qos_keys_.count(key)) {
        continue;
      }
      auto sep = key.find(':');
      std::string flow_str = key.substr(0, sep);
      std::string ctrl_str = (sep == std::string::npos) ? "" : key.substr(sep + 1);
      int flow_val = 0;
      try {
        flow_val = std::stoi(flow_str);
      } catch (...) {
        flow_val = 0;
      }
      auto metrics = finalizeQosStats(stats, flow_val, ctrl_str);
      out << run_id_ << ","
          << metrics.flow_type << ","
          << metrics.control_type << ","
          << metrics.generated << ","
          << metrics.delivered << ","
          << metrics.dropped << ","
          << metrics.pdr << ","
          << metrics.delay_mean_ms << ","
          << metrics.delay_p95_ms << ","
          << metrics.jitter_ms << ","
          << metrics.throughput_bps << ","
          << metrics.generated_bps << ","
          << metrics.qos_score
          << std::endl;
      exported_qos_keys_.insert(key);
    }
  }

  void writeChargeEventsCsv(bool final_flush)
  {
    std::error_code ec;
    std::filesystem::create_directories(output_root_, ec);
    if (ec) {
      RCLCPP_WARN(this->get_logger(), "Failed to create output directory %s: %s",
                  output_root_.c_str(), ec.message().c_str());
      return;
    }

    auto path = std::filesystem::path(output_root_) / "charge_events.csv";
    bool need_header = !charge_events_file_initialized_;
    std::ofstream out(path,
                      charge_events_file_initialized_ ? std::ios::app : (std::ios::out | std::ios::trunc));
    if (!out.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open %s for writing", path.string().c_str());
      return;
    }
    charge_events_file_initialized_ = true;

    if (need_header) {
      out << "run_id,request_msg_id,uav_id,ugv_id,role,outcome,failure_reason,"
          << "request_time,decision_time,dock_start_time,charge_end_time,"
          << "decision_latency_ms,waiting_time_ms,charge_duration_ms,"
          << "charge_completed,request_battery,start_battery,end_battery,energy_recovered,"
          << "preempted_flag,preempt_count,"
          << "decision_policy,decision_priority,decision_tte_sec,decision_score,"
          << "decision_rank_index,decision_queue_size,"
          << "decision_ctrl_pdr,decision_ctrl_delay_mean_ms,decision_ctrl_delay_p95_ms,"
          << "decision_ctrl_drop_reasons" << std::endl;
    }

    for (const auto & [id, rec] : charge_records_) {
      if (!final_flush && exported_charge_requests_.count(id)) {
        continue;
      }
      double decision_latency_ms = (rec.decision_time.nanoseconds() != 0 && rec.request_time.nanoseconds() != 0)
        ? (rec.decision_time - rec.request_time).seconds() * 1000.0
        : -1.0;
      double waiting_time_ms = (rec.dock_start_time.nanoseconds() != 0 && rec.request_time.nanoseconds() != 0)
        ? (rec.dock_start_time - rec.request_time).seconds() * 1000.0
        : -1.0;
      double charge_duration_ms = (rec.charge_completed && rec.charge_end_time.nanoseconds() != 0 && rec.dock_start_time.nanoseconds() != 0)
        ? (rec.charge_end_time - rec.dock_start_time).seconds() * 1000.0
        : -1.0;
      double energy_recovered = (rec.charge_completed && rec.end_battery >= 0.0 && rec.start_battery >= 0.0)
        ? (rec.end_battery - rec.start_battery)
        : -1.0;

      out << run_id_ << ','
          << rec.request_msg_id << ','
          << rec.uav_id << ','
          << rec.ugv_id << ','
          << (rec.role_known ? static_cast<int>(rec.role) : -1) << ','
          << chargeOutcomeToString(rec.outcome) << ','
          << rec.failure_reason << ','
          << rec.request_time.seconds() << ','
          << rec.decision_time.seconds() << ','
          << rec.dock_start_time.seconds() << ','
          << (rec.charge_end_time.nanoseconds() != 0 ? rec.charge_end_time.seconds() : -1.0) << ','
          << decision_latency_ms << ','
          << waiting_time_ms << ','
          << charge_duration_ms << ','
          << (rec.charge_completed ? "true" : "false") << ','
          << rec.request_battery << ','
          << rec.start_battery << ','
          << rec.end_battery << ','
          << energy_recovered << ','
          << (rec.preempted_flag ? "true" : "false") << ','
          << rec.preempt_count << ','
          << rec.decision_policy << ','
          << rec.decision_priority << ','
          << rec.decision_tte_sec << ','
          << rec.decision_score << ','
          << rec.decision_rank_index << ','
          << rec.decision_queue_size << ','
          << rec.decision_ctrl_pdr << ','
          << rec.decision_ctrl_delay_mean_ms << ','
          << rec.decision_ctrl_delay_p95_ms << ','
          << rec.decision_ctrl_drop_reasons
          << std::endl;
      exported_charge_requests_.insert(id);
    }
  }

  void writePreemptionEventsCsv(bool final_flush)
  {
    if (preemption_events_.empty()) {
      return;
    }

    std::error_code ec;
    std::filesystem::create_directories(output_root_, ec);
    if (ec) {
      RCLCPP_WARN(this->get_logger(), "Failed to create output directory %s: %s",
                  output_root_.c_str(), ec.message().c_str());
      return;
    }

    auto path = std::filesystem::path(output_root_) / "preemption_events.csv";
    bool need_header = !preemption_events_file_initialized_;
    std::ofstream out(path,
                      preemption_events_file_initialized_ ? std::ios::app
                                                          : (std::ios::out | std::ios::trunc));
    if (!out.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open %s for writing", path.string().c_str());
      return;
    }
    preemption_events_file_initialized_ = true;

    if (need_header) {
      out << "run_id,time,victim_uav_id,winner_uav_id,"
          << "victim_role,winner_role,"
          << "victim_priority,winner_priority,delta_priority,"
          << "victim_charge_time_s,policy" << std::endl;
    }

    for (size_t i = exported_preemption_count_; i < preemption_events_.size(); ++i) {
      const auto & pe = preemption_events_[i];
      out << run_id_ << ','
          << pe.time.seconds() << ','
          << pe.victim_uav_id << ','
          << pe.winner_uav_id << ','
          << static_cast<int>(pe.victim_role) << ','
          << static_cast<int>(pe.winner_role) << ','
          << pe.victim_priority << ','
          << pe.winner_priority << ','
          << pe.delta_priority << ','
          << pe.victim_charge_time_s << ','
          << pe.policy
          << std::endl;
    }
    if (final_flush) {
      exported_preemption_count_ = preemption_events_.size();
    } else {
      exported_preemption_count_ = preemption_events_.size();
    }
  }

  void writeRecoveryEventsCsv(bool final_flush)
  {
    std::error_code ec;
    std::filesystem::create_directories(output_root_, ec);
    if (ec) {
      RCLCPP_WARN(this->get_logger(), "Failed to create output directory %s: %s",
                  output_root_.c_str(), ec.message().c_str());
      return;
    }

    auto path = std::filesystem::path(output_root_) / "recovery_events.csv";
    bool need_header = !recovery_events_file_initialized_;
    std::ofstream out(path,
                      recovery_events_file_initialized_ ? std::ios::app : (std::ios::out | std::ios::trunc));
    if (!out.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open %s for writing", path.string().c_str());
      return;
    }
    recovery_events_file_initialized_ = true;

    if (need_header) {
      out << "run_id,msg_id,control_type,src_id,dst_id,epoch,member_id,ch_id,"
          << "task_count,x,y,z,creation_time" << std::endl;
    }

    for (const auto & [msg_id, rec] : recovery_events_) {
      if (!final_flush && exported_recovery_events_.count(msg_id)) {
        continue;
      }
      out << run_id_ << ','
          << rec.msg_id << ','
          << rec.control_type << ','
          << rec.src_id << ','
          << rec.dst_id << ','
          << rec.epoch << ','
          << rec.member_id << ','
          << rec.ch_id << ','
          << rec.task_count << ','
          << rec.x << ','
          << rec.y << ','
          << rec.z << ','
          << rec.creation_time.seconds()
          << std::endl;
      exported_recovery_events_.insert(msg_id);
    }
  }

  double percentile(std::vector<double> values, double pct) const
  {
    if (values.empty()) {
      return -1.0;
    }
    std::sort(values.begin(), values.end());
    double idx = (pct / 100.0) * (values.size() - 1);
    size_t lower = static_cast<size_t>(std::floor(idx));
    size_t upper = static_cast<size_t>(std::ceil(idx));
    if (upper >= values.size()) {
      upper = values.size() - 1;
    }
    double weight = idx - lower;
    return values[lower] * (1.0 - weight) + values[upper] * weight;
  }

  double computeJitterMs(std::vector<std::pair<rclcpp::Time, double>> delays) const
  {
    if (delays.size() < 2) {
      return -1.0;
    }
    std::sort(delays.begin(), delays.end(),
              [](const auto & a, const auto & b) { return a.first < b.first; });
    double sum = 0.0;
    for (size_t i = 1; i < delays.size(); ++i) {
      sum += std::abs(delays[i].second - delays[i - 1].second);
    }
    return sum / static_cast<double>(delays.size() - 1);
  }

  struct QosMetrics
  {
    int flow_type = 0;
    std::string control_type;
    size_t generated = 0;
    size_t delivered = 0;
    size_t dropped = 0;
    double pdr = 0.0;
    double delay_mean_ms = -1.0;
    double delay_p95_ms = -1.0;
    double jitter_ms = -1.0;
    double throughput_bps = -1.0;
    double generated_bps = -1.0;
    double qos_score = 0.0;
  };

  QosMetrics finalizeQosStats(const QosAggregate & stats,
                              int flow_type,
                              const std::string & control_type) const
  {
    QosMetrics metrics;
    metrics.flow_type = flow_type;
    metrics.control_type = control_type;
    metrics.generated = stats.generated;
    metrics.delivered = stats.delivered;
    metrics.dropped = stats.dropped;
    metrics.pdr = stats.generated == 0
      ? 0.0
      : static_cast<double>(stats.delivered) / static_cast<double>(stats.generated);

    std::vector<double> delays_ms;
    delays_ms.reserve(stats.delays_ms.size());
    for (const auto & [time, delay_ms] : stats.delays_ms) {
      (void)time;
      delays_ms.push_back(delay_ms);
    }
    if (!delays_ms.empty()) {
      double sum = 0.0;
      for (double v : delays_ms) sum += v;
      metrics.delay_mean_ms = sum / static_cast<double>(delays_ms.size());
      metrics.delay_p95_ms = percentile(delays_ms, 95.0);
    }
    metrics.jitter_ms = computeJitterMs(stats.delays_ms);

    double duration_sec = -1.0;
    if (stats.first_delivered.nanoseconds() != 0 &&
        stats.last_delivered.nanoseconds() != 0) {
      duration_sec = (stats.last_delivered - stats.first_delivered).seconds();
    }
    if (duration_sec <= 0.0) {
      duration_sec = (this->now() - start_time_).seconds();
    }
    if (duration_sec > 0.0) {
      metrics.throughput_bps = (stats.delivered_bytes * 8.0) / duration_sec;
      metrics.generated_bps = (stats.generated_bytes * 8.0) / duration_sec;
    }

    double score_pdr = qos_target_pdr_ <= 0.0
      ? 0.0
      : std::min(metrics.pdr / qos_target_pdr_, 1.0);
    double score_delay = (metrics.delay_mean_ms <= 0.0 || qos_target_delay_ms_ <= 0.0)
      ? 0.0
      : std::min(qos_target_delay_ms_ / metrics.delay_mean_ms, 1.0);
    double score_jitter = (metrics.jitter_ms <= 0.0 || qos_target_jitter_ms_ <= 0.0)
      ? 0.0
      : std::min(qos_target_jitter_ms_ / metrics.jitter_ms, 1.0);

    double weight_sum = qos_weight_pdr_ + qos_weight_delay_ + qos_weight_jitter_;
    if (weight_sum <= 0.0) {
      metrics.qos_score = 0.0;
    } else {
      metrics.qos_score = (score_pdr * qos_weight_pdr_ +
                           score_delay * qos_weight_delay_ +
                           score_jitter * qos_weight_jitter_) / weight_sum;
    }
    return metrics;
  }

  std::unordered_map<std::string, QosAggregate> buildQosStats()
  {
    std::unordered_map<std::string, QosAggregate> stats_map;
    for (const auto & [msg_id, rec] : records_) {
      std::string key = std::to_string(rec.flow_type) + ":" + rec.control_type;
      auto & stats = stats_map[key];
      stats.generated++;
      stats.generated_bytes += static_cast<double>(rec.payload_bytes);

      if (rec.delivered) {
        stats.delivered++;
        stats.delivered_bytes += static_cast<double>(rec.payload_bytes);
        double delay_ms = (rec.delivered_time - rec.creation_time).seconds() * 1000.0;
        stats.delays_ms.emplace_back(rec.delivered_time, delay_ms);
        if (stats.first_delivered.nanoseconds() == 0) {
          stats.first_delivered = rec.delivered_time;
        }
        stats.last_delivered = rec.delivered_time;
      }

      if (rec.dropped) {
        stats.dropped++;
        if (!rec.drop_reason.empty()) {
          stats.drop_reasons[rec.drop_reason]++;
        }
      }
    }
    return stats_map;
  }

  void writeStatusTimeseriesRow()
  {
    std::error_code ec;
    std::filesystem::create_directories(output_root_, ec);
    if (ec) {
      RCLCPP_WARN(this->get_logger(), "Failed to create output directory %s: %s",
                  output_root_.c_str(), ec.message().c_str());
      return;
    }

    auto path = std::filesystem::path(output_root_) / "status_timeseries.csv";
    bool need_header = !status_timeseries_file_initialized_;
    std::ofstream out(path,
                      status_timeseries_file_initialized_ ? std::ios::app : (std::ios::out | std::ios::trunc));
    if (!out.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open %s for writing", path.string().c_str());
      return;
    }
    status_timeseries_file_initialized_ = true;

    if (need_header) {
      out << "run_id,time,uav_id,role,charging_state,battery_level,backbone_active,x,y,z,energy_consumption_rate" << std::endl;
    }

    double t = this->now().seconds();
    for (const auto & [uav_id, st] : uav_states_) {
      out << run_id_ << ","
          << t << ","
          << uav_id << ","
          << static_cast<int>(st.role) << ","
          << static_cast<int>(st.charging_state) << ","
          << st.battery_level << ","
          << (st.backbone_active ? "true" : "false") << ","
          << st.x << ","
          << st.y << ","
          << st.z << ","
          << st.energy_consumption_rate
          << std::endl;
    }
  }

  void writeChargeQueueTimeseriesRow()
  {
    std::error_code ec;
    std::filesystem::create_directories(output_root_, ec);
    if (ec) {
      RCLCPP_WARN(this->get_logger(), "Failed to create output directory %s: %s",
                  output_root_.c_str(), ec.message().c_str());
      return;
    }

    auto path = std::filesystem::path(output_root_) / "charge_queue_timeseries.csv";
    bool need_header = !charge_queue_timeseries_file_initialized_;
    std::ofstream out(path,
                      charge_queue_timeseries_file_initialized_ ? std::ios::app
                                                                : (std::ios::out | std::ios::trunc));
    if (!out.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open %s for writing", path.string().c_str());
      return;
    }
    charge_queue_timeseries_file_initialized_ = true;

    if (need_header) {
      out << "run_id,time,queue_length,queue_length_ch,queue_length_member,queue_length_unknown,"
          << "active_charging,ugv_dock_capacity,ugv_dock_utilization,"
          << "mean_wait_ch_ms,mean_wait_member_ms" << std::endl;
    }

    size_t queue_length = 0;
    size_t queue_ch = 0;
    size_t queue_member = 0;
    size_t queue_unknown = 0;
    std::vector<double> wait_ch_ms;
    std::vector<double> wait_member_ms;

    for (const auto & [id, rec] : charge_records_) {
      if (rec.request_time.nanoseconds() == 0) {
        continue;
      }
      bool started = rec.dock_start_time.nanoseconds() != 0;
      if (!started && !isTerminalOutcome(rec.outcome)) {
        queue_length++;
        if (rec.role_known) {
          if (rec.role == 1) {
            queue_ch++;
          } else {
            queue_member++;
          }
        } else {
          queue_unknown++;
        }
      }

      if (started) {
        double wait_ms = (rec.dock_start_time - rec.request_time).seconds() * 1000.0;
        if (wait_ms < 0.0) {
          wait_ms = 0.0;
        }
        if (rec.role_known) {
          if (rec.role == 1) {
            wait_ch_ms.push_back(wait_ms);
          } else {
            wait_member_ms.push_back(wait_ms);
          }
        }
      }
    }

    size_t active_charging = 0;
    for (const auto & [uav_id, st] : uav_states_) {
      if (st.charging_state == 2) {
        active_charging++;
      }
    }

    double mean_wait_ch = -1.0;
    if (!wait_ch_ms.empty()) {
      double sum = 0.0;
      for (double v : wait_ch_ms) {
        sum += v;
      }
      mean_wait_ch = sum / static_cast<double>(wait_ch_ms.size());
    }

    double mean_wait_member = -1.0;
    if (!wait_member_ms.empty()) {
      double sum = 0.0;
      for (double v : wait_member_ms) {
        sum += v;
      }
      mean_wait_member = sum / static_cast<double>(wait_member_ms.size());
    }

    double utilization = -1.0;
    if (ugv_dock_capacity_ > 0) {
      utilization = static_cast<double>(active_charging) / static_cast<double>(ugv_dock_capacity_);
    }

    out << run_id_ << ","
        << this->now().seconds() << ","
        << queue_length << ","
        << queue_ch << ","
        << queue_member << ","
        << queue_unknown << ","
        << active_charging << ","
        << ugv_dock_capacity_ << ","
        << utilization << ","
        << mean_wait_ch << ","
        << mean_wait_member
        << std::endl;
  }

  void writeWeatherTimeseriesRow()
  {
    if (!weather_received_) {
      return;
    }

    std::error_code ec;
    std::filesystem::create_directories(output_root_, ec);
    if (ec) {
      RCLCPP_WARN(this->get_logger(), "Failed to create output directory %s: %s",
                  output_root_.c_str(), ec.message().c_str());
      return;
    }

    auto path = std::filesystem::path(output_root_) / "weather_timeseries.csv";
    bool need_header = !weather_timeseries_file_initialized_;
    std::ofstream out(path,
                      weather_timeseries_file_initialized_ ? std::ios::app : (std::ios::out | std::ios::trunc));
    if (!out.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open %s for writing", path.string().c_str());
      return;
    }
    weather_timeseries_file_initialized_ = true;

    if (need_header) {
      out << "run_id,time,regime,temperature_c,wind_speed,wind_direction_deg,rain_intensity" << std::endl;
    }

    out << run_id_ << ","
        << this->now().seconds() << ","
        << current_weather_regime_ << ","
        << current_weather_temp_c_ << ","
        << current_weather_wind_speed_ << ","
        << current_weather_wind_dir_ << ","
        << current_weather_rain_
        << std::endl;
  }

  void writeSummaryJson()
  {
    std::error_code ec;
    std::filesystem::create_directories(output_root_, ec);
    if (ec) {
      RCLCPP_WARN(this->get_logger(), "Failed to create output directory %s: %s",
                  output_root_.c_str(), ec.message().c_str());
      return;
    }

    size_t accepted = 0, rejected = 0, dropped = 0, timeouts = 0, started = 0, preempted = 0, energy_depleted = 0;
    std::vector<double> decision_latencies_ms;
    std::vector<double> waiting_times_ms;
    std::vector<double> energy_recovered;
    for (const auto & [id, rec] : charge_records_) {
      switch (rec.outcome) {
        case ChargeOutcome::ACCEPTED: accepted++; break;
        case ChargeOutcome::REJECTED: rejected++; break;
        case ChargeOutcome::DROPPED: dropped++; break;
        case ChargeOutcome::TIMEOUT: timeouts++; break;
        case ChargeOutcome::STARTED: started++; break;
        case ChargeOutcome::PREEMPTED: preempted++; break;
        case ChargeOutcome::ENERGY_DEPLETED: energy_depleted++; break;
        default: break;
      }

      if (rec.decision_time.nanoseconds() != 0 && rec.request_time.nanoseconds() != 0) {
        decision_latencies_ms.push_back((rec.decision_time - rec.request_time).seconds() * 1000.0);
      }
      if (rec.dock_start_time.nanoseconds() != 0 && rec.request_time.nanoseconds() != 0) {
        waiting_times_ms.push_back((rec.dock_start_time - rec.request_time).seconds() * 1000.0);
      }
      if (rec.charge_completed && rec.end_battery >= 0.0 && rec.start_battery >= 0.0) {
        energy_recovered.push_back(rec.end_battery - rec.start_battery);
      }
    }

    double mean_decision_latency = -1.0;
    if (!decision_latencies_ms.empty()) {
      double sum = 0.0;
      for (double v : decision_latencies_ms) sum += v;
      mean_decision_latency = sum / static_cast<double>(decision_latencies_ms.size());
    }

    double mean_wait = -1.0;
    if (!waiting_times_ms.empty()) {
      double sum = 0.0;
      for (double v : waiting_times_ms) sum += v;
      mean_wait = sum / static_cast<double>(waiting_times_ms.size());
    }

    double mean_energy = -1.0;
    if (!energy_recovered.empty()) {
      double sum = 0.0;
      for (double v : energy_recovered) sum += v;
      mean_energy = sum / static_cast<double>(energy_recovered.size());
    }

    auto path = std::filesystem::path(output_root_) / "summary.json";
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open %s for writing", path.string().c_str());
      return;
    }

    size_t recovery_start = recovery_counts_["RECOVERY_START"];
    size_t recovery_done = recovery_counts_["RECOVERY_DONE"];
    size_t cluster_reassign = recovery_counts_["CLUSTER_REASSIGN"];
    size_t task_assign = recovery_counts_["TASK_ASSIGN"];
    size_t new_deployment = recovery_counts_["NEW_DEPLOYMENT"];

    out << "{\n"
        << "  \"run_id\": \"" << run_id_ << "\",\n"
        << "  \"charging\": {\n"
        << "    \"requests_total\": " << charge_records_.size() << ",\n"
        << "    \"accepted\": " << accepted << ",\n"
        << "    \"rejected\": " << rejected << ",\n"
        << "    \"dropped\": " << dropped << ",\n"
        << "    \"timeouts\": " << timeouts << ",\n"
        << "    \"started\": " << started << ",\n"
        << "    \"preempted\": " << preempted << ",\n"
        << "    \"energy_depleted\": " << energy_depleted << ",\n"
        << "    \"success_rate\": " << (charge_records_.empty() ? 0.0 : static_cast<double>(started) / static_cast<double>(charge_records_.size())) << ",\n"
        << "    \"decision_latency_ms\": {\n"
        << "      \"mean\": " << mean_decision_latency << ",\n"
        << "      \"p95\": " << percentile(decision_latencies_ms, 95.0) << "\n"
        << "    },\n"
        << "    \"waiting_time_ms\": {\n"
        << "      \"mean\": " << mean_wait << "\n"
        << "    },\n"
        << "    \"energy_recovered\": {\n"
        << "      \"mean\": " << mean_energy << "\n"
        << "    }\n"
        << "  },\n"
        << "  \"charging_fairness\": {\n";

    std::unordered_map<std::string, size_t> rejected_by_uav;
    std::unordered_map<std::string, size_t> timeout_by_uav;
    std::unordered_map<std::string, double> max_wait_ms_by_uav;
    for (const auto & [id, rec] : charge_records_) {
      if (!rec.uav_id.empty()) {
        if (rec.outcome == ChargeOutcome::REJECTED) {
          rejected_by_uav[rec.uav_id]++;
        } else if (rec.outcome == ChargeOutcome::TIMEOUT) {
          timeout_by_uav[rec.uav_id]++;
        }
        if (rec.request_time.nanoseconds() != 0 &&
            rec.dock_start_time.nanoseconds() != 0) {
          double wait_ms = (rec.dock_start_time - rec.request_time).seconds() * 1000.0;
          if (wait_ms < 0.0) {
            wait_ms = 0.0;
          }
          auto it_wait = max_wait_ms_by_uav.find(rec.uav_id);
          if (it_wait == max_wait_ms_by_uav.end() || wait_ms > it_wait->second) {
            max_wait_ms_by_uav[rec.uav_id] = wait_ms;
          }
        }
      }
    }

    out << "    \"rejections_by_uav\": {";
    bool first_reject = true;
    for (const auto & pair : rejected_by_uav) {
      if (!first_reject) out << ", ";
      first_reject = false;
      out << "\"" << pair.first << "\": " << pair.second;
    }
    out << "},\n";

    out << "    \"timeouts_by_uav\": {";
    bool first_timeout = true;
    for (const auto & pair : timeout_by_uav) {
      if (!first_timeout) out << ", ";
      first_timeout = false;
      out << "\"" << pair.first << "\": " << pair.second;
    }
    out << "},\n";

    out << "    \"max_waiting_time_ms_by_uav\": {";
    bool first_wait = true;
    for (const auto & pair : max_wait_ms_by_uav) {
      if (!first_wait) out << ", ";
      first_wait = false;
      out << "\"" << pair.first << "\": " << pair.second;
    }
    out << "}\n";

    out << "  },\n"
        << "  \"network\": {\n"
        << "    \"qos_targets\": {\n"
        << "      \"pdr\": " << qos_target_pdr_ << ",\n"
        << "      \"delay_ms\": " << qos_target_delay_ms_ << ",\n"
        << "      \"jitter_ms\": " << qos_target_jitter_ms_ << "\n"
        << "    },\n"
        << "    \"qos_weights\": {\n"
        << "      \"pdr\": " << qos_weight_pdr_ << ",\n"
        << "      \"delay\": " << qos_weight_delay_ << ",\n"
        << "      \"jitter\": " << qos_weight_jitter_ << "\n"
        << "    },\n"
        << "    \"by_category\": [\n";

    bool first_cat = true;
    auto qos_stats = buildQosStats();
    for (const auto & [key, stats] : qos_stats) {
      if (!first_cat) out << ",\n";
      first_cat = false;
      auto sep = key.find(':');
      std::string flow_str = key.substr(0, sep);
      std::string ctrl_str = (sep == std::string::npos) ? "" : key.substr(sep + 1);
      int flow_val = 0;
      try {
        flow_val = std::stoi(flow_str);
      } catch (...) {
        flow_val = 0;
      }
      QosMetrics metrics = finalizeQosStats(stats, flow_val, ctrl_str);
      out << "      {\"flow_type\": " << metrics.flow_type
          << ", \"control_type\": \"" << metrics.control_type << "\",\n"
          << "       \"generated\": " << metrics.generated << ",\n"
          << "       \"delivered\": " << metrics.delivered << ",\n"
          << "       \"dropped\": " << metrics.dropped << ",\n"
          << "       \"pdr\": " << metrics.pdr << ",\n"
          << "       \"delay_ms\": {\"mean\": " << metrics.delay_mean_ms
          << ", \"p95\": " << metrics.delay_p95_ms << "},\n"
          << "       \"jitter_ms\": " << metrics.jitter_ms << ",\n"
          << "       \"throughput_bps\": " << metrics.throughput_bps << ",\n"
          << "       \"generated_bps\": " << metrics.generated_bps << ",\n"
          << "       \"qos_score\": " << metrics.qos_score << ",\n"
          << "       \"drops\": {";
      bool first_reason = true;
      for (const auto & [reason, count] : stats.drop_reasons) {
        if (!first_reason) out << ", ";
        first_reason = false;
        out << "\"" << reason << "\": " << count;
      }
      out << "}\n"
          << "      }";
    }
    out << "\n    ]\n"
        << "  },\n"
        << "  \"recovery\": {\n"
        << "    \"start\": " << recovery_start << ",\n"
        << "    \"done\": " << recovery_done << ",\n"
        << "    \"cluster_reassign\": " << cluster_reassign << ",\n"
        << "    \"task_assign\": " << task_assign << ",\n"
        << "    \"new_deployment\": " << new_deployment << "\n"
        << "  }\n"
        << "}\n";
  }

  void trackRecoveryEvent(const uav_msgs::msg::TrafficMessage & msg)
  {
    if (msg.msg_id.empty()) {
      return;
    }
    if (recovery_events_.count(msg.msg_id) > 0) {
      return;
    }
    if (msg.control_type != "RECOVERY_START" &&
        msg.control_type != "RECOVERY_DONE" &&
        msg.control_type != "CLUSTER_REASSIGN" &&
        msg.control_type != "TASK_ASSIGN" &&
        msg.control_type != "NEW_DEPLOYMENT") {
      return;
    }

    RecoveryEvent rec;
    rec.msg_id = msg.msg_id;
    rec.control_type = msg.control_type;
    rec.src_id = msg.src_id;
    rec.dst_id = msg.dst_id;
    rec.creation_time = rclcpp::Time(msg.creation_time);

    if (msg.control_type == "RECOVERY_START" || msg.control_type == "RECOVERY_DONE") {
      try {
        rec.epoch = std::stoi(msg.payload);
      } catch (...) {
        rec.epoch = -1;
      }
    } else if (msg.control_type == "CLUSTER_REASSIGN") {
      rec.member_id = msg.dst_id;
      rec.ch_id = msg.payload;
    } else if (msg.control_type == "TASK_ASSIGN") {
      rec.member_id = msg.dst_id;
      rec.task_count = countTaskAssignPoints(msg.payload);
    } else if (msg.control_type == "NEW_DEPLOYMENT") {
      rec.ch_id = msg.dst_id;
      parseDeploymentPose(msg.payload, rec.x, rec.y, rec.z);
    }

    recovery_events_[msg.msg_id] = rec;
    recovery_counts_[msg.control_type]++;
  }

  int countTaskAssignPoints(const std::string & payload) const
  {
    if (payload.empty()) {
      return 0;
    }
    int count = 0;
    std::stringstream ss(payload);
    std::string token;
    while (std::getline(ss, token, ';')) {
      if (!token.empty()) {
        count++;
      }
    }
    return count;
  }

  void parseDeploymentPose(const std::string & payload, double & x, double & y, double & z) const
  {
    x = 0.0;
    y = 0.0;
    z = 0.0;
    std::stringstream ss(payload);
    std::string token;
    if (!std::getline(ss, token, ',')) {
      return;
    }
    try {
      x = std::stod(token);
    } catch (...) {
      return;
    }
    if (!std::getline(ss, token, ',')) {
      return;
    }
    try {
      y = std::stod(token);
    } catch (...) {
      return;
    }
    if (!std::getline(ss, token, ',')) {
      return;
    }
    try {
      z = std::stod(token);
    } catch (...) {
      return;
    }
  }


  // ---- Members ----
  // Traffic
  std::unordered_map<std::string, MsgRecord> records_;
  std::unordered_map<std::string, std::pair<std::string, std::string>> drop_by_ref_;
  std::unordered_map<std::string, rclcpp::Time> ack_by_ref_;
  size_t total_generated_;
  size_t total_delivered_;
  double avg_delay_sec_;
  size_t drop_total_ = 0;
  size_t ack_total_ = 0;
  size_t telemetry_delivered_ = 0;
  double telemetry_avg_delay_sec_ = 0.0;
  size_t telemetry_dropped_ = 0;
  std::unordered_map<std::string, size_t> telemetry_drop_reasons_;
  std::unordered_map<uint8_t, std::unordered_map<std::string, size_t>> delivered_by_flow_control_;
  std::unordered_map<std::string, size_t> control_type_counts_;
  std::unordered_map<std::string, RecoveryEvent> recovery_events_;
  std::unordered_set<std::string> exported_recovery_events_;
  std::unordered_map<std::string, size_t> recovery_counts_;
  std::unordered_map<std::string, size_t> drop_reason_counts_;
  std::unordered_set<std::string> seen_control_msg_ids_;
  std::unordered_set<std::string> dropped_ids_;
  std::deque<rclcpp::Time> generated_timestamps_;
  std::deque<rclcpp::Time> drop_timestamps_;
  std::deque<rclcpp::Time> delivered_timestamps_;
  rclcpp::Time last_msg_time_;
  rclcpp::Time last_drop_time_;
  rclcpp::Time last_delivered_time_;

  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_raw_sub_;
  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_sub_;
  rclcpp::Subscription<uav_msgs::msg::UavStatus>::SharedPtr status_sub_;
  rclcpp::Subscription<uav_msgs::msg::RoutingTable>::SharedPtr routing_table_sub_;
  rclcpp::Subscription<uav_msgs::msg::WeatherStatus>::SharedPtr weather_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr queue_event_sub_;

  // Preemption events
  std::vector<PreemptionEvent> preemption_events_;
  size_t exported_preemption_count_ = 0;
  bool preemption_events_file_initialized_ = false;

  // Weather
  std::string current_weather_regime_;
  float current_weather_temp_c_ = 0.0f;
  float current_weather_wind_speed_ = 0.0f;
  float current_weather_wind_dir_ = 0.0f;
  float current_weather_rain_ = 0.0f;
  bool weather_received_ = false;

  // Failures
  size_t battery_dead_count_ = 0;
  std::unordered_set<std::string> seen_failure_ids_;
  std::unordered_set<std::string> dead_uavs_;
  std::unordered_map<std::string, size_t> drop_reasons_;

  // Charging
  std::unordered_map<std::string, ChargeRecord> charge_records_;
  std::unordered_map<std::string, std::string> latest_request_by_uav_;
  std::unordered_map<std::string, uint8_t> latest_role_by_uav_;
  std::unordered_map<std::string, float> latest_request_battery_by_uav_;
  std::unordered_map<std::string, UavState> uav_states_;
  std::unordered_map<std::string, rclcpp::Time> request_times_;
  size_t total_charging_sessions_;
  double avg_charge_wait_sec_;
  double decision_timeout_sec_ = 30.0;

  rclcpp::Subscription<uav_msgs::msg::ChargeRequest>::SharedPtr  charge_request_sub_;
  rclcpp::Subscription<uav_msgs::msg::ChargeDecision>::SharedPtr charge_decision_sub_;

  // Output
  std::string run_id_;
  std::string output_dir_;
  std::string output_root_;
  bool messages_file_initialized_ = false;
  bool qos_metrics_file_initialized_ = false;
  bool charge_events_file_initialized_ = false;
  bool recovery_events_file_initialized_ = false;
  bool status_timeseries_file_initialized_ = false;
  bool charge_queue_timeseries_file_initialized_ = false;
  bool weather_timeseries_file_initialized_ = false;
  rclcpp::TimerBase::SharedPtr csv_timer_;
  rclcpp::TimerBase::SharedPtr charge_timeout_timer_;
  rclcpp::TimerBase::SharedPtr status_timeseries_timer_;
  rclcpp::TimerBase::SharedPtr weather_timeseries_timer_;
  rclcpp::TimerBase::SharedPtr queue_timeseries_timer_;
  rclcpp::TimerBase::SharedPtr stats_timer_;
  rclcpp::TimerBase::SharedPtr shutdown_check_timer_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr stats_pub_;
  double status_sample_period_sec_ = 1.0;
  double queue_stats_period_sec_ = 1.0;
  int ugv_dock_capacity_ = 1;
  rclcpp::Time routing_table_empty_since_{0, 0, RCL_ROS_TIME};
  bool routing_table_seen_ = false;
  std::unordered_set<std::string> exported_messages_;
  std::unordered_set<std::string> exported_charge_requests_;
  std::unordered_set<std::string> exported_qos_keys_;

  rclcpp::Time start_time_;
  double max_runtime_sec_ = 0.0;
  bool stop_on_backbone_loss_ = false;
  double routing_table_empty_shutdown_sec_ = 10.0;
  std::vector<std::string> backbone_ids_;
  double rate_window_sec_ = 10.0;
  double qos_target_pdr_ = 0.95;
  double qos_target_delay_ms_ = 200.0;
  double qos_target_jitter_ms_ = 50.0;
  double qos_weight_pdr_ = 0.5;
  double qos_weight_delay_ = 0.3;
  double qos_weight_jitter_ = 0.2;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<NetworkMonitorNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
