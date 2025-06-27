# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "MinSizeRel")
  file(REMOVE_RECURSE
  "ArbitrageEngine_autogen"
  "CMakeFiles/ArbitrageEngine_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/ArbitrageEngine_autogen.dir/ParseCache.txt"
  )
endif()
