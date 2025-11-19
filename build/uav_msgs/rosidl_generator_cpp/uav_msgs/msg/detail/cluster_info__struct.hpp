// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from uav_msgs:msg/ClusterInfo.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__CLUSTER_INFO__STRUCT_HPP_
#define UAV_MSGS__MSG__DETAIL__CLUSTER_INFO__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__uav_msgs__msg__ClusterInfo __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__msg__ClusterInfo __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct ClusterInfo_
{
  using Type = ClusterInfo_<ContainerAllocator>;

  explicit ClusterInfo_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->cluster_id = "";
      this->ch_id = "";
    }
  }

  explicit ClusterInfo_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : cluster_id(_alloc),
    ch_id(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->cluster_id = "";
      this->ch_id = "";
    }
  }

  // field types and members
  using _cluster_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _cluster_id_type cluster_id;
  using _ch_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _ch_id_type ch_id;
  using _member_ids_type =
    std::vector<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>>>;
  _member_ids_type member_ids;

  // setters for named parameter idiom
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
  Type & set__member_ids(
    const std::vector<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>>> & _arg)
  {
    this->member_ids = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::msg::ClusterInfo_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::msg::ClusterInfo_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::msg::ClusterInfo_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::msg::ClusterInfo_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::ClusterInfo_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::ClusterInfo_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::ClusterInfo_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::ClusterInfo_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::msg::ClusterInfo_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::msg::ClusterInfo_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__msg__ClusterInfo
    std::shared_ptr<uav_msgs::msg::ClusterInfo_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__msg__ClusterInfo
    std::shared_ptr<uav_msgs::msg::ClusterInfo_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const ClusterInfo_ & other) const
  {
    if (this->cluster_id != other.cluster_id) {
      return false;
    }
    if (this->ch_id != other.ch_id) {
      return false;
    }
    if (this->member_ids != other.member_ids) {
      return false;
    }
    return true;
  }
  bool operator!=(const ClusterInfo_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct ClusterInfo_

// alias to use template instance with default allocator
using ClusterInfo =
  uav_msgs::msg::ClusterInfo_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__CLUSTER_INFO__STRUCT_HPP_
