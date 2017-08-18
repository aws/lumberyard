cmake_minimum_required (VERSION 2.8)
project (AZCore)
  
 include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/../"
)



file(GLOB_RECURSE SRCS1 *.cpp *.c)
file(GLOB_RECURSE HDRS *.h)
add_library(AZCore ${SRCS1} ${HDRS})