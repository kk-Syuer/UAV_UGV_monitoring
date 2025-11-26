#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "uav_msgs/msg/uav_deployment.hpp"

using namespace std::chrono_literals;

class CoveragePlannerNode : public rclcpp::Node
{
public:
  CoveragePlannerNode()
  : Node("coverage_planner_node")
  {
    // List of all UAV IDs participating in this scenario
    uav_ids_ = this->declare_parameter<std::vector<std::string>>(
      "uav_ids", std::vector<std::string>{});

    // How many of them should be CHs (they will be the first num_ch_ in uav_ids_)
    num_ch_ = this->declare_parameter<int>("num_ch", 1);

    // Area bounds (2D rectangle); 3D via z_ch/z_member
    x_min_ = this->declare_parameter<double>("x_min", 0.0);
    x_max_ = this->declare_parameter<double>("x_max", 500.0);
    y_min_ = this->declare_parameter<double>("y_min", 0.0);
    y_max_ = this->declare_parameter<double>("y_max", 500.0);

    // Altitudes
    z_ch_     = this->declare_parameter<double>("z_ch", 80.0);
    z_member_ = this->declare_parameter<double>("z_member", 60.0);

    deployment_pub_ = this->create_publisher<uav_msgs::msg::UavDeployment>(
      "/coverage_planner/deployment", 10);

    timer_ = this->create_wall_timer(
      2s, std::bind(&CoveragePlannerNode::timerCallback, this));

    RCLCPP_INFO(this->get_logger(),
                "Coverage planner started. num_ch=%d, uav_ids=%zu, area=[%.1f,%.1f]x[%.1f,%.1f]",
                num_ch_, uav_ids_.size(), x_min_, x_max_, y_min_, y_max_);
  }

private:
  void timerCallback()
  {
    if (planned_) {
      return;  // only do initial deployment once for now
    }

    if (uav_ids_.empty()) {
      RCLCPP_WARN(this->get_logger(),
                  "Coverage planner: no uav_ids provided, nothing to do.");
      planned_ = true;
      return;
    }

    computeDeployment();
    planned_ = true;
    RCLCPP_INFO(this->get_logger(), "Coverage planner: initial deployment sent.");
  }

  void computeDeployment()
  {
    // Decide CH vs members: first num_ch_ are CHs
    int n = std::min<int>(num_ch_, static_cast<int>(uav_ids_.size()));
    std::vector<std::string> ch_ids(uav_ids_.begin(), uav_ids_.begin() + n);
    std::vector<std::string> member_ids(uav_ids_.begin() + n, uav_ids_.end());

    int num_ch = static_cast<int>(ch_ids.size());
    if (num_ch == 0) {
      RCLCPP_WARN(this->get_logger(),
                  "Coverage planner: num_ch=0 or no UAVs -> no CHs selected.");
      return;
    }

    // Place CHs on a grid over the rectangle
    std::vector<geometry_msgs::msg::Pose> ch_poses;
    ch_poses.reserve(ch_ids.size());

    int n_rows = static_cast<int>(std::floor(std::sqrt(num_ch)));
    if (n_rows < 1) n_rows = 1;
    int n_cols = (num_ch + n_rows - 1) / n_rows;  // ceil

    double width  = x_max_ - x_min_;
    double height = y_max_ - y_min_;

    int idx = 0;
    for (int r = 0; r < n_rows; ++r) {
      for (int c = 0; c < n_cols; ++c) {
        if (idx >= num_ch) break;

        double fx = (static_cast<double>(c) + 0.5) / static_cast<double>(n_cols);
        double fy = (static_cast<double>(r) + 0.5) / static_cast<double>(n_rows);

        geometry_msgs::msg::Pose pose;
        pose.position.x = x_min_ + fx * width;
        pose.position.y = y_min_ + fy * height;
        pose.position.z = z_ch_;
        pose.orientation.w = 1.0;

        ch_poses.push_back(pose);
        idx++;
      }
    }

    // Build mapping from CH index -> cluster_id (cluster_1, cluster_2, ...)
    std::vector<std::string> cluster_ids;
    cluster_ids.reserve(num_ch);
    for (int i = 0; i < num_ch; ++i) {
      cluster_ids.push_back("cluster_" + std::to_string(i + 1));
    }

    // RNG for member positions
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist_x(x_min_, x_max_);
    std::uniform_real_distribution<double> dist_y(y_min_, y_max_);

    // --- Deploy CHs ---
    for (int i = 0; i < num_ch; ++i) {
      uav_msgs::msg::UavDeployment dep;
      dep.uav_id = ch_ids[static_cast<size_t>(i)];
      dep.target_pose = ch_poses[static_cast<size_t>(i)];

      dep.role = 1;  // CH
      dep.cluster_id = cluster_ids[static_cast<size_t>(i)];
      dep.ch_id = dep.uav_id;  // CH is its own CH

      RCLCPP_INFO(this->get_logger(),
                  "Deploy CH %s -> cluster=%s pose=(%.1f, %.1f, %.1f)",
                  dep.uav_id.c_str(),
                  dep.cluster_id.c_str(),
                  dep.target_pose.position.x,
                  dep.target_pose.position.y,
                  dep.target_pose.position.z);

      deployment_pub_->publish(dep);
    }

    // --- Deploy members (random position, assigned to nearest CH) ---
    for (const auto & id : member_ids) {
      geometry_msgs::msg::Pose pose;
      pose.position.x = dist_x(gen);
      pose.position.y = dist_y(gen);
      pose.position.z = z_member_;
      pose.orientation.w = 1.0;

      // Find nearest CH in XY
      double best_d2 = std::numeric_limits<double>::infinity();
      int best_ch_idx = 0;

      for (int i = 0; i < num_ch; ++i) {
        const auto & ch_pose = ch_poses[static_cast<size_t>(i)];
        double dx = pose.position.x - ch_pose.position.x;
        double dy = pose.position.y - ch_pose.position.y;
        double d2 = dx * dx + dy * dy;
        if (d2 < best_d2) {
          best_d2 = d2;
          best_ch_idx = i;
        }
      }

      uav_msgs::msg::UavDeployment dep;
      dep.uav_id = id;
      dep.target_pose = pose;

      dep.role = 0;  // MEMBER
      dep.cluster_id = cluster_ids[static_cast<size_t>(best_ch_idx)];
      dep.ch_id = ch_ids[static_cast<size_t>(best_ch_idx)];

      RCLCPP_INFO(this->get_logger(),
                  "Deploy MEMBER %s -> cluster=%s CH=%s pose=(%.1f, %.1f, %.1f)",
                  dep.uav_id.c_str(),
                  dep.cluster_id.c_str(),
                  dep.ch_id.c_str(),
                  dep.target_pose.position.x,
                  dep.target_pose.position.y,
                  dep.target_pose.position.z);

      deployment_pub_->publish(dep);
    }
  }

  // Parameters
  std::vector<std::string> uav_ids_;
  int num_ch_;
  double x_min_, x_max_, y_min_, y_max_;
  double z_ch_, z_member_;

  bool planned_ = false;

  rclcpp::Publisher<uav_msgs::msg::UavDeployment>::SharedPtr deployment_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<CoveragePlannerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
