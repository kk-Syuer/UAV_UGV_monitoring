cmake_minimum_required(VERSION 3.8)
project(coverage_planner)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(uav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)

add_executable(coverage_planner_node src/coverage_planner_node.cpp)
ament_target_dependencies(coverage_planner_node
  rclcpp
  uav_msgs
  geometry_msgs
)

install(TARGETS
  coverage_planner_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
