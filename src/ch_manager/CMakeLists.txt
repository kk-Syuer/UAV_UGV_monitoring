cmake_minimum_required(VERSION 3.8)
project(ch_manager)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(uav_msgs REQUIRED)

add_executable(ch_manager_node src/ch_manager_node.cpp)

ament_target_dependencies(ch_manager_node
  rclcpp
  uav_msgs
)

install(TARGETS
  ch_manager_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
