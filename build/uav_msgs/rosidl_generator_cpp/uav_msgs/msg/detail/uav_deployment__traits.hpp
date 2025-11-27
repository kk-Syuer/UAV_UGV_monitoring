// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from uav_msgs:msg/UavDeployment.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__UAV_DEPLOYMENT__TRAITS_HPP_
#define UAV_MSGS__MSG__DETAIL__UAV_DEPLOYMENT__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "uav_msgs/msg/detail/uav_deployment__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'target_pose'
#include "geometry_msgs/msg/detail/pose__traits.hpp"

namespace uav_msgs
{

namespace msg
{

inline void to_flow_style_yaml(
  const UavDeployment & msg,
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

  // member: ch_id
  {
    out << "ch_id: ";
    rosidl_generator_traits::value_to_yaml(msg.ch_id, out);
    out << ", ";
  }

  // member: target_pose
  {
    out << "target_pose: ";
    to_flow_style_yaml(msg.target_pose, out);
    out << ", ";
  }

  // member: next_hop_to_sink
  {
    out << "next_hop_to_sink: ";
    rosidl_generator_traits::value_to_yaml(msg.next_hop_to_sink, out);
    out << ", ";
  }

  // member: next_hop_to_ugv
  {
    out << "next_hop_to_ugv: ";
    rosidl_generator_traits::value_to_yaml(msg.next_hop_to_ugv, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const UavDeployment & msg,
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

  // member: ch_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "ch_id: ";
    rosidl_generator_traits::value_to_yaml(msg.ch_id, out);
    out << "\n";
  }

  // member: target_pose
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "target_pose:\n";
    to_block_style_yaml(msg.target_pose, out, indentation + 2);
  }

  // member: next_hop_to_sink
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "next_hop_to_sink: ";
    rosidl_generator_traits::value_to_yaml(msg.next_hop_to_sink, out);
    out << "\n";
  }

  // member: next_hop_to_ugv
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "next_hop_to_ugv: ";
    rosidl_generator_traits::value_to_yaml(msg.next_hop_to_ugv, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const UavDeployment & msg, bool use_flow_style = false)
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
  const uav_msgs::msg::UavDeployment & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::msg::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::msg::UavDeployment & msg)
{
  return uav_msgs::msg::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::msg::UavDeployment>()
{
  return "uav_msgs::msg::UavDeployment";
}

template<>
inline const char * name<uav_msgs::msg::UavDeployment>()
{
  return "uav_msgs/msg/UavDeployment";
}

template<>
struct has_fixed_size<uav_msgs::msg::UavDeployment>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<uav_msgs::msg::UavDeployment>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<uav_msgs::msg::UavDeployment>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // UAV_MSGS__MSG__DETAIL__UAV_DEPLOYMENT__TRAITS_HPP_
