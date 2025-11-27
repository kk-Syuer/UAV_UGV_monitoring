#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <cmath>
#include <queue>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "uav_msgs/msg/uav_deployment.hpp"
#include <limits>


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

    // How many of them should be CHs (first num_ch_ in uav_ids_)
    num_ch_ = this->declare_parameter<int>("num_ch", 1);

    // Area bounds (2D rectangle); 3D via z_ch/z_member
    x_min_ = this->declare_parameter<double>("x_min", 0.0);
    x_max_ = this->declare_parameter<double>("x_max", 500.0);
    y_min_ = this->declare_parameter<double>("y_min", 0.0);
    y_max_ = this->declare_parameter<double>("y_max", 500.0);

    // Altitudes
    z_ch_     = this->declare_parameter<double>("z_ch", 80.0);
    z_member_ = this->declare_parameter<double>("z_member", 60.0);

    // Service & communication radius for CHs (for diagnostics)
    service_radius_ch_ = this->declare_parameter<double>("service_radius_ch", 250.0);
    comm_radius_ch_    = this->declare_parameter<double>("comm_radius_ch", 400.0);

    // Random engine (optional seed param to get repeatable runs)
    int seed = this->declare_parameter<int>("rng_seed", 42);
    rng_ = std::mt19937(seed);

    deployment_pub_ = this->create_publisher<uav_msgs::msg::UavDeployment>(
      "/coverage_planner/deployment", 10);

    timer_ = this->create_wall_timer(
      2s, std::bind(&CoveragePlannerNode::timerCallback, this));

    RCLCPP_INFO(this->get_logger(),
                "Coverage planner started. num_ch=%d, uav_ids=%zu, "
                "area=[%.1f,%.1f]x[%.1f,%.1f], R_s=%.1f, R_c=%.1f",
                num_ch_, uav_ids_.size(),
                x_min_, x_max_, y_min_, y_max_,
                service_radius_ch_, comm_radius_ch_);
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

  void randomizeSinkAndUgv()
  {
    // If already placed (e.g. we want deterministic behaviour) do nothing.
    if (sink_placed_ && ugv_placed_) {
      return;
    }

    std::uniform_real_distribution<double> dist_x(x_min_, x_max_);
    std::uniform_real_distribution<double> dist_y(y_min_, y_max_);

    sink_x_ = dist_x(rng_);
    sink_y_ = dist_y(rng_);
    sink_placed_ = true;

    ugv_x_ = dist_x(rng_);
    ugv_y_ = dist_y(rng_);
    ugv_placed_ = true;

    RCLCPP_INFO(this->get_logger(),
                "Randomized sink=(%.1f, %.1f), UGV=(%.1f, %.1f)",
                sink_x_, sink_y_, ugv_x_, ugv_y_);
  }

  void publishSinkAndUgvDeployment()
  {
    // Sink
    {
      uav_msgs::msg::UavDeployment dep;
      dep.uav_id = "sink_gateway";
      dep.role   = 2;   // 2 = sink (convention for viz, doesn't matter elsewhere)
      dep.cluster_id = "";
      dep.ch_id      = "";
      dep.next_hop_to_sink = "";

      dep.target_pose.position.x = sink_x_;
      dep.target_pose.position.y = sink_y_;
      dep.target_pose.position.z = 0.0;
      dep.target_pose.orientation.w = 1.0;

      deployment_pub_->publish(dep);
      RCLCPP_INFO(this->get_logger(),
                  "Deploy SINK sink_gateway at (%.1f, %.1f)",
                  sink_x_, sink_y_);
    }

    // UGV
    {
      uav_msgs::msg::UavDeployment dep;
      dep.uav_id = "ugv";
      dep.role   = 3;    // 3 = UGV (for viz only)
      dep.cluster_id = "";
      dep.ch_id      = "";
      dep.next_hop_to_sink = "";

      dep.target_pose.position.x = ugv_x_;
      dep.target_pose.position.y = ugv_y_;
      dep.target_pose.position.z = 0.0;
      dep.target_pose.orientation.w = 1.0;

      deployment_pub_->publish(dep);
      RCLCPP_INFO(this->get_logger(),
                  "Deploy UGV ugv at (%.1f, %.1f)",
                  ugv_x_, ugv_y_);
    }
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

    double width  = x_max_ - x_min_;
    double height = y_max_ - y_min_;

    // ---- 1) Place CHs on a simple grid ----
    int n_rows = static_cast<int>(std::floor(std::sqrt(num_ch)));
    if (n_rows < 1) n_rows = 1;
    int n_cols = (num_ch + n_rows - 1) / n_rows;  // ceil

    double dx = (n_cols > 0) ? (width  / static_cast<double>(n_cols)) : 0.0;
    double dy = (n_rows > 0) ? (height / static_cast<double>(n_rows)) : 0.0;

    RCLCPP_INFO(this->get_logger(),
                "CH grid: rows=%d cols=%d spacing=(dx=%.1f, dy=%.1f)",
                n_rows, n_cols, dx, dy);

    double cov_limit  = 2.0 * service_radius_ch_;
    double conn_limit = comm_radius_ch_;

    if (dx > cov_limit || dy > cov_limit) {
      RCLCPP_WARN(this->get_logger(),
                  "Coverage warning: spacing (%.1f, %.1f) > 2*R_s=%.1f. "
                  "Area may not be fully covered.",
                  dx, dy, cov_limit);
    }

    if (dx > conn_limit || dy > conn_limit) {
      RCLCPP_WARN(this->get_logger(),
                  "Connectivity warning: spacing (%.1f, %.1f) > R_c=%.1f. "
                  "CH backbone may be disconnected.",
                  dx, dy, conn_limit);
    }

    // Grid centre positions for CHs
    std::vector<geometry_msgs::msg::Pose> ch_poses;
    ch_poses.reserve(ch_ids.size());

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

    // Build cluster IDs: cluster_1, cluster_2, ...
    std::vector<std::string> cluster_ids;
    cluster_ids.reserve(num_ch);
    for (int i = 0; i < num_ch; ++i) {
      cluster_ids.push_back("cluster_" + std::to_string(i + 1));
    }

    // ---- 2) Build CH + sink graph (weighted) and run Dijkstra ----

    const int sink_idx   = num_ch;        // extra node for sink
    const int num_nodes  = num_ch + 1;    // CHs + sink
    const double INF     = std::numeric_limits<double>::infinity();
    const double Rc2     = comm_radius_ch_ * comm_radius_ch_;

    auto dist2_xy = [](double x1, double y1, double x2, double y2) {
      double dx = x1 - x2;
      double dy = y1 - y2;
      return dx * dx + dy * dy;
    };

    // adjacency list: (neighbor, weight)
    std::vector<std::vector<std::pair<int, double>>> adj(num_nodes);

    // CH-CH edges
    for (int i = 0; i < num_ch; ++i) {
      for (int j = i + 1; j < num_ch; ++j) {
        double d2 = dist2_xy(ch_poses[i].position.x, ch_poses[i].position.y,
                             ch_poses[j].position.x, ch_poses[j].position.y);
        if (d2 <= Rc2) {
          double w = std::sqrt(d2);
          adj[i].push_back({j, w});
          adj[j].push_back({i, w});
        }
      }
    }

    // CH-sink edges
    for (int i = 0; i < num_ch; ++i) {
      double d2 = dist2_xy(ch_poses[i].position.x, ch_poses[i].position.y,
                           sink_x_, sink_y_);
      if (d2 <= Rc2) {
        double w = std::sqrt(d2);
        adj[i].push_back({sink_idx, w});
        adj[sink_idx].push_back({i, w});
      }
    }

    // Dijkstra from sink_idx
    std::vector<double> dist(num_nodes, INF);
    std::vector<int>    prev(num_nodes, -1);

    using QItem = std::pair<double, int>;  // (distance, node)
    std::priority_queue<QItem, std::vector<QItem>, std::greater<QItem>> pq;

    dist[sink_idx] = 0.0;
    prev[sink_idx] = -1;
    pq.push({0.0, sink_idx});

    while (!pq.empty()) {
      auto [d, u] = pq.top();
      pq.pop();
      if (d > dist[u]) continue;  // stale entry

      for (const auto & edge : adj[u]) {
        int v       = edge.first;
        double w    = edge.second;
        double cand = d + w;
        if (cand < dist[v]) {
          dist[v] = cand;
          prev[v] = u;
          pq.push({cand, v});
        }
      }
    }

    // For each CH i, compute its next hop towards the sink
    std::vector<std::string> next_hop_to_sink(num_ch, "");
    int unreachable_count = 0;

    for (int i = 0; i < num_ch; ++i) {
      if (!std::isfinite(dist[i])) {
        ++unreachable_count;
        next_hop_to_sink[i].clear();
        continue;
      }

      int parent = prev[i];

      if (parent == sink_idx) {
        // Direct neighbor of sink
        next_hop_to_sink[i] = "sink_gateway";
      } else if (parent >= 0 && parent < num_ch) {
        // Next hop is another CH
        next_hop_to_sink[i] = ch_ids[static_cast<size_t>(parent)];
      } else {
        // Should not happen, but keep safe
        next_hop_to_sink[i].clear();
      }

      RCLCPP_INFO(this->get_logger(),
                  "Routing: CH %s -> sink via %s (dist=%.1f m)",
                  ch_ids[static_cast<size_t>(i)].c_str(),
                  next_hop_to_sink[i].empty()
                    ? "<NONE>"
                    : next_hop_to_sink[i].c_str(),
                  dist[i]);
    }

    if (unreachable_count > 0) {
      RCLCPP_WARN(this->get_logger(),
                  "Routing warning: %d CH(s) unreachable from sink "
                  "with R_c=%.1f. Their next_hop_to_sink will be empty.",
                  unreachable_count, comm_radius_ch_);
    }

    // ---- 3) RNG for member positions ----
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist_x(x_min_, x_max_);
    std::uniform_real_distribution<double> dist_y(y_min_, y_max_);

    // ---- 4) Publish CH deployments ----
    for (int i = 0; i < num_ch; ++i) {
      uav_msgs::msg::UavDeployment dep;
      dep.uav_id      = ch_ids[static_cast<size_t>(i)];
      dep.target_pose = ch_poses[static_cast<size_t>(i)];

      dep.role       = 1;  // CH
      dep.cluster_id = cluster_ids[static_cast<size_t>(i)];
      dep.ch_id      = dep.uav_id;  // CH is its own CH
      dep.next_hop_to_sink = next_hop_to_sink[static_cast<size_t>(i)];

      RCLCPP_INFO(this->get_logger(),
                  "Deploy CH %s -> cluster=%s pose=(%.1f, %.1f, %.1f) next_hop_to_sink=%s",
                  dep.uav_id.c_str(),
                  dep.cluster_id.c_str(),
                  dep.target_pose.position.x,
                  dep.target_pose.position.y,
                  dep.target_pose.position.z,
                  dep.next_hop_to_sink.empty()
                    ? "<UNREACHABLE>"
                    : dep.next_hop_to_sink.c_str());

      deployment_pub_->publish(dep);
    }

    // ---- 5) Publish MEMBER deployments (random XY, nearest CH) ----
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
        double dxm = pose.position.x - ch_pose.position.x;
        double dym = pose.position.y - ch_pose.position.y;
        double d2  = dxm * dxm + dym * dym;
        if (d2 < best_d2) {
          best_d2 = d2;
          best_ch_idx = i;
        }
      }

      uav_msgs::msg::UavDeployment dep;
      dep.uav_id      = id;
      dep.target_pose = pose;

      dep.role       = 0;  // MEMBER
      dep.cluster_id = cluster_ids[static_cast<size_t>(best_ch_idx)];
      dep.ch_id      = ch_ids[static_cast<size_t>(best_ch_idx)];
      dep.next_hop_to_sink = "";  // members always go via their CH

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
  double service_radius_ch_;
  double comm_radius_ch_;

  // Random sink / UGV
  double sink_x_ = 0.0, sink_y_ = 0.0;
  double ugv_x_  = 0.0, ugv_y_  = 0.0;
  bool sink_placed_ = false;
  bool ugv_placed_  = false;
  std::mt19937 rng_;

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
