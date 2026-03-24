cmake_minimum_required(VERSION 3.25)

# Build nanosvg
add_subdirectory(nanosvg)
# NOTE: This is a fix for nanosvg's include directories
target_include_directories(nanosvg     PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/nanosvg/src>)
target_include_directories(nanosvgrast PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/nanosvg/src>)

# Build raylib
set(BUILD_EXAMPLES FALSE)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(raylib)
target_compile_definitions(raylib PUBLIC
  -DSUPPORT_FILEFORMAT_JPG
)

# Build raygui
add_subdirectory(raygui)

# Build keystone
set(BUILD_LIBS_ONLY TRUE)
set(LLVM_TARGETS_TO_BUILD "X86")
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(keystone)
# NOTE: This is a fix for keystone's include directories
target_include_directories(keystone PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/keystone/include>
  $<INSTALL_INTERFACE:keystone/include>
)
