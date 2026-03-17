cmake_minimum_required(VERSION 3.25)
project(base C)

add_library(base STATIC
  src/pak.c
  src/sdf-font.c
  src/arena.c
)

target_include_directories(base PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(base
  raylib
)
