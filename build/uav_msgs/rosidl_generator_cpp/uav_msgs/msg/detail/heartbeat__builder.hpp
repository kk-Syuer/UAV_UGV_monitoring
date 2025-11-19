// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from uav_msgs:msg/Heartbeat.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__HEARTBEAT__BUILDER_HPP_
#define UAV_MSGS__MSG__DETAIL__HEARTBEAT__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "uav_msgs/msg/detail/heartbeat__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace uav_msgs
{

namespace msg
{

namespace builder
{

class Init_Heartbeat_stamp
{
public:
  explicit Init_Heartbeat_stamp(::uav_msgs::msg::Heartbeat & msg)
  : msg_(msg)
  {}
  ::uav_msgs::msg::Heartbeat stamp(::uav_msgs::msg::Heartbeat::_stamp_type arg)
  {
    msg_.stamp = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::msg::Heartbeat msg_;
};

class Init_Heartbeat_uav_id
{
public:
  Init_Heartbeat_uav_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_Heartbeat_stamp uav_id(::uav_msgs::msg::Heartbeat::_uav_id_type arg)
  {
    msg_.uav_id = std::move(arg);
    return Init_Heartbeat_stamp(msg_);
  }

private:
  ::uav_msgs::msg::Heartbeat msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::msg::Heartbeat>()
{
  return uav_msgs::msg::builder::Init_Heartbeat_uav_id();
}

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__HEARTBEAT__BUILDER_HPP_
