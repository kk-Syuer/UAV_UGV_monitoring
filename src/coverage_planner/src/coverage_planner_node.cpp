#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <cmath>
#include <queue>
#include <limits>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "uav_msgs/msg/uav_deployment.hpp"
#include "uav_msgs/msg/uav_status.hpp"

using namespace std::chrono_literals;

class CoveragePlannerNode : public rclcpp::Node
{
public:
  CoveragePlannerNode()
  : Node("coverage_planner_node")
  {
    // ---- Parameters ----
    uav_ids_ = this->declare_parameter<std::vector<std::string>>(
      "uav_ids", std::vector<std::string>{});

    num_ch_ = this->declare_parameter<int>("num_ch", 1);

    x_min_ = this->declare_parameter<double>("x_min", 0.0);
    x_max_ = this->declare_parameter<double>("x_max", 500.0);
    y_min_ = this->declare_parameter<double>("y_min", 0.0);
    y_max_ = this->declare_parameter<double>("y_max", 500.0);

    z_ch_     = this->declare_parameter<double>("z_ch", 80.0);
    z_member_ = this->declare_parameter<double>("z_member", 60.0);

    service_radius_ch_ = this->declare_parameter<double>("service_radius_ch", 250.0);
    comm_radius_ch_    = this->declare_parameter<double>("comm_radius_ch", 400.0);

    int seed = this->declare_parameter<int>("rng_seed", 42);
    rng_ = std::mt19937(seed);

    deployment_pub_ = this->create_publisher<uav_msgs::msg::UavDeployment>(
      "/coverage_planner/deployment", 10);

    // Timer: handles initial deployment and later routing recomputes
    timer_ = this->create_wall_timer(
      2s, std::bind(&CoveragePlannerNode::periodicUpdate, this));

    // Initialize backbone_active state for the *first num_ch_* UAVs (planned CHs)
    int n = std::min<int>(num_ch_, static_cast<int>(uav_ids_.size()));
    for (int i = 0; i < n; ++i) {
      ch_backbone_active_[uav_ids_[i]] = true;  // start as ACTIVE
    }

    // Subscribe to UAV status to see backbone_active changes
    status_sub_ = this->create_subscription<uav_msgs::msg::UavStatus>(
      "/uav_fleet/status", 20,
      std::bind(&CoveragePlannerNode::statusCallback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(),
                "Coverage planner started. num_ch=%d, uav_ids=%zu, "
                "area=[%.1f,%.1f]x[%.1f,%.1f], R_s=%.1f, R_c=%.1f",
                num_ch_, uav_ids_.size(),
                x_min_, x_max_, y_min_, y_max_,
                service_radius_ch_, comm_radius_ch_);
  }

private:
  // ------------------------------------------------------------------
  // Periodic timer: initial deployment + later routing recompute
  // ------------------------------------------------------------------
  void periodicUpdate()
  {
    // First time: compute full deployment (CHs, members, sink, UGV)
    if (!first_deployment_done_) {
      if (uav_ids_.empty()) {
        RCLCPP_WARN(this->get_logger(),
                    "Coverage planner: no uav_ids provided, nothing to do.");
        first_deployment_done_ = true;
        return;
      }

      computeDeployment();
      first_deployment_done_ = true;
      RCLCPP_INFO(this->get_logger(), "Coverage planner: initial deployment sent.");
      return;
    }

    // Later: only recompute routing if some CH changed backbone state
    if (need_recompute_) {
      RCLCPP_WARN(this->get_logger(),
                  "[planner] Backbone state changed — routing recompute needed");
      need_recompute_ = false;
      recomputeRouting();
    }
  }

  // ------------------------------------------------------------------
  // Random placement of sink & UGV
  // ------------------------------------------------------------------
  void randomizeSinkAndUgv()
  {
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
      dep.role   = 2;   // 2 = sink (for viz only)
      dep.cluster_id = "";
      dep.ch_id      = "";
      dep.next_hop_to_sink = "";
      dep.next_hop_to_ugv  = "";

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
      dep.role   = 3;   // 3 = UGV (for viz only)
      dep.cluster_id = "";
      dep.ch_id      = "";
      dep.next_hop_to_sink = "";
      dep.next_hop_to_ugv  = "";

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

  // ------------------------------------------------------------------
  // Initial deployment (CHs, members, routing to sink & UGV)
  // ------------------------------------------------------------------
  void computeDeployment()
  {
    // 1) Place sink & UGV, publish them
    randomizeSinkAndUgv();
    publishSinkAndUgvDeployment();

    // 2) Decide CH vs members: first num_ch_ are CHs
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

    // ---- CH grid placement ----
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

    // Store for later dynamic routing recompute
    ch_ids_ = ch_ids;
    ch_poses_ = ch_poses;
    cluster_ids_ = cluster_ids;

    // ---- Helper: Dijkstra towards arbitrary ground target (sink or UGV) ----
    auto dist2_xy = [](double x1, double y1, double x2, double y2) {
      double dx = x1 - x2;
      double dy = y1 - y2;
      return dx * dx + dy * dy;
    };

    const double Rc2 = comm_radius_ch_ * comm_radius_ch_;

    auto compute_next_hops_to_target =
      [&](double target_x,
          double target_y,
          const std::string &direct_label,
          const std::string &target_name)
        -> std::vector<std::string>
    {
      const int target_idx  = num_ch;       // extra node for target
      const int num_nodes   = num_ch + 1;   // CHs + target
      const double INF      = std::numeric_limits<double>::infinity();

      std::vector<std::vector<std::pair<int, double>>> adj(num_nodes);

      // CH-CH edges
      for (int i = 0; i < num_ch; ++i) {
        for (int j = i + 1; j < num_ch; ++j) {
          double d2 = dist2_xy(ch_poses[i].position.x, ch_poses[i].position.y,
                               ch_poses[j].position.x, ch_poses[j].position.y);
          if (d2 <= Rc2) {
            double w = std::sqrt(d2);   // weight = distance
            adj[i].push_back({j, w});
            adj[j].push_back({i, w});
          }
        }
      }

      // CH - target edges
      for (int i = 0; i < num_ch; ++i) {
        double d2 = dist2_xy(ch_poses[i].position.x, ch_poses[i].position.y,
                             target_x, target_y);
        if (d2 <= Rc2) {
          double w = std::sqrt(d2);
          adj[i].push_back({target_idx, w});
          adj[target_idx].push_back({i, w});
        }
      }

      // Dijkstra from target_idx
      std::vector<double> dist(num_nodes, INF);
      std::vector<int>    prev(num_nodes, -1);

      using QItem = std::pair<double, int>;
      std::priority_queue<QItem,
                          std::vector<QItem>,
                          std::greater<QItem>> pq;

      dist[target_idx] = 0.0;
      prev[target_idx] = -1;
      pq.push({0.0, target_idx});

      while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[u]) continue;

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

      std::vector<std::string> next_hop(num_ch, "");
      int unreachable_count = 0;

      for (int i = 0; i < num_ch; ++i) {
        if (!std::isfinite(dist[i])) {
          ++unreachable_count;
          next_hop[i].clear();
          continue;
        }

        int parent = prev[i];

        if (parent == target_idx) {
          next_hop[i] = direct_label;          // direct neighbor of sink/UGV
        } else if (parent >= 0 && parent < num_ch) {
          next_hop[i] = ch_ids[static_cast<size_t>(parent)];  // via CH
        } else {
          next_hop[i].clear();
        }

        RCLCPP_INFO(this->get_logger(),
                    "Routing (%s): CH %s -> %s via %s (dist=%.1f m)",
                    target_name.c_str(),
                    ch_ids[static_cast<size_t>(i)].c_str(),
                    target_name.c_str(),
                    next_hop[i].empty()
                      ? "<NONE>"
                      : next_hop[i].c_str(),
                    dist[i]);
      }

      if (unreachable_count > 0) {
        RCLCPP_WARN(this->get_logger(),
                    "Routing warning (%s): %d CH(s) unreachable with R_c=%.1f.",
                    target_name.c_str(),
                    unreachable_count, comm_radius_ch_);
      }

      return next_hop;
    };

    // Compute routing towards sink and towards UGV
    std::vector<std::string> next_hop_to_sink =
      compute_next_hops_to_target(sink_x_, sink_y_, "sink_gateway", "sink");

    std::vector<std::string> next_hop_to_ugv =
      compute_next_hops_to_target(ugv_x_, ugv_y_, "ugv", "ugv");

    // ---- RNG for member positions ----
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist_x(x_min_, x_max_);
    std::uniform_real_distribution<double> dist_y(y_min_, y_max_);

    // ---- Publish CH deployments ----
    for (int i = 0; i < num_ch; ++i) {
      uav_msgs::msg::UavDeployment dep;
      dep.uav_id      = ch_ids[static_cast<size_t>(i)];
      dep.target_pose = ch_poses[static_cast<size_t>(i)];

      dep.role       = 1;  // CH
      dep.cluster_id = cluster_ids[static_cast<size_t>(i)];
      dep.ch_id      = dep.uav_id;
      dep.next_hop_to_sink = next_hop_to_sink[static_cast<size_t>(i)];
      dep.next_hop_to_ugv  = next_hop_to_ugv[static_cast<size_t>(i)];

      RCLCPP_INFO(this->get_logger(),
                  "Deploy CH %s -> cluster=%s pose=(%.1f, %.1f, %.1f) "
                  "next_hop_to_sink=%s next_hop_to_ugv=%s",
                  dep.uav_id.c_str(),
                  dep.cluster_id.c_str(),
                  dep.target_pose.position.x,
                  dep.target_pose.position.y,
                  dep.target_pose.position.z,
                  dep.next_hop_to_sink.empty()
                    ? "<UNREACHABLE>"
                    : dep.next_hop_to_sink.c_str(),
                  dep.next_hop_to_ugv.empty()
                    ? "<UNREACHABLE>"
                    : dep.next_hop_to_ugv.c_str());

      deployment_pub_->publish(dep);
    }

    // ---- Publish MEMBER deployments (random XY, nearest CH) ----
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
      dep.next_hop_to_sink = "";  // members use their CH
      dep.next_hop_to_ugv  = "";  // members use CH backbone

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

  // ------------------------------------------------------------------
  // Dynamic routing recompute when CH backbone_active changes
  // ------------------------------------------------------------------
  void recomputeRouting()
  {
    RCLCPP_WARN(this->get_logger(), "[planner] Recomputing routing...");

    const int num_ch = static_cast<int>(ch_ids_.size());
    if (num_ch == 0) {
      RCLCPP_WARN(this->get_logger(),
                  "[planner] recomputeRouting called but no CHs stored.");
      return;
    }

    // List of ACTIVE CH indices (into ch_ids_/ch_poses_)
    std::vector<int> active_index;
    std::vector<std::string> active_ids;

    for (int i = 0; i < num_ch; ++i) {
      const std::string & id = ch_ids_[i];
      auto it = ch_backbone_active_.find(id);
      bool active = (it == ch_backbone_active_.end()) ? true : it->second;
      if (active) {
        active_index.push_back(i);
        active_ids.push_back(id);
      }
    }

    int A = static_cast<int>(active_index.size());
    if (A == 0) {
      RCLCPP_ERROR(this->get_logger(),
                   "[planner] No active CHs — network fully disconnected!");
      return;
    }

    // Graph node indexing:
    // 0..A-1: active CHs
    // A    : sink
    // A+1  : UGV
    int sink_node = A;
    int ugv_node  = A + 1;
    int N         = A + 2;

    auto dist_xy = [](double x1, double y1, double x2, double y2) {
      double dx = x1 - x2;
      double dy = y1 - y2;
      return std::sqrt(dx*dx + dy*dy);
    };

    std::vector<std::vector<std::pair<int,double>>> adj(N);

    // CH-CH + CH-sink + CH-UGV edges
    for (int iv = 0; iv < A; ++iv) {
      int i = active_index[iv];
      double ix = ch_poses_[i].position.x;
      double iy = ch_poses_[i].position.y;

      // CH-CH
      for (int jv = iv + 1; jv < A; ++jv) {
        int j = active_index[jv];
        double jx = ch_poses_[j].position.x;
        double jy = ch_poses_[j].position.y;
        double d  = dist_xy(ix, iy, jx, jy);
        if (d <= comm_radius_ch_) {
          adj[iv].push_back({jv, d});
          adj[jv].push_back({iv, d});
        }
      }

      // CH-sink
      double ds = dist_xy(ix, iy, sink_x_, sink_y_);
      if (ds <= comm_radius_ch_) {
        adj[iv].push_back({sink_node, ds});
        adj[sink_node].push_back({iv, ds});
      }

      // CH-UGV
      double du = dist_xy(ix, iy, ugv_x_, ugv_y_);
      if (du <= comm_radius_ch_) {
        adj[iv].push_back({ugv_node, du});
        adj[ugv_node].push_back({iv, du});
      }
    }

    // Generic Dijkstra
    auto dijkstra = [&](int src) {
      const double INF = 1e18;
      std::vector<double> dist(N, INF);
      std::vector<int>    prev(N, -1);

      dist[src] = 0.0;
      using P = std::pair<double,int>;
      std::priority_queue<P,std::vector<P>,std::greater<P>> pq;
      pq.push({0.0, src});

      while (!pq.empty()) {
        auto [d,u] = pq.top(); pq.pop();
        if (d > dist[u]) continue;
        for (auto & e : adj[u]) {
          int v = e.first;
          double w = e.second;
          if (dist[u] + w < dist[v]) {
            dist[v] = dist[u] + w;
            prev[v] = u;
            pq.push({dist[v], v});
          }
        }
      }
      return prev;
    };

    auto prev_sink = dijkstra(sink_node);
    auto prev_ugv  = dijkstra(ugv_node);

    // Update and publish routing for ACTIVE CHs
    for (int iv = 0; iv < A; ++iv) {
      int i = active_index[iv];
      const std::string & id = ch_ids_[i];

      std::string nh_sink = "";
      std::string nh_ugv  = "";

      // ---- towards sink ----
      if (prev_sink[iv] != -1) {
        int v = prev_sink[iv];
        if (v == sink_node) {
          nh_sink = "sink_gateway";
        } else if (v >= 0 && v < A) {
          int real = active_index[v];
          nh_sink = ch_ids_[real];
        }
      }

      // ---- towards UGV ----
      if (prev_ugv[iv] != -1) {
        int v = prev_ugv[iv];
        if (v == ugv_node) {
          nh_ugv = "ugv";
        } else if (v >= 0 && v < A) {
          int real = active_index[v];
          nh_ugv = ch_ids_[real];
        }
      }

      uav_msgs::msg::UavDeployment dep;
      dep.uav_id      = id;
      dep.role        = 1;
      dep.cluster_id  = cluster_ids_[i];
      dep.ch_id       = id;
      dep.target_pose = ch_poses_[i];
      dep.next_hop_to_sink = nh_sink;
      dep.next_hop_to_ugv  = nh_ugv;

      deployment_pub_->publish(dep);

      RCLCPP_WARN(this->get_logger(),
                  "[planner] Updated routing for CH %s: sink=%s  ugv=%s",
                  id.c_str(),
                  nh_sink.empty() ? "<NONE>" : nh_sink.c_str(),
                  nh_ugv.empty()  ? "<NONE>" : nh_ugv.c_str());
    }
  }

  // ------------------------------------------------------------------
  // Status callback: watch CH backbone_active flag
  // ------------------------------------------------------------------
  void statusCallback(const uav_msgs::msg::UavStatus::SharedPtr msg)
  {
    if (msg->role != 1) {
      return;  // only CHs matter for backbone routing
    }

    const std::string & id = msg->uav_id;

    auto it = ch_backbone_active_.find(id);
    if (it == ch_backbone_active_.end()) {
      // Not a planned CH (or planner started before declaring map)
      return;
    }

    bool old_state = it->second;
    bool new_state = msg->backbone_active;

    if (old_state != new_state) {
      it->second = new_state;
      need_recompute_ = true;

      RCLCPP_WARN(this->get_logger(),
                  "CH %s backbone state changed: %s -> %s",
                  id.c_str(),
                  old_state ? "ACTIVE" : "INACTIVE",
                  new_state ? "ACTIVE" : "INACTIVE");
    }
  }

  // ------------------------------------------------------------------
  // Members
  // ------------------------------------------------------------------
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

  // Deployment state
  bool first_deployment_done_ = false;

  rclcpp::Publisher<uav_msgs::msg::UavDeployment>::SharedPtr deployment_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Subscription<uav_msgs::msg::UavStatus>::SharedPtr status_sub_;

  // Backbone state: CH id -> active?
  std::unordered_map<std::string, bool> ch_backbone_active_;
  bool need_recompute_ = false;

  // Persistent CH info for routing recompute
  std::vector<std::string>             ch_ids_;
  std::vector<geometry_msgs::msg::Pose> ch_poses_;
  std::vector<std::string>             cluster_ids_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<CoveragePlannerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
