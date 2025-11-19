// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from uav_msgs:msg/ClusterInfo.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__CLUSTER_INFO__BUILDER_HPP_
#define UAV_MSGS__MSG__DETAIL__CLUSTER_INFO__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "uav_msgs/msg/detail/cluster_info__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace uav_msgs
{

namespace msg
{

namespace builder
{

class Init_ClusterInfo_member_ids
{
public:
  explicit Init_ClusterInfo_member_ids(::uav_msgs::msg::ClusterInfo & msg)
  : msg_(msg)
  {}
  ::uav_msgs::msg::ClusterInfo member_ids(::uav_msgs::msg::ClusterInfo::_member_ids_type arg)
  {
    msg_.member_ids = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::msg::ClusterInfo msg_;
};

class Init_ClusterInfo_ch_id
{
public:
  explicit Init_ClusterInfo_ch_id(::uav_msgs::msg::ClusterInfo & msg)
  : msg_(msg)
  {}
  Init_ClusterInfo_member_ids ch_id(::uav_msgs::msg::ClusterInfo::_ch_id_type arg)
  {
    msg_.ch_id = std::move(arg);
    return Init_ClusterInfo_member_ids(msg_);
  }

private:
  ::uav_msgs::msg::ClusterInfo msg_;
};

class Init_ClusterInfo_cluster_id
{
public:
  Init_ClusterInfo_cluster_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_ClusterInfo_ch_id cluster_id(::uav_msgs::msg::ClusterInfo::_cluster_id_type arg)
  {
    msg_.cluster_id = std::move(arg);
    return Init_ClusterInfo_ch_id(msg_);
  }

private:
  ::uav_msgs::msg::ClusterInfo msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::msg::ClusterInfo>()
{
  return uav_msgs::msg::builder::Init_ClusterInfo_cluster_id();
}

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__CLUSTER_INFO__BUILDER_HPP_
