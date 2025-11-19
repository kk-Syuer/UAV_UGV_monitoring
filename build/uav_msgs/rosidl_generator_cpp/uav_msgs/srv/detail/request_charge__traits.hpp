// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from uav_msgs:srv/RequestCharge.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__SRV__DETAIL__REQUEST_CHARGE__TRAITS_HPP_
#define UAV_MSGS__SRV__DETAIL__REQUEST_CHARGE__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "uav_msgs/srv/detail/request_charge__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace uav_msgs
{

namespace srv
{

inline void to_flow_style_yaml(
  const RequestCharge_Request & msg,
  std::ostream & out)
{
  out << "{";
  // member: uav_id
  {
    out << "uav_id: ";
    rosidl_generator_traits::value_to_yaml(msg.uav_id, out);
    out << ", ";
  }

  // member: battery_level
  {
    out << "battery_level: ";
    rosidl_generator_traits::value_to_yaml(msg.battery_level, out);
    out << ", ";
  }

  // member: role
  {
    out << "role: ";
    rosidl_generator_traits::value_to_yaml(msg.role, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const RequestCharge_Request & msg,
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

  // member: battery_level
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "battery_level: ";
    rosidl_generator_traits::value_to_yaml(msg.battery_level, out);
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
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const RequestCharge_Request & msg, bool use_flow_style = false)
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
  const uav_msgs::srv::RequestCharge_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::srv::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::srv::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::srv::RequestCharge_Request & msg)
{
  return uav_msgs::srv::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::srv::RequestCharge_Request>()
{
  return "uav_msgs::srv::RequestCharge_Request";
}

template<>
inline const char * name<uav_msgs::srv::RequestCharge_Request>()
{
  return "uav_msgs/srv/RequestCharge_Request";
}

template<>
struct has_fixed_size<uav_msgs::srv::RequestCharge_Request>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<uav_msgs::srv::RequestCharge_Request>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<uav_msgs::srv::RequestCharge_Request>
  : std::true_type {};

}  // namespace rosidl_generator_traits

// Include directives for member types
// Member 'eta'
#include "builtin_interfaces/msg/detail/time__traits.hpp"

namespace uav_msgs
{

namespace srv
{

inline void to_flow_style_yaml(
  const RequestCharge_Response & msg,
  std::ostream & out)
{
  out << "{";
  // member: accepted
  {
    out << "accepted: ";
    rosidl_generator_traits::value_to_yaml(msg.accepted, out);
    out << ", ";
  }

  // member: eta
  {
    out << "eta: ";
    to_flow_style_yaml(msg.eta, out);
    out << ", ";
  }

  // member: assigned_priority
  {
    out << "assigned_priority: ";
    rosidl_generator_traits::value_to_yaml(msg.assigned_priority, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const RequestCharge_Response & msg,
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

  // member: eta
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "eta:\n";
    to_block_style_yaml(msg.eta, out, indentation + 2);
  }

  // member: assigned_priority
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "assigned_priority: ";
    rosidl_generator_traits::value_to_yaml(msg.assigned_priority, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const RequestCharge_Response & msg, bool use_flow_style = false)
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
  const uav_msgs::srv::RequestCharge_Response & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::srv::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::srv::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::srv::RequestCharge_Response & msg)
{
  return uav_msgs::srv::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::srv::RequestCharge_Response>()
{
  return "uav_msgs::srv::RequestCharge_Response";
}

template<>
inline const char * name<uav_msgs::srv::RequestCharge_Response>()
{
  return "uav_msgs/srv/RequestCharge_Response";
}

template<>
struct has_fixed_size<uav_msgs::srv::RequestCharge_Response>
  : std::integral_constant<bool, has_fixed_size<builtin_interfaces::msg::Time>::value> {};

template<>
struct has_bounded_size<uav_msgs::srv::RequestCharge_Response>
  : std::integral_constant<bool, has_bounded_size<builtin_interfaces::msg::Time>::value> {};

template<>
struct is_message<uav_msgs::srv::RequestCharge_Response>
  : std::true_type {};

}  // namespace rosidl_generator_traits

namespace rosidl_generator_traits
{

template<>
inline const char * data_type<uav_msgs::srv::RequestCharge>()
{
  return "uav_msgs::srv::RequestCharge";
}

template<>
inline const char * name<uav_msgs::srv::RequestCharge>()
{
  return "uav_msgs/srv/RequestCharge";
}

template<>
struct has_fixed_size<uav_msgs::srv::RequestCharge>
  : std::integral_constant<
    bool,
    has_fixed_size<uav_msgs::srv::RequestCharge_Request>::value &&
    has_fixed_size<uav_msgs::srv::RequestCharge_Response>::value
  >
{
};

template<>
struct has_bounded_size<uav_msgs::srv::RequestCharge>
  : std::integral_constant<
    bool,
    has_bounded_size<uav_msgs::srv::RequestCharge_Request>::value &&
    has_bounded_size<uav_msgs::srv::RequestCharge_Response>::value
  >
{
};

template<>
struct is_service<uav_msgs::srv::RequestCharge>
  : std::true_type
{
};

template<>
struct is_service_request<uav_msgs::srv::RequestCharge_Request>
  : std::true_type
{
};

template<>
struct is_service_response<uav_msgs::srv::RequestCharge_Response>
  : std::true_type
{
};

}  // namespace rosidl_generator_traits

#endif  // UAV_MSGS__SRV__DETAIL__REQUEST_CHARGE__TRAITS_HPP_
