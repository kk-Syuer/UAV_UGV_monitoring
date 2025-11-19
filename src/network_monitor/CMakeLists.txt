cmake_minimum_required(VERSION 3.8)
project(network_monitor)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(uav_msgs REQUIRED)

add_executable(network_monitor_node src/network_monitor_node.cpp)

ament_target_dependencies(network_monitor_node
  rclcpp
  uav_msgs
)

install(TARGETS
  network_monitor_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
