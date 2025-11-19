cmake_minimum_required(VERSION 3.8)
project(uav_fleet)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(uav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)

add_executable(uav_node src/uav_node.cpp)

ament_target_dependencies(uav_node
  rclcpp
  uav_msgs
  geometry_msgs
)

install(TARGETS
  uav_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
