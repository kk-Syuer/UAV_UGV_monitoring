// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from uav_msgs:srv/SendDebugText.idl
// generated code does not contain a copyright notice
#include "uav_msgs/srv/detail/send_debug_text__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"

// Include directives for member types
// Member `dst_id`
// Member `text`
#include "rosidl_runtime_c/string_functions.h"

bool
uav_msgs__srv__SendDebugText_Request__init(uav_msgs__srv__SendDebugText_Request * msg)
{
  if (!msg) {
    return false;
  }
  // dst_id
  if (!rosidl_runtime_c__String__init(&msg->dst_id)) {
    uav_msgs__srv__SendDebugText_Request__fini(msg);
    return false;
  }
  // text
  if (!rosidl_runtime_c__String__init(&msg->text)) {
    uav_msgs__srv__SendDebugText_Request__fini(msg);
    return false;
  }
  return true;
}

void
uav_msgs__srv__SendDebugText_Request__fini(uav_msgs__srv__SendDebugText_Request * msg)
{
  if (!msg) {
    return;
  }
  // dst_id
  rosidl_runtime_c__String__fini(&msg->dst_id);
  // text
  rosidl_runtime_c__String__fini(&msg->text);
}

bool
uav_msgs__srv__SendDebugText_Request__are_equal(const uav_msgs__srv__SendDebugText_Request * lhs, const uav_msgs__srv__SendDebugText_Request * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // dst_id
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->dst_id), &(rhs->dst_id)))
  {
    return false;
  }
  // text
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->text), &(rhs->text)))
  {
    return false;
  }
  return true;
}

bool
uav_msgs__srv__SendDebugText_Request__copy(
  const uav_msgs__srv__SendDebugText_Request * input,
  uav_msgs__srv__SendDebugText_Request * output)
{
  if (!input || !output) {
    return false;
  }
  // dst_id
  if (!rosidl_runtime_c__String__copy(
      &(input->dst_id), &(output->dst_id)))
  {
    return false;
  }
  // text
  if (!rosidl_runtime_c__String__copy(
      &(input->text), &(output->text)))
  {
    return false;
  }
  return true;
}

uav_msgs__srv__SendDebugText_Request *
uav_msgs__srv__SendDebugText_Request__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__srv__SendDebugText_Request * msg = (uav_msgs__srv__SendDebugText_Request *)allocator.allocate(sizeof(uav_msgs__srv__SendDebugText_Request), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(uav_msgs__srv__SendDebugText_Request));
  bool success = uav_msgs__srv__SendDebugText_Request__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
uav_msgs__srv__SendDebugText_Request__destroy(uav_msgs__srv__SendDebugText_Request * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    uav_msgs__srv__SendDebugText_Request__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
uav_msgs__srv__SendDebugText_Request__Sequence__init(uav_msgs__srv__SendDebugText_Request__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__srv__SendDebugText_Request * data = NULL;

  if (size) {
    data = (uav_msgs__srv__SendDebugText_Request *)allocator.zero_allocate(size, sizeof(uav_msgs__srv__SendDebugText_Request), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = uav_msgs__srv__SendDebugText_Request__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        uav_msgs__srv__SendDebugText_Request__fini(&data[i - 1]);
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
uav_msgs__srv__SendDebugText_Request__Sequence__fini(uav_msgs__srv__SendDebugText_Request__Sequence * array)
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
      uav_msgs__srv__SendDebugText_Request__fini(&array->data[i]);
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

uav_msgs__srv__SendDebugText_Request__Sequence *
uav_msgs__srv__SendDebugText_Request__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__srv__SendDebugText_Request__Sequence * array = (uav_msgs__srv__SendDebugText_Request__Sequence *)allocator.allocate(sizeof(uav_msgs__srv__SendDebugText_Request__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = uav_msgs__srv__SendDebugText_Request__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
uav_msgs__srv__SendDebugText_Request__Sequence__destroy(uav_msgs__srv__SendDebugText_Request__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    uav_msgs__srv__SendDebugText_Request__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
uav_msgs__srv__SendDebugText_Request__Sequence__are_equal(const uav_msgs__srv__SendDebugText_Request__Sequence * lhs, const uav_msgs__srv__SendDebugText_Request__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!uav_msgs__srv__SendDebugText_Request__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
uav_msgs__srv__SendDebugText_Request__Sequence__copy(
  const uav_msgs__srv__SendDebugText_Request__Sequence * input,
  uav_msgs__srv__SendDebugText_Request__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(uav_msgs__srv__SendDebugText_Request);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    uav_msgs__srv__SendDebugText_Request * data =
      (uav_msgs__srv__SendDebugText_Request *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!uav_msgs__srv__SendDebugText_Request__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          uav_msgs__srv__SendDebugText_Request__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!uav_msgs__srv__SendDebugText_Request__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}


// Include directives for member types
// Member `info`
// already included above
// #include "rosidl_runtime_c/string_functions.h"

bool
uav_msgs__srv__SendDebugText_Response__init(uav_msgs__srv__SendDebugText_Response * msg)
{
  if (!msg) {
    return false;
  }
  // accepted
  // info
  if (!rosidl_runtime_c__String__init(&msg->info)) {
    uav_msgs__srv__SendDebugText_Response__fini(msg);
    return false;
  }
  return true;
}

void
uav_msgs__srv__SendDebugText_Response__fini(uav_msgs__srv__SendDebugText_Response * msg)
{
  if (!msg) {
    return;
  }
  // accepted
  // info
  rosidl_runtime_c__String__fini(&msg->info);
}

bool
uav_msgs__srv__SendDebugText_Response__are_equal(const uav_msgs__srv__SendDebugText_Response * lhs, const uav_msgs__srv__SendDebugText_Response * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // accepted
  if (lhs->accepted != rhs->accepted) {
    return false;
  }
  // info
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->info), &(rhs->info)))
  {
    return false;
  }
  return true;
}

bool
uav_msgs__srv__SendDebugText_Response__copy(
  const uav_msgs__srv__SendDebugText_Response * input,
  uav_msgs__srv__SendDebugText_Response * output)
{
  if (!input || !output) {
    return false;
  }
  // accepted
  output->accepted = input->accepted;
  // info
  if (!rosidl_runtime_c__String__copy(
      &(input->info), &(output->info)))
  {
    return false;
  }
  return true;
}

uav_msgs__srv__SendDebugText_Response *
uav_msgs__srv__SendDebugText_Response__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__srv__SendDebugText_Response * msg = (uav_msgs__srv__SendDebugText_Response *)allocator.allocate(sizeof(uav_msgs__srv__SendDebugText_Response), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(uav_msgs__srv__SendDebugText_Response));
  bool success = uav_msgs__srv__SendDebugText_Response__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
uav_msgs__srv__SendDebugText_Response__destroy(uav_msgs__srv__SendDebugText_Response * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    uav_msgs__srv__SendDebugText_Response__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
uav_msgs__srv__SendDebugText_Response__Sequence__init(uav_msgs__srv__SendDebugText_Response__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__srv__SendDebugText_Response * data = NULL;

  if (size) {
    data = (uav_msgs__srv__SendDebugText_Response *)allocator.zero_allocate(size, sizeof(uav_msgs__srv__SendDebugText_Response), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = uav_msgs__srv__SendDebugText_Response__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        uav_msgs__srv__SendDebugText_Response__fini(&data[i - 1]);
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
uav_msgs__srv__SendDebugText_Response__Sequence__fini(uav_msgs__srv__SendDebugText_Response__Sequence * array)
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
      uav_msgs__srv__SendDebugText_Response__fini(&array->data[i]);
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

uav_msgs__srv__SendDebugText_Response__Sequence *
uav_msgs__srv__SendDebugText_Response__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__srv__SendDebugText_Response__Sequence * array = (uav_msgs__srv__SendDebugText_Response__Sequence *)allocator.allocate(sizeof(uav_msgs__srv__SendDebugText_Response__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = uav_msgs__srv__SendDebugText_Response__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
uav_msgs__srv__SendDebugText_Response__Sequence__destroy(uav_msgs__srv__SendDebugText_Response__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    uav_msgs__srv__SendDebugText_Response__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
uav_msgs__srv__SendDebugText_Response__Sequence__are_equal(const uav_msgs__srv__SendDebugText_Response__Sequence * lhs, const uav_msgs__srv__SendDebugText_Response__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!uav_msgs__srv__SendDebugText_Response__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
uav_msgs__srv__SendDebugText_Response__Sequence__copy(
  const uav_msgs__srv__SendDebugText_Response__Sequence * input,
  uav_msgs__srv__SendDebugText_Response__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(uav_msgs__srv__SendDebugText_Response);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    uav_msgs__srv__SendDebugText_Response * data =
      (uav_msgs__srv__SendDebugText_Response *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!uav_msgs__srv__SendDebugText_Response__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          uav_msgs__srv__SendDebugText_Response__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!uav_msgs__srv__SendDebugText_Response__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
