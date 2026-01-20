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
// Member 'last_tx_time'
// Member 'last_rx_time'
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

  // member: last_hop_id
  {
    out << "last_hop_id: ";
    rosidl_generator_traits::value_to_yaml(msg.last_hop_id, out);
    out << ", ";
  }

  // member: flow_type
  {
    out << "flow_type: ";
    rosidl_generator_traits::value_to_yaml(msg.flow_type, out);
    out << ", ";
  }

  // member: control_type
  {
    out << "control_type: ";
    rosidl_generator_traits::value_to_yaml(msg.control_type, out);
    out << ", ";
  }

  // member: seq
  {
    out << "seq: ";
    rosidl_generator_traits::value_to_yaml(msg.seq, out);
    out << ", ";
  }

  // member: hop_count
  {
    out << "hop_count: ";
    rosidl_generator_traits::value_to_yaml(msg.hop_count, out);
    out << ", ";
  }

  // member: ttl
  {
    out << "ttl: ";
    rosidl_generator_traits::value_to_yaml(msg.ttl, out);
    out << ", ";
  }

  // member: requires_ack
  {
    out << "requires_ack: ";
    rosidl_generator_traits::value_to_yaml(msg.requires_ack, out);
    out << ", ";
  }

  // member: payload
  {
    out << "payload: ";
    rosidl_generator_traits::value_to_yaml(msg.payload, out);
    out << ", ";
  }

  // member: creation_time
  {
    out << "creation_time: ";
    to_flow_style_yaml(msg.creation_time, out);
    out << ", ";
  }

  // member: ref_msg_id
  {
    out << "ref_msg_id: ";
    rosidl_generator_traits::value_to_yaml(msg.ref_msg_id, out);
    out << ", ";
  }

  // member: last_tx_time
  {
    out << "last_tx_time: ";
    to_flow_style_yaml(msg.last_tx_time, out);
    out << ", ";
  }

  // member: last_rx_time
  {
    out << "last_rx_time: ";
    to_flow_style_yaml(msg.last_rx_time, out);
    out << ", ";
  }

  // member: drop_reason
  {
    out << "drop_reason: ";
    rosidl_generator_traits::value_to_yaml(msg.drop_reason, out);
    out << ", ";
  }

  // member: recent_hops
  {
    if (msg.recent_hops.size() == 0) {
      out << "recent_hops: []";
    } else {
      out << "recent_hops: [";
      size_t pending_items = msg.recent_hops.size();
      for (auto item : msg.recent_hops) {
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

  // member: last_hop_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "last_hop_id: ";
    rosidl_generator_traits::value_to_yaml(msg.last_hop_id, out);
    out << "\n";
  }

  // member: flow_type
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "flow_type: ";
    rosidl_generator_traits::value_to_yaml(msg.flow_type, out);
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

  // member: seq
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "seq: ";
    rosidl_generator_traits::value_to_yaml(msg.seq, out);
    out << "\n";
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

  // member: ttl
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "ttl: ";
    rosidl_generator_traits::value_to_yaml(msg.ttl, out);
    out << "\n";
  }

  // member: requires_ack
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "requires_ack: ";
    rosidl_generator_traits::value_to_yaml(msg.requires_ack, out);
    out << "\n";
  }

  // member: payload
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "payload: ";
    rosidl_generator_traits::value_to_yaml(msg.payload, out);
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

  // member: ref_msg_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "ref_msg_id: ";
    rosidl_generator_traits::value_to_yaml(msg.ref_msg_id, out);
    out << "\n";
  }

  // member: last_tx_time
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "last_tx_time:\n";
    to_block_style_yaml(msg.last_tx_time, out, indentation + 2);
  }

  // member: last_rx_time
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "last_rx_time:\n";
    to_block_style_yaml(msg.last_rx_time, out, indentation + 2);
  }

  // member: drop_reason
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "drop_reason: ";
    rosidl_generator_traits::value_to_yaml(msg.drop_reason, out);
    out << "\n";
  }

  // member: recent_hops
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.recent_hops.size() == 0) {
      out << "recent_hops: []\n";
    } else {
      out << "recent_hops:\n";
      for (auto item : msg.recent_hops) {
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
