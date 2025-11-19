# generated from ament/cmake/core/templates/nameConfig.cmake.in

# prevent multiple inclusion
if(_fault_injector_CONFIG_INCLUDED)
  # ensure to keep the found flag the same
  if(NOT DEFINED fault_injector_FOUND)
    # explicitly set it to FALSE, otherwise CMake will set it to TRUE
    set(fault_injector_FOUND FALSE)
  elseif(NOT fault_injector_FOUND)
    # use separate condition to avoid uninitialized variable warning
    set(fault_injector_FOUND FALSE)
  endif()
  return()
endif()
set(_fault_injector_CONFIG_INCLUDED TRUE)

# output package information
if(NOT fault_injector_FIND_QUIETLY)
  message(STATUS "Found fault_injector: 0.0.0 (${fault_injector_DIR})")
endif()

# warn when using a deprecated package
if(NOT "" STREQUAL "")
  set(_msg "Package 'fault_injector' is deprecated")
  # append custom deprecation text if available
  if(NOT "" STREQUAL "TRUE")
    set(_msg "${_msg} ()")
  endif()
  # optionally quiet the deprecation message
  if(NOT ${fault_injector_DEPRECATED_QUIET})
    message(DEPRECATION "${_msg}")
  endif()
endif()

# flag package as ament-based to distinguish it after being find_package()-ed
set(fault_injector_FOUND_AMENT_PACKAGE TRUE)

# include all config extra files
set(_extras "")
foreach(_extra ${_extras})
  include("${fault_injector_DIR}/${_extra}")
endforeach()
