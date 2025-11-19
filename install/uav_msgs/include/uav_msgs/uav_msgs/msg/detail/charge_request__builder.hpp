// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from uav_msgs:msg/ChargeRequest.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__CHARGE_REQUEST__BUILDER_HPP_
#define UAV_MSGS__MSG__DETAIL__CHARGE_REQUEST__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "uav_msgs/msg/detail/charge_request__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace uav_msgs
{

namespace msg
{

namespace builder
{

class Init_ChargeRequest_stamp
{
public:
  explicit Init_ChargeRequest_stamp(::uav_msgs::msg::ChargeRequest & msg)
  : msg_(msg)
  {}
  ::uav_msgs::msg::ChargeRequest stamp(::uav_msgs::msg::ChargeRequest::_stamp_type arg)
  {
    msg_.stamp = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::msg::ChargeRequest msg_;
};

class Init_ChargeRequest_battery_level
{
public:
  explicit Init_ChargeRequest_battery_level(::uav_msgs::msg::ChargeRequest & msg)
  : msg_(msg)
  {}
  Init_ChargeRequest_stamp battery_level(::uav_msgs::msg::ChargeRequest::_battery_level_type arg)
  {
    msg_.battery_level = std::move(arg);
    return Init_ChargeRequest_stamp(msg_);
  }

private:
  ::uav_msgs::msg::ChargeRequest msg_;
};

class Init_ChargeRequest_role
{
public:
  explicit Init_ChargeRequest_role(::uav_msgs::msg::ChargeRequest & msg)
  : msg_(msg)
  {}
  Init_ChargeRequest_battery_level role(::uav_msgs::msg::ChargeRequest::_role_type arg)
  {
    msg_.role = std::move(arg);
    return Init_ChargeRequest_battery_level(msg_);
  }

private:
  ::uav_msgs::msg::ChargeRequest msg_;
};

class Init_ChargeRequest_uav_id
{
public:
  Init_ChargeRequest_uav_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_ChargeRequest_role uav_id(::uav_msgs::msg::ChargeRequest::_uav_id_type arg)
  {
    msg_.uav_id = std::move(arg);
    return Init_ChargeRequest_role(msg_);
  }

private:
  ::uav_msgs::msg::ChargeRequest msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::msg::ChargeRequest>()
{
  return uav_msgs::msg::builder::Init_ChargeRequest_uav_id();
}

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__CHARGE_REQUEST__BUILDER_HPP_
