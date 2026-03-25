cmake_minimum_required(VERSION 3.25)
project(ss0 CXX)

add_executable(ss0
  src/ss0.c
)

target_resource_pak(ss0
  res/Inter-Regular.ttf
  res/Inter-Bold.ttf
  res/Inter-Italic.ttf
  res/Inter-BoldItalic.ttf
  res/JetBrainsMonoNL-Regular.ttf
  res/JetBrainsMonoNL-Bold.ttf
  res/JetBrainsMonoNL-Italic.ttf
  res/JetBrainsMonoNL-BoldItalic.ttf
  res/cpu-mem.svg
)

target_link_libraries(ss0
  raylib
  raygui
  keystone
  debugger
  slideshow
  components
)
