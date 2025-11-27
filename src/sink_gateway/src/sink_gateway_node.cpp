#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/uav_deployment.hpp"

using std::placeholders::_1;

class SinkGatewayNode : public rclcpp::Node
{
public:
  SinkGatewayNode()
  : Node("sink_gateway_node"),
    msg_counter_(0)
  {
    sink_id_ =
      this->declare_parameter<std::string>("sink_id", "sink_gateway");
    deployment_sub_ = this->create_subscription<uav_msgs::msg::UavDeployment>(
      "/coverage_planner/deployment", 10,
      std::bind(&SinkGatewayNode::deploymentCallback, this, std::placeholders::_1));
    uplink_ch_id_ =
      this->declare_parameter<std::string>("uplink_ch_id", "uav_3");

    target_uav_id_ =
      this->declare_parameter<std::string>("target_uav_id", "");

    double period =
      this->declare_parameter<double>("control_period_sec", 0.0);

    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 100,
      std::bind(&SinkGatewayNode::trafficCallback, this, _1));

    delivered_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/network/traffic_delivered", 100);

    control_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 100);

    RCLCPP_INFO(this->get_logger(),
                "Sink gateway started with id='%s', uplink_ch_id='%s', target_uav_id='%s', period=%.1fs",
                sink_id_.c_str(), uplink_ch_id_.c_str(),
                target_uav_id_.c_str(), period);

    // If period > 0 and target_uav_id_ non-empty, start sending control messages
    if (!target_uav_id_.empty() && period > 0.0) {
      using namespace std::chrono_literals;
      auto dur = std::chrono::duration<double>(period);
      control_timer_ = this->create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(dur),
        std::bind(&SinkGatewayNode::publishControlToUav, this));
      RCLCPP_INFO(this->get_logger(),
                  "Control timer enabled: every %.1f s send to '%s' via '%s'",
                  period, target_uav_id_.c_str(), uplink_ch_id_.c_str());
    } else {
      RCLCPP_INFO(this->get_logger(),
                  "Control timer disabled (set target_uav_id and control_period_sec to enable).");
    }
  }

private:
  // Receive path: messages whose *final destination* is the sink
  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    if (msg->dst_id != sink_id_) {
      return;
    }

    RCLCPP_INFO(this->get_logger(),
                "[SINK RX] msg_id=%s src=%s dst=%s hop=%u (delivered to gateway)",
                msg->msg_id.c_str(), msg->src_id.c_str(),
                msg->dst_id.c_str(), msg->hop_count);

    delivered_pub_->publish(*msg);
  }

  // Transmit path: send control messages to a target UAV via backbone
  void publishControlToUav()
  {
    if (target_uav_id_.empty()) {
      return;
    }

    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = sink_id_ + "_ctrl_" + target_uav_id_ + "_" + std::to_string(msg_counter_++);
    msg.src_id = sink_id_;
    msg.dst_id = target_uav_id_;
    msg.next_hop_id = uplink_ch_id_;

    // Let’s treat this as CONTROL_ALERT (just a convention; 3 is arbitrary)
    msg.msg_type = 3;
    msg.priority = 1;
    msg.size_bytes = 50;
    msg.creation_time = this->now();
    msg.hop_count = 0;

    RCLCPP_INFO(this->get_logger(),
                "[SINK TX] CONTROL msg_id=%s src=%s dst=%s next_hop=%s",
                msg.msg_id.c_str(), msg.src_id.c_str(),
                msg.dst_id.c_str(), msg.next_hop_id.c_str());

    control_pub_->publish(msg);
  }

  void deploymentCallback(const uav_msgs::msg::UavDeployment::SharedPtr msg)
  {
    if (msg->uav_id != sink_id_) {
      return;
    }
    sink_pose_ = msg->target_pose;
    RCLCPP_INFO(this->get_logger(),
                "Sink '%s' updated pose to (%.1f, %.1f, %.1f)",
                sink_id_.c_str(),
                sink_pose_.position.x,
                sink_pose_.position.y,
                sink_pose_.position.z);
  }

  // Members
  std::string sink_id_;
  std::string uplink_ch_id_;
  std::string target_uav_id_;
  uint64_t msg_counter_;
  geometry_msgs::msg::Pose sink_pose_;
  rclcpp::Subscription<uav_msgs::msg::UavDeployment>::SharedPtr deployment_sub_;

  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr    delivered_pub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr    control_pub_;
  rclcpp::TimerBase::SharedPtr                                   control_timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<SinkGatewayNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
