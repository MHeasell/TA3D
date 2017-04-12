cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0048 NEW)

# Get the current commit SHA1 from git
execute_process(
		COMMAND "git" "rev-parse" "--verify" "HEAD"
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		OUTPUT_VARIABLE GIT_COMMIT
		OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT GIT_COMMIT)
	message(FATAL_ERROR "Failed to get the current Git commit")
endif()

execute_process(
		COMMAND "git" "describe" "--dirty=-d"
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		OUTPUT_VARIABLE TA3D_GIT_DESCRIPTION
		OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT TA3D_GIT_DESCRIPTION)
	message(FATAL_ERROR "Failed to get the Git description")
endif()

if(NOT TA3D_GIT_DESCRIPTION MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+)")
	message(FATAL_ERROR "Failed to parse version from Git")
endif()

project(TA3D VERSION ${CMAKE_MATCH_1})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

enable_testing()

message(STATUS "")
message(STATUS "--- TA3D, Remake of the Total Annihilation engine ---")
message(STATUS "")


set(TA3D_WEBSITE                "http://www.ta3d.org")
set(TA3D_WEBSITE_FORUM          "http://www.ta3d.org/forums/")
set(TA3D_MAIL_BUGREPORT         "dev@ta3d.org")
set(TA3D_WEBSITE_NEW_BUGREPORT  "http://trac.ta3d.org/newticket")

set(TA3D_VERSION_HI ${PROJECT_VERSION_MAJOR})
set(TA3D_VERSION_LO ${PROJECT_VERSION_MINOR})
set(TA3D_VERSION_PATCH ${PROJECT_VERSION_PATCH})


if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "debug")
endif()


message(STATUS "Version: ${TA3D_GIT_DESCRIPTION}")

# Information about the current Operating System
message(STATUS "System: ${CMAKE_SYSTEM} (${CMAKE_SYSTEM_PROCESSOR})")


# FTGL
add_subdirectory(libs/ftgl)

add_subdirectory(libs/fmt-3.0.1)

# TA3D
add_subdirectory(ta3d)

