// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from uav_msgs:srv/RequestCharge.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__SRV__DETAIL__REQUEST_CHARGE__STRUCT_H_
#define UAV_MSGS__SRV__DETAIL__REQUEST_CHARGE__STRUCT_H_

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

/// Struct defined in srv/RequestCharge in the package uav_msgs.
typedef struct uav_msgs__srv__RequestCharge_Request
{
  rosidl_runtime_c__String uav_id;
  float battery_level;
  /// 0=MEMBER, 1=CH, 2=BACKUP_CH
  uint8_t role;
} uav_msgs__srv__RequestCharge_Request;

// Struct for a sequence of uav_msgs__srv__RequestCharge_Request.
typedef struct uav_msgs__srv__RequestCharge_Request__Sequence
{
  uav_msgs__srv__RequestCharge_Request * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__srv__RequestCharge_Request__Sequence;


// Constants defined in the message

// Include directives for member types
// Member 'eta'
#include "builtin_interfaces/msg/detail/time__struct.h"

/// Struct defined in srv/RequestCharge in the package uav_msgs.
typedef struct uav_msgs__srv__RequestCharge_Response
{
  bool accepted;
  /// estimated charging start time
  builtin_interfaces__msg__Time eta;
  uint8_t assigned_priority;
} uav_msgs__srv__RequestCharge_Response;

// Struct for a sequence of uav_msgs__srv__RequestCharge_Response.
typedef struct uav_msgs__srv__RequestCharge_Response__Sequence
{
  uav_msgs__srv__RequestCharge_Response * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__srv__RequestCharge_Response__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__SRV__DETAIL__REQUEST_CHARGE__STRUCT_H_
