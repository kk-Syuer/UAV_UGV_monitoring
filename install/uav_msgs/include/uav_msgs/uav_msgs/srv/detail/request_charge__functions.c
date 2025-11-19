// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from uav_msgs:srv/RequestCharge.idl
// generated code does not contain a copyright notice
#include "uav_msgs/srv/detail/request_charge__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"

// Include directives for member types
// Member `uav_id`
#include "rosidl_runtime_c/string_functions.h"

bool
uav_msgs__srv__RequestCharge_Request__init(uav_msgs__srv__RequestCharge_Request * msg)
{
  if (!msg) {
    return false;
  }
  // uav_id
  if (!rosidl_runtime_c__String__init(&msg->uav_id)) {
    uav_msgs__srv__RequestCharge_Request__fini(msg);
    return false;
  }
  // battery_level
  // role
  return true;
}

void
uav_msgs__srv__RequestCharge_Request__fini(uav_msgs__srv__RequestCharge_Request * msg)
{
  if (!msg) {
    return;
  }
  // uav_id
  rosidl_runtime_c__String__fini(&msg->uav_id);
  // battery_level
  // role
}

bool
uav_msgs__srv__RequestCharge_Request__are_equal(const uav_msgs__srv__RequestCharge_Request * lhs, const uav_msgs__srv__RequestCharge_Request * rhs)
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
  // battery_level
  if (lhs->battery_level != rhs->battery_level) {
    return false;
  }
  // role
  if (lhs->role != rhs->role) {
    return false;
  }
  return true;
}

bool
uav_msgs__srv__RequestCharge_Request__copy(
  const uav_msgs__srv__RequestCharge_Request * input,
  uav_msgs__srv__RequestCharge_Request * output)
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
  // battery_level
  output->battery_level = input->battery_level;
  // role
  output->role = input->role;
  return true;
}

uav_msgs__srv__RequestCharge_Request *
uav_msgs__srv__RequestCharge_Request__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__srv__RequestCharge_Request * msg = (uav_msgs__srv__RequestCharge_Request *)allocator.allocate(sizeof(uav_msgs__srv__RequestCharge_Request), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(uav_msgs__srv__RequestCharge_Request));
  bool success = uav_msgs__srv__RequestCharge_Request__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
uav_msgs__srv__RequestCharge_Request__destroy(uav_msgs__srv__RequestCharge_Request * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    uav_msgs__srv__RequestCharge_Request__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
uav_msgs__srv__RequestCharge_Request__Sequence__init(uav_msgs__srv__RequestCharge_Request__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__srv__RequestCharge_Request * data = NULL;

  if (size) {
    data = (uav_msgs__srv__RequestCharge_Request *)allocator.zero_allocate(size, sizeof(uav_msgs__srv__RequestCharge_Request), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = uav_msgs__srv__RequestCharge_Request__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        uav_msgs__srv__RequestCharge_Request__fini(&data[i - 1]);
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
uav_msgs__srv__RequestCharge_Request__Sequence__fini(uav_msgs__srv__RequestCharge_Request__Sequence * array)
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
      uav_msgs__srv__RequestCharge_Request__fini(&array->data[i]);
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

uav_msgs__srv__RequestCharge_Request__Sequence *
uav_msgs__srv__RequestCharge_Request__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__srv__RequestCharge_Request__Sequence * array = (uav_msgs__srv__RequestCharge_Request__Sequence *)allocator.allocate(sizeof(uav_msgs__srv__RequestCharge_Request__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = uav_msgs__srv__RequestCharge_Request__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
uav_msgs__srv__RequestCharge_Request__Sequence__destroy(uav_msgs__srv__RequestCharge_Request__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    uav_msgs__srv__RequestCharge_Request__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
uav_msgs__srv__RequestCharge_Request__Sequence__are_equal(const uav_msgs__srv__RequestCharge_Request__Sequence * lhs, const uav_msgs__srv__RequestCharge_Request__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!uav_msgs__srv__RequestCharge_Request__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
uav_msgs__srv__RequestCharge_Request__Sequence__copy(
  const uav_msgs__srv__RequestCharge_Request__Sequence * input,
  uav_msgs__srv__RequestCharge_Request__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(uav_msgs__srv__RequestCharge_Request);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    uav_msgs__srv__RequestCharge_Request * data =
      (uav_msgs__srv__RequestCharge_Request *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!uav_msgs__srv__RequestCharge_Request__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          uav_msgs__srv__RequestCharge_Request__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!uav_msgs__srv__RequestCharge_Request__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}


// Include directives for member types
// Member `eta`
#include "builtin_interfaces/msg/detail/time__functions.h"

bool
uav_msgs__srv__RequestCharge_Response__init(uav_msgs__srv__RequestCharge_Response * msg)
{
  if (!msg) {
    return false;
  }
  // accepted
  // eta
  if (!builtin_interfaces__msg__Time__init(&msg->eta)) {
    uav_msgs__srv__RequestCharge_Response__fini(msg);
    return false;
  }
  // assigned_priority
  return true;
}

void
uav_msgs__srv__RequestCharge_Response__fini(uav_msgs__srv__RequestCharge_Response * msg)
{
  if (!msg) {
    return;
  }
  // accepted
  // eta
  builtin_interfaces__msg__Time__fini(&msg->eta);
  // assigned_priority
}

bool
uav_msgs__srv__RequestCharge_Response__are_equal(const uav_msgs__srv__RequestCharge_Response * lhs, const uav_msgs__srv__RequestCharge_Response * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // accepted
  if (lhs->accepted != rhs->accepted) {
    return false;
  }
  // eta
  if (!builtin_interfaces__msg__Time__are_equal(
      &(lhs->eta), &(rhs->eta)))
  {
    return false;
  }
  // assigned_priority
  if (lhs->assigned_priority != rhs->assigned_priority) {
    return false;
  }
  return true;
}

bool
uav_msgs__srv__RequestCharge_Response__copy(
  const uav_msgs__srv__RequestCharge_Response * input,
  uav_msgs__srv__RequestCharge_Response * output)
{
  if (!input || !output) {
    return false;
  }
  // accepted
  output->accepted = input->accepted;
  // eta
  if (!builtin_interfaces__msg__Time__copy(
      &(input->eta), &(output->eta)))
  {
    return false;
  }
  // assigned_priority
  output->assigned_priority = input->assigned_priority;
  return true;
}

uav_msgs__srv__RequestCharge_Response *
uav_msgs__srv__RequestCharge_Response__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__srv__RequestCharge_Response * msg = (uav_msgs__srv__RequestCharge_Response *)allocator.allocate(sizeof(uav_msgs__srv__RequestCharge_Response), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(uav_msgs__srv__RequestCharge_Response));
  bool success = uav_msgs__srv__RequestCharge_Response__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
uav_msgs__srv__RequestCharge_Response__destroy(uav_msgs__srv__RequestCharge_Response * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    uav_msgs__srv__RequestCharge_Response__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
uav_msgs__srv__RequestCharge_Response__Sequence__init(uav_msgs__srv__RequestCharge_Response__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__srv__RequestCharge_Response * data = NULL;

  if (size) {
    data = (uav_msgs__srv__RequestCharge_Response *)allocator.zero_allocate(size, sizeof(uav_msgs__srv__RequestCharge_Response), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = uav_msgs__srv__RequestCharge_Response__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        uav_msgs__srv__RequestCharge_Response__fini(&data[i - 1]);
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
uav_msgs__srv__RequestCharge_Response__Sequence__fini(uav_msgs__srv__RequestCharge_Response__Sequence * array)
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
      uav_msgs__srv__RequestCharge_Response__fini(&array->data[i]);
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

uav_msgs__srv__RequestCharge_Response__Sequence *
uav_msgs__srv__RequestCharge_Response__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__srv__RequestCharge_Response__Sequence * array = (uav_msgs__srv__RequestCharge_Response__Sequence *)allocator.allocate(sizeof(uav_msgs__srv__RequestCharge_Response__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = uav_msgs__srv__RequestCharge_Response__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
uav_msgs__srv__RequestCharge_Response__Sequence__destroy(uav_msgs__srv__RequestCharge_Response__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    uav_msgs__srv__RequestCharge_Response__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
uav_msgs__srv__RequestCharge_Response__Sequence__are_equal(const uav_msgs__srv__RequestCharge_Response__Sequence * lhs, const uav_msgs__srv__RequestCharge_Response__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!uav_msgs__srv__RequestCharge_Response__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
uav_msgs__srv__RequestCharge_Response__Sequence__copy(
  const uav_msgs__srv__RequestCharge_Response__Sequence * input,
  uav_msgs__srv__RequestCharge_Response__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(uav_msgs__srv__RequestCharge_Response);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    uav_msgs__srv__RequestCharge_Response * data =
      (uav_msgs__srv__RequestCharge_Response *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!uav_msgs__srv__RequestCharge_Response__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          uav_msgs__srv__RequestCharge_Response__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!uav_msgs__srv__RequestCharge_Response__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
