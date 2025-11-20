#include <memory>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "uav_msgs/msg/weather_status.hpp"

using namespace std::chrono_literals;

class WeatherNode : public rclcpp::Node
{
public:
  WeatherNode()
  : Node("weather_node")
  {
    base_temp_c_ = this->declare_parameter<double>("base_temperature_c", 22.0);
    temp_amplitude_c_ = this->declare_parameter<double>("temp_amplitude_c", 13.0);
    temp_period_sec_ = this->declare_parameter<double>("temp_period_sec", 600.0);  // 10 minutes

    weather_pub_ = this->create_publisher<uav_msgs::msg::WeatherStatus>(
      "/environment/weather", 10);

    timer_ = this->create_wall_timer(
      1s, std::bind(&WeatherNode::publishWeather, this));

    RCLCPP_INFO(this->get_logger(),
                "Weather server started (base=%.1fC, amp=%.1fC, period=%.1fs)",
                base_temp_c_, temp_amplitude_c_, temp_period_sec_);
  }

private:
  void publishWeather()
  {
    auto now = this->now();
    double t = now.seconds();

    double phase = (temp_period_sec_ > 0.0)
                     ? (2.0 * M_PI * t / temp_period_sec_)
                     : 0.0;
    float temp_c = static_cast<float>(base_temp_c_ +
                                      temp_amplitude_c_ * std::sin(phase));

    uav_msgs::msg::WeatherStatus msg;
    msg.rain_intensity = 0.0f;        // you can randomize later
    msg.wind_speed = 2.0f;            // m/s
    msg.wind_direction_deg = 90.0f;   // east
    msg.temperature_c = temp_c;

    weather_pub_->publish(msg);
  }

  rclcpp::Publisher<uav_msgs::msg::WeatherStatus>::SharedPtr weather_pub_;
  rclcpp::TimerBase::SharedPtr timer_;

  double base_temp_c_;
  double temp_amplitude_c_;
  double temp_period_sec_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<WeatherNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
