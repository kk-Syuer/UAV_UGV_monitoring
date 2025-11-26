// generated from rosidl_typesupport_fastrtps_c/resource/idl__type_support_c.cpp.em
// with input from uav_msgs:msg/UavDeployment.idl
// generated code does not contain a copyright notice
#include "uav_msgs/msg/detail/uav_deployment__rosidl_typesupport_fastrtps_c.h"


#include <cassert>
#include <limits>
#include <string>
#include "rosidl_typesupport_fastrtps_c/identifier.h"
#include "rosidl_typesupport_fastrtps_c/wstring_conversion.hpp"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "uav_msgs/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "uav_msgs/msg/detail/uav_deployment__struct.h"
#include "uav_msgs/msg/detail/uav_deployment__functions.h"
#include "fastcdr/Cdr.h"

#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# ifdef __clang__
#  pragma clang diagnostic ignored "-Wdeprecated-register"
#  pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
# endif
#endif
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif

// includes and forward declarations of message dependencies and their conversion functions

#if defined(__cplusplus)
extern "C"
{
#endif

#include "geometry_msgs/msg/detail/pose__functions.h"  // target_pose
#include "rosidl_runtime_c/string.h"  // ch_id, cluster_id, uav_id
#include "rosidl_runtime_c/string_functions.h"  // ch_id, cluster_id, uav_id

// forward declare type support functions
ROSIDL_TYPESUPPORT_FASTRTPS_C_IMPORT_uav_msgs
size_t get_serialized_size_geometry_msgs__msg__Pose(
  const void * untyped_ros_message,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_IMPORT_uav_msgs
size_t max_serialized_size_geometry_msgs__msg__Pose(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_IMPORT_uav_msgs
const rosidl_message_type_support_t *
  ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, geometry_msgs, msg, Pose)();


using _UavDeployment__ros_msg_type = uav_msgs__msg__UavDeployment;

static bool _UavDeployment__cdr_serialize(
  const void * untyped_ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  const _UavDeployment__ros_msg_type * ros_message = static_cast<const _UavDeployment__ros_msg_type *>(untyped_ros_message);
  // Field name: uav_id
  {
    const rosidl_runtime_c__String * str = &ros_message->uav_id;
    if (str->capacity == 0 || str->capacity <= str->size) {
      fprintf(stderr, "string capacity not greater than size\n");
      return false;
    }
    if (str->data[str->size] != '\0') {
      fprintf(stderr, "string not null-terminated\n");
      return false;
    }
    cdr << str->data;
  }

  // Field name: target_pose
  {
    const message_type_support_callbacks_t * callbacks =
      static_cast<const message_type_support_callbacks_t *>(
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
        rosidl_typesupport_fastrtps_c, geometry_msgs, msg, Pose
      )()->data);
    if (!callbacks->cdr_serialize(
        &ros_message->target_pose, cdr))
    {
      return false;
    }
  }

  // Field name: role
  {
    cdr << ros_message->role;
  }

  // Field name: cluster_id
  {
    const rosidl_runtime_c__String * str = &ros_message->cluster_id;
    if (str->capacity == 0 || str->capacity <= str->size) {
      fprintf(stderr, "string capacity not greater than size\n");
      return false;
    }
    if (str->data[str->size] != '\0') {
      fprintf(stderr, "string not null-terminated\n");
      return false;
    }
    cdr << str->data;
  }

  // Field name: ch_id
  {
    const rosidl_runtime_c__String * str = &ros_message->ch_id;
    if (str->capacity == 0 || str->capacity <= str->size) {
      fprintf(stderr, "string capacity not greater than size\n");
      return false;
    }
    if (str->data[str->size] != '\0') {
      fprintf(stderr, "string not null-terminated\n");
      return false;
    }
    cdr << str->data;
  }

  return true;
}

static bool _UavDeployment__cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  void * untyped_ros_message)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  _UavDeployment__ros_msg_type * ros_message = static_cast<_UavDeployment__ros_msg_type *>(untyped_ros_message);
  // Field name: uav_id
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->uav_id.data) {
      rosidl_runtime_c__String__init(&ros_message->uav_id);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->uav_id,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'uav_id'\n");
      return false;
    }
  }

  // Field name: target_pose
  {
    const message_type_support_callbacks_t * callbacks =
      static_cast<const message_type_support_callbacks_t *>(
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
        rosidl_typesupport_fastrtps_c, geometry_msgs, msg, Pose
      )()->data);
    if (!callbacks->cdr_deserialize(
        cdr, &ros_message->target_pose))
    {
      return false;
    }
  }

  // Field name: role
  {
    cdr >> ros_message->role;
  }

  // Field name: cluster_id
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->cluster_id.data) {
      rosidl_runtime_c__String__init(&ros_message->cluster_id);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->cluster_id,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'cluster_id'\n");
      return false;
    }
  }

  // Field name: ch_id
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->ch_id.data) {
      rosidl_runtime_c__String__init(&ros_message->ch_id);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->ch_id,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'ch_id'\n");
      return false;
    }
  }

  return true;
}  // NOLINT(readability/fn_size)

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_uav_msgs
size_t get_serialized_size_uav_msgs__msg__UavDeployment(
  const void * untyped_ros_message,
  size_t current_alignment)
{
  const _UavDeployment__ros_msg_type * ros_message = static_cast<const _UavDeployment__ros_msg_type *>(untyped_ros_message);
  (void)ros_message;
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  (void)padding;
  (void)wchar_size;

  // field.name uav_id
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->uav_id.size + 1);
  // field.name target_pose

  current_alignment += get_serialized_size_geometry_msgs__msg__Pose(
    &(ros_message->target_pose), current_alignment);
  // field.name role
  {
    size_t item_size = sizeof(ros_message->role);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name cluster_id
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->cluster_id.size + 1);
  // field.name ch_id
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->ch_id.size + 1);

  return current_alignment - initial_alignment;
}

static uint32_t _UavDeployment__get_serialized_size(const void * untyped_ros_message)
{
  return static_cast<uint32_t>(
    get_serialized_size_uav_msgs__msg__UavDeployment(
      untyped_ros_message, 0));
}

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_uav_msgs
size_t max_serialized_size_uav_msgs__msg__UavDeployment(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment)
{
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  size_t last_member_size = 0;
  (void)last_member_size;
  (void)padding;
  (void)wchar_size;

  full_bounded = true;
  is_plain = true;

  // member: uav_id
  {
    size_t array_size = 1;

    full_bounded = false;
    is_plain = false;
    for (size_t index = 0; index < array_size; ++index) {
      current_alignment += padding +
        eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
        1;
    }
  }
  // member: target_pose
  {
    size_t array_size = 1;


    last_member_size = 0;
    for (size_t index = 0; index < array_size; ++index) {
      bool inner_full_bounded;
      bool inner_is_plain;
      size_t inner_size;
      inner_size =
        max_serialized_size_geometry_msgs__msg__Pose(
        inner_full_bounded, inner_is_plain, current_alignment);
      last_member_size += inner_size;
      current_alignment += inner_size;
      full_bounded &= inner_full_bounded;
      is_plain &= inner_is_plain;
    }
  }
  // member: role
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }
  // member: cluster_id
  {
    size_t array_size = 1;

    full_bounded = false;
    is_plain = false;
    for (size_t index = 0; index < array_size; ++index) {
      current_alignment += padding +
        eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
        1;
    }
  }
  // member: ch_id
  {
    size_t array_size = 1;

    full_bounded = false;
    is_plain = false;
    for (size_t index = 0; index < array_size; ++index) {
      current_alignment += padding +
        eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
        1;
    }
  }

  size_t ret_val = current_alignment - initial_alignment;
  if (is_plain) {
    // All members are plain, and type is not empty.
    // We still need to check that the in-memory alignment
    // is the same as the CDR mandated alignment.
    using DataType = uav_msgs__msg__UavDeployment;
    is_plain =
      (
      offsetof(DataType, ch_id) +
      last_member_size
      ) == ret_val;
  }

  return ret_val;
}

static size_t _UavDeployment__max_serialized_size(char & bounds_info)
{
  bool full_bounded;
  bool is_plain;
  size_t ret_val;

  ret_val = max_serialized_size_uav_msgs__msg__UavDeployment(
    full_bounded, is_plain, 0);

  bounds_info =
    is_plain ? ROSIDL_TYPESUPPORT_FASTRTPS_PLAIN_TYPE :
    full_bounded ? ROSIDL_TYPESUPPORT_FASTRTPS_BOUNDED_TYPE : ROSIDL_TYPESUPPORT_FASTRTPS_UNBOUNDED_TYPE;
  return ret_val;
}


static message_type_support_callbacks_t __callbacks_UavDeployment = {
  "uav_msgs::msg",
  "UavDeployment",
  _UavDeployment__cdr_serialize,
  _UavDeployment__cdr_deserialize,
  _UavDeployment__get_serialized_size,
  _UavDeployment__max_serialized_size
};

static rosidl_message_type_support_t _UavDeployment__type_support = {
  rosidl_typesupport_fastrtps_c__identifier,
  &__callbacks_UavDeployment,
  get_message_typesupport_handle_function,
};

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, uav_msgs, msg, UavDeployment)() {
  return &_UavDeployment__type_support;
}

#if defined(__cplusplus)
}
#endif
