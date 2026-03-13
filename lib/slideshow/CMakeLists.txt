cmake_minimum_required(VERSION 3.25)
project(slideshow C)

add_library(slideshow STATIC
  src/slideshow.c
  src/slide.c
)

target_include_directories(slideshow PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(slideshow
  base
  raylib
)
