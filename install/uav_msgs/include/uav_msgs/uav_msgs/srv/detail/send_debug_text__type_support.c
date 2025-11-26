// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from uav_msgs:srv/SendDebugText.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "uav_msgs/srv/detail/send_debug_text__rosidl_typesupport_introspection_c.h"
#include "uav_msgs/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "uav_msgs/srv/detail/send_debug_text__functions.h"
#include "uav_msgs/srv/detail/send_debug_text__struct.h"


// Include directives for member types
// Member `dst_id`
// Member `text`
#include "rosidl_runtime_c/string_functions.h"

#ifdef __cplusplus
extern "C"
{
#endif

void uav_msgs__srv__SendDebugText_Request__rosidl_typesupport_introspection_c__SendDebugText_Request_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  uav_msgs__srv__SendDebugText_Request__init(message_memory);
}

void uav_msgs__srv__SendDebugText_Request__rosidl_typesupport_introspection_c__SendDebugText_Request_fini_function(void * message_memory)
{
  uav_msgs__srv__SendDebugText_Request__fini(message_memory);
}

static rosidl_typesupport_introspection_c__MessageMember uav_msgs__srv__SendDebugText_Request__rosidl_typesupport_introspection_c__SendDebugText_Request_message_member_array[2] = {
  {
    "dst_id",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__srv__SendDebugText_Request, dst_id),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "text",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__srv__SendDebugText_Request, text),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers uav_msgs__srv__SendDebugText_Request__rosidl_typesupport_introspection_c__SendDebugText_Request_message_members = {
  "uav_msgs__srv",  // message namespace
  "SendDebugText_Request",  // message name
  2,  // number of fields
  sizeof(uav_msgs__srv__SendDebugText_Request),
  uav_msgs__srv__SendDebugText_Request__rosidl_typesupport_introspection_c__SendDebugText_Request_message_member_array,  // message members
  uav_msgs__srv__SendDebugText_Request__rosidl_typesupport_introspection_c__SendDebugText_Request_init_function,  // function to initialize message memory (memory has to be allocated)
  uav_msgs__srv__SendDebugText_Request__rosidl_typesupport_introspection_c__SendDebugText_Request_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t uav_msgs__srv__SendDebugText_Request__rosidl_typesupport_introspection_c__SendDebugText_Request_message_type_support_handle = {
  0,
  &uav_msgs__srv__SendDebugText_Request__rosidl_typesupport_introspection_c__SendDebugText_Request_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_uav_msgs
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, uav_msgs, srv, SendDebugText_Request)() {
  if (!uav_msgs__srv__SendDebugText_Request__rosidl_typesupport_introspection_c__SendDebugText_Request_message_type_support_handle.typesupport_identifier) {
    uav_msgs__srv__SendDebugText_Request__rosidl_typesupport_introspection_c__SendDebugText_Request_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &uav_msgs__srv__SendDebugText_Request__rosidl_typesupport_introspection_c__SendDebugText_Request_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif

// already included above
// #include <stddef.h>
// already included above
// #include "uav_msgs/srv/detail/send_debug_text__rosidl_typesupport_introspection_c.h"
// already included above
// #include "uav_msgs/msg/rosidl_typesupport_introspection_c__visibility_control.h"
// already included above
// #include "rosidl_typesupport_introspection_c/field_types.h"
// already included above
// #include "rosidl_typesupport_introspection_c/identifier.h"
// already included above
// #include "rosidl_typesupport_introspection_c/message_introspection.h"
// already included above
// #include "uav_msgs/srv/detail/send_debug_text__functions.h"
// already included above
// #include "uav_msgs/srv/detail/send_debug_text__struct.h"


// Include directives for member types
// Member `info`
// already included above
// #include "rosidl_runtime_c/string_functions.h"

#ifdef __cplusplus
extern "C"
{
#endif

void uav_msgs__srv__SendDebugText_Response__rosidl_typesupport_introspection_c__SendDebugText_Response_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  uav_msgs__srv__SendDebugText_Response__init(message_memory);
}

void uav_msgs__srv__SendDebugText_Response__rosidl_typesupport_introspection_c__SendDebugText_Response_fini_function(void * message_memory)
{
  uav_msgs__srv__SendDebugText_Response__fini(message_memory);
}

static rosidl_typesupport_introspection_c__MessageMember uav_msgs__srv__SendDebugText_Response__rosidl_typesupport_introspection_c__SendDebugText_Response_message_member_array[2] = {
  {
    "accepted",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_BOOLEAN,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__srv__SendDebugText_Response, accepted),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "info",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(uav_msgs__srv__SendDebugText_Response, info),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers uav_msgs__srv__SendDebugText_Response__rosidl_typesupport_introspection_c__SendDebugText_Response_message_members = {
  "uav_msgs__srv",  // message namespace
  "SendDebugText_Response",  // message name
  2,  // number of fields
  sizeof(uav_msgs__srv__SendDebugText_Response),
  uav_msgs__srv__SendDebugText_Response__rosidl_typesupport_introspection_c__SendDebugText_Response_message_member_array,  // message members
  uav_msgs__srv__SendDebugText_Response__rosidl_typesupport_introspection_c__SendDebugText_Response_init_function,  // function to initialize message memory (memory has to be allocated)
  uav_msgs__srv__SendDebugText_Response__rosidl_typesupport_introspection_c__SendDebugText_Response_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t uav_msgs__srv__SendDebugText_Response__rosidl_typesupport_introspection_c__SendDebugText_Response_message_type_support_handle = {
  0,
  &uav_msgs__srv__SendDebugText_Response__rosidl_typesupport_introspection_c__SendDebugText_Response_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_uav_msgs
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, uav_msgs, srv, SendDebugText_Response)() {
  if (!uav_msgs__srv__SendDebugText_Response__rosidl_typesupport_introspection_c__SendDebugText_Response_message_type_support_handle.typesupport_identifier) {
    uav_msgs__srv__SendDebugText_Response__rosidl_typesupport_introspection_c__SendDebugText_Response_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &uav_msgs__srv__SendDebugText_Response__rosidl_typesupport_introspection_c__SendDebugText_Response_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif

#include "rosidl_runtime_c/service_type_support_struct.h"
// already included above
// #include "uav_msgs/msg/rosidl_typesupport_introspection_c__visibility_control.h"
// already included above
// #include "uav_msgs/srv/detail/send_debug_text__rosidl_typesupport_introspection_c.h"
// already included above
// #include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"

// this is intentionally not const to allow initialization later to prevent an initialization race
static rosidl_typesupport_introspection_c__ServiceMembers uav_msgs__srv__detail__send_debug_text__rosidl_typesupport_introspection_c__SendDebugText_service_members = {
  "uav_msgs__srv",  // service namespace
  "SendDebugText",  // service name
  // these two fields are initialized below on the first access
  NULL,  // request message
  // uav_msgs__srv__detail__send_debug_text__rosidl_typesupport_introspection_c__SendDebugText_Request_message_type_support_handle,
  NULL  // response message
  // uav_msgs__srv__detail__send_debug_text__rosidl_typesupport_introspection_c__SendDebugText_Response_message_type_support_handle
};

static rosidl_service_type_support_t uav_msgs__srv__detail__send_debug_text__rosidl_typesupport_introspection_c__SendDebugText_service_type_support_handle = {
  0,
  &uav_msgs__srv__detail__send_debug_text__rosidl_typesupport_introspection_c__SendDebugText_service_members,
  get_service_typesupport_handle_function,
};

// Forward declaration of request/response type support functions
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, uav_msgs, srv, SendDebugText_Request)();

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, uav_msgs, srv, SendDebugText_Response)();

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_uav_msgs
const rosidl_service_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_SYMBOL_NAME(rosidl_typesupport_introspection_c, uav_msgs, srv, SendDebugText)() {
  if (!uav_msgs__srv__detail__send_debug_text__rosidl_typesupport_introspection_c__SendDebugText_service_type_support_handle.typesupport_identifier) {
    uav_msgs__srv__detail__send_debug_text__rosidl_typesupport_introspection_c__SendDebugText_service_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  rosidl_typesupport_introspection_c__ServiceMembers * service_members =
    (rosidl_typesupport_introspection_c__ServiceMembers *)uav_msgs__srv__detail__send_debug_text__rosidl_typesupport_introspection_c__SendDebugText_service_type_support_handle.data;

  if (!service_members->request_members_) {
    service_members->request_members_ =
      (const rosidl_typesupport_introspection_c__MessageMembers *)
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, uav_msgs, srv, SendDebugText_Request)()->data;
  }
  if (!service_members->response_members_) {
    service_members->response_members_ =
      (const rosidl_typesupport_introspection_c__MessageMembers *)
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, uav_msgs, srv, SendDebugText_Response)()->data;
  }

  return &uav_msgs__srv__detail__send_debug_text__rosidl_typesupport_introspection_c__SendDebugText_service_type_support_handle;
}
