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
//
// Two independent timers:
//   1. Parameter update timer (default 1 s) — applies a mean-reverting drift
//      model to temperature, wind speed, rainfall, and wind direction, then
//      publishes.  The drift targets and variation amplitudes are determined
//      by the current macrostate.
//   2. Macrostate timer (default 30 s) — evaluates the Markov transition
//      matrix and potentially switches the regime.  The macrostate never
//      changes on the fast timer.
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
    macrostate_period_sec_ = this->declare_parameter<double>("macrostate_period_sec", 30.0);
    // Legacy alias — maps to the new macrostate timer.
    macrostate_period_sec_ = this->declare_parameter<double>("transition_period_sec", macrostate_period_sec_);

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

    if (macrostate_period_sec_ <= 0.0) {
      RCLCPP_WARN(this->get_logger(), "Invalid macrostate_period_sec=%.3f, using 30.0s", macrostate_period_sec_);
      macrostate_period_sec_ = 30.0;
    }

    // Initialise live weather values from the starting regime so the first
    // published message is already in the right ballpark.
    initLiveValues();

    weather_pub_ = this->create_publisher<uav_msgs::msg::WeatherStatus>(
      "/environment/weather", 10);

    // Fast timer: drift + publish weather parameters every update_period_sec_.
    auto period = std::chrono::duration<double>(update_period_sec_);
    update_timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(period),
      std::bind(&WeatherNode::updateCallback, this));

    // Slow timer: Markov macrostate transitions.
    if (transition_mode_ == TransitionMode::MARKOV) {
      auto macro_period = std::chrono::duration<double>(macrostate_period_sec_);
      macrostate_timer_ = this->create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(macro_period),
        std::bind(&WeatherNode::stepRegime, this));
    }

    const bool transitions_on = (transition_mode_ == TransitionMode::MARKOV);
    RCLCPP_INFO(
      this->get_logger(),
      "mode=%s start_state=%s update_period=%.1fs macrostate_period=%.1fs "
      "states=[sunny,cloudy,windy,rainy,stormy] matrix_source=%s seed=%u (%s) transitions=%s",
      modeToString(transition_mode_).c_str(),
      regimeToString(current_regime_).c_str(),
      update_period_sec_,
      macrostate_period_sec_,
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

  // Per-regime parameters that govern the drift model.
  //   centre   — the value that the parameter mean-reverts toward
  //   jitter   — standard deviation of the noise added each tick
  //   drift    — fraction of the gap (centre − current) closed per tick (0..1)
  struct RegimeParams
  {
    double temp_centre;
    double temp_jitter;
    double wind_centre;
    double wind_jitter;
    double rain_centre;
    double rain_jitter;
    double drift_rate;          // mean-reversion strength (higher = faster snap)
    double wind_dir_jitter_deg; // std of wind-direction random walk per tick
  };

  // Returns the drift-model parameters for a given regime.
  RegimeParams regimeParams(Regime regime) const
  {
    switch (regime) {
      case Regime::SUNNY:
        return {base_temp_c_ + 3.0, 0.3,   // temp: warm, low jitter
                2.0,                0.2,    // wind: light
                0.0,                0.02,   // rain: essentially none
                0.15,                       // drift: moderate convergence
                2.0};                       // wind dir: stable
      case Regime::CLOUDY:
        return {base_temp_c_ + 1.0, 0.4,
                4.0,                0.4,
                0.8,                0.15,
                0.12,
                3.0};
      case Regime::WINDY:
        return {base_temp_c_,       0.5,
                8.0,                0.8,
                0.2,                0.08,
                0.10,
                6.0};                       // wind dir: gusty shifts
      case Regime::RAINY:
        return {base_temp_c_ - 1.0, 0.35,
                6.0,                0.6,
                5.0,                0.5,
                0.12,
                4.0};
      case Regime::STORMY:
        return {base_temp_c_ - 2.0, 0.7,
                12.0,               1.2,
                10.0,               1.0,
                0.08,                       // drift: slow — stormy values swing widely
                8.0};                       // wind dir: chaotic
      default:
        return {base_temp_c_ + 3.0, 0.3,
                2.0,                0.2,
                0.0,                0.02,
                0.15,
                2.0};
    }
  }

  // Seed the live values from the current regime's centres so the very first
  // publish isn't starting from zero.
  void initLiveValues()
  {
    const auto rp = regimeParams(current_regime_);
    live_temp_c_          = rp.temp_centre;
    live_wind_ms_         = rp.wind_centre;
    live_rain_mm_         = rp.rain_centre;
    live_wind_dir_deg_    = sampleUniform(0.0, 360.0);
  }

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

  // ── Fast timer callback (every update_period_sec_) ──────────────────
  // Applies a mean-reverting drift model:
  //   new = old + drift * (centre − old) + jitter * N(0,1)
  // The regime is NOT changed here — only the numeric parameters evolve.
  void updateCallback()
  {
    const auto rp = regimeParams(current_regime_);

    // Mean-reverting drift for each parameter.
    live_temp_c_ += rp.drift_rate * (rp.temp_centre - live_temp_c_)
                  + rp.temp_jitter * sampleNormal(0.0, 1.0);

    live_wind_ms_ += rp.drift_rate * (rp.wind_centre - live_wind_ms_)
                   + rp.wind_jitter * sampleNormal(0.0, 1.0);
    live_wind_ms_ = std::max(0.0, live_wind_ms_);

    live_rain_mm_ += rp.drift_rate * (rp.rain_centre - live_rain_mm_)
                   + rp.rain_jitter * sampleNormal(0.0, 1.0);
    live_rain_mm_ = std::max(0.0, live_rain_mm_);

    // Wind direction: random walk with regime-dependent jitter.
    live_wind_dir_deg_ += sampleNormal(0.0, rp.wind_dir_jitter_deg);
    if (live_wind_dir_deg_ < 0.0) {
      live_wind_dir_deg_ += 360.0;
    }
    if (live_wind_dir_deg_ >= 360.0) {
      live_wind_dir_deg_ -= 360.0;
    }

    uav_msgs::msg::WeatherStatus msg;
    msg.temperature_c    = static_cast<float>(live_temp_c_);
    msg.wind_speed       = static_cast<float>(live_wind_ms_);
    msg.rain_intensity   = static_cast<float>(live_rain_mm_);
    msg.wind_direction_deg = static_cast<float>(live_wind_dir_deg_);
    msg.regime           = regimeToString(current_regime_);

    weather_pub_->publish(msg);
  }

  // ── Slow timer callback (every macrostate_period_sec_) ──────────────
  // Evaluates the Markov transition matrix and potentially switches regime.
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
        "regime transition %s -> %s (p=%.2f, rand=%.4f, t=%.3f)",
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

  double sampleUniform(double lo, double hi)
  {
    std::uniform_real_distribution<double> dist(lo, hi);
    return dist(rng_);
  }

  rclcpp::Publisher<uav_msgs::msg::WeatherStatus>::SharedPtr weather_pub_;
  rclcpp::TimerBase::SharedPtr update_timer_;
  rclcpp::TimerBase::SharedPtr macrostate_timer_;

  Regime current_regime_;
  TransitionMode transition_mode_;
  double base_temp_c_;
  double update_period_sec_;
  double macrostate_period_sec_;
  TransitionMatrix transition_matrix_;
  std::string transition_matrix_source_;
  uint32_t seed_value_;
  std::string seed_source_;

  // Live (persistent) weather values — evolved by the drift model each tick.
  double live_temp_c_{0.0};
  double live_wind_ms_{0.0};
  double live_rain_mm_{0.0};
  double live_wind_dir_deg_{0.0};

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
