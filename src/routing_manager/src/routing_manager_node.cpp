#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "std_msgs/msg/string.hpp"
#include "uav_msgs/msg/uav_status.hpp"
#include "uav_msgs/msg/routing_table.hpp"

using namespace std::chrono_literals;

namespace {
struct NodeInfo
{
  std::string id;
  uint8_t role = 0;
  geometry_msgs::msg::Pose pose;
  rclcpp::Time stamp;
  float comm_radius_m = 0.0f;
  float traffic_load = 0.0f;
  float packet_loss_estimate = 0.0f;
};

struct EdgeKey
{
  std::string a;
  std::string b;

  bool operator==(const EdgeKey & other) const
  {
    return a == other.a && b == other.b;
  }
};

struct EdgeKeyHash
{
  std::size_t operator()(const EdgeKey & key) const
  {
    return std::hash<std::string>{}(key.a) ^ (std::hash<std::string>{}(key.b) << 1);
  }
};

double distance2d(const geometry_msgs::msg::Point & a, const geometry_msgs::msg::Point & b)
{
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}

enum class NodeClass
{
  BackboneCh,
  Endpoint
};

bool isBackboneCh(uint8_t role)
{
  return role == 1;
}
}  // namespace

class RoutingManagerNode : public rclcpp::Node
{
public:
  RoutingManagerNode()
  : Node("routing_manager_node")
  {
    comm_range_m_ = this->declare_parameter<double>("comm_range_m", 400.0);
    hysteresis_margin_m_ = this->declare_parameter<double>("hysteresis_margin_m", 3.0);
    recompute_period_sec_ = this->declare_parameter<double>("recompute_period_sec", 2.0);
    status_timeout_sec_ = this->declare_parameter<double>("status_timeout_sec", 5.0);
    ch_move_threshold_m_ = this->declare_parameter<double>("ch_move_threshold_m", 7.5);
    log_routes_ = this->declare_parameter<bool>("log_routes", true);
    sink_id_ = this->declare_parameter<std::string>("sink_id", "sink_gateway");
    ugv_id_ = this->declare_parameter<std::string>("ugv_id", "ugv");

    status_sub_ = this->create_subscription<uav_msgs::msg::UavStatus>(
      "/fanet/status", 200,
      std::bind(&RoutingManagerNode::statusCallback, this, std::placeholders::_1));

    event_sub_ = this->create_subscription<std_msgs::msg::String>(
      "/fanet/routing_event", 50,
      std::bind(&RoutingManagerNode::eventCallback, this, std::placeholders::_1));

    routing_pub_ = this->create_publisher<uav_msgs::msg::RoutingTable>(
      "/fanet/routing_table", 50);
    alert_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/routing_manager/alerts", 10);

    auto period = std::chrono::duration<double>(recompute_period_sec_);
    recompute_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(period),
      std::bind(&RoutingManagerNode::recomputeTimer, this));

    RCLCPP_INFO(this->get_logger(),
                "Routing manager started (R=%.1f, margin=%.1f, period=%.2fs, ch_move=%.1f, timeout=%.1f).",
                comm_range_m_, hysteresis_margin_m_, recompute_period_sec_,
                ch_move_threshold_m_, status_timeout_sec_);
  }

private:
  void statusCallback(const uav_msgs::msg::UavStatus::SharedPtr msg)
  {
    NodeInfo info;
    info.id = msg->uav_id;
    info.role = msg->role;
    info.pose = msg->pose;
    info.stamp = msg->stamp;
    info.comm_radius_m = msg->comm_radius_m;
    info.traffic_load = msg->traffic_load;
    info.packet_loss_estimate = msg->packet_loss_estimate;

    auto it = nodes_.find(info.id);
    if (it != nodes_.end() && info.role == 1) {
      double moved = distance2d(it->second.pose.position, info.pose.position);
      if (moved >= ch_move_threshold_m_) {
        recompute_due_ = true;
        RCLCPP_INFO(this->get_logger(),
                    "[routing] CH %s moved %.1f m -> recompute requested",
                    info.id.c_str(), moved);
      }
    }

    if (info.role == 1) {
      RCLCPP_DEBUG(this->get_logger(),
                   "[routing] CH beacon %s pos=(%.1f,%.1f) load=%.2f loss=%.2f",
                   info.id.c_str(),
                   info.pose.position.x, info.pose.position.y,
                   info.traffic_load, info.packet_loss_estimate);
    }

    nodes_[info.id] = info;
  }

  void eventCallback(const std_msgs::msg::String::SharedPtr msg)
  {
    recompute_due_ = true;
    RCLCPP_WARN(this->get_logger(), "[routing] event-driven recompute: %s", msg->data.c_str());
    recomputeRoutes();
  }

  void recomputeTimer()
  {
    if (!recompute_due_) {
      recomputeRoutes();
      return;
    }
    recompute_due_ = false;
    recomputeRoutes();
  }

  void recomputeRoutes()
  {
    if (!routing_pub_) {
      return;
    }

    const rclcpp::Time now = this->now();
    std::vector<NodeInfo> active_nodes;
    active_nodes.reserve(nodes_.size());
    for (const auto & kv : nodes_) {
      const auto & info = kv.second;
      if ((now - info.stamp).seconds() <= status_timeout_sec_) {
        active_nodes.push_back(info);
      }
    }

    if (active_nodes.empty()) {
      return;
    }

    std::sort(active_nodes.begin(), active_nodes.end(),
              [](const NodeInfo & a, const NodeInfo & b) {
                return a.id < b.id;
              });

    std::vector<NodeClass> node_class(active_nodes.size(), NodeClass::Endpoint);
    std::vector<size_t> ch_node_indices;
    std::vector<size_t> endpoint_indices;
    ch_node_indices.reserve(active_nodes.size());
    endpoint_indices.reserve(active_nodes.size());

    for (size_t i = 0; i < active_nodes.size(); ++i) {
      if (isBackboneCh(active_nodes[i].role)) {
        node_class[i] = NodeClass::BackboneCh;
        ch_node_indices.push_back(i);
      } else {
        node_class[i] = NodeClass::Endpoint;
        endpoint_indices.push_back(i);
      }
    }

    std::vector<int> ch_graph_index(active_nodes.size(), -1);
    for (size_t i = 0; i < ch_node_indices.size(); ++i) {
      ch_graph_index[ch_node_indices[i]] = static_cast<int>(i);
    }

    std::vector<std::vector<std::pair<size_t, double>>> ch_graph(ch_node_indices.size());
    for (size_t a_idx = 0; a_idx < ch_node_indices.size(); ++a_idx) {
      for (size_t b_idx = a_idx + 1; b_idx < ch_node_indices.size(); ++b_idx) {
        const auto & a = active_nodes[ch_node_indices[a_idx]];
        const auto & b = active_nodes[ch_node_indices[b_idx]];
        double dist = distance2d(a.pose.position, b.pose.position);

        bool connected = false;
        EdgeKey key{std::min(a.id, b.id), std::max(a.id, b.id)};
        auto state_it = link_state_.find(key);
        bool was_connected = (state_it != link_state_.end()) ? state_it->second : false;

        if (was_connected) {
          connected = dist <= comm_range_m_;
        } else {
          connected = dist <= (comm_range_m_ - hysteresis_margin_m_);
        }

        link_state_[key] = connected;
        if (connected) {
          ch_graph[a_idx].push_back({b_idx, dist});
          ch_graph[b_idx].push_back({a_idx, dist});
        }
      }
    }

    std::vector<int> gateway_index(active_nodes.size(), -1);
    for (size_t endpoint_idx : endpoint_indices) {
      double best_dist = std::numeric_limits<double>::infinity();
      int best_gateway = -1;
      const auto & endpoint = active_nodes[endpoint_idx];
      for (size_t ch_idx : ch_node_indices) {
        const auto & ch = active_nodes[ch_idx];
        double dist = distance2d(endpoint.pose.position, ch.pose.position);
        if (dist <= comm_range_m_ && dist < best_dist) {
          best_dist = dist;
          best_gateway = static_cast<int>(ch_idx);
        }
      }
      gateway_index[endpoint_idx] = best_gateway;
    }

    std::vector<std::vector<int>> ch_next_hops(ch_node_indices.size());
    for (size_t src_ch = 0; src_ch < ch_node_indices.size(); ++src_ch) {
      ch_next_hops[src_ch] = computeRoutesForSource(src_ch, ch_graph);
    }

    int sink_index = -1;
    int ugv_index = -1;
    for (size_t i = 0; i < active_nodes.size(); ++i) {
      if (active_nodes[i].id == sink_id_) {
        sink_index = static_cast<int>(i);
      }
      if (active_nodes[i].id == ugv_id_) {
        ugv_index = static_cast<int>(i);
      }
    }

    bool sink_reachable = false;
    bool ugv_reachable = false;

    for (size_t src_idx = 0; src_idx < active_nodes.size(); ++src_idx) {
      std::vector<int> next_hops(active_nodes.size(), -1);
      if (node_class[src_idx] == NodeClass::Endpoint) {
        int gateway = gateway_index[src_idx];
        if (gateway >= 0) {
          for (size_t dst_idx = 0; dst_idx < active_nodes.size(); ++dst_idx) {
            if (dst_idx == src_idx) {
              continue;
            }
            next_hops[dst_idx] = gateway;
          }
        }
      } else {
        int src_ch_graph = ch_graph_index[src_idx];
        if (src_ch_graph >= 0) {
          for (size_t dst_idx = 0; dst_idx < active_nodes.size(); ++dst_idx) {
            if (dst_idx == src_idx) {
              continue;
            }
            if (node_class[dst_idx] == NodeClass::BackboneCh) {
              int dst_ch_graph = ch_graph_index[dst_idx];
              if (dst_ch_graph >= 0 && dst_ch_graph < static_cast<int>(ch_next_hops[src_ch_graph].size())) {
                int hop_ch_graph = ch_next_hops[src_ch_graph][dst_ch_graph];
                if (hop_ch_graph >= 0 &&
                    hop_ch_graph < static_cast<int>(ch_node_indices.size())) {
                  next_hops[dst_idx] = static_cast<int>(ch_node_indices[hop_ch_graph]);
                }
              }
              continue;
            }

            int dst_gateway = gateway_index[dst_idx];
            if (dst_gateway < 0) {
              continue;
            }
            if (dst_gateway == static_cast<int>(src_idx)) {
              next_hops[dst_idx] = static_cast<int>(dst_idx);
              continue;
            }
            int dst_gateway_graph = ch_graph_index[dst_gateway];
            if (dst_gateway_graph >= 0 &&
                dst_gateway_graph < static_cast<int>(ch_next_hops[src_ch_graph].size())) {
              int hop_ch_graph = ch_next_hops[src_ch_graph][dst_gateway_graph];
              if (hop_ch_graph >= 0 &&
                  hop_ch_graph < static_cast<int>(ch_node_indices.size())) {
                next_hops[dst_idx] = static_cast<int>(ch_node_indices[hop_ch_graph]);
              }
            }
          }
        }
      }

      publishTable(active_nodes, node_class, src_idx, next_hops);

      if (node_class[src_idx] == NodeClass::BackboneCh) {
        if (sink_index >= 0 && next_hops[static_cast<size_t>(sink_index)] >= 0) {
          sink_reachable = true;
        }
        if (ugv_index >= 0 && next_hops[static_cast<size_t>(ugv_index)] >= 0) {
          ugv_reachable = true;
        }
      }
    }

    publishReachabilityAlerts(sink_reachable, ugv_reachable);

    last_recompute_time_ = now;
  }

  std::vector<int> computeRoutesForSource(
    size_t src_idx,
    const std::vector<std::vector<std::pair<size_t, double>>> & graph)
  {
    const size_t n = graph.size();
    std::vector<double> dist(n, std::numeric_limits<double>::infinity());
    std::vector<int> parent(n, -1);

    struct NodeEntry
    {
      double cost;
      size_t idx;
    };
    auto cmp = [](const NodeEntry & a, const NodeEntry & b) { return a.cost > b.cost; };
    std::priority_queue<NodeEntry, std::vector<NodeEntry>, decltype(cmp)> pq(cmp);

    dist[src_idx] = 0.0;
    pq.push({0.0, src_idx});

    while (!pq.empty()) {
      NodeEntry entry = pq.top();
      pq.pop();
      const double cost = entry.cost;
      const size_t u = entry.idx;
      if (cost > dist[u]) {
        continue;
      }

      for (const auto & edge : graph[u]) {
        size_t v = edge.first;
        double w = edge.second;
        if (dist[u] + w < dist[v]) {
          dist[v] = dist[u] + w;
          parent[v] = static_cast<int>(u);
          pq.push({dist[v], v});
        }
      }
    }

    std::vector<int> next_hop(n, -1);
    for (size_t dst = 0; dst < n; ++dst) {
      if (dst == src_idx || !std::isfinite(dist[dst])) {
        next_hop[dst] = -1;
        continue;
      }
      int cur = static_cast<int>(dst);
      int prev = parent[cur];
      while (prev != -1 && static_cast<size_t>(prev) != src_idx) {
        cur = prev;
        prev = parent[cur];
      }
      if (prev == -1) {
        next_hop[dst] = -1;
      } else {
        next_hop[dst] = cur;
      }
    }

    return next_hop;
  }

  void publishTable(const std::vector<NodeInfo> & nodes,
                    const std::vector<NodeClass> & node_class,
                    size_t src_idx,
                    const std::vector<int> & next_hops)
  {
    uav_msgs::msg::RoutingTable msg;
    msg.node_id = nodes[src_idx].id;
    msg.stamp = this->now();
    msg.destinations.reserve(nodes.size());
    msg.next_hops.reserve(nodes.size());

    for (size_t i = 0; i < nodes.size(); ++i) {
      if (i == src_idx) {
        continue;
      }
      msg.destinations.push_back(nodes[i].id);
      if (next_hops[i] >= 0 && static_cast<size_t>(next_hops[i]) < nodes.size()) {
        if (node_class[src_idx] == NodeClass::Endpoint &&
            node_class[static_cast<size_t>(next_hops[i])] == NodeClass::Endpoint) {
          RCLCPP_FATAL(this->get_logger(),
                       "[routing] invalid endpoint hop %s -> %s via %s",
                       msg.node_id.c_str(),
                       nodes[i].id.c_str(),
                       nodes[static_cast<size_t>(next_hops[i])].id.c_str());
          throw std::runtime_error("RoutingManager produced endpoint-to-endpoint hop");
        }
        msg.next_hops.push_back(nodes[static_cast<size_t>(next_hops[i])].id);
      } else {
        msg.next_hops.push_back("");
      }
      if (log_routes_) {
        RCLCPP_INFO(this->get_logger(),
                    "[routing] %s -> %s via %s",
                    msg.node_id.c_str(),
                    nodes[i].id.c_str(),
                    msg.next_hops.back().empty() ? "UNREACHABLE" : msg.next_hops.back().c_str());
      }
    }

    routing_pub_->publish(msg);
  }

  void publishReachabilityAlerts(bool sink_reachable, bool ugv_reachable)
  {
    if (!alert_pub_) {
      return;
    }

    if (sink_reachable != sink_reachable_) {
      std_msgs::msg::String msg;
      msg.data = sink_reachable ? "SINK_REACHABLE" : "SINK_UNREACHABLE";
      alert_pub_->publish(msg);
      RCLCPP_WARN(this->get_logger(), "[routing] %s", msg.data.c_str());
      sink_reachable_ = sink_reachable;
    }

    if (ugv_reachable != ugv_reachable_) {
      std_msgs::msg::String msg;
      msg.data = ugv_reachable ? "UGV_REACHABLE" : "UGV_UNREACHABLE";
      alert_pub_->publish(msg);
      RCLCPP_WARN(this->get_logger(), "[routing] %s", msg.data.c_str());
      ugv_reachable_ = ugv_reachable;
    }
  }

  std::unordered_map<std::string, NodeInfo> nodes_;
  std::unordered_map<EdgeKey, bool, EdgeKeyHash> link_state_;

  double comm_range_m_ = 0.0;
  double hysteresis_margin_m_ = 0.0;
  double recompute_period_sec_ = 0.0;
  double status_timeout_sec_ = 0.0;
  double ch_move_threshold_m_ = 0.0;
  bool log_routes_ = true;
  bool recompute_due_ = true;
  rclcpp::Time last_recompute_time_;
  std::string sink_id_;
  std::string ugv_id_;
  bool sink_reachable_ = true;
  bool ugv_reachable_ = true;

  rclcpp::Subscription<uav_msgs::msg::UavStatus>::SharedPtr status_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr event_sub_;
  rclcpp::Publisher<uav_msgs::msg::RoutingTable>::SharedPtr routing_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr alert_pub_;
  rclcpp::TimerBase::SharedPtr recompute_timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<RoutingManagerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
