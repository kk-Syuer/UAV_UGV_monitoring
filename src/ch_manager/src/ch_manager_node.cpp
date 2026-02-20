#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/cluster_info.hpp"
#include "uav_msgs/msg/failure_event.hpp"
#include "uav_msgs/msg/traffic_message.hpp"

using namespace std::chrono_literals;

// Cluster head manager: publishes cluster membership and reacts to failures.
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

    // Publisher periodically shares cluster membership for discovery.
    pub_ = this->create_publisher<uav_msgs::msg::ClusterInfo>(
      "/ch_manager/cluster_info", 10);

    // Timer drives periodic cluster-info broadcasts.
    timer_ = this->create_wall_timer(
      1s, std::bind(&ChManagerNode::publishClusterInfo, this));

    // Listen for failure events to log CH/member battery deaths.
    failure_sub_ = this->create_subscription<uav_msgs::msg::FailureEvent>(
      "/uav_fleet/failure_events", 100,
      std::bind(&ChManagerNode::failureCallback, this, std::placeholders::_1));

    // Listen for recovery CLUSTER_REASSIGN messages to update membership.
    network_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/fanet/network_bus", 200,
      std::bind(&ChManagerNode::networkCallback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(),
                "CH Manager started: cluster=%s ch=%s members=%zu",
                cluster_id_.c_str(), ch_id_.c_str(), member_ids_.size());
  }

private:
  // Publish cluster info snapshot used by members and observers.
  void publishClusterInfo()
  {
    uav_msgs::msg::ClusterInfo msg;
    msg.cluster_id = cluster_id_;
    msg.ch_id = ch_id_;
    msg.member_ids = member_ids_;
    pub_->publish(msg);
  }

  // Detect CH/member battery failures and log for operators.
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
      // Remove dead member from our list
      auto it = std::find(member_ids_.begin(), member_ids_.end(), msg->uav_id);
      if (it != member_ids_.end()) {
        member_ids_.erase(it);
        RCLCPP_WARN(this->get_logger(),
                    "[MEM-FAIL] Cluster %s: member %s BATTERY_DEAD, removed (remaining=%zu)",
                    cluster_id_.c_str(), msg->uav_id.c_str(), member_ids_.size());
      } else {
        RCLCPP_WARN(this->get_logger(),
                    "[MEM-FAIL] Cluster %s: member %s is BATTERY_DEAD (not in our list)",
                    cluster_id_.c_str(), msg->uav_id.c_str());
      }
    }
  }

  // React to CLUSTER_REASSIGN from recovery manager to keep membership current.
  void networkCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    if (msg->flow_type != 1 || msg->control_type != "CLUSTER_REASSIGN") {
      return;
    }
    const std::string & member_id = msg->dst_id;
    const std::string & new_ch_id = msg->payload;

    if (new_ch_id == ch_id_) {
      // Member reassigned TO this cluster — add if not already present
      auto it = std::find(member_ids_.begin(), member_ids_.end(), member_id);
      if (it == member_ids_.end()) {
        member_ids_.push_back(member_id);
        RCLCPP_WARN(this->get_logger(),
                    "[RECOVERY] Cluster %s: member %s joined (reassigned from recovery)",
                    cluster_id_.c_str(), member_id.c_str());
      }
    } else {
      // Member reassigned AWAY from this cluster — remove if present
      auto it = std::find(member_ids_.begin(), member_ids_.end(), member_id);
      if (it != member_ids_.end()) {
        member_ids_.erase(it);
        RCLCPP_WARN(this->get_logger(),
                    "[RECOVERY] Cluster %s: member %s reassigned to CH %s, removed",
                    cluster_id_.c_str(), member_id.c_str(), new_ch_id.c_str());
      }
    }
  }

  // ---- Members ----
  std::string cluster_id_;
  std::string ch_id_;
  std::vector<std::string> member_ids_;

  rclcpp::Publisher<uav_msgs::msg::ClusterInfo>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;

  rclcpp::Subscription<uav_msgs::msg::FailureEvent>::SharedPtr failure_sub_;
  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr network_sub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ChManagerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
