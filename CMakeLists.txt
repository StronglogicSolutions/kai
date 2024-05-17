cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard")

project(kai)

find_package(fmt CONFIG REQUIRED)
set(KLOGGER "${CMAKE_SOURCE_DIR}/third_party/klogger/build/libklog.a")

# Find libcurl
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

include_directories(
  third_party/klogger/src
  third_party/kutils/include
  third_party/kproto/include
  third_party/simdjson/singleheader
)

# Add your source files here
set(SOURCES
  src/main.cpp
  third_party/simdjson/singleheader/simdjson.cpp
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} zmq curl fmt::fmt ${KLOGGER})
