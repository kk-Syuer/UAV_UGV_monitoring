#include <chrono>
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/charge_request.hpp"
#include "uav_msgs/msg/charge_decision.hpp"

using std::placeholders::_1;

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

    RCLCPP_INFO(this->get_logger(), "Network monitor started.");
  }

private:
  // ---- Traffic monitoring ----
  // Track first-seen messages to compute end-to-end delay.
  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    rclcpp::Time now = this->now();
    auto it = traffic_metrics_.find(msg->msg_id);
    if (it == traffic_metrics_.end()) {
      TrafficInfo info;
      info.created = rclcpp::Time(msg->creation_time);
      info.last_seen = now;
      info.last_hop_count = msg->hop_count;
      traffic_metrics_[msg->msg_id] = info;
      creation_times_[msg->msg_id] = info.created;
      total_generated_++;

      RCLCPP_INFO(this->get_logger(),
                  "[GEN] msg_id=%s src=%s dst=%s | total_generated=%zu",
                  msg->msg_id.c_str(), msg->src_id.c_str(), msg->dst_id.c_str(),
                  total_generated_);
    } else {
      auto & info = it->second;
      if (msg->hop_count > info.last_hop_count) {
        double hop_delay = (now - info.last_seen).seconds();
        info.hop_delays.push_back(hop_delay);
        info.last_hop_count = msg->hop_count;
      }
      info.last_seen = now;
    }

    if (msg->flow_type == 1 && msg->control_type == "FAILURE_EVENT") {
      handleFailureFromTraffic(*msg);
    }

    if (msg->flow_type == 1 && msg->control_type == "DROP") {
      std::string reason = !msg->drop_reason.empty() ? msg->drop_reason
                          : (!msg->payload.empty() ? msg->payload : "UNKNOWN");
      drop_reasons_[reason]++;
      RCLCPP_WARN(this->get_logger(),
                  "[DROP] msg_id=%s reason=%s total_reason=%zu",
                  msg->msg_id.c_str(), reason.c_str(), drop_reasons_[reason]);
    }
  }

  // Compute delivery delay when messages arrive at final destination.
  void deliveredCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    auto now = this->now();
    rclcpp::Time t_start;

    auto it = creation_times_.find(msg->msg_id);
    if (it != creation_times_.end()) {
      t_start = it->second;
      creation_times_.erase(it);
    } else {
      t_start = rclcpp::Time(msg->creation_time);
    }

    double delay_sec = (now - t_start).seconds();

    total_delivered_++;
    avg_delay_sec_ += (delay_sec - avg_delay_sec_) / static_cast<double>(total_delivered_);

    double avg_hop_delay = 0.0;
    auto it_info = traffic_metrics_.find(msg->msg_id);
    if (it_info != traffic_metrics_.end()) {
      if (!it_info->second.hop_delays.empty()) {
        double sum = 0.0;
        for (double d : it_info->second.hop_delays) {
          sum += d;
        }
        avg_hop_delay = sum / static_cast<double>(it_info->second.hop_delays.size());
      }
      traffic_metrics_.erase(it_info);
    }

    bool ttl_respected = true;
    if (msg->ttl != 0 && msg->hop_count > msg->ttl) {
      ttl_respected = false;
    }

    RCLCPP_INFO(this->get_logger(),
                "[DEL] msg_id=%s delay=%.4f s avg_hop_delay=%.4f s ttl_ok=%s | delivered=%zu / generated=%zu | avg_delay=%.4f s",
                msg->msg_id.c_str(),
                delay_sec,
                avg_hop_delay,
                ttl_respected ? "yes" : "no",
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


  // ---- Members ----
  // Traffic
  std::unordered_map<std::string, rclcpp::Time> creation_times_;
  struct TrafficInfo
  {
    rclcpp::Time created;
    rclcpp::Time last_seen;
    uint32_t last_hop_count = 0;
    std::vector<double> hop_delays;
  };
  std::unordered_map<std::string, TrafficInfo> traffic_metrics_;
  size_t total_generated_;
  size_t total_delivered_;
  double avg_delay_sec_;

  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_sub_;

  // Failures
  size_t battery_dead_count_ = 0;
  std::unordered_set<std::string> seen_failure_ids_;
  std::unordered_map<std::string, size_t> drop_reasons_;

  // Charging
  std::unordered_map<std::string, rclcpp::Time> request_times_;
  size_t total_charging_sessions_;
  double avg_charge_wait_sec_;

  rclcpp::Subscription<uav_msgs::msg::ChargeRequest>::SharedPtr  charge_request_sub_;
  rclcpp::Subscription<uav_msgs::msg::ChargeDecision>::SharedPtr charge_decision_sub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<NetworkMonitorNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
