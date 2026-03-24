cmake_minimum_required(VERSION 3.25)
project(components C)

add_library(components STATIC
  src/code-editor.c
  src/number-field.c
  src/reg-view.c
)

target_include_directories(components PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(components
  base
  raylib
  raygui
)
