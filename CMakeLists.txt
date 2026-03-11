cmake_minimum_required(VERSION 3.25)
project(ccit-slides)

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type
if(NOT DEFINED ${CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message(STATUS "Building with '${CMAKE_BUILD_TYPE}' profile")

# Build raylib
set(BUILD_EXAMPLES FALSE)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(thirdparty/raylib)

# Build raygui
add_subdirectory(thirdparty/raygui)

# Build keystone
set(BUILD_LIBS_ONLY TRUE)
set(LLVM_TARGETS_TO_BUILD "X86")
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(thirdparty/keystone)
# NOTE: This is a fix for keystone's include directories
target_include_directories(keystone PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/keystone/include>
  $<INSTALL_INTERFACE:thirdparty/keystone/include>
)

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

include_directories(BEFORE SYSTEM include/)

add_subdirectory(debugger)
add_subdirectory(components)
add_subdirectory(slideshow)
