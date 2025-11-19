// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from uav_msgs:msg/ChargeDecision.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__TRAITS_HPP_
#define UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "uav_msgs/msg/detail/charge_decision__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'slot_start_time'
#include "builtin_interfaces/msg/detail/time__traits.hpp"

namespace uav_msgs
{

namespace msg
{

inline void to_flow_style_yaml(
  const ChargeDecision & msg,
  std::ostream & out)
{
  out << "{";
  // member: uav_id
  {
    out << "uav_id: ";
    rosidl_generator_traits::value_to_yaml(msg.uav_id, out);
    out << ", ";
  }

  // member: accepted
  {
    out << "accepted: ";
    rosidl_generator_traits::value_to_yaml(msg.accepted, out);
    out << ", ";
  }

  // member: slot_start_time
  {
    out << "slot_start_time: ";
    to_flow_style_yaml(msg.slot_start_time, out);
    out << ", ";
  }

  // member: priority
  {
    out << "priority: ";
    rosidl_generator_traits::value_to_yaml(msg.priority, out);
    out << ", ";
  }

  // member: policy
  {
    out << "policy: ";
    rosidl_generator_traits::value_to_yaml(msg.policy, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const ChargeDecision & msg,
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

  // member: accepted
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "accepted: ";
    rosidl_generator_traits::value_to_yaml(msg.accepted, out);
    out << "\n";
  }

  // member: slot_start_time
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "slot_start_time:\n";
    to_block_style_yaml(msg.slot_start_time, out, indentation + 2);
  }

  // member: priority
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "priority: ";
    rosidl_generator_traits::value_to_yaml(msg.priority, out);
    out << "\n";
  }

  // member: policy
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "policy: ";
    rosidl_generator_traits::value_to_yaml(msg.policy, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const ChargeDecision & msg, bool use_flow_style = false)
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
  const uav_msgs::msg::ChargeDecision & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::msg::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::msg::ChargeDecision & msg)
{
  return uav_msgs::msg::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::msg::ChargeDecision>()
{
  return "uav_msgs::msg::ChargeDecision";
}

template<>
inline const char * name<uav_msgs::msg::ChargeDecision>()
{
  return "uav_msgs/msg/ChargeDecision";
}

template<>
struct has_fixed_size<uav_msgs::msg::ChargeDecision>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<uav_msgs::msg::ChargeDecision>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<uav_msgs::msg::ChargeDecision>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__TRAITS_HPP_
