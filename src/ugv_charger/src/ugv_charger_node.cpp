#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/srv/request_charge.hpp"

class UgvChargerNode : public rclcpp::Node
{
public:
  UgvChargerNode()
  : Node("ugv_charger_node"),
    first_request_(true)
  {
    charging_duration_sec_ =
      this->declare_parameter<double>("charging_duration_sec", 20.0);  // seconds

    service_ = this->create_service<uav_msgs::srv::RequestCharge>(
      "/ugv/request_charge",
      std::bind(&UgvChargerNode::handleRequest, this,
                std::placeholders::_1, std::placeholders::_2));

    RCLCPP_INFO(this->get_logger(),
                "UGV charger started. Charging duration=%.1f s",
                charging_duration_sec_);
  }

private:
  void handleRequest(
    const std::shared_ptr<uav_msgs::srv::RequestCharge::Request> request,
    std::shared_ptr<uav_msgs::srv::RequestCharge::Response> response)
  {
    auto now = this->now();

    if (first_request_) {
      next_free_time_ = now;
      first_request_ = false;
    }

    // If dock is already free in the past, reset it to now.
    if (next_free_time_ < now) {
      next_free_time_ = now;
    }

    // FCFS: assign this UAV the next available slot
    response->accepted = true;
    response->eta = next_free_time_;
    response->assigned_priority = 0;  // for future use

    double wait = (next_free_time_ - now).seconds();

    RCLCPP_INFO(this->get_logger(),
                "Assigned charge slot to %s (role=%u, batt=%.1f%%). "
                "Start in %.1f s (at t=%.1f).",
                request->uav_id.c_str(),
                request->role,
                request->battery_level,
                wait,
                response->eta.sec + response->eta.nanosec * 1e-9);

    // Move dock's next free time forward by one charging session
    next_free_time_ = next_free_time_ +
                      rclcpp::Duration::from_seconds(charging_duration_sec_);
  }

  rclcpp::Service<uav_msgs::srv::RequestCharge>::SharedPtr service_;

  double charging_duration_sec_;
  rclcpp::Time next_free_time_;
  bool first_request_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UgvChargerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
