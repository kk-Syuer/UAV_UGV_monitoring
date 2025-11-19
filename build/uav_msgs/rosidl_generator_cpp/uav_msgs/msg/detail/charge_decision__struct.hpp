// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from uav_msgs:msg/ChargeDecision.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__STRUCT_HPP_
#define UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'slot_start_time'
#include "builtin_interfaces/msg/detail/time__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__uav_msgs__msg__ChargeDecision __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__msg__ChargeDecision __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct ChargeDecision_
{
  using Type = ChargeDecision_<ContainerAllocator>;

  explicit ChargeDecision_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : slot_start_time(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
      this->accepted = false;
      this->priority = 0;
      this->policy = "";
    }
  }

  explicit ChargeDecision_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : uav_id(_alloc),
    slot_start_time(_alloc, _init),
    policy(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
      this->accepted = false;
      this->priority = 0;
      this->policy = "";
    }
  }

  // field types and members
  using _uav_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _uav_id_type uav_id;
  using _accepted_type =
    bool;
  _accepted_type accepted;
  using _slot_start_time_type =
    builtin_interfaces::msg::Time_<ContainerAllocator>;
  _slot_start_time_type slot_start_time;
  using _priority_type =
    uint8_t;
  _priority_type priority;
  using _policy_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _policy_type policy;

  // setters for named parameter idiom
  Type & set__uav_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->uav_id = _arg;
    return *this;
  }
  Type & set__accepted(
    const bool & _arg)
  {
    this->accepted = _arg;
    return *this;
  }
  Type & set__slot_start_time(
    const builtin_interfaces::msg::Time_<ContainerAllocator> & _arg)
  {
    this->slot_start_time = _arg;
    return *this;
  }
  Type & set__priority(
    const uint8_t & _arg)
  {
    this->priority = _arg;
    return *this;
  }
  Type & set__policy(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->policy = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::msg::ChargeDecision_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::msg::ChargeDecision_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::msg::ChargeDecision_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::msg::ChargeDecision_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::ChargeDecision_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::ChargeDecision_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::ChargeDecision_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::ChargeDecision_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::msg::ChargeDecision_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::msg::ChargeDecision_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__msg__ChargeDecision
    std::shared_ptr<uav_msgs::msg::ChargeDecision_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__msg__ChargeDecision
    std::shared_ptr<uav_msgs::msg::ChargeDecision_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const ChargeDecision_ & other) const
  {
    if (this->uav_id != other.uav_id) {
      return false;
    }
    if (this->accepted != other.accepted) {
      return false;
    }
    if (this->slot_start_time != other.slot_start_time) {
      return false;
    }
    if (this->priority != other.priority) {
      return false;
    }
    if (this->policy != other.policy) {
      return false;
    }
    return true;
  }
  bool operator!=(const ChargeDecision_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct ChargeDecision_

// alias to use template instance with default allocator
using ChargeDecision =
  uav_msgs::msg::ChargeDecision_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__CHARGE_DECISION__STRUCT_HPP_
