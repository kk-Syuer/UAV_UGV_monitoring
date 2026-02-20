#include <memory>
#include <cmath>
#include <random>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/weather_status.hpp"

using namespace std::chrono_literals;

// Publishes a synthetic weather stream with a simple regime model.
class WeatherNode : public rclcpp::Node
{
public:
  WeatherNode()
  : Node("weather_node"),
    rng_(0),
    uni01_(0.0, 1.0)
  {
    base_temp_c_ = this->declare_parameter<double>("base_temperature_c", 22.0);
    update_period_sec_ = this->declare_parameter<double>("update_period_sec", 1.0);
    // Backward-compatible aliases used in existing launch configs.
    base_temp_c_ = this->declare_parameter<double>("temp_c", base_temp_c_);
    update_period_sec_ = this->declare_parameter<double>("publish_period_sec", update_period_sec_);
    transition_period_sec_ = this->declare_parameter<double>("transition_period_sec", 120.0);

    const auto mode_str = this->declare_parameter<std::string>("mode", "markov");
    const auto start_state_str = this->declare_parameter<std::string>("start_state", "sunny");
    const int64_t seed_param = this->declare_parameter<int64_t>("seed", -1);

    transition_mode_ = parseMode(mode_str);
    if (transition_mode_ == TransitionMode::INVALID) {
      RCLCPP_WARN(
        this->get_logger(),
        "Invalid weather mode '%s'; falling back to mode=markov.",
        mode_str.c_str());
      transition_mode_ = TransitionMode::MARKOV;
    }

    const auto parsed_start_state = parseRegime(start_state_str);
    if (parsed_start_state == Regime::INVALID) {
      RCLCPP_WARN(
        this->get_logger(),
        "Invalid weather start_state '%s'; falling back to default 'sunny'.",
        start_state_str.c_str());
      current_regime_ = Regime::SUNNY;
    } else {
      current_regime_ = parsed_start_state;
    }

    if (seed_param >= 0) {
      seed_value_ = static_cast<uint32_t>(seed_param);
      seed_source_ = "yaml";
    } else {
      seed_value_ = std::random_device{}();
      seed_source_ = "random_device";
    }
    rng_.seed(seed_value_);

    if (transition_period_sec_ <= 0.0) {
      RCLCPP_WARN(this->get_logger(), "Invalid transition_period_sec=%.3f, using 120.0s", transition_period_sec_);
      transition_period_sec_ = 120.0;
    }

    wind_direction_deg_ = 0.0;

    // Broadcast weather updates to interested nodes (UAVs, viz, etc.).
    weather_pub_ = this->create_publisher<uav_msgs::msg::WeatherStatus>(
      "/environment/weather", 10);

    auto period = std::chrono::duration<double>(update_period_sec_);
    timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(period),
      std::bind(&WeatherNode::timerCallback, this));

    if (transition_mode_ == TransitionMode::MARKOV) {
      auto transition_period = std::chrono::duration<double>(transition_period_sec_);
      transition_timer_ = this->create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(transition_period),
        std::bind(&WeatherNode::stepRegime, this));
    }

    const bool transitions_on = (transition_mode_ == TransitionMode::MARKOV);
    RCLCPP_INFO(
      this->get_logger(),
      "mode=%s start_state=%s transition_period_sec=%.1f seed=%u (%s) transitions=%s",
      modeToString(transition_mode_).c_str(),
      regimeToString(current_regime_).c_str(),
      transition_period_sec_,
      seed_value_,
      seed_source_.c_str(),
      transitions_on ? "ON" : "OFF");
  }

private:
  enum class Regime { SUNNY = 0, WINDY = 1, STORMY = 2, INVALID = 255 };
  enum class TransitionMode { MARKOV = 0, FIXED = 1, INVALID = 255 };

  static std::string normalize(std::string s)
  {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
    if (s.size() >= 2 && ((s.front() == '\'' && s.back() == '\'') || (s.front() == '"' && s.back() == '"'))) {
      s = s.substr(1, s.size() - 2);
    }
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
  }

  static Regime parseRegime(const std::string & regime)
  {
    const auto val = normalize(regime);
    if (val == "sunny") {
      return Regime::SUNNY;
    }
    if (val == "windy") {
      return Regime::WINDY;
    }
    if (val == "stormy") {
      return Regime::STORMY;
    }
    return Regime::INVALID;
  }

  static TransitionMode parseMode(const std::string & mode)
  {
    const auto val = normalize(mode);
    if (val == "markov") {
      return TransitionMode::MARKOV;
    }
    if (val == "fixed") {
      return TransitionMode::FIXED;
    }
    return TransitionMode::INVALID;
  }

  static std::string regimeToString(Regime regime)
  {
    switch (regime) {
      case Regime::SUNNY: return "sunny";
      case Regime::WINDY: return "windy";
      case Regime::STORMY: return "stormy";
      default: return "invalid";
    }
  }

  static std::string modeToString(TransitionMode mode)
  {
    switch (mode) {
      case TransitionMode::MARKOV: return "markov";
      case TransitionMode::FIXED: return "fixed";
      default: return "invalid";
    }
  }

  // Periodically step the regime model and publish the resulting weather.
  void timerCallback()
  {
    double temp_c = 0.0;
    double wind_ms = 0.0;
    double rain_mm = 0.0;

    // Choose regime-dependent distributions
    double t_mean, t_std, w_mean, w_std, r_mean, r_std;

    switch (current_regime_) {
      case Regime::SUNNY:
        t_mean = base_temp_c_ + 3.0;
        t_std = 2.0;
        w_mean = 2.0;
        w_std = 1.0;
        r_mean = 0.0;
        r_std = 0.05;  // almost no rain
        break;
      case Regime::WINDY:
        t_mean = base_temp_c_;
        t_std = 3.0;
        w_mean = 8.0;
        w_std = 3.0;
        r_mean = 0.2;
        r_std = 0.3;  // light drizzle sometimes
        break;
      case Regime::STORMY:
        t_mean = base_temp_c_ - 2.0;
        t_std = 3.0;
        w_mean = 12.0;
        w_std = 4.0;
        r_mean = 10.0;
        r_std = 5.0;  // heavy rain, mm/h
        break;
      default:
        t_mean = base_temp_c_ + 3.0;
        t_std = 2.0;
        w_mean = 2.0;
        w_std = 1.0;
        r_mean = 0.0;
        r_std = 0.05;
        break;
    }

    temp_c = sampleNormal(t_mean, t_std);
    wind_ms = std::max(0.0, sampleNormal(w_mean, w_std));
    rain_mm = std::max(0.0, sampleNormal(r_mean, r_std));

    // Wind direction: slow random walk
    wind_direction_deg_ += sampleNormal(0.0, 10.0);  // +-10 deg step
    if (wind_direction_deg_ < 0.0)
      wind_direction_deg_ += 360.0;
    if (wind_direction_deg_ >= 360.0)
      wind_direction_deg_ -= 360.0;

    uav_msgs::msg::WeatherStatus msg;
    msg.temperature_c = static_cast<float>(temp_c);
    msg.wind_speed = static_cast<float>(wind_ms);
    msg.rain_intensity = static_cast<float>(rain_mm);  // mm/h
    msg.wind_direction_deg = static_cast<float>(wind_direction_deg_);
    msg.regime = regimeToString(current_regime_);

    weather_pub_->publish(msg);

  }

  // Markov-like regime transition: tends to stay in same regime.
  void stepRegime()
  {
    // Simple Markov chain: regimes tend to persist, but can change
    const double r = uni01_(rng_);
    const auto before = current_regime_;
    double transition_prob = 0.0;

    switch (current_regime_) {
      case Regime::SUNNY:
        if (r < 0.85) {
          /* stay */
        } else if (r < 0.95) {
          current_regime_ = Regime::WINDY;
          transition_prob = 0.10;
        } else {
          current_regime_ = Regime::STORMY;
          transition_prob = 0.05;
        }
        break;
      case Regime::WINDY:
        if (r < 0.15) {
          current_regime_ = Regime::SUNNY;
          transition_prob = 0.15;
        } else if (r < 0.80) {
          /* stay */
        } else {
          current_regime_ = Regime::STORMY;
          transition_prob = 0.20;
        }
        break;
      case Regime::STORMY:
        if (r < 0.40) {
          current_regime_ = Regime::WINDY;
          transition_prob = 0.40;
        } else if (r < 0.80) {
          /* stay */
        } else {
          current_regime_ = Regime::SUNNY;
          transition_prob = 0.20;
        }
        break;
      default:
        current_regime_ = Regime::SUNNY;
        break;
    }

    if (current_regime_ != before) {
      RCLCPP_INFO(
        this->get_logger(),
        "transition %s -> %s (p=%.2f, rand=%.4f)",
        regimeToString(before).c_str(),
        regimeToString(current_regime_).c_str(),
        transition_prob,
        r);
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
  rclcpp::TimerBase::SharedPtr transition_timer_;

  // Weather state
  Regime current_regime_;
  TransitionMode transition_mode_;
  double base_temp_c_;
  double update_period_sec_;
  double transition_period_sec_;
  double wind_direction_deg_;
  rclcpp::Time last_transition_time_;
  uint32_t seed_value_;
  std::string seed_source_;

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
