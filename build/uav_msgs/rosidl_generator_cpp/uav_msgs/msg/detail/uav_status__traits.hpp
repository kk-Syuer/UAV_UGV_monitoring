// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from uav_msgs:msg/UavStatus.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__UAV_STATUS__TRAITS_HPP_
#define UAV_MSGS__MSG__DETAIL__UAV_STATUS__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "uav_msgs/msg/detail/uav_status__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'pose'
#include "geometry_msgs/msg/detail/pose__traits.hpp"
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__traits.hpp"

namespace uav_msgs
{

namespace msg
{

inline void to_flow_style_yaml(
  const UavStatus & msg,
  std::ostream & out)
{
  out << "{";
  // member: uav_id
  {
    out << "uav_id: ";
    rosidl_generator_traits::value_to_yaml(msg.uav_id, out);
    out << ", ";
  }

  // member: role
  {
    out << "role: ";
    rosidl_generator_traits::value_to_yaml(msg.role, out);
    out << ", ";
  }

  // member: cluster_id
  {
    out << "cluster_id: ";
    rosidl_generator_traits::value_to_yaml(msg.cluster_id, out);
    out << ", ";
  }

  // member: battery_level
  {
    out << "battery_level: ";
    rosidl_generator_traits::value_to_yaml(msg.battery_level, out);
    out << ", ";
  }

  // member: battery_capacity
  {
    out << "battery_capacity: ";
    rosidl_generator_traits::value_to_yaml(msg.battery_capacity, out);
    out << ", ";
  }

  // member: pose
  {
    out << "pose: ";
    to_flow_style_yaml(msg.pose, out);
    out << ", ";
  }

  // member: service_radius
  {
    out << "service_radius: ";
    rosidl_generator_traits::value_to_yaml(msg.service_radius, out);
    out << ", ";
  }

  // member: connected_users
  {
    out << "connected_users: ";
    rosidl_generator_traits::value_to_yaml(msg.connected_users, out);
    out << ", ";
  }

  // member: traffic_load
  {
    out << "traffic_load: ";
    rosidl_generator_traits::value_to_yaml(msg.traffic_load, out);
    out << ", ";
  }

  // member: packet_loss_estimate
  {
    out << "packet_loss_estimate: ";
    rosidl_generator_traits::value_to_yaml(msg.packet_loss_estimate, out);
    out << ", ";
  }

  // member: energy_consumption_rate
  {
    out << "energy_consumption_rate: ";
    rosidl_generator_traits::value_to_yaml(msg.energy_consumption_rate, out);
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
  const UavStatus & msg,
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

  // member: role
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "role: ";
    rosidl_generator_traits::value_to_yaml(msg.role, out);
    out << "\n";
  }

  // member: cluster_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "cluster_id: ";
    rosidl_generator_traits::value_to_yaml(msg.cluster_id, out);
    out << "\n";
  }

  // member: battery_level
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "battery_level: ";
    rosidl_generator_traits::value_to_yaml(msg.battery_level, out);
    out << "\n";
  }

  // member: battery_capacity
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "battery_capacity: ";
    rosidl_generator_traits::value_to_yaml(msg.battery_capacity, out);
    out << "\n";
  }

  // member: pose
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "pose:\n";
    to_block_style_yaml(msg.pose, out, indentation + 2);
  }

  // member: service_radius
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "service_radius: ";
    rosidl_generator_traits::value_to_yaml(msg.service_radius, out);
    out << "\n";
  }

  // member: connected_users
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "connected_users: ";
    rosidl_generator_traits::value_to_yaml(msg.connected_users, out);
    out << "\n";
  }

  // member: traffic_load
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "traffic_load: ";
    rosidl_generator_traits::value_to_yaml(msg.traffic_load, out);
    out << "\n";
  }

  // member: packet_loss_estimate
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "packet_loss_estimate: ";
    rosidl_generator_traits::value_to_yaml(msg.packet_loss_estimate, out);
    out << "\n";
  }

  // member: energy_consumption_rate
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "energy_consumption_rate: ";
    rosidl_generator_traits::value_to_yaml(msg.energy_consumption_rate, out);
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

inline std::string to_yaml(const UavStatus & msg, bool use_flow_style = false)
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
  const uav_msgs::msg::UavStatus & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::msg::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::msg::UavStatus & msg)
{
  return uav_msgs::msg::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::msg::UavStatus>()
{
  return "uav_msgs::msg::UavStatus";
}

template<>
inline const char * name<uav_msgs::msg::UavStatus>()
{
  return "uav_msgs/msg/UavStatus";
}

template<>
struct has_fixed_size<uav_msgs::msg::UavStatus>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<uav_msgs::msg::UavStatus>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<uav_msgs::msg::UavStatus>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // UAV_MSGS__MSG__DETAIL__UAV_STATUS__TRAITS_HPP_
