cmake_minimum_required(VERSION 2.6)
project(ftgl)

find_package(Freetype)
include_directories(${FREETYPE_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}) # for config.h
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)
