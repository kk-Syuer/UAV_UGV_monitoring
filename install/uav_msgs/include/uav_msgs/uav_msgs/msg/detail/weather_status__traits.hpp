// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from uav_msgs:msg/WeatherStatus.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__TRAITS_HPP_
#define UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "uav_msgs/msg/detail/weather_status__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace uav_msgs
{

namespace msg
{

inline void to_flow_style_yaml(
  const WeatherStatus & msg,
  std::ostream & out)
{
  out << "{";
  // member: rain_intensity
  {
    out << "rain_intensity: ";
    rosidl_generator_traits::value_to_yaml(msg.rain_intensity, out);
    out << ", ";
  }

  // member: wind_speed
  {
    out << "wind_speed: ";
    rosidl_generator_traits::value_to_yaml(msg.wind_speed, out);
    out << ", ";
  }

  // member: wind_direction_deg
  {
    out << "wind_direction_deg: ";
    rosidl_generator_traits::value_to_yaml(msg.wind_direction_deg, out);
    out << ", ";
  }

  // member: temperature_c
  {
    out << "temperature_c: ";
    rosidl_generator_traits::value_to_yaml(msg.temperature_c, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const WeatherStatus & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: rain_intensity
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "rain_intensity: ";
    rosidl_generator_traits::value_to_yaml(msg.rain_intensity, out);
    out << "\n";
  }

  // member: wind_speed
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "wind_speed: ";
    rosidl_generator_traits::value_to_yaml(msg.wind_speed, out);
    out << "\n";
  }

  // member: wind_direction_deg
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "wind_direction_deg: ";
    rosidl_generator_traits::value_to_yaml(msg.wind_direction_deg, out);
    out << "\n";
  }

  // member: temperature_c
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "temperature_c: ";
    rosidl_generator_traits::value_to_yaml(msg.temperature_c, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const WeatherStatus & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace msg

}  // namespace uav_msgs

namespace rosidl_generator_traits
{

[[deprecated("use uav_msgs::msg::to_block_style_yaml() instead")]]
inline void to_yaml(
  const uav_msgs::msg::WeatherStatus & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::msg::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::msg::WeatherStatus & msg)
{
  return uav_msgs::msg::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::msg::WeatherStatus>()
{
  return "uav_msgs::msg::WeatherStatus";
}

template<>
inline const char * name<uav_msgs::msg::WeatherStatus>()
{
  return "uav_msgs/msg/WeatherStatus";
}

template<>
struct has_fixed_size<uav_msgs::msg::WeatherStatus>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<uav_msgs::msg::WeatherStatus>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<uav_msgs::msg::WeatherStatus>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__TRAITS_HPP_
