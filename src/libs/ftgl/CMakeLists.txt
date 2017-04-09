project(ftgl)
cmake_minimum_required(VERSION 2.6)

include_directories(
			${CMAKE_CURRENT_SOURCE_DIR}/src/
			${CMAKE_CURRENT_SOURCE_DIR}
			${CMAKE_CURRENT_SOURCE_DIR}/include/
			${CMAKE_CURRENT_SOURCE_DIR}/src/FTGL/)

set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)

find_package(Freetype)

include_directories(${FREETYPE_INCLUDE_DIRS})

add_subdirectory(src)
