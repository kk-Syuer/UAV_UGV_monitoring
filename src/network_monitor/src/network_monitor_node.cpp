#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"

using std::placeholders::_1;

class NetworkMonitorNode : public rclcpp::Node
{
public:
  NetworkMonitorNode()
  : Node("network_monitor_node"),
    total_generated_(0),
    total_delivered_(0),
    avg_delay_sec_(0.0)
  {
    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 100,
      std::bind(&NetworkMonitorNode::trafficCallback, this, _1));

    delivered_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/network/traffic_delivered", 100,
      std::bind(&NetworkMonitorNode::deliveredCallback, this, _1));

    RCLCPP_INFO(this->get_logger(), "Network monitor started.");
  }

private:
  // Called when a message is first injected into the network
  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    auto it = creation_times_.find(msg->msg_id);
    if (it == creation_times_.end()) {
      // Store creation time (from the message)
      rclcpp::Time t_created(msg->creation_time);
      creation_times_[msg->msg_id] = t_created;
      total_generated_++;

      RCLCPP_DEBUG(this->get_logger(),
                   "[GEN] msg_id=%s src=%s dst=%s",
                   msg->msg_id.c_str(), msg->src_id.c_str(), msg->dst_id.c_str());
    }
  }

  // Called when a message is considered delivered (at sink or user)
  void deliveredCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    auto now = this->now();
    rclcpp::Time t_start;

    auto it = creation_times_.find(msg->msg_id);
    if (it != creation_times_.end()) {
      t_start = it->second;
      // Optionally erase to avoid unbounded growth
      creation_times_.erase(it);
    } else {
      // Fallback: use the creation_time carried in the message
      t_start = rclcpp::Time(msg->creation_time);
    }

    double delay_sec = (now - t_start).seconds();

    total_delivered_++;
    // Online average update
    avg_delay_sec_ += (delay_sec - avg_delay_sec_) / static_cast<double>(total_delivered_);

    RCLCPP_INFO(this->get_logger(),
                "[DEL] msg_id=%s delay=%.4f s | delivered=%zu / generated=%zu | avg_delay=%.4f s",
                msg->msg_id.c_str(),
                delay_sec,
                total_delivered_,
                total_generated_,
                avg_delay_sec_);
  }

  // Map msg_id -> creation_time
  std::unordered_map<std::string, rclcpp::Time> creation_times_;

  size_t total_generated_;
  size_t total_delivered_;
  double avg_delay_sec_;

  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_sub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<NetworkMonitorNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
