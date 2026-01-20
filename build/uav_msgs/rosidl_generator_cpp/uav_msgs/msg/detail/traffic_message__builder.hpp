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

class Init_TrafficMessage_recent_hops
{
public:
  explicit Init_TrafficMessage_recent_hops(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  ::uav_msgs::msg::TrafficMessage recent_hops(::uav_msgs::msg::TrafficMessage::_recent_hops_type arg)
  {
    msg_.recent_hops = std::move(arg);
    return std::move(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_drop_reason
{
public:
  explicit Init_TrafficMessage_drop_reason(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_recent_hops drop_reason(::uav_msgs::msg::TrafficMessage::_drop_reason_type arg)
  {
    msg_.drop_reason = std::move(arg);
    return Init_TrafficMessage_recent_hops(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_last_rx_time
{
public:
  explicit Init_TrafficMessage_last_rx_time(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_drop_reason last_rx_time(::uav_msgs::msg::TrafficMessage::_last_rx_time_type arg)
  {
    msg_.last_rx_time = std::move(arg);
    return Init_TrafficMessage_drop_reason(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_last_tx_time
{
public:
  explicit Init_TrafficMessage_last_tx_time(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_last_rx_time last_tx_time(::uav_msgs::msg::TrafficMessage::_last_tx_time_type arg)
  {
    msg_.last_tx_time = std::move(arg);
    return Init_TrafficMessage_last_rx_time(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_ref_msg_id
{
public:
  explicit Init_TrafficMessage_ref_msg_id(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_last_tx_time ref_msg_id(::uav_msgs::msg::TrafficMessage::_ref_msg_id_type arg)
  {
    msg_.ref_msg_id = std::move(arg);
    return Init_TrafficMessage_last_tx_time(msg_);
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
  Init_TrafficMessage_ref_msg_id creation_time(::uav_msgs::msg::TrafficMessage::_creation_time_type arg)
  {
    msg_.creation_time = std::move(arg);
    return Init_TrafficMessage_ref_msg_id(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_payload
{
public:
  explicit Init_TrafficMessage_payload(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_creation_time payload(::uav_msgs::msg::TrafficMessage::_payload_type arg)
  {
    msg_.payload = std::move(arg);
    return Init_TrafficMessage_creation_time(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_requires_ack
{
public:
  explicit Init_TrafficMessage_requires_ack(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_payload requires_ack(::uav_msgs::msg::TrafficMessage::_requires_ack_type arg)
  {
    msg_.requires_ack = std::move(arg);
    return Init_TrafficMessage_payload(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_ttl
{
public:
  explicit Init_TrafficMessage_ttl(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_requires_ack ttl(::uav_msgs::msg::TrafficMessage::_ttl_type arg)
  {
    msg_.ttl = std::move(arg);
    return Init_TrafficMessage_requires_ack(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_hop_count
{
public:
  explicit Init_TrafficMessage_hop_count(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_ttl hop_count(::uav_msgs::msg::TrafficMessage::_hop_count_type arg)
  {
    msg_.hop_count = std::move(arg);
    return Init_TrafficMessage_ttl(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_seq
{
public:
  explicit Init_TrafficMessage_seq(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_hop_count seq(::uav_msgs::msg::TrafficMessage::_seq_type arg)
  {
    msg_.seq = std::move(arg);
    return Init_TrafficMessage_hop_count(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_control_type
{
public:
  explicit Init_TrafficMessage_control_type(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_seq control_type(::uav_msgs::msg::TrafficMessage::_control_type_type arg)
  {
    msg_.control_type = std::move(arg);
    return Init_TrafficMessage_seq(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_flow_type
{
public:
  explicit Init_TrafficMessage_flow_type(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_control_type flow_type(::uav_msgs::msg::TrafficMessage::_flow_type_type arg)
  {
    msg_.flow_type = std::move(arg);
    return Init_TrafficMessage_control_type(msg_);
  }

private:
  ::uav_msgs::msg::TrafficMessage msg_;
};

class Init_TrafficMessage_last_hop_id
{
public:
  explicit Init_TrafficMessage_last_hop_id(::uav_msgs::msg::TrafficMessage & msg)
  : msg_(msg)
  {}
  Init_TrafficMessage_flow_type last_hop_id(::uav_msgs::msg::TrafficMessage::_last_hop_id_type arg)
  {
    msg_.last_hop_id = std::move(arg);
    return Init_TrafficMessage_flow_type(msg_);
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
  Init_TrafficMessage_last_hop_id next_hop_id(::uav_msgs::msg::TrafficMessage::_next_hop_id_type arg)
  {
    msg_.next_hop_id = std::move(arg);
    return Init_TrafficMessage_last_hop_id(msg_);
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
