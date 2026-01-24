#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <random>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/traffic_message.hpp"
#include "uav_msgs/msg/weather_status.hpp"

using uav_msgs::msg::TrafficMessage;
using uav_msgs::msg::WeatherStatus;

class FaultInjectorNode : public rclcpp::Node
{
public:
  FaultInjectorNode()
  : Node("fault_injector_node"),
    rng_(std::random_device{}())
  {
    injector_id_ = this->declare_parameter<std::string>("injector_id", "fault_injector");
    monitor_id_ = this->declare_parameter<std::string>("monitor_id", "network_monitor");

    enabled_ = this->declare_parameter<bool>("enabled", true);
    drop_delivered_ = this->declare_parameter<bool>("drop_delivered", false);

    p0_ = this->declare_parameter<double>("p0", 0.0);
    aw_ = this->declare_parameter<double>("aw", 0.15);
    ar_ = this->declare_parameter<double>("ar", 0.15);
    at_ = this->declare_parameter<double>("at", 0.15);
    p_max_ = this->declare_parameter<double>("p_max", 0.8);
    wind_max_ = this->declare_parameter<double>("wind_max", 20.0);
    rain_max_ = this->declare_parameter<double>("rain_max", 50.0);
    temp_opt_ = this->declare_parameter<double>("temp_opt", 22.0);
    temp_span_ = this->declare_parameter<double>("temp_span", 20.0);
    drop_control_multiplier_ = this->declare_parameter<double>("drop_control_multiplier", 1.0);
    drop_data_multiplier_ = this->declare_parameter<double>("drop_data_multiplier", 1.0);

    network_bus_pub_ = this->create_publisher<TrafficMessage>("/fanet/network_bus", 200);
    if (drop_delivered_) {
      delivered_pub_ = this->create_publisher<TrafficMessage>("/fanet/delivered", 200);
    }

    network_bus_raw_sub_ = this->create_subscription<TrafficMessage>(
      "/fanet/network_bus_raw", 200,
      [this](TrafficMessage::SharedPtr msg)
      {
        this->trafficCallback(msg, false);
      });

    if (drop_delivered_) {
      delivered_raw_sub_ = this->create_subscription<TrafficMessage>(
        "/fanet/delivered_raw", 200,
        [this](TrafficMessage::SharedPtr msg)
        {
          this->trafficCallback(msg, true);
        });
    }

    weather_sub_ = this->create_subscription<WeatherStatus>(
      "/environment/weather", 10,
      std::bind(&FaultInjectorNode::weatherCallback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(),
                "Fault injector started (enabled=%s, drop_delivered=%s)",
                enabled_ ? "true" : "false",
                drop_delivered_ ? "true" : "false");
  }

private:
  struct WeatherState
  {
    double wind{0.0};
    double rain{0.0};
    double temp{22.0};
  };

  static double clamp(double v, double lo, double hi)
  {
    return std::max(lo, std::min(v, hi));
  }

  void weatherCallback(const WeatherStatus::SharedPtr msg)
  {
    weather_.wind = msg->wind_speed;
    weather_.rain = msg->rain_intensity;
    weather_.temp = msg->temperature_c;
  }

  void trafficCallback(const TrafficMessage::SharedPtr msg, bool is_delivered_stream)
  {
    if (!msg) {
      return;
    }

    if (!enabled_ || msg->control_type == "FAILURE_EVENT") {
      forward(*msg, is_delivered_stream);
      return;
    }

    double p = computeDropProbability(msg->flow_type == 1);
    if (p <= 0.0) {
      forward(*msg, is_delivered_stream);
      return;
    }

    double sample = uniform01_(rng_);
    if (sample < p) {
      publishDropReport(*msg);
      RCLCPP_WARN(this->get_logger(),
                  "[DROP] weather drop for msg_id=%s (flow=%u ctrl=%s) p=%.3f sample=%.3f",
                  msg->msg_id.c_str(), msg->flow_type, msg->control_type.c_str(), p, sample);
      return;
    }

    forward(*msg, is_delivered_stream);
  }

  void forward(const TrafficMessage & msg, bool is_delivered_stream)
  {
    if (is_delivered_stream) {
      if (delivered_pub_) {
        delivered_pub_->publish(msg);
      }
      return;
    }
    network_bus_pub_->publish(msg);
  }

  double computeDropProbability(bool is_control) const
  {
    double wind_term = 0.0;
    if (wind_max_ > 0.0) {
      double ratio = weather_.wind / wind_max_;
      wind_term = aw_ * ratio * ratio;
    }

    double rain_term = 0.0;
    if (rain_max_ > 0.0) {
      double ratio = weather_.rain / rain_max_;
      rain_term = ar_ * ratio * ratio;
    }

    double temp_term = 0.0;
    if (temp_span_ > 0.0) {
      temp_term = at_ * (std::abs(weather_.temp - temp_opt_) / temp_span_);
    }

    double base_p = clamp(p0_ + wind_term + rain_term + temp_term, 0.0, p_max_);
    double scaled = base_p * (is_control ? drop_control_multiplier_ : drop_data_multiplier_);
    return clamp(scaled, 0.0, p_max_);
  }

  void publishDropReport(const TrafficMessage & msg)
  {
    TrafficMessage drop;
    drop.msg_id = msg.msg_id + "_DROP_" + injector_id_;
    drop.src_id = injector_id_;
    drop.dst_id = monitor_id_;
    drop.flow_type = 1;
    drop.control_type = "DROP";
    drop.payload = msg.msg_id + ",WEATHER_DROP";
    drop.creation_time = this->now();
    drop.hop_count = 0;
    drop.ttl = 4;
    drop.next_hop_id = "";

    network_bus_pub_->publish(drop);
  }

  // Params
  bool enabled_{true};
  bool drop_delivered_{false};
  double p0_{0.0};
  double aw_{0.0};
  double ar_{0.0};
  double at_{0.0};
  double p_max_{1.0};
  double wind_max_{1.0};
  double rain_max_{1.0};
  double temp_opt_{22.0};
  double temp_span_{1.0};
  double drop_control_multiplier_{1.0};
  double drop_data_multiplier_{1.0};
  std::string injector_id_;
  std::string monitor_id_;

  WeatherState weather_;
  std::mt19937 rng_;
  mutable std::uniform_real_distribution<double> uniform01_{0.0, 1.0};

  rclcpp::Publisher<TrafficMessage>::SharedPtr network_bus_pub_;
  rclcpp::Publisher<TrafficMessage>::SharedPtr delivered_pub_;
  rclcpp::Subscription<TrafficMessage>::SharedPtr network_bus_raw_sub_;
  rclcpp::Subscription<TrafficMessage>::SharedPtr delivered_raw_sub_;
  rclcpp::Subscription<WeatherStatus>::SharedPtr weather_sub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<FaultInjectorNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
