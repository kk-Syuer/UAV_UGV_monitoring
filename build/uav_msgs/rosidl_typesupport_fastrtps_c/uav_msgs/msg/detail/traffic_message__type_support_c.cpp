// generated from rosidl_typesupport_fastrtps_c/resource/idl__type_support_c.cpp.em
// with input from uav_msgs:msg/TrafficMessage.idl
// generated code does not contain a copyright notice
#include "uav_msgs/msg/detail/traffic_message__rosidl_typesupport_fastrtps_c.h"


#include <cassert>
#include <limits>
#include <string>
#include "rosidl_typesupport_fastrtps_c/identifier.h"
#include "rosidl_typesupport_fastrtps_c/wstring_conversion.hpp"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "uav_msgs/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "uav_msgs/msg/detail/traffic_message__struct.h"
#include "uav_msgs/msg/detail/traffic_message__functions.h"
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

#include "builtin_interfaces/msg/detail/time__functions.h"  // creation_time, last_rx_time, last_tx_time
#include "rosidl_runtime_c/string.h"  // control_type, drop_reason, dst_id, last_hop_id, msg_id, next_hop_id, payload, recent_hops, ref_msg_id, src_id
#include "rosidl_runtime_c/string_functions.h"  // control_type, drop_reason, dst_id, last_hop_id, msg_id, next_hop_id, payload, recent_hops, ref_msg_id, src_id

// forward declare type support functions
ROSIDL_TYPESUPPORT_FASTRTPS_C_IMPORT_uav_msgs
size_t get_serialized_size_builtin_interfaces__msg__Time(
  const void * untyped_ros_message,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_IMPORT_uav_msgs
size_t max_serialized_size_builtin_interfaces__msg__Time(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_IMPORT_uav_msgs
const rosidl_message_type_support_t *
  ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, builtin_interfaces, msg, Time)();


using _TrafficMessage__ros_msg_type = uav_msgs__msg__TrafficMessage;

static bool _TrafficMessage__cdr_serialize(
  const void * untyped_ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  const _TrafficMessage__ros_msg_type * ros_message = static_cast<const _TrafficMessage__ros_msg_type *>(untyped_ros_message);
  // Field name: msg_id
  {
    const rosidl_runtime_c__String * str = &ros_message->msg_id;
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

  // Field name: src_id
  {
    const rosidl_runtime_c__String * str = &ros_message->src_id;
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

  // Field name: dst_id
  {
    const rosidl_runtime_c__String * str = &ros_message->dst_id;
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

  // Field name: next_hop_id
  {
    const rosidl_runtime_c__String * str = &ros_message->next_hop_id;
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

  // Field name: last_hop_id
  {
    const rosidl_runtime_c__String * str = &ros_message->last_hop_id;
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

  // Field name: flow_type
  {
    cdr << ros_message->flow_type;
  }

  // Field name: control_type
  {
    const rosidl_runtime_c__String * str = &ros_message->control_type;
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

  // Field name: seq
  {
    cdr << ros_message->seq;
  }

  // Field name: hop_count
  {
    cdr << ros_message->hop_count;
  }

  // Field name: ttl
  {
    cdr << ros_message->ttl;
  }

  // Field name: requires_ack
  {
    cdr << (ros_message->requires_ack ? true : false);
  }

  // Field name: payload
  {
    const rosidl_runtime_c__String * str = &ros_message->payload;
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

  // Field name: creation_time
  {
    const message_type_support_callbacks_t * callbacks =
      static_cast<const message_type_support_callbacks_t *>(
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
        rosidl_typesupport_fastrtps_c, builtin_interfaces, msg, Time
      )()->data);
    if (!callbacks->cdr_serialize(
        &ros_message->creation_time, cdr))
    {
      return false;
    }
  }

  // Field name: ref_msg_id
  {
    const rosidl_runtime_c__String * str = &ros_message->ref_msg_id;
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

  // Field name: last_tx_time
  {
    const message_type_support_callbacks_t * callbacks =
      static_cast<const message_type_support_callbacks_t *>(
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
        rosidl_typesupport_fastrtps_c, builtin_interfaces, msg, Time
      )()->data);
    if (!callbacks->cdr_serialize(
        &ros_message->last_tx_time, cdr))
    {
      return false;
    }
  }

  // Field name: last_rx_time
  {
    const message_type_support_callbacks_t * callbacks =
      static_cast<const message_type_support_callbacks_t *>(
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
        rosidl_typesupport_fastrtps_c, builtin_interfaces, msg, Time
      )()->data);
    if (!callbacks->cdr_serialize(
        &ros_message->last_rx_time, cdr))
    {
      return false;
    }
  }

  // Field name: drop_reason
  {
    const rosidl_runtime_c__String * str = &ros_message->drop_reason;
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

  // Field name: recent_hops
  {
    size_t size = ros_message->recent_hops.size;
    auto array_ptr = ros_message->recent_hops.data;
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

static bool _TrafficMessage__cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  void * untyped_ros_message)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  _TrafficMessage__ros_msg_type * ros_message = static_cast<_TrafficMessage__ros_msg_type *>(untyped_ros_message);
  // Field name: msg_id
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->msg_id.data) {
      rosidl_runtime_c__String__init(&ros_message->msg_id);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->msg_id,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'msg_id'\n");
      return false;
    }
  }

  // Field name: src_id
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->src_id.data) {
      rosidl_runtime_c__String__init(&ros_message->src_id);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->src_id,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'src_id'\n");
      return false;
    }
  }

  // Field name: dst_id
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->dst_id.data) {
      rosidl_runtime_c__String__init(&ros_message->dst_id);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->dst_id,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'dst_id'\n");
      return false;
    }
  }

  // Field name: next_hop_id
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->next_hop_id.data) {
      rosidl_runtime_c__String__init(&ros_message->next_hop_id);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->next_hop_id,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'next_hop_id'\n");
      return false;
    }
  }

  // Field name: last_hop_id
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->last_hop_id.data) {
      rosidl_runtime_c__String__init(&ros_message->last_hop_id);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->last_hop_id,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'last_hop_id'\n");
      return false;
    }
  }

  // Field name: flow_type
  {
    cdr >> ros_message->flow_type;
  }

  // Field name: control_type
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->control_type.data) {
      rosidl_runtime_c__String__init(&ros_message->control_type);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->control_type,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'control_type'\n");
      return false;
    }
  }

  // Field name: seq
  {
    cdr >> ros_message->seq;
  }

  // Field name: hop_count
  {
    cdr >> ros_message->hop_count;
  }

  // Field name: ttl
  {
    cdr >> ros_message->ttl;
  }

  // Field name: requires_ack
  {
    uint8_t tmp;
    cdr >> tmp;
    ros_message->requires_ack = tmp ? true : false;
  }

  // Field name: payload
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->payload.data) {
      rosidl_runtime_c__String__init(&ros_message->payload);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->payload,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'payload'\n");
      return false;
    }
  }

  // Field name: creation_time
  {
    const message_type_support_callbacks_t * callbacks =
      static_cast<const message_type_support_callbacks_t *>(
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
        rosidl_typesupport_fastrtps_c, builtin_interfaces, msg, Time
      )()->data);
    if (!callbacks->cdr_deserialize(
        cdr, &ros_message->creation_time))
    {
      return false;
    }
  }

  // Field name: ref_msg_id
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->ref_msg_id.data) {
      rosidl_runtime_c__String__init(&ros_message->ref_msg_id);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->ref_msg_id,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'ref_msg_id'\n");
      return false;
    }
  }

  // Field name: last_tx_time
  {
    const message_type_support_callbacks_t * callbacks =
      static_cast<const message_type_support_callbacks_t *>(
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
        rosidl_typesupport_fastrtps_c, builtin_interfaces, msg, Time
      )()->data);
    if (!callbacks->cdr_deserialize(
        cdr, &ros_message->last_tx_time))
    {
      return false;
    }
  }

  // Field name: last_rx_time
  {
    const message_type_support_callbacks_t * callbacks =
      static_cast<const message_type_support_callbacks_t *>(
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
        rosidl_typesupport_fastrtps_c, builtin_interfaces, msg, Time
      )()->data);
    if (!callbacks->cdr_deserialize(
        cdr, &ros_message->last_rx_time))
    {
      return false;
    }
  }

  // Field name: drop_reason
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->drop_reason.data) {
      rosidl_runtime_c__String__init(&ros_message->drop_reason);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->drop_reason,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'drop_reason'\n");
      return false;
    }
  }

  // Field name: recent_hops
  {
    uint32_t cdrSize;
    cdr >> cdrSize;
    size_t size = static_cast<size_t>(cdrSize);
    if (ros_message->recent_hops.data) {
      rosidl_runtime_c__String__Sequence__fini(&ros_message->recent_hops);
    }
    if (!rosidl_runtime_c__String__Sequence__init(&ros_message->recent_hops, size)) {
      fprintf(stderr, "failed to create array for field 'recent_hops'");
      return false;
    }
    auto array_ptr = ros_message->recent_hops.data;
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
        fprintf(stderr, "failed to assign string into field 'recent_hops'\n");
        return false;
      }
    }
  }

  return true;
}  // NOLINT(readability/fn_size)

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_uav_msgs
size_t get_serialized_size_uav_msgs__msg__TrafficMessage(
  const void * untyped_ros_message,
  size_t current_alignment)
{
  const _TrafficMessage__ros_msg_type * ros_message = static_cast<const _TrafficMessage__ros_msg_type *>(untyped_ros_message);
  (void)ros_message;
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  (void)padding;
  (void)wchar_size;

  // field.name msg_id
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->msg_id.size + 1);
  // field.name src_id
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->src_id.size + 1);
  // field.name dst_id
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->dst_id.size + 1);
  // field.name next_hop_id
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->next_hop_id.size + 1);
  // field.name last_hop_id
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->last_hop_id.size + 1);
  // field.name flow_type
  {
    size_t item_size = sizeof(ros_message->flow_type);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name control_type
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->control_type.size + 1);
  // field.name seq
  {
    size_t item_size = sizeof(ros_message->seq);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name hop_count
  {
    size_t item_size = sizeof(ros_message->hop_count);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name ttl
  {
    size_t item_size = sizeof(ros_message->ttl);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name requires_ack
  {
    size_t item_size = sizeof(ros_message->requires_ack);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name payload
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->payload.size + 1);
  // field.name creation_time

  current_alignment += get_serialized_size_builtin_interfaces__msg__Time(
    &(ros_message->creation_time), current_alignment);
  // field.name ref_msg_id
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->ref_msg_id.size + 1);
  // field.name last_tx_time

  current_alignment += get_serialized_size_builtin_interfaces__msg__Time(
    &(ros_message->last_tx_time), current_alignment);
  // field.name last_rx_time

  current_alignment += get_serialized_size_builtin_interfaces__msg__Time(
    &(ros_message->last_rx_time), current_alignment);
  // field.name drop_reason
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->drop_reason.size + 1);
  // field.name recent_hops
  {
    size_t array_size = ros_message->recent_hops.size;
    auto array_ptr = ros_message->recent_hops.data;
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

static uint32_t _TrafficMessage__get_serialized_size(const void * untyped_ros_message)
{
  return static_cast<uint32_t>(
    get_serialized_size_uav_msgs__msg__TrafficMessage(
      untyped_ros_message, 0));
}

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_uav_msgs
size_t max_serialized_size_uav_msgs__msg__TrafficMessage(
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

  // member: msg_id
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
  // member: src_id
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
  // member: dst_id
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
  // member: next_hop_id
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
  // member: last_hop_id
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
  // member: flow_type
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }
  // member: control_type
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
  // member: seq
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint32_t);
    current_alignment += array_size * sizeof(uint32_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint32_t));
  }
  // member: hop_count
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint32_t);
    current_alignment += array_size * sizeof(uint32_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint32_t));
  }
  // member: ttl
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint32_t);
    current_alignment += array_size * sizeof(uint32_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint32_t));
  }
  // member: requires_ack
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }
  // member: payload
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
  // member: creation_time
  {
    size_t array_size = 1;


    last_member_size = 0;
    for (size_t index = 0; index < array_size; ++index) {
      bool inner_full_bounded;
      bool inner_is_plain;
      size_t inner_size;
      inner_size =
        max_serialized_size_builtin_interfaces__msg__Time(
        inner_full_bounded, inner_is_plain, current_alignment);
      last_member_size += inner_size;
      current_alignment += inner_size;
      full_bounded &= inner_full_bounded;
      is_plain &= inner_is_plain;
    }
  }
  // member: ref_msg_id
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
  // member: last_tx_time
  {
    size_t array_size = 1;


    last_member_size = 0;
    for (size_t index = 0; index < array_size; ++index) {
      bool inner_full_bounded;
      bool inner_is_plain;
      size_t inner_size;
      inner_size =
        max_serialized_size_builtin_interfaces__msg__Time(
        inner_full_bounded, inner_is_plain, current_alignment);
      last_member_size += inner_size;
      current_alignment += inner_size;
      full_bounded &= inner_full_bounded;
      is_plain &= inner_is_plain;
    }
  }
  // member: last_rx_time
  {
    size_t array_size = 1;


    last_member_size = 0;
    for (size_t index = 0; index < array_size; ++index) {
      bool inner_full_bounded;
      bool inner_is_plain;
      size_t inner_size;
      inner_size =
        max_serialized_size_builtin_interfaces__msg__Time(
        inner_full_bounded, inner_is_plain, current_alignment);
      last_member_size += inner_size;
      current_alignment += inner_size;
      full_bounded &= inner_full_bounded;
      is_plain &= inner_is_plain;
    }
  }
  // member: drop_reason
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
  // member: recent_hops
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
    using DataType = uav_msgs__msg__TrafficMessage;
    is_plain =
      (
      offsetof(DataType, recent_hops) +
      last_member_size
      ) == ret_val;
  }

  return ret_val;
}

static size_t _TrafficMessage__max_serialized_size(char & bounds_info)
{
  bool full_bounded;
  bool is_plain;
  size_t ret_val;

  ret_val = max_serialized_size_uav_msgs__msg__TrafficMessage(
    full_bounded, is_plain, 0);

  bounds_info =
    is_plain ? ROSIDL_TYPESUPPORT_FASTRTPS_PLAIN_TYPE :
    full_bounded ? ROSIDL_TYPESUPPORT_FASTRTPS_BOUNDED_TYPE : ROSIDL_TYPESUPPORT_FASTRTPS_UNBOUNDED_TYPE;
  return ret_val;
}


static message_type_support_callbacks_t __callbacks_TrafficMessage = {
  "uav_msgs::msg",
  "TrafficMessage",
  _TrafficMessage__cdr_serialize,
  _TrafficMessage__cdr_deserialize,
  _TrafficMessage__get_serialized_size,
  _TrafficMessage__max_serialized_size
};

static rosidl_message_type_support_t _TrafficMessage__type_support = {
  rosidl_typesupport_fastrtps_c__identifier,
  &__callbacks_TrafficMessage,
  get_message_typesupport_handle_function,
};

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, uav_msgs, msg, TrafficMessage)() {
  return &_TrafficMessage__type_support;
}

#if defined(__cplusplus)
}
#endif
