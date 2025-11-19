// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from uav_msgs:msg/ClusterInfo.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__CLUSTER_INFO__TRAITS_HPP_
#define UAV_MSGS__MSG__DETAIL__CLUSTER_INFO__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "uav_msgs/msg/detail/cluster_info__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace uav_msgs
{

namespace msg
{

inline void to_flow_style_yaml(
  const ClusterInfo & msg,
  std::ostream & out)
{
  out << "{";
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

  // member: member_ids
  {
    if (msg.member_ids.size() == 0) {
      out << "member_ids: []";
    } else {
      out << "member_ids: [";
      size_t pending_items = msg.member_ids.size();
      for (auto item : msg.member_ids) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const ClusterInfo & msg,
  std::ostream & out, size_t indentation = 0)
{
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

  // member: member_ids
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.member_ids.size() == 0) {
      out << "member_ids: []\n";
    } else {
      out << "member_ids:\n";
      for (auto item : msg.member_ids) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const ClusterInfo & msg, bool use_flow_style = false)
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
  const uav_msgs::msg::ClusterInfo & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::msg::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::msg::ClusterInfo & msg)
{
  return uav_msgs::msg::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::msg::ClusterInfo>()
{
  return "uav_msgs::msg::ClusterInfo";
}

template<>
inline const char * name<uav_msgs::msg::ClusterInfo>()
{
  return "uav_msgs/msg/ClusterInfo";
}

template<>
struct has_fixed_size<uav_msgs::msg::ClusterInfo>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<uav_msgs::msg::ClusterInfo>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<uav_msgs::msg::ClusterInfo>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // UAV_MSGS__MSG__DETAIL__CLUSTER_INFO__TRAITS_HPP_
