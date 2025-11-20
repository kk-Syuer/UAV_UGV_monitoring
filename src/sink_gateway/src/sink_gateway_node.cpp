#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"

using std::placeholders::_1;

class SinkGatewayNode : public rclcpp::Node
{
public:
  SinkGatewayNode()
  : Node("sink_gateway_node")
  {
    std::string my_id =
      this->declare_parameter<std::string>("sink_id", "sink_gateway");
    sink_id_ = my_id;

    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 100,
      std::bind(&SinkGatewayNode::trafficCallback, this, _1));

    delivered_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/network/traffic_delivered", 100);

    RCLCPP_INFO(this->get_logger(),
                "Sink gateway started with id='%s'.", sink_id_.c_str());
  }

private:
  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    // I'm the gateway; I only care about messages whose final destination is me
    if (msg->dst_id != sink_id_) {
      return;
    }

    // Optionally we could also check msg->next_hop_id == sink_id_ here,
    // but it's not strictly necessary since only messages destined to me
    // should set dst_id = sink_id_.
    RCLCPP_INFO(this->get_logger(),
                "[SINK RX] msg_id=%s src=%s dst=%s hop=%u (delivered to gateway)",
                msg->msg_id.c_str(), msg->src_id.c_str(),
                msg->dst_id.c_str(), msg->hop_count);

    delivered_pub_->publish(*msg);
  }


  std::string sink_id_;

  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr    delivered_pub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<SinkGatewayNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
