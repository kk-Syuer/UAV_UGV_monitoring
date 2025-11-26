// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from uav_msgs:srv/SendDebugText.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__SRV__DETAIL__SEND_DEBUG_TEXT__STRUCT_H_
#define UAV_MSGS__SRV__DETAIL__SEND_DEBUG_TEXT__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'dst_id'
// Member 'text'
#include "rosidl_runtime_c/string.h"

/// Struct defined in srv/SendDebugText in the package uav_msgs.
typedef struct uav_msgs__srv__SendDebugText_Request
{
  rosidl_runtime_c__String dst_id;
  rosidl_runtime_c__String text;
} uav_msgs__srv__SendDebugText_Request;

// Struct for a sequence of uav_msgs__srv__SendDebugText_Request.
typedef struct uav_msgs__srv__SendDebugText_Request__Sequence
{
  uav_msgs__srv__SendDebugText_Request * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__srv__SendDebugText_Request__Sequence;


// Constants defined in the message

// Include directives for member types
// Member 'info'
// already included above
// #include "rosidl_runtime_c/string.h"

/// Struct defined in srv/SendDebugText in the package uav_msgs.
typedef struct uav_msgs__srv__SendDebugText_Response
{
  bool accepted;
  rosidl_runtime_c__String info;
} uav_msgs__srv__SendDebugText_Response;

// Struct for a sequence of uav_msgs__srv__SendDebugText_Response.
typedef struct uav_msgs__srv__SendDebugText_Response__Sequence
{
  uav_msgs__srv__SendDebugText_Response * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__srv__SendDebugText_Response__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__SRV__DETAIL__SEND_DEBUG_TEXT__STRUCT_H_
