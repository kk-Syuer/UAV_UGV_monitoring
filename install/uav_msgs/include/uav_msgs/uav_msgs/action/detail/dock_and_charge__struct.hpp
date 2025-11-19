// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from uav_msgs:action/DockAndCharge.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__ACTION__DETAIL__DOCK_AND_CHARGE__STRUCT_HPP_
#define UAV_MSGS__ACTION__DETAIL__DOCK_AND_CHARGE__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__uav_msgs__action__DockAndCharge_Goal __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__action__DockAndCharge_Goal __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace action
{

// message struct
template<class ContainerAllocator>
struct DockAndCharge_Goal_
{
  using Type = DockAndCharge_Goal_<ContainerAllocator>;

  explicit DockAndCharge_Goal_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
    }
  }

  explicit DockAndCharge_Goal_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : uav_id(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->uav_id = "";
    }
  }

  // field types and members
  using _uav_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _uav_id_type uav_id;

  // setters for named parameter idiom
  Type & set__uav_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->uav_id = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_Goal
    std::shared_ptr<uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_Goal
    std::shared_ptr<uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const DockAndCharge_Goal_ & other) const
  {
    if (this->uav_id != other.uav_id) {
      return false;
    }
    return true;
  }
  bool operator!=(const DockAndCharge_Goal_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct DockAndCharge_Goal_

// alias to use template instance with default allocator
using DockAndCharge_Goal =
  uav_msgs::action::DockAndCharge_Goal_<std::allocator<void>>;

// constant definitions

}  // namespace action

}  // namespace uav_msgs


#ifndef _WIN32
# define DEPRECATED__uav_msgs__action__DockAndCharge_Result __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__action__DockAndCharge_Result __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace action
{

// message struct
template<class ContainerAllocator>
struct DockAndCharge_Result_
{
  using Type = DockAndCharge_Result_<ContainerAllocator>;

  explicit DockAndCharge_Result_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->battery_level = 0.0f;
    }
  }

  explicit DockAndCharge_Result_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->battery_level = 0.0f;
    }
  }

  // field types and members
  using _battery_level_type =
    float;
  _battery_level_type battery_level;

  // setters for named parameter idiom
  Type & set__battery_level(
    const float & _arg)
  {
    this->battery_level = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::action::DockAndCharge_Result_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::action::DockAndCharge_Result_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_Result_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_Result_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_Result_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_Result_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_Result_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_Result_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_Result_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_Result_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_Result
    std::shared_ptr<uav_msgs::action::DockAndCharge_Result_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_Result
    std::shared_ptr<uav_msgs::action::DockAndCharge_Result_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const DockAndCharge_Result_ & other) const
  {
    if (this->battery_level != other.battery_level) {
      return false;
    }
    return true;
  }
  bool operator!=(const DockAndCharge_Result_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct DockAndCharge_Result_

// alias to use template instance with default allocator
using DockAndCharge_Result =
  uav_msgs::action::DockAndCharge_Result_<std::allocator<void>>;

// constant definitions

}  // namespace action

}  // namespace uav_msgs


#ifndef _WIN32
# define DEPRECATED__uav_msgs__action__DockAndCharge_Feedback __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__action__DockAndCharge_Feedback __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace action
{

// message struct
template<class ContainerAllocator>
struct DockAndCharge_Feedback_
{
  using Type = DockAndCharge_Feedback_<ContainerAllocator>;

  explicit DockAndCharge_Feedback_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->current_battery = 0.0f;
    }
  }

  explicit DockAndCharge_Feedback_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->current_battery = 0.0f;
    }
  }

  // field types and members
  using _current_battery_type =
    float;
  _current_battery_type current_battery;

  // setters for named parameter idiom
  Type & set__current_battery(
    const float & _arg)
  {
    this->current_battery = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_Feedback
    std::shared_ptr<uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_Feedback
    std::shared_ptr<uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const DockAndCharge_Feedback_ & other) const
  {
    if (this->current_battery != other.current_battery) {
      return false;
    }
    return true;
  }
  bool operator!=(const DockAndCharge_Feedback_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct DockAndCharge_Feedback_

// alias to use template instance with default allocator
using DockAndCharge_Feedback =
  uav_msgs::action::DockAndCharge_Feedback_<std::allocator<void>>;

// constant definitions

}  // namespace action

}  // namespace uav_msgs


// Include directives for member types
// Member 'goal_id'
#include "unique_identifier_msgs/msg/detail/uuid__struct.hpp"
// Member 'goal'
#include "uav_msgs/action/detail/dock_and_charge__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__uav_msgs__action__DockAndCharge_SendGoal_Request __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__action__DockAndCharge_SendGoal_Request __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace action
{

// message struct
template<class ContainerAllocator>
struct DockAndCharge_SendGoal_Request_
{
  using Type = DockAndCharge_SendGoal_Request_<ContainerAllocator>;

  explicit DockAndCharge_SendGoal_Request_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : goal_id(_init),
    goal(_init)
  {
    (void)_init;
  }

  explicit DockAndCharge_SendGoal_Request_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : goal_id(_alloc, _init),
    goal(_alloc, _init)
  {
    (void)_init;
  }

  // field types and members
  using _goal_id_type =
    unique_identifier_msgs::msg::UUID_<ContainerAllocator>;
  _goal_id_type goal_id;
  using _goal_type =
    uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator>;
  _goal_type goal;

  // setters for named parameter idiom
  Type & set__goal_id(
    const unique_identifier_msgs::msg::UUID_<ContainerAllocator> & _arg)
  {
    this->goal_id = _arg;
    return *this;
  }
  Type & set__goal(
    const uav_msgs::action::DockAndCharge_Goal_<ContainerAllocator> & _arg)
  {
    this->goal = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::action::DockAndCharge_SendGoal_Request_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::action::DockAndCharge_SendGoal_Request_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_SendGoal_Request_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_SendGoal_Request_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_SendGoal_Request_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_SendGoal_Request_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_SendGoal_Request_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_SendGoal_Request_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_SendGoal_Request_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_SendGoal_Request_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_SendGoal_Request
    std::shared_ptr<uav_msgs::action::DockAndCharge_SendGoal_Request_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_SendGoal_Request
    std::shared_ptr<uav_msgs::action::DockAndCharge_SendGoal_Request_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const DockAndCharge_SendGoal_Request_ & other) const
  {
    if (this->goal_id != other.goal_id) {
      return false;
    }
    if (this->goal != other.goal) {
      return false;
    }
    return true;
  }
  bool operator!=(const DockAndCharge_SendGoal_Request_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct DockAndCharge_SendGoal_Request_

// alias to use template instance with default allocator
using DockAndCharge_SendGoal_Request =
  uav_msgs::action::DockAndCharge_SendGoal_Request_<std::allocator<void>>;

// constant definitions

}  // namespace action

}  // namespace uav_msgs


// Include directives for member types
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__uav_msgs__action__DockAndCharge_SendGoal_Response __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__action__DockAndCharge_SendGoal_Response __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace action
{

// message struct
template<class ContainerAllocator>
struct DockAndCharge_SendGoal_Response_
{
  using Type = DockAndCharge_SendGoal_Response_<ContainerAllocator>;

  explicit DockAndCharge_SendGoal_Response_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : stamp(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->accepted = false;
    }
  }

  explicit DockAndCharge_SendGoal_Response_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : stamp(_alloc, _init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->accepted = false;
    }
  }

  // field types and members
  using _accepted_type =
    bool;
  _accepted_type accepted;
  using _stamp_type =
    builtin_interfaces::msg::Time_<ContainerAllocator>;
  _stamp_type stamp;

  // setters for named parameter idiom
  Type & set__accepted(
    const bool & _arg)
  {
    this->accepted = _arg;
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
    uav_msgs::action::DockAndCharge_SendGoal_Response_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::action::DockAndCharge_SendGoal_Response_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_SendGoal_Response_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_SendGoal_Response_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_SendGoal_Response_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_SendGoal_Response_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_SendGoal_Response_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_SendGoal_Response_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_SendGoal_Response_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_SendGoal_Response_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_SendGoal_Response
    std::shared_ptr<uav_msgs::action::DockAndCharge_SendGoal_Response_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_SendGoal_Response
    std::shared_ptr<uav_msgs::action::DockAndCharge_SendGoal_Response_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const DockAndCharge_SendGoal_Response_ & other) const
  {
    if (this->accepted != other.accepted) {
      return false;
    }
    if (this->stamp != other.stamp) {
      return false;
    }
    return true;
  }
  bool operator!=(const DockAndCharge_SendGoal_Response_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct DockAndCharge_SendGoal_Response_

// alias to use template instance with default allocator
using DockAndCharge_SendGoal_Response =
  uav_msgs::action::DockAndCharge_SendGoal_Response_<std::allocator<void>>;

// constant definitions

}  // namespace action

}  // namespace uav_msgs

namespace uav_msgs
{

namespace action
{

struct DockAndCharge_SendGoal
{
  using Request = uav_msgs::action::DockAndCharge_SendGoal_Request;
  using Response = uav_msgs::action::DockAndCharge_SendGoal_Response;
};

}  // namespace action

}  // namespace uav_msgs


// Include directives for member types
// Member 'goal_id'
// already included above
// #include "unique_identifier_msgs/msg/detail/uuid__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__uav_msgs__action__DockAndCharge_GetResult_Request __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__action__DockAndCharge_GetResult_Request __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace action
{

// message struct
template<class ContainerAllocator>
struct DockAndCharge_GetResult_Request_
{
  using Type = DockAndCharge_GetResult_Request_<ContainerAllocator>;

  explicit DockAndCharge_GetResult_Request_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : goal_id(_init)
  {
    (void)_init;
  }

  explicit DockAndCharge_GetResult_Request_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : goal_id(_alloc, _init)
  {
    (void)_init;
  }

  // field types and members
  using _goal_id_type =
    unique_identifier_msgs::msg::UUID_<ContainerAllocator>;
  _goal_id_type goal_id;

  // setters for named parameter idiom
  Type & set__goal_id(
    const unique_identifier_msgs::msg::UUID_<ContainerAllocator> & _arg)
  {
    this->goal_id = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::action::DockAndCharge_GetResult_Request_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::action::DockAndCharge_GetResult_Request_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_GetResult_Request_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_GetResult_Request_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_GetResult_Request_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_GetResult_Request_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_GetResult_Request_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_GetResult_Request_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_GetResult_Request_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_GetResult_Request_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_GetResult_Request
    std::shared_ptr<uav_msgs::action::DockAndCharge_GetResult_Request_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_GetResult_Request
    std::shared_ptr<uav_msgs::action::DockAndCharge_GetResult_Request_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const DockAndCharge_GetResult_Request_ & other) const
  {
    if (this->goal_id != other.goal_id) {
      return false;
    }
    return true;
  }
  bool operator!=(const DockAndCharge_GetResult_Request_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct DockAndCharge_GetResult_Request_

// alias to use template instance with default allocator
using DockAndCharge_GetResult_Request =
  uav_msgs::action::DockAndCharge_GetResult_Request_<std::allocator<void>>;

// constant definitions

}  // namespace action

}  // namespace uav_msgs


// Include directives for member types
// Member 'result'
// already included above
// #include "uav_msgs/action/detail/dock_and_charge__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__uav_msgs__action__DockAndCharge_GetResult_Response __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__action__DockAndCharge_GetResult_Response __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace action
{

// message struct
template<class ContainerAllocator>
struct DockAndCharge_GetResult_Response_
{
  using Type = DockAndCharge_GetResult_Response_<ContainerAllocator>;

  explicit DockAndCharge_GetResult_Response_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : result(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->status = 0;
    }
  }

  explicit DockAndCharge_GetResult_Response_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : result(_alloc, _init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->status = 0;
    }
  }

  // field types and members
  using _status_type =
    int8_t;
  _status_type status;
  using _result_type =
    uav_msgs::action::DockAndCharge_Result_<ContainerAllocator>;
  _result_type result;

  // setters for named parameter idiom
  Type & set__status(
    const int8_t & _arg)
  {
    this->status = _arg;
    return *this;
  }
  Type & set__result(
    const uav_msgs::action::DockAndCharge_Result_<ContainerAllocator> & _arg)
  {
    this->result = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::action::DockAndCharge_GetResult_Response_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::action::DockAndCharge_GetResult_Response_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_GetResult_Response_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_GetResult_Response_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_GetResult_Response_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_GetResult_Response_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_GetResult_Response_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_GetResult_Response_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_GetResult_Response_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_GetResult_Response_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_GetResult_Response
    std::shared_ptr<uav_msgs::action::DockAndCharge_GetResult_Response_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_GetResult_Response
    std::shared_ptr<uav_msgs::action::DockAndCharge_GetResult_Response_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const DockAndCharge_GetResult_Response_ & other) const
  {
    if (this->status != other.status) {
      return false;
    }
    if (this->result != other.result) {
      return false;
    }
    return true;
  }
  bool operator!=(const DockAndCharge_GetResult_Response_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct DockAndCharge_GetResult_Response_

// alias to use template instance with default allocator
using DockAndCharge_GetResult_Response =
  uav_msgs::action::DockAndCharge_GetResult_Response_<std::allocator<void>>;

// constant definitions

}  // namespace action

}  // namespace uav_msgs

namespace uav_msgs
{

namespace action
{

struct DockAndCharge_GetResult
{
  using Request = uav_msgs::action::DockAndCharge_GetResult_Request;
  using Response = uav_msgs::action::DockAndCharge_GetResult_Response;
};

}  // namespace action

}  // namespace uav_msgs


// Include directives for member types
// Member 'goal_id'
// already included above
// #include "unique_identifier_msgs/msg/detail/uuid__struct.hpp"
// Member 'feedback'
// already included above
// #include "uav_msgs/action/detail/dock_and_charge__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__uav_msgs__action__DockAndCharge_FeedbackMessage __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__action__DockAndCharge_FeedbackMessage __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace action
{

// message struct
template<class ContainerAllocator>
struct DockAndCharge_FeedbackMessage_
{
  using Type = DockAndCharge_FeedbackMessage_<ContainerAllocator>;

  explicit DockAndCharge_FeedbackMessage_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : goal_id(_init),
    feedback(_init)
  {
    (void)_init;
  }

  explicit DockAndCharge_FeedbackMessage_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : goal_id(_alloc, _init),
    feedback(_alloc, _init)
  {
    (void)_init;
  }

  // field types and members
  using _goal_id_type =
    unique_identifier_msgs::msg::UUID_<ContainerAllocator>;
  _goal_id_type goal_id;
  using _feedback_type =
    uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator>;
  _feedback_type feedback;

  // setters for named parameter idiom
  Type & set__goal_id(
    const unique_identifier_msgs::msg::UUID_<ContainerAllocator> & _arg)
  {
    this->goal_id = _arg;
    return *this;
  }
  Type & set__feedback(
    const uav_msgs::action::DockAndCharge_Feedback_<ContainerAllocator> & _arg)
  {
    this->feedback = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::action::DockAndCharge_FeedbackMessage_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::action::DockAndCharge_FeedbackMessage_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_FeedbackMessage_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::action::DockAndCharge_FeedbackMessage_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_FeedbackMessage_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_FeedbackMessage_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::action::DockAndCharge_FeedbackMessage_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::action::DockAndCharge_FeedbackMessage_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_FeedbackMessage_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::action::DockAndCharge_FeedbackMessage_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_FeedbackMessage
    std::shared_ptr<uav_msgs::action::DockAndCharge_FeedbackMessage_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__action__DockAndCharge_FeedbackMessage
    std::shared_ptr<uav_msgs::action::DockAndCharge_FeedbackMessage_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const DockAndCharge_FeedbackMessage_ & other) const
  {
    if (this->goal_id != other.goal_id) {
      return false;
    }
    if (this->feedback != other.feedback) {
      return false;
    }
    return true;
  }
  bool operator!=(const DockAndCharge_FeedbackMessage_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct DockAndCharge_FeedbackMessage_

// alias to use template instance with default allocator
using DockAndCharge_FeedbackMessage =
  uav_msgs::action::DockAndCharge_FeedbackMessage_<std::allocator<void>>;

// constant definitions

}  // namespace action

}  // namespace uav_msgs

#include "action_msgs/srv/cancel_goal.hpp"
#include "action_msgs/msg/goal_info.hpp"
#include "action_msgs/msg/goal_status_array.hpp"

namespace uav_msgs
{

namespace action
{

struct DockAndCharge
{
  /// The goal message defined in the action definition.
  using Goal = uav_msgs::action::DockAndCharge_Goal;
  /// The result message defined in the action definition.
  using Result = uav_msgs::action::DockAndCharge_Result;
  /// The feedback message defined in the action definition.
  using Feedback = uav_msgs::action::DockAndCharge_Feedback;

  struct Impl
  {
    /// The send_goal service using a wrapped version of the goal message as a request.
    using SendGoalService = uav_msgs::action::DockAndCharge_SendGoal;
    /// The get_result service using a wrapped version of the result message as a response.
    using GetResultService = uav_msgs::action::DockAndCharge_GetResult;
    /// The feedback message with generic fields which wraps the feedback message.
    using FeedbackMessage = uav_msgs::action::DockAndCharge_FeedbackMessage;

    /// The generic service to cancel a goal.
    using CancelGoalService = action_msgs::srv::CancelGoal;
    /// The generic message for the status of a goal.
    using GoalStatusMessage = action_msgs::msg::GoalStatusArray;
  };
};

typedef struct DockAndCharge DockAndCharge;

}  // namespace action

}  // namespace uav_msgs

#endif  // UAV_MSGS__ACTION__DETAIL__DOCK_AND_CHARGE__STRUCT_HPP_
