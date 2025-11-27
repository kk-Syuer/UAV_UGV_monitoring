// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from uav_msgs:msg/UavDeployment.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__UAV_DEPLOYMENT__STRUCT_H_
#define UAV_MSGS__MSG__DETAIL__UAV_DEPLOYMENT__STRUCT_H_

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
// Member 'cluster_id'
// Member 'ch_id'
// Member 'next_hop_to_sink'
#include "rosidl_runtime_c/string.h"
// Member 'target_pose'
#include "geometry_msgs/msg/detail/pose__struct.h"

/// Struct defined in msg/UavDeployment in the package uav_msgs.
typedef struct uav_msgs__msg__UavDeployment
{
  rosidl_runtime_c__String uav_id;
  geometry_msgs__msg__Pose target_pose;
  /// 0 = MEMBER, 1 = CH
  uint8_t role;
  /// e.g. "cluster_1"
  rosidl_runtime_c__String cluster_id;
  /// for members: their CH; for CHs: self
  rosidl_runtime_c__String ch_id;
  /// Backbone routing: where should this UAV send packets towards the sink?
  /// For CHs: usually another CH id or "sink_gateway".
  /// For members: can be empty.
  rosidl_runtime_c__String next_hop_to_sink;
} uav_msgs__msg__UavDeployment;

// Struct for a sequence of uav_msgs__msg__UavDeployment.
typedef struct uav_msgs__msg__UavDeployment__Sequence
{
  uav_msgs__msg__UavDeployment * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__msg__UavDeployment__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__MSG__DETAIL__UAV_DEPLOYMENT__STRUCT_H_
