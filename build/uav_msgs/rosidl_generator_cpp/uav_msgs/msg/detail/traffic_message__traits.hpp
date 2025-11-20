// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from uav_msgs:msg/TrafficMessage.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__TRAFFIC_MESSAGE__TRAITS_HPP_
#define UAV_MSGS__MSG__DETAIL__TRAFFIC_MESSAGE__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "uav_msgs/msg/detail/traffic_message__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'creation_time'
#include "builtin_interfaces/msg/detail/time__traits.hpp"

namespace uav_msgs
{

namespace msg
{

inline void to_flow_style_yaml(
  const TrafficMessage & msg,
  std::ostream & out)
{
  out << "{";
  // member: msg_id
  {
    out << "msg_id: ";
    rosidl_generator_traits::value_to_yaml(msg.msg_id, out);
    out << ", ";
  }

  // member: src_id
  {
    out << "src_id: ";
    rosidl_generator_traits::value_to_yaml(msg.src_id, out);
    out << ", ";
  }

  // member: dst_id
  {
    out << "dst_id: ";
    rosidl_generator_traits::value_to_yaml(msg.dst_id, out);
    out << ", ";
  }

  // member: next_hop_id
  {
    out << "next_hop_id: ";
    rosidl_generator_traits::value_to_yaml(msg.next_hop_id, out);
    out << ", ";
  }

  // member: msg_type
  {
    out << "msg_type: ";
    rosidl_generator_traits::value_to_yaml(msg.msg_type, out);
    out << ", ";
  }

  // member: priority
  {
    out << "priority: ";
    rosidl_generator_traits::value_to_yaml(msg.priority, out);
    out << ", ";
  }

  // member: size_bytes
  {
    out << "size_bytes: ";
    rosidl_generator_traits::value_to_yaml(msg.size_bytes, out);
    out << ", ";
  }

  // member: creation_time
  {
    out << "creation_time: ";
    to_flow_style_yaml(msg.creation_time, out);
    out << ", ";
  }

  // member: hop_count
  {
    out << "hop_count: ";
    rosidl_generator_traits::value_to_yaml(msg.hop_count, out);
    out << ", ";
  }

  // member: control_type
  {
    out << "control_type: ";
    rosidl_generator_traits::value_to_yaml(msg.control_type, out);
    out << ", ";
  }

  // member: control_payload
  {
    out << "control_payload: ";
    rosidl_generator_traits::value_to_yaml(msg.control_payload, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const TrafficMessage & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: msg_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "msg_id: ";
    rosidl_generator_traits::value_to_yaml(msg.msg_id, out);
    out << "\n";
  }

  // member: src_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "src_id: ";
    rosidl_generator_traits::value_to_yaml(msg.src_id, out);
    out << "\n";
  }

  // member: dst_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "dst_id: ";
    rosidl_generator_traits::value_to_yaml(msg.dst_id, out);
    out << "\n";
  }

  // member: next_hop_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "next_hop_id: ";
    rosidl_generator_traits::value_to_yaml(msg.next_hop_id, out);
    out << "\n";
  }

  // member: msg_type
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "msg_type: ";
    rosidl_generator_traits::value_to_yaml(msg.msg_type, out);
    out << "\n";
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

  // member: size_bytes
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "size_bytes: ";
    rosidl_generator_traits::value_to_yaml(msg.size_bytes, out);
    out << "\n";
  }

  // member: creation_time
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "creation_time:\n";
    to_block_style_yaml(msg.creation_time, out, indentation + 2);
  }

  // member: hop_count
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "hop_count: ";
    rosidl_generator_traits::value_to_yaml(msg.hop_count, out);
    out << "\n";
  }

  // member: control_type
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "control_type: ";
    rosidl_generator_traits::value_to_yaml(msg.control_type, out);
    out << "\n";
  }

  // member: control_payload
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "control_payload: ";
    rosidl_generator_traits::value_to_yaml(msg.control_payload, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const TrafficMessage & msg, bool use_flow_style = false)
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
  const uav_msgs::msg::TrafficMessage & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::msg::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::msg::TrafficMessage & msg)
{
  return uav_msgs::msg::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::msg::TrafficMessage>()
{
  return "uav_msgs::msg::TrafficMessage";
}

template<>
inline const char * name<uav_msgs::msg::TrafficMessage>()
{
  return "uav_msgs/msg/TrafficMessage";
}

template<>
struct has_fixed_size<uav_msgs::msg::TrafficMessage>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<uav_msgs::msg::TrafficMessage>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<uav_msgs::msg::TrafficMessage>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // UAV_MSGS__MSG__DETAIL__TRAFFIC_MESSAGE__TRAITS_HPP_
