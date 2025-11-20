// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from uav_msgs:msg/WeatherStatus.idl
// generated code does not contain a copyright notice

#ifndef UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__STRUCT_HPP_
#define UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__uav_msgs__msg__WeatherStatus __attribute__((deprecated))
#else
# define DEPRECATED__uav_msgs__msg__WeatherStatus __declspec(deprecated)
#endif

namespace uav_msgs
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct WeatherStatus_
{
  using Type = WeatherStatus_<ContainerAllocator>;

  explicit WeatherStatus_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->rain_intensity = 0.0f;
      this->wind_speed = 0.0f;
      this->wind_direction_deg = 0.0f;
      this->temperature_c = 0.0f;
    }
  }

  explicit WeatherStatus_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->rain_intensity = 0.0f;
      this->wind_speed = 0.0f;
      this->wind_direction_deg = 0.0f;
      this->temperature_c = 0.0f;
    }
  }

  // field types and members
  using _rain_intensity_type =
    float;
  _rain_intensity_type rain_intensity;
  using _wind_speed_type =
    float;
  _wind_speed_type wind_speed;
  using _wind_direction_deg_type =
    float;
  _wind_direction_deg_type wind_direction_deg;
  using _temperature_c_type =
    float;
  _temperature_c_type temperature_c;

  // setters for named parameter idiom
  Type & set__rain_intensity(
    const float & _arg)
  {
    this->rain_intensity = _arg;
    return *this;
  }
  Type & set__wind_speed(
    const float & _arg)
  {
    this->wind_speed = _arg;
    return *this;
  }
  Type & set__wind_direction_deg(
    const float & _arg)
  {
    this->wind_direction_deg = _arg;
    return *this;
  }
  Type & set__temperature_c(
    const float & _arg)
  {
    this->temperature_c = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    uav_msgs::msg::WeatherStatus_<ContainerAllocator> *;
  using ConstRawPtr =
    const uav_msgs::msg::WeatherStatus_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<uav_msgs::msg::WeatherStatus_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<uav_msgs::msg::WeatherStatus_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::WeatherStatus_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::WeatherStatus_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      uav_msgs::msg::WeatherStatus_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<uav_msgs::msg::WeatherStatus_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<uav_msgs::msg::WeatherStatus_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<uav_msgs::msg::WeatherStatus_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__uav_msgs__msg__WeatherStatus
    std::shared_ptr<uav_msgs::msg::WeatherStatus_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__uav_msgs__msg__WeatherStatus
    std::shared_ptr<uav_msgs::msg::WeatherStatus_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const WeatherStatus_ & other) const
  {
    if (this->rain_intensity != other.rain_intensity) {
      return false;
    }
    if (this->wind_speed != other.wind_speed) {
      return false;
    }
    if (this->wind_direction_deg != other.wind_direction_deg) {
      return false;
    }
    if (this->temperature_c != other.temperature_c) {
      return false;
    }
    return true;
  }
  bool operator!=(const WeatherStatus_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct WeatherStatus_

// alias to use template instance with default allocator
using WeatherStatus =
  uav_msgs::msg::WeatherStatus_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace uav_msgs

#endif  // UAV_MSGS__MSG__DETAIL__WEATHER_STATUS__STRUCT_HPP_
