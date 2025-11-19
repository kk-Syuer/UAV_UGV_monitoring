// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from uav_msgs:msg/UavStatus.idl
// generated code does not contain a copyright notice
#include "uav_msgs/msg/detail/uav_status__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `uav_id`
// Member `cluster_id`
#include "rosidl_runtime_c/string_functions.h"
// Member `pose`
#include "geometry_msgs/msg/detail/pose__functions.h"
// Member `stamp`
#include "builtin_interfaces/msg/detail/time__functions.h"

bool
uav_msgs__msg__UavStatus__init(uav_msgs__msg__UavStatus * msg)
{
  if (!msg) {
    return false;
  }
  // uav_id
  if (!rosidl_runtime_c__String__init(&msg->uav_id)) {
    uav_msgs__msg__UavStatus__fini(msg);
    return false;
  }
  // role
  // cluster_id
  if (!rosidl_runtime_c__String__init(&msg->cluster_id)) {
    uav_msgs__msg__UavStatus__fini(msg);
    return false;
  }
  // battery_level
  // battery_capacity
  // pose
  if (!geometry_msgs__msg__Pose__init(&msg->pose)) {
    uav_msgs__msg__UavStatus__fini(msg);
    return false;
  }
  // service_radius
  // connected_users
  // traffic_load
  // packet_loss_estimate
  // energy_consumption_rate
  // stamp
  if (!builtin_interfaces__msg__Time__init(&msg->stamp)) {
    uav_msgs__msg__UavStatus__fini(msg);
    return false;
  }
  return true;
}

void
uav_msgs__msg__UavStatus__fini(uav_msgs__msg__UavStatus * msg)
{
  if (!msg) {
    return;
  }
  // uav_id
  rosidl_runtime_c__String__fini(&msg->uav_id);
  // role
  // cluster_id
  rosidl_runtime_c__String__fini(&msg->cluster_id);
  // battery_level
  // battery_capacity
  // pose
  geometry_msgs__msg__Pose__fini(&msg->pose);
  // service_radius
  // connected_users
  // traffic_load
  // packet_loss_estimate
  // energy_consumption_rate
  // stamp
  builtin_interfaces__msg__Time__fini(&msg->stamp);
}

bool
uav_msgs__msg__UavStatus__are_equal(const uav_msgs__msg__UavStatus * lhs, const uav_msgs__msg__UavStatus * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // uav_id
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->uav_id), &(rhs->uav_id)))
  {
    return false;
  }
  // role
  if (lhs->role != rhs->role) {
    return false;
  }
  // cluster_id
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->cluster_id), &(rhs->cluster_id)))
  {
    return false;
  }
  // battery_level
  if (lhs->battery_level != rhs->battery_level) {
    return false;
  }
  // battery_capacity
  if (lhs->battery_capacity != rhs->battery_capacity) {
    return false;
  }
  // pose
  if (!geometry_msgs__msg__Pose__are_equal(
      &(lhs->pose), &(rhs->pose)))
  {
    return false;
  }
  // service_radius
  if (lhs->service_radius != rhs->service_radius) {
    return false;
  }
  // connected_users
  if (lhs->connected_users != rhs->connected_users) {
    return false;
  }
  // traffic_load
  if (lhs->traffic_load != rhs->traffic_load) {
    return false;
  }
  // packet_loss_estimate
  if (lhs->packet_loss_estimate != rhs->packet_loss_estimate) {
    return false;
  }
  // energy_consumption_rate
  if (lhs->energy_consumption_rate != rhs->energy_consumption_rate) {
    return false;
  }
  // stamp
  if (!builtin_interfaces__msg__Time__are_equal(
      &(lhs->stamp), &(rhs->stamp)))
  {
    return false;
  }
  return true;
}

bool
uav_msgs__msg__UavStatus__copy(
  const uav_msgs__msg__UavStatus * input,
  uav_msgs__msg__UavStatus * output)
{
  if (!input || !output) {
    return false;
  }
  // uav_id
  if (!rosidl_runtime_c__String__copy(
      &(input->uav_id), &(output->uav_id)))
  {
    return false;
  }
  // role
  output->role = input->role;
  // cluster_id
  if (!rosidl_runtime_c__String__copy(
      &(input->cluster_id), &(output->cluster_id)))
  {
    return false;
  }
  // battery_level
  output->battery_level = input->battery_level;
  // battery_capacity
  output->battery_capacity = input->battery_capacity;
  // pose
  if (!geometry_msgs__msg__Pose__copy(
      &(input->pose), &(output->pose)))
  {
    return false;
  }
  // service_radius
  output->service_radius = input->service_radius;
  // connected_users
  output->connected_users = input->connected_users;
  // traffic_load
  output->traffic_load = input->traffic_load;
  // packet_loss_estimate
  output->packet_loss_estimate = input->packet_loss_estimate;
  // energy_consumption_rate
  output->energy_consumption_rate = input->energy_consumption_rate;
  // stamp
  if (!builtin_interfaces__msg__Time__copy(
      &(input->stamp), &(output->stamp)))
  {
    return false;
  }
  return true;
}

uav_msgs__msg__UavStatus *
uav_msgs__msg__UavStatus__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__UavStatus * msg = (uav_msgs__msg__UavStatus *)allocator.allocate(sizeof(uav_msgs__msg__UavStatus), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(uav_msgs__msg__UavStatus));
  bool success = uav_msgs__msg__UavStatus__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
uav_msgs__msg__UavStatus__destroy(uav_msgs__msg__UavStatus * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    uav_msgs__msg__UavStatus__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
uav_msgs__msg__UavStatus__Sequence__init(uav_msgs__msg__UavStatus__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__UavStatus * data = NULL;

  if (size) {
    data = (uav_msgs__msg__UavStatus *)allocator.zero_allocate(size, sizeof(uav_msgs__msg__UavStatus), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = uav_msgs__msg__UavStatus__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        uav_msgs__msg__UavStatus__fini(&data[i - 1]);
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
uav_msgs__msg__UavStatus__Sequence__fini(uav_msgs__msg__UavStatus__Sequence * array)
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
      uav_msgs__msg__UavStatus__fini(&array->data[i]);
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

uav_msgs__msg__UavStatus__Sequence *
uav_msgs__msg__UavStatus__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__UavStatus__Sequence * array = (uav_msgs__msg__UavStatus__Sequence *)allocator.allocate(sizeof(uav_msgs__msg__UavStatus__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = uav_msgs__msg__UavStatus__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
uav_msgs__msg__UavStatus__Sequence__destroy(uav_msgs__msg__UavStatus__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    uav_msgs__msg__UavStatus__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
uav_msgs__msg__UavStatus__Sequence__are_equal(const uav_msgs__msg__UavStatus__Sequence * lhs, const uav_msgs__msg__UavStatus__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!uav_msgs__msg__UavStatus__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
uav_msgs__msg__UavStatus__Sequence__copy(
  const uav_msgs__msg__UavStatus__Sequence * input,
  uav_msgs__msg__UavStatus__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(uav_msgs__msg__UavStatus);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    uav_msgs__msg__UavStatus * data =
      (uav_msgs__msg__UavStatus *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!uav_msgs__msg__UavStatus__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          uav_msgs__msg__UavStatus__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!uav_msgs__msg__UavStatus__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
