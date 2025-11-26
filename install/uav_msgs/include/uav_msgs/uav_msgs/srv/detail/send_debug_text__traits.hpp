// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from uav_msgs:srv/SendDebugText.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__SRV__DETAIL__SEND_DEBUG_TEXT__TRAITS_HPP_
#define UAV_MSGS__SRV__DETAIL__SEND_DEBUG_TEXT__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "uav_msgs/srv/detail/send_debug_text__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace uav_msgs
{

namespace srv
{

inline void to_flow_style_yaml(
  const SendDebugText_Request & msg,
  std::ostream & out)
{
  out << "{";
  // member: dst_id
  {
    out << "dst_id: ";
    rosidl_generator_traits::value_to_yaml(msg.dst_id, out);
    out << ", ";
  }

  // member: text
  {
    out << "text: ";
    rosidl_generator_traits::value_to_yaml(msg.text, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const SendDebugText_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: dst_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "dst_id: ";
    rosidl_generator_traits::value_to_yaml(msg.dst_id, out);
    out << "\n";
  }

  // member: text
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "text: ";
    rosidl_generator_traits::value_to_yaml(msg.text, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const SendDebugText_Request & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace srv

}  // namespace uav_msgs

namespace rosidl_generator_traits
{

[[deprecated("use uav_msgs::srv::to_block_style_yaml() instead")]]
inline void to_yaml(
  const uav_msgs::srv::SendDebugText_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::srv::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::srv::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::srv::SendDebugText_Request & msg)
{
  return uav_msgs::srv::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::srv::SendDebugText_Request>()
{
  return "uav_msgs::srv::SendDebugText_Request";
}

template<>
inline const char * name<uav_msgs::srv::SendDebugText_Request>()
{
  return "uav_msgs/srv/SendDebugText_Request";
}

template<>
struct has_fixed_size<uav_msgs::srv::SendDebugText_Request>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<uav_msgs::srv::SendDebugText_Request>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<uav_msgs::srv::SendDebugText_Request>
  : std::true_type {};

}  // namespace rosidl_generator_traits

namespace uav_msgs
{

namespace srv
{

inline void to_flow_style_yaml(
  const SendDebugText_Response & msg,
  std::ostream & out)
{
  out << "{";
  // member: accepted
  {
    out << "accepted: ";
    rosidl_generator_traits::value_to_yaml(msg.accepted, out);
    out << ", ";
  }

  // member: info
  {
    out << "info: ";
    rosidl_generator_traits::value_to_yaml(msg.info, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const SendDebugText_Response & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: accepted
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "accepted: ";
    rosidl_generator_traits::value_to_yaml(msg.accepted, out);
    out << "\n";
  }

  // member: info
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "info: ";
    rosidl_generator_traits::value_to_yaml(msg.info, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const SendDebugText_Response & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace srv

}  // namespace uav_msgs

namespace rosidl_generator_traits
{

[[deprecated("use uav_msgs::srv::to_block_style_yaml() instead")]]
inline void to_yaml(
  const uav_msgs::srv::SendDebugText_Response & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::srv::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::srv::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::srv::SendDebugText_Response & msg)
{
  return uav_msgs::srv::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::srv::SendDebugText_Response>()
{
  return "uav_msgs::srv::SendDebugText_Response";
}

template<>
inline const char * name<uav_msgs::srv::SendDebugText_Response>()
{
  return "uav_msgs/srv/SendDebugText_Response";
}

template<>
struct has_fixed_size<uav_msgs::srv::SendDebugText_Response>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<uav_msgs::srv::SendDebugText_Response>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<uav_msgs::srv::SendDebugText_Response>
  : std::true_type {};

}  // namespace rosidl_generator_traits

namespace rosidl_generator_traits
{

template<>
inline const char * data_type<uav_msgs::srv::SendDebugText>()
{
  return "uav_msgs::srv::SendDebugText";
}

template<>
inline const char * name<uav_msgs::srv::SendDebugText>()
{
  return "uav_msgs/srv/SendDebugText";
}

template<>
struct has_fixed_size<uav_msgs::srv::SendDebugText>
  : std::integral_constant<
    bool,
    has_fixed_size<uav_msgs::srv::SendDebugText_Request>::value &&
    has_fixed_size<uav_msgs::srv::SendDebugText_Response>::value
  >
{
};

template<>
struct has_bounded_size<uav_msgs::srv::SendDebugText>
  : std::integral_constant<
    bool,
    has_bounded_size<uav_msgs::srv::SendDebugText_Request>::value &&
    has_bounded_size<uav_msgs::srv::SendDebugText_Response>::value
  >
{
};

template<>
struct is_service<uav_msgs::srv::SendDebugText>
  : std::true_type
{
};

template<>
struct is_service_request<uav_msgs::srv::SendDebugText_Request>
  : std::true_type
{
};

template<>
struct is_service_response<uav_msgs::srv::SendDebugText_Response>
  : std::true_type
{
};

}  // namespace rosidl_generator_traits

#endif  // UAV_MSGS__SRV__DETAIL__SEND_DEBUG_TEXT__TRAITS_HPP_
