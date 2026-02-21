#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "std_msgs/msg/string.hpp"
#include "uav_msgs/msg/task_point_array.hpp"
#include "uav_msgs/msg/uav_status.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/failure_event.hpp"
#include "uav_msgs/msg/cluster_info.hpp"

using namespace std::chrono_literals;

namespace {
struct ChState
{
  std::string id;
  geometry_msgs::msg::Pose pose;
  double battery_percent = 0.0;
  double battery_energy = 0.0;
  rclcpp::Time last_seen;
  bool alive = false;
};

struct MemberState
{
  std::string id;
  geometry_msgs::msg::Pose pose;
  double battery_percent = 0.0;
  bool alive = false;
  std::string ch_id;
};

struct TaskPoint
{
  std::string id;
  geometry_msgs::msg::Point position;
};

double distance2d(const geometry_msgs::msg::Point & a,
                  const geometry_msgs::msg::Point & b)
{
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}
}  // namespace

class RecoveryManagerNode : public rclcpp::Node
{
public:
  RecoveryManagerNode()
  : Node("recovery_manager_node")
  {
    sink_id_ = this->declare_parameter<std::string>("sink_id", "sink_gateway");
    ugv_id_ = this->declare_parameter<std::string>("ugv_id", "ugv");
    control_src_id_ = this->declare_parameter<std::string>("control_src_id", sink_id_);
    comm_range_m_ = this->declare_parameter<double>("comm_range_m", 400.0);
    status_timeout_sec_ = this->declare_parameter<double>("status_timeout_sec", 5.0);
    heartbeat_timeout_sec_ = this->declare_parameter<double>("heartbeat_timeout_sec", 4.0);
    recovery_cooldown_sec_ = this->declare_parameter<double>("recovery_cooldown_sec", 2.0);
    movement_delta_m_ = this->declare_parameter<double>("movement_delta_m", 5.0);
    control_ttl_ = static_cast<uint32_t>(this->declare_parameter<int>("control_ttl", 12));
    ack_retry_period_sec_ = this->declare_parameter<double>("ack_retry_period_sec", 0.5);
    max_ack_retries_ = this->declare_parameter<int>("max_ack_retries", 5);
    recovery_lock_duration_sec_ = this->declare_parameter<double>("recovery_lock_duration_sec", 10.0);

    status_sub_ = this->create_subscription<uav_msgs::msg::UavStatus>(
      "/fanet/status", 200,
      std::bind(&RecoveryManagerNode::statusCallback, this, std::placeholders::_1));

    heartbeat_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/fanet/network_bus", 200,
      std::bind(&RecoveryManagerNode::heartbeatCallback, this, std::placeholders::_1));

    routing_alert_sub_ = this->create_subscription<std_msgs::msg::String>(
      "/routing_manager/alerts", 20,
      std::bind(&RecoveryManagerNode::routingAlertCallback, this, std::placeholders::_1));

    failure_sub_ = this->create_subscription<uav_msgs::msg::FailureEvent>(
      "/uav_fleet/failure_events", 50,
      std::bind(&RecoveryManagerNode::failureCallback, this, std::placeholders::_1));

    task_sub_ = this->create_subscription<uav_msgs::msg::TaskPointArray>(
      "/coverage_planner/task_points", rclcpp::QoS(1).transient_local(),
      std::bind(&RecoveryManagerNode::taskCallback, this, std::placeholders::_1));

    cluster_sub_ = this->create_subscription<uav_msgs::msg::ClusterInfo>(
      "/ch_manager/cluster_info", 20,
      std::bind(&RecoveryManagerNode::clusterInfoCallback, this, std::placeholders::_1));

    traffic_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/fanet/network_bus_raw", 50);

    ack_retry_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::duration<double>(ack_retry_period_sec_)),
      std::bind(&RecoveryManagerNode::resendPendingAcks, this));

    watchdog_timer_ = this->create_wall_timer(
      500ms, std::bind(&RecoveryManagerNode::watchdog, this));

    RCLCPP_INFO(this->get_logger(), "Recovery manager started.");
  }

private:
  struct PendingAck
  {
    uav_msgs::msg::TrafficMessage msg;
    rclcpp::Time last_send_time;
    int attempts = 0;
    int max_retries = 0;
    double retry_interval_sec = 1.0;
  };

  void statusCallback(const uav_msgs::msg::UavStatus::SharedPtr msg)
  {
    if (msg->uav_id == sink_id_) {
      sink_pose_ = msg->pose;
      sink_pose_known_ = true;
      return;
    }
    if (msg->uav_id == ugv_id_) {
      ugv_pose_ = msg->pose;
      ugv_pose_known_ = true;
      return;
    }

    if (msg->role == 1) {
      auto & ch = ch_states_[msg->uav_id];
      bool was_dead = !ch.alive;
      ch.id = msg->uav_id;
      ch.pose = msg->pose;
      ch.battery_percent = msg->battery_level;
      ch.battery_energy = (msg->battery_level / 100.0) * msg->battery_capacity;
      ch.last_seen = msg->stamp;
      ch.alive = msg->battery_level > 0.0f;
      // CH came back from the dead (respawned) — trigger recovery so the
      // system can redeploy it and rebalance members/tasks.
      if (was_dead && ch.alive) {
        recovery_requested_ = true;
        RCLCPP_WARN(this->get_logger(),
                    "[RECOVERY][DECISION] CH %s returned alive (respawned, batt=%.1f%%) "
                    "-> recovery_requested",
                    msg->uav_id.c_str(), msg->battery_level);
      }
    } else {
      auto & member = member_states_[msg->uav_id];
      member.id = msg->uav_id;
      member.pose = msg->pose;
      member.battery_percent = msg->battery_level;
      member.alive = msg->battery_level > 0.0f;
    }
  }

  void heartbeatCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    if (msg->flow_type == 1 && msg->control_type == "HEARTBEAT") {
      last_heartbeat_[msg->src_id] = msg->creation_time;
    }
    if (msg->flow_type == 1 && msg->control_type == "ACK") {
      const std::string acked_id = extractAckedMsgId(*msg);
      if (!acked_id.empty()) {
        auto it = pending_acks_.find(acked_id);
        if (it != pending_acks_.end()) {
          RCLCPP_INFO(this->get_logger(),
                      "[RECOVERY] ACK received msg_id=%s type=%s dst=%s attempts=%d",
                      acked_id.c_str(),
                      it->second.msg.control_type.c_str(),
                      it->second.msg.dst_id.c_str(),
                      it->second.attempts);
          pending_acks_.erase(it);
        }
      }
    }
  }

  void routingAlertCallback(const std_msgs::msg::String::SharedPtr msg)
  {
    if (msg->data == "SINK_UNREACHABLE") {
      sink_unreachable_ = true;
    } else if (msg->data == "SINK_REACHABLE") {
      sink_unreachable_ = false;
    } else if (msg->data == "UGV_UNREACHABLE") {
      ugv_unreachable_ = true;
    } else if (msg->data == "UGV_REACHABLE") {
      ugv_unreachable_ = false;
    }
  }

  void failureCallback(const uav_msgs::msg::FailureEvent::SharedPtr msg)
  {
    if (msg->failure_type != 1) {
      return;
    }
    if (ch_states_.count(msg->uav_id) > 0) {
      ch_states_[msg->uav_id].alive = false;
      recovery_requested_ = true;
      RCLCPP_WARN(this->get_logger(),
                  "[RECOVERY][DECISION] CH %s BATTERY_DEAD -> recovery_requested",
                  msg->uav_id.c_str());
    }
    if (member_states_.count(msg->uav_id) > 0) {
      member_states_[msg->uav_id].alive = false;
      // Member death: trigger recovery to redistribute its tasks
      recovery_requested_ = true;
      RCLCPP_WARN(this->get_logger(),
                  "[RECOVERY][DECISION] Member %s BATTERY_DEAD -> recovery_requested "
                  "(task redistribution needed)",
                  msg->uav_id.c_str());
    }
  }

  void taskCallback(const uav_msgs::msg::TaskPointArray::SharedPtr msg)
  {
    task_points_.clear();
    task_points_.reserve(msg->tasks.size());
    for (const auto & task : msg->tasks) {
      TaskPoint tp;
      tp.id = task.id;
      tp.position = task.position;
      task_points_.push_back(tp);
    }
  }

  void clusterInfoCallback(const uav_msgs::msg::ClusterInfo::SharedPtr msg)
  {
    if (!msg->ch_id.empty()) {
      ch_cluster_ids_[msg->ch_id] = msg->cluster_id;
    }
    const auto now = this->now();
    for (const auto & member_id : msg->member_ids) {
      // Skip members recently reassigned by recovery to avoid ch_manager
      // overriding recovery decisions with stale static membership data.
      auto lock_it = recovery_assigned_.find(member_id);
      if (lock_it != recovery_assigned_.end()) {
        double elapsed = (now - lock_it->second).seconds();
        if (elapsed < recovery_lock_duration_sec_) {
          RCLCPP_DEBUG(this->get_logger(),
                       "[RECOVERY][LOCK] ignoring ch_manager update for %s "
                       "(recovery-assigned %.1fs ago, lock=%.1fs)",
                       member_id.c_str(), elapsed, recovery_lock_duration_sec_);
          continue;
        }
        recovery_assigned_.erase(lock_it);
      }
      auto & member = member_states_[member_id];
      member.id = member_id;
      member.ch_id = msg->ch_id;
    }
  }

  void watchdog()
  {
    const auto now = this->now();
    bool ch_timeout = false;
    for (auto & kv : ch_states_) {
      auto & ch = kv.second;
      if (!ch.alive) {
        continue;
      }
      if ((now - ch.last_seen).seconds() > status_timeout_sec_) {
        ch.alive = false;
        ch_timeout = true;
      }
    }

    for (const auto & kv : ch_states_) {
      const auto & ch = kv.second;
      if (!ch.alive) {
        continue;
      }
      auto hb_it = last_heartbeat_.find(ch.id);
      if (hb_it != last_heartbeat_.end()) {
        if ((now - hb_it->second).seconds() > heartbeat_timeout_sec_) {
          ch_timeout = true;
          ch_states_[ch.id].alive = false;
        }
      }
    }

    if (!ch_timeout && !sink_unreachable_ && !ugv_unreachable_ && !recovery_requested_) {
      return;
    }

    RCLCPP_WARN(this->get_logger(),
                "[RECOVERY][DECISION] watchdog trigger: ch_timeout=%d sink_unreachable=%d "
                "ugv_unreachable=%d recovery_requested=%d",
                ch_timeout, sink_unreachable_, ugv_unreachable_, recovery_requested_);

    if ((now - last_recovery_time_).seconds() < recovery_cooldown_sec_) {
      RCLCPP_INFO(this->get_logger(),
                  "[RECOVERY][DECISION] cooldown active (%.1fs < %.1fs), deferring",
                  (now - last_recovery_time_).seconds(), recovery_cooldown_sec_);
      return;
    }

    last_recovery_time_ = now;
    recovery_requested_ = false;
    runRecovery();
  }

  void runRecovery()
  {
    const auto now = this->now();
    epoch_++;

    std::vector<ChState> alive_chs;
    for (const auto & kv : ch_states_) {
      if (kv.second.alive) {
        alive_chs.push_back(kv.second);
      }
    }

    {
      size_t alive_members = 0;
      for (const auto & kv : member_states_) {
        if (kv.second.alive) {
          ++alive_members;
        }
      }
      RCLCPP_WARN(this->get_logger(),
                  "[RECOVERY][DECISION] epoch=%d alive_chs=%zu alive_members=%zu "
                  "sink_unreachable=%d ugv_unreachable=%d",
                  epoch_, alive_chs.size(), alive_members,
                  sink_unreachable_, ugv_unreachable_);
    }

    publishRecoveryStart(epoch_);

    if (alive_chs.empty()) {
      RCLCPP_WARN(this->get_logger(), "[RECOVERY] no alive CHs. Triggering member fallback.");
      publishFallbackForMembers();
      publishRecoveryDone(epoch_);
      return;
    }

    std::sort(alive_chs.begin(), alive_chs.end(),
              [](const ChState & a, const ChState & b) { return a.id < b.id; });

    const std::string leader_id = electLeader(alive_chs);
    RCLCPP_INFO(this->get_logger(), "[RECOVERY] leader elected: %s", leader_id.c_str());
    RCLCPP_INFO(this->get_logger(), "[RECOVERY] cluster count K=%zu", alive_chs.size());

    recomputeMembership(alive_chs);
    recomputeTasks(alive_chs);
    planCoverageRedeployments(alive_chs);

    const bool sink_degraded = sink_unreachable_ || isEndpointDisconnected(alive_chs, sink_pose_known_, sink_pose_);
    const bool ugv_degraded = ugv_unreachable_ || isEndpointDisconnected(alive_chs, ugv_pose_known_, ugv_pose_);
    if (sink_degraded || ugv_degraded) {
      planRedeployments(alive_chs, sink_degraded, ugv_degraded);
    }

    publishRecoveryDone(epoch_);
  }

  std::string electLeader(const std::vector<ChState> & alive_chs) const
  {
    std::unordered_map<std::string, int> degrees;
    for (const auto & ch : alive_chs) {
      degrees[ch.id] = 0;
    }
    for (size_t i = 0; i < alive_chs.size(); ++i) {
      for (size_t j = i + 1; j < alive_chs.size(); ++j) {
        double dist = distance2d(alive_chs[i].pose.position, alive_chs[j].pose.position);
        if (dist <= comm_range_m_) {
          degrees[alive_chs[i].id] += 1;
          degrees[alive_chs[j].id] += 1;
        }
      }
    }
    std::string best_id;
    double best_score = -1.0;
    for (const auto & ch : alive_chs) {
      double score = static_cast<double>(degrees[ch.id]) * (ch.battery_percent / 100.0);
      if (score > best_score) {
        best_score = score;
        best_id = ch.id;
      } else if (std::abs(score - best_score) < 1e-9 && ch.id < best_id) {
        best_id = ch.id;
      }
    }
    RCLCPP_INFO(this->get_logger(), "[RECOVERY] leader election winner=%s score=%.3f",
                best_id.c_str(), best_score);
    return best_id;
  }

  void recomputeMembership(const std::vector<ChState> & alive_chs)
  {
    for (auto & kv : member_states_) {
      auto & member = kv.second;
      if (!member.alive) {
        continue;
      }
      std::string best_ch;
      double best_dist = std::numeric_limits<double>::infinity();
      double best_batt = -1.0;
      for (const auto & ch : alive_chs) {
        double dist = distance2d(member.pose.position, ch.pose.position);
        if (dist < best_dist - 1e-6) {
          best_dist = dist;
          best_batt = ch.battery_percent;
          best_ch = ch.id;
        } else if (std::abs(dist - best_dist) < 1e-6) {
          if (ch.battery_percent > best_batt) {
            best_batt = ch.battery_percent;
            best_ch = ch.id;
          }
        }
      }
      if (!best_ch.empty() && member.ch_id != best_ch) {
        member.ch_id = best_ch;
        recovery_assigned_[member.id] = this->now();
        publishClusterReassign(member.id, best_ch);
      }
    }
  }

  void recomputeTasks(const std::vector<ChState> & alive_chs)
  {
    const auto ch_tasks = buildTaskBuckets(alive_chs);

    for (const auto & ch : alive_chs) {
      std::vector<std::string> member_ids = membersForCh(ch.id);
      if (member_ids.empty()) {
        continue;
      }
      auto it_tasks = ch_tasks.find(ch.id);
      if (it_tasks == ch_tasks.end() || it_tasks->second.empty()) {
        continue;
      }
      const auto & tasks = it_tasks->second;
      std::sort(member_ids.begin(), member_ids.end());
      std::unordered_map<std::string, std::vector<TaskPoint>> assignments;
      for (const auto & task : tasks) {
        double best_dist = std::numeric_limits<double>::infinity();
        std::string best_member;
        for (const auto & member_id : member_ids) {
          auto it_member = member_states_.find(member_id);
          if (it_member == member_states_.end()) {
            continue;
          }
          double dist = distance2d(task.position, it_member->second.pose.position);
          if (dist < best_dist - 1e-6 ||
              (std::abs(dist - best_dist) < 1e-6 && member_id < best_member)) {
            best_dist = dist;
            best_member = member_id;
          }
        }
        if (!best_member.empty()) {
          assignments[best_member].push_back(task);
        }
      }
      for (const auto & kv : assignments) {
        publishTaskAssign(kv.first, kv.second);
      }
    }
  }

  std::unordered_map<std::string, std::vector<TaskPoint>> buildTaskBuckets(
    const std::vector<ChState> & alive_chs) const
  {
    std::unordered_map<std::string, std::vector<TaskPoint>> ch_tasks;
    for (const auto & tp : task_points_) {
      double best_dist = std::numeric_limits<double>::infinity();
      std::string best_ch;
      double best_batt = -1.0;
      for (const auto & ch : alive_chs) {
        double dist = distance2d(tp.position, ch.pose.position);
        if (dist < best_dist - 1e-6) {
          best_dist = dist;
          best_batt = ch.battery_percent;
          best_ch = ch.id;
        } else if (std::abs(dist - best_dist) < 1e-6) {
          if (ch.battery_percent > best_batt) {
            best_batt = ch.battery_percent;
            best_ch = ch.id;
          }
        }
      }
      if (!best_ch.empty()) {
        ch_tasks[best_ch].push_back(tp);
      }
    }
    return ch_tasks;
  }

  void planCoverageRedeployments(const std::vector<ChState> & alive_chs)
  {
    if (alive_chs.empty() || task_points_.empty()) {
      return;
    }

    const auto ch_tasks = buildTaskBuckets(alive_chs);
    std::unordered_map<std::string, geometry_msgs::msg::Pose> overrides;
    for (const auto & ch : alive_chs) {
      auto it_tasks = ch_tasks.find(ch.id);
      if (it_tasks == ch_tasks.end() || it_tasks->second.empty()) {
        continue;
      }

      geometry_msgs::msg::Pose target = ch.pose;
      double sum_x = 0.0;
      double sum_y = 0.0;
      for (const auto & tp : it_tasks->second) {
        sum_x += tp.position.x;
        sum_y += tp.position.y;
      }
      target.position.x = sum_x / static_cast<double>(it_tasks->second.size());
      target.position.y = sum_y / static_cast<double>(it_tasks->second.size());

      if (!hasBackboneNeighbor(ch.id, target, alive_chs, overrides)) {
        continue;
      }
      overrides[ch.id] = target;
      publishNewDeployment(ch.id, target);
    }
  }

  bool isEndpointDisconnected(const std::vector<ChState> & alive_chs,
                              bool endpoint_pose_known,
                              const geometry_msgs::msg::Pose & endpoint_pose) const
  {
    if (!endpoint_pose_known || alive_chs.empty()) {
      return false;
    }
    for (const auto & ch : alive_chs) {
      if (distance2d(ch.pose.position, endpoint_pose.position) <= comm_range_m_) {
        return false;
      }
    }
    return true;
  }

  void planRedeployments(const std::vector<ChState> & alive_chs,
                         bool sink_degraded,
                         bool ugv_degraded)
  {
    if (!sink_pose_known_ && sink_degraded) {
      RCLCPP_WARN(this->get_logger(), "[RECOVERY] sink pose unknown; cannot bridge.");
    }
    if (!ugv_pose_known_ && ugv_degraded) {
      RCLCPP_WARN(this->get_logger(), "[RECOVERY] UGV pose unknown; cannot bridge.");
    }

    std::vector<ChState> candidates = alive_chs;
    if (candidates.empty()) {
      return;
    }

    if (sink_degraded && ugv_degraded) {
      double best_cost = std::numeric_limits<double>::infinity();
      std::string best_one;
      geometry_msgs::msg::Pose best_pose;
      if (sink_pose_known_ && ugv_pose_known_) {
        for (const auto & ch : candidates) {
          geometry_msgs::msg::Pose pose;
          pose.position.x = 0.5 * (sink_pose_.position.x + ugv_pose_.position.x);
          pose.position.y = 0.5 * (sink_pose_.position.y + ugv_pose_.position.y);
          pose.position.z = ch.pose.position.z;
          double dist = distance2d(ch.pose.position, pose.position);
          if (dist < best_cost && hasBackboneNeighbor(ch.id, pose, alive_chs, {})) {
            best_cost = dist;
            best_one = ch.id;
            best_pose = pose;
          }
        }
      }
      if (!best_one.empty()) {
        publishNewDeployment(best_one, best_pose);
        return;
      }

      if (sink_pose_known_ && ugv_pose_known_) {
        double best_sum = std::numeric_limits<double>::infinity();
        std::pair<std::string, std::string> best_pair;
        geometry_msgs::msg::Pose sink_pose = sink_pose_;
        geometry_msgs::msg::Pose ugv_pose = ugv_pose_;
        for (size_t i = 0; i < candidates.size(); ++i) {
          for (size_t j = i + 1; j < candidates.size(); ++j) {
            double dist_i = distance2d(candidates[i].pose.position, sink_pose.position);
            double dist_j = distance2d(candidates[j].pose.position, ugv_pose.position);
            double sum = dist_i + dist_j;
            std::unordered_map<std::string, geometry_msgs::msg::Pose> overrides;
            geometry_msgs::msg::Pose sink_target = sink_pose;
            sink_target.position.z = candidates[i].pose.position.z;
            geometry_msgs::msg::Pose ugv_target = ugv_pose;
            ugv_target.position.z = candidates[j].pose.position.z;
            overrides[candidates[i].id] = sink_target;
            overrides[candidates[j].id] = ugv_target;
            if (sum < best_sum &&
                hasBackboneNeighbor(candidates[i].id, sink_target, alive_chs, overrides) &&
                hasBackboneNeighbor(candidates[j].id, ugv_target, alive_chs, overrides)) {
              best_sum = sum;
              best_pair = {candidates[i].id, candidates[j].id};
            }
          }
        }
        if (!best_pair.first.empty()) {
          geometry_msgs::msg::Pose sink_target = sink_pose;
          sink_target.position.z = ch_states_[best_pair.first].pose.position.z;
          geometry_msgs::msg::Pose ugv_target = ugv_pose;
          ugv_target.position.z = ch_states_[best_pair.second].pose.position.z;
          publishNewDeployment(best_pair.first, sink_target);
          publishNewDeployment(best_pair.second, ugv_target);
        }
      }
      return;
    }

    if (sink_degraded && sink_pose_known_) {
      publishBridgeToTarget("SINK", sink_pose_, candidates, alive_chs);
    }
    if (ugv_degraded && ugv_pose_known_) {
      publishBridgeToTarget("UGV", ugv_pose_, candidates, alive_chs);
    }
  }

  void publishFallbackForMembers()
  {
    geometry_msgs::msg::Pose fallback_pose;
    std::string target_label = "NONE";
    if (ugv_pose_known_) {
      fallback_pose = ugv_pose_;
      target_label = "UGV";
    } else if (sink_pose_known_) {
      fallback_pose = sink_pose_;
      target_label = "SINK";
    }

    if (target_label == "NONE") {
      RCLCPP_WARN(this->get_logger(),
                  "[RECOVERY] fallback requested, but neither sink nor UGV pose is known.");
      return;
    }

    for (const auto & kv : member_states_) {
      const auto & member = kv.second;
      if (!member.alive) {
        continue;
      }

      uav_msgs::msg::TrafficMessage msg;
      msg.msg_id = "MEMBER_FALLBACK_" + member.id + "_" + std::to_string(msg_counter_++);
      msg.src_id = control_src_id_;
      msg.dst_id = member.id;
      msg.flow_type = 1;
      msg.control_type = "MEMBER_FALLBACK";
      msg.creation_time = this->now();
      msg.hop_count = 0;
      msg.ttl = control_ttl_;
      msg.requires_ack = true;

      std::ostringstream oss;
      oss << target_label << ","
          << fallback_pose.position.x << ","
          << fallback_pose.position.y << ","
          << fallback_pose.position.z;
      msg.payload = oss.str();
      msg.next_hop_id = member.id;
      sendWithAck(msg);
      RCLCPP_WARN(this->get_logger(),
                  "[RECOVERY] MEMBER_FALLBACK %s -> %s (%.1f, %.1f, %.1f)",
                  member.id.c_str(), target_label.c_str(),
                  fallback_pose.position.x,
                  fallback_pose.position.y,
                  fallback_pose.position.z);
    }
  }

  void publishBridgeToTarget(const std::string & label,
                             const geometry_msgs::msg::Pose & target_pose,
                             const std::vector<ChState> & candidates,
                             const std::vector<ChState> & alive_chs)
  {
    double best_dist = std::numeric_limits<double>::infinity();
    std::string best_ch;
    geometry_msgs::msg::Pose best_pose;
    for (const auto & ch : candidates) {
      geometry_msgs::msg::Pose pose = target_pose;
      pose.position.z = ch.pose.position.z;
      double dist = distance2d(ch.pose.position, pose.position);
      if (dist < best_dist && hasBackboneNeighbor(ch.id, pose, alive_chs, {})) {
        best_dist = dist;
        best_ch = ch.id;
        best_pose = pose;
      }
    }
    if (!best_ch.empty()) {
      RCLCPP_WARN(this->get_logger(), "[RECOVERY] %s bridge: moving %s (%.1f m)",
                  label.c_str(), best_ch.c_str(), best_dist);
      publishNewDeployment(best_ch, best_pose);
    }
  }

  bool hasBackboneNeighbor(
    const std::string & ch_id,
    const geometry_msgs::msg::Pose & pose,
    const std::vector<ChState> & alive_chs,
    const std::unordered_map<std::string, geometry_msgs::msg::Pose> & overrides) const
  {
    if (alive_chs.size() <= 1) {
      return true;
    }
    for (const auto & other : alive_chs) {
      if (other.id == ch_id) {
        continue;
      }
      auto it = overrides.find(other.id);
      const auto & other_pose = (it != overrides.end()) ? it->second : other.pose;
      if (distance2d(pose.position, other_pose.position) <= comm_range_m_) {
        return true;
      }
    }
    return false;
  }

  std::vector<std::string> membersForCh(const std::string & ch_id) const
  {
    std::vector<std::string> out;
    for (const auto & kv : member_states_) {
      if (!kv.second.alive) {
        continue;
      }
      if (kv.second.ch_id == ch_id) {
        out.push_back(kv.second.id);
      }
    }
    return out;
  }

  void publishRecoveryStart(int epoch)
  {
    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = "RECOVERY_START_" + std::to_string(epoch);
    msg.src_id = control_src_id_;
    msg.dst_id = "broadcast";
    msg.flow_type = 1;
    msg.control_type = "RECOVERY_START";
    msg.payload = std::to_string(epoch);
    msg.creation_time = this->now();
    msg.hop_count = 0;
    msg.ttl = control_ttl_;
    msg.requires_ack = false;
    msg.next_hop_id = "";
    traffic_pub_->publish(msg);
  }

  void publishClusterReassign(const std::string & member_id, const std::string & ch_id)
  {
    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = "CLUSTER_REASSIGN_" + member_id + "_" + std::to_string(msg_counter_++);
    msg.src_id = control_src_id_;
    msg.dst_id = member_id;
    msg.flow_type = 1;
    msg.control_type = "CLUSTER_REASSIGN";
    msg.payload = ch_id;
    msg.creation_time = this->now();
    msg.hop_count = 0;
    msg.ttl = control_ttl_;
    msg.requires_ack = true;
    msg.next_hop_id = ch_id;
    sendWithAck(msg);
    const std::string cluster_id =
      ch_cluster_ids_.count(ch_id) > 0 ? ch_cluster_ids_.at(ch_id) : std::string("unknown");
    RCLCPP_WARN(this->get_logger(),
                "[RECOVERY] CLUSTER_REASSIGN member=%s new_ch=%s cluster=%s msg_id=%s",
                member_id.c_str(), ch_id.c_str(), cluster_id.c_str(), msg.msg_id.c_str());
  }

  void publishTaskAssign(const std::string & member_id,
                         const std::vector<TaskPoint> & tasks)
  {
    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = "TASK_ASSIGN_" + member_id + "_" + std::to_string(msg_counter_++);
    msg.src_id = control_src_id_;
    msg.dst_id = member_id;
    msg.flow_type = 1;
    msg.control_type = "TASK_ASSIGN";
    msg.creation_time = this->now();
    msg.hop_count = 0;
    msg.ttl = control_ttl_;
    msg.requires_ack = true;

    std::ostringstream oss;
    for (size_t i = 0; i < tasks.size(); ++i) {
      const auto & tp = tasks[i];
      if (i > 0) {
        oss << ";";
      }
      oss << tp.position.x << "," << tp.position.y << "," << tp.position.z;
    }
    msg.payload = oss.str();

    std::string next_hop = member_id;
    auto it = member_states_.find(member_id);
    if (it != member_states_.end() && !it->second.ch_id.empty()) {
      next_hop = it->second.ch_id;
    }
    msg.next_hop_id = next_hop;
    sendWithAck(msg);
    RCLCPP_WARN(this->get_logger(),
                "[RECOVERY] TASK_ASSIGN %s points=%zu via %s",
                member_id.c_str(), tasks.size(), next_hop.c_str());
  }

  void publishNewDeployment(const std::string & ch_id, const geometry_msgs::msg::Pose & pose)
  {
    if (!ch_states_.count(ch_id)) {
      return;
    }
    const auto & current = ch_states_[ch_id].pose;
    double dist = distance2d(current.position, pose.position);
    if (dist < movement_delta_m_) {
      return;
    }

    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = "NEW_DEPLOYMENT_" + ch_id + "_" + std::to_string(msg_counter_++);
    msg.src_id = control_src_id_;
    msg.dst_id = ch_id;
    msg.flow_type = 1;
    msg.control_type = "NEW_DEPLOYMENT";
    msg.creation_time = this->now();
    msg.hop_count = 0;
    msg.ttl = control_ttl_;
    msg.requires_ack = true;

    std::ostringstream oss;
    oss << pose.position.x << "," << pose.position.y << "," << pose.position.z;
    msg.payload = oss.str();
    msg.next_hop_id = ch_id;
    sendWithAck(msg);
    RCLCPP_WARN(this->get_logger(),
                "[RECOVERY] NEW_DEPLOYMENT %s -> (%.1f, %.1f, %.1f)",
                ch_id.c_str(),
                pose.position.x,
                pose.position.y,
                pose.position.z);
  }

  void publishRecoveryDone(int epoch)
  {
    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = "RECOVERY_DONE_" + std::to_string(epoch);
    msg.src_id = control_src_id_;
    msg.dst_id = "broadcast";
    msg.flow_type = 1;
    msg.control_type = "RECOVERY_DONE";
    msg.payload = std::to_string(epoch);
    msg.creation_time = this->now();
    msg.hop_count = 0;
    msg.ttl = control_ttl_;
    msg.requires_ack = false;
    msg.next_hop_id = "";
    traffic_pub_->publish(msg);
  }

  void sendWithAck(const uav_msgs::msg::TrafficMessage & msg)
  {
    if (!traffic_pub_) {
      return;
    }
    traffic_pub_->publish(msg);
    PendingAck pending;
    pending.msg = msg;
    pending.last_send_time = this->now();
    pending.attempts = 1;
    pending.max_retries = max_ack_retries_;
    pending.retry_interval_sec = ack_retry_period_sec_;
    pending_acks_[msg.msg_id] = pending;
    RCLCPP_INFO(this->get_logger(),
                "[RECOVERY] sent %s msg_id=%s dst=%s next_hop=%s requires_ack=%s",
                msg.control_type.c_str(),
                msg.msg_id.c_str(),
                msg.dst_id.c_str(),
                msg.next_hop_id.c_str(),
                msg.requires_ack ? "true" : "false");
  }

  void resendPendingAcks()
  {
    if (pending_acks_.empty()) {
      return;
    }
    auto now = this->now();
    for (auto it = pending_acks_.begin(); it != pending_acks_.end();) {
      auto & pending = it->second;
      if (pending.max_retries > 0 && pending.attempts >= pending.max_retries) {
        RCLCPP_WARN(this->get_logger(),
                    "[RECOVERY] giving up on %s msg_id=%s after %d attempts",
                    pending.msg.control_type.c_str(),
                    pending.msg.msg_id.c_str(),
                    pending.attempts);
        it = pending_acks_.erase(it);
        continue;
      }
      double elapsed = (now - pending.last_send_time).seconds();
      if (elapsed >= pending.retry_interval_sec) {
        traffic_pub_->publish(pending.msg);
        pending.last_send_time = now;
        pending.attempts++;
      }
      ++it;
    }
  }

  std::string extractAckedMsgId(const uav_msgs::msg::TrafficMessage & msg) const
  {
    if (!msg.ref_msg_id.empty()) {
      return msg.ref_msg_id;
    }
    return msg.payload;
  }

  std::string sink_id_;
  std::string ugv_id_;
  std::string control_src_id_;
  double comm_range_m_ = 400.0;
  double status_timeout_sec_ = 5.0;
  double heartbeat_timeout_sec_ = 4.0;
  double recovery_cooldown_sec_ = 2.0;
  double movement_delta_m_ = 5.0;
  uint32_t control_ttl_ = 12;
  double ack_retry_period_sec_ = 0.5;
  int max_ack_retries_ = 5;
  double recovery_lock_duration_sec_ = 10.0;

  geometry_msgs::msg::Pose sink_pose_;
  bool sink_pose_known_ = false;
  geometry_msgs::msg::Pose ugv_pose_;
  bool ugv_pose_known_ = false;

  std::unordered_map<std::string, ChState> ch_states_;
  std::unordered_map<std::string, MemberState> member_states_;
  std::unordered_map<std::string, std::string> ch_cluster_ids_;
  std::unordered_map<std::string, rclcpp::Time> last_heartbeat_;
  std::vector<TaskPoint> task_points_;
  std::unordered_map<std::string, PendingAck> pending_acks_;
  // Members recently reassigned by recovery; ch_manager updates are suppressed
  // for recovery_lock_duration_sec_ after reassignment.
  std::unordered_map<std::string, rclcpp::Time> recovery_assigned_;

  bool sink_unreachable_ = false;
  bool ugv_unreachable_ = false;
  bool recovery_requested_ = false;

  int epoch_ = 0;
  int msg_counter_ = 0;
  rclcpp::Time last_recovery_time_;

  rclcpp::Subscription<uav_msgs::msg::UavStatus>::SharedPtr status_sub_;
  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr heartbeat_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr routing_alert_sub_;
  rclcpp::Subscription<uav_msgs::msg::FailureEvent>::SharedPtr failure_sub_;
  rclcpp::Subscription<uav_msgs::msg::TaskPointArray>::SharedPtr task_sub_;
  rclcpp::Subscription<uav_msgs::msg::ClusterInfo>::SharedPtr cluster_sub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_pub_;
  rclcpp::TimerBase::SharedPtr ack_retry_timer_;
  rclcpp::TimerBase::SharedPtr watchdog_timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<RecoveryManagerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
