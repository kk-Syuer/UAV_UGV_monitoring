// generated from rosidl_typesupport_fastrtps_c/resource/idl__type_support_c.cpp.em
// with input from uav_msgs:msg/ClusterInfo.idl
// generated code does not contain a copyright notice
#include "uav_msgs/msg/detail/cluster_info__rosidl_typesupport_fastrtps_c.h"


#include <cassert>
#include <limits>
#include <string>
#include "rosidl_typesupport_fastrtps_c/identifier.h"
#include "rosidl_typesupport_fastrtps_c/wstring_conversion.hpp"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "uav_msgs/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "uav_msgs/msg/detail/cluster_info__struct.h"
#include "uav_msgs/msg/detail/cluster_info__functions.h"
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

#include "rosidl_runtime_c/string.h"  // ch_id, cluster_id, member_ids
#include "rosidl_runtime_c/string_functions.h"  // ch_id, cluster_id, member_ids

// forward declare type support functions


using _ClusterInfo__ros_msg_type = uav_msgs__msg__ClusterInfo;

static bool _ClusterInfo__cdr_serialize(
  const void * untyped_ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  const _ClusterInfo__ros_msg_type * ros_message = static_cast<const _ClusterInfo__ros_msg_type *>(untyped_ros_message);
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

  // Field name: member_ids
  {
    size_t size = ros_message->member_ids.size;
    auto array_ptr = ros_message->member_ids.data;
    cdr << static_cast<uint32_t>(size);
    for (size_t i = 0; i < size; ++i) {
      const rosidl_runtime_c__String * str = &array_ptr[i];
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
  }

  return true;
}

static bool _ClusterInfo__cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  void * untyped_ros_message)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  _ClusterInfo__ros_msg_type * ros_message = static_cast<_ClusterInfo__ros_msg_type *>(untyped_ros_message);
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

  // Field name: member_ids
  {
    uint32_t cdrSize;
    cdr >> cdrSize;
    size_t size = static_cast<size_t>(cdrSize);
    if (ros_message->member_ids.data) {
      rosidl_runtime_c__String__Sequence__fini(&ros_message->member_ids);
    }
    if (!rosidl_runtime_c__String__Sequence__init(&ros_message->member_ids, size)) {
      fprintf(stderr, "failed to create array for field 'member_ids'");
      return false;
    }
    auto array_ptr = ros_message->member_ids.data;
    for (size_t i = 0; i < size; ++i) {
      std::string tmp;
      cdr >> tmp;
      auto & ros_i = array_ptr[i];
      if (!ros_i.data) {
        rosidl_runtime_c__String__init(&ros_i);
      }
      bool succeeded = rosidl_runtime_c__String__assign(
        &ros_i,
        tmp.c_str());
      if (!succeeded) {
        fprintf(stderr, "failed to assign string into field 'member_ids'\n");
        return false;
      }
    }
  }

  return true;
}  // NOLINT(readability/fn_size)

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_uav_msgs
size_t get_serialized_size_uav_msgs__msg__ClusterInfo(
  const void * untyped_ros_message,
  size_t current_alignment)
{
  const _ClusterInfo__ros_msg_type * ros_message = static_cast<const _ClusterInfo__ros_msg_type *>(untyped_ros_message);
  (void)ros_message;
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  (void)padding;
  (void)wchar_size;

  // field.name cluster_id
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->cluster_id.size + 1);
  // field.name ch_id
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->ch_id.size + 1);
  // field.name member_ids
  {
    size_t array_size = ros_message->member_ids.size;
    auto array_ptr = ros_message->member_ids.data;
    current_alignment += padding +
      eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
    for (size_t index = 0; index < array_size; ++index) {
      current_alignment += padding +
        eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
        (array_ptr[index].size + 1);
    }
  }

  return current_alignment - initial_alignment;
}

static uint32_t _ClusterInfo__get_serialized_size(const void * untyped_ros_message)
{
  return static_cast<uint32_t>(
    get_serialized_size_uav_msgs__msg__ClusterInfo(
      untyped_ros_message, 0));
}

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_uav_msgs
size_t max_serialized_size_uav_msgs__msg__ClusterInfo(
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
  // member: member_ids
  {
    size_t array_size = 0;
    full_bounded = false;
    is_plain = false;
    current_alignment += padding +
      eprosima::fastcdr::Cdr::alignment(current_alignment, padding);

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
    using DataType = uav_msgs__msg__ClusterInfo;
    is_plain =
      (
      offsetof(DataType, member_ids) +
      last_member_size
      ) == ret_val;
  }

  return ret_val;
}

static size_t _ClusterInfo__max_serialized_size(char & bounds_info)
{
  bool full_bounded;
  bool is_plain;
  size_t ret_val;

  ret_val = max_serialized_size_uav_msgs__msg__ClusterInfo(
    full_bounded, is_plain, 0);

  bounds_info =
    is_plain ? ROSIDL_TYPESUPPORT_FASTRTPS_PLAIN_TYPE :
    full_bounded ? ROSIDL_TYPESUPPORT_FASTRTPS_BOUNDED_TYPE : ROSIDL_TYPESUPPORT_FASTRTPS_UNBOUNDED_TYPE;
  return ret_val;
}


static message_type_support_callbacks_t __callbacks_ClusterInfo = {
  "uav_msgs::msg",
  "ClusterInfo",
  _ClusterInfo__cdr_serialize,
  _ClusterInfo__cdr_deserialize,
  _ClusterInfo__get_serialized_size,
  _ClusterInfo__max_serialized_size
};

static rosidl_message_type_support_t _ClusterInfo__type_support = {
  rosidl_typesupport_fastrtps_c__identifier,
  &__callbacks_ClusterInfo,
  get_message_typesupport_handle_function,
};

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, uav_msgs, msg, ClusterInfo)() {
  return &_ClusterInfo__type_support;
}

#if defined(__cplusplus)
}
#endif
