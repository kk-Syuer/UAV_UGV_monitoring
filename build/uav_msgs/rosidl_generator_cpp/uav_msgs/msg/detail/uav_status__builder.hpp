// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from uav_msgs:msg/UavStatus.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__UAV_STATUS__BUILDER_HPP_
#define UAV_MSGS__MSG__DETAIL__UAV_STATUS__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "uav_msgs/msg/detail/uav_status__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace uav_msgs
{

namespace msg
{

namespace builder
{

class Init_UavStatus_stamp
{
public:
  explicit Init_UavStatus_stamp(::uav_msgs::msg::UavStatus & msg)
  : msg_(msg)
  {}
  ::uav_msgs::msg::UavStatus stamp(::uav_msgs::msg::UavStatus::_stamp_type arg)
  {
    msg_.stamp = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::msg::UavStatus msg_;
};

class Init_UavStatus_energy_consumption_rate
{
public:
  explicit Init_UavStatus_energy_consumption_rate(::uav_msgs::msg::UavStatus & msg)
  : msg_(msg)
  {}
  Init_UavStatus_stamp energy_consumption_rate(::uav_msgs::msg::UavStatus::_energy_consumption_rate_type arg)
  {
    msg_.energy_consumption_rate = std::move(arg);
    return Init_UavStatus_stamp(msg_);
  }

private:
  ::uav_msgs::msg::UavStatus msg_;
};

class Init_UavStatus_packet_loss_estimate
{
public:
  explicit Init_UavStatus_packet_loss_estimate(::uav_msgs::msg::UavStatus & msg)
  : msg_(msg)
  {}
  Init_UavStatus_energy_consumption_rate packet_loss_estimate(::uav_msgs::msg::UavStatus::_packet_loss_estimate_type arg)
  {
    msg_.packet_loss_estimate = std::move(arg);
    return Init_UavStatus_energy_consumption_rate(msg_);
  }

private:
  ::uav_msgs::msg::UavStatus msg_;
};

class Init_UavStatus_traffic_load
{
public:
  explicit Init_UavStatus_traffic_load(::uav_msgs::msg::UavStatus & msg)
  : msg_(msg)
  {}
  Init_UavStatus_packet_loss_estimate traffic_load(::uav_msgs::msg::UavStatus::_traffic_load_type arg)
  {
    msg_.traffic_load = std::move(arg);
    return Init_UavStatus_packet_loss_estimate(msg_);
  }

private:
  ::uav_msgs::msg::UavStatus msg_;
};

class Init_UavStatus_connected_users
{
public:
  explicit Init_UavStatus_connected_users(::uav_msgs::msg::UavStatus & msg)
  : msg_(msg)
  {}
  Init_UavStatus_traffic_load connected_users(::uav_msgs::msg::UavStatus::_connected_users_type arg)
  {
    msg_.connected_users = std::move(arg);
    return Init_UavStatus_traffic_load(msg_);
  }

private:
  ::uav_msgs::msg::UavStatus msg_;
};

class Init_UavStatus_service_radius
{
public:
  explicit Init_UavStatus_service_radius(::uav_msgs::msg::UavStatus & msg)
  : msg_(msg)
  {}
  Init_UavStatus_connected_users service_radius(::uav_msgs::msg::UavStatus::_service_radius_type arg)
  {
    msg_.service_radius = std::move(arg);
    return Init_UavStatus_connected_users(msg_);
  }

private:
  ::uav_msgs::msg::UavStatus msg_;
};

class Init_UavStatus_pose
{
public:
  explicit Init_UavStatus_pose(::uav_msgs::msg::UavStatus & msg)
  : msg_(msg)
  {}
  Init_UavStatus_service_radius pose(::uav_msgs::msg::UavStatus::_pose_type arg)
  {
    msg_.pose = std::move(arg);
    return Init_UavStatus_service_radius(msg_);
  }

private:
  ::uav_msgs::msg::UavStatus msg_;
};

class Init_UavStatus_battery_capacity
{
public:
  explicit Init_UavStatus_battery_capacity(::uav_msgs::msg::UavStatus & msg)
  : msg_(msg)
  {}
  Init_UavStatus_pose battery_capacity(::uav_msgs::msg::UavStatus::_battery_capacity_type arg)
  {
    msg_.battery_capacity = std::move(arg);
    return Init_UavStatus_pose(msg_);
  }

private:
  ::uav_msgs::msg::UavStatus msg_;
};

class Init_UavStatus_battery_level
{
public:
  explicit Init_UavStatus_battery_level(::uav_msgs::msg::UavStatus & msg)
  : msg_(msg)
  {}
  Init_UavStatus_battery_capacity battery_level(::uav_msgs::msg::UavStatus::_battery_level_type arg)
  {
    msg_.battery_level = std::move(arg);
    return Init_UavStatus_battery_capacity(msg_);
  }

private:
  ::uav_msgs::msg::UavStatus msg_;
};

class Init_UavStatus_cluster_id
{
public:
  explicit Init_UavStatus_cluster_id(::uav_msgs::msg::UavStatus & msg)
  : msg_(msg)
  {}
  Init_UavStatus_battery_level cluster_id(::uav_msgs::msg::UavStatus::_cluster_id_type arg)
  {
    msg_.cluster_id = std::move(arg);
    return Init_UavStatus_battery_level(msg_);
  }

private:
  ::uav_msgs::msg::UavStatus msg_;
};

class Init_UavStatus_role
{
public:
  explicit Init_UavStatus_role(::uav_msgs::msg::UavStatus & msg)
  : msg_(msg)
  {}
  Init_UavStatus_cluster_id role(::uav_msgs::msg::UavStatus::_role_type arg)
  {
    msg_.role = std::move(arg);
    return Init_UavStatus_cluster_id(msg_);
  }

private:
  ::uav_msgs::msg::UavStatus msg_;
};

class Init_UavStatus_uav_id
{
public:
  Init_UavStatus_uav_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_UavStatus_role uav_id(::uav_msgs::msg::UavStatus::_uav_id_type arg)
  {
    msg_.uav_id = std::move(arg);
    return Init_UavStatus_role(msg_);
  }

private:
  ::uav_msgs::msg::UavStatus msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::msg::UavStatus>()
{
  return uav_msgs::msg::builder::Init_UavStatus_uav_id();
}

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__UAV_STATUS__BUILDER_HPP_
