#include <memory>
#include <cmath>
#include <random>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/weather_status.hpp"

using namespace std::chrono_literals;

// Publishes a synthetic weather stream with a simple regime model.
class WeatherNode : public rclcpp::Node
{
public:
  WeatherNode()
  : Node("weather_node"),
    rng_(std::random_device{}()),
    uni01_(0.0, 1.0)
  {
    base_temp_c_      = this->declare_parameter<double>("base_temperature_c", 22.0);
    update_period_sec_ = this->declare_parameter<double>("update_period_sec", 1.0);

    current_regime_ = Regime::SUNNY;
    wind_direction_deg_ = 0.0;

    // Broadcast weather updates to interested nodes (UAVs, viz, etc.).
    weather_pub_ = this->create_publisher<uav_msgs::msg::WeatherStatus>(
      "/environment/weather", 10);

    auto period = std::chrono::duration<double>(update_period_sec_);
    timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(period),
      std::bind(&WeatherNode::timerCallback, this));

    RCLCPP_INFO(this->get_logger(), "WeatherNode started with probabilistic model");
  }

private:
  enum class Regime { SUNNY = 0, WINDY = 1, STORMY = 2 };

  // Periodically step the regime model and publish the resulting weather.
  void timerCallback()
  {
    stepRegime();

    double temp_c   = 0.0;
    double wind_ms  = 0.0;
    double rain_mm  = 0.0;

    // Choose regime-dependent distributions
    double t_mean, t_std, w_mean, w_std, r_mean, r_std;

    switch (current_regime_) {
      case Regime::SUNNY:
        t_mean = base_temp_c_ + 3.0;  t_std = 2.0;
        w_mean = 2.0;                 w_std = 1.0;
        r_mean = 0.0;                 r_std = 0.05;   // almost no rain
        break;
      case Regime::WINDY:
        t_mean = base_temp_c_;        t_std = 3.0;
        w_mean = 8.0;                 w_std = 3.0;
        r_mean = 0.2;                 r_std = 0.3;    // light drizzle sometimes
        break;
      case Regime::STORMY:
        t_mean = base_temp_c_ - 2.0;  t_std = 3.0;
        w_mean = 12.0;                w_std = 4.0;
        r_mean = 10.0;                r_std = 5.0;    // heavy rain, mm/h
        break;
    }

    temp_c  = sampleNormal(t_mean, t_std);
    wind_ms = std::max(0.0, sampleNormal(w_mean, w_std));
    rain_mm = std::max(0.0, sampleNormal(r_mean, r_std));

    // Wind direction: slow random walk
    wind_direction_deg_ += sampleNormal(0.0, 10.0);  // +-10 deg step
    if (wind_direction_deg_ < 0.0)
      wind_direction_deg_ += 360.0;
    if (wind_direction_deg_ >= 360.0)
      wind_direction_deg_ -= 360.0;

    uav_msgs::msg::WeatherStatus msg;
    msg.temperature_c      = static_cast<float>(temp_c);
    msg.wind_speed         = static_cast<float>(wind_ms);
    msg.rain_intensity     = static_cast<float>(rain_mm);          // mm/h
    msg.wind_direction_deg = static_cast<float>(wind_direction_deg_);

    weather_pub_->publish(msg);
  }

  // Markov-like regime transition: tends to stay in same regime.
  void stepRegime()
  {
    // Simple Markov chain: regimes tend to persist, but can change
    double r = uni01_(rng_);

    switch (current_regime_) {
      case Regime::SUNNY:
        if      (r < 0.85) { /* stay */ }
        else if (r < 0.95) { current_regime_ = Regime::WINDY; }
        else               { current_regime_ = Regime::STORMY; }
        break;
      case Regime::WINDY:
        if      (r < 0.15) { current_regime_ = Regime::SUNNY; }
        else if (r < 0.80) { /* stay */ }
        else               { current_regime_ = Regime::STORMY; }
        break;
      case Regime::STORMY:
        if      (r < 0.40) { current_regime_ = Regime::WINDY; }
        else if (r < 0.80) { /* stay */ }
        else               { current_regime_ = Regime::SUNNY; }
        break;
    }
  }

  // Sample a normal distribution using the node's RNG.
  double sampleNormal(double mean, double stddev)
  {
    std::normal_distribution<double> dist(mean, stddev);
    return dist(rng_);
  }

  // ROS
  rclcpp::Publisher<uav_msgs::msg::WeatherStatus>::SharedPtr weather_pub_;
  rclcpp::TimerBase::SharedPtr timer_;

  // Weather state
  Regime current_regime_;
  double base_temp_c_;
  double update_period_sec_;
  double wind_direction_deg_;

  // RNG
  std::mt19937 rng_;
  std::uniform_real_distribution<double> uni01_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<WeatherNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
