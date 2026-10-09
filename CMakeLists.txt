cmake_minimum_required(VERSION 3.14)
project(DinoRunner LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Put the finished game in <build dir>/bin on every platform / generator.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# GLFW is downloaded and built automatically: nothing to install by hand.
include(FetchContent)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)  # X11 / XWayland is enough
FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(glfw)

add_executable(dino_runner trex.cpp)

if(WIN32)
  target_link_libraries(dino_runner PRIVATE glfw opengl32)
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(dino_runner PRIVATE glfw OpenGL::GL)
endif()

# MinGW (Windows build, also used by Docker cross-compile): one self-contained .exe,
# no console window, no extra DLLs to ship.
if(MINGW)
  target_link_options(dino_runner PRIVATE -static -mwindows)
endif()
# Linux: don't depend on the newest libstdc++ of the build machine.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_options(dino_runner PRIVATE -static-libstdc++ -static-libgcc)
endif()
