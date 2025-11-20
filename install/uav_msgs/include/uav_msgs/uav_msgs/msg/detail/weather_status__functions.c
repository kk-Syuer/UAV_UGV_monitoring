// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from uav_msgs:msg/WeatherStatus.idl
// generated code does not contain a copyright notice
#include "uav_msgs/msg/detail/weather_status__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


bool
uav_msgs__msg__WeatherStatus__init(uav_msgs__msg__WeatherStatus * msg)
{
  if (!msg) {
    return false;
  }
  // rain_intensity
  // wind_speed
  // wind_direction_deg
  // temperature_c
  return true;
}

void
uav_msgs__msg__WeatherStatus__fini(uav_msgs__msg__WeatherStatus * msg)
{
  if (!msg) {
    return;
  }
  // rain_intensity
  // wind_speed
  // wind_direction_deg
  // temperature_c
}

bool
uav_msgs__msg__WeatherStatus__are_equal(const uav_msgs__msg__WeatherStatus * lhs, const uav_msgs__msg__WeatherStatus * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // rain_intensity
  if (lhs->rain_intensity != rhs->rain_intensity) {
    return false;
  }
  // wind_speed
  if (lhs->wind_speed != rhs->wind_speed) {
    return false;
  }
  // wind_direction_deg
  if (lhs->wind_direction_deg != rhs->wind_direction_deg) {
    return false;
  }
  // temperature_c
  if (lhs->temperature_c != rhs->temperature_c) {
    return false;
  }
  return true;
}

bool
uav_msgs__msg__WeatherStatus__copy(
  const uav_msgs__msg__WeatherStatus * input,
  uav_msgs__msg__WeatherStatus * output)
{
  if (!input || !output) {
    return false;
  }
  // rain_intensity
  output->rain_intensity = input->rain_intensity;
  // wind_speed
  output->wind_speed = input->wind_speed;
  // wind_direction_deg
  output->wind_direction_deg = input->wind_direction_deg;
  // temperature_c
  output->temperature_c = input->temperature_c;
  return true;
}

uav_msgs__msg__WeatherStatus *
uav_msgs__msg__WeatherStatus__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__WeatherStatus * msg = (uav_msgs__msg__WeatherStatus *)allocator.allocate(sizeof(uav_msgs__msg__WeatherStatus), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(uav_msgs__msg__WeatherStatus));
  bool success = uav_msgs__msg__WeatherStatus__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
uav_msgs__msg__WeatherStatus__destroy(uav_msgs__msg__WeatherStatus * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    uav_msgs__msg__WeatherStatus__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
uav_msgs__msg__WeatherStatus__Sequence__init(uav_msgs__msg__WeatherStatus__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__WeatherStatus * data = NULL;

  if (size) {
    data = (uav_msgs__msg__WeatherStatus *)allocator.zero_allocate(size, sizeof(uav_msgs__msg__WeatherStatus), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = uav_msgs__msg__WeatherStatus__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        uav_msgs__msg__WeatherStatus__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
uav_msgs__msg__WeatherStatus__Sequence__fini(uav_msgs__msg__WeatherStatus__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      uav_msgs__msg__WeatherStatus__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

uav_msgs__msg__WeatherStatus__Sequence *
uav_msgs__msg__WeatherStatus__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__WeatherStatus__Sequence * array = (uav_msgs__msg__WeatherStatus__Sequence *)allocator.allocate(sizeof(uav_msgs__msg__WeatherStatus__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = uav_msgs__msg__WeatherStatus__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
uav_msgs__msg__WeatherStatus__Sequence__destroy(uav_msgs__msg__WeatherStatus__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    uav_msgs__msg__WeatherStatus__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
uav_msgs__msg__WeatherStatus__Sequence__are_equal(const uav_msgs__msg__WeatherStatus__Sequence * lhs, const uav_msgs__msg__WeatherStatus__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!uav_msgs__msg__WeatherStatus__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
uav_msgs__msg__WeatherStatus__Sequence__copy(
  const uav_msgs__msg__WeatherStatus__Sequence * input,
  uav_msgs__msg__WeatherStatus__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(uav_msgs__msg__WeatherStatus);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    uav_msgs__msg__WeatherStatus * data =
      (uav_msgs__msg__WeatherStatus *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!uav_msgs__msg__WeatherStatus__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          uav_msgs__msg__WeatherStatus__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!uav_msgs__msg__WeatherStatus__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
