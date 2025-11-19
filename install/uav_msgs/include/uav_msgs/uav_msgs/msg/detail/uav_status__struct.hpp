// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from uav_msgs:msg/UavStatus.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__UAV_STATUS__STRUCT_HPP_
#define UAV_MSGS__MSG__DETAIL__UAV_STATUS__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'pose'
#include "geometry_msgs/msg/detail/pose__struct.hpp"
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__uav_msgs__msg__UavStatus __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__msg__UavStatus __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct UavStatus_
{
  using Type = UavStatus_<ContainerAllocator>;

  explicit UavStatus_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : pose(_init),
    stamp(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
      this->role = 0;
      this->cluster_id = "";
      this->battery_level = 0.0f;
      this->battery_capacity = 0.0f;
      this->service_radius = 0.0f;
      this->connected_users = 0ul;
      this->traffic_load = 0.0f;
      this->packet_loss_estimate = 0.0f;
      this->energy_consumption_rate = 0.0f;
    }
  }

  explicit UavStatus_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : uav_id(_alloc),
    cluster_id(_alloc),
    pose(_alloc, _init),
    stamp(_alloc, _init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
      this->role = 0;
      this->cluster_id = "";
      this->battery_level = 0.0f;
      this->battery_capacity = 0.0f;
      this->service_radius = 0.0f;
      this->connected_users = 0ul;
      this->traffic_load = 0.0f;
      this->packet_loss_estimate = 0.0f;
      this->energy_consumption_rate = 0.0f;
    }
  }

  // field types and members
  using _uav_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _uav_id_type uav_id;
  using _role_type =
    uint8_t;
  _role_type role;
  using _cluster_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _cluster_id_type cluster_id;
  using _battery_level_type =
    float;
  _battery_level_type battery_level;
  using _battery_capacity_type =
    float;
  _battery_capacity_type battery_capacity;
  using _pose_type =
    geometry_msgs::msg::Pose_<ContainerAllocator>;
  _pose_type pose;
  using _service_radius_type =
    float;
  _service_radius_type service_radius;
  using _connected_users_type =
    uint32_t;
  _connected_users_type connected_users;
  using _traffic_load_type =
    float;
  _traffic_load_type traffic_load;
  using _packet_loss_estimate_type =
    float;
  _packet_loss_estimate_type packet_loss_estimate;
  using _energy_consumption_rate_type =
    float;
  _energy_consumption_rate_type energy_consumption_rate;
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
  Type & set__cluster_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->cluster_id = _arg;
    return *this;
  }
  Type & set__battery_level(
    const float & _arg)
  {
    this->battery_level = _arg;
    return *this;
  }
  Type & set__battery_capacity(
    const float & _arg)
  {
    this->battery_capacity = _arg;
    return *this;
  }
  Type & set__pose(
    const geometry_msgs::msg::Pose_<ContainerAllocator> & _arg)
  {
    this->pose = _arg;
    return *this;
  }
  Type & set__service_radius(
    const float & _arg)
  {
    this->service_radius = _arg;
    return *this;
  }
  Type & set__connected_users(
    const uint32_t & _arg)
  {
    this->connected_users = _arg;
    return *this;
  }
  Type & set__traffic_load(
    const float & _arg)
  {
    this->traffic_load = _arg;
    return *this;
  }
  Type & set__packet_loss_estimate(
    const float & _arg)
  {
    this->packet_loss_estimate = _arg;
    return *this;
  }
  Type & set__energy_consumption_rate(
    const float & _arg)
  {
    this->energy_consumption_rate = _arg;
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
    uav_msgs::msg::UavStatus_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::msg::UavStatus_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::msg::UavStatus_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::msg::UavStatus_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::UavStatus_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::UavStatus_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::UavStatus_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::UavStatus_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::msg::UavStatus_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::msg::UavStatus_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__msg__UavStatus
    std::shared_ptr<uav_msgs::msg::UavStatus_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__msg__UavStatus
    std::shared_ptr<uav_msgs::msg::UavStatus_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const UavStatus_ & other) const
  {
    if (this->uav_id != other.uav_id) {
      return false;
    }
    if (this->role != other.role) {
      return false;
    }
    if (this->cluster_id != other.cluster_id) {
      return false;
    }
    if (this->battery_level != other.battery_level) {
      return false;
    }
    if (this->battery_capacity != other.battery_capacity) {
      return false;
    }
    if (this->pose != other.pose) {
      return false;
    }
    if (this->service_radius != other.service_radius) {
      return false;
    }
    if (this->connected_users != other.connected_users) {
      return false;
    }
    if (this->traffic_load != other.traffic_load) {
      return false;
    }
    if (this->packet_loss_estimate != other.packet_loss_estimate) {
      return false;
    }
    if (this->energy_consumption_rate != other.energy_consumption_rate) {
      return false;
    }
    if (this->stamp != other.stamp) {
      return false;
    }
    return true;
  }
  bool operator!=(const UavStatus_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct UavStatus_

// alias to use template instance with default allocator
using UavStatus =
  uav_msgs::msg::UavStatus_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__UAV_STATUS__STRUCT_HPP_
