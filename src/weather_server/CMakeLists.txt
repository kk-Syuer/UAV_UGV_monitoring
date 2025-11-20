cmake_minimum_required(VERSION 3.8)
project(weather_server)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(uav_msgs REQUIRED)

add_executable(weather_node src/weather_node.cpp)

ament_target_dependencies(weather_node
  rclcpp
  uav_msgs
)

install(TARGETS
  weather_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
