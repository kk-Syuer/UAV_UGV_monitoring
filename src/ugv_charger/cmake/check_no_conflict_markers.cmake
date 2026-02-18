if(NOT DEFINED INPUT_FILE)
  message(FATAL_ERROR "check_no_conflict_markers.cmake requires -DINPUT_FILE=<path>")
endif()

file(READ "${INPUT_FILE}" _content)

if(_content MATCHES "(^|\n)(<<<<<<<|=======|>>>>>>>)[^\n]*")
  message(FATAL_ERROR
    "Git conflict markers detected in ${INPUT_FILE}. Resolve merge conflicts before building.")
endif()
