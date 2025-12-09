#pragma once

#include <map>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/uav_deployment.hpp"

class CoveragePlanner
{
public:
  struct ClusterHeadInfo
  {
    std::string id;
    std::string cluster_id;
    double x{};
    double y{};
    double z{};
  };

  struct TargetPosition
  {
    double x{};
    double y{};
  };

  using Layout = std::vector<TargetPosition>;

  CoveragePlanner(double area_min_x,
                  double area_max_x,
                  double area_min_y,
                  double area_max_y,
                  double comm_radius,
                  double service_radius,
                  const rclcpp::Logger & logger,
                  rclcpp::Publisher<uav_msgs::msg::UavDeployment>::SharedPtr deployment_pub);

  void initializeIdealLayouts();

  void setActiveClusterHeads(const std::vector<ClusterHeadInfo> & active_chs);

  std::vector<TargetPosition> computeAssignments(
    const std::vector<ClusterHeadInfo> & active_chs) const;

  void updateClusterHeadTargets();

private:
  double area_min_x_;
  double area_max_x_;
  double area_min_y_;
  double area_max_y_;
  double comm_radius_;
  double service_radius_;
  double layout_spacing_;

  rclcpp::Logger logger_;
  rclcpp::Publisher<uav_msgs::msg::UavDeployment>::SharedPtr deployment_pub_;

  std::map<int, Layout> ideal_layouts_;
  std::vector<ClusterHeadInfo> active_cluster_heads_;

  static double dist2(double x1, double y1, double x2, double y2);

  static TargetPosition axialToCartesian(int q, int r, double spacing);
  Layout generateHexLayout(int n) const;
};

