// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from uav_msgs:msg/FailureEvent.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__FAILURE_EVENT__BUILDER_HPP_
#define UAV_MSGS__MSG__DETAIL__FAILURE_EVENT__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "uav_msgs/msg/detail/failure_event__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace uav_msgs
{

namespace msg
{

namespace builder
{

class Init_FailureEvent_stamp
{
public:
  explicit Init_FailureEvent_stamp(::uav_msgs::msg::FailureEvent & msg)
  : msg_(msg)
  {}
  ::uav_msgs::msg::FailureEvent stamp(::uav_msgs::msg::FailureEvent::_stamp_type arg)
  {
    msg_.stamp = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::msg::FailureEvent msg_;
};

class Init_FailureEvent_description
{
public:
  explicit Init_FailureEvent_description(::uav_msgs::msg::FailureEvent & msg)
  : msg_(msg)
  {}
  Init_FailureEvent_stamp description(::uav_msgs::msg::FailureEvent::_description_type arg)
  {
    msg_.description = std::move(arg);
    return Init_FailureEvent_stamp(msg_);
  }

private:
  ::uav_msgs::msg::FailureEvent msg_;
};

class Init_FailureEvent_failure_type
{
public:
  explicit Init_FailureEvent_failure_type(::uav_msgs::msg::FailureEvent & msg)
  : msg_(msg)
  {}
  Init_FailureEvent_description failure_type(::uav_msgs::msg::FailureEvent::_failure_type_type arg)
  {
    msg_.failure_type = std::move(arg);
    return Init_FailureEvent_description(msg_);
  }

private:
  ::uav_msgs::msg::FailureEvent msg_;
};

class Init_FailureEvent_uav_id
{
public:
  Init_FailureEvent_uav_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_FailureEvent_failure_type uav_id(::uav_msgs::msg::FailureEvent::_uav_id_type arg)
  {
    msg_.uav_id = std::move(arg);
    return Init_FailureEvent_failure_type(msg_);
  }

private:
  ::uav_msgs::msg::FailureEvent msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::msg::FailureEvent>()
{
  return uav_msgs::msg::builder::Init_FailureEvent_uav_id();
}

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__FAILURE_EVENT__BUILDER_HPP_
