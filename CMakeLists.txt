cmake_minimum_required(VERSION 3.0)
project(kai)

# Find ZeroMQ
find_package(ZMQ REQUIRED)
include_directories(${ZMQ_INCLUDE_DIRS})

# Find libcurl
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# Add your source files here
set(SOURCES
  src/main.cpp
)

# Add executable
add_executable(my_program ${SOURCES})

# Link libraries
target_link_libraries(my_program ${ZMQ_LIBRARIES} ${CURL_LIBRARIES})
