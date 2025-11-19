#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/cluster_info.hpp"

using namespace std::chrono_literals;

class UserDeviceNode : public rclcpp::Node
{
public:
  UserDeviceNode()
  : Node("user_device_node"),
    msg_counter_(0)
  {
    user_id_ = this->declare_parameter<std::string>("user_id", "user_1");
    cluster_id_ = this->declare_parameter<std::string>("cluster_id", "cluster_1");
    sink_id_ = this->declare_parameter<std::string>("sink_id", "sink_gateway");

    ch_id_.clear();

    traffic_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 10);

    cluster_sub_ = this->create_subscription<uav_msgs::msg::ClusterInfo>(
      "/ch_manager/cluster_info", 10,
      std::bind(&UserDeviceNode::clusterInfoCallback, this, std::placeholders::_1));

    traffic_timer_ = this->create_wall_timer(
      3s, std::bind(&UserDeviceNode::publishTraffic, this));

    RCLCPP_INFO(this->get_logger(),
                "User device %s started, cluster=%s",
                user_id_.c_str(), cluster_id_.c_str());
  }

private:
  void clusterInfoCallback(const uav_msgs::msg::ClusterInfo::SharedPtr msg)
  {
    if (msg->cluster_id == cluster_id_) {
      ch_id_ = msg->ch_id;
      RCLCPP_INFO(this->get_logger(),
                  "User %s attached to CH %s in cluster %s",
                  user_id_.c_str(), ch_id_.c_str(), cluster_id_.c_str());
    }
  }

  void publishTraffic()
  {
    if (ch_id_.empty()) {
      RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
                           "User %s has no CH yet, not sending traffic.",
                           user_id_.c_str());
      return;
    }

    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = user_id_ + "_" + std::to_string(msg_counter_++);
    msg.src_id = user_id_;
    msg.dst_id = ch_id_;  // send to CH
    msg.msg_type = 0;     // TEXT
    msg.priority = 1;     // maybe a bit higher
    msg.size_bytes = 200;
    msg.creation_time = this->now();
    msg.hop_count = 0;

    RCLCPP_INFO(this->get_logger(),
                "[USER TX] msg_id=%s src=%s dst=%s",
                msg.msg_id.c_str(), msg.src_id.c_str(), msg.dst_id.c_str());

    traffic_pub_->publish(msg);
  }

  std::string user_id_;
  std::string cluster_id_;
  std::string sink_id_;
  std::string ch_id_;

  uint64_t msg_counter_;

  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_pub_;
  rclcpp::Subscription<uav_msgs::msg::ClusterInfo>::SharedPtr cluster_sub_;
  rclcpp::TimerBase::SharedPtr traffic_timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UserDeviceNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
