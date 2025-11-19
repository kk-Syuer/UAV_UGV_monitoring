#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/cluster_info.hpp"

using namespace std::chrono_literals;

class ChManagerNode : public rclcpp::Node
{
public:
  ChManagerNode()
  : Node("ch_manager_node")
  {
    cluster_id_ = this->declare_parameter<std::string>("cluster_id", "cluster_1");
    ch_id_      = this->declare_parameter<std::string>("ch_id", "uav_1");
    member_ids_ = this->declare_parameter<std::vector<std::string>>(
      "member_ids", std::vector<std::string>({"uav_1"}));

    pub_ = this->create_publisher<uav_msgs::msg::ClusterInfo>(
      "/ch_manager/cluster_info", 10);

    timer_ = this->create_wall_timer(
      1s, std::bind(&ChManagerNode::publishClusterInfo, this));

    RCLCPP_INFO(this->get_logger(),
                "CH Manager started: cluster=%s ch=%s members=%zu",
                cluster_id_.c_str(), ch_id_.c_str(), member_ids_.size());
  }

private:
  void publishClusterInfo()
  {
    uav_msgs::msg::ClusterInfo msg;
    msg.cluster_id = cluster_id_;
    msg.ch_id = ch_id_;
    msg.member_ids = member_ids_;
    pub_->publish(msg);
  }

  std::string cluster_id_;
  std::string ch_id_;
  std::vector<std::string> member_ids_;

  rclcpp::Publisher<uav_msgs::msg::ClusterInfo>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ChManagerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
