cmake_minimum_required(VERSION 3.8)
project(ugv_charger)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(uav_msgs REQUIRED)

add_executable(ugv_charger_node src/ugv_charger_node.cpp)

ament_target_dependencies(ugv_charger_node
  rclcpp
  uav_msgs
)

install(TARGETS
  ugv_charger_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
