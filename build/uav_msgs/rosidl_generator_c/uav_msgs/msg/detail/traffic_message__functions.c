// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from uav_msgs:msg/TrafficMessage.idl
// generated code does not contain a copyright notice
#include "uav_msgs/msg/detail/traffic_message__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `msg_id`
// Member `src_id`
// Member `dst_id`
// Member `next_hop_id`
// Member `control_type`
// Member `payload`
#include "rosidl_runtime_c/string_functions.h"
// Member `creation_time`
#include "builtin_interfaces/msg/detail/time__functions.h"

bool
uav_msgs__msg__TrafficMessage__init(uav_msgs__msg__TrafficMessage * msg)
{
  if (!msg) {
    return false;
  }
  // msg_id
  if (!rosidl_runtime_c__String__init(&msg->msg_id)) {
    uav_msgs__msg__TrafficMessage__fini(msg);
    return false;
  }
  // src_id
  if (!rosidl_runtime_c__String__init(&msg->src_id)) {
    uav_msgs__msg__TrafficMessage__fini(msg);
    return false;
  }
  // dst_id
  if (!rosidl_runtime_c__String__init(&msg->dst_id)) {
    uav_msgs__msg__TrafficMessage__fini(msg);
    return false;
  }
  // next_hop_id
  if (!rosidl_runtime_c__String__init(&msg->next_hop_id)) {
    uav_msgs__msg__TrafficMessage__fini(msg);
    return false;
  }
  // flow_type
  // control_type
  if (!rosidl_runtime_c__String__init(&msg->control_type)) {
    uav_msgs__msg__TrafficMessage__fini(msg);
    return false;
  }
  // seq
  // hop_count
  // ttl
  // requires_ack
  // payload
  if (!rosidl_runtime_c__String__init(&msg->payload)) {
    uav_msgs__msg__TrafficMessage__fini(msg);
    return false;
  }
  // creation_time
  if (!builtin_interfaces__msg__Time__init(&msg->creation_time)) {
    uav_msgs__msg__TrafficMessage__fini(msg);
    return false;
  }
  return true;
}

void
uav_msgs__msg__TrafficMessage__fini(uav_msgs__msg__TrafficMessage * msg)
{
  if (!msg) {
    return;
  }
  // msg_id
  rosidl_runtime_c__String__fini(&msg->msg_id);
  // src_id
  rosidl_runtime_c__String__fini(&msg->src_id);
  // dst_id
  rosidl_runtime_c__String__fini(&msg->dst_id);
  // next_hop_id
  rosidl_runtime_c__String__fini(&msg->next_hop_id);
  // flow_type
  // control_type
  rosidl_runtime_c__String__fini(&msg->control_type);
  // seq
  // hop_count
  // ttl
  // requires_ack
  // payload
  rosidl_runtime_c__String__fini(&msg->payload);
  // creation_time
  builtin_interfaces__msg__Time__fini(&msg->creation_time);
}

bool
uav_msgs__msg__TrafficMessage__are_equal(const uav_msgs__msg__TrafficMessage * lhs, const uav_msgs__msg__TrafficMessage * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // msg_id
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->msg_id), &(rhs->msg_id)))
  {
    return false;
  }
  // src_id
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->src_id), &(rhs->src_id)))
  {
    return false;
  }
  // dst_id
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->dst_id), &(rhs->dst_id)))
  {
    return false;
  }
  // next_hop_id
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->next_hop_id), &(rhs->next_hop_id)))
  {
    return false;
  }
  // flow_type
  if (lhs->flow_type != rhs->flow_type) {
    return false;
  }
  // control_type
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->control_type), &(rhs->control_type)))
  {
    return false;
  }
  // seq
  if (lhs->seq != rhs->seq) {
    return false;
  }
  // hop_count
  if (lhs->hop_count != rhs->hop_count) {
    return false;
  }
  // ttl
  if (lhs->ttl != rhs->ttl) {
    return false;
  }
  // requires_ack
  if (lhs->requires_ack != rhs->requires_ack) {
    return false;
  }
  // payload
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->payload), &(rhs->payload)))
  {
    return false;
  }
  // creation_time
  if (!builtin_interfaces__msg__Time__are_equal(
      &(lhs->creation_time), &(rhs->creation_time)))
  {
    return false;
  }
  return true;
}

bool
uav_msgs__msg__TrafficMessage__copy(
  const uav_msgs__msg__TrafficMessage * input,
  uav_msgs__msg__TrafficMessage * output)
{
  if (!input || !output) {
    return false;
  }
  // msg_id
  if (!rosidl_runtime_c__String__copy(
      &(input->msg_id), &(output->msg_id)))
  {
    return false;
  }
  // src_id
  if (!rosidl_runtime_c__String__copy(
      &(input->src_id), &(output->src_id)))
  {
    return false;
  }
  // dst_id
  if (!rosidl_runtime_c__String__copy(
      &(input->dst_id), &(output->dst_id)))
  {
    return false;
  }
  // next_hop_id
  if (!rosidl_runtime_c__String__copy(
      &(input->next_hop_id), &(output->next_hop_id)))
  {
    return false;
  }
  // flow_type
  output->flow_type = input->flow_type;
  // control_type
  if (!rosidl_runtime_c__String__copy(
      &(input->control_type), &(output->control_type)))
  {
    return false;
  }
  // seq
  output->seq = input->seq;
  // hop_count
  output->hop_count = input->hop_count;
  // ttl
  output->ttl = input->ttl;
  // requires_ack
  output->requires_ack = input->requires_ack;
  // payload
  if (!rosidl_runtime_c__String__copy(
      &(input->payload), &(output->payload)))
  {
    return false;
  }
  // creation_time
  if (!builtin_interfaces__msg__Time__copy(
      &(input->creation_time), &(output->creation_time)))
  {
    return false;
  }
  return true;
}

uav_msgs__msg__TrafficMessage *
uav_msgs__msg__TrafficMessage__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__TrafficMessage * msg = (uav_msgs__msg__TrafficMessage *)allocator.allocate(sizeof(uav_msgs__msg__TrafficMessage), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(uav_msgs__msg__TrafficMessage));
  bool success = uav_msgs__msg__TrafficMessage__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
uav_msgs__msg__TrafficMessage__destroy(uav_msgs__msg__TrafficMessage * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    uav_msgs__msg__TrafficMessage__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
uav_msgs__msg__TrafficMessage__Sequence__init(uav_msgs__msg__TrafficMessage__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__TrafficMessage * data = NULL;

  if (size) {
    data = (uav_msgs__msg__TrafficMessage *)allocator.zero_allocate(size, sizeof(uav_msgs__msg__TrafficMessage), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = uav_msgs__msg__TrafficMessage__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        uav_msgs__msg__TrafficMessage__fini(&data[i - 1]);
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
uav_msgs__msg__TrafficMessage__Sequence__fini(uav_msgs__msg__TrafficMessage__Sequence * array)
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
      uav_msgs__msg__TrafficMessage__fini(&array->data[i]);
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

uav_msgs__msg__TrafficMessage__Sequence *
uav_msgs__msg__TrafficMessage__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  uav_msgs__msg__TrafficMessage__Sequence * array = (uav_msgs__msg__TrafficMessage__Sequence *)allocator.allocate(sizeof(uav_msgs__msg__TrafficMessage__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = uav_msgs__msg__TrafficMessage__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
uav_msgs__msg__TrafficMessage__Sequence__destroy(uav_msgs__msg__TrafficMessage__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    uav_msgs__msg__TrafficMessage__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
uav_msgs__msg__TrafficMessage__Sequence__are_equal(const uav_msgs__msg__TrafficMessage__Sequence * lhs, const uav_msgs__msg__TrafficMessage__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!uav_msgs__msg__TrafficMessage__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
uav_msgs__msg__TrafficMessage__Sequence__copy(
  const uav_msgs__msg__TrafficMessage__Sequence * input,
  uav_msgs__msg__TrafficMessage__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(uav_msgs__msg__TrafficMessage);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    uav_msgs__msg__TrafficMessage * data =
      (uav_msgs__msg__TrafficMessage *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!uav_msgs__msg__TrafficMessage__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          uav_msgs__msg__TrafficMessage__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!uav_msgs__msg__TrafficMessage__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
