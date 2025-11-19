// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from uav_msgs:msg/Heartbeat.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__HEARTBEAT__TRAITS_HPP_
#define UAV_MSGS__MSG__DETAIL__HEARTBEAT__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "uav_msgs/msg/detail/heartbeat__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__traits.hpp"

namespace uav_msgs
{

namespace msg
{

inline void to_flow_style_yaml(
  const Heartbeat & msg,
  std::ostream & out)
{
  out << "{";
  // member: uav_id
  {
    out << "uav_id: ";
    rosidl_generator_traits::value_to_yaml(msg.uav_id, out);
    out << ", ";
  }

  // member: stamp
  {
    out << "stamp: ";
    to_flow_style_yaml(msg.stamp, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const Heartbeat & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: uav_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "uav_id: ";
    rosidl_generator_traits::value_to_yaml(msg.uav_id, out);
    out << "\n";
  }

  // member: stamp
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "stamp:\n";
    to_block_style_yaml(msg.stamp, out, indentation + 2);
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const Heartbeat & msg, bool use_flow_style = false)
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
  const uav_msgs::msg::Heartbeat & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::msg::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::msg::Heartbeat & msg)
{
  return uav_msgs::msg::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::msg::Heartbeat>()
{
  return "uav_msgs::msg::Heartbeat";
}

template<>
inline const char * name<uav_msgs::msg::Heartbeat>()
{
  return "uav_msgs/msg/Heartbeat";
}

template<>
struct has_fixed_size<uav_msgs::msg::Heartbeat>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<uav_msgs::msg::Heartbeat>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<uav_msgs::msg::Heartbeat>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // UAV_MSGS__MSG__DETAIL__HEARTBEAT__TRAITS_HPP_
