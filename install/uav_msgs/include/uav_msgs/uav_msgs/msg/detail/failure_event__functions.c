// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from uav_msgs:msg/FailureEvent.idl
// generated code does not contain a copyright notice
#include "uav_msgs/msg/detail/failure_event__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `uav_id`
// Member `description`
#include "rosidl_runtime_c/string_functions.h"
// Member `stamp`
#include "builtin_interfaces/msg/detail/time__functions.h"

bool
uav_msgs__msg__FailureEvent__init(uav_msgs__msg__FailureEvent * msg)
{
  if (!msg) {
    return false;
  }
  // uav_id
  if (!rosidl_runtime_c__String__init(&msg->uav_id)) {
    uav_msgs__msg__FailureEvent__fini(msg);
    return false;
  }
  // failure_type
  // description
  if (!rosidl_runtime_c__String__init(&msg->description)) {
    uav_msgs__msg__FailureEvent__fini(msg);
    return false;
  }
  // stamp
  if (!builtin_interfaces__msg__Time__init(&msg->stamp)) {
    uav_msgs__msg__FailureEvent__fini(msg);
    return false;
  }
  return true;
}

void
uav_msgs__msg__FailureEvent__fini(uav_msgs__msg__FailureEvent * msg)
{
  if (!msg) {
    return;
  }
  // uav_id
  rosidl_runtime_c__String__fini(&msg->uav_id);
  // failure_type
  // description
  rosidl_runtime_c__String__fini(&msg->description);
  // stamp
  builtin_interfaces__msg__Time__fini(&msg->stamp);
}

bool
uav_msgs__msg__FailureEvent__are_equal(const uav_msgs__msg__FailureEvent * lhs, const uav_msgs__msg__FailureEvent * rhs)
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
  // failure_type
  if (lhs->failure_type != rhs->failure_type) {
    return false;
  }
  // description
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->description), &(rhs->description)))
  {
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
uav_msgs__msg__FailureEvent__copy(
  const uav_msgs__msg__FailureEvent * input,
  uav_msgs__msg__FailureEvent * output)
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
  // failure_type
  output->failure_type = input->failure_type;
  // description
  if (!rosidl_runtime_c__String__copy(
      &(input->description), &(output->description)))
  {
    return false;
  }
  // stamp
  if (!builtin_interfaces__msg__Time__copy(
      &(input->stamp), &(output->stamp)))
  {
    return false;
  }
  return true;
}

uav_msgs__msg__FailureEvent *
uav_msgs__msg__FailureEvent__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__FailureEvent * msg = (uav_msgs__msg__FailureEvent *)allocator.allocate(sizeof(uav_msgs__msg__FailureEvent), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(uav_msgs__msg__FailureEvent));
  bool success = uav_msgs__msg__FailureEvent__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
uav_msgs__msg__FailureEvent__destroy(uav_msgs__msg__FailureEvent * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    uav_msgs__msg__FailureEvent__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
uav_msgs__msg__FailureEvent__Sequence__init(uav_msgs__msg__FailureEvent__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__FailureEvent * data = NULL;

  if (size) {
    data = (uav_msgs__msg__FailureEvent *)allocator.zero_allocate(size, sizeof(uav_msgs__msg__FailureEvent), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = uav_msgs__msg__FailureEvent__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        uav_msgs__msg__FailureEvent__fini(&data[i - 1]);
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
uav_msgs__msg__FailureEvent__Sequence__fini(uav_msgs__msg__FailureEvent__Sequence * array)
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
      uav_msgs__msg__FailureEvent__fini(&array->data[i]);
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

uav_msgs__msg__FailureEvent__Sequence *
uav_msgs__msg__FailureEvent__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__FailureEvent__Sequence * array = (uav_msgs__msg__FailureEvent__Sequence *)allocator.allocate(sizeof(uav_msgs__msg__FailureEvent__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = uav_msgs__msg__FailureEvent__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
uav_msgs__msg__FailureEvent__Sequence__destroy(uav_msgs__msg__FailureEvent__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    uav_msgs__msg__FailureEvent__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
uav_msgs__msg__FailureEvent__Sequence__are_equal(const uav_msgs__msg__FailureEvent__Sequence * lhs, const uav_msgs__msg__FailureEvent__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!uav_msgs__msg__FailureEvent__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
uav_msgs__msg__FailureEvent__Sequence__copy(
  const uav_msgs__msg__FailureEvent__Sequence * input,
  uav_msgs__msg__FailureEvent__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(uav_msgs__msg__FailureEvent);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    uav_msgs__msg__FailureEvent * data =
      (uav_msgs__msg__FailureEvent *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!uav_msgs__msg__FailureEvent__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          uav_msgs__msg__FailureEvent__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!uav_msgs__msg__FailureEvent__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
