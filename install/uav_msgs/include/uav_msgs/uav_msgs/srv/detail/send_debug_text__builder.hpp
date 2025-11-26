// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from uav_msgs:srv/SendDebugText.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__SRV__DETAIL__SEND_DEBUG_TEXT__BUILDER_HPP_
#define UAV_MSGS__SRV__DETAIL__SEND_DEBUG_TEXT__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "uav_msgs/srv/detail/send_debug_text__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace uav_msgs
{

namespace srv
{

namespace builder
{

class Init_SendDebugText_Request_text
{
public:
  explicit Init_SendDebugText_Request_text(::uav_msgs::srv::SendDebugText_Request & msg)
  : msg_(msg)
  {}
  ::uav_msgs::srv::SendDebugText_Request text(::uav_msgs::srv::SendDebugText_Request::_text_type arg)
  {
    msg_.text = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::srv::SendDebugText_Request msg_;
};

class Init_SendDebugText_Request_dst_id
{
public:
  Init_SendDebugText_Request_dst_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SendDebugText_Request_text dst_id(::uav_msgs::srv::SendDebugText_Request::_dst_id_type arg)
  {
    msg_.dst_id = std::move(arg);
    return Init_SendDebugText_Request_text(msg_);
  }

private:
  ::uav_msgs::srv::SendDebugText_Request msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::srv::SendDebugText_Request>()
{
  return uav_msgs::srv::builder::Init_SendDebugText_Request_dst_id();
}

}  // namespace uav_msgs


namespace uav_msgs
{

namespace srv
{

namespace builder
{

class Init_SendDebugText_Response_info
{
public:
  explicit Init_SendDebugText_Response_info(::uav_msgs::srv::SendDebugText_Response & msg)
  : msg_(msg)
  {}
  ::uav_msgs::srv::SendDebugText_Response info(::uav_msgs::srv::SendDebugText_Response::_info_type arg)
  {
    msg_.info = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::srv::SendDebugText_Response msg_;
};

class Init_SendDebugText_Response_accepted
{
public:
  Init_SendDebugText_Response_accepted()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SendDebugText_Response_info accepted(::uav_msgs::srv::SendDebugText_Response::_accepted_type arg)
  {
    msg_.accepted = std::move(arg);
    return Init_SendDebugText_Response_info(msg_);
  }

private:
  ::uav_msgs::srv::SendDebugText_Response msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::srv::SendDebugText_Response>()
{
  return uav_msgs::srv::builder::Init_SendDebugText_Response_accepted();
}

}  // namespace uav_msgs

#endif  // UAV_MSGS__SRV__DETAIL__SEND_DEBUG_TEXT__BUILDER_HPP_
