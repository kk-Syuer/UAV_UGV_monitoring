#include <memory>
#include <string>
#include <deque>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/charge_decision.hpp"
#include "uav_msgs/msg/uav_status.hpp"

using namespace std::chrono_literals;

class UgvChargerNode : public rclcpp::Node
{
public:
  UgvChargerNode()
  : Node("ugv_charger_node"),
    dock_busy_(false)
  {
    ugv_id_ = this->declare_parameter<std::string>("ugv_id", "ugv");

    charging_duration_sec_ =
      this->declare_parameter<double>("charging_duration_sec", 20.0);

    std::string policy_name =
      this->declare_parameter<std::string>("charging_policy", "fcfs");

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
  };

  // ------------- Callbacks -------------

  void statusCallback(const uav_msgs::msg::UavStatus::SharedPtr msg)
  {
    UavInfo info;
    info.role = msg->role;
    info.battery_level = msg->battery_level;
    uav_status_[msg->uav_id] = info;
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

    // If control_type is set, check it's a CHARGE_REQUEST
    if (!msg->control_type.empty() && msg->control_type != "CHARGE_REQUEST") {
      return;
    }

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
  }

  // ------------- Scheduler -------------

  void schedulerLoop()
  {
    auto now = this->now();

    // If dock is busy, check if current session is finished
    if (dock_busy_) {
      if (now >= dock_free_time_) {
        RCLCPP_INFO(this->get_logger(),
                    "Charging session completed for %s at t=%.1f",
                    current_uav_id_.c_str(),
                    dock_free_time_.seconds());
        dock_busy_ = false;
        current_uav_id_.clear();
      } else {
        return; // still charging
      }
    }

    // Dock is free; if no one waiting, nothing to do
    if (queue_.empty()) {
      return;
    }

    // Choose according to current policy
    size_t idx = chooseNextIndex(now);
    QueueEntry job = queue_[idx];
    queue_.erase(queue_.begin() + static_cast<long>(idx));

    // Assign dock now
    rclcpp::Time slot_start_time = now;
    dock_busy_ = true;
    dock_free_time_ = slot_start_time +
                      rclcpp::Duration::from_seconds(charging_duration_sec_);
    current_uav_id_ = job.uav_id;

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
                "Session: [%.1f, %.1f], queue size now: %zu",
                job.uav_id.c_str(), job.role, job.battery_level,
                policy_name_.c_str(),
                slot_start_time.seconds(), dock_free_time_.seconds(),
                queue_.size());
    // Also send the decision through the routed network as a control message
    sendDecisionControlMessage(job, now);

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

  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Subscription<uav_msgs::msg::UavStatus>::SharedPtr status_sub_;
  rclcpp::Publisher<uav_msgs::msg::ChargeDecision>::SharedPtr charge_decision_pub_;
  rclcpp::TimerBase::SharedPtr scheduler_timer_;
  std::string uplink_ch_id_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr control_pub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr delivered_pub_;

  double charging_duration_sec_;
  Policy policy_;
  std::string policy_name_;

  // EDF parameters
  double drain_percent_member_;
  double drain_percent_ch_;

  // Dynamic-score parameters
  double w_role_;
  double w_batt_;
  double w_wait_;

  std::deque<QueueEntry> queue_;

  bool dock_busy_;
  rclcpp::Time dock_free_time_;
  std::string current_uav_id_;

  std::unordered_map<std::string, UavInfo> uav_status_;
  void sendDecisionControlMessage(const QueueEntry & job,
                                  const rclcpp::Time & now)
  {
    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = ugv_id_ + "_charge_decision_" + job.uav_id + "_" +
                 std::to_string(now.nanoseconds());
    msg.src_id = ugv_id_;
    msg.dst_id = job.uav_id;
    msg.next_hop_id = uplink_ch_id_;

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

};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UgvChargerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
