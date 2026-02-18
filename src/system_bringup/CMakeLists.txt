cmake_minimum_required(VERSION 3.8)
project(system_bringup)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

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

# Install files explicitly instead of installing whole directories.
# This avoids recursive directory symlink-install globbing warnings (CMP0009)
# raised from ament_cmake_symlink_install.cmake.
install(
  FILES launch/experiment.launch.py
  DESTINATION share/${PROJECT_NAME}/launch
)

install(
  FILES config/experiment_defaults.yaml
  DESTINATION share/${PROJECT_NAME}/config
)

file(GLOB SYSTEM_BRINGUP_RUN_CONFIGS
  "${CMAKE_CURRENT_SOURCE_DIR}/config/runs/*.yaml"
)
install(
  FILES ${SYSTEM_BRINGUP_RUN_CONFIGS}
  DESTINATION share/${PROJECT_NAME}/config/runs
)
