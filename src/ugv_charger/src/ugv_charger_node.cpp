#include <cmath>
#include <memory>
#include <string>
#include <deque>
#include <unordered_map>
#include <vector>
#include <sstream>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/charge_decision.hpp"
#include "uav_msgs/msg/uav_status.hpp"
#include "uav_msgs/msg/uav_deployment.hpp"
#include "geometry_msgs/msg/pose.hpp"

using namespace std::chrono_literals;

int compute_required_charging_spots(
    int    num_ch,
    int    num_members,
    double flight_time_ch_min,
    double charge_time_ch_min,
    double flight_time_mem_min,
    double charge_time_mem_min,
    double target_utilization)
{
  if (num_ch == 0 && num_members == 0) {
    return 0;
  }

  double load_ch = 0.0;
  if (flight_time_ch_min + charge_time_ch_min > 0.0) {
    load_ch = charge_time_ch_min / (flight_time_ch_min + charge_time_ch_min);
  }

  double load_mem = 0.0;
  if (flight_time_mem_min + charge_time_mem_min > 0.0) {
    load_mem = charge_time_mem_min / (flight_time_mem_min + charge_time_mem_min);
  }

  double total_load = num_ch * load_ch + num_members * load_mem;

  double effective_target = target_utilization <= 0.0 ? 0.9 : target_utilization;

  double raw_spots = total_load / effective_target;

  int num_spots = static_cast<int>(std::ceil(raw_spots));
  if (num_spots < 1 && total_load > 0.0) {
    num_spots = 1;
  }

  return num_spots;
}

class UgvChargerNode : public rclcpp::Node
{
public:
  UgvChargerNode()
  : Node("ugv_charger_node"),
    max_parallel_spots_(1)
  {
    ugv_id_ = this->declare_parameter<std::string>("ugv_id", "ugv");

    charging_duration_sec_ =
      this->declare_parameter<double>("charging_duration_sec", 20.0);

    std::string policy_name =
      this->declare_parameter<std::string>("charging_policy", "fcfs");
    target_utilization_ = this->declare_parameter<double>("target_utilization", 0.8);
    flight_time_ch_min_ = this->declare_parameter<double>("flight_time_ch_min", 90.0);
    charge_time_ch_min_ = this->declare_parameter<double>("charge_time_ch_min", 30.0);
    flight_time_mem_min_ = this->declare_parameter<double>("flight_time_mem_min", 45.0);
    charge_time_mem_min_ = this->declare_parameter<double>("charge_time_mem_min", 20.0);
    deployment_sub_ = this->create_subscription<uav_msgs::msg::UavDeployment>(
      "/coverage_planner/deployment", 10,
      std::bind(&UgvChargerNode::deploymentCallback, this, std::placeholders::_1));
    sink_id_ = this->declare_parameter<std::string>("sink_id", "sink_gateway");

    mobility_enabled_ = this->declare_parameter<bool>("mobility_enabled", true);
    mobility_dt_sec_  = this->declare_parameter<double>("mobility_dt_sec", 0.2);
    ugv_speed_mps_    = this->declare_parameter<double>("ugv_speed_mps", 15.3);

    if (policy_name == "role_priority") {
      policy_ = Policy::ROLE_PRIORITY;
    } else if (policy_name == "edf") {
      policy_ = Policy::EDF;
    } else if (policy_name == "dynamic") {
      policy_ = Policy::DYNAMIC_SCORE;
    } else {
      policy_ = Policy::FCFS;
      policy_name = "fcfs";
    }
    policy_name_ = policy_name;
    uplink_ch_id_ =
      this->declare_parameter<std::string>("uplink_ch_id", "uav_3");

    // Parameters for EDF: approximate drain of battery percentage per second
    drain_percent_member_ = this->declare_parameter<double>("drain_percent_member", 0.5); // %/s
    drain_percent_ch_     = this->declare_parameter<double>("drain_percent_ch", 0.25);    // %/s

    // Parameters for dynamic score
    w_role_ = this->declare_parameter<double>("w_role", 5.0);
    w_batt_ = this->declare_parameter<double>("w_batt", 1.0);
    w_wait_ = this->declare_parameter<double>("w_wait", 0.1);

    charge_decision_pub_ = this->create_publisher<uav_msgs::msg::ChargeDecision>(
      "/ugv/charge_decisions", 10);
    control_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 100);
    hello_period_sec_ = this->declare_parameter<double>("hello_period_sec", 1.0);
    RCLCPP_INFO(this->get_logger(),
                "UGV charger started. id='%s', policy='%s', charging_duration=%.1f s, uplink_ch_id='%s'",
                ugv_id_.c_str(), policy_name_.c_str(),
                charging_duration_sec_, uplink_ch_id_.c_str());
    delivered_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/network/traffic_delivered", 100);

    // NEW: subscribe to network control messages
    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 100,
      std::bind(&UgvChargerNode::trafficCallback, this, std::placeholders::_1));

    // NEW: subscribe to UAV status to know battery & role
    status_sub_ = this->create_subscription<uav_msgs::msg::UavStatus>(
      "/uav_fleet/status", 100,
      std::bind(&UgvChargerNode::statusCallback, this, std::placeholders::_1));

    scheduler_timer_ = this->create_wall_timer(
      500ms, std::bind(&UgvChargerNode::schedulerLoop, this));

    auto hello_period = std::chrono::duration<double>(hello_period_sec_);
    hello_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(hello_period),
      std::bind(&UgvChargerNode::publishHello, this));

    ugv_pose_.position.x = 0.0;
    ugv_pose_.position.y = 0.0;
    ugv_pose_.position.z = 0.0;
    ugv_pose_.orientation.w = 1.0;
    deployment_goal_pose_ = ugv_pose_;
    last_pose_time_ = this->now();

    if (mobility_enabled_) {
      auto dt = std::chrono::duration<double>(mobility_dt_sec_);
      mobility_timer_ = this->create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(dt),
        std::bind(&UgvChargerNode::mobilityStep, this));
    }

    recomputeChargingSpots();

    RCLCPP_INFO(this->get_logger(),
                "UGV charger started. id='%s', policy='%s', charging_duration=%.1f s",
                ugv_id_.c_str(), policy_name_.c_str(), charging_duration_sec_);
  }

private:
  enum class Policy
  {
    FCFS,
    ROLE_PRIORITY,
    EDF,
    DYNAMIC_SCORE
  };

  struct QueueEntry
  {
    std::string uav_id;
    uint8_t role;         // 0=member,1=CH
    float battery_level;  // percent at request time
    rclcpp::Time request_time;
  };

  struct UavInfo
  {
    uint8_t role;
    float battery_level;
    bool backbone_active; // whether this UAV is usable as backbone
  };

  // ------------- Callbacks -------------

  void statusCallback(const uav_msgs::msg::UavStatus::SharedPtr msg)
  {
    UavInfo info;
    info.role = msg->role;
    info.battery_level = msg->battery_level;
    info.backbone_active = msg->backbone_active;
    uav_status_[msg->uav_id] = info;
    recomputeChargingSpots();
  }

  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    // Only consider messages whose final destination is this UGV
    if (msg->dst_id != ugv_id_) {
      return;
    }

    // Only consider CONTROL_ALERT messages
    if (msg->msg_type != 3) { // 3 = CONTROL_ALERT
      return;
    }

    if (msg->control_type == "MOTION_START" || msg->control_type == "START_MOBILITY") {
      motion_start_received_ = true;
      last_pose_time_ = this->now();
      RCLCPP_INFO(this->get_logger(),
                  "[MOB-START] %s received %s from %s", ugv_id_.c_str(),
                  msg->control_type.c_str(), msg->src_id.c_str());
      return;
    }

    if (msg->control_type == "CHARGE_REQUEST") {
      const std::string & uav_id = msg->src_id;
      auto now = this->now();

      // Lookup last known status for this UAV
      auto it = uav_status_.find(uav_id);
      if (it == uav_status_.end()) {
        RCLCPP_WARN(this->get_logger(),
                    "UGV: received CHARGE_REQUEST from '%s' but no status known. Ignoring.",
                    uav_id.c_str());
        return;
      }

      QueueEntry entry;
      entry.uav_id = uav_id;
      entry.role = it->second.role;
      entry.battery_level = it->second.battery_level;
      entry.request_time = now;

      queue_.push_back(entry);
      delivered_pub_->publish(*msg);


      RCLCPP_INFO(this->get_logger(),
                  "UGV: enqueued CHARGE_REQUEST from %s (role=%u, batt=%.1f%%). "
                  "Queue size now: %zu",
                  entry.uav_id.c_str(), entry.role, entry.battery_level, queue_.size());
      return;
    }

    if (msg->control_type == "DEPLOYMENT_CMD" || msg->control_type == "DEPLOYMENT") {
      handleDeploymentFromNetwork(msg);
      return;
    }
  }

  void setDeploymentGoal(const geometry_msgs::msg::Pose & target)
  {
    deployment_goal_pose_ = target;
    has_deployment_goal_ = true;
    ugv_in_motion_ = true;
    deployment_received_ = true;
    motion_start_received_ = false;

    if (last_pose_time_.nanoseconds() == 0) {
      last_pose_time_ = this->now();
    }

    RCLCPP_INFO(this->get_logger(),
                "UGV %s: received deployment target -> (%.1f, %.1f, %.1f)",
                ugv_id_.c_str(),
                deployment_goal_pose_.position.x,
                deployment_goal_pose_.position.y,
                deployment_goal_pose_.position.z);
  }

  void mobilityStep()
  {
    if (!mobility_enabled_ || !has_deployment_goal_ ||
        !deployment_received_ || !motion_start_received_) {
      return;
    }

    auto now = this->now();
    double dt = mobility_dt_sec_;
    if (last_pose_time_.nanoseconds() > 0 &&
        last_pose_time_.get_clock_type() == now.get_clock_type())
    {
      dt = (now - last_pose_time_).seconds();
      if (dt <= 0.0) {
        dt = mobility_dt_sec_;
      }
    }
    last_pose_time_ = now;

    double dx = deployment_goal_pose_.position.x - ugv_pose_.position.x;
    double dy = deployment_goal_pose_.position.y - ugv_pose_.position.y;
    double dist = std::sqrt(dx * dx + dy * dy);
    double max_step = ugv_speed_mps_ * dt;

    if (dist <= 1e-3) {
      ugv_in_motion_ = false;
      return;
    }

    if (dist <= max_step) {
      ugv_pose_ = deployment_goal_pose_;
      ugv_in_motion_ = false;
      RCLCPP_INFO(this->get_logger(),
                  "UGV %s reached deployment pose (%.1f, %.1f, %.1f)",
                  ugv_id_.c_str(),
                  ugv_pose_.position.x,
                  ugv_pose_.position.y,
                  ugv_pose_.position.z);
      return;
    }

    double ux = dx / dist;
    double uy = dy / dist;
    ugv_pose_.position.x += ux * max_step;
    ugv_pose_.position.y += uy * max_step;
    // keep current z (UGV stays on ground unless target dictates otherwise)
  }

  void deploymentCallback(const uav_msgs::msg::UavDeployment::SharedPtr msg)
  {
    // Always store the pose for possible future use
    if (msg->role == 1) {
      // CH
      ch_poses_[msg->uav_id] = msg->target_pose;
    }

    // Remember each UAV's "home CH":
    //  - For members (role=0), ch_id is their CH
    //  - For CHs (role=1), the CH is itself
    if (msg->role == 0) {
      uav_to_ch_[msg->uav_id] = msg->ch_id;
    } else if (msg->role == 1) {
      uav_to_ch_[msg->uav_id] = msg->uav_id;
    }

    // If this deployment is for the UGV itself, update its pose and log
    if (msg->uav_id == ugv_id_) {
      deployment_ack_sent_ = false;
      setDeploymentGoal(msg->target_pose);
      sendDeploymentAck();
    }
  }

  void handleDeploymentFromNetwork(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    // msg->control_payload format:
    // "role,cluster_id,ch_id,x,y,z,next_sink,next_ugv"
    std::stringstream ss(msg->control_payload);
    std::string token;
    std::string cluster_id, ch_id;
    double x = 0.0, y = 0.0, z = 0.0;
    std::string next_sink;

    // role (unused beyond validation)
    if (!std::getline(ss, token, ',')) {
      RCLCPP_WARN(this->get_logger(),
                  "UGV %s: bad DEPLOYMENT payload \"%s\"",
                  ugv_id_.c_str(), msg->control_payload.c_str());
      return;
    }

    // cluster_id
    std::getline(ss, cluster_id, ',');
    // ch_id
    std::getline(ss, ch_id, ',');

    // x
    std::getline(ss, token, ',');
    x = std::stod(token);
    // y
    std::getline(ss, token, ',');
    y = std::stod(token);
    // z
    std::getline(ss, token, ',');
    z = std::stod(token);

    // next_sink
    std::getline(ss, next_sink, ',');

    deployment_ack_sent_ = false;

    geometry_msgs::msg::Pose goal;
    goal.position.x = x;
    goal.position.y = y;
    goal.position.z = z;
    goal.orientation.w = 1.0;
    setDeploymentGoal(goal);

    RCLCPP_INFO(this->get_logger(),
                "UGV %s: DEPLOYMENT via network -> pos=(%.1f,%.1f,%.1f) next_sink=%s",
                ugv_id_.c_str(),
                deployment_goal_pose_.position.x,
                deployment_goal_pose_.position.y,
                deployment_goal_pose_.position.z,
                next_sink.empty() ? "-" : next_sink.c_str());

    delivered_pub_->publish(*msg);
    sendDeploymentAck(next_sink);
  }

  void sendDeploymentAck(const std::string & suggested_next_hop = "")
  {
    if (deployment_ack_sent_) {
      return;
    }

    if (!control_pub_) {
      return;
    }

    uav_msgs::msg::TrafficMessage ack;
    ack.msg_id = "DEP_ACK_" + ugv_id_ + "_" + std::to_string(dep_ack_seq_++);
    ack.src_id = ugv_id_;
    ack.dst_id = sink_id_;

    if (!suggested_next_hop.empty() && suggested_next_hop != "-") {
      ack.next_hop_id = suggested_next_hop;
    } else if (!uplink_ch_id_.empty()) {
      ack.next_hop_id = uplink_ch_id_;
    } else {
      ack.next_hop_id = sink_id_;
    }

    ack.msg_type = 3;
    ack.priority = 1;
    ack.size_bytes = 16;
    ack.creation_time = this->now();
    ack.hop_count = 0;

    ack.control_type = "DEPLOYMENT_ACK";
    ack.control_payload = "";

    control_pub_->publish(ack);
    deployment_ack_sent_ = true;

    RCLCPP_INFO(this->get_logger(),
                "UGV %s sent DEPLOYMENT_ACK to sink via %s",
                ugv_id_.c_str(), ack.next_hop_id.c_str());
  }

  void publishHello()
  {
    if (!control_pub_) {
      return;
    }

    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = ugv_id_ + "_HELLO_" + std::to_string(msg_counter_++);
    msg.src_id = ugv_id_;
    msg.dst_id = "broadcast";
    msg.next_hop_id = "";  // broadcast semantics

    msg.msg_type = 3;       // CONTROL
    msg.priority = 0;
    msg.size_bytes = 32;
    msg.creation_time = this->now();
    msg.hop_count = 0;
    msg.ttl = 1;            // single-hop broadcast
    msg.control_type = "HELLO";

    std::ostringstream oss;
    oss << "UGV,"
        << ugv_pose_.position.x << ","
        << ugv_pose_.position.y << ","
        << 100.0;
    msg.control_payload = oss.str();

    control_pub_->publish(msg);
  }

  // ------------- Scheduler -------------

  void schedulerLoop()
  {
    auto now = this->now();

    // Remove finished charging sessions
    auto it = active_sessions_.begin();
    while (it != active_sessions_.end()) {
      if (now >= it->end_time) {
        RCLCPP_INFO(this->get_logger(),
                    "Charging session completed for %s at t=%.1f",
                    it->uav_id.c_str(),
                    it->end_time.seconds());
        it = active_sessions_.erase(it);
      } else {
        ++it;
      }
    }

    // If no dock capacity, exit early
    if (max_parallel_spots_ == 0) {
      return;
    }

    // Dock is free; if no one waiting, nothing to do
    if (queue_.empty()) {
      return;
    }

    size_t available_spots = 0;
    if (max_parallel_spots_ > static_cast<int>(active_sessions_.size())) {
      available_spots = static_cast<size_t>(max_parallel_spots_ - active_sessions_.size());
    }

    while (available_spots > 0 && !queue_.empty()) {
      size_t idx = chooseNextIndex(now);
      QueueEntry job = queue_[idx];
      queue_.erase(queue_.begin() + static_cast<long>(idx));

      rclcpp::Time slot_start_time = now;
      rclcpp::Time slot_end_time = slot_start_time +
                                   rclcpp::Duration::from_seconds(charging_duration_sec_);

      // Publish ChargeDecision (direct, not routed yet)
      uav_msgs::msg::ChargeDecision decision;
      decision.uav_id = job.uav_id;
      decision.accepted = true;
      decision.slot_start_time = slot_start_time;
      decision.priority = (job.role == 1 ? 1 : 0);
      decision.policy = policy_name_;

      charge_decision_pub_->publish(decision);

      RCLCPP_INFO(this->get_logger(),
                  "UGV: assigned dock to %s (role=%u, batt=%.1f%%) with policy='%s'. "
                  "Session: [%.1f, %.1f], queue size now: %zu, active sessions: %zu/%d",
                  job.uav_id.c_str(), job.role, job.battery_level,
                  policy_name_.c_str(),
                  slot_start_time.seconds(), slot_end_time.seconds(),
                  queue_.size(), active_sessions_.size() + 1, max_parallel_spots_);
      // Also send the decision through the routed network as a control message
      sendDecisionControlMessage(job, now);

      active_sessions_.push_back({job.uav_id, slot_start_time, slot_end_time});

      if (max_parallel_spots_ > static_cast<int>(active_sessions_.size())) {
        available_spots = static_cast<size_t>(max_parallel_spots_ - active_sessions_.size());
      } else {
        available_spots = 0;
      }
    }

  }

  // ------------- Policy-specific selection -------------

  size_t chooseNextIndex(const rclcpp::Time & now)
  {
    if (policy_ == Policy::FCFS) {
      return 0;
    }

    if (policy_ == Policy::ROLE_PRIORITY) {
      for (size_t i = 0; i < queue_.size(); ++i) {
        if (queue_[i].role == 1) { // CH
          return i;
        }
      }
      return 0;
    }

    if (policy_ == Policy::EDF) {
      double best_tte = std::numeric_limits<double>::infinity();
      size_t best_idx = 0;

      for (size_t i = 0; i < queue_.size(); ++i) {
        const auto & q = queue_[i];
        double drain = (q.role == 1) ? drain_percent_ch_ : drain_percent_member_;
        if (drain <= 0.0) {
          continue;
        }
        double tte = q.battery_level / drain;  // seconds until empty (approx.)
        if (tte < best_tte) {
          best_tte = tte;
          best_idx = i;
        }
      }
      return best_idx;
    }

    // Policy::DYNAMIC_SCORE
    double best_score = -1e18;
    size_t best_idx = 0;

    for (size_t i = 0; i < queue_.size(); ++i) {
      const auto & q = queue_[i];

      double role_term = (q.role == 1 ? 1.0 : 0.0);      // CH = 1, member = 0
      double batt_term = (100.0 - q.battery_level);      // lower battery -> larger term
      double wait_sec = (now - q.request_time).seconds();
      if (wait_sec < 0.0) wait_sec = 0.0;

      double score = w_role_ * role_term
                   + w_batt_ * batt_term
                   + w_wait_ * wait_sec;

      if (score > best_score) {
        best_score = score;
        best_idx = i;
      }
    }

    return best_idx;
  }

  // ------------- Members -------------

  std::string ugv_id_;
  std::string sink_id_;
  geometry_msgs::msg::Pose ugv_pose_;
  rclcpp::Subscription<uav_msgs::msg::UavDeployment>::SharedPtr deployment_sub_;

  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Subscription<uav_msgs::msg::UavStatus>::SharedPtr status_sub_;
  rclcpp::Publisher<uav_msgs::msg::ChargeDecision>::SharedPtr charge_decision_pub_;
  rclcpp::TimerBase::SharedPtr scheduler_timer_;
  rclcpp::TimerBase::SharedPtr hello_timer_;
  rclcpp::TimerBase::SharedPtr mobility_timer_;
  std::string uplink_ch_id_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr control_pub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_pub_;
  uint64_t msg_counter_ = 0;

  double charging_duration_sec_;
  double target_utilization_;
  double flight_time_ch_min_;
  double charge_time_ch_min_;
  double flight_time_mem_min_;
  double charge_time_mem_min_;
  bool mobility_enabled_ = true;
  double mobility_dt_sec_ = 0.2;
  double ugv_speed_mps_ = 15.3;
  Policy policy_;
  std::string policy_name_;

  // EDF parameters
  double drain_percent_member_;
  double drain_percent_ch_;

  // Dynamic-score parameters
  double w_role_;
  double w_batt_;
  double w_wait_;
  double hello_period_sec_ = 1.0;
  geometry_msgs::msg::Pose deployment_goal_pose_;
  bool has_deployment_goal_ = false;
  bool ugv_in_motion_ = false;
  bool deployment_received_ = false;
  bool motion_start_received_ = false;
  rclcpp::Time last_pose_time_;

  bool deployment_ack_sent_ = false;
  uint64_t dep_ack_seq_ = 0;

  std::deque<QueueEntry> queue_;
  int max_parallel_spots_;
  struct ChargingSession
  {
    std::string uav_id;
    rclcpp::Time start_time;
    rclcpp::Time end_time;
  };

  std::vector<ChargingSession> active_sessions_;

  std::unordered_map<std::string, UavInfo> uav_status_;

  // Map each UAV to its CH, based on deployments
  std::unordered_map<std::string, std::string> uav_to_ch_;

  // Optional: store CH poses if we want geometric reasoning later
  std::unordered_map<std::string, geometry_msgs::msg::Pose> ch_poses_;
  void sendDecisionControlMessage(const QueueEntry & job,
                                  const rclcpp::Time & now)
  {
    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = ugv_id_ + "_charge_decision_" + job.uav_id + "_" +
                 std::to_string(now.nanoseconds());
    msg.src_id = ugv_id_;
    msg.dst_id = job.uav_id;

    // Decide first hop:
    //  - If we know this UAV's CH from deployments:
    //      * For CHs: CH is itself -> direct (ugv -> uav_i)
    //      * For members: CH is their cluster head -> ugv -> CH
    //  - Otherwise: fall back to fixed uplink_ch_id_
    std::string first_hop = uplink_ch_id_;
    auto it = uav_to_ch_.find(job.uav_id);
    if (it != uav_to_ch_.end()) {
      first_hop = it->second;
    }
    msg.next_hop_id = first_hop;

    msg.msg_type = 3;              // CONTROL_ALERT
    msg.priority = 2;
    msg.size_bytes = 40;
    msg.creation_time = now;
    msg.hop_count = 0;

    msg.control_type = "CHARGE_DECISION";
    msg.control_payload = "";      // we start immediately, so no schedule needed

    RCLCPP_INFO(this->get_logger(),
                "UGV: sending CHARGE_DECISION msg_id=%s to %s via %s",
                msg.msg_id.c_str(), msg.dst_id.c_str(), msg.next_hop_id.c_str());

    control_pub_->publish(msg);
  }

  void recomputeChargingSpots()
  {
    int num_ch = 0;
    int num_members = 0;
    for (const auto & kv : uav_status_) {
      if (kv.second.role == 1) {
        ++num_ch;
      } else {
        ++num_members;
      }
    }

    double load_ch = 0.0;
    double load_mem = 0.0;
    if (flight_time_ch_min_ + charge_time_ch_min_ <= 0.0) {
      RCLCPP_WARN(this->get_logger(), "Invalid CH flight/charge times; skipping CH load contribution");
    } else {
      load_ch = charge_time_ch_min_ / (flight_time_ch_min_ + charge_time_ch_min_);
    }
    if (flight_time_mem_min_ + charge_time_mem_min_ <= 0.0) {
      RCLCPP_WARN(this->get_logger(), "Invalid member flight/charge times; skipping member load contribution");
    } else {
      load_mem = charge_time_mem_min_ / (flight_time_mem_min_ + charge_time_mem_min_);
    }

    double total_load = num_ch * load_ch + num_members * load_mem;
    double effective_target = target_utilization_ <= 0.0 ? 0.9 : target_utilization_;

    int required_spots = compute_required_charging_spots(
      num_ch,
      num_members,
      flight_time_ch_min_,
      charge_time_ch_min_,
      flight_time_mem_min_,
      charge_time_mem_min_,
      effective_target);

    if (required_spots != max_parallel_spots_) {
      RCLCPP_INFO(this->get_logger(),
                  "Computed required charging spots: %d (total_load=%.3f, target_utilization=%.2f, num_ch=%d, num_members=%d)",
                  required_spots, total_load, effective_target, num_ch, num_members);
    }
    max_parallel_spots_ = required_spots;
  }


};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UgvChargerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
