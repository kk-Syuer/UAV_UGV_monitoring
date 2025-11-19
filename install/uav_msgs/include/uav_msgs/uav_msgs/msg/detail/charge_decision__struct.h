// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from uav_msgs:msg/ChargeDecision.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__STRUCT_H_
#define UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'uav_id'
// Member 'policy'
#include "rosidl_runtime_c/string.h"
// Member 'slot_start_time'
#include "builtin_interfaces/msg/detail/time__struct.h"

/// Struct defined in msg/ChargeDecision in the package uav_msgs.
typedef struct uav_msgs__msg__ChargeDecision
{
  /// which UAV this decision is for
  rosidl_runtime_c__String uav_id;
  /// whether it got a charging slot
  bool accepted;
  /// when it should start charging
  builtin_interfaces__msg__Time slot_start_time;
  /// priority level (e.g. 0=member,1=CH)
  uint8_t priority;
  /// policy name used (fcfs, role_priority, ...)
  rosidl_runtime_c__String policy;
} uav_msgs__msg__ChargeDecision;

// Struct for a sequence of uav_msgs__msg__ChargeDecision.
typedef struct uav_msgs__msg__ChargeDecision__Sequence
{
  uav_msgs__msg__ChargeDecision * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__msg__ChargeDecision__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__STRUCT_H_
