// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from uav_msgs:action/DockAndCharge.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__ACTION__DETAIL__DOCK_AND_CHARGE__BUILDER_HPP_
#define UAV_MSGS__ACTION__DETAIL__DOCK_AND_CHARGE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "uav_msgs/action/detail/dock_and_charge__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace uav_msgs
{

namespace action
{

namespace builder
{

class Init_DockAndCharge_Goal_uav_id
{
public:
  Init_DockAndCharge_Goal_uav_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::uav_msgs::action::DockAndCharge_Goal uav_id(::uav_msgs::action::DockAndCharge_Goal::_uav_id_type arg)
  {
    msg_.uav_id = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::action::DockAndCharge_Goal msg_;
};

}  // namespace builder

}  // namespace action

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::action::DockAndCharge_Goal>()
{
  return uav_msgs::action::builder::Init_DockAndCharge_Goal_uav_id();
}

}  // namespace uav_msgs


namespace uav_msgs
{

namespace action
{

namespace builder
{

class Init_DockAndCharge_Result_battery_level
{
public:
  Init_DockAndCharge_Result_battery_level()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::uav_msgs::action::DockAndCharge_Result battery_level(::uav_msgs::action::DockAndCharge_Result::_battery_level_type arg)
  {
    msg_.battery_level = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::action::DockAndCharge_Result msg_;
};

}  // namespace builder

}  // namespace action

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::action::DockAndCharge_Result>()
{
  return uav_msgs::action::builder::Init_DockAndCharge_Result_battery_level();
}

}  // namespace uav_msgs


namespace uav_msgs
{

namespace action
{

namespace builder
{

class Init_DockAndCharge_Feedback_current_battery
{
public:
  Init_DockAndCharge_Feedback_current_battery()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::uav_msgs::action::DockAndCharge_Feedback current_battery(::uav_msgs::action::DockAndCharge_Feedback::_current_battery_type arg)
  {
    msg_.current_battery = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::action::DockAndCharge_Feedback msg_;
};

}  // namespace builder

}  // namespace action

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::action::DockAndCharge_Feedback>()
{
  return uav_msgs::action::builder::Init_DockAndCharge_Feedback_current_battery();
}

}  // namespace uav_msgs


namespace uav_msgs
{

namespace action
{

namespace builder
{

class Init_DockAndCharge_SendGoal_Request_goal
{
public:
  explicit Init_DockAndCharge_SendGoal_Request_goal(::uav_msgs::action::DockAndCharge_SendGoal_Request & msg)
  : msg_(msg)
  {}
  ::uav_msgs::action::DockAndCharge_SendGoal_Request goal(::uav_msgs::action::DockAndCharge_SendGoal_Request::_goal_type arg)
  {
    msg_.goal = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::action::DockAndCharge_SendGoal_Request msg_;
};

class Init_DockAndCharge_SendGoal_Request_goal_id
{
public:
  Init_DockAndCharge_SendGoal_Request_goal_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_DockAndCharge_SendGoal_Request_goal goal_id(::uav_msgs::action::DockAndCharge_SendGoal_Request::_goal_id_type arg)
  {
    msg_.goal_id = std::move(arg);
    return Init_DockAndCharge_SendGoal_Request_goal(msg_);
  }

private:
  ::uav_msgs::action::DockAndCharge_SendGoal_Request msg_;
};

}  // namespace builder

}  // namespace action

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::action::DockAndCharge_SendGoal_Request>()
{
  return uav_msgs::action::builder::Init_DockAndCharge_SendGoal_Request_goal_id();
}

}  // namespace uav_msgs


namespace uav_msgs
{

namespace action
{

namespace builder
{

class Init_DockAndCharge_SendGoal_Response_stamp
{
public:
  explicit Init_DockAndCharge_SendGoal_Response_stamp(::uav_msgs::action::DockAndCharge_SendGoal_Response & msg)
  : msg_(msg)
  {}
  ::uav_msgs::action::DockAndCharge_SendGoal_Response stamp(::uav_msgs::action::DockAndCharge_SendGoal_Response::_stamp_type arg)
  {
    msg_.stamp = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::action::DockAndCharge_SendGoal_Response msg_;
};

class Init_DockAndCharge_SendGoal_Response_accepted
{
public:
  Init_DockAndCharge_SendGoal_Response_accepted()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_DockAndCharge_SendGoal_Response_stamp accepted(::uav_msgs::action::DockAndCharge_SendGoal_Response::_accepted_type arg)
  {
    msg_.accepted = std::move(arg);
    return Init_DockAndCharge_SendGoal_Response_stamp(msg_);
  }

private:
  ::uav_msgs::action::DockAndCharge_SendGoal_Response msg_;
};

}  // namespace builder

}  // namespace action

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::action::DockAndCharge_SendGoal_Response>()
{
  return uav_msgs::action::builder::Init_DockAndCharge_SendGoal_Response_accepted();
}

}  // namespace uav_msgs


namespace uav_msgs
{

namespace action
{

namespace builder
{

class Init_DockAndCharge_GetResult_Request_goal_id
{
public:
  Init_DockAndCharge_GetResult_Request_goal_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::uav_msgs::action::DockAndCharge_GetResult_Request goal_id(::uav_msgs::action::DockAndCharge_GetResult_Request::_goal_id_type arg)
  {
    msg_.goal_id = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::action::DockAndCharge_GetResult_Request msg_;
};

}  // namespace builder

}  // namespace action

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::action::DockAndCharge_GetResult_Request>()
{
  return uav_msgs::action::builder::Init_DockAndCharge_GetResult_Request_goal_id();
}

}  // namespace uav_msgs


namespace uav_msgs
{

namespace action
{

namespace builder
{

class Init_DockAndCharge_GetResult_Response_result
{
public:
  explicit Init_DockAndCharge_GetResult_Response_result(::uav_msgs::action::DockAndCharge_GetResult_Response & msg)
  : msg_(msg)
  {}
  ::uav_msgs::action::DockAndCharge_GetResult_Response result(::uav_msgs::action::DockAndCharge_GetResult_Response::_result_type arg)
  {
    msg_.result = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::action::DockAndCharge_GetResult_Response msg_;
};

class Init_DockAndCharge_GetResult_Response_status
{
public:
  Init_DockAndCharge_GetResult_Response_status()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_DockAndCharge_GetResult_Response_result status(::uav_msgs::action::DockAndCharge_GetResult_Response::_status_type arg)
  {
    msg_.status = std::move(arg);
    return Init_DockAndCharge_GetResult_Response_result(msg_);
  }

private:
  ::uav_msgs::action::DockAndCharge_GetResult_Response msg_;
};

}  // namespace builder

}  // namespace action

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::action::DockAndCharge_GetResult_Response>()
{
  return uav_msgs::action::builder::Init_DockAndCharge_GetResult_Response_status();
}

}  // namespace uav_msgs


namespace uav_msgs
{

namespace action
{

namespace builder
{

class Init_DockAndCharge_FeedbackMessage_feedback
{
public:
  explicit Init_DockAndCharge_FeedbackMessage_feedback(::uav_msgs::action::DockAndCharge_FeedbackMessage & msg)
  : msg_(msg)
  {}
  ::uav_msgs::action::DockAndCharge_FeedbackMessage feedback(::uav_msgs::action::DockAndCharge_FeedbackMessage::_feedback_type arg)
  {
    msg_.feedback = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::action::DockAndCharge_FeedbackMessage msg_;
};

class Init_DockAndCharge_FeedbackMessage_goal_id
{
public:
  Init_DockAndCharge_FeedbackMessage_goal_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_DockAndCharge_FeedbackMessage_feedback goal_id(::uav_msgs::action::DockAndCharge_FeedbackMessage::_goal_id_type arg)
  {
    msg_.goal_id = std::move(arg);
    return Init_DockAndCharge_FeedbackMessage_feedback(msg_);
  }

private:
  ::uav_msgs::action::DockAndCharge_FeedbackMessage msg_;
};

}  // namespace builder

}  // namespace action

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::action::DockAndCharge_FeedbackMessage>()
{
  return uav_msgs::action::builder::Init_DockAndCharge_FeedbackMessage_goal_id();
}

}  // namespace uav_msgs

#endif  // UAV_MSGS__ACTION__DETAIL__DOCK_AND_CHARGE__BUILDER_HPP_
