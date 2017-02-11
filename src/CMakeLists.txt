project(TA3DRepository)
cmake_minimum_required(VERSION 2.6)

Message(STATUS "")
Message(STATUS "--- TA3D, Remake of the Total Annihilation engine ---")
Message(STATUS "")


set(TA3D_WEBSITE                "http://www.ta3d.org")
set(TA3D_WEBSITE_FORUM          "http://www.ta3d.org/forums/")
set(TA3D_MAIL_BUGREPORT         "dev@ta3d.org")
set(TA3D_WEBSITE_NEW_BUGREPORT  "http://trac.ta3d.org/newticket")

set(TA3D_VERSION_HI     "0")     # 0.6.0
set(TA3D_VERSION_LO     "6")
set(TA3D_VERSION_PATCH  "0")
set(TA3D_VERSION_TAG    "WIP")


if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "debug")
endif()


if (APPLE)
	set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")
endif()

#
# Getting the latest revision number
#
if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
	execute_process(COMMAND "subwcrev.exe" "${PROJECT_SOURCE_DIR}" OUTPUT_VARIABLE SVN_REV)
	string(REGEX REPLACE ".* committed at revision ([0-9]+).*" "\\1" SVN_REV "${SVN_REV}")
else(WIN32 AND NOT CMAKE_CROSSCOMPILING)
	execute_process(COMMAND "svnversion" "-n" "${PROJECT_SOURCE_DIR}" OUTPUT_VARIABLE SVN_REV)
	string(REGEX REPLACE "[0-9]+:" "" SVN_REV "${SVN_REV}")
	string(REPLACE "M" "" SVN_REV "${SVN_REV}")
endif(WIN32 AND NOT CMAKE_CROSSCOMPILING)

# Dealing with bad revision numbers
if(NOT "${SVN_REV}" GREATER 1)
	set(SVN_REV 1)
endif(NOT "${SVN_REV}" GREATER 1)



message(STATUS "Version : v${TA3D_VERSION_HI}.${TA3D_VERSION_LO}.${TA3D_VERSION_PATCH} (Rev: ${SVN_REV})")

# Information about the current Operating System
Message(STATUS " System : ${CMAKE_SYSTEM} (${CMAKE_SYSTEM_PROCESSOR})")



#
# Yuni
#
# We have to copy the yuni's profile dedicated to Ta3D into the yuni sources folder.
#
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/YuniProfileBuildTemplate.cmake"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/yuni/src/ProfileBuild.cmake" COPYONLY)
# The sources
add_subdirectory(libs/yuni/src)
# At this point, the program 'yuni-config' should be available, as requested
# by the profile.
# We have to provide the path where the file 'FindYuni.cmake' is located
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/yuni/packages/cmake")
# Trying to find the Yuni framework
Find_Package(Yuni COMPONENTS core)
if(Yuni_FOUND)
	# Infos
	message(STATUS "The Yuni Framework : ${Yuni_INFOS}")
else(Yuni_FOUND)
	Message(SEND_ERROR "The Yuni Framework could not be found.")
endif(Yuni_FOUND)



# FTGL
if(NOT APPLE)
	add_subdirectory(libs/ftgl)
endif(NOT APPLE)

# SDL_sgui
if (WIN32 OR (CMAKE_CROSSCOMPILING AND NOT UNIX))
	set( SDL_FOUND included )
	set( SDL_INCLUDE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/ta3d/src/tools/win32/mingw32/include/SDL")
	set( SDL_LIBRARY "mingw32;${CMAKE_CURRENT_SOURCE_DIR}/ta3d/src/tools/win32/mingw32/libs/libSDLmain.a;${CMAKE_CURRENT_SOURCE_DIR}/src/tools/win32/mingw32/libs/SDL.dll")
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ta3d/src/tools/win32/mingw32/include)
endif (WIN32 OR (CMAKE_CROSSCOMPILING AND NOT UNIX))
add_subdirectory(libs/SDL_sgui)

# TA3D
add_subdirectory(ta3d)

