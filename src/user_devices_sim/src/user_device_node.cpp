#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"

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
    ch_id_ = this->declare_parameter<std::string>("ch_id", "uav_1");

    RCLCPP_INFO(this->get_logger(),
                "User device %s started, cluster=%s, attached to CH %s",
                user_id_.c_str(), cluster_id_.c_str(), ch_id_.c_str());

    traffic_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 10);

    delivered_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/network/traffic_delivered", 10);

    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 10,
      std::bind(&UserDeviceNode::trafficCallback, this, std::placeholders::_1));

    send_timer_ = this->create_wall_timer(
      3s, std::bind(&UserDeviceNode::publishTraffic, this));
  }

private:
  void publishTraffic()
  {
    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = user_id_ + "_" + std::to_string(msg_counter_++);
    msg.src_id = user_id_;

    // Final destination is usually sink_gateway in this scenario
    std::string final_dst = "sink_gateway";
    msg.dst_id = final_dst;

    // First hop is my attached CH
    msg.next_hop_id = ch_id_;

    msg.msg_type = 0;       // TEXT
    msg.priority = 0;
    msg.size_bytes = 200;
    msg.creation_time = this->now();
    msg.hop_count = 0;

    RCLCPP_INFO(this->get_logger(),
                "[USER TX] msg_id=%s src=%s dst=%s next_hop=%s",
                msg.msg_id.c_str(), msg.src_id.c_str(),
                msg.dst_id.c_str(), msg.next_hop_id.c_str());

    traffic_pub_->publish(msg);
  }

  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    if (msg->dst_id != user_id_) {
      return;
    }

    RCLCPP_INFO(this->get_logger(),
                "[USER RX] msg_id=%s delivered to %s (from %s)",
                msg->msg_id.c_str(), user_id_.c_str(), msg->src_id.c_str());

    // Mark as delivered for metrics
    delivered_pub_->publish(*msg);
  }

  // Members
  std::string user_id_;
  std::string cluster_id_;
  std::string ch_id_;
  uint64_t msg_counter_;

  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_pub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_pub_;
  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::TimerBase::SharedPtr send_timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UserDeviceNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
