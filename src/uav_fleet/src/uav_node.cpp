#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"

#include "uav_msgs/msg/uav_status.hpp"
#include "uav_msgs/msg/heartbeat.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/cluster_info.hpp"

#include "uav_msgs/srv/request_charge.hpp"

#include "geometry_msgs/msg/pose.hpp"

using namespace std::chrono_literals;

class UavNode : public rclcpp::Node
{
public:
  UavNode()
  : Node("uav_node"),
    msg_counter_(0),
    waiting_for_charge_response_(false),
    is_charging_(false),
    has_charge_slot_(false)
  {
    // ---- Parameters ----
    uav_id_ = this->declare_parameter<std::string>("uav_id", "uav_1");
    int role_int = this->declare_parameter<int>("role", 0);  // 0=MEMBER,1=CH
    role_ = static_cast<uint8_t>(role_int);

    cluster_id_ = this->declare_parameter<std::string>("cluster_id", "cluster_1");
    default_dst_id_ = this->declare_parameter<std::string>("default_dst_id", "sink_gateway");
    my_ch_id_ = this->declare_parameter<std::string>("my_ch_id", "uav_1");

    // Battery capacities (units are arbitrary "energy units")
    double cap_member = this->declare_parameter<double>("battery_capacity_member", 100.0);
    double cap_ch     = this->declare_parameter<double>("battery_capacity_ch", 200.0);
    battery_capacity_ = (role_ == 1) ? static_cast<float>(cap_ch)
                                     : static_cast<float>(cap_member);
    battery_energy_ = battery_capacity_;  // start full

    battery_threshold_percent_ =
      static_cast<float>(this->declare_parameter<double>("battery_threshold", 30.0));
    charging_duration_sec_ =
      this->declare_parameter<double>("charging_duration_sec", 20.0);

    RCLCPP_INFO(this->get_logger(),
                "Starting UAV node id='%s', role=%u, cluster=%s, dst='%s'. "
                "Batt capacity=%.1f, threshold=%.1f%%, charge_duration=%.1f s",
                uav_id_.c_str(), role_, cluster_id_.c_str(), default_dst_id_.c_str(),
                battery_capacity_, battery_threshold_percent_, charging_duration_sec_);

    // ---- Publishers ----
    status_pub_ = this->create_publisher<uav_msgs::msg::UavStatus>(
      "/uav_fleet/status", 10);
    heartbeat_pub_ = this->create_publisher<uav_msgs::msg::Heartbeat>(
      "/uav_fleet/heartbeat", 10);
    traffic_pub_ = this->create_publisher<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 10);

    // ---- Subscribers ----
    traffic_sub_ = this->create_subscription<uav_msgs::msg::TrafficMessage>(
      "/network/traffic", 10,
      std::bind(&UavNode::trafficCallback, this, std::placeholders::_1));

    cluster_sub_ = this->create_subscription<uav_msgs::msg::ClusterInfo>(
      "/ch_manager/cluster_info", 10,
      std::bind(&UavNode::clusterInfoCallback, this, std::placeholders::_1));

    // ---- Timers ----
    status_timer_ = this->create_wall_timer(
      1s, std::bind(&UavNode::publishStatus, this));
    heartbeat_timer_ = this->create_wall_timer(
      1s, std::bind(&UavNode::publishHeartbeat, this));
    traffic_timer_ = this->create_wall_timer(
      2s, std::bind(&UavNode::publishTraffic, this));

    // Dummy pose for now (0,0,10)
    pose_.position.x = 0.0;
    pose_.position.y = 0.0;
    pose_.position.z = 10.0;
    pose_.orientation.w = 1.0;

    service_radius_ = 100.0f;  // arbitrary

    // Charging client
    charge_client_ = this->create_client<uav_msgs::srv::RequestCharge>("/ugv/request_charge");

    RCLCPP_INFO(this->get_logger(),
                "UAV %s ready. Role=%u, CH capacity flag=%s",
                uav_id_.c_str(), role_, (role_ == 1 ? "YES" : "NO"));
  }

private:
  // ---------------- Battery & charging ----------------

  void publishStatus()
  {
    auto now = this->now();

    // START charging when our reserved slot begins
    if (!is_charging_ && has_charge_slot_ && now >= charge_start_time_) {
      is_charging_ = true;
      has_charge_slot_ = false;
      energy_at_charge_start_ = battery_energy_;
      charge_end_time_ = charge_start_time_ +
                         rclcpp::Duration::from_seconds(charging_duration_sec_);

      RCLCPP_INFO(this->get_logger(),
                  "UAV %s: starting charging session (from %.1f / %.1f)",
                  uav_id_.c_str(), battery_energy_, battery_capacity_);
    }

    if (is_charging_) {
      // Charging: interpolate energy from start value to full capacity
      if (now >= charge_end_time_) {
        battery_energy_ = battery_capacity_;
        is_charging_ = false;
        RCLCPP_INFO(this->get_logger(),
                    "UAV %s: finished charging, battery full (%.1f).",
                    uav_id_.c_str(), battery_energy_);
      } else {
        double total = (charge_end_time_ - charge_start_time_).seconds();
        double elapsed = (now - charge_start_time_).seconds();
        if (total > 0.0 && elapsed >= 0.0) {
          double frac = elapsed / total;
          if (frac < 0.0) frac = 0.0;
          if (frac > 1.0) frac = 1.0;
          battery_energy_ = energy_at_charge_start_ +
            static_cast<float>((battery_capacity_ - energy_at_charge_start_) * frac);
        }
      }
    } else {
      // Not charging: drain battery
      float drain_rate = 0.5f;  // energy units per second
      battery_energy_ -= drain_rate;
      if (battery_energy_ < 0.0f) battery_energy_ = 0.0f;
    }

    // Convert to percentage for reporting & thresholds
    float battery_percent = 0.0f;
    if (battery_capacity_ > 0.0f) {
      battery_percent = (battery_energy_ / battery_capacity_) * 100.0f;
    }

    // If low and not already waiting or scheduled, request a charge slot
    if (!is_charging_ && !waiting_for_charge_response_ && !has_charge_slot_ &&
        battery_percent <= battery_threshold_percent_)
    {
      requestCharge(battery_percent);
    }

    // Publish UavStatus
    uav_msgs::msg::UavStatus msg;
    msg.uav_id = uav_id_;
    msg.role = role_;
    msg.cluster_id = cluster_id_;
    msg.battery_level = battery_percent;
    msg.battery_capacity = battery_capacity_;
    msg.pose = pose_;
    msg.service_radius = service_radius_;
    msg.connected_users = 0;
    msg.traffic_load = 0.0f;
    msg.packet_loss_estimate = 0.0f;
    msg.energy_consumption_rate = 0.0f;
    msg.stamp = now;

    status_pub_->publish(msg);
  }

  void requestCharge(float battery_percent)
  {
    if (!charge_client_->wait_for_service(0s)) {
      RCLCPP_WARN(this->get_logger(),
                  "UAV %s: charge service not available", uav_id_.c_str());
      return;
    }

    auto req = std::make_shared<uav_msgs::srv::RequestCharge::Request>();
    req->uav_id = uav_id_;
    req->battery_level = battery_percent;
    req->role = role_;

    waiting_for_charge_response_ = true;

    RCLCPP_INFO(this->get_logger(),
                "UAV %s: requesting charge (battery=%.1f%%)",
                uav_id_.c_str(), battery_percent);

    auto cb =
      [this](rclcpp::Client<uav_msgs::srv::RequestCharge>::SharedFuture future)
      {
        handleChargeResponse(future);
      };

    charge_client_->async_send_request(req, cb);
  }

  void handleChargeResponse(
    rclcpp::Client<uav_msgs::srv::RequestCharge>::SharedFuture future)
  {
    waiting_for_charge_response_ = false;
    auto res = future.get();

    if (!res->accepted) {
      RCLCPP_WARN(this->get_logger(),
                  "UAV %s: charge request rejected", uav_id_.c_str());
      return;
    }

    charge_start_time_ = rclcpp::Time(res->eta);
    has_charge_slot_ = true;

    auto now = this->now();
    double wait = (charge_start_time_ - now).seconds();

    RCLCPP_INFO(this->get_logger(),
                "UAV %s: charge accepted. Will start charging in %.1f seconds.",
                uav_id_.c_str(), wait);
  }

  // ---------------- Heartbeat & traffic ----------------

  void publishHeartbeat()
  {
    uav_msgs::msg::Heartbeat hb;
    hb.uav_id = uav_id_;
    hb.stamp = this->now();
    heartbeat_pub_->publish(hb);
  }

  void publishTraffic()
  {
    // Do not generate application traffic while charging
    if (is_charging_) {
      return;
    }

    uav_msgs::msg::TrafficMessage msg;
    msg.msg_id = uav_id_ + "_" + std::to_string(msg_counter_++);
    msg.src_id = uav_id_;

    if (role_ == 0) { // MEMBER
      msg.dst_id = my_ch_id_;
    } else { // CH
      msg.dst_id = default_dst_id_;
    }

    msg.msg_type = 0;       // TEXT
    msg.priority = 0;
    msg.size_bytes = 100;
    msg.creation_time = this->now();
    msg.hop_count = 0;

    RCLCPP_INFO(this->get_logger(),
                "[TX] msg_id=%s src=%s dst=%s",
                msg.msg_id.c_str(), msg.src_id.c_str(), msg.dst_id.c_str());

    traffic_pub_->publish(msg);
  }

  void trafficCallback(const uav_msgs::msg::TrafficMessage::SharedPtr msg)
  {
    // Don't route anything while charging
    if (is_charging_) {
      return;
    }

    if (msg->dst_id == uav_id_) {
      // If I am CH and the message came from someone else -> forward to sink
      if (role_ == 1 && msg->src_id != uav_id_) {
        uav_msgs::msg::TrafficMessage fwd = *msg;
        fwd.dst_id = default_dst_id_;
        fwd.hop_count = msg->hop_count + 1;

        RCLCPP_INFO(this->get_logger(),
                    "[FWD] CH %s forwarding msg_id=%s from=%s to=%s",
                    uav_id_.c_str(), msg->msg_id.c_str(),
                    msg->src_id.c_str(), fwd.dst_id.c_str());

        traffic_pub_->publish(fwd);
      } else {
        RCLCPP_INFO(this->get_logger(),
                    "[RX] msg_id=%s delivered to %s (from %s)",
                    msg->msg_id.c_str(), uav_id_.c_str(), msg->src_id.c_str());
      }
    }
  }

  void clusterInfoCallback(const uav_msgs::msg::ClusterInfo::SharedPtr msg)
  {
    for (const auto & id : msg->member_ids) {
      if (id == uav_id_) {
        my_ch_id_ = msg->ch_id;
        cluster_id_ = msg->cluster_id;
        RCLCPP_INFO(this->get_logger(),
                    "UAV %s: cluster=%s CH=%s",
                    uav_id_.c_str(), cluster_id_.c_str(), my_ch_id_.c_str());
        break;
      }
    }
  }

  // ---------------- Members ----------------

  std::string uav_id_;
  uint8_t role_;
  std::string cluster_id_;
  std::string default_dst_id_;
  std::string my_ch_id_;

  geometry_msgs::msg::Pose pose_;
  float service_radius_;

  // Battery model
  float battery_capacity_;          // energy units
  float battery_energy_;            // current energy
  float battery_threshold_percent_; // when to request charge
  double charging_duration_sec_;

  bool waiting_for_charge_response_;
  bool is_charging_;
  bool has_charge_slot_;
  rclcpp::Time charge_start_time_;
  rclcpp::Time charge_end_time_;
  float energy_at_charge_start_;

  uint64_t msg_counter_;

  rclcpp::Publisher<uav_msgs::msg::UavStatus>::SharedPtr status_pub_;
  rclcpp::Publisher<uav_msgs::msg::Heartbeat>::SharedPtr heartbeat_pub_;
  rclcpp::Publisher<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_pub_;

  rclcpp::Subscription<uav_msgs::msg::TrafficMessage>::SharedPtr traffic_sub_;
  rclcpp::Subscription<uav_msgs::msg::ClusterInfo>::SharedPtr   cluster_sub_;

  rclcpp::TimerBase::SharedPtr status_timer_;
  rclcpp::TimerBase::SharedPtr heartbeat_timer_;
  rclcpp::TimerBase::SharedPtr traffic_timer_;

  rclcpp::Client<uav_msgs::srv::RequestCharge>::SharedPtr charge_client_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UavNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
