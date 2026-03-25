cmake_minimum_required(VERSION 3.25)
project(ccit-ss C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message(STATUS "Building with '${CMAKE_BUILD_TYPE}' profile")

add_subdirectory(thirdparty)

# Set compiler options
add_compile_options(-Wall -Wextra)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Debug compiler options
  add_compile_options(-O0 -ggdb)
  if (ENABLE_ASAN)
    message(STATUS "Enabling ASAN")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
  endif()
else()
  # Release compiler options
  add_compile_options(-O3 -Werror)
endif()

include(cmake/ResPak.cmake)

add_subdirectory(lib)
add_subdirectory(ss0)
