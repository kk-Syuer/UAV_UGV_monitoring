// generated from rosidl_typesupport_fastrtps_cpp/resource/idl__type_support.cpp.em
// with input from uav_msgs:msg/WeatherStatus.idl
// generated code does not contain a copyright notice
#include "uav_msgs/msg/detail/weather_status__rosidl_typesupport_fastrtps_cpp.hpp"
#include "uav_msgs/msg/detail/weather_status__struct.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_fastrtps_cpp/identifier.hpp"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support_decl.hpp"
#include "rosidl_typesupport_fastrtps_cpp/wstring_conversion.hpp"
#include "fastcdr/Cdr.h"


// forward declaration of message dependencies and their conversion functions

namespace uav_msgs
{

namespace msg
{

namespace typesupport_fastrtps_cpp
{

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_uav_msgs
cdr_serialize(
  const uav_msgs::msg::WeatherStatus & ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  // Member: rain_intensity
  cdr << ros_message.rain_intensity;
  // Member: wind_speed
  cdr << ros_message.wind_speed;
  // Member: wind_direction_deg
  cdr << ros_message.wind_direction_deg;
  // Member: temperature_c
  cdr << ros_message.temperature_c;
  return true;
}

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_uav_msgs
cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  uav_msgs::msg::WeatherStatus & ros_message)
{
  // Member: rain_intensity
  cdr >> ros_message.rain_intensity;

  // Member: wind_speed
  cdr >> ros_message.wind_speed;

  // Member: wind_direction_deg
  cdr >> ros_message.wind_direction_deg;

  // Member: temperature_c
  cdr >> ros_message.temperature_c;

  return true;
}

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_uav_msgs
get_serialized_size(
  const uav_msgs::msg::WeatherStatus & ros_message,
  size_t current_alignment)
{
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  (void)padding;
  (void)wchar_size;

  // Member: rain_intensity
  {
    size_t item_size = sizeof(ros_message.rain_intensity);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // Member: wind_speed
  {
    size_t item_size = sizeof(ros_message.wind_speed);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // Member: wind_direction_deg
  {
    size_t item_size = sizeof(ros_message.wind_direction_deg);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // Member: temperature_c
  {
    size_t item_size = sizeof(ros_message.temperature_c);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  return current_alignment - initial_alignment;
}

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_uav_msgs
max_serialized_size_WeatherStatus(
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


  // Member: rain_intensity
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint32_t);
    current_alignment += array_size * sizeof(uint32_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint32_t));
  }

  // Member: wind_speed
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint32_t);
    current_alignment += array_size * sizeof(uint32_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint32_t));
  }

  // Member: wind_direction_deg
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint32_t);
    current_alignment += array_size * sizeof(uint32_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint32_t));
  }

  // Member: temperature_c
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint32_t);
    current_alignment += array_size * sizeof(uint32_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint32_t));
  }

  size_t ret_val = current_alignment - initial_alignment;
  if (is_plain) {
    // All members are plain, and type is not empty.
    // We still need to check that the in-memory alignment
    // is the same as the CDR mandated alignment.
    using DataType = uav_msgs::msg::WeatherStatus;
    is_plain =
      (
      offsetof(DataType, temperature_c) +
      last_member_size
      ) == ret_val;
  }

  return ret_val;
}

static bool _WeatherStatus__cdr_serialize(
  const void * untyped_ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  auto typed_message =
    static_cast<const uav_msgs::msg::WeatherStatus *>(
    untyped_ros_message);
  return cdr_serialize(*typed_message, cdr);
}

static bool _WeatherStatus__cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  void * untyped_ros_message)
{
  auto typed_message =
    static_cast<uav_msgs::msg::WeatherStatus *>(
    untyped_ros_message);
  return cdr_deserialize(cdr, *typed_message);
}

static uint32_t _WeatherStatus__get_serialized_size(
  const void * untyped_ros_message)
{
  auto typed_message =
    static_cast<const uav_msgs::msg::WeatherStatus *>(
    untyped_ros_message);
  return static_cast<uint32_t>(get_serialized_size(*typed_message, 0));
}

static size_t _WeatherStatus__max_serialized_size(char & bounds_info)
{
  bool full_bounded;
  bool is_plain;
  size_t ret_val;

  ret_val = max_serialized_size_WeatherStatus(full_bounded, is_plain, 0);

  bounds_info =
    is_plain ? ROSIDL_TYPESUPPORT_FASTRTPS_PLAIN_TYPE :
    full_bounded ? ROSIDL_TYPESUPPORT_FASTRTPS_BOUNDED_TYPE : ROSIDL_TYPESUPPORT_FASTRTPS_UNBOUNDED_TYPE;
  return ret_val;
}

static message_type_support_callbacks_t _WeatherStatus__callbacks = {
  "uav_msgs::msg",
  "WeatherStatus",
  _WeatherStatus__cdr_serialize,
  _WeatherStatus__cdr_deserialize,
  _WeatherStatus__get_serialized_size,
  _WeatherStatus__max_serialized_size
};

static rosidl_message_type_support_t _WeatherStatus__handle = {
  rosidl_typesupport_fastrtps_cpp::typesupport_identifier,
  &_WeatherStatus__callbacks,
  get_message_typesupport_handle_function,
};

}  // namespace typesupport_fastrtps_cpp

}  // namespace msg

}  // namespace uav_msgs

namespace rosidl_typesupport_fastrtps_cpp
{

template<>
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_EXPORT_uav_msgs
const rosidl_message_type_support_t *
get_message_type_support_handle<uav_msgs::msg::WeatherStatus>()
{
  return &uav_msgs::msg::typesupport_fastrtps_cpp::_WeatherStatus__handle;
}

}  // namespace rosidl_typesupport_fastrtps_cpp

#ifdef __cplusplus
extern "C"
{
#endif

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_cpp, uav_msgs, msg, WeatherStatus)() {
  return &uav_msgs::msg::typesupport_fastrtps_cpp::_WeatherStatus__handle;
}

#ifdef __cplusplus
}
#endif
