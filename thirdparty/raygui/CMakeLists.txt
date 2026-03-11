cmake_minimum_required(VERSION 3.25)
project(raygui C)

add_library(raygui STATIC
  src/raygui.c
)

target_link_libraries(raygui
  raylib
)

target_include_directories(raygui PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
