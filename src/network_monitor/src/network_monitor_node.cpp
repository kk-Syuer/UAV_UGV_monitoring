#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/charge_request.hpp"
#include "uav_msgs/msg/charge_decision.hpp"
#include "uav_msgs/msg/failure_event.hpp"

using std::placeholders::_1;

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
    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 100,
      std::bind(&NetworkMonitorNode::trafficCallback, this, _1));

    delivered_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/network/traffic_delivered", 100,
      std::bind(&NetworkMonitorNode::deliveredCallback, this, _1));

    charge_request_sub_ = this->create_subscription<uav_msgs::msg::ChargeRequest>(
      "/uav_fleet/charge_requests", 100,
      std::bind(&NetworkMonitorNode::chargeRequestCallback, this, _1));

    charge_decision_sub_ = this->create_subscription<uav_msgs::msg::ChargeDecision>(
      "/ugv/charge_decisions", 100,
      std::bind(&NetworkMonitorNode::chargeDecisionCallback, this, _1));

    failure_sub_ = this->create_subscription<uav_msgs::msg::FailureEvent>(
      "/uav_fleet/failure_events", 100,
      std::bind(&NetworkMonitorNode::failureCallback, this, _1));

    RCLCPP_INFO(this->get_logger(), "Network monitor started.");
  }

private:
  // ---- Traffic monitoring ----
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
  }

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

  void chargeRequestCallback(const uav_msgs::msg::ChargeRequest::SharedPtr msg)
  {
    rclcpp::Time t(msg->stamp);
    request_times_[msg->uav_id] = t;

    RCLCPP_INFO(this->get_logger(),
                "[CHG-REQ] uav=%s role=%u batt=%.1f%% at t=%.1f",
                msg->uav_id.c_str(), msg->role, msg->battery_level,
                t.seconds());
  }

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
  
  void failureCallback(const uav_msgs::msg::FailureEvent::SharedPtr msg)
  {
    // Convert builtin_interfaces/Time -> rclcpp::Time so we can use .seconds()
    rclcpp::Time t(msg->stamp);

    if (msg->failure_type == 1)  // BATTERY_DEAD
    {
      battery_dead_count_++;
      RCLCPP_WARN(this->get_logger(),
                  "[FAIL] BATTERY_DEAD from %s at t=%.3f (total=%zu)",
                  msg->uav_id.c_str(),
                  t.seconds(),
                  battery_dead_count_);
    }
    else
    {
      RCLCPP_WARN(this->get_logger(),
                  "[FAIL] failure from %s type=%u desc=%s",
                  msg->uav_id.c_str(),
                  msg->failure_type,
                  msg->description.c_str());
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
  rclcpp::Subscription<uav_msgs::msg::FailureEvent>::SharedPtr failure_sub_;

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
