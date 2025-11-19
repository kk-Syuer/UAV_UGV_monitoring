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
#include "rosidl_runtime_c/string_functions.h"
// Member `creation_time`
#include "builtin_interfaces/msg/time.h"
// Member `creation_time`
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

static rosidl_typesupport_introspection_c__MessageMember uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_member_array[8] = {
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
    "msg_type",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, msg_type),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "priority",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, priority),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "size_bytes",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__TrafficMessage, size_bytes),  // bytes offset in struct
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
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_members = {
  "uav_msgs__msg",  // message namespace
  "TrafficMessage",  // message name
  8,  // number of fields
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
  uav_msgs__msg__TrafficMessage__rosidl_typesupport_introspection_c__TrafficMessage_message_member_array[6].members_ =
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
