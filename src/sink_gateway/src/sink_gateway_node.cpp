#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <algorithm>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "std_msgs/msg/string.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/uav_status.hpp"
#include "uav_msgs/msg/uav_deployment.hpp"
#include "uav_msgs/msg/routing_table.hpp"

using std::placeholders::_1;
using namespace std::chrono_literals;

// Simple delimiter-based split used for STATUS_CH payloads.
static std::vector<std::string> splitString(const std::string & s, char delim)
{
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    out.push_back(item);
  }
  return out;
}

// Sink gateway terminates traffic, injects deployments, and starts mobility.
class SinkGatewayNode : public rclcpp::Node
{
public:
  SinkGatewayNode()
  : Node("sink_gateway_node"),
    msg_counter_(0)
  {
    sink_id_ =
      this->declare_parameter<std::string>("sink_id", "sink_gateway");
    ugv_id_ =
      this->declare_parameter<std::string>("ugv_id", "ugv");

    // Subscribe to planner deployments and re-encode as network messages.
    deployment_sub_ = this->create_subscription<uav_msgs::msg::UavDeployment>(
      "/coverage_planner/deployment", 10,
      std::bind(&SinkGatewayNode::deploymentCallback, this, std::placeholders::_1));

    uplink_ch_id_ =
      this->declare_parameter<std::string>("uplink_ch_id", "uav_3");

    target_uav_id_ =
      this->declare_parameter<std::string>("target_uav_id", "");

    double period =
      this->declare_parameter<double>("control_period_sec", 0.0);

    ch_timeout_sec_ = this->declare_parameter<double>("ch_timeout_sec", 15.0);
    status_period_sec_ = this->declare_parameter<double>("status_period_sec", 1.0);
    comm_radius_m_ = this->declare_parameter<double>("comm_radius_m", 400.0);
    deployment_resend_period_sec_ =
      this->declare_parameter<double>("deployment_resend_period_sec", 2.0);
    deployment_max_resends_ =
      this->declare_parameter<int>("deployment_max_resends", 0);  // 0 = unlimited
    motion_start_resend_period_sec_ =
      this->declare_parameter<double>("motion_start_resend_period_sec", 2.0);
    motion_start_max_resends_ =
      this->declare_parameter<int>("motion_start_max_resends", 0);  // 0 = unlimited

    // Network traffic destined for the sink is processed here.
    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/fanet/network_bus", 100,
      std::bind(&SinkGatewayNode::trafficCallback, this, _1));

    delivered_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/fanet/delivered", 100);

    // Control messages are injected into /fanet/network_bus_raw.
    control_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/fanet/network_bus_raw", 100);
    routing_event_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/fanet/routing_event", 10);

    status_pub_ = this->create_publisher<uav_msgs::msg::UavStatus>(
      "/fanet/status", 10);

    routing_table_sub_ = this->create_subscription<uav_msgs::msg::RoutingTable>(
      "/fanet/routing_table", 20,
      std::bind(&SinkGatewayNode::routingTableCallback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(),
                "Sink gateway started with id='%s', ugv_id='%s', uplink_ch_id='%s', "
                "target_uav_id='%s', period=%.1fs",
                sink_id_.c_str(), ugv_id_.c_str(), uplink_ch_id_.c_str(),
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

    ch_timeout_timer_ = this->create_wall_timer(
      1s, std::bind(&SinkGatewayNode::checkChTimeouts, this));

    auto status_period = std::chrono::duration<double>(status_period_sec_);
    status_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(status_period),
      std::bind(&SinkGatewayNode::publishStatus, this));

    auto resend_period = std::chrono::duration<double>(deployment_resend_period_sec_);
    deployment_resend_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(resend_period),
      std::bind(&SinkGatewayNode::checkDeploymentAcks, this));

    auto motion_resend_period = std::chrono::duration<double>(motion_start_resend_period_sec_);
    motion_start_resend_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(motion_resend_period),
      std::bind(&SinkGatewayNode::checkMotionStartAcks, this));
  }

private:
  // --------------------------------------------------------------------------
  // Receive path: messages whose *final destination* is the sink
  // --------------------------------------------------------------------------
  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    auto now = this->now();
    if (msg->dst_id != sink_id_) {
      return;
    }
    if (!msg->next_hop_id.empty() && msg->next_hop_id != sink_id_) {
      return;
    }

    publishDelivered(*msg, now);
    maybePublishAck(*msg);

    if (msg->flow_type == 1 && msg->control_type == "STATUS_CH") {
      handleStatusCh(msg);
      return;
    }

    // Handle generic ACK messages that reference DEPLOYMENT or MOTION_START
    if (msg->flow_type == 1 && msg->control_type == "ACK" && !msg->ref_msg_id.empty()) {
      const std::string & ref = msg->ref_msg_id;

      // Check if the ACK references a DEPLOYMENT message (DEP_ prefix)
      if (!all_deployed_ && ref.rfind("DEP_", 0) == 0) {
        const std::string & u = msg->src_id;
        if (expected_uavs_.count(u) > 0 && acked_uavs_.count(u) == 0) {
          acked_uavs_.insert(u);
          deployment_cache_.erase(u);
          RCLCPP_INFO(this->get_logger(),
                      "[DEP-ACK] (via generic ACK) from %s (%zu / %zu)",
                      u.c_str(), acked_uavs_.size(), expected_uavs_.size());
          if (!expected_uavs_.empty() &&
              acked_uavs_.size() == expected_uavs_.size()) {
            all_deployed_ = true;
            RCLCPP_INFO(this->get_logger(),
                        "All deployments ACKed – broadcasting MOTION_START");
            broadcastStartMobility();
          }
        }
      }

      // Check if the ACK references a MOTION_START message
      for (auto it = motion_start_cache_.begin(); it != motion_start_cache_.end(); ++it) {
        if (it->second.msg_id == ref) {
          motion_start_acked_uavs_.insert(it->first);
          motion_start_cache_.erase(it);
          RCLCPP_INFO(this->get_logger(),
                      "[MOB-ACK] from %s (%zu / %zu)",
                      msg->src_id.c_str(),
                      motion_start_acked_uavs_.size(),
                      expected_uavs_.size());
          break;
        }
      }
    }

    // Handle DEPLOYMENT_ACK control messages
    if (msg->flow_type == 1 && msg->control_type == "DEPLOYMENT_ACK") {
      const std::string & u = msg->src_id;

      if (!all_deployed_) {
        if (expected_uavs_.count(u) == 0) {
          RCLCPP_WARN(this->get_logger(),
                      "[DEP-ACK] from %s but it was not in expected set", u.c_str());
        } else {
          acked_uavs_.insert(u);
          deployment_cache_.erase(u);
          RCLCPP_INFO(this->get_logger(),
                      "[DEP-ACK] from %s (%zu / %zu)",
                      u.c_str(),
                      acked_uavs_.size(),
                      expected_uavs_.size());
        }

        if (!expected_uavs_.empty() &&
            acked_uavs_.size() == expected_uavs_.size()) {
          all_deployed_ = true;
          RCLCPP_INFO(this->get_logger(),
                      "All deployments ACKed – broadcasting MOTION_START");
          broadcastStartMobility();
        }
      }
    }

    // In any case, this message reached the sink – publish to delivered topic
    RCLCPP_INFO(this->get_logger(),
                "[SINK RX] msg_id=%s src=%s dst=%s hop=%u (delivered to gateway)",
                msg->msg_id.c_str(), msg->src_id.c_str(),
                msg->dst_id.c_str(), msg->hop_count);

  }

  // --------------------------------------------------------------------------
  // Transmit path: periodic control messages (unused in our current tests)
  // --------------------------------------------------------------------------
  // Periodic control/diagnostic message to a target UAV.
  void publishControlToUav()
  {
    if (target_uav_id_.empty()) {
      return;
    }

    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = sink_id_ + "_ctrl_" + target_uav_id_ + "_" + std::to_string(msg_counter_++);
    msg.src_id = sink_id_;
    msg.dst_id = target_uav_id_;
    msg.next_hop_id = resolveNextHop(target_uav_id_);
    if (msg.next_hop_id.empty()) {
      RCLCPP_WARN(this->get_logger(),
                  "[SINK TX] CONTROL no route to %s",
                  target_uav_id_.c_str());
      publishRoutingEvent("NO_ROUTE_CONTROL", target_uav_id_, msg.next_hop_id);
      return;
    }

    // CONTROL_ALERT
    msg.flow_type = 1;
    msg.creation_time = this->now();
    msg.hop_count = 0;

    RCLCPP_INFO(this->get_logger(),
                "[SINK TX] CONTROL msg_id=%s src=%s dst=%s next_hop=%s",
                msg.msg_id.c_str(), msg.src_id.c_str(),
                msg.dst_id.c_str(), msg.next_hop_id.c_str());

    control_pub_->publish(msg);
  }

  // --------------------------------------------------------------------------
  // Deployment from coverage planner -> encoded as TrafficMessage into network
  // --------------------------------------------------------------------------
  // Convert planner deployment messages into network control traffic.
  void deploymentCallback(const uav_msgs::msg::UavDeployment::SharedPtr msg)
  {
    // 1) If the deployment is for the sink itself, just update pose (for viz).
    if (msg->uav_id == sink_id_) {
      sink_pose_ = msg->target_pose;
      RCLCPP_INFO(this->get_logger(),
                  "Sink '%s' updated pose to (%.1f, %.1f, %.1f)",
                  sink_id_.c_str(),
                  sink_pose_.position.x,
                  sink_pose_.position.y,
                  sink_pose_.position.z);
      // No need to send a network message for the sink itself.
      return;
    }

    // During the initial deployment phase, remember that we expect an ACK
    if (!all_deployed_) {
      expected_uavs_.insert(msg->uav_id);
    }

    // 2) Build a DEPLOYMENT TrafficMessage and inject it into /fanet/network_bus_raw.
    if (!control_pub_) {
      return;
    }

    uav_msgs::msg::TrafficMessage tm;
    tm.msg_id = "DEP_" + msg->uav_id + "_" + std::to_string(msg_counter_++);
    tm.src_id = sink_id_;
    tm.dst_id = msg->uav_id;

    // Decide the first hop from the sink into the ad-hoc network.
    // We reuse uplink_ch_id_ as the "bootstrap CH" (typically uav_1).
    std::string next_hop = resolveNextHop(msg->uav_id);

    if (next_hop.empty()) {
      if (msg->uav_id == ugv_id_) {
        // UGV: send via the bootstrap CH near the UGV.
        next_hop = uplink_ch_id_;
      } else if (msg->role == 1) {
        // CH deployment: first packet always goes through the bootstrap CH.
        // If this CH *is* the bootstrap, it will just receive it directly.
        next_hop = uplink_ch_id_;
      } else if (msg->role == 0) {
        // MEMBER: first hop is its cluster head.
        next_hop = msg->ch_id;
      } else {
        // Any other role: fall back to bootstrap CH.
        next_hop = uplink_ch_id_;
      }
    }

    tm.next_hop_id = next_hop;

    // Use msg_type=3 as CONTROL_ALERT (same convention as UAV/UGV nodes).
    tm.flow_type = 1;
    tm.creation_time = this->now();
    tm.hop_count = 0;
    tm.ttl = 12;
    tm.requires_ack = true;

    tm.control_type = "DEPLOYMENT";

    // Encode the deployment info into the control payload.
    std::string safe_sink = msg->next_hop_to_sink.empty() ? "-" : msg->next_hop_to_sink;
    std::string safe_ugv  = msg->next_hop_to_ugv.empty()  ? "-" : msg->next_hop_to_ugv;

    std::ostringstream oss;
    oss << static_cast<int>(msg->role) << ","
        << msg->cluster_id << ","
        << msg->ch_id << ","
        << msg->target_pose.position.x << ","
        << msg->target_pose.position.y << ","
        << msg->target_pose.position.z << ","
        << safe_sink << ","
        << safe_ugv;
    tm.payload = oss.str();

    sendDeployment(tm, "initial");

    deployment_cache_[msg->uav_id] = DeploymentRecord{
      tm.next_hop_id,
      tm.payload,
      this->now(),
      0
    };
  }

  // --------------------------------------------------------------------------
  // Broadcast MOTION_START when all DEPLOYMENT_ACKs arrived
  // --------------------------------------------------------------------------
  // Send motion start signal after all deployments are acknowledged.
  void broadcastStartMobility()
  {
    if (!control_pub_) {
      RCLCPP_ERROR(this->get_logger(),
                   "Cannot broadcast MOTION_START: control_pub_ is null");
      return;
    }

    motion_start_acked_uavs_.clear();
    motion_start_cache_.clear();

    for (const auto & u : expected_uavs_) {
      uav_msgs::msg::TrafficMessage msg;
      msg.msg_id = "MOTION_START_" + u + "_" +
                   std::to_string(start_mobility_seq_++);
      msg.src_id = sink_id_;
      msg.dst_id = u;
      msg.next_hop_id = resolveNextHop(u);
      if (msg.next_hop_id.empty()) {
        msg.next_hop_id = uplink_ch_id_;
        RCLCPP_WARN(this->get_logger(),
                    "[MOB-START] no route to %s, falling back to uplink CH %s",
                    u.c_str(), uplink_ch_id_.c_str());
        publishRoutingEvent("NO_ROUTE_MOTION_START", u, msg.next_hop_id);
      }

      msg.flow_type = 1;  // CONTROL_ALERT
      msg.creation_time = this->now();
      msg.hop_count = 0;
      msg.ttl = 12;
      msg.requires_ack = true;

      msg.control_type = "MOTION_START";
      msg.payload = "";

      control_pub_->publish(msg);
      motion_start_cache_[u] = MotionStartRecord{
        msg.msg_id,
        msg.next_hop_id,
        this->now(),
        0
      };
      RCLCPP_INFO(this->get_logger(),
                  "[MOB-START] sent MOTION_START to %s via %s (requires_ack=true)",
                  u.c_str(), msg.next_hop_id.c_str());
    }

    // Broadcast a routing-independent MOTION_START so that every UAV
    // unblocks even when the per-UAV routed messages are delayed by
    // incomplete routing tables or fault-injector drops.
    {
      uav_msgs::msg::TrafficMessage bcast;
      bcast.msg_id = "MOTION_START_broadcast_" +
                     std::to_string(start_mobility_seq_++);
      bcast.src_id = sink_id_;
      bcast.dst_id = "broadcast";
      bcast.flow_type = 1;
      bcast.creation_time = this->now();
      bcast.hop_count = 0;
      bcast.ttl = 1;
      bcast.requires_ack = false;
      bcast.control_type = "MOTION_START";
      bcast.payload = "";
      control_pub_->publish(bcast);
      RCLCPP_INFO(this->get_logger(),
                  "[MOB-START] sent broadcast MOTION_START (routing-independent)");
    }
  }

  // Parse STATUS_CH updates from CHs to track liveness.
  void handleStatusCh(const uav_msgs::msg::TrafficMessage::SharedPtr & msg)
  {
    auto parts = splitString(msg->payload, ',');

    ChStatus & s = ch_status_table_[msg->src_id];
    s.id = msg->src_id;
    s.last_update_time = this->now().seconds();

    if (parts.size() >= 4) {
      s.x = std::stod(parts[0]);
      s.y = std::stod(parts[1]);
      s.battery = std::stod(parts[2]);
      s.state = parts[3];
    }

    active_ch_ids_.insert(s.id);

    RCLCPP_INFO(this->get_logger(),
                "[STATUS_CH] from %s pos=(%.1f, %.1f) batt=%.1f state=%s", s.id.c_str(),
                s.x, s.y, s.battery, s.state.c_str());
  }

  void routingTableCallback(const uav_msgs::msg::RoutingTable::SharedPtr msg)
  {
    if (msg->node_id != sink_id_) {
      return;
    }

    routing_table_.clear();
    const size_t count = std::min(msg->destinations.size(), msg->next_hops.size());
    for (size_t i = 0; i < count; ++i) {
      if (!msg->destinations[i].empty() && !msg->next_hops[i].empty()) {
        routing_table_[msg->destinations[i]] = msg->next_hops[i];
      }
    }

    RCLCPP_INFO(this->get_logger(),
                "Sink routing table updated (%zu entries)",
                routing_table_.size());
  }

  std::string resolveNextHop(const std::string & dst) const
  {
    auto it = routing_table_.find(dst);
    if (it != routing_table_.end()) {
      return it->second;
    }
    return "";
  }

  void publishRoutingEvent(const std::string & reason,
                           const std::string & dst_id,
                           const std::string & next_hop) const
  {
    if (!routing_event_pub_) {
      return;
    }
    std_msgs::msg::String msg;
    std::ostringstream oss;
    oss << "node=" << sink_id_
        << " dst=" << dst_id
        << " next_hop=" << (next_hop.empty() ? "UNREACHABLE" : next_hop)
        << " reason=" << reason;
    msg.data = oss.str();
    routing_event_pub_->publish(msg);
  }

  void handleChDead(const std::string & ch_id)
  {
    RCLCPP_WARN(this->get_logger(),
                "CH %s timed out at sink – triggering coverage recompute", ch_id.c_str());
    triggerCoverageRecompute();
  }

  // Periodically verify CH status updates have not timed out.
  void checkChTimeouts()
  {
    const double now = this->now().seconds();
    for (auto it = active_ch_ids_.begin(); it != active_ch_ids_.end(); ) {
      const auto & ch_id = *it;
      auto st_it = ch_status_table_.find(ch_id);
      if (st_it == ch_status_table_.end()) {
        it = active_ch_ids_.erase(it);
        continue;
      }

      if (now - st_it->second.last_update_time > ch_timeout_sec_) {
        handleChDead(ch_id);
        it = active_ch_ids_.erase(it);
      } else {
        ++it;
      }
    }
  }

  void triggerCoverageRecompute()
  {
    RCLCPP_INFO(this->get_logger(),
                "[COVERAGE] requesting planner to recompute layouts after CH loss");
  }

  void checkDeploymentAcks()
  {
    if (all_deployed_ || expected_uavs_.empty() || !control_pub_) {
      return;
    }

    const auto now = this->now();
    for (const auto & uav_id : expected_uavs_) {
      if (acked_uavs_.count(uav_id) > 0) {
        continue;
      }

      auto it = deployment_cache_.find(uav_id);
      if (it == deployment_cache_.end()) {
        RCLCPP_WARN(this->get_logger(),
                    "[DEP-RESEND] missing cached deployment for %s",
                    uav_id.c_str());
        continue;
      }

      auto elapsed = now - it->second.last_sent;
      if (elapsed.seconds() < deployment_resend_period_sec_) {
        continue;
      }

      if (deployment_max_resends_ > 0 &&
          it->second.resend_count >= deployment_max_resends_) {
        RCLCPP_ERROR(this->get_logger(),
                     "[DEP-RESEND] giving up on DEPLOYMENT to %s after %d resends",
                     uav_id.c_str(), it->second.resend_count);
        continue;
      }

      // Try to update next_hop from current routing table
      std::string next_hop = resolveNextHop(uav_id);
      if (!next_hop.empty()) {
        it->second.next_hop_id = next_hop;
      }

      uav_msgs::msg::TrafficMessage tm;
      tm.msg_id = "DEP_" + uav_id + "_" + std::to_string(msg_counter_++);
      tm.src_id = sink_id_;
      tm.dst_id = uav_id;
      tm.next_hop_id = it->second.next_hop_id;
      tm.flow_type = 1;
      tm.creation_time = now;
      tm.hop_count = 0;
      tm.ttl = 12;
      tm.requires_ack = true;
      tm.control_type = "DEPLOYMENT";
      tm.payload = it->second.payload;

      sendDeployment(tm, "resend");
      it->second.last_sent = now;
      it->second.resend_count++;
    }
  }

  void checkMotionStartAcks()
  {
    if (!all_deployed_ || motion_start_cache_.empty() || !control_pub_) {
      return;
    }

    const auto now = this->now();
    for (auto & [uav_id, record] : motion_start_cache_) {
      if (motion_start_acked_uavs_.count(uav_id) > 0) {
        continue;
      }

      auto elapsed = now - record.last_sent;
      if (elapsed.seconds() < motion_start_resend_period_sec_) {
        continue;
      }

      if (motion_start_max_resends_ > 0 &&
          record.resend_count >= motion_start_max_resends_) {
        RCLCPP_ERROR(this->get_logger(),
                     "[MOB-START] giving up on MOTION_START to %s after %d resends",
                     uav_id.c_str(), record.resend_count);
        continue;
      }

      uav_msgs::msg::TrafficMessage msg;
      msg.msg_id = record.msg_id;  // Keep same msg_id for dedup
      msg.src_id = sink_id_;
      msg.dst_id = uav_id;
      msg.next_hop_id = resolveNextHop(uav_id);
      if (msg.next_hop_id.empty()) {
        msg.next_hop_id = record.next_hop_id;
      }
      msg.flow_type = 1;
      msg.creation_time = now;
      msg.hop_count = 0;
      msg.ttl = 12;
      msg.requires_ack = true;
      msg.control_type = "MOTION_START";
      msg.payload = "";

      control_pub_->publish(msg);
      record.last_sent = now;
      record.resend_count++;

      RCLCPP_WARN(this->get_logger(),
                  "[MOB-START] resend #%d MOTION_START to %s via %s",
                  record.resend_count, uav_id.c_str(), msg.next_hop_id.c_str());
    }
  }

  void sendDeployment(const uav_msgs::msg::TrafficMessage & msg, const std::string & reason)
  {
    if (!control_pub_) {
      return;
    }

    control_pub_->publish(msg);

    RCLCPP_INFO(this->get_logger(),
                "[DEP-TX] (%s) sink sending deployment to=%s via first_hop=%s payload=\"%s\"",
                reason.c_str(),
                msg.dst_id.c_str(),
                msg.next_hop_id.c_str(),
                msg.payload.c_str());
  }

  // Members
  std::string sink_id_;
  std::string ugv_id_;
  std::string uplink_ch_id_;
  std::string target_uav_id_;
  uint64_t msg_counter_;
  geometry_msgs::msg::Pose sink_pose_;

  struct ChStatus
  {
    std::string id;
    double last_update_time = 0.0;
    double x = 0.0;
    double y = 0.0;
    double battery = 0.0;
    std::string state;
  };

  std::unordered_map<std::string, ChStatus> ch_status_table_;
  std::unordered_set<std::string> active_ch_ids_;
  std::unordered_map<std::string, std::string> routing_table_;
  double ch_timeout_sec_ = 0.0;
  double status_period_sec_ = 1.0;
  double comm_radius_m_ = 0.0;
  rclcpp::TimerBase::SharedPtr ch_timeout_timer_;
  rclcpp::TimerBase::SharedPtr status_timer_;

  // Deployment tracking
  struct DeploymentRecord
  {
    std::string next_hop_id;
    std::string payload;
    rclcpp::Time last_sent;
    int resend_count = 0;
  };

  // MOTION_START tracking
  struct MotionStartRecord
  {
    std::string msg_id;
    std::string next_hop_id;
    rclcpp::Time last_sent;
    int resend_count = 0;
  };

  std::unordered_set<std::string> expected_uavs_;
  std::unordered_set<std::string> acked_uavs_;
  std::unordered_map<std::string, DeploymentRecord> deployment_cache_;
  bool all_deployed_ = false;
  uint64_t start_mobility_seq_ = 0;
  double deployment_resend_period_sec_ = 0.0;
  int deployment_max_resends_ = 0;
  rclcpp::TimerBase::SharedPtr deployment_resend_timer_;

  // MOTION_START ACK tracking
  std::unordered_set<std::string> motion_start_acked_uavs_;
  std::unordered_map<std::string, MotionStartRecord> motion_start_cache_;
  double motion_start_resend_period_sec_ = 2.0;
  int motion_start_max_resends_ = 0;
  rclcpp::TimerBase::SharedPtr motion_start_resend_timer_;

  rclcpp::Subscription<uav_msgs::msg::UavDeployment>::SharedPtr deployment_sub_;
  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Subscription<uav_msgs::msg::RoutingTable>::SharedPtr routing_table_sub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr    delivered_pub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr    control_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr            routing_event_pub_;
  rclcpp::Publisher<uav_msgs::msg::UavStatus>::SharedPtr         status_pub_;
  rclcpp::TimerBase::SharedPtr                                   control_timer_;

  void maybePublishAck(const uav_msgs::msg::TrafficMessage & msg)
  {
    if (!msg.requires_ack || !control_pub_) {
      return;
    }

    uav_msgs::msg::TrafficMessage ack;
    ack.msg_id = sink_id_ + "_ACK_" + msg.msg_id;
    ack.src_id = sink_id_;
    ack.dst_id = msg.src_id;
    ack.ref_msg_id = msg.msg_id;
    ack.flow_type = 1;
    ack.control_type = "ACK";
    ack.payload = msg.msg_id;
    ack.creation_time = this->now();
    ack.hop_count = 0;
    ack.ttl = 4;
    ack.requires_ack = false;
    ack.next_hop_id = resolveNextHop(ack.dst_id);

    if (!ack.next_hop_id.empty()) {
      control_pub_->publish(ack);
    } else {
      RCLCPP_WARN(this->get_logger(),
                  "[ACK] Sink could not ACK msg_id=%s (no next hop)",
                  msg.msg_id.c_str());
      publishRoutingEvent("NO_ROUTE_ACK", ack.dst_id, ack.next_hop_id);
    }
  }

  void publishDelivered(const uav_msgs::msg::TrafficMessage & msg, const rclcpp::Time & now)
  {
    if (!delivered_pub_) {
      return;
    }
    uav_msgs::msg::TrafficMessage delivered = msg;
    delivered.next_hop_id.clear();
    delivered_pub_->publish(delivered);
  }

  void publishStatus()
  {
    if (!status_pub_) {
      return;
    }

    auto now = this->now();
    uav_msgs::msg::UavStatus status;
    status.uav_id = sink_id_;
    status.role = 2;
    status.cluster_id = "sink";
    status.battery_level = 100.0f;
    status.battery_capacity = 0.0f;
    status.pose = sink_pose_;
    status.service_radius = static_cast<float>(comm_radius_m_);
    status.connected_users = 0;
    status.traffic_load = 0.0f;
    status.packet_loss_estimate = 0.0f;
    status.energy_consumption_rate = 0.0f;
    status.charging_state = 0;
    status.intent_to_leave = false;
    status.eta_to_leave_sec = 0.0f;
    status.comm_radius_m = static_cast<float>(comm_radius_m_);
    status.stamp = now;
    status.backbone_active = true;

    status_pub_->publish(status);
  }
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<SinkGatewayNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
