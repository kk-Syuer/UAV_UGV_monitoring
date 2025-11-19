// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from uav_msgs:msg/ClusterInfo.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__CLUSTER_INFO__STRUCT_H_
#define UAV_MSGS__MSG__DETAIL__CLUSTER_INFO__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'cluster_id'
// Member 'ch_id'
// Member 'member_ids'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/ClusterInfo in the package uav_msgs.
typedef struct uav_msgs__msg__ClusterInfo
{
  rosidl_runtime_c__String cluster_id;
  rosidl_runtime_c__String ch_id;
  rosidl_runtime_c__String__Sequence member_ids;
} uav_msgs__msg__ClusterInfo;

// Struct for a sequence of uav_msgs__msg__ClusterInfo.
typedef struct uav_msgs__msg__ClusterInfo__Sequence
{
  uav_msgs__msg__ClusterInfo * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__msg__ClusterInfo__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__MSG__DETAIL__CLUSTER_INFO__STRUCT_H_
