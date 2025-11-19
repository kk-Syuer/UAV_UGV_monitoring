cmake_minimum_required(VERSION 3.8)
project(sink_gateway)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(uav_msgs REQUIRED)

add_executable(sink_gateway_node src/sink_gateway_node.cpp)

ament_target_dependencies(sink_gateway_node
  rclcpp
  uav_msgs
)

install(TARGETS
  sink_gateway_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
