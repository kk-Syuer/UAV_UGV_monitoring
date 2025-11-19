// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from uav_msgs:srv/RequestCharge.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__SRV__DETAIL__REQUEST_CHARGE__STRUCT_HPP_
#define UAV_MSGS__SRV__DETAIL__REQUEST_CHARGE__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__uav_msgs__srv__RequestCharge_Request __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__srv__RequestCharge_Request __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace srv
{

// message struct
template<class ContainerAllocator>
struct RequestCharge_Request_
{
  using Type = RequestCharge_Request_<ContainerAllocator>;

  explicit RequestCharge_Request_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
      this->battery_level = 0.0f;
      this->role = 0;
    }
  }

  explicit RequestCharge_Request_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : uav_id(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
      this->battery_level = 0.0f;
      this->role = 0;
    }
  }

  // field types and members
  using _uav_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _uav_id_type uav_id;
  using _battery_level_type =
    float;
  _battery_level_type battery_level;
  using _role_type =
    uint8_t;
  _role_type role;

  // setters for named parameter idiom
  Type & set__uav_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->uav_id = _arg;
    return *this;
  }
  Type & set__battery_level(
    const float & _arg)
  {
    this->battery_level = _arg;
    return *this;
  }
  Type & set__role(
    const uint8_t & _arg)
  {
    this->role = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::srv::RequestCharge_Request_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::srv::RequestCharge_Request_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::srv::RequestCharge_Request_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::srv::RequestCharge_Request_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::srv::RequestCharge_Request_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::srv::RequestCharge_Request_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::srv::RequestCharge_Request_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::srv::RequestCharge_Request_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::srv::RequestCharge_Request_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::srv::RequestCharge_Request_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__srv__RequestCharge_Request
    std::shared_ptr<uav_msgs::srv::RequestCharge_Request_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__srv__RequestCharge_Request
    std::shared_ptr<uav_msgs::srv::RequestCharge_Request_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const RequestCharge_Request_ & other) const
  {
    if (this->uav_id != other.uav_id) {
      return false;
    }
    if (this->battery_level != other.battery_level) {
      return false;
    }
    if (this->role != other.role) {
      return false;
    }
    return true;
  }
  bool operator!=(const RequestCharge_Request_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct RequestCharge_Request_

// alias to use template instance with default allocator
using RequestCharge_Request =
  uav_msgs::srv::RequestCharge_Request_<std::allocator<void>>;

// constant definitions

}  // namespace srv

}  // namespace uav_msgs


// Include directives for member types
// Member 'eta'
#include "builtin_interfaces/msg/detail/time__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__uav_msgs__srv__RequestCharge_Response __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__srv__RequestCharge_Response __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace srv
{

// message struct
template<class ContainerAllocator>
struct RequestCharge_Response_
{
  using Type = RequestCharge_Response_<ContainerAllocator>;

  explicit RequestCharge_Response_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : eta(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->accepted = false;
      this->assigned_priority = 0;
    }
  }

  explicit RequestCharge_Response_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : eta(_alloc, _init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->accepted = false;
      this->assigned_priority = 0;
    }
  }

  // field types and members
  using _accepted_type =
    bool;
  _accepted_type accepted;
  using _eta_type =
    builtin_interfaces::msg::Time_<ContainerAllocator>;
  _eta_type eta;
  using _assigned_priority_type =
    uint8_t;
  _assigned_priority_type assigned_priority;

  // setters for named parameter idiom
  Type & set__accepted(
    const bool & _arg)
  {
    this->accepted = _arg;
    return *this;
  }
  Type & set__eta(
    const builtin_interfaces::msg::Time_<ContainerAllocator> & _arg)
  {
    this->eta = _arg;
    return *this;
  }
  Type & set__assigned_priority(
    const uint8_t & _arg)
  {
    this->assigned_priority = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::srv::RequestCharge_Response_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::srv::RequestCharge_Response_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::srv::RequestCharge_Response_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::srv::RequestCharge_Response_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::srv::RequestCharge_Response_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::srv::RequestCharge_Response_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::srv::RequestCharge_Response_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::srv::RequestCharge_Response_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::srv::RequestCharge_Response_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::srv::RequestCharge_Response_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__srv__RequestCharge_Response
    std::shared_ptr<uav_msgs::srv::RequestCharge_Response_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__srv__RequestCharge_Response
    std::shared_ptr<uav_msgs::srv::RequestCharge_Response_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const RequestCharge_Response_ & other) const
  {
    if (this->accepted != other.accepted) {
      return false;
    }
    if (this->eta != other.eta) {
      return false;
    }
    if (this->assigned_priority != other.assigned_priority) {
      return false;
    }
    return true;
  }
  bool operator!=(const RequestCharge_Response_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct RequestCharge_Response_

// alias to use template instance with default allocator
using RequestCharge_Response =
  uav_msgs::srv::RequestCharge_Response_<std::allocator<void>>;

// constant definitions

}  // namespace srv

}  // namespace uav_msgs

namespace uav_msgs
{

namespace srv
{

struct RequestCharge
{
  using Request = uav_msgs::srv::RequestCharge_Request;
  using Response = uav_msgs::srv::RequestCharge_Response;
};

}  // namespace srv

}  // namespace uav_msgs

#endif  // UAV_MSGS__SRV__DETAIL__REQUEST_CHARGE__STRUCT_HPP_
