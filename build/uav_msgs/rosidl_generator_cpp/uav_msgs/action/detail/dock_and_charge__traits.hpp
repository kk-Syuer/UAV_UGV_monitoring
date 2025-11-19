// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from uav_msgs:action/DockAndCharge.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__ACTION__DETAIL__DOCK_AND_CHARGE__TRAITS_HPP_
#define UAV_MSGS__ACTION__DETAIL__DOCK_AND_CHARGE__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "uav_msgs/action/detail/dock_and_charge__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace uav_msgs
{

namespace action
{

inline void to_flow_style_yaml(
  const DockAndCharge_Goal & msg,
  std::ostream & out)
{
  out << "{";
  // member: uav_id
  {
    out << "uav_id: ";
    rosidl_generator_traits::value_to_yaml(msg.uav_id, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const DockAndCharge_Goal & msg,
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
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const DockAndCharge_Goal & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace action

}  // namespace uav_msgs

namespace rosidl_generator_traits
{

[[deprecated("use uav_msgs::action::to_block_style_yaml() instead")]]
inline void to_yaml(
  const uav_msgs::action::DockAndCharge_Goal & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::action::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::action::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::action::DockAndCharge_Goal & msg)
{
  return uav_msgs::action::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::action::DockAndCharge_Goal>()
{
  return "uav_msgs::action::DockAndCharge_Goal";
}

template<>
inline const char * name<uav_msgs::action::DockAndCharge_Goal>()
{
  return "uav_msgs/action/DockAndCharge_Goal";
}

template<>
struct has_fixed_size<uav_msgs::action::DockAndCharge_Goal>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<uav_msgs::action::DockAndCharge_Goal>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<uav_msgs::action::DockAndCharge_Goal>
  : std::true_type {};

}  // namespace rosidl_generator_traits

namespace uav_msgs
{

namespace action
{

inline void to_flow_style_yaml(
  const DockAndCharge_Result & msg,
  std::ostream & out)
{
  out << "{";
  // member: battery_level
  {
    out << "battery_level: ";
    rosidl_generator_traits::value_to_yaml(msg.battery_level, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const DockAndCharge_Result & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: battery_level
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "battery_level: ";
    rosidl_generator_traits::value_to_yaml(msg.battery_level, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const DockAndCharge_Result & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace action

}  // namespace uav_msgs

namespace rosidl_generator_traits
{

[[deprecated("use uav_msgs::action::to_block_style_yaml() instead")]]
inline void to_yaml(
  const uav_msgs::action::DockAndCharge_Result & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::action::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::action::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::action::DockAndCharge_Result & msg)
{
  return uav_msgs::action::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::action::DockAndCharge_Result>()
{
  return "uav_msgs::action::DockAndCharge_Result";
}

template<>
inline const char * name<uav_msgs::action::DockAndCharge_Result>()
{
  return "uav_msgs/action/DockAndCharge_Result";
}

template<>
struct has_fixed_size<uav_msgs::action::DockAndCharge_Result>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<uav_msgs::action::DockAndCharge_Result>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<uav_msgs::action::DockAndCharge_Result>
  : std::true_type {};

}  // namespace rosidl_generator_traits

namespace uav_msgs
{

namespace action
{

inline void to_flow_style_yaml(
  const DockAndCharge_Feedback & msg,
  std::ostream & out)
{
  out << "{";
  // member: current_battery
  {
    out << "current_battery: ";
    rosidl_generator_traits::value_to_yaml(msg.current_battery, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const DockAndCharge_Feedback & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: current_battery
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "current_battery: ";
    rosidl_generator_traits::value_to_yaml(msg.current_battery, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const DockAndCharge_Feedback & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace action

}  // namespace uav_msgs

namespace rosidl_generator_traits
{

[[deprecated("use uav_msgs::action::to_block_style_yaml() instead")]]
inline void to_yaml(
  const uav_msgs::action::DockAndCharge_Feedback & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::action::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::action::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::action::DockAndCharge_Feedback & msg)
{
  return uav_msgs::action::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::action::DockAndCharge_Feedback>()
{
  return "uav_msgs::action::DockAndCharge_Feedback";
}

template<>
inline const char * name<uav_msgs::action::DockAndCharge_Feedback>()
{
  return "uav_msgs/action/DockAndCharge_Feedback";
}

template<>
struct has_fixed_size<uav_msgs::action::DockAndCharge_Feedback>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<uav_msgs::action::DockAndCharge_Feedback>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<uav_msgs::action::DockAndCharge_Feedback>
  : std::true_type {};

}  // namespace rosidl_generator_traits

// Include directives for member types
// Member 'goal_id'
#include "unique_identifier_msgs/msg/detail/uuid__traits.hpp"
// Member 'goal'
#include "uav_msgs/action/detail/dock_and_charge__traits.hpp"

namespace uav_msgs
{

namespace action
{

inline void to_flow_style_yaml(
  const DockAndCharge_SendGoal_Request & msg,
  std::ostream & out)
{
  out << "{";
  // member: goal_id
  {
    out << "goal_id: ";
    to_flow_style_yaml(msg.goal_id, out);
    out << ", ";
  }

  // member: goal
  {
    out << "goal: ";
    to_flow_style_yaml(msg.goal, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const DockAndCharge_SendGoal_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: goal_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "goal_id:\n";
    to_block_style_yaml(msg.goal_id, out, indentation + 2);
  }

  // member: goal
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "goal:\n";
    to_block_style_yaml(msg.goal, out, indentation + 2);
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const DockAndCharge_SendGoal_Request & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace action

}  // namespace uav_msgs

namespace rosidl_generator_traits
{

[[deprecated("use uav_msgs::action::to_block_style_yaml() instead")]]
inline void to_yaml(
  const uav_msgs::action::DockAndCharge_SendGoal_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::action::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::action::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::action::DockAndCharge_SendGoal_Request & msg)
{
  return uav_msgs::action::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::action::DockAndCharge_SendGoal_Request>()
{
  return "uav_msgs::action::DockAndCharge_SendGoal_Request";
}

template<>
inline const char * name<uav_msgs::action::DockAndCharge_SendGoal_Request>()
{
  return "uav_msgs/action/DockAndCharge_SendGoal_Request";
}

template<>
struct has_fixed_size<uav_msgs::action::DockAndCharge_SendGoal_Request>
  : std::integral_constant<bool, has_fixed_size<uav_msgs::action::DockAndCharge_Goal>::value && has_fixed_size<unique_identifier_msgs::msg::UUID>::value> {};

template<>
struct has_bounded_size<uav_msgs::action::DockAndCharge_SendGoal_Request>
  : std::integral_constant<bool, has_bounded_size<uav_msgs::action::DockAndCharge_Goal>::value && has_bounded_size<unique_identifier_msgs::msg::UUID>::value> {};

template<>
struct is_message<uav_msgs::action::DockAndCharge_SendGoal_Request>
  : std::true_type {};

}  // namespace rosidl_generator_traits

// Include directives for member types
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__traits.hpp"

namespace uav_msgs
{

namespace action
{

inline void to_flow_style_yaml(
  const DockAndCharge_SendGoal_Response & msg,
  std::ostream & out)
{
  out << "{";
  // member: accepted
  {
    out << "accepted: ";
    rosidl_generator_traits::value_to_yaml(msg.accepted, out);
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
  const DockAndCharge_SendGoal_Response & msg,
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

  // member: stamp
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "stamp:\n";
    to_block_style_yaml(msg.stamp, out, indentation + 2);
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const DockAndCharge_SendGoal_Response & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace action

}  // namespace uav_msgs

namespace rosidl_generator_traits
{

[[deprecated("use uav_msgs::action::to_block_style_yaml() instead")]]
inline void to_yaml(
  const uav_msgs::action::DockAndCharge_SendGoal_Response & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::action::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::action::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::action::DockAndCharge_SendGoal_Response & msg)
{
  return uav_msgs::action::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::action::DockAndCharge_SendGoal_Response>()
{
  return "uav_msgs::action::DockAndCharge_SendGoal_Response";
}

template<>
inline const char * name<uav_msgs::action::DockAndCharge_SendGoal_Response>()
{
  return "uav_msgs/action/DockAndCharge_SendGoal_Response";
}

template<>
struct has_fixed_size<uav_msgs::action::DockAndCharge_SendGoal_Response>
  : std::integral_constant<bool, has_fixed_size<builtin_interfaces::msg::Time>::value> {};

template<>
struct has_bounded_size<uav_msgs::action::DockAndCharge_SendGoal_Response>
  : std::integral_constant<bool, has_bounded_size<builtin_interfaces::msg::Time>::value> {};

template<>
struct is_message<uav_msgs::action::DockAndCharge_SendGoal_Response>
  : std::true_type {};

}  // namespace rosidl_generator_traits

namespace rosidl_generator_traits
{

template<>
inline const char * data_type<uav_msgs::action::DockAndCharge_SendGoal>()
{
  return "uav_msgs::action::DockAndCharge_SendGoal";
}

template<>
inline const char * name<uav_msgs::action::DockAndCharge_SendGoal>()
{
  return "uav_msgs/action/DockAndCharge_SendGoal";
}

template<>
struct has_fixed_size<uav_msgs::action::DockAndCharge_SendGoal>
  : std::integral_constant<
    bool,
    has_fixed_size<uav_msgs::action::DockAndCharge_SendGoal_Request>::value &&
    has_fixed_size<uav_msgs::action::DockAndCharge_SendGoal_Response>::value
  >
{
};

template<>
struct has_bounded_size<uav_msgs::action::DockAndCharge_SendGoal>
  : std::integral_constant<
    bool,
    has_bounded_size<uav_msgs::action::DockAndCharge_SendGoal_Request>::value &&
    has_bounded_size<uav_msgs::action::DockAndCharge_SendGoal_Response>::value
  >
{
};

template<>
struct is_service<uav_msgs::action::DockAndCharge_SendGoal>
  : std::true_type
{
};

template<>
struct is_service_request<uav_msgs::action::DockAndCharge_SendGoal_Request>
  : std::true_type
{
};

template<>
struct is_service_response<uav_msgs::action::DockAndCharge_SendGoal_Response>
  : std::true_type
{
};

}  // namespace rosidl_generator_traits

// Include directives for member types
// Member 'goal_id'
// already included above
// #include "unique_identifier_msgs/msg/detail/uuid__traits.hpp"

namespace uav_msgs
{

namespace action
{

inline void to_flow_style_yaml(
  const DockAndCharge_GetResult_Request & msg,
  std::ostream & out)
{
  out << "{";
  // member: goal_id
  {
    out << "goal_id: ";
    to_flow_style_yaml(msg.goal_id, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const DockAndCharge_GetResult_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: goal_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "goal_id:\n";
    to_block_style_yaml(msg.goal_id, out, indentation + 2);
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const DockAndCharge_GetResult_Request & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace action

}  // namespace uav_msgs

namespace rosidl_generator_traits
{

[[deprecated("use uav_msgs::action::to_block_style_yaml() instead")]]
inline void to_yaml(
  const uav_msgs::action::DockAndCharge_GetResult_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::action::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::action::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::action::DockAndCharge_GetResult_Request & msg)
{
  return uav_msgs::action::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::action::DockAndCharge_GetResult_Request>()
{
  return "uav_msgs::action::DockAndCharge_GetResult_Request";
}

template<>
inline const char * name<uav_msgs::action::DockAndCharge_GetResult_Request>()
{
  return "uav_msgs/action/DockAndCharge_GetResult_Request";
}

template<>
struct has_fixed_size<uav_msgs::action::DockAndCharge_GetResult_Request>
  : std::integral_constant<bool, has_fixed_size<unique_identifier_msgs::msg::UUID>::value> {};

template<>
struct has_bounded_size<uav_msgs::action::DockAndCharge_GetResult_Request>
  : std::integral_constant<bool, has_bounded_size<unique_identifier_msgs::msg::UUID>::value> {};

template<>
struct is_message<uav_msgs::action::DockAndCharge_GetResult_Request>
  : std::true_type {};

}  // namespace rosidl_generator_traits

// Include directives for member types
// Member 'result'
// already included above
// #include "uav_msgs/action/detail/dock_and_charge__traits.hpp"

namespace uav_msgs
{

namespace action
{

inline void to_flow_style_yaml(
  const DockAndCharge_GetResult_Response & msg,
  std::ostream & out)
{
  out << "{";
  // member: status
  {
    out << "status: ";
    rosidl_generator_traits::value_to_yaml(msg.status, out);
    out << ", ";
  }

  // member: result
  {
    out << "result: ";
    to_flow_style_yaml(msg.result, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const DockAndCharge_GetResult_Response & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: status
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "status: ";
    rosidl_generator_traits::value_to_yaml(msg.status, out);
    out << "\n";
  }

  // member: result
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "result:\n";
    to_block_style_yaml(msg.result, out, indentation + 2);
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const DockAndCharge_GetResult_Response & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace action

}  // namespace uav_msgs

namespace rosidl_generator_traits
{

[[deprecated("use uav_msgs::action::to_block_style_yaml() instead")]]
inline void to_yaml(
  const uav_msgs::action::DockAndCharge_GetResult_Response & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::action::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::action::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::action::DockAndCharge_GetResult_Response & msg)
{
  return uav_msgs::action::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::action::DockAndCharge_GetResult_Response>()
{
  return "uav_msgs::action::DockAndCharge_GetResult_Response";
}

template<>
inline const char * name<uav_msgs::action::DockAndCharge_GetResult_Response>()
{
  return "uav_msgs/action/DockAndCharge_GetResult_Response";
}

template<>
struct has_fixed_size<uav_msgs::action::DockAndCharge_GetResult_Response>
  : std::integral_constant<bool, has_fixed_size<uav_msgs::action::DockAndCharge_Result>::value> {};

template<>
struct has_bounded_size<uav_msgs::action::DockAndCharge_GetResult_Response>
  : std::integral_constant<bool, has_bounded_size<uav_msgs::action::DockAndCharge_Result>::value> {};

template<>
struct is_message<uav_msgs::action::DockAndCharge_GetResult_Response>
  : std::true_type {};

}  // namespace rosidl_generator_traits

namespace rosidl_generator_traits
{

template<>
inline const char * data_type<uav_msgs::action::DockAndCharge_GetResult>()
{
  return "uav_msgs::action::DockAndCharge_GetResult";
}

template<>
inline const char * name<uav_msgs::action::DockAndCharge_GetResult>()
{
  return "uav_msgs/action/DockAndCharge_GetResult";
}

template<>
struct has_fixed_size<uav_msgs::action::DockAndCharge_GetResult>
  : std::integral_constant<
    bool,
    has_fixed_size<uav_msgs::action::DockAndCharge_GetResult_Request>::value &&
    has_fixed_size<uav_msgs::action::DockAndCharge_GetResult_Response>::value
  >
{
};

template<>
struct has_bounded_size<uav_msgs::action::DockAndCharge_GetResult>
  : std::integral_constant<
    bool,
    has_bounded_size<uav_msgs::action::DockAndCharge_GetResult_Request>::value &&
    has_bounded_size<uav_msgs::action::DockAndCharge_GetResult_Response>::value
  >
{
};

template<>
struct is_service<uav_msgs::action::DockAndCharge_GetResult>
  : std::true_type
{
};

template<>
struct is_service_request<uav_msgs::action::DockAndCharge_GetResult_Request>
  : std::true_type
{
};

template<>
struct is_service_response<uav_msgs::action::DockAndCharge_GetResult_Response>
  : std::true_type
{
};

}  // namespace rosidl_generator_traits

// Include directives for member types
// Member 'goal_id'
// already included above
// #include "unique_identifier_msgs/msg/detail/uuid__traits.hpp"
// Member 'feedback'
// already included above
// #include "uav_msgs/action/detail/dock_and_charge__traits.hpp"

namespace uav_msgs
{

namespace action
{

inline void to_flow_style_yaml(
  const DockAndCharge_FeedbackMessage & msg,
  std::ostream & out)
{
  out << "{";
  // member: goal_id
  {
    out << "goal_id: ";
    to_flow_style_yaml(msg.goal_id, out);
    out << ", ";
  }

  // member: feedback
  {
    out << "feedback: ";
    to_flow_style_yaml(msg.feedback, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const DockAndCharge_FeedbackMessage & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: goal_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "goal_id:\n";
    to_block_style_yaml(msg.goal_id, out, indentation + 2);
  }

  // member: feedback
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "feedback:\n";
    to_block_style_yaml(msg.feedback, out, indentation + 2);
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const DockAndCharge_FeedbackMessage & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace action

}  // namespace uav_msgs

namespace rosidl_generator_traits
{

[[deprecated("use uav_msgs::action::to_block_style_yaml() instead")]]
inline void to_yaml(
  const uav_msgs::action::DockAndCharge_FeedbackMessage & msg,
  std::ostream & out, size_t indentation = 0)
{
  uav_msgs::action::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use uav_msgs::action::to_yaml() instead")]]
inline std::string to_yaml(const uav_msgs::action::DockAndCharge_FeedbackMessage & msg)
{
  return uav_msgs::action::to_yaml(msg);
}

template<>
inline const char * data_type<uav_msgs::action::DockAndCharge_FeedbackMessage>()
{
  return "uav_msgs::action::DockAndCharge_FeedbackMessage";
}

template<>
inline const char * name<uav_msgs::action::DockAndCharge_FeedbackMessage>()
{
  return "uav_msgs/action/DockAndCharge_FeedbackMessage";
}

template<>
struct has_fixed_size<uav_msgs::action::DockAndCharge_FeedbackMessage>
  : std::integral_constant<bool, has_fixed_size<uav_msgs::action::DockAndCharge_Feedback>::value && has_fixed_size<unique_identifier_msgs::msg::UUID>::value> {};

template<>
struct has_bounded_size<uav_msgs::action::DockAndCharge_FeedbackMessage>
  : std::integral_constant<bool, has_bounded_size<uav_msgs::action::DockAndCharge_Feedback>::value && has_bounded_size<unique_identifier_msgs::msg::UUID>::value> {};

template<>
struct is_message<uav_msgs::action::DockAndCharge_FeedbackMessage>
  : std::true_type {};

}  // namespace rosidl_generator_traits


namespace rosidl_generator_traits
{

template<>
struct is_action<uav_msgs::action::DockAndCharge>
  : std::true_type
{
};

template<>
struct is_action_goal<uav_msgs::action::DockAndCharge_Goal>
  : std::true_type
{
};

template<>
struct is_action_result<uav_msgs::action::DockAndCharge_Result>
  : std::true_type
{
};

template<>
struct is_action_feedback<uav_msgs::action::DockAndCharge_Feedback>
  : std::true_type
{
};

}  // namespace rosidl_generator_traits


#endif  // UAV_MSGS__ACTION__DETAIL__DOCK_AND_CHARGE__TRAITS_HPP_
