// generated from rosidl_typesupport_fastrtps_cpp/resource/idl__rosidl_typesupport_fastrtps_cpp.hpp.em
// with input from uav_msgs:msg/ChargeDecision.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__ROSIDL_TYPESUPPORT_FASTRTPS_CPP_HPP_
#define UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__ROSIDL_TYPESUPPORT_FASTRTPS_CPP_HPP_

#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"
#include "uav_msgs/msg/rosidl_typesupport_fastrtps_cpp__visibility_control.h"
#include "uav_msgs/msg/detail/charge_decision__struct.hpp"

#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# ifdef __clang__
#  pragma clang diagnostic ignored "-Wdeprecated-register"
#  pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
# endif
#endif
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif

#include "fastcdr/Cdr.h"

namespace uav_msgs
{

namespace msg
{

namespace typesupport_fastrtps_cpp
{

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_uav_msgs
cdr_serialize(
  const uav_msgs::msg::ChargeDecision & ros_message,
  eprosima::fastcdr::Cdr & cdr);

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_uav_msgs
cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  uav_msgs::msg::ChargeDecision & ros_message);

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_uav_msgs
get_serialized_size(
  const uav_msgs::msg::ChargeDecision & ros_message,
  size_t current_alignment);

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_uav_msgs
max_serialized_size_ChargeDecision(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

}  // namespace typesupport_fastrtps_cpp

}  // namespace msg

}  // namespace uav_msgs

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_uav_msgs
const rosidl_message_type_support_t *
  ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_cpp, uav_msgs, msg, ChargeDecision)();

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__ROSIDL_TYPESUPPORT_FASTRTPS_CPP_HPP_
