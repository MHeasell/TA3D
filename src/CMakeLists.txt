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

# Get the current commit SHA1 from git
execute_process(
    COMMAND "git" "rev-parse" "--verify" "HEAD"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)



message(STATUS "Version : v${TA3D_VERSION_HI}.${TA3D_VERSION_LO}.${TA3D_VERSION_PATCH} (commit: ${GIT_COMMIT})")

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
else()
	Message(SEND_ERROR "The Yuni Framework could not be found.")
endif()



# FTGL
if(NOT APPLE)
	add_subdirectory(libs/ftgl)
endif()

# SDL_sgui
if (WIN32 OR (CMAKE_CROSSCOMPILING AND NOT UNIX))
	set( SDL_FOUND included )
	set( SDL_INCLUDE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/ta3d/src/tools/win32/mingw32/include/SDL")
	set( SDL_LIBRARY "mingw32;${CMAKE_CURRENT_SOURCE_DIR}/ta3d/src/tools/win32/mingw32/libs/libSDLmain.a;${CMAKE_CURRENT_SOURCE_DIR}/src/tools/win32/mingw32/libs/SDL.dll")
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ta3d/src/tools/win32/mingw32/include)
endif ()
add_subdirectory(libs/SDL_sgui)

# TA3D
add_subdirectory(ta3d)

