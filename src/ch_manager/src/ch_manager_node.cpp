#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/cluster_info.hpp"
#include "uav_msgs/msg/failure_event.hpp"

using namespace std::chrono_literals;

class ChManagerNode : public rclcpp::Node
{
public:
  ChManagerNode()
  : Node("ch_manager_node")
  {
    // ---- Parameters ----
    cluster_id_ = this->declare_parameter<std::string>("cluster_id", "cluster_1");
    ch_id_      = this->declare_parameter<std::string>("ch_id", "uav_1");
    member_ids_ = this->declare_parameter<std::vector<std::string>>(
      "member_ids", std::vector<std::string>({"uav_1"}));

    // ---- Publisher ----
    pub_ = this->create_publisher<uav_msgs::msg::ClusterInfo>(
      "/ch_manager/cluster_info", 10);

    // ---- Timer: periodically publish cluster info ----
    timer_ = this->create_wall_timer(
      1s, std::bind(&ChManagerNode::publishClusterInfo, this));

    // ---- NEW: subscribe to failure events ----
    failure_sub_ = this->create_subscription<uav_msgs::msg::FailureEvent>(
      "/uav_fleet/failure_events", 100,
      std::bind(&ChManagerNode::failureCallback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(),
                "CH Manager started: cluster=%s ch=%s members=%zu",
                cluster_id_.c_str(), ch_id_.c_str(), member_ids_.size());
  }

private:
  // ----- Publish cluster info -----
  void publishClusterInfo()
  {
    uav_msgs::msg::ClusterInfo msg;
    msg.cluster_id = cluster_id_;
    msg.ch_id = ch_id_;
    msg.member_ids = member_ids_;
    pub_->publish(msg);
  }

  // ----- NEW: detect CH or member failure -----
  void failureCallback(const uav_msgs::msg::FailureEvent::SharedPtr msg)
  {
    if (msg->failure_type != 1)  // 1 = BATTERY_DEAD
      return;

    rclcpp::Time t(msg->stamp);

    if (msg->uav_id == ch_id_) {
      RCLCPP_WARN(this->get_logger(),
                  "[CH-FAIL] Cluster %s: CH %s is BATTERY_DEAD at t=%.3f",
                  cluster_id_.c_str(), ch_id_.c_str(), t.seconds());
    } else {
      RCLCPP_WARN(this->get_logger(),
                  "[MEM-FAIL] Cluster %s: member %s is BATTERY_DEAD",
                  cluster_id_.c_str(), msg->uav_id.c_str());
    }
  }

  // ---- Members ----
  std::string cluster_id_;
  std::string ch_id_;
  std::vector<std::string> member_ids_;

  rclcpp::Publisher<uav_msgs::msg::ClusterInfo>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;

  // ---- NEW ----
  rclcpp::Subscription<uav_msgs::msg::FailureEvent>::SharedPtr failure_sub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ChManagerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
