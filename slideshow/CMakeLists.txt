cmake_minimum_required(VERSION 3.25)
project(slideshow C)

add_library(slideshow STATIC
  src/slideshow.c
  src/slide.c
)

target_link_libraries(slideshow
  raylib
)
