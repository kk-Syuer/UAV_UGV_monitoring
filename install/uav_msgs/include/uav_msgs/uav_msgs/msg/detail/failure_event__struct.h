// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from uav_msgs:msg/FailureEvent.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__FAILURE_EVENT__STRUCT_H_
#define UAV_MSGS__MSG__DETAIL__FAILURE_EVENT__STRUCT_H_

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
// Member 'description'
#include "rosidl_runtime_c/string.h"
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__struct.h"

/// Struct defined in msg/FailureEvent in the package uav_msgs.
typedef struct uav_msgs__msg__FailureEvent
{
  rosidl_runtime_c__String uav_id;
  /// 0=UNKNOWN, 1=BATTERY_DEAD, 2=OTHER
  uint8_t failure_type;
  rosidl_runtime_c__String description;
  builtin_interfaces__msg__Time stamp;
} uav_msgs__msg__FailureEvent;

// Struct for a sequence of uav_msgs__msg__FailureEvent.
typedef struct uav_msgs__msg__FailureEvent__Sequence
{
  uav_msgs__msg__FailureEvent * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__msg__FailureEvent__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__MSG__DETAIL__FAILURE_EVENT__STRUCT_H_
