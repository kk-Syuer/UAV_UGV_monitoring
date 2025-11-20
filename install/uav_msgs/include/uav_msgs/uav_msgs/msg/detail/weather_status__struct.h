// NOLINT: This file starts with a BOM since it contain non-ASCII characters
// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from uav_msgs:msg/WeatherStatus.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__STRUCT_H_
#define UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/WeatherStatus in the package uav_msgs.
/**
  * Simple weather model for the simulator
 */
typedef struct uav_msgs__msg__WeatherStatus
{
  /// mm/h (0 = no rain)
  float rain_intensity;
  /// m/s
  float wind_speed;
  /// 0–360, 0 = north
  float wind_direction_deg;
  /// ambient temperature in Celsius
  float temperature_c;
} uav_msgs__msg__WeatherStatus;

// Struct for a sequence of uav_msgs__msg__WeatherStatus.
typedef struct uav_msgs__msg__WeatherStatus__Sequence
{
  uav_msgs__msg__WeatherStatus * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__msg__WeatherStatus__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__STRUCT_H_
