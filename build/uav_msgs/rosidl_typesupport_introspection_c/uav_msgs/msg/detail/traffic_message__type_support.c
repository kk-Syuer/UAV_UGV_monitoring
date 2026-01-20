// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from uav_msgs:msg/TrafficMessage.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "uav_msgs/msg/detail/traffic_message__rosidl_typesupport_introspection_c.h"
#include "uav_msgs/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "uav_msgs/msg/detail/traffic_message__functions.h"
#include "uav_msgs/msg/detail/traffic_message__struct.h"


// Include directives for member types
// Member `msg_id`
// Member `src_id`
// Member `dst_id`
// Member `next_hop_id`
// Member `last_hop_id`
// Member `control_type`
// Member `payload`
// Member `ref_msg_id`
// Member `drop_reason`
// Member `recent_hops`
#include "rosidl_runtime_c/string_functions.h"
// Member `creation_time`
// Member `last_tx_time`
// Member `last_rx_time`
#include "builtin_interfaces/msg/time.h"
// Member `creation_time`
// Member `last_tx_time`
// Member `last_rx_time`
#include "builtin_interfaces/msg/detail/time__rosidl_typesupport_introspection_c.h"

#ifdef __cplusplus
extern "C"
{
#endif

void uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  uav_msgs__msg__TrafficMessage__init(message_memory);
}

void uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_fini_function(void * message_memory)
{
  uav_msgs__msg__TrafficMessage__fini(message_memory);
}

size_t uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__size_function__TrafficMessage__recent_hops(
  const void * untyped_member)
{
  const rosidl_runtime_c__String__Sequence * member =
    (const rosidl_runtime_c__String__Sequence *)(untyped_member);
  return member->size;
}

const void * uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__get_const_function__TrafficMessage__recent_hops(
  const void * untyped_member, size_t index)
{
  const rosidl_runtime_c__String__Sequence * member =
    (const rosidl_runtime_c__String__Sequence *)(untyped_member);
  return &member->data[index];
}

void * uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__get_function__TrafficMessage__recent_hops(
  void * untyped_member, size_t index)
{
  rosidl_runtime_c__String__Sequence * member =
    (rosidl_runtime_c__String__Sequence *)(untyped_member);
  return &member->data[index];
}

void uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__fetch_function__TrafficMessage__recent_hops(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const rosidl_runtime_c__String * item =
    ((const rosidl_runtime_c__String *)
    uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__get_const_function__TrafficMessage__recent_hops(untyped_member, index));
  rosidl_runtime_c__String * value =
    (rosidl_runtime_c__String *)(untyped_value);
  *value = *item;
}

void uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__assign_function__TrafficMessage__recent_hops(
  void * untyped_member, size_t index, const void * untyped_value)
{
  rosidl_runtime_c__String * item =
    ((rosidl_runtime_c__String *)
    uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__get_function__TrafficMessage__recent_hops(untyped_member, index));
  const rosidl_runtime_c__String * value =
    (const rosidl_runtime_c__String *)(untyped_value);
  *item = *value;
}

bool uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__resize_function__TrafficMessage__recent_hops(
  void * untyped_member, size_t size)
{
  rosidl_runtime_c__String__Sequence * member =
    (rosidl_runtime_c__String__Sequence *)(untyped_member);
  rosidl_runtime_c__String__Sequence__fini(member);
  return rosidl_runtime_c__String__Sequence__init(member, size);
}

static rosidl_typesupport_introspection_c__MessageMember uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_member_array[18] = {
  {
    "msg_id",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, msg_id),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "src_id",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, src_id),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "dst_id",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, dst_id),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "next_hop_id",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, next_hop_id),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "last_hop_id",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, last_hop_id),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "flow_type",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, flow_type),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "control_type",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, control_type),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "seq",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, seq),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "hop_count",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, hop_count),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "ttl",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, ttl),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "requires_ack",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_BOOLEAN,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, requires_ack),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "payload",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, payload),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "creation_time",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, creation_time),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "ref_msg_id",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, ref_msg_id),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "last_tx_time",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, last_tx_time),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "last_rx_time",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, last_rx_time),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "drop_reason",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, drop_reason),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "recent_hops",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, recent_hops),  // bytes offset in struct
    NULL,  // default value
    uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__size_function__TrafficMessage__recent_hops,  // size() function pointer
    uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__get_const_function__TrafficMessage__recent_hops,  // get_const(index) function pointer
    uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__get_function__TrafficMessage__recent_hops,  // get(index) function pointer
    uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__fetch_function__TrafficMessage__recent_hops,  // fetch(index, &value) function pointer
    uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__assign_function__TrafficMessage__recent_hops,  // assign(index, value) function pointer
    uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__resize_function__TrafficMessage__recent_hops  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_members = {
  "uav_msgs__msg",  // message namespace
  "TrafficMessage",  // message name
  18,  // number of fields
  sizeof(uav_msgs__msg__TrafficMessage),
  uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_member_array,  // message members
  uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_init_function,  // function to initialize message memory (memory has to be allocated)
  uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_type_support_handle = {
  0,
  &uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_uav_msgs
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, uav_msgs, msg, TrafficMessage)() {
  uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_member_array[12].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, builtin_interfaces, msg, Time)();
  uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_member_array[14].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, builtin_interfaces, msg, Time)();
  uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_member_array[15].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, builtin_interfaces, msg, Time)();
  if (!uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_type_support_handle.typesupport_identifier) {
    uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
