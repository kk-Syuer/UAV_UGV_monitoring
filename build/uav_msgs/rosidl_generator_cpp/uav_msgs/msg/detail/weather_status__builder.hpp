// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from uav_msgs:msg/WeatherStatus.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__BUILDER_HPP_
#define UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "uav_msgs/msg/detail/weather_status__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace uav_msgs
{

namespace msg
{

namespace builder
{

class Init_WeatherStatus_temperature_c
{
public:
  explicit Init_WeatherStatus_temperature_c(::uav_msgs::msg::WeatherStatus & msg)
  : msg_(msg)
  {}
  ::uav_msgs::msg::WeatherStatus temperature_c(::uav_msgs::msg::WeatherStatus::_temperature_c_type arg)
  {
    msg_.temperature_c = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::msg::WeatherStatus msg_;
};

class Init_WeatherStatus_wind_direction_deg
{
public:
  explicit Init_WeatherStatus_wind_direction_deg(::uav_msgs::msg::WeatherStatus & msg)
  : msg_(msg)
  {}
  Init_WeatherStatus_temperature_c wind_direction_deg(::uav_msgs::msg::WeatherStatus::_wind_direction_deg_type arg)
  {
    msg_.wind_direction_deg = std::move(arg);
    return Init_WeatherStatus_temperature_c(msg_);
  }

private:
  ::uav_msgs::msg::WeatherStatus msg_;
};

class Init_WeatherStatus_wind_speed
{
public:
  explicit Init_WeatherStatus_wind_speed(::uav_msgs::msg::WeatherStatus & msg)
  : msg_(msg)
  {}
  Init_WeatherStatus_wind_direction_deg wind_speed(::uav_msgs::msg::WeatherStatus::_wind_speed_type arg)
  {
    msg_.wind_speed = std::move(arg);
    return Init_WeatherStatus_wind_direction_deg(msg_);
  }

private:
  ::uav_msgs::msg::WeatherStatus msg_;
};

class Init_WeatherStatus_rain_intensity
{
public:
  Init_WeatherStatus_rain_intensity()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_WeatherStatus_wind_speed rain_intensity(::uav_msgs::msg::WeatherStatus::_rain_intensity_type arg)
  {
    msg_.rain_intensity = std::move(arg);
    return Init_WeatherStatus_wind_speed(msg_);
  }

private:
  ::uav_msgs::msg::WeatherStatus msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::msg::WeatherStatus>()
{
  return uav_msgs::msg::builder::Init_WeatherStatus_rain_intensity();
}

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__BUILDER_HPP_
