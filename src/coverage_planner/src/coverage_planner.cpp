#include "coverage_planner.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

CoveragePlanner::CoveragePlanner(double area_min_x,
                                 double area_max_x,
                                 double area_min_y,
                                 double area_max_y,
                                 double comm_radius,
                                 double service_radius,
                                 const rclcpp::Logger & logger,
                                 rclcpp::Publisher<uav_msgs::msg::UavDeployment>::SharedPtr deployment_pub)
: area_min_x_(area_min_x),
  area_max_x_(area_max_x),
  area_min_y_(area_min_y),
  area_max_y_(area_max_y),
  comm_radius_(comm_radius),
  service_radius_(service_radius),
  layout_spacing_(0.0),
  logger_(logger),
  deployment_pub_(std::move(deployment_pub))
{
  const double base_radius = std::max(1.0, std::min(comm_radius_, service_radius_));
  layout_spacing_ = base_radius / std::sqrt(3.0);  // neighbor distance <= base_radius
}

void CoveragePlanner::initializeIdealLayouts()
{
  ideal_layouts_.clear();

  // Precompute layouts for up to 20 CHs using a hexagonal lattice that expands
  // outward from the origin. This guarantees connectivity (neighbor distance
  // <= min(comm_radius, service_radius)) and keeps the sink at the center of
  // the first Voronoi cell.
  const int max_layout = 20;
  for (int n = 1; n <= max_layout; ++n) {
    ideal_layouts_[n] = generateHexLayout(n);
  }
}

void CoveragePlanner::setActiveClusterHeads(const std::vector<ClusterHeadInfo> & active_chs)
{
  active_cluster_heads_ = active_chs;
  std::sort(active_cluster_heads_.begin(), active_cluster_heads_.end(),
    [](const ClusterHeadInfo & a, const ClusterHeadInfo & b) {
      return a.id < b.id;
    });
}

std::vector<CoveragePlanner::TargetPosition> CoveragePlanner::computeAssignments(
  const std::vector<ClusterHeadInfo> & active_chs) const
{
  std::vector<TargetPosition> assignments;
  assignments.reserve(active_chs.size());

  const int n = static_cast<int>(active_chs.size());
  auto it = ideal_layouts_.find(n);
  if (it == ideal_layouts_.end()) {
    RCLCPP_WARN(logger_, "No ideal layout for %d active CHs. Keeping current positions.", n);
    for (const auto & ch : active_chs) {
      assignments.push_back(TargetPosition{ch.x, ch.y});
    }
    return assignments;
  }

  const Layout & layout = it->second;

  // Track which indices are still available
  std::vector<int> ch_indices(n);
  std::vector<int> pos_indices(n);
  for (int i = 0; i < n; ++i) {
    ch_indices[i] = i;
    pos_indices[i] = i;
  }

  assignments.assign(n, TargetPosition{});

  while (!ch_indices.empty() && !pos_indices.empty()) {
    double best_dist2 = std::numeric_limits<double>::infinity();
    int best_ch_idx = 0;
    int best_pos_idx = 0;

    for (int ci = 0; ci < static_cast<int>(ch_indices.size()); ++ci) {
      int ch_idx = ch_indices[static_cast<size_t>(ci)];
      for (int pi = 0; pi < static_cast<int>(pos_indices.size()); ++pi) {
        int pos_idx = pos_indices[static_cast<size_t>(pi)];
        double d2 = dist2(active_chs[static_cast<size_t>(ch_idx)].x,
                          active_chs[static_cast<size_t>(ch_idx)].y,
                          layout[static_cast<size_t>(pos_idx)].x,
                          layout[static_cast<size_t>(pos_idx)].y);
        if (d2 < best_dist2) {
          best_dist2 = d2;
          best_ch_idx = ci;
          best_pos_idx = pi;
        }
      }
    }

    int chosen_ch = ch_indices[static_cast<size_t>(best_ch_idx)];
    int chosen_pos = pos_indices[static_cast<size_t>(best_pos_idx)];
    assignments[static_cast<size_t>(chosen_ch)] = layout[static_cast<size_t>(chosen_pos)];

    ch_indices.erase(ch_indices.begin() + best_ch_idx);
    pos_indices.erase(pos_indices.begin() + best_pos_idx);
  }

  return assignments;
}

void CoveragePlanner::updateClusterHeadTargets()
{
  if (!deployment_pub_) {
    RCLCPP_WARN(logger_, "Deployment publisher not available; skipping CH target update.");
    return;
  }

  if (active_cluster_heads_.empty()) {
    RCLCPP_INFO(logger_, "No active cluster heads to update.");
    return;
  }

  auto targets = computeAssignments(active_cluster_heads_);
  for (size_t i = 0; i < active_cluster_heads_.size(); ++i) {
    const auto & ch = active_cluster_heads_[i];
    const auto & tgt = targets[i];

    uav_msgs::msg::UavDeployment dep;
    dep.uav_id = ch.id;
    dep.cluster_id = ch.cluster_id;
    dep.role = 1;  // CH
    dep.ch_id = ch.id;
    dep.next_hop_to_sink = "";
    dep.next_hop_to_ugv = "";
    dep.target_pose.position.x = tgt.x;
    dep.target_pose.position.y = tgt.y;
    dep.target_pose.position.z = ch.z;
    dep.target_pose.orientation.w = 1.0;

    deployment_pub_->publish(dep);
    RCLCPP_INFO(logger_, "Assigned CH %s to target (%.1f, %.1f)",
                ch.id.c_str(), tgt.x, tgt.y);
  }
}

double CoveragePlanner::dist2(double x1, double y1, double x2, double y2)
{
  const double dx = x1 - x2;
  const double dy = y1 - y2;
  return dx * dx + dy * dy;
}

CoveragePlanner::TargetPosition CoveragePlanner::axialToCartesian(int q, int r, double spacing)
{
  // Pointy-top axial to Cartesian conversion.
  double x = spacing * (std::sqrt(3.0) * (static_cast<double>(q) + static_cast<double>(r) / 2.0));
  double y = spacing * (1.5 * static_cast<double>(r));
  return TargetPosition{x, y};
}

CoveragePlanner::Layout CoveragePlanner::generateHexLayout(int n) const
{
  Layout layout;
  layout.reserve(static_cast<size_t>(n));

  layout.push_back(TargetPosition{0.0, 0.0});
  if (n == 1) {
    return layout;
  }

  std::vector<std::pair<int, int>> directions = {
    {1, 0}, {0, 1}, {-1, 1}, {-1, 0}, {0, -1}, {1, -1}
  };

  int q = 0;
  int r = 0;
  int placed = 1;

  for (int radius = 1; placed < n; ++radius) {
    q = radius;
    r = 0;

    for (const auto & dir : directions) {
      for (int step = 0; step < radius && placed < n; ++step) {
        layout.push_back(axialToCartesian(q, r, layout_spacing_));
        ++placed;
        q += dir.first;
        r += dir.second;
      }
    }
  }

  return layout;
}

