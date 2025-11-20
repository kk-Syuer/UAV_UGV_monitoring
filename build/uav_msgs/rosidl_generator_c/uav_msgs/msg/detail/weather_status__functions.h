// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from uav_msgs:msg/WeatherStatus.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__FUNCTIONS_H_
#define UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "uav_msgs/msg/rosidl_generator_c__visibility_control.h"

#include "uav_msgs/msg/detail/weather_status__struct.h"

/// Initialize msg/WeatherStatus message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * uav_msgs__msg__WeatherStatus
 * )) before or use
 * uav_msgs__msg__WeatherStatus__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_uav_msgs
bool
uav_msgs__msg__WeatherStatus__init(uav_msgs__msg__WeatherStatus * msg);

/// Finalize msg/WeatherStatus message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_uav_msgs
void
uav_msgs__msg__WeatherStatus__fini(uav_msgs__msg__WeatherStatus * msg);

/// Create msg/WeatherStatus message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * uav_msgs__msg__WeatherStatus__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_uav_msgs
uav_msgs__msg__WeatherStatus *
uav_msgs__msg__WeatherStatus__create();

/// Destroy msg/WeatherStatus message.
/**
 * It calls
 * uav_msgs__msg__WeatherStatus__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_uav_msgs
void
uav_msgs__msg__WeatherStatus__destroy(uav_msgs__msg__WeatherStatus * msg);

/// Check for msg/WeatherStatus message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_uav_msgs
bool
uav_msgs__msg__WeatherStatus__are_equal(const uav_msgs__msg__WeatherStatus * lhs, const uav_msgs__msg__WeatherStatus * rhs);

/// Copy a msg/WeatherStatus message.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source message pointer.
 * \param[out] output The target message pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer is null
 *   or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_uav_msgs
bool
uav_msgs__msg__WeatherStatus__copy(
  const uav_msgs__msg__WeatherStatus * input,
  uav_msgs__msg__WeatherStatus * output);

/// Initialize array of msg/WeatherStatus messages.
/**
 * It allocates the memory for the number of elements and calls
 * uav_msgs__msg__WeatherStatus__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_uav_msgs
bool
uav_msgs__msg__WeatherStatus__Sequence__init(uav_msgs__msg__WeatherStatus__Sequence * array, size_t size);

/// Finalize array of msg/WeatherStatus messages.
/**
 * It calls
 * uav_msgs__msg__WeatherStatus__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_uav_msgs
void
uav_msgs__msg__WeatherStatus__Sequence__fini(uav_msgs__msg__WeatherStatus__Sequence * array);

/// Create array of msg/WeatherStatus messages.
/**
 * It allocates the memory for the array and calls
 * uav_msgs__msg__WeatherStatus__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_uav_msgs
uav_msgs__msg__WeatherStatus__Sequence *
uav_msgs__msg__WeatherStatus__Sequence__create(size_t size);

/// Destroy array of msg/WeatherStatus messages.
/**
 * It calls
 * uav_msgs__msg__WeatherStatus__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_uav_msgs
void
uav_msgs__msg__WeatherStatus__Sequence__destroy(uav_msgs__msg__WeatherStatus__Sequence * array);

/// Check for msg/WeatherStatus message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_uav_msgs
bool
uav_msgs__msg__WeatherStatus__Sequence__are_equal(const uav_msgs__msg__WeatherStatus__Sequence * lhs, const uav_msgs__msg__WeatherStatus__Sequence * rhs);

/// Copy an array of msg/WeatherStatus messages.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source array pointer.
 * \param[out] output The target array pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer
 *   is null or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_uav_msgs
bool
uav_msgs__msg__WeatherStatus__Sequence__copy(
  const uav_msgs__msg__WeatherStatus__Sequence * input,
  uav_msgs__msg__WeatherStatus__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__FUNCTIONS_H_
