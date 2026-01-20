#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/charge_request.hpp"
#include "uav_msgs/msg/charge_decision.hpp"
#include "uav_msgs/msg/uav_status.hpp"

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

  double start_battery = -1.0;
  double end_battery = -1.0;
  bool charge_completed = false;
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
    max_runtime_sec_ = this->declare_parameter<double>("max_runtime_sec", 0.0);
    stop_on_backbone_loss_ = this->declare_parameter<bool>("stop_on_backbone_loss", false);
    backbone_ids_ = this->declare_parameter<std::vector<std::string>>(
      "backbone_ids", std::vector<std::string>{});
    output_root_ = (std::filesystem::path(output_dir_) / run_id_).string();
    start_time_ = this->now();

    // Listen to traffic generation and delivery for latency metrics.
    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/fanet/network_bus", 100,
      std::bind(&NetworkMonitorNode::trafficCallback, this, _1));

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

    csv_timer_ = this->create_wall_timer(
      std::chrono::duration<double>(csv_write_period_sec),
      [this]() { this->writeOutputs(false); });

    charge_timeout_timer_ = this->create_wall_timer(
      std::chrono::seconds(1),
      std::bind(&NetworkMonitorNode::checkChargeTimeouts, this));

    status_timeseries_timer_ = this->create_wall_timer(
      std::chrono::duration<double>(status_sample_period_sec_),
      std::bind(&NetworkMonitorNode::writeStatusTimeseriesRow, this));

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
    if (msg->control_type == "DROP") {
      if (!msg->ref_msg_id.empty()) {
        drop_by_ref_[msg->ref_msg_id] = {msg->drop_reason, msg->src_id};

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
        ack_by_ref_[msg->ref_msg_id] = this->now();
      }
      return;
    }

    rclcpp::Time now = this->now();
    if (msg->control_type == "CHARGE_REQUEST") {
      auto & rec = charge_records_[msg->msg_id];
      rec.request_msg_id = msg->msg_id;
      rec.uav_id = msg->src_id;
      rec.ugv_id = msg->dst_id;
      rec.request_time = now;
      if (!isTerminalOutcome(rec.outcome) || rec.outcome == ChargeOutcome::PENDING) {
        rec.outcome = ChargeOutcome::PENDING;
      }
      latest_request_by_uav_[rec.uav_id] = msg->msg_id;
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
      rec.first_seen_bus_time = now;
      total_generated_++;

      RCLCPP_INFO(this->get_logger(),
                  "[GEN] msg_id=%s src=%s dst=%s | total_generated=%zu",
                  msg->msg_id.c_str(), msg->src_id.c_str(), msg->dst_id.c_str(),
                  total_generated_);
    }
    rec.forward_count++;

    if (msg->flow_type == 1 && msg->control_type == "FAILURE_EVENT") {
      handleFailureFromTraffic(*msg);
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
      bool accepted = msg->payload.find("REJECT") == std::string::npos;
      if (!isTerminalOutcome(charge_rec.outcome)) {
        charge_rec.outcome = accepted ? ChargeOutcome::ACCEPTED : ChargeOutcome::REJECTED;
      }
      if (!accepted) {
        charge_rec.failure_reason = "REJECTED";
      }
      latest_request_by_uav_[charge_rec.uav_id] = msg->ref_msg_id;
    }

    auto & rec = records_[msg->msg_id];
    if (rec.msg_id.empty()) {
      rec.msg_id = msg->msg_id;
      rec.flow_type = msg->flow_type;
      rec.control_type = msg->control_type;
      rec.src_id = msg->src_id;
      rec.dst_id = msg->dst_id;
      rec.creation_time = rclcpp::Time(msg->creation_time);
    }

    rec.delivered = true;
    rec.delivered_time = delivered_wall_time;

    rec.hop_count = msg->hop_count;
    rec.ttl_hops = msg->ttl;

    double delay_sec = (rec.delivered_time - rec.creation_time).seconds();
    total_delivered_++;
    avg_delay_sec_ += (delay_sec - avg_delay_sec_) / static_cast<double>(total_delivered_);

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

  void writeOutputs(bool final_flush)
  {
    reconcileCausality();
    writeMessagesCsv(final_flush);
    writeChargeEventsCsv(final_flush);
    writeStatusTimeseriesRow();
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
    bool need_header = !std::filesystem::exists(path);
    std::ofstream out(path, std::ios::app);
    if (!out.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open %s for writing", path.string().c_str());
      return;
    }

    if (need_header) {
      out << "run_id,msg_id,flow_type,control_type,src_id,dst_id,"
          << "delivered,e2e_delay_ms,forward_count,hop_count,ttl_hops,"
          << "dropped,drop_reason,dropper_id,ack_time" << std::endl;
    }

    for (const auto & [msg_id, rec] : records_) {
      if (!final_flush && exported_messages_.count(msg_id)) {
        continue;
      }
      double delay_ms = rec.delivered
        ? (rec.delivered_time - rec.creation_time).seconds() * 1000.0
        : -1.0;
      double ack_time = rec.ack_time.nanoseconds() == 0 ? -1.0 : rec.ack_time.seconds();
      out << run_id_ << ','
          << msg_id << ','
          << static_cast<int>(rec.flow_type) << ','
          << rec.control_type << ','
          << rec.src_id << ','
          << rec.dst_id << ','
          << (rec.delivered ? "true" : "false") << ','
          << delay_ms << ','
          << rec.forward_count << ','
          << rec.hop_count << ','
          << rec.ttl_hops << ','
          << (rec.dropped ? "true" : "false") << ','
          << rec.drop_reason << ','
          << rec.dropper_id << ','
          << ack_time
          << std::endl;
      exported_messages_.insert(msg_id);
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
    bool need_header = !std::filesystem::exists(path);
    std::ofstream out(path, std::ios::app);
    if (!out.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open %s for writing", path.string().c_str());
      return;
    }

    if (need_header) {
      out << "run_id,request_msg_id,uav_id,ugv_id,outcome,failure_reason,"
          << "request_time,decision_time,dock_start_time,decision_latency_ms,waiting_time_ms,"
          << "charge_completed,start_battery,end_battery,energy_recovered" << std::endl;
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
      double energy_recovered = (rec.charge_completed && rec.end_battery >= 0.0 && rec.start_battery >= 0.0)
        ? (rec.end_battery - rec.start_battery)
        : -1.0;

      out << run_id_ << ','
          << rec.request_msg_id << ','
          << rec.uav_id << ','
          << rec.ugv_id << ','
          << chargeOutcomeToString(rec.outcome) << ','
          << rec.failure_reason << ','
          << rec.request_time.seconds() << ','
          << rec.decision_time.seconds() << ','
          << rec.dock_start_time.seconds() << ','
          << decision_latency_ms << ','
          << waiting_time_ms << ','
          << (rec.charge_completed ? "true" : "false") << ','
          << rec.start_battery << ','
          << rec.end_battery << ','
          << energy_recovered
          << std::endl;
      exported_charge_requests_.insert(id);
    }
  }

  double percentile(std::vector<double> values, double pct)
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
    bool need_header = !std::filesystem::exists(path);
    std::ofstream out(path, std::ios::app);
    if (!out.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open %s for writing", path.string().c_str());
      return;
    }

    if (need_header) {
      out << "run_id,time,uav_id,role,charging_state,battery_level,backbone_active,x,y,z" << std::endl;
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
          << st.z
          << std::endl;
    }
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
    struct NetStats {
      size_t generated = 0;
      size_t delivered = 0;
      double forward_sum = 0.0;
      std::vector<double> delays_ms;
      std::unordered_map<std::string, size_t> drop_reasons;
    };
    std::unordered_map<std::string, NetStats> net_stats;

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

    for (const auto & [msg_id, rec] : records_) {
      std::string key = std::to_string(rec.flow_type) + ":" + rec.control_type;
      auto & stats = net_stats[key];
      stats.generated++;
      if (rec.delivered) {
        stats.delivered++;
        stats.forward_sum += static_cast<double>(rec.forward_count);
        stats.delays_ms.push_back((rec.delivered_time - rec.creation_time).seconds() * 1000.0);
      }
      if (rec.dropped && !rec.drop_reason.empty()) {
        stats.drop_reasons[rec.drop_reason]++;
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
        << "  \"network\": {\n"
        << "    \"by_category\": [\n";

    bool first_cat = true;
    for (const auto & [key, stats] : net_stats) {
      if (!first_cat) out << ",\n";
      first_cat = false;
      double pdr = stats.generated == 0 ? 0.0 : static_cast<double>(stats.delivered) / static_cast<double>(stats.generated);
      double delay_mean = -1.0;
      if (!stats.delays_ms.empty()) {
        double sum = 0.0;
        for (double v : stats.delays_ms) sum += v;
        delay_mean = sum / static_cast<double>(stats.delays_ms.size());
      }
      double forward_mean = stats.delivered == 0 ? -1.0 : stats.forward_sum / static_cast<double>(stats.delivered);
      auto sep = key.find(':');
      std::string flow_str = key.substr(0, sep);
      std::string ctrl_str = (sep == std::string::npos) ? "" : key.substr(sep + 1);
      int flow_val = 0;
      try {
        flow_val = std::stoi(flow_str);
      } catch (...) {
        flow_val = 0;
      }
      out << "      {\"flow_type\": " << flow_val
          << ", \"control_type\": \"" << ctrl_str << "\",\n"
          << "       \"generated\": " << stats.generated << ",\n"
          << "       \"delivered\": " << stats.delivered << ",\n"
          << "       \"pdr\": " << pdr << ",\n"
          << "       \"delay_ms\": {\"mean\": " << delay_mean << ", \"p95\": " << percentile(stats.delays_ms, 95.0) << "},\n"
          << "       \"drops\": {";
      bool first_reason = true;
      for (const auto & [reason, count] : stats.drop_reasons) {
        if (!first_reason) out << ", ";
        first_reason = false;
        out << "\"" << reason << "\": " << count;
      }
      out << "},\n"
          << "       \"overhead\": {\"avg_forward_count_delivered\": " << forward_mean << "}\n"
          << "      }";
    }
    out << "\n    ]\n"
        << "  }\n"
        << "}\n";
  }


  // ---- Members ----
  // Traffic
  std::unordered_map<std::string, MsgRecord> records_;
  std::unordered_map<std::string, std::pair<std::string, std::string>> drop_by_ref_;
  std::unordered_map<std::string, rclcpp::Time> ack_by_ref_;
  size_t total_generated_;
  size_t total_delivered_;
  double avg_delay_sec_;
  size_t telemetry_delivered_ = 0;
  double telemetry_avg_delay_sec_ = 0.0;
  size_t telemetry_dropped_ = 0;
  std::unordered_map<std::string, size_t> telemetry_drop_reasons_;
  std::unordered_map<uint8_t, std::unordered_map<std::string, size_t>> delivered_by_flow_control_;

  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_sub_;
  rclcpp::Subscription<uav_msgs::msg::UavStatus>::SharedPtr status_sub_;

  // Failures
  size_t battery_dead_count_ = 0;
  std::unordered_set<std::string> seen_failure_ids_;
  std::unordered_set<std::string> dead_uavs_;
  std::unordered_map<std::string, size_t> drop_reasons_;

  // Charging
  std::unordered_map<std::string, ChargeRecord> charge_records_;
  std::unordered_map<std::string, std::string> latest_request_by_uav_;
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
  rclcpp::TimerBase::SharedPtr csv_timer_;
  rclcpp::TimerBase::SharedPtr charge_timeout_timer_;
  rclcpp::TimerBase::SharedPtr status_timeseries_timer_;
  rclcpp::TimerBase::SharedPtr shutdown_check_timer_;
  double status_sample_period_sec_ = 1.0;
  std::unordered_set<std::string> exported_messages_;
  std::unordered_set<std::string> exported_charge_requests_;

  rclcpp::Time start_time_;
  double max_runtime_sec_ = 0.0;
  bool stop_on_backbone_loss_ = false;
  std::vector<std::string> backbone_ids_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<NetworkMonitorNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
