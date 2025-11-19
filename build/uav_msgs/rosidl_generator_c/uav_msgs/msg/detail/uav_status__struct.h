// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from uav_msgs:msg/UavStatus.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__UAV_STATUS__STRUCT_H_
#define UAV_MSGS__MSG__DETAIL__UAV_STATUS__STRUCT_H_

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
#include "rosidl_runtime_c/string.h"
// Member 'pose'
#include "geometry_msgs/msg/detail/pose__struct.h"
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__struct.h"

/// Struct defined in msg/UavStatus in the package uav_msgs.
typedef struct uav_msgs__msg__UavStatus
{
  /// "uav_1", "uav_2", ...
  rosidl_runtime_c__String uav_id;
  /// 0 = MEMBER, 1 = CH, 2 = BACKUP_CH
  uint8_t role;
  /// e.g. "cluster_1"
  rosidl_runtime_c__String cluster_id;
  /// 0.0 - 100.0 (%)
  float battery_level;
  /// optional, in Wh or similar
  float battery_capacity;
  geometry_msgs__msg__Pose pose;
  /// meters (CH > MEMBER)
  float service_radius;
  /// number of user devices associated
  uint32_t connected_users;
  /// e.g. kbps or some abstract load
  float traffic_load;
  float packet_loss_estimate;
  float energy_consumption_rate;
  builtin_interfaces__msg__Time stamp;
} uav_msgs__msg__UavStatus;

// Struct for a sequence of uav_msgs__msg__UavStatus.
typedef struct uav_msgs__msg__UavStatus__Sequence
{
  uav_msgs__msg__UavStatus * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__msg__UavStatus__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__MSG__DETAIL__UAV_STATUS__STRUCT_H_
