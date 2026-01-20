// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from uav_msgs:msg/TrafficMessage.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__TRAFFIC_MESSAGE__STRUCT_HPP_
#define UAV_MSGS__MSG__DETAIL__TRAFFIC_MESSAGE__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'creation_time'
// Member 'last_tx_time'
// Member 'last_rx_time'
#include "builtin_interfaces/msg/detail/time__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__uav_msgs__msg__TrafficMessage __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__msg__TrafficMessage __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct TrafficMessage_
{
  using Type = TrafficMessage_<ContainerAllocator>;

  explicit TrafficMessage_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : creation_time(_init),
    last_tx_time(_init),
    last_rx_time(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->msg_id = "";
      this->src_id = "";
      this->dst_id = "";
      this->next_hop_id = "";
      this->last_hop_id = "";
      this->flow_type = 0;
      this->control_type = "";
      this->seq = 0ul;
      this->hop_count = 0ul;
      this->ttl = 0ul;
      this->requires_ack = false;
      this->payload = "";
      this->ref_msg_id = "";
      this->drop_reason = "";
    }
  }

  explicit TrafficMessage_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : msg_id(_alloc),
    src_id(_alloc),
    dst_id(_alloc),
    next_hop_id(_alloc),
    last_hop_id(_alloc),
    control_type(_alloc),
    payload(_alloc),
    creation_time(_alloc, _init),
    ref_msg_id(_alloc),
    last_tx_time(_alloc, _init),
    last_rx_time(_alloc, _init),
    drop_reason(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->msg_id = "";
      this->src_id = "";
      this->dst_id = "";
      this->next_hop_id = "";
      this->last_hop_id = "";
      this->flow_type = 0;
      this->control_type = "";
      this->seq = 0ul;
      this->hop_count = 0ul;
      this->ttl = 0ul;
      this->requires_ack = false;
      this->payload = "";
      this->ref_msg_id = "";
      this->drop_reason = "";
    }
  }

  // field types and members
  using _msg_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _msg_id_type msg_id;
  using _src_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _src_id_type src_id;
  using _dst_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _dst_id_type dst_id;
  using _next_hop_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _next_hop_id_type next_hop_id;
  using _last_hop_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _last_hop_id_type last_hop_id;
  using _flow_type_type =
    uint8_t;
  _flow_type_type flow_type;
  using _control_type_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _control_type_type control_type;
  using _seq_type =
    uint32_t;
  _seq_type seq;
  using _hop_count_type =
    uint32_t;
  _hop_count_type hop_count;
  using _ttl_type =
    uint32_t;
  _ttl_type ttl;
  using _requires_ack_type =
    bool;
  _requires_ack_type requires_ack;
  using _payload_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _payload_type payload;
  using _creation_time_type =
    builtin_interfaces::msg::Time_<ContainerAllocator>;
  _creation_time_type creation_time;
  using _ref_msg_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _ref_msg_id_type ref_msg_id;
  using _last_tx_time_type =
    builtin_interfaces::msg::Time_<ContainerAllocator>;
  _last_tx_time_type last_tx_time;
  using _last_rx_time_type =
    builtin_interfaces::msg::Time_<ContainerAllocator>;
  _last_rx_time_type last_rx_time;
  using _drop_reason_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _drop_reason_type drop_reason;
  using _recent_hops_type =
    std::vector<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>>>;
  _recent_hops_type recent_hops;

  // setters for named parameter idiom
  Type & set__msg_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->msg_id = _arg;
    return *this;
  }
  Type & set__src_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->src_id = _arg;
    return *this;
  }
  Type & set__dst_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->dst_id = _arg;
    return *this;
  }
  Type & set__next_hop_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->next_hop_id = _arg;
    return *this;
  }
  Type & set__last_hop_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->last_hop_id = _arg;
    return *this;
  }
  Type & set__flow_type(
    const uint8_t & _arg)
  {
    this->flow_type = _arg;
    return *this;
  }
  Type & set__control_type(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->control_type = _arg;
    return *this;
  }
  Type & set__seq(
    const uint32_t & _arg)
  {
    this->seq = _arg;
    return *this;
  }
  Type & set__hop_count(
    const uint32_t & _arg)
  {
    this->hop_count = _arg;
    return *this;
  }
  Type & set__ttl(
    const uint32_t & _arg)
  {
    this->ttl = _arg;
    return *this;
  }
  Type & set__requires_ack(
    const bool & _arg)
  {
    this->requires_ack = _arg;
    return *this;
  }
  Type & set__payload(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->payload = _arg;
    return *this;
  }
  Type & set__creation_time(
    const builtin_interfaces::msg::Time_<ContainerAllocator> & _arg)
  {
    this->creation_time = _arg;
    return *this;
  }
  Type & set__ref_msg_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->ref_msg_id = _arg;
    return *this;
  }
  Type & set__last_tx_time(
    const builtin_interfaces::msg::Time_<ContainerAllocator> & _arg)
  {
    this->last_tx_time = _arg;
    return *this;
  }
  Type & set__last_rx_time(
    const builtin_interfaces::msg::Time_<ContainerAllocator> & _arg)
  {
    this->last_rx_time = _arg;
    return *this;
  }
  Type & set__drop_reason(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->drop_reason = _arg;
    return *this;
  }
  Type & set__recent_hops(
    const std::vector<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>>> & _arg)
  {
    this->recent_hops = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::msg::TrafficMessage_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::msg::TrafficMessage_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::msg::TrafficMessage_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::msg::TrafficMessage_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::TrafficMessage_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::TrafficMessage_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::TrafficMessage_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::TrafficMessage_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::msg::TrafficMessage_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::msg::TrafficMessage_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__msg__TrafficMessage
    std::shared_ptr<uav_msgs::msg::TrafficMessage_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__msg__TrafficMessage
    std::shared_ptr<uav_msgs::msg::TrafficMessage_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const TrafficMessage_ & other) const
  {
    if (this->msg_id != other.msg_id) {
      return false;
    }
    if (this->src_id != other.src_id) {
      return false;
    }
    if (this->dst_id != other.dst_id) {
      return false;
    }
    if (this->next_hop_id != other.next_hop_id) {
      return false;
    }
    if (this->last_hop_id != other.last_hop_id) {
      return false;
    }
    if (this->flow_type != other.flow_type) {
      return false;
    }
    if (this->control_type != other.control_type) {
      return false;
    }
    if (this->seq != other.seq) {
      return false;
    }
    if (this->hop_count != other.hop_count) {
      return false;
    }
    if (this->ttl != other.ttl) {
      return false;
    }
    if (this->requires_ack != other.requires_ack) {
      return false;
    }
    if (this->payload != other.payload) {
      return false;
    }
    if (this->creation_time != other.creation_time) {
      return false;
    }
    if (this->ref_msg_id != other.ref_msg_id) {
      return false;
    }
    if (this->last_tx_time != other.last_tx_time) {
      return false;
    }
    if (this->last_rx_time != other.last_rx_time) {
      return false;
    }
    if (this->drop_reason != other.drop_reason) {
      return false;
    }
    if (this->recent_hops != other.recent_hops) {
      return false;
    }
    return true;
  }
  bool operator!=(const TrafficMessage_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct TrafficMessage_

// alias to use template instance with default allocator
using TrafficMessage =
  uav_msgs::msg::TrafficMessage_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__TRAFFIC_MESSAGE__STRUCT_HPP_
