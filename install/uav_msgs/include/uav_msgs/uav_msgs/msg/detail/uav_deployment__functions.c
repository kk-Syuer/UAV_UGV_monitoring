// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from uav_msgs:msg/UavDeployment.idl
// generated code does not contain a copyright notice
#include "uav_msgs/msg/detail/uav_deployment__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `uav_id`
// Member `cluster_id`
// Member `ch_id`
// Member `next_hop_to_sink`
#include "rosidl_runtime_c/string_functions.h"
// Member `target_pose`
#include "geometry_msgs/msg/detail/pose__functions.h"

bool
uav_msgs__msg__UavDeployment__init(uav_msgs__msg__UavDeployment * msg)
{
  if (!msg) {
    return false;
  }
  // uav_id
  if (!rosidl_runtime_c__String__init(&msg->uav_id)) {
    uav_msgs__msg__UavDeployment__fini(msg);
    return false;
  }
  // target_pose
  if (!geometry_msgs__msg__Pose__init(&msg->target_pose)) {
    uav_msgs__msg__UavDeployment__fini(msg);
    return false;
  }
  // role
  // cluster_id
  if (!rosidl_runtime_c__String__init(&msg->cluster_id)) {
    uav_msgs__msg__UavDeployment__fini(msg);
    return false;
  }
  // ch_id
  if (!rosidl_runtime_c__String__init(&msg->ch_id)) {
    uav_msgs__msg__UavDeployment__fini(msg);
    return false;
  }
  // next_hop_to_sink
  if (!rosidl_runtime_c__String__init(&msg->next_hop_to_sink)) {
    uav_msgs__msg__UavDeployment__fini(msg);
    return false;
  }
  return true;
}

void
uav_msgs__msg__UavDeployment__fini(uav_msgs__msg__UavDeployment * msg)
{
  if (!msg) {
    return;
  }
  // uav_id
  rosidl_runtime_c__String__fini(&msg->uav_id);
  // target_pose
  geometry_msgs__msg__Pose__fini(&msg->target_pose);
  // role
  // cluster_id
  rosidl_runtime_c__String__fini(&msg->cluster_id);
  // ch_id
  rosidl_runtime_c__String__fini(&msg->ch_id);
  // next_hop_to_sink
  rosidl_runtime_c__String__fini(&msg->next_hop_to_sink);
}

bool
uav_msgs__msg__UavDeployment__are_equal(const uav_msgs__msg__UavDeployment * lhs, const uav_msgs__msg__UavDeployment * rhs)
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
  // target_pose
  if (!geometry_msgs__msg__Pose__are_equal(
      &(lhs->target_pose), &(rhs->target_pose)))
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
  // ch_id
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->ch_id), &(rhs->ch_id)))
  {
    return false;
  }
  // next_hop_to_sink
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->next_hop_to_sink), &(rhs->next_hop_to_sink)))
  {
    return false;
  }
  return true;
}

bool
uav_msgs__msg__UavDeployment__copy(
  const uav_msgs__msg__UavDeployment * input,
  uav_msgs__msg__UavDeployment * output)
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
  // target_pose
  if (!geometry_msgs__msg__Pose__copy(
      &(input->target_pose), &(output->target_pose)))
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
  // ch_id
  if (!rosidl_runtime_c__String__copy(
      &(input->ch_id), &(output->ch_id)))
  {
    return false;
  }
  // next_hop_to_sink
  if (!rosidl_runtime_c__String__copy(
      &(input->next_hop_to_sink), &(output->next_hop_to_sink)))
  {
    return false;
  }
  return true;
}

uav_msgs__msg__UavDeployment *
uav_msgs__msg__UavDeployment__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__UavDeployment * msg = (uav_msgs__msg__UavDeployment *)allocator.allocate(sizeof(uav_msgs__msg__UavDeployment), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(uav_msgs__msg__UavDeployment));
  bool success = uav_msgs__msg__UavDeployment__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
uav_msgs__msg__UavDeployment__destroy(uav_msgs__msg__UavDeployment * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    uav_msgs__msg__UavDeployment__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
uav_msgs__msg__UavDeployment__Sequence__init(uav_msgs__msg__UavDeployment__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__UavDeployment * data = NULL;

  if (size) {
    data = (uav_msgs__msg__UavDeployment *)allocator.zero_allocate(size, sizeof(uav_msgs__msg__UavDeployment), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = uav_msgs__msg__UavDeployment__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        uav_msgs__msg__UavDeployment__fini(&data[i - 1]);
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
uav_msgs__msg__UavDeployment__Sequence__fini(uav_msgs__msg__UavDeployment__Sequence * array)
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
      uav_msgs__msg__UavDeployment__fini(&array->data[i]);
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

uav_msgs__msg__UavDeployment__Sequence *
uav_msgs__msg__UavDeployment__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__UavDeployment__Sequence * array = (uav_msgs__msg__UavDeployment__Sequence *)allocator.allocate(sizeof(uav_msgs__msg__UavDeployment__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = uav_msgs__msg__UavDeployment__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
uav_msgs__msg__UavDeployment__Sequence__destroy(uav_msgs__msg__UavDeployment__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    uav_msgs__msg__UavDeployment__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
uav_msgs__msg__UavDeployment__Sequence__are_equal(const uav_msgs__msg__UavDeployment__Sequence * lhs, const uav_msgs__msg__UavDeployment__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!uav_msgs__msg__UavDeployment__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
uav_msgs__msg__UavDeployment__Sequence__copy(
  const uav_msgs__msg__UavDeployment__Sequence * input,
  uav_msgs__msg__UavDeployment__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(uav_msgs__msg__UavDeployment);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    uav_msgs__msg__UavDeployment * data =
      (uav_msgs__msg__UavDeployment *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!uav_msgs__msg__UavDeployment__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          uav_msgs__msg__UavDeployment__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!uav_msgs__msg__UavDeployment__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
