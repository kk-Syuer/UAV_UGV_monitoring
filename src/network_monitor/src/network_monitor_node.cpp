#include <chrono>
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

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
      "/network/traffic", 100,
      std::bind(&NetworkMonitorNode::trafficCallback, this, _1));

    delivered_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/network/traffic_delivered", 100,
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
    auto it = creation_times_.find(msg->msg_id);
    if (it == creation_times_.end()) {
      rclcpp::Time t_created(msg->creation_time);
      creation_times_[msg->msg_id] = t_created;
      total_generated_++;

      RCLCPP_INFO(this->get_logger(),
                  "[GEN] msg_id=%s src=%s dst=%s | total_generated=%zu",
                  msg->msg_id.c_str(), msg->src_id.c_str(), msg->dst_id.c_str(),
                  total_generated_);
    }
    // If we've already seen this msg_id, we don't log again; it's just forwarding/duplicates

    if (msg->flow_type == 1 && msg->control_type == "FAILURE_EVENT") {
      handleFailureFromTraffic(*msg);
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

    RCLCPP_INFO(this->get_logger(),
                "[DEL] msg_id=%s delay=%.4f s | delivered=%zu / generated=%zu | avg_delay=%.4f s",
                msg->msg_id.c_str(),
                delay_sec,
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
  size_t total_generated_;
  size_t total_delivered_;
  double avg_delay_sec_;

  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_sub_;

  // Failures
  size_t battery_dead_count_ = 0;
  std::unordered_set<std::string> seen_failure_ids_;

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
