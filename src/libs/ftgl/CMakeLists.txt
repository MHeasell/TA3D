project(ftgl)
cmake_minimum_required(VERSION 2.6)

include_directories(
			${CMAKE_CURRENT_SOURCE_DIR}/src/
			${CMAKE_CURRENT_SOURCE_DIR}
			${CMAKE_CURRENT_SOURCE_DIR}/include/
			${CMAKE_CURRENT_SOURCE_DIR}/src/FTGL/)

set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)

if(WIN32 OR CMAKE_CROSSCOMPILING)
	set(FREETYPE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../ta3d/src/tools/win32/mingw32/include/"
							  "${CMAKE_CURRENT_SOURCE_DIR}/../../ta3d/src/tools/win32/mingw32/include/freetype2/")
	set(FREETYPE_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/../../ta3d/src/tools/win32/mingw32/libs/libfreetype.dll.a")
	message(STATUS "FreeType Library : ${FREETYPE_LIBRARIES}")
else()
	find_package(Freetype)
endif()

include_directories(${FREETYPE_INCLUDE_DIRS})

add_subdirectory(src)
