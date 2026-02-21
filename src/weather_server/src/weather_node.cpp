#include <memory>
#include <cmath>
#include <random>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/weather_status.hpp"

using namespace std::chrono_literals;

// Publishes a synthetic weather stream with a Markov regime model.
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
    const auto start_state_str = this->declare_parameter<std::string>("start_state", "cloudy");
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
        "Invalid weather start_state '%s'; falling back to default 'cloudy'.",
        start_state_str.c_str());
      current_regime_ = Regime::CLOUDY;
    } else {
      current_regime_ = parsed_start_state;
    }

    transition_matrix_ = floodDefaultMatrix();
    transition_matrix_source_ = "default";
    loadTransitionMatrix();

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
      "mode=%s start_state=%s transition_period_sec=%.1f states=[sunny,cloudy,windy,rainy,stormy] matrix_source=%s seed=%u (%s) transitions=%s",
      modeToString(transition_mode_).c_str(),
      regimeToString(current_regime_).c_str(),
      transition_period_sec_,
      transition_matrix_source_.c_str(),
      seed_value_,
      seed_source_.c_str(),
      transitions_on ? "ON" : "OFF");
  }

private:
  enum class Regime { SUNNY = 0, CLOUDY = 1, WINDY = 2, RAINY = 3, STORMY = 4, INVALID = 255 };
  enum class TransitionMode { MARKOV = 0, FIXED = 1, INVALID = 255 };

  static constexpr size_t kRegimeCount = 5;
  using TransitionRow = std::array<double, kRegimeCount>;
  using TransitionMatrix = std::array<TransitionRow, kRegimeCount>;

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

  static const std::array<Regime, kRegimeCount> & regimeOrder()
  {
    static const std::array<Regime, kRegimeCount> order = {
      Regime::SUNNY, Regime::CLOUDY, Regime::WINDY, Regime::RAINY, Regime::STORMY};
    return order;
  }

  static size_t regimeIndex(Regime regime)
  {
    switch (regime) {
      case Regime::SUNNY: return 0;
      case Regime::CLOUDY: return 1;
      case Regime::WINDY: return 2;
      case Regime::RAINY: return 3;
      case Regime::STORMY: return 4;
      default: return 0;
    }
  }

  static TransitionMatrix floodDefaultMatrix()
  {
    return TransitionMatrix{{
      TransitionRow{{0.60, 0.25, 0.10, 0.04, 0.01}},
      TransitionRow{{0.08, 0.55, 0.10, 0.25, 0.02}},
      TransitionRow{{0.06, 0.35, 0.40, 0.15, 0.04}},
      TransitionRow{{0.02, 0.20, 0.10, 0.50, 0.18}},
      TransitionRow{{0.01, 0.10, 0.14, 0.35, 0.40}},
    }};
  }

  static Regime parseRegime(const std::string & regime)
  {
    const auto val = normalize(regime);
    if (val == "sunny") {
      return Regime::SUNNY;
    }
    if (val == "cloudy") {
      return Regime::CLOUDY;
    }
    if (val == "windy") {
      return Regime::WINDY;
    }
    if (val == "rainy") {
      return Regime::RAINY;
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
      case Regime::CLOUDY: return "cloudy";
      case Regime::WINDY: return "windy";
      case Regime::RAINY: return "rainy";
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

  void loadTransitionMatrix()
  {
    const bool matrix_from_yaml = this->declare_parameter<bool>("transition_matrix_from_yaml", false);
    if (!matrix_from_yaml) {
      return;
    }

    transition_matrix_source_ = "yaml";
    const auto & regimes = regimeOrder();
    for (size_t i = 0; i < kRegimeCount; ++i) {
      for (size_t j = 0; j < kRegimeCount; ++j) {
        const auto key = "transition_matrix." + regimeToString(regimes[i]) + "." + regimeToString(regimes[j]);
        transition_matrix_[i][j] = this->declare_parameter<double>(key, transition_matrix_[i][j]);
      }
      normalizeTransitionRow(i);
    }
  }

  void normalizeTransitionRow(size_t row)
  {
    const auto row_name = regimeToString(regimeOrder()[row]);
    double sum = 0.0;
    for (double p : transition_matrix_[row]) {
      sum += std::max(0.0, p);
    }

    if (sum <= 0.0) {
      RCLCPP_WARN(this->get_logger(), "transition_matrix row=%s sums to <=0; using flood-default row", row_name.c_str());
      transition_matrix_[row] = floodDefaultMatrix()[row];
      return;
    }

    if (std::abs(sum - 1.0) > 1e-6) {
      RCLCPP_WARN(this->get_logger(), "transition_matrix row=%s sums to %.6f; normalizing to 1.0", row_name.c_str(), sum);
    }

    for (double & p : transition_matrix_[row]) {
      p = std::max(0.0, p) / sum;
    }
  }

  // Periodically sample and publish weather values from current regime.
  void timerCallback()
  {
    double temp_c = 0.0;
    double wind_ms = 0.0;
    double rain_mm = 0.0;

    double t_mean, t_std, w_mean, w_std, r_mean, r_std;

    switch (current_regime_) {
      case Regime::SUNNY:
        t_mean = base_temp_c_ + 3.0;
        t_std = 2.0;
        w_mean = 2.0;
        w_std = 1.0;
        r_mean = 0.0;
        r_std = 0.05;
        break;
      case Regime::CLOUDY:
        t_mean = base_temp_c_ + 1.0;
        t_std = 2.5;
        w_mean = 4.0;
        w_std = 1.8;
        r_mean = 0.8;
        r_std = 0.7;
        break;
      case Regime::WINDY:
        t_mean = base_temp_c_;
        t_std = 3.0;
        w_mean = 8.0;
        w_std = 3.0;
        r_mean = 0.2;
        r_std = 0.3;
        break;
      case Regime::RAINY:
        t_mean = base_temp_c_ - 1.0;
        t_std = 2.0;
        w_mean = 6.0;
        w_std = 2.5;
        r_mean = 5.0;
        r_std = 2.0;
        break;
      case Regime::STORMY:
        t_mean = base_temp_c_ - 2.0;
        t_std = 3.0;
        w_mean = 12.0;
        w_std = 4.0;
        r_mean = 10.0;
        r_std = 5.0;
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

    wind_direction_deg_ += sampleNormal(0.0, 10.0);
    if (wind_direction_deg_ < 0.0) {
      wind_direction_deg_ += 360.0;
    }
    if (wind_direction_deg_ >= 360.0) {
      wind_direction_deg_ -= 360.0;
    }

    uav_msgs::msg::WeatherStatus msg;
    msg.temperature_c = static_cast<float>(temp_c);
    msg.wind_speed = static_cast<float>(wind_ms);
    msg.rain_intensity = static_cast<float>(rain_mm);
    msg.wind_direction_deg = static_cast<float>(wind_direction_deg_);
    msg.regime = regimeToString(current_regime_);

    weather_pub_->publish(msg);
  }

  void stepRegime()
  {
    const double r = uni01_(rng_);
    const auto before = current_regime_;
    const auto & row = transition_matrix_[regimeIndex(before)];

    double cumulative = 0.0;
    double transition_prob = row.back();
    current_regime_ = regimeOrder().back();

    for (size_t idx = 0; idx < kRegimeCount; ++idx) {
      cumulative += row[idx];
      if (r <= cumulative || idx == kRegimeCount - 1) {
        current_regime_ = regimeOrder()[idx];
        transition_prob = row[idx];
        break;
      }
    }

    if (current_regime_ != before) {
      RCLCPP_INFO(
        this->get_logger(),
        "transition %s -> %s (p=%.2f, rand=%.4f, t=%.3f)",
        regimeToString(before).c_str(),
        regimeToString(current_regime_).c_str(),
        transition_prob,
        r,
        this->now().seconds());
    }
  }

  double sampleNormal(double mean, double stddev)
  {
    std::normal_distribution<double> dist(mean, stddev);
    return dist(rng_);
  }

  rclcpp::Publisher<uav_msgs::msg::WeatherStatus>::SharedPtr weather_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::TimerBase::SharedPtr transition_timer_;

  Regime current_regime_;
  TransitionMode transition_mode_;
  double base_temp_c_;
  double update_period_sec_;
  double transition_period_sec_;
  double wind_direction_deg_;
  TransitionMatrix transition_matrix_;
  std::string transition_matrix_source_;
  uint32_t seed_value_;
  std::string seed_source_;

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
