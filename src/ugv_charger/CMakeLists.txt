cmake_minimum_required(VERSION 3.8)
project(ugv_charger)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(uav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(ugv_charger_node src/ugv_charger_node.cpp)

add_custom_command(
  TARGET ugv_charger_node
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND}
          -DINPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/src/ugv_charger_node.cpp
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_no_conflict_markers.cmake
  COMMENT "Checking ugv_charger_node.cpp for unresolved merge conflict markers"
)

ament_target_dependencies(ugv_charger_node
  rclcpp
  uav_msgs
  geometry_msgs
  std_msgs
)

install(TARGETS
  ugv_charger_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
