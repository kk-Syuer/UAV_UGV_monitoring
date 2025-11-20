// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from uav_msgs:msg/TrafficMessage.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__TRAFFIC_MESSAGE__BUILDER_HPP_
#define UAV_MSGS__MSG__DETAIL__TRAFFIC_MESSAGE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "uav_msgs/msg/detail/traffic_message__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace uav_msgs
{

namespace msg
{

namespace builder
{

class Init_TrafficMessage_hop_count
{
public:
  explicit Init_TrafficMessage_hop_count(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  ::uav_msgs::msg::TrafficMessage hop_count(::uav_msgs::msg::TrafficMessage::_hop_count_type arg)
  {
    msg_.hop_count = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_creation_time
{
public:
  explicit Init_TrafficMessage_creation_time(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_hop_count creation_time(::uav_msgs::msg::TrafficMessage::_creation_time_type arg)
  {
    msg_.creation_time = std::move(arg);
    return Init_TrafficMessage_hop_count(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_size_bytes
{
public:
  explicit Init_TrafficMessage_size_bytes(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_creation_time size_bytes(::uav_msgs::msg::TrafficMessage::_size_bytes_type arg)
  {
    msg_.size_bytes = std::move(arg);
    return Init_TrafficMessage_creation_time(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_priority
{
public:
  explicit Init_TrafficMessage_priority(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_size_bytes priority(::uav_msgs::msg::TrafficMessage::_priority_type arg)
  {
    msg_.priority = std::move(arg);
    return Init_TrafficMessage_size_bytes(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_msg_type
{
public:
  explicit Init_TrafficMessage_msg_type(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_priority msg_type(::uav_msgs::msg::TrafficMessage::_msg_type_type arg)
  {
    msg_.msg_type = std::move(arg);
    return Init_TrafficMessage_priority(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_next_hop_id
{
public:
  explicit Init_TrafficMessage_next_hop_id(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_msg_type next_hop_id(::uav_msgs::msg::TrafficMessage::_next_hop_id_type arg)
  {
    msg_.next_hop_id = std::move(arg);
    return Init_TrafficMessage_msg_type(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_dst_id
{
public:
  explicit Init_TrafficMessage_dst_id(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_next_hop_id dst_id(::uav_msgs::msg::TrafficMessage::_dst_id_type arg)
  {
    msg_.dst_id = std::move(arg);
    return Init_TrafficMessage_next_hop_id(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_src_id
{
public:
  explicit Init_TrafficMessage_src_id(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_dst_id src_id(::uav_msgs::msg::TrafficMessage::_src_id_type arg)
  {
    msg_.src_id = std::move(arg);
    return Init_TrafficMessage_dst_id(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_msg_id
{
public:
  Init_TrafficMessage_msg_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_TrafficMessage_src_id msg_id(::uav_msgs::msg::TrafficMessage::_msg_id_type arg)
  {
    msg_.msg_id = std::move(arg);
    return Init_TrafficMessage_src_id(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::uav_msgs::msg::TrafficMessage>()
{
  return uav_msgs::msg::builder::Init_TrafficMessage_msg_id();
}

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__TRAFFIC_MESSAGE__BUILDER_HPP_
