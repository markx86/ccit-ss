cmake_minimum_required(VERSION 3.25)
project(debugger C)

add_executable(runner
  src/runner.c
)

add_library(debugger STATIC
  src/debugger.c
)
