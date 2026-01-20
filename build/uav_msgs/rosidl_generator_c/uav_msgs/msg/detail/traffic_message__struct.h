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
// Member 'last_hop_id'
// Member 'control_type'
// Member 'payload'
// Member 'ref_msg_id'
// Member 'drop_reason'
// Member 'recent_hops'
#include "rosidl_runtime_c/string.h"
// Member 'creation_time'
// Member 'last_tx_time'
// Member 'last_rx_time'
#include "builtin_interfaces/msg/detail/time__struct.h"

/// Struct defined in msg/TrafficMessage in the package uav_msgs.
typedef struct uav_msgs__msg__TrafficMessage
{
  /// optional human-readable id for logging
  rosidl_runtime_c__String msg_id;
  /// "uav_1", "user_3", "ugv_1", "sink"
  rosidl_runtime_c__String src_id;
  /// final destination (who should ultimately receive it)
  rosidl_runtime_c__String dst_id;
  /// for this hop, who we are sending to (empty for broadcast)
  rosidl_runtime_c__String next_hop_id;
  /// who forwarded this packet most recently
  rosidl_runtime_c__String last_hop_id;
  /// 0=DATA, 1=CONTROL
  uint8_t flow_type;
  /// NONE, HEARTBEAT, CHARGE_REQUEST, CHARGE_DECISION, DEPLOYMENT, DEPLOYMENT_ACK, MOTION_START, DROP, DEBUG_TEXT, STATUS_CH
  rosidl_runtime_c__String control_type;
  /// sequence number (per src + control_type)
  uint32_t seq;
  uint32_t hop_count;
  /// max hops allowed (0 = unlimited)
  uint32_t ttl;
  /// whether the receiver should ACK this control message
  bool requires_ack;
  /// serialized application payload (JSON, CSV, plain text, ...)
  rosidl_runtime_c__String payload;
  builtin_interfaces__msg__Time creation_time;
  /// optional: references another message (e.g., ACK/DROP/DECISION)
  rosidl_runtime_c__String ref_msg_id;
  /// when the previous hop transmitted
  builtin_interfaces__msg__Time last_tx_time;
  /// when this hop received it
  builtin_interfaces__msg__Time last_rx_time;
  /// if control_type == DROP, why it was dropped
  rosidl_runtime_c__String drop_reason;
  /// history of forwarders (optional, bounded by producer)
  rosidl_runtime_c__String__Sequence recent_hops;
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
