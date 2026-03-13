cmake_minimum_required(VERSION 3.25)
project(debugger C)

# Runner
add_executable(runner
  src/runner.c
)

target_include_directories(runner PRIVATE
  include
)

# Debugger
add_library(debugger STATIC
  src/debugger.c
)

target_include_directories(debugger PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
