// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from uav_msgs:msg/ChargeRequest.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__CHARGE_REQUEST__STRUCT_HPP_
#define UAV_MSGS__MSG__DETAIL__CHARGE_REQUEST__STRUCT_HPP_

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
# define DEPRECATED__uav_msgs__msg__ChargeRequest __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__msg__ChargeRequest __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct ChargeRequest_
{
  using Type = ChargeRequest_<ContainerAllocator>;

  explicit ChargeRequest_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : stamp(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
      this->role = 0;
      this->battery_level = 0.0f;
    }
  }

  explicit ChargeRequest_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : uav_id(_alloc),
    stamp(_alloc, _init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
      this->role = 0;
      this->battery_level = 0.0f;
    }
  }

  // field types and members
  using _uav_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _uav_id_type uav_id;
  using _role_type =
    uint8_t;
  _role_type role;
  using _battery_level_type =
    float;
  _battery_level_type battery_level;
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
  Type & set__role(
    const uint8_t & _arg)
  {
    this->role = _arg;
    return *this;
  }
  Type & set__battery_level(
    const float & _arg)
  {
    this->battery_level = _arg;
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
    uav_msgs::msg::ChargeRequest_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::msg::ChargeRequest_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::msg::ChargeRequest_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::msg::ChargeRequest_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::ChargeRequest_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::ChargeRequest_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::ChargeRequest_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::ChargeRequest_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::msg::ChargeRequest_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::msg::ChargeRequest_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__msg__ChargeRequest
    std::shared_ptr<uav_msgs::msg::ChargeRequest_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__msg__ChargeRequest
    std::shared_ptr<uav_msgs::msg::ChargeRequest_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const ChargeRequest_ & other) const
  {
    if (this->uav_id != other.uav_id) {
      return false;
    }
    if (this->role != other.role) {
      return false;
    }
    if (this->battery_level != other.battery_level) {
      return false;
    }
    if (this->stamp != other.stamp) {
      return false;
    }
    return true;
  }
  bool operator!=(const ChargeRequest_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct ChargeRequest_

// alias to use template instance with default allocator
using ChargeRequest =
  uav_msgs::msg::ChargeRequest_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__CHARGE_REQUEST__STRUCT_HPP_
