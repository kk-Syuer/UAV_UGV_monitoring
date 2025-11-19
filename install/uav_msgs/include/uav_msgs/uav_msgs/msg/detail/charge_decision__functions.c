// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from uav_msgs:msg/ChargeDecision.idl
// generated code does not contain a copyright notice
#include "uav_msgs/msg/detail/charge_decision__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `uav_id`
// Member `policy`
#include "rosidl_runtime_c/string_functions.h"
// Member `slot_start_time`
#include "builtin_interfaces/msg/detail/time__functions.h"

bool
uav_msgs__msg__ChargeDecision__init(uav_msgs__msg__ChargeDecision * msg)
{
  if (!msg) {
    return false;
  }
  // uav_id
  if (!rosidl_runtime_c__String__init(&msg->uav_id)) {
    uav_msgs__msg__ChargeDecision__fini(msg);
    return false;
  }
  // accepted
  // slot_start_time
  if (!builtin_interfaces__msg__Time__init(&msg->slot_start_time)) {
    uav_msgs__msg__ChargeDecision__fini(msg);
    return false;
  }
  // priority
  // policy
  if (!rosidl_runtime_c__String__init(&msg->policy)) {
    uav_msgs__msg__ChargeDecision__fini(msg);
    return false;
  }
  return true;
}

void
uav_msgs__msg__ChargeDecision__fini(uav_msgs__msg__ChargeDecision * msg)
{
  if (!msg) {
    return;
  }
  // uav_id
  rosidl_runtime_c__String__fini(&msg->uav_id);
  // accepted
  // slot_start_time
  builtin_interfaces__msg__Time__fini(&msg->slot_start_time);
  // priority
  // policy
  rosidl_runtime_c__String__fini(&msg->policy);
}

bool
uav_msgs__msg__ChargeDecision__are_equal(const uav_msgs__msg__ChargeDecision * lhs, const uav_msgs__msg__ChargeDecision * rhs)
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
  // accepted
  if (lhs->accepted != rhs->accepted) {
    return false;
  }
  // slot_start_time
  if (!builtin_interfaces__msg__Time__are_equal(
      &(lhs->slot_start_time), &(rhs->slot_start_time)))
  {
    return false;
  }
  // priority
  if (lhs->priority != rhs->priority) {
    return false;
  }
  // policy
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->policy), &(rhs->policy)))
  {
    return false;
  }
  return true;
}

bool
uav_msgs__msg__ChargeDecision__copy(
  const uav_msgs__msg__ChargeDecision * input,
  uav_msgs__msg__ChargeDecision * output)
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
  // accepted
  output->accepted = input->accepted;
  // slot_start_time
  if (!builtin_interfaces__msg__Time__copy(
      &(input->slot_start_time), &(output->slot_start_time)))
  {
    return false;
  }
  // priority
  output->priority = input->priority;
  // policy
  if (!rosidl_runtime_c__String__copy(
      &(input->policy), &(output->policy)))
  {
    return false;
  }
  return true;
}

uav_msgs__msg__ChargeDecision *
uav_msgs__msg__ChargeDecision__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__ChargeDecision * msg = (uav_msgs__msg__ChargeDecision *)allocator.allocate(sizeof(uav_msgs__msg__ChargeDecision), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(uav_msgs__msg__ChargeDecision));
  bool success = uav_msgs__msg__ChargeDecision__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
uav_msgs__msg__ChargeDecision__destroy(uav_msgs__msg__ChargeDecision * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    uav_msgs__msg__ChargeDecision__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
uav_msgs__msg__ChargeDecision__Sequence__init(uav_msgs__msg__ChargeDecision__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__ChargeDecision * data = NULL;

  if (size) {
    data = (uav_msgs__msg__ChargeDecision *)allocator.zero_allocate(size, sizeof(uav_msgs__msg__ChargeDecision), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = uav_msgs__msg__ChargeDecision__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        uav_msgs__msg__ChargeDecision__fini(&data[i - 1]);
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
uav_msgs__msg__ChargeDecision__Sequence__fini(uav_msgs__msg__ChargeDecision__Sequence * array)
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
      uav_msgs__msg__ChargeDecision__fini(&array->data[i]);
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

uav_msgs__msg__ChargeDecision__Sequence *
uav_msgs__msg__ChargeDecision__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__ChargeDecision__Sequence * array = (uav_msgs__msg__ChargeDecision__Sequence *)allocator.allocate(sizeof(uav_msgs__msg__ChargeDecision__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = uav_msgs__msg__ChargeDecision__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
uav_msgs__msg__ChargeDecision__Sequence__destroy(uav_msgs__msg__ChargeDecision__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    uav_msgs__msg__ChargeDecision__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
uav_msgs__msg__ChargeDecision__Sequence__are_equal(const uav_msgs__msg__ChargeDecision__Sequence * lhs, const uav_msgs__msg__ChargeDecision__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!uav_msgs__msg__ChargeDecision__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
uav_msgs__msg__ChargeDecision__Sequence__copy(
  const uav_msgs__msg__ChargeDecision__Sequence * input,
  uav_msgs__msg__ChargeDecision__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(uav_msgs__msg__ChargeDecision);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    uav_msgs__msg__ChargeDecision * data =
      (uav_msgs__msg__ChargeDecision *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!uav_msgs__msg__ChargeDecision__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          uav_msgs__msg__ChargeDecision__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!uav_msgs__msg__ChargeDecision__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
