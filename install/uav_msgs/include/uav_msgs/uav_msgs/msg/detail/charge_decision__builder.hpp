// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from uav_msgs:msg/ChargeDecision.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__BUILDER_HPP_
#define UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "uav_msgs/msg/detail/charge_decision__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace uav_msgs
{

namespace msg
{

namespace builder
{

class Init_ChargeDecision_policy
{
public:
  explicit Init_ChargeDecision_policy(::uav_msgs::msg::ChargeDecision & msg)
  : msg_(msg)
  {}
  ::uav_msgs::msg::ChargeDecision policy(::uav_msgs::msg::ChargeDecision::_policy_type arg)
  {
    msg_.policy = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::msg::ChargeDecision msg_;
};

class Init_ChargeDecision_priority
{
public:
  explicit Init_ChargeDecision_priority(::uav_msgs::msg::ChargeDecision & msg)
  : msg_(msg)
  {}
  Init_ChargeDecision_policy priority(::uav_msgs::msg::ChargeDecision::_priority_type arg)
  {
    msg_.priority = std::move(arg);
    return Init_ChargeDecision_policy(msg_);
  }

private:
  ::uav_msgs::msg::ChargeDecision msg_;
};

class Init_ChargeDecision_slot_start_time
{
public:
  explicit Init_ChargeDecision_slot_start_time(::uav_msgs::msg::ChargeDecision & msg)
  : msg_(msg)
  {}
  Init_ChargeDecision_priority slot_start_time(::uav_msgs::msg::ChargeDecision::_slot_start_time_type arg)
  {
    msg_.slot_start_time = std::move(arg);
    return Init_ChargeDecision_priority(msg_);
  }

private:
  ::uav_msgs::msg::ChargeDecision msg_;
};

class Init_ChargeDecision_accepted
{
public:
  explicit Init_ChargeDecision_accepted(::uav_msgs::msg::ChargeDecision & msg)
  : msg_(msg)
  {}
  Init_ChargeDecision_slot_start_time accepted(::uav_msgs::msg::ChargeDecision::_accepted_type arg)
  {
    msg_.accepted = std::move(arg);
    return Init_ChargeDecision_slot_start_time(msg_);
  }

private:
  ::uav_msgs::msg::ChargeDecision msg_;
};

class Init_ChargeDecision_uav_id
{
public:
  Init_ChargeDecision_uav_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_ChargeDecision_accepted uav_id(::uav_msgs::msg::ChargeDecision::_uav_id_type arg)
  {
    msg_.uav_id = std::move(arg);
    return Init_ChargeDecision_accepted(msg_);
  }

private:
  ::uav_msgs::msg::ChargeDecision msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::msg::ChargeDecision>()
{
  return uav_msgs::msg::builder::Init_ChargeDecision_uav_id();
}

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__BUILDER_HPP_
