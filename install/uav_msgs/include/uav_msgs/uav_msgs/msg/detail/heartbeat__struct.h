// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from uav_msgs:msg/Heartbeat.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__HEARTBEAT__STRUCT_H_
#define UAV_MSGS__MSG__DETAIL__HEARTBEAT__STRUCT_H_

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
#include "rosidl_runtime_c/string.h"
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__struct.h"

/// Struct defined in msg/Heartbeat in the package uav_msgs.
typedef struct uav_msgs__msg__Heartbeat
{
  rosidl_runtime_c__String uav_id;
  builtin_interfaces__msg__Time stamp;
} uav_msgs__msg__Heartbeat;

// Struct for a sequence of uav_msgs__msg__Heartbeat.
typedef struct uav_msgs__msg__Heartbeat__Sequence
{
  uav_msgs__msg__Heartbeat * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__msg__Heartbeat__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__MSG__DETAIL__HEARTBEAT__STRUCT_H_
