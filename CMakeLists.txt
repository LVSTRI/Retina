cmake_minimum_required(VERSION 3.27)
project(Retina)

set(CMAKE_CXX_STANDARD 23)

set(RETINA_INCLUDE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(RETINA_SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(RETINA_ROOT_BINARY_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

option(RETINA_ENABLE_PROFILER "Enable CPU profiling" OFF)

add_subdirectory(Dependencies)
add_subdirectory(src/Retina)

include(CMake/PrecompiledHeaders.cmake)
