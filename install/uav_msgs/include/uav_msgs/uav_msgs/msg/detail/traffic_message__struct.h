// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from uav_msgs:msg/TrafficMessage.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__TRAFFIC_MESSAGE__STRUCT_H_
#define UAV_MSGS__MSG__DETAIL__TRAFFIC_MESSAGE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'msg_id'
// Member 'src_id'
// Member 'dst_id'
// Member 'next_hop_id'
// Member 'control_type'
// Member 'control_payload'
#include "rosidl_runtime_c/string.h"
// Member 'creation_time'
#include "builtin_interfaces/msg/detail/time__struct.h"

/// Struct defined in msg/TrafficMessage in the package uav_msgs.
typedef struct uav_msgs__msg__TrafficMessage
{
  /// unique per message
  rosidl_runtime_c__String msg_id;
  /// "uav_1", "user_3", "ugv", "sink_gateway"
  rosidl_runtime_c__String src_id;
  /// final destination (who should ultimately receive it)
  rosidl_runtime_c__String dst_id;
  /// for this hop, who we are sending to
  rosidl_runtime_c__String next_hop_id;
  /// 0=TEXT, 1=IMAGE, 2=VIDEO_CHUNK, 3=CONTROL_ALERT
  uint8_t msg_type;
  /// 0=low ... 3=emergency (you decide later)
  uint8_t priority;
  uint32_t size_bytes;
  builtin_interfaces__msg__Time creation_time;
  uint32_t hop_count;
  /// optional for CONTROL_ALERT, e.g. "CHARGE_REQUEST"
  rosidl_runtime_c__String control_type;
  /// optional free-form payload (e.g. JSON, key=value, etc.)
  rosidl_runtime_c__String control_payload;
} uav_msgs__msg__TrafficMessage;

// Struct for a sequence of uav_msgs__msg__TrafficMessage.
typedef struct uav_msgs__msg__TrafficMessage__Sequence
{
  uav_msgs__msg__TrafficMessage * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__msg__TrafficMessage__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__MSG__DETAIL__TRAFFIC_MESSAGE__STRUCT_H_
