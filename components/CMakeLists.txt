cmake_minimum_required(VERSION 3.25)
project(components C)

add_library(components STATIC
  src/code-editor.c
  src/sdf-font.c
)

target_link_libraries(components
  raylib
)
