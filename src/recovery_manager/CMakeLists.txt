cmake_minimum_required(VERSION 3.8)
project(recovery_manager)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(uav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(recovery_manager_node src/recovery_manager_node.cpp)

ament_target_dependencies(recovery_manager_node
  rclcpp
  uav_msgs
  geometry_msgs
  std_msgs
)

install(TARGETS
  recovery_manager_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
