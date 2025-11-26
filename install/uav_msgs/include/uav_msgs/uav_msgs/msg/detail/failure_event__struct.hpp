// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from uav_msgs:msg/FailureEvent.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__FAILURE_EVENT__STRUCT_HPP_
#define UAV_MSGS__MSG__DETAIL__FAILURE_EVENT__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__uav_msgs__msg__FailureEvent __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__msg__FailureEvent __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct FailureEvent_
{
  using Type = FailureEvent_<ContainerAllocator>;

  explicit FailureEvent_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : stamp(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
      this->failure_type = 0;
      this->description = "";
    }
  }

  explicit FailureEvent_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : uav_id(_alloc),
    description(_alloc),
    stamp(_alloc, _init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
      this->failure_type = 0;
      this->description = "";
    }
  }

  // field types and members
  using _uav_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _uav_id_type uav_id;
  using _failure_type_type =
    uint8_t;
  _failure_type_type failure_type;
  using _description_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _description_type description;
  using _stamp_type =
    builtin_interfaces::msg::Time_<ContainerAllocator>;
  _stamp_type stamp;

  // setters for named parameter idiom
  Type & set__uav_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->uav_id = _arg;
    return *this;
  }
  Type & set__failure_type(
    const uint8_t & _arg)
  {
    this->failure_type = _arg;
    return *this;
  }
  Type & set__description(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->description = _arg;
    return *this;
  }
  Type & set__stamp(
    const builtin_interfaces::msg::Time_<ContainerAllocator> & _arg)
  {
    this->stamp = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::msg::FailureEvent_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::msg::FailureEvent_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::msg::FailureEvent_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::msg::FailureEvent_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::FailureEvent_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::FailureEvent_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::FailureEvent_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::FailureEvent_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::msg::FailureEvent_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::msg::FailureEvent_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__msg__FailureEvent
    std::shared_ptr<uav_msgs::msg::FailureEvent_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__msg__FailureEvent
    std::shared_ptr<uav_msgs::msg::FailureEvent_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const FailureEvent_ & other) const
  {
    if (this->uav_id != other.uav_id) {
      return false;
    }
    if (this->failure_type != other.failure_type) {
      return false;
    }
    if (this->description != other.description) {
      return false;
    }
    if (this->stamp != other.stamp) {
      return false;
    }
    return true;
  }
  bool operator!=(const FailureEvent_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct FailureEvent_

// alias to use template instance with default allocator
using FailureEvent =
  uav_msgs::msg::FailureEvent_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__FAILURE_EVENT__STRUCT_HPP_
