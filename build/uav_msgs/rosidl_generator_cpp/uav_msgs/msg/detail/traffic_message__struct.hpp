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
  : creation_time(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->msg_id = "";
      this->src_id = "";
      this->dst_id = "";
      this->msg_type = 0;
      this->priority = 0;
      this->size_bytes = 0ul;
      this->hop_count = 0ul;
    }
  }

  explicit TrafficMessage_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : msg_id(_alloc),
    src_id(_alloc),
    dst_id(_alloc),
    creation_time(_alloc, _init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->msg_id = "";
      this->src_id = "";
      this->dst_id = "";
      this->msg_type = 0;
      this->priority = 0;
      this->size_bytes = 0ul;
      this->hop_count = 0ul;
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
  using _msg_type_type =
    uint8_t;
  _msg_type_type msg_type;
  using _priority_type =
    uint8_t;
  _priority_type priority;
  using _size_bytes_type =
    uint32_t;
  _size_bytes_type size_bytes;
  using _creation_time_type =
    builtin_interfaces::msg::Time_<ContainerAllocator>;
  _creation_time_type creation_time;
  using _hop_count_type =
    uint32_t;
  _hop_count_type hop_count;

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
  Type & set__msg_type(
    const uint8_t & _arg)
  {
    this->msg_type = _arg;
    return *this;
  }
  Type & set__priority(
    const uint8_t & _arg)
  {
    this->priority = _arg;
    return *this;
  }
  Type & set__size_bytes(
    const uint32_t & _arg)
  {
    this->size_bytes = _arg;
    return *this;
  }
  Type & set__creation_time(
    const builtin_interfaces::msg::Time_<ContainerAllocator> & _arg)
  {
    this->creation_time = _arg;
    return *this;
  }
  Type & set__hop_count(
    const uint32_t & _arg)
  {
    this->hop_count = _arg;
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
    if (this->msg_type != other.msg_type) {
      return false;
    }
    if (this->priority != other.priority) {
      return false;
    }
    if (this->size_bytes != other.size_bytes) {
      return false;
    }
    if (this->creation_time != other.creation_time) {
      return false;
    }
    if (this->hop_count != other.hop_count) {
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
