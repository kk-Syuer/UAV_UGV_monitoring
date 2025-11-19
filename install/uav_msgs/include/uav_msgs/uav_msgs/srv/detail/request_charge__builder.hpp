// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from uav_msgs:srv/RequestCharge.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__SRV__DETAIL__REQUEST_CHARGE__BUILDER_HPP_
#define UAV_MSGS__SRV__DETAIL__REQUEST_CHARGE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "uav_msgs/srv/detail/request_charge__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace uav_msgs
{

namespace srv
{

namespace builder
{

class Init_RequestCharge_Request_role
{
public:
  explicit Init_RequestCharge_Request_role(::uav_msgs::srv::RequestCharge_Request & msg)
  : msg_(msg)
  {}
  ::uav_msgs::srv::RequestCharge_Request role(::uav_msgs::srv::RequestCharge_Request::_role_type arg)
  {
    msg_.role = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::srv::RequestCharge_Request msg_;
};

class Init_RequestCharge_Request_battery_level
{
public:
  explicit Init_RequestCharge_Request_battery_level(::uav_msgs::srv::RequestCharge_Request & msg)
  : msg_(msg)
  {}
  Init_RequestCharge_Request_role battery_level(::uav_msgs::srv::RequestCharge_Request::_battery_level_type arg)
  {
    msg_.battery_level = std::move(arg);
    return Init_RequestCharge_Request_role(msg_);
  }

private:
  ::uav_msgs::srv::RequestCharge_Request msg_;
};

class Init_RequestCharge_Request_uav_id
{
public:
  Init_RequestCharge_Request_uav_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_RequestCharge_Request_battery_level uav_id(::uav_msgs::srv::RequestCharge_Request::_uav_id_type arg)
  {
    msg_.uav_id = std::move(arg);
    return Init_RequestCharge_Request_battery_level(msg_);
  }

private:
  ::uav_msgs::srv::RequestCharge_Request msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::srv::RequestCharge_Request>()
{
  return uav_msgs::srv::builder::Init_RequestCharge_Request_uav_id();
}

}  // namespace uav_msgs


namespace uav_msgs
{

namespace srv
{

namespace builder
{

class Init_RequestCharge_Response_assigned_priority
{
public:
  explicit Init_RequestCharge_Response_assigned_priority(::uav_msgs::srv::RequestCharge_Response & msg)
  : msg_(msg)
  {}
  ::uav_msgs::srv::RequestCharge_Response assigned_priority(::uav_msgs::srv::RequestCharge_Response::_assigned_priority_type arg)
  {
    msg_.assigned_priority = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::srv::RequestCharge_Response msg_;
};

class Init_RequestCharge_Response_eta
{
public:
  explicit Init_RequestCharge_Response_eta(::uav_msgs::srv::RequestCharge_Response & msg)
  : msg_(msg)
  {}
  Init_RequestCharge_Response_assigned_priority eta(::uav_msgs::srv::RequestCharge_Response::_eta_type arg)
  {
    msg_.eta = std::move(arg);
    return Init_RequestCharge_Response_assigned_priority(msg_);
  }

private:
  ::uav_msgs::srv::RequestCharge_Response msg_;
};

class Init_RequestCharge_Response_accepted
{
public:
  Init_RequestCharge_Response_accepted()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_RequestCharge_Response_eta accepted(::uav_msgs::srv::RequestCharge_Response::_accepted_type arg)
  {
    msg_.accepted = std::move(arg);
    return Init_RequestCharge_Response_eta(msg_);
  }

private:
  ::uav_msgs::srv::RequestCharge_Response msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::srv::RequestCharge_Response>()
{
  return uav_msgs::srv::builder::Init_RequestCharge_Response_accepted();
}

}  // namespace uav_msgs

#endif  // UAV_MSGS__SRV__DETAIL__REQUEST_CHARGE__BUILDER_HPP_
