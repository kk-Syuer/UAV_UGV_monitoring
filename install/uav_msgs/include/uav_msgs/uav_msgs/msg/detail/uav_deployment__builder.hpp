// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from uav_msgs:msg/UavDeployment.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__UAV_DEPLOYMENT__BUILDER_HPP_
#define UAV_MSGS__MSG__DETAIL__UAV_DEPLOYMENT__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "uav_msgs/msg/detail/uav_deployment__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace uav_msgs
{

namespace msg
{

namespace builder
{

class Init_UavDeployment_next_hop_to_ugv
{
public:
  explicit Init_UavDeployment_next_hop_to_ugv(::uav_msgs::msg::UavDeployment & msg)
  : msg_(msg)
  {}
  ::uav_msgs::msg::UavDeployment next_hop_to_ugv(::uav_msgs::msg::UavDeployment::_next_hop_to_ugv_type arg)
  {
    msg_.next_hop_to_ugv = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::msg::UavDeployment msg_;
};

class Init_UavDeployment_next_hop_to_sink
{
public:
  explicit Init_UavDeployment_next_hop_to_sink(::uav_msgs::msg::UavDeployment & msg)
  : msg_(msg)
  {}
  Init_UavDeployment_next_hop_to_ugv next_hop_to_sink(::uav_msgs::msg::UavDeployment::_next_hop_to_sink_type arg)
  {
    msg_.next_hop_to_sink = std::move(arg);
    return Init_UavDeployment_next_hop_to_ugv(msg_);
  }

private:
  ::uav_msgs::msg::UavDeployment msg_;
};

class Init_UavDeployment_target_pose
{
public:
  explicit Init_UavDeployment_target_pose(::uav_msgs::msg::UavDeployment & msg)
  : msg_(msg)
  {}
  Init_UavDeployment_next_hop_to_sink target_pose(::uav_msgs::msg::UavDeployment::_target_pose_type arg)
  {
    msg_.target_pose = std::move(arg);
    return Init_UavDeployment_next_hop_to_sink(msg_);
  }

private:
  ::uav_msgs::msg::UavDeployment msg_;
};

class Init_UavDeployment_ch_id
{
public:
  explicit Init_UavDeployment_ch_id(::uav_msgs::msg::UavDeployment & msg)
  : msg_(msg)
  {}
  Init_UavDeployment_target_pose ch_id(::uav_msgs::msg::UavDeployment::_ch_id_type arg)
  {
    msg_.ch_id = std::move(arg);
    return Init_UavDeployment_target_pose(msg_);
  }

private:
  ::uav_msgs::msg::UavDeployment msg_;
};

class Init_UavDeployment_cluster_id
{
public:
  explicit Init_UavDeployment_cluster_id(::uav_msgs::msg::UavDeployment & msg)
  : msg_(msg)
  {}
  Init_UavDeployment_ch_id cluster_id(::uav_msgs::msg::UavDeployment::_cluster_id_type arg)
  {
    msg_.cluster_id = std::move(arg);
    return Init_UavDeployment_ch_id(msg_);
  }

private:
  ::uav_msgs::msg::UavDeployment msg_;
};

class Init_UavDeployment_role
{
public:
  explicit Init_UavDeployment_role(::uav_msgs::msg::UavDeployment & msg)
  : msg_(msg)
  {}
  Init_UavDeployment_cluster_id role(::uav_msgs::msg::UavDeployment::_role_type arg)
  {
    msg_.role = std::move(arg);
    return Init_UavDeployment_cluster_id(msg_);
  }

private:
  ::uav_msgs::msg::UavDeployment msg_;
};

class Init_UavDeployment_uav_id
{
public:
  Init_UavDeployment_uav_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_UavDeployment_role uav_id(::uav_msgs::msg::UavDeployment::_uav_id_type arg)
  {
    msg_.uav_id = std::move(arg);
    return Init_UavDeployment_role(msg_);
  }

private:
  ::uav_msgs::msg::UavDeployment msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::msg::UavDeployment>()
{
  return uav_msgs::msg::builder::Init_UavDeployment_uav_id();
}

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__UAV_DEPLOYMENT__BUILDER_HPP_
