// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from uav_msgs:msg/WeatherStatus.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "uav_msgs/msg/detail/weather_status__rosidl_typesupport_introspection_c.h"
#include "uav_msgs/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "uav_msgs/msg/detail/weather_status__functions.h"
#include "uav_msgs/msg/detail/weather_status__struct.h"


#ifdef __cplusplus
extern "C"
{
#endif

void uav_msgs__msg__WeatherStatus__rosidl_typesupport_introspection_c__WeatherStatus_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  uav_msgs__msg__WeatherStatus__init(message_memory);
}

void uav_msgs__msg__WeatherStatus__rosidl_typesupport_introspection_c__WeatherStatus_fini_function(void * message_memory)
{
  uav_msgs__msg__WeatherStatus__fini(message_memory);
}

static rosidl_typesupport_introspection_c__MessageMember uav_msgs__msg__WeatherStatus__rosidl_typesupport_introspection_c__WeatherStatus_message_member_array[4] = {
  {
    "rain_intensity",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__WeatherStatus, rain_intensity),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "wind_speed",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__WeatherStatus, wind_speed),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "wind_direction_deg",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__WeatherStatus, wind_direction_deg),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "temperature_c",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__msg__WeatherStatus, temperature_c),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers uav_msgs__msg__WeatherStatus__rosidl_typesupport_introspection_c__WeatherStatus_message_members = {
  "uav_msgs__msg",  // message namespace
  "WeatherStatus",  // message name
  4,  // number of fields
  sizeof(uav_msgs__msg__WeatherStatus),
  uav_msgs__msg__WeatherStatus__rosidl_typesupport_introspection_c__WeatherStatus_message_member_array,  // message members
  uav_msgs__msg__WeatherStatus__rosidl_typesupport_introspection_c__WeatherStatus_init_function,  // function to initialize message memory (memory has to be allocated)
  uav_msgs__msg__WeatherStatus__rosidl_typesupport_introspection_c__WeatherStatus_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t uav_msgs__msg__WeatherStatus__rosidl_typesupport_introspection_c__WeatherStatus_message_type_support_handle = {
  0,
  &uav_msgs__msg__WeatherStatus__rosidl_typesupport_introspection_c__WeatherStatus_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_uav_msgs
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, uav_msgs, msg, WeatherStatus)() {
  if (!uav_msgs__msg__WeatherStatus__rosidl_typesupport_introspection_c__WeatherStatus_message_type_support_handle.typesupport_identifier) {
    uav_msgs__msg__WeatherStatus__rosidl_typesupport_introspection_c__WeatherStatus_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &uav_msgs__msg__WeatherStatus__rosidl_typesupport_introspection_c__WeatherStatus_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
