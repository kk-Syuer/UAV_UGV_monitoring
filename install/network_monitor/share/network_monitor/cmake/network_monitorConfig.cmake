# generated from ament/cmake/core/templates/nameConfig.cmake.in

# prevent multiple inclusion
if(_network_monitor_CONFIG_INCLUDED)
  # ensure to keep the found flag the same
  if(NOT DEFINED network_monitor_FOUND)
    # explicitly set it to FALSE, otherwise CMake will set it to TRUE
    set(network_monitor_FOUND FALSE)
  elseif(NOT network_monitor_FOUND)
    # use separate condition to avoid uninitialized variable warning
    set(network_monitor_FOUND FALSE)
  endif()
  return()
endif()
set(_network_monitor_CONFIG_INCLUDED TRUE)

# output package information
if(NOT network_monitor_FIND_QUIETLY)
  message(STATUS "Found network_monitor: 0.0.0 (${network_monitor_DIR})")
endif()

# warn when using a deprecated package
if(NOT "" STREQUAL "")
  set(_msg "Package 'network_monitor' is deprecated")
  # append custom deprecation text if available
  if(NOT "" STREQUAL "TRUE")
    set(_msg "${_msg} ()")
  endif()
  # optionally quiet the deprecation message
  if(NOT ${network_monitor_DEPRECATED_QUIET})
    message(DEPRECATION "${_msg}")
  endif()
endif()

# flag package as ament-based to distinguish it after being find_package()-ed
set(network_monitor_FOUND_AMENT_PACKAGE TRUE)

# include all config extra files
set(_extras "")
foreach(_extra ${_extras})
  include("${network_monitor_DIR}/${_extra}")
endforeach()
