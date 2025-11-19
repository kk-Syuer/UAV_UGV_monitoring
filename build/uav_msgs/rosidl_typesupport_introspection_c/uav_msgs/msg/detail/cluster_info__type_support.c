// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from uav_msgs:msg/ClusterInfo.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "uav_msgs/msg/detail/cluster_info__rosidl_typesupport_introspection_c.h"
#include "uav_msgs/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "uav_msgs/msg/detail/cluster_info__functions.h"
#include "uav_msgs/msg/detail/cluster_info__struct.h"


// Include directives for member types
// Member `cluster_id`
// Member `ch_id`
// Member `member_ids`
#include "rosidl_runtime_c/string_functions.h"

#ifdef __cplusplus
extern "C"
{
#endif

void uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__ClusterInfo_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  uav_msgs__msg__ClusterInfo__init(message_memory);
}

void uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__ClusterInfo_fini_function(void * message_memory)
{
  uav_msgs__msg__ClusterInfo__fini(message_memory);
}

size_t uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__size_function__ClusterInfo__member_ids(
  const void * untyped_member)
{
  const rosidl_runtime_c__String__Sequence * member =
    (const rosidl_runtime_c__String__Sequence *)(untyped_member);
  return member->size;
}

const void * uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__get_const_function__ClusterInfo__member_ids(
  const void * untyped_member, size_t index)
{
  const rosidl_runtime_c__String__Sequence * member =
    (const rosidl_runtime_c__String__Sequence *)(untyped_member);
  return &member->data[index];
}

void * uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__get_function__ClusterInfo__member_ids(
  void * untyped_member, size_t index)
{
  rosidl_runtime_c__String__Sequence * member =
    (rosidl_runtime_c__String__Sequence *)(untyped_member);
  return &member->data[index];
}

void uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__fetch_function__ClusterInfo__member_ids(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const rosidl_runtime_c__String * item =
    ((const rosidl_runtime_c__String *)
    uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__get_const_function__ClusterInfo__member_ids(untyped_member, index));
  rosidl_runtime_c__String * value =
    (rosidl_runtime_c__String *)(untyped_value);
  *value = *item;
}

void uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__assign_function__ClusterInfo__member_ids(
  void * untyped_member, size_t index, const void * untyped_value)
{
  rosidl_runtime_c__String * item =
    ((rosidl_runtime_c__String *)
    uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__get_function__ClusterInfo__member_ids(untyped_member, index));
  const rosidl_runtime_c__String * value =
    (const rosidl_runtime_c__String *)(untyped_value);
  *item = *value;
}

bool uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__resize_function__ClusterInfo__member_ids(
  void * untyped_member, size_t size)
{
  rosidl_runtime_c__String__Sequence * member =
    (rosidl_runtime_c__String__Sequence *)(untyped_member);
  rosidl_runtime_c__String__Sequence__fini(member);
  return rosidl_runtime_c__String__Sequence__init(member, size);
}

static rosidl_typesupport_introspection_c__MessageMember uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__ClusterInfo_message_member_array[3] = {
  {
    "cluster_id",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__ClusterInfo, cluster_id),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "ch_id",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__ClusterInfo, ch_id),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "member_ids",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__ClusterInfo, member_ids),  // bytes offset in struct
    NULL,  // default value
    uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__size_function__ClusterInfo__member_ids,  // size() function pointer
    uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__get_const_function__ClusterInfo__member_ids,  // get_const(index) function pointer
    uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__get_function__ClusterInfo__member_ids,  // get(index) function pointer
    uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__fetch_function__ClusterInfo__member_ids,  // fetch(index, &value) function pointer
    uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__assign_function__ClusterInfo__member_ids,  // assign(index, value) function pointer
    uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__resize_function__ClusterInfo__member_ids  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__ClusterInfo_message_members = {
  "uav_msgs__msg",  // message namespace
  "ClusterInfo",  // message name
  3,  // number of fields
  sizeof(uav_msgs__msg__ClusterInfo),
  uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__ClusterInfo_message_member_array,  // message members
  uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__ClusterInfo_init_function,  // function to initialize message memory (memory has to be allocated)
  uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__ClusterInfo_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__ClusterInfo_message_type_support_handle = {
  0,
  &uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__ClusterInfo_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_uav_msgs
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, uav_msgs, msg, ClusterInfo)() {
  if (!uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__ClusterInfo_message_type_support_handle.typesupport_identifier) {
    uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__ClusterInfo_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &uav_msgs__msg__ClusterInfo__rosidl_typesupport_introspection_c__ClusterInfo_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
