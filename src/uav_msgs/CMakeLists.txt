cmake_minimum_required(VERSION 3.8)
project(uav_msgs)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
# Clean up any stale python symlink path from previous builds so the
# ament_cmake_python target can recreate it without failing.
file(REMOVE_RECURSE
  "${CMAKE_CURRENT_BINARY_DIR}/ament_cmake_python/${PROJECT_NAME}/${PROJECT_NAME}")
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)
set(ROSIDL_TYPESUPPORT "rosidl_typesupport_cpp")
set(ROSIDL_DEFAULT_GENERATORS_DISABLE_C_TYPESUPPORT ON CACHE BOOL
  "Disable C typesupport generation for ROSIDL default generators" FORCE)
find_package(rosidl_typesupport_cpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(geometry_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/UavStatus.msg"
  "msg/Heartbeat.msg"
  "msg/TrafficMessage.msg"
  "msg/ClusterInfo.msg"
  "msg/TaskPoint.msg"
  "msg/TaskPointArray.msg"
  "msg/ChargeDecision.msg"
  "msg/ChargeRequest.msg"
  "srv/RequestCharge.srv"
  "srv/SendDebugText.srv"

  "action/DockAndCharge.action"
  "msg/WeatherStatus.msg"
  "msg/FailureEvent.msg"  
  "msg/UavDeployment.msg"
  "msg/RoutingTable.msg"

  DEPENDENCIES builtin_interfaces geometry_msgs
)


ament_export_dependencies(rosidl_default_runtime)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
