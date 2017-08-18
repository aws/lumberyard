cmake_minimum_required (VERSION 2.8)
project (DHTests)
  
 include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/../"
)



file(GLOB_RECURSE SRCS1 *.cpp)
file(GLOB_RECURSE HDRS *.h)
add_executable(DHTests ${SRCS1} ${HDRS})
target_link_libraries( AZTests AZCore pthread rt atomic)