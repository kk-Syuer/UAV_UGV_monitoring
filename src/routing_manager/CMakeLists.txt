cmake_minimum_required(VERSION 3.8)
project(routing_manager)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(uav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(routing_manager_node src/routing_manager_node.cpp)

ament_target_dependencies(routing_manager_node
  rclcpp
  uav_msgs
  geometry_msgs
  std_msgs
)

install(TARGETS
  routing_manager_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
