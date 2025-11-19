// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from uav_msgs:msg/ChargeRequest.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__CHARGE_REQUEST__STRUCT_H_
#define UAV_MSGS__MSG__DETAIL__CHARGE_REQUEST__STRUCT_H_

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

/// Struct defined in msg/ChargeRequest in the package uav_msgs.
typedef struct uav_msgs__msg__ChargeRequest
{
  rosidl_runtime_c__String uav_id;
  /// 0=MEMBER, 1=CH, 2=BACKUP_CH (if any)
  uint8_t role;
  /// battery percent at request time
  float battery_level;
  /// when the UAV sent the request
  builtin_interfaces__msg__Time stamp;
} uav_msgs__msg__ChargeRequest;

// Struct for a sequence of uav_msgs__msg__ChargeRequest.
typedef struct uav_msgs__msg__ChargeRequest__Sequence
{
  uav_msgs__msg__ChargeRequest * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__msg__ChargeRequest__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__MSG__DETAIL__CHARGE_REQUEST__STRUCT_H_
