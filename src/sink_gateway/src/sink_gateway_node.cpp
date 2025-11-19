#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"

class SinkGatewayNode : public rclcpp::Node
{
public:
  SinkGatewayNode()
  : Node("sink_gateway_node")
  {
    sink_id_ = this->declare_parameter<std::string>("sink_id", "sink_gateway");

    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 10,
      std::bind(&SinkGatewayNode::trafficCallback, this, std::placeholders::_1));

    delivered_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/network/traffic_delivered", 10);

    RCLCPP_INFO(this->get_logger(), "Sink gateway started with id='%s'", sink_id_.c_str());
  }

private:
  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    // Only handle messages destined to this sink
    if (msg->dst_id != sink_id_) {
      return;
    }

    RCLCPP_INFO(this->get_logger(),
                "[SINK] Received msg_id=%s from=%s, size=%u bytes",
                msg->msg_id.c_str(), msg->src_id.c_str(), msg->size_bytes);

    // Here we conceptually send it outside the system.
    // For metrics, re-publish the same message on /network/traffic_delivered.
    delivered_pub_->publish(*msg);
  }

  std::string sink_id_;
  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_pub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<SinkGatewayNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
