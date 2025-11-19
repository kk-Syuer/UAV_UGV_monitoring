// generated from rosidl_typesupport_fastrtps_c/resource/idl__type_support_c.cpp.em
// with input from uav_msgs:srv/RequestCharge.idl
// generated code does not contain a copyright notice
#include "uav_msgs/srv/detail/request_charge__rosidl_typesupport_fastrtps_c.h"


#include <cassert>
#include <limits>
#include <string>
#include "rosidl_typesupport_fastrtps_c/identifier.h"
#include "rosidl_typesupport_fastrtps_c/wstring_conversion.hpp"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "uav_msgs/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "uav_msgs/srv/detail/request_charge__struct.h"
#include "uav_msgs/srv/detail/request_charge__functions.h"
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

#include "rosidl_runtime_c/string.h"  // uav_id
#include "rosidl_runtime_c/string_functions.h"  // uav_id

// forward declare type support functions


using _RequestCharge_Request__ros_msg_type = uav_msgs__srv__RequestCharge_Request;

static bool _RequestCharge_Request__cdr_serialize(
  const void * untyped_ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  const _RequestCharge_Request__ros_msg_type * ros_message = static_cast<const _RequestCharge_Request__ros_msg_type *>(untyped_ros_message);
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

  // Field name: battery_level
  {
    cdr << ros_message->battery_level;
  }

  // Field name: role
  {
    cdr << ros_message->role;
  }

  return true;
}

static bool _RequestCharge_Request__cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  void * untyped_ros_message)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  _RequestCharge_Request__ros_msg_type * ros_message = static_cast<_RequestCharge_Request__ros_msg_type *>(untyped_ros_message);
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

  // Field name: battery_level
  {
    cdr >> ros_message->battery_level;
  }

  // Field name: role
  {
    cdr >> ros_message->role;
  }

  return true;
}  // NOLINT(readability/fn_size)

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_uav_msgs
size_t get_serialized_size_uav_msgs__srv__RequestCharge_Request(
  const void * untyped_ros_message,
  size_t current_alignment)
{
  const _RequestCharge_Request__ros_msg_type * ros_message = static_cast<const _RequestCharge_Request__ros_msg_type *>(untyped_ros_message);
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
  // field.name battery_level
  {
    size_t item_size = sizeof(ros_message->battery_level);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name role
  {
    size_t item_size = sizeof(ros_message->role);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  return current_alignment - initial_alignment;
}

static uint32_t _RequestCharge_Request__get_serialized_size(const void * untyped_ros_message)
{
  return static_cast<uint32_t>(
    get_serialized_size_uav_msgs__srv__RequestCharge_Request(
      untyped_ros_message, 0));
}

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_uav_msgs
size_t max_serialized_size_uav_msgs__srv__RequestCharge_Request(
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
  // member: battery_level
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint32_t);
    current_alignment += array_size * sizeof(uint32_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint32_t));
  }
  // member: role
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }

  size_t ret_val = current_alignment - initial_alignment;
  if (is_plain) {
    // All members are plain, and type is not empty.
    // We still need to check that the in-memory alignment
    // is the same as the CDR mandated alignment.
    using DataType = uav_msgs__srv__RequestCharge_Request;
    is_plain =
      (
      offsetof(DataType, role) +
      last_member_size
      ) == ret_val;
  }

  return ret_val;
}

static size_t _RequestCharge_Request__max_serialized_size(char & bounds_info)
{
  bool full_bounded;
  bool is_plain;
  size_t ret_val;

  ret_val = max_serialized_size_uav_msgs__srv__RequestCharge_Request(
    full_bounded, is_plain, 0);

  bounds_info =
    is_plain ? ROSIDL_TYPESUPPORT_FASTRTPS_PLAIN_TYPE :
    full_bounded ? ROSIDL_TYPESUPPORT_FASTRTPS_BOUNDED_TYPE : ROSIDL_TYPESUPPORT_FASTRTPS_UNBOUNDED_TYPE;
  return ret_val;
}


static message_type_support_callbacks_t __callbacks_RequestCharge_Request = {
  "uav_msgs::srv",
  "RequestCharge_Request",
  _RequestCharge_Request__cdr_serialize,
  _RequestCharge_Request__cdr_deserialize,
  _RequestCharge_Request__get_serialized_size,
  _RequestCharge_Request__max_serialized_size
};

static rosidl_message_type_support_t _RequestCharge_Request__type_support = {
  rosidl_typesupport_fastrtps_c__identifier,
  &__callbacks_RequestCharge_Request,
  get_message_typesupport_handle_function,
};

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, uav_msgs, srv, RequestCharge_Request)() {
  return &_RequestCharge_Request__type_support;
}

#if defined(__cplusplus)
}
#endif

// already included above
// #include <cassert>
// already included above
// #include <limits>
// already included above
// #include <string>
// already included above
// #include "rosidl_typesupport_fastrtps_c/identifier.h"
// already included above
// #include "rosidl_typesupport_fastrtps_c/wstring_conversion.hpp"
// already included above
// #include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
// already included above
// #include "uav_msgs/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
// already included above
// #include "uav_msgs/srv/detail/request_charge__struct.h"
// already included above
// #include "uav_msgs/srv/detail/request_charge__functions.h"
// already included above
// #include "fastcdr/Cdr.h"

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

#include "builtin_interfaces/msg/detail/time__functions.h"  // eta

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


using _RequestCharge_Response__ros_msg_type = uav_msgs__srv__RequestCharge_Response;

static bool _RequestCharge_Response__cdr_serialize(
  const void * untyped_ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  const _RequestCharge_Response__ros_msg_type * ros_message = static_cast<const _RequestCharge_Response__ros_msg_type *>(untyped_ros_message);
  // Field name: accepted
  {
    cdr << (ros_message->accepted ? true : false);
  }

  // Field name: eta
  {
    const message_type_support_callbacks_t * callbacks =
      static_cast<const message_type_support_callbacks_t *>(
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
        rosidl_typesupport_fastrtps_c, builtin_interfaces, msg, Time
      )()->data);
    if (!callbacks->cdr_serialize(
        &ros_message->eta, cdr))
    {
      return false;
    }
  }

  // Field name: assigned_priority
  {
    cdr << ros_message->assigned_priority;
  }

  return true;
}

static bool _RequestCharge_Response__cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  void * untyped_ros_message)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  _RequestCharge_Response__ros_msg_type * ros_message = static_cast<_RequestCharge_Response__ros_msg_type *>(untyped_ros_message);
  // Field name: accepted
  {
    uint8_t tmp;
    cdr >> tmp;
    ros_message->accepted = tmp ? true : false;
  }

  // Field name: eta
  {
    const message_type_support_callbacks_t * callbacks =
      static_cast<const message_type_support_callbacks_t *>(
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
        rosidl_typesupport_fastrtps_c, builtin_interfaces, msg, Time
      )()->data);
    if (!callbacks->cdr_deserialize(
        cdr, &ros_message->eta))
    {
      return false;
    }
  }

  // Field name: assigned_priority
  {
    cdr >> ros_message->assigned_priority;
  }

  return true;
}  // NOLINT(readability/fn_size)

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_uav_msgs
size_t get_serialized_size_uav_msgs__srv__RequestCharge_Response(
  const void * untyped_ros_message,
  size_t current_alignment)
{
  const _RequestCharge_Response__ros_msg_type * ros_message = static_cast<const _RequestCharge_Response__ros_msg_type *>(untyped_ros_message);
  (void)ros_message;
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  (void)padding;
  (void)wchar_size;

  // field.name accepted
  {
    size_t item_size = sizeof(ros_message->accepted);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name eta

  current_alignment += get_serialized_size_builtin_interfaces__msg__Time(
    &(ros_message->eta), current_alignment);
  // field.name assigned_priority
  {
    size_t item_size = sizeof(ros_message->assigned_priority);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  return current_alignment - initial_alignment;
}

static uint32_t _RequestCharge_Response__get_serialized_size(const void * untyped_ros_message)
{
  return static_cast<uint32_t>(
    get_serialized_size_uav_msgs__srv__RequestCharge_Response(
      untyped_ros_message, 0));
}

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_uav_msgs
size_t max_serialized_size_uav_msgs__srv__RequestCharge_Response(
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

  // member: accepted
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }
  // member: eta
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
  // member: assigned_priority
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }

  size_t ret_val = current_alignment - initial_alignment;
  if (is_plain) {
    // All members are plain, and type is not empty.
    // We still need to check that the in-memory alignment
    // is the same as the CDR mandated alignment.
    using DataType = uav_msgs__srv__RequestCharge_Response;
    is_plain =
      (
      offsetof(DataType, assigned_priority) +
      last_member_size
      ) == ret_val;
  }

  return ret_val;
}

static size_t _RequestCharge_Response__max_serialized_size(char & bounds_info)
{
  bool full_bounded;
  bool is_plain;
  size_t ret_val;

  ret_val = max_serialized_size_uav_msgs__srv__RequestCharge_Response(
    full_bounded, is_plain, 0);

  bounds_info =
    is_plain ? ROSIDL_TYPESUPPORT_FASTRTPS_PLAIN_TYPE :
    full_bounded ? ROSIDL_TYPESUPPORT_FASTRTPS_BOUNDED_TYPE : ROSIDL_TYPESUPPORT_FASTRTPS_UNBOUNDED_TYPE;
  return ret_val;
}


static message_type_support_callbacks_t __callbacks_RequestCharge_Response = {
  "uav_msgs::srv",
  "RequestCharge_Response",
  _RequestCharge_Response__cdr_serialize,
  _RequestCharge_Response__cdr_deserialize,
  _RequestCharge_Response__get_serialized_size,
  _RequestCharge_Response__max_serialized_size
};

static rosidl_message_type_support_t _RequestCharge_Response__type_support = {
  rosidl_typesupport_fastrtps_c__identifier,
  &__callbacks_RequestCharge_Response,
  get_message_typesupport_handle_function,
};

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, uav_msgs, srv, RequestCharge_Response)() {
  return &_RequestCharge_Response__type_support;
}

#if defined(__cplusplus)
}
#endif

#include "rosidl_typesupport_fastrtps_cpp/service_type_support.h"
#include "rosidl_typesupport_cpp/service_type_support.hpp"
// already included above
// #include "rosidl_typesupport_fastrtps_c/identifier.h"
// already included above
// #include "uav_msgs/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "uav_msgs/srv/request_charge.h"

#if defined(__cplusplus)
extern "C"
{
#endif

static service_type_support_callbacks_t RequestCharge__callbacks = {
  "uav_msgs::srv",
  "RequestCharge",
  ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, uav_msgs, srv, RequestCharge_Request)(),
  ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, uav_msgs, srv, RequestCharge_Response)(),
};

static rosidl_service_type_support_t RequestCharge__handle = {
  rosidl_typesupport_fastrtps_c__identifier,
  &RequestCharge__callbacks,
  get_service_typesupport_handle_function,
};

const rosidl_service_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, uav_msgs, srv, RequestCharge)() {
  return &RequestCharge__handle;
}

#if defined(__cplusplus)
}
#endif
