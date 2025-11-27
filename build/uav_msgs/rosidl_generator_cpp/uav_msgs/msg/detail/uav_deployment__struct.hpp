// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from uav_msgs:msg/UavDeployment.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__UAV_DEPLOYMENT__STRUCT_HPP_
#define UAV_MSGS__MSG__DETAIL__UAV_DEPLOYMENT__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'target_pose'
#include "geometry_msgs/msg/detail/pose__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__uav_msgs__msg__UavDeployment __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__msg__UavDeployment __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct UavDeployment_
{
  using Type = UavDeployment_<ContainerAllocator>;

  explicit UavDeployment_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : target_pose(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
      this->role = 0;
      this->cluster_id = "";
      this->ch_id = "";
      this->next_hop_to_sink = "";
    }
  }

  explicit UavDeployment_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : uav_id(_alloc),
    target_pose(_alloc, _init),
    cluster_id(_alloc),
    ch_id(_alloc),
    next_hop_to_sink(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
      this->role = 0;
      this->cluster_id = "";
      this->ch_id = "";
      this->next_hop_to_sink = "";
    }
  }

  // field types and members
  using _uav_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _uav_id_type uav_id;
  using _target_pose_type =
    geometry_msgs::msg::Pose_<ContainerAllocator>;
  _target_pose_type target_pose;
  using _role_type =
    uint8_t;
  _role_type role;
  using _cluster_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _cluster_id_type cluster_id;
  using _ch_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _ch_id_type ch_id;
  using _next_hop_to_sink_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _next_hop_to_sink_type next_hop_to_sink;

  // setters for named parameter idiom
  Type & set__uav_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->uav_id = _arg;
    return *this;
  }
  Type & set__target_pose(
    const geometry_msgs::msg::Pose_<ContainerAllocator> & _arg)
  {
    this->target_pose = _arg;
    return *this;
  }
  Type & set__role(
    const uint8_t & _arg)
  {
    this->role = _arg;
    return *this;
  }
  Type & set__cluster_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->cluster_id = _arg;
    return *this;
  }
  Type & set__ch_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->ch_id = _arg;
    return *this;
  }
  Type & set__next_hop_to_sink(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->next_hop_to_sink = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::msg::UavDeployment_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::msg::UavDeployment_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::msg::UavDeployment_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::msg::UavDeployment_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::UavDeployment_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::UavDeployment_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::UavDeployment_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::UavDeployment_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::msg::UavDeployment_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::msg::UavDeployment_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__msg__UavDeployment
    std::shared_ptr<uav_msgs::msg::UavDeployment_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__msg__UavDeployment
    std::shared_ptr<uav_msgs::msg::UavDeployment_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const UavDeployment_ & other) const
  {
    if (this->uav_id != other.uav_id) {
      return false;
    }
    if (this->target_pose != other.target_pose) {
      return false;
    }
    if (this->role != other.role) {
      return false;
    }
    if (this->cluster_id != other.cluster_id) {
      return false;
    }
    if (this->ch_id != other.ch_id) {
      return false;
    }
    if (this->next_hop_to_sink != other.next_hop_to_sink) {
      return false;
    }
    return true;
  }
  bool operator!=(const UavDeployment_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct UavDeployment_

// alias to use template instance with default allocator
using UavDeployment =
  uav_msgs::msg::UavDeployment_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__UAV_DEPLOYMENT__STRUCT_HPP_
