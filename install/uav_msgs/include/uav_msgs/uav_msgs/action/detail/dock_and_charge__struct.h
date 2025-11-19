// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from uav_msgs:action/DockAndCharge.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__ACTION__DETAIL__DOCK_AND_CHARGE__STRUCT_H_
#define UAV_MSGS__ACTION__DETAIL__DOCK_AND_CHARGE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'uav_id'
#include "rosidl_runtime_c/string.h"

/// Struct defined in action/DockAndCharge in the package uav_msgs.
typedef struct uav_msgs__action__DockAndCharge_Goal
{
  rosidl_runtime_c__String uav_id;
} uav_msgs__action__DockAndCharge_Goal;

// Struct for a sequence of uav_msgs__action__DockAndCharge_Goal.
typedef struct uav_msgs__action__DockAndCharge_Goal__Sequence
{
  uav_msgs__action__DockAndCharge_Goal * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__action__DockAndCharge_Goal__Sequence;


// Constants defined in the message

/// Struct defined in action/DockAndCharge in the package uav_msgs.
typedef struct uav_msgs__action__DockAndCharge_Result
{
  /// final battery level after charge
  float battery_level;
} uav_msgs__action__DockAndCharge_Result;

// Struct for a sequence of uav_msgs__action__DockAndCharge_Result.
typedef struct uav_msgs__action__DockAndCharge_Result__Sequence
{
  uav_msgs__action__DockAndCharge_Result * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__action__DockAndCharge_Result__Sequence;


// Constants defined in the message

/// Struct defined in action/DockAndCharge in the package uav_msgs.
typedef struct uav_msgs__action__DockAndCharge_Feedback
{
  float current_battery;
} uav_msgs__action__DockAndCharge_Feedback;

// Struct for a sequence of uav_msgs__action__DockAndCharge_Feedback.
typedef struct uav_msgs__action__DockAndCharge_Feedback__Sequence
{
  uav_msgs__action__DockAndCharge_Feedback * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__action__DockAndCharge_Feedback__Sequence;


// Constants defined in the message

// Include directives for member types
// Member 'goal_id'
#include "unique_identifier_msgs/msg/detail/uuid__struct.h"
// Member 'goal'
#include "uav_msgs/action/detail/dock_and_charge__struct.h"

/// Struct defined in action/DockAndCharge in the package uav_msgs.
typedef struct uav_msgs__action__DockAndCharge_SendGoal_Request
{
  unique_identifier_msgs__msg__UUID goal_id;
  uav_msgs__action__DockAndCharge_Goal goal;
} uav_msgs__action__DockAndCharge_SendGoal_Request;

// Struct for a sequence of uav_msgs__action__DockAndCharge_SendGoal_Request.
typedef struct uav_msgs__action__DockAndCharge_SendGoal_Request__Sequence
{
  uav_msgs__action__DockAndCharge_SendGoal_Request * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__action__DockAndCharge_SendGoal_Request__Sequence;


// Constants defined in the message

// Include directives for member types
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__struct.h"

/// Struct defined in action/DockAndCharge in the package uav_msgs.
typedef struct uav_msgs__action__DockAndCharge_SendGoal_Response
{
  bool accepted;
  builtin_interfaces__msg__Time stamp;
} uav_msgs__action__DockAndCharge_SendGoal_Response;

// Struct for a sequence of uav_msgs__action__DockAndCharge_SendGoal_Response.
typedef struct uav_msgs__action__DockAndCharge_SendGoal_Response__Sequence
{
  uav_msgs__action__DockAndCharge_SendGoal_Response * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__action__DockAndCharge_SendGoal_Response__Sequence;


// Constants defined in the message

// Include directives for member types
// Member 'goal_id'
// already included above
// #include "unique_identifier_msgs/msg/detail/uuid__struct.h"

/// Struct defined in action/DockAndCharge in the package uav_msgs.
typedef struct uav_msgs__action__DockAndCharge_GetResult_Request
{
  unique_identifier_msgs__msg__UUID goal_id;
} uav_msgs__action__DockAndCharge_GetResult_Request;

// Struct for a sequence of uav_msgs__action__DockAndCharge_GetResult_Request.
typedef struct uav_msgs__action__DockAndCharge_GetResult_Request__Sequence
{
  uav_msgs__action__DockAndCharge_GetResult_Request * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__action__DockAndCharge_GetResult_Request__Sequence;


// Constants defined in the message

// Include directives for member types
// Member 'result'
// already included above
// #include "uav_msgs/action/detail/dock_and_charge__struct.h"

/// Struct defined in action/DockAndCharge in the package uav_msgs.
typedef struct uav_msgs__action__DockAndCharge_GetResult_Response
{
  int8_t status;
  uav_msgs__action__DockAndCharge_Result result;
} uav_msgs__action__DockAndCharge_GetResult_Response;

// Struct for a sequence of uav_msgs__action__DockAndCharge_GetResult_Response.
typedef struct uav_msgs__action__DockAndCharge_GetResult_Response__Sequence
{
  uav_msgs__action__DockAndCharge_GetResult_Response * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__action__DockAndCharge_GetResult_Response__Sequence;


// Constants defined in the message

// Include directives for member types
// Member 'goal_id'
// already included above
// #include "unique_identifier_msgs/msg/detail/uuid__struct.h"
// Member 'feedback'
// already included above
// #include "uav_msgs/action/detail/dock_and_charge__struct.h"

/// Struct defined in action/DockAndCharge in the package uav_msgs.
typedef struct uav_msgs__action__DockAndCharge_FeedbackMessage
{
  unique_identifier_msgs__msg__UUID goal_id;
  uav_msgs__action__DockAndCharge_Feedback feedback;
} uav_msgs__action__DockAndCharge_FeedbackMessage;

// Struct for a sequence of uav_msgs__action__DockAndCharge_FeedbackMessage.
typedef struct uav_msgs__action__DockAndCharge_FeedbackMessage__Sequence
{
  uav_msgs__action__DockAndCharge_FeedbackMessage * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} uav_msgs__action__DockAndCharge_FeedbackMessage__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // UAV_MSGS__ACTION__DETAIL__DOCK_AND_CHARGE__STRUCT_H_
