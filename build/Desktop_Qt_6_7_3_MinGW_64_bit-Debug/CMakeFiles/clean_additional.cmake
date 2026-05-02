# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\smartoil_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\smartoil_autogen.dir\\ParseCache.txt"
  "smartoil_autogen"
  )
endif()
