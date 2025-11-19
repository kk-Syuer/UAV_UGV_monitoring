#include <memory>
#include <string>
#include <deque>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/srv/request_charge.hpp"
#include "uav_msgs/msg/charge_decision.hpp"

using namespace std::chrono_literals;

class UgvChargerNode : public rclcpp::Node
{
public:
  UgvChargerNode()
  : Node("ugv_charger_node"),
    dock_busy_(false)
  {
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

    // Parameters for EDF: approximate drain of battery percentage per second
    drain_percent_member_ = this->declare_parameter<double>("drain_percent_member", 0.5); // %/s
    drain_percent_ch_     = this->declare_parameter<double>("drain_percent_ch", 0.25);    // %/s

    // Parameters for dynamic score
    w_role_ = this->declare_parameter<double>("w_role", 5.0);
    w_batt_ = this->declare_parameter<double>("w_batt", 1.0);
    w_wait_ = this->declare_parameter<double>("w_wait", 0.1);

    charge_decision_pub_ = this->create_publisher<uav_msgs::msg::ChargeDecision>(
      "/ugv/charge_decisions", 10);

    service_ = this->create_service<uav_msgs::srv::RequestCharge>(
      "/ugv/request_charge",
      std::bind(&UgvChargerNode::handleRequest, this,
                std::placeholders::_1, std::placeholders::_2));

    scheduler_timer_ = this->create_wall_timer(
      500ms, std::bind(&UgvChargerNode::schedulerLoop, this));

    RCLCPP_INFO(this->get_logger(),
                "UGV charger started. Policy='%s', charging_duration=%.1f s",
                policy_name_.c_str(), charging_duration_sec_);
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

  // --- Service: enqueue requests ---
  void handleRequest(
    const std::shared_ptr<uav_msgs::srv::RequestCharge::Request> request,
    std::shared_ptr<uav_msgs::srv::RequestCharge::Response> response)
  {
    auto now = this->now();

    QueueEntry entry;
    entry.uav_id = request->uav_id;
    entry.role = request->role;
    entry.battery_level = request->battery_level;
    entry.request_time = now;

    queue_.push_back(entry);

    // Service just acknowledges; real schedule will come via ChargeDecision
    response->accepted = true;
    response->eta = now;
    response->assigned_priority = (entry.role == 1 ? 1 : 0);

    RCLCPP_INFO(this->get_logger(),
                "Enqueued charge request from %s (role=%u, batt=%.1f%%). "
                "Queue size now: %zu",
                entry.uav_id.c_str(), entry.role, entry.battery_level, queue_.size());
  }

  // --- Main scheduler loop (runs periodically) ---
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

    // Choose index according to current policy
    size_t idx = chooseNextIndex(now);
    QueueEntry job = queue_[idx];
    queue_.erase(queue_.begin() + static_cast<long>(idx));

    // Assign dock now
    rclcpp::Time slot_start_time = now;
    dock_busy_ = true;
    dock_free_time_ = slot_start_time +
                      rclcpp::Duration::from_seconds(charging_duration_sec_);
    current_uav_id_ = job.uav_id;

    // Publish ChargeDecision
    uav_msgs::msg::ChargeDecision decision;
    decision.uav_id = job.uav_id;
    decision.accepted = true;
    decision.slot_start_time = slot_start_time;
    decision.priority = (job.role == 1 ? 1 : 0);
    decision.policy = policy_name_;

    charge_decision_pub_->publish(decision);

    RCLCPP_INFO(this->get_logger(),
                "Assigned dock to %s (role=%u, batt=%.1f%%) with policy='%s'. "
                "Session: [%.1f, %.1f], queue size now: %zu",
                job.uav_id.c_str(), job.role, job.battery_level,
                policy_name_.c_str(),
                slot_start_time.seconds(), dock_free_time_.seconds(),
                queue_.size());
  }

  // --- Policy-specific selection ---
  size_t chooseNextIndex(const rclcpp::Time & now)
  {
    if (policy_ == Policy::FCFS) {
      return 0;
    }

    if (policy_ == Policy::ROLE_PRIORITY) {
      // First CH in queue if any, else first
      for (size_t i = 0; i < queue_.size(); ++i) {
        if (queue_[i].role == 1) { // CH
          return i;
        }
      }
      return 0;
    }

    if (policy_ == Policy::EDF) {
      // Choose UAV with smallest estimated time-to-empty
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

  // --- Members ---
  rclcpp::Service<uav_msgs::srv::RequestCharge>::SharedPtr service_;
  rclcpp::Publisher<uav_msgs::msg::ChargeDecision>::SharedPtr charge_decision_pub_;
  rclcpp::TimerBase::SharedPtr scheduler_timer_;

  double charging_duration_sec_;
  Policy policy_;
  std::string policy_name_;

  // EDF parameters: approximate drain of battery percentage per second
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
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UgvChargerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
