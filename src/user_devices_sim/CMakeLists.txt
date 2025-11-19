cmake_minimum_required(VERSION 3.8)
project(user_devices_sim)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(uav_msgs REQUIRED)

add_executable(user_device_node src/user_device_node.cpp)

ament_target_dependencies(user_device_node
  rclcpp
  uav_msgs
)

install(TARGETS
  user_device_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
