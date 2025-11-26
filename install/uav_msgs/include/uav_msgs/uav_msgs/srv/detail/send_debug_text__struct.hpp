// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from uav_msgs:srv/SendDebugText.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__SRV__DETAIL__SEND_DEBUG_TEXT__STRUCT_HPP_
#define UAV_MSGS__SRV__DETAIL__SEND_DEBUG_TEXT__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__uav_msgs__srv__SendDebugText_Request __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__srv__SendDebugText_Request __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace srv
{

// message struct
template<class ContainerAllocator>
struct SendDebugText_Request_
{
  using Type = SendDebugText_Request_<ContainerAllocator>;

  explicit SendDebugText_Request_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->dst_id = "";
      this->text = "";
    }
  }

  explicit SendDebugText_Request_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : dst_id(_alloc),
    text(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->dst_id = "";
      this->text = "";
    }
  }

  // field types and members
  using _dst_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _dst_id_type dst_id;
  using _text_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _text_type text;

  // setters for named parameter idiom
  Type & set__dst_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->dst_id = _arg;
    return *this;
  }
  Type & set__text(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->text = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::srv::SendDebugText_Request_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::srv::SendDebugText_Request_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::srv::SendDebugText_Request_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::srv::SendDebugText_Request_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::srv::SendDebugText_Request_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::srv::SendDebugText_Request_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::srv::SendDebugText_Request_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::srv::SendDebugText_Request_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::srv::SendDebugText_Request_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::srv::SendDebugText_Request_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__srv__SendDebugText_Request
    std::shared_ptr<uav_msgs::srv::SendDebugText_Request_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__srv__SendDebugText_Request
    std::shared_ptr<uav_msgs::srv::SendDebugText_Request_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const SendDebugText_Request_ & other) const
  {
    if (this->dst_id != other.dst_id) {
      return false;
    }
    if (this->text != other.text) {
      return false;
    }
    return true;
  }
  bool operator!=(const SendDebugText_Request_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct SendDebugText_Request_

// alias to use template instance with default allocator
using SendDebugText_Request =
  uav_msgs::srv::SendDebugText_Request_<std::allocator<void>>;

// constant definitions

}  // namespace srv

}  // namespace uav_msgs


#ifndef _WIN32
# define DEPRECATED__uav_msgs__srv__SendDebugText_Response __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__srv__SendDebugText_Response __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace srv
{

// message struct
template<class ContainerAllocator>
struct SendDebugText_Response_
{
  using Type = SendDebugText_Response_<ContainerAllocator>;

  explicit SendDebugText_Response_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->accepted = false;
      this->info = "";
    }
  }

  explicit SendDebugText_Response_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : info(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->accepted = false;
      this->info = "";
    }
  }

  // field types and members
  using _accepted_type =
    bool;
  _accepted_type accepted;
  using _info_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _info_type info;

  // setters for named parameter idiom
  Type & set__accepted(
    const bool & _arg)
  {
    this->accepted = _arg;
    return *this;
  }
  Type & set__info(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->info = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::srv::SendDebugText_Response_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::srv::SendDebugText_Response_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::srv::SendDebugText_Response_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::srv::SendDebugText_Response_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::srv::SendDebugText_Response_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::srv::SendDebugText_Response_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::srv::SendDebugText_Response_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::srv::SendDebugText_Response_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::srv::SendDebugText_Response_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::srv::SendDebugText_Response_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__srv__SendDebugText_Response
    std::shared_ptr<uav_msgs::srv::SendDebugText_Response_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__srv__SendDebugText_Response
    std::shared_ptr<uav_msgs::srv::SendDebugText_Response_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const SendDebugText_Response_ & other) const
  {
    if (this->accepted != other.accepted) {
      return false;
    }
    if (this->info != other.info) {
      return false;
    }
    return true;
  }
  bool operator!=(const SendDebugText_Response_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct SendDebugText_Response_

// alias to use template instance with default allocator
using SendDebugText_Response =
  uav_msgs::srv::SendDebugText_Response_<std::allocator<void>>;

// constant definitions

}  // namespace srv

}  // namespace uav_msgs

namespace uav_msgs
{

namespace srv
{

struct SendDebugText
{
  using Request = uav_msgs::srv::SendDebugText_Request;
  using Response = uav_msgs::srv::SendDebugText_Response;
};

}  // namespace srv

}  // namespace uav_msgs

#endif  // UAV_MSGS__SRV__DETAIL__SEND_DEBUG_TEXT__STRUCT_HPP_
