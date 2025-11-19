// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from uav_msgs:msg/ClusterInfo.idl
// generated code does not contain a copyright notice
#include "uav_msgs/msg/detail/cluster_info__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `cluster_id`
// Member `ch_id`
// Member `member_ids`
#include "rosidl_runtime_c/string_functions.h"

bool
uav_msgs__msg__ClusterInfo__init(uav_msgs__msg__ClusterInfo * msg)
{
  if (!msg) {
    return false;
  }
  // cluster_id
  if (!rosidl_runtime_c__String__init(&msg->cluster_id)) {
    uav_msgs__msg__ClusterInfo__fini(msg);
    return false;
  }
  // ch_id
  if (!rosidl_runtime_c__String__init(&msg->ch_id)) {
    uav_msgs__msg__ClusterInfo__fini(msg);
    return false;
  }
  // member_ids
  if (!rosidl_runtime_c__String__Sequence__init(&msg->member_ids, 0)) {
    uav_msgs__msg__ClusterInfo__fini(msg);
    return false;
  }
  return true;
}

void
uav_msgs__msg__ClusterInfo__fini(uav_msgs__msg__ClusterInfo * msg)
{
  if (!msg) {
    return;
  }
  // cluster_id
  rosidl_runtime_c__String__fini(&msg->cluster_id);
  // ch_id
  rosidl_runtime_c__String__fini(&msg->ch_id);
  // member_ids
  rosidl_runtime_c__String__Sequence__fini(&msg->member_ids);
}

bool
uav_msgs__msg__ClusterInfo__are_equal(const uav_msgs__msg__ClusterInfo * lhs, const uav_msgs__msg__ClusterInfo * rhs)
{
  if (!lhs || !rhs) {
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
  // member_ids
  if (!rosidl_runtime_c__String__Sequence__are_equal(
      &(lhs->member_ids), &(rhs->member_ids)))
  {
    return false;
  }
  return true;
}

bool
uav_msgs__msg__ClusterInfo__copy(
  const uav_msgs__msg__ClusterInfo * input,
  uav_msgs__msg__ClusterInfo * output)
{
  if (!input || !output) {
    return false;
  }
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
  // member_ids
  if (!rosidl_runtime_c__String__Sequence__copy(
      &(input->member_ids), &(output->member_ids)))
  {
    return false;
  }
  return true;
}

uav_msgs__msg__ClusterInfo *
uav_msgs__msg__ClusterInfo__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__ClusterInfo * msg = (uav_msgs__msg__ClusterInfo *)allocator.allocate(sizeof(uav_msgs__msg__ClusterInfo), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(uav_msgs__msg__ClusterInfo));
  bool success = uav_msgs__msg__ClusterInfo__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
uav_msgs__msg__ClusterInfo__destroy(uav_msgs__msg__ClusterInfo * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    uav_msgs__msg__ClusterInfo__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
uav_msgs__msg__ClusterInfo__Sequence__init(uav_msgs__msg__ClusterInfo__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__ClusterInfo * data = NULL;

  if (size) {
    data = (uav_msgs__msg__ClusterInfo *)allocator.zero_allocate(size, sizeof(uav_msgs__msg__ClusterInfo), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = uav_msgs__msg__ClusterInfo__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        uav_msgs__msg__ClusterInfo__fini(&data[i - 1]);
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
uav_msgs__msg__ClusterInfo__Sequence__fini(uav_msgs__msg__ClusterInfo__Sequence * array)
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
      uav_msgs__msg__ClusterInfo__fini(&array->data[i]);
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

uav_msgs__msg__ClusterInfo__Sequence *
uav_msgs__msg__ClusterInfo__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__ClusterInfo__Sequence * array = (uav_msgs__msg__ClusterInfo__Sequence *)allocator.allocate(sizeof(uav_msgs__msg__ClusterInfo__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = uav_msgs__msg__ClusterInfo__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
uav_msgs__msg__ClusterInfo__Sequence__destroy(uav_msgs__msg__ClusterInfo__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    uav_msgs__msg__ClusterInfo__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
uav_msgs__msg__ClusterInfo__Sequence__are_equal(const uav_msgs__msg__ClusterInfo__Sequence * lhs, const uav_msgs__msg__ClusterInfo__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!uav_msgs__msg__ClusterInfo__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
uav_msgs__msg__ClusterInfo__Sequence__copy(
  const uav_msgs__msg__ClusterInfo__Sequence * input,
  uav_msgs__msg__ClusterInfo__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(uav_msgs__msg__ClusterInfo);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    uav_msgs__msg__ClusterInfo * data =
      (uav_msgs__msg__ClusterInfo *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!uav_msgs__msg__ClusterInfo__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          uav_msgs__msg__ClusterInfo__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!uav_msgs__msg__ClusterInfo__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
