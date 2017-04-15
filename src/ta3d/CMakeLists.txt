
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

message(STATUS "")
message(STATUS "--- TA3D ---")
message(STATUS "")


set(CPACK_PACKAGE_VERSION_MAJOR "${TA3D_VERSION_HI}")
set(CPACK_PACKAGE_VERSION_MINOR "${TA3D_VERSION_LO}")
set(CPACK_PACKAGE_VERSION_PATCH "${TA3D_VERSION_PATCH}")

set(CPACK_PACKAGE_VERSION "${TA3D_VERSION_HI}.${TA3D_VERSION_LO}.${TA3D_VERSION_PATCH}")
set(CPACK_PACKAGE_NAME "TA3D")

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${TA3D_GIT_DESCRIPTION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_OUTPUT_FILE_PREFIX "dist")



#
# -----------------------------------------------------------------------------
#


# Initialization
set(LIBS "")
set(PACKAGE_BUGREPORT "${TA3D_MAIL_BUGREPORT}")
set(PACKAGE_VERSION "${TA3D_VERSION_HI}.${TA3D_VERSION_LO}.${TA3D_VERSION_PATCH}")
set(PACKAGE_STRING "TA3D ${PACKAGE_VERSION}")
set(TA3D_ENGINE_VERSION "${PACKAGE_STRING}")


set(TA3D_COMMON_CPP_FLAGS  "")
if(NOT WIN32 OR MINGW)
	set(TA3D_COMMON_CPP_FLAGS  "${TA3D_COMMON_CPP_FLAGS} -W -Wextra -Wall -Wfloat-equal -Woverloaded-virtual")
	set(TA3D_COMMON_CPP_FLAGS  "${TA3D_COMMON_CPP_FLAGS} -Wconversion")
endif()


#
# Command line options for G++ (Debug)
#
# Ex: cmake . -DCMAKE_BUILD_TYPE=release
#
if("${CMAKE_BUILD_TYPE}" STREQUAL "release")

	#
	# Build Configuration: Release
	#
	message(STATUS "Build Configuration: Release")

	if(NOT WIN32)
		set(CMAKE_CXX_FLAGS "${TA3D_COMMON_CPP_FLAGS} -O2 -fomit-frame-pointer -mfpmath=sse -msse -msse2")
		message(STATUS "Default C++ flags set to `${CMAKE_CXX_FLAGS}`")
	else()
		if(MINGW)
			set(CMAKE_CXX_FLAGS "${TA3D_COMMON_CPP_FLAGS} -O2 -fomit-frame-pointer")
		endif()
		if(MSVC)
			set(CMAKE_CXX_FLAGS "${TA3D_COMMON_CPP_FLAGS} /Ob2 /Ot /O2 /MDd /Wp64 /Oy /MT /GL /GS-")
		endif()
	endif()
	add_definitions("-DNDEBUG") # Remove asserts

else()

	#
	# Build Configuration: Debug
	#
	message(STATUS "Build Configuration: Debug")

	if(NOT WIN32)
		Set(CMAKE_CXX_FLAGS "${TA3D_COMMON_CPP_FLAGS} -g -ggdb2")
	else()
		if(MINGW)
			set(CMAKE_CXX_FLAGS "${TA3D_COMMON_CPP_FLAGS} -g2")
		endif()
		if(MSVC)
			set(CMAKE_CXX_FLAGS "/GR /Ob2 /Ot /MDd /MTd /fp:except")
		endif()
	endif()
	add_definitions("-DLOGS_USE_DEBUG")

endif()





#
# Platform
#
if(NOT WIN32)
	set(LINUX 1)
	add_definitions("-DTA3D_PLATFORM_LINUX")
else()
	add_definitions("-DTA3D_PLATFORM_WINDOWS")
	if(MSVC)
		add_definitions("-DTA3D_PLATFORM_MSVC")
	endif()
endif()
add_definitions("-DTA3D_PLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"")


include(CheckIncludeFile)


#
# --- SDL ---
#
set(SDL_PREFIX "[SDL]")
find_package(SDL2)
find_package(SDL2_image)
find_package(SDL2_mixer)
find_package(SDL2_net)

message(STATUS "${SDL_PREFIX} SDL: ${SDL2_FOUND}")
message(STATUS "${SDL_PREFIX} SDL_image: ${SDL2_IMAGE_FOUND}")
message(STATUS "${SDL_PREFIX} SDL_mixer: ${SDL2_MIXER_FOUND}")
message(STATUS "${SDL_PREFIX} SDL_net: ${SDL2_NET_FOUND}")
message(STATUS "${SDL_PREFIX} Include dir: ${SDL2_INCLUDE_DIR}")
message(STATUS "${SDL_PREFIX} The SDL library: ${SDL2_LIBRARY}")
message(STATUS "${SDL_PREFIX} The SDL_image library: ${SDL2_IMAGE_LIBRARIES}")
message(STATUS "${SDL_PREFIX} The SDL_mixer library: ${SDL2_MIXER_LIBRARIES}")
message(STATUS "${SDL_PREFIX} The SDL_net library: ${SDL2_NET_LIBRARIES}")

if(NOT SDL2_FOUND)
	message(FATAL_ERROR "SDL not found, SDL is required")
endif()
if(NOT SDL2_IMAGE_FOUND)
	message(FATAL_ERROR "SDL_image not found, SDL_image is required")
endif()
if(NOT SDL2_MIXER_FOUND)
	message(FATAL_ERROR "SDL_mixer not found, SDL_mixer is required")
endif()
if(NOT SDL2_NET_FOUND)
	message(FATAL_ERROR "SDL_net not found, SDL_net is required")
endif()

link_libraries(${SDL2_LIBRARY})
link_libraries(${SDL2_IMAGE_LIBRARIES})
link_libraries(${SDL2_MIXER_LIBRARIES})
link_libraries(${SDL2_NET_LIBRARIES})

include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
include_directories(${SDL2_MIXER_INCLUDE_DIRS})
include_directories(${SDL2_NET_INCLUDE_DIRS})

set(TA3D_LUA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libs/lua-5.1.4")
set(TA3D_LUA_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/lua")
set(TA3D_LUA_INCLUDE_DIR "${TA3D_LUA_INSTALL_DIR}/include")
set(TA3D_LUA_LIBRARY_DIR "${TA3D_LUA_INSTALL_DIR}/lib")

link_directories(${TA3D_LUA_LIBRARY_DIR})
include_directories(${TA3D_LUA_INCLUDE_DIR})

#
# --- LUA ---
#
if(WIN32)
    set(TA3D_LUA_PLATFORM mingw)
else()
    set(TA3D_LUA_PLATFORM linux)
endif()

ExternalProject_Add(
    lua
    CONFIGURE_COMMAND ""
    SOURCE_DIR ${TA3D_LUA_SOURCE_DIR}
    BUILD_COMMAND make ${TA3D_LUA_PLATFORM}
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND make INSTALL_TOP="<INSTALL_DIR>" install
    INSTALL_DIR ${TA3D_LUA_INSTALL_DIR}
)

set(LIBS "-llua -lz -ldl")

#
# --- GLEW ---
#
find_library(GLEW_LIB GLEW)
if(NOT GLEW_LIB)
	find_library(GLEW_LIB glew)
    if(NOT GLEW_LIB)
        find_library(GLEW_LIB glew32)
    endif()
endif()
find_file(GLEW_HEADER GL/glew.h)
if(NOT GLEW_HEADER)
	message(FATAL_ERROR "glew.h not found!")
endif()
message(STATUS "GLEW Header : ${GLEW_HEADER}")
message(STATUS "GLEW Library : ${GLEW_LIB}")

if(NOT GLEW_LIB)
	message(FATAL_ERROR "GLEW library not found!")
endif()
set(LIBS "${LIBS} ${GLEW_LIB}")

#
# --- GLU ---
#
if(UNIX)
	find_library(GLU_LIB GLU)
	if(NOT GLU_LIB)
		find_library(GLEW_LIB glu)
	endif()
	message(STATUS "GLU Library : ${GLU_LIB}")
	set(LIBS "${LIBS} ${GLU_LIB} -lGL")
endif()


#
# --- FTGL ---
#
find_package(Freetype)

include_directories(${FREETYPE_INCLUDE_DIRS})

set(FTGL_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../libs/ftgl/lib/libftgl.a)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libs/ftgl/include)
set(__FTGL__lower__ true)

message(STATUS "FTGL Library : ${FTGL_LIB}")
set(LIBS "${LIBS} ${FTGL_LIB}")

#
# --- CPack: Installation process ---
#

message("-- TA3D will be installed in: ${CMAKE_INSTALL_PREFIX}")

if(WIN32)
	set(TA3D_RESOURCES_INSTALL_PATH ".")
	set(TA3D_BINARIES_INSTALL_PATH ".")
else()
	set(TA3D_RESOURCES_INSTALL_PATH "share/games/ta3d")
	set(TA3D_BINARIES_INSTALL_PATH "games")
endif()

# Find and install Windows DLLs
if(WIN32)
	find_program(SDL2_DLL "SDL2.dll")
	find_program(SDL2_IMAGE_DLL "SDL2_image.dll")
	find_program(SDL2_MIXER_DLL "libSDL2_mixer-2-0-0.dll")
	find_program(SDL2_NET_DLL "SDL2_net.dll")
	find_program(GLEW_DLL "glew32.dll")
	find_program(FREETYPE_DLL "libfreetype-6.dll")
	find_program(JPEG_DLL "libjpeg-8.dll")
	find_program(PNG_DLL "libpng16-16.dll")
	find_program(TIFF_DLL "libtiff-5.dll")
	find_program(WEBP_DLL "libwebp-7.dll")
	find_program(ZLIB_DLL "zlib1.dll")
	find_program(LZMA_DLL "liblzma-5.dll")
	find_program(BZ2_DLL "libbz2-1.dll")
	find_program(MAD_DLL "libmad-0.dll")
	find_program(WINPTHREAD_DLL "libwinpthread-1.dll")
	find_program(GCC_DLL "libgcc_s_seh-1.dll")
	find_program(STDLIB_DLL "libstdc++-6.dll")
	find_program(HARFBUZZ_DLL "libharfbuzz-0.dll")
	find_program(GLIB_DLL "libglib-2.0-0.dll")
	find_program(INTL_DLL "libintl-8.dll")
	find_program(ICONV_DLL "libiconv-2.dll")
	find_program(PCRE_DLL "libpcre-1.dll")
	find_program(GRAPHITE_DLL "libgraphite2.dll")

	message("SDL2 DLL: ${SDL2_DLL}")
	message("SDL2_image DLL: ${SDL2_IMAGE_DLL}")
	message("SDL2_mixer DLL: ${SDL2_MIXER_DLL}")
	message("SDL2_net DLL: ${SDL2_NET_DLL}")
	message("glew DLL: ${GLEW_DLL}")
	message("freetype DLL: ${FREETYPE_DLL}")
	message("JPEG DLL: ${JPEG_DLL}")
	message("PNG DLL: ${PNG_DLL}")
	message("TIFF DLL: ${TIFF_DLL}")
	message("WEBP DLL: ${WEBP_DLL}")
	message("ZLIB DLL: ${ZLIB_DLL}")
	message("LZMA DLL: ${LZMA_DLL}")
	message("BZ2 DLL: ${BZ2_DLL}")
	message("MAD DLL: ${MAD_DLL}")
	message("winpthread DLL: ${WINPTHREAD_DLL}")
	message("libgcc DLL: ${GCC_DLL}")
	message("stdlib DLL: ${STDLIB_DLL}")
	message("harfbuzz DLL: ${HARFBUZZ_DLL}")
	message("glib DLL: ${GLIB_DLL}")
	message("intl DLL: ${INTL_DLL}")
	message("iconv DLL: ${ICONV_DLL}")
	message("pcre DLL: ${PCRE_DLL}")
	message("grapite DLL: ${GRAPHITE_DLL}")

	if(SDL2_DLL STREQUAL "SDL2_DLL-NOTFOUND")
		message(FATAL_ERROR "SDL DLL not found")
	endif()
	if(SDL2_IMAGE_DLL STREQUAL "SDL2_DLL-NOTFOUND")
		message(FATAL_ERROR "SDL_image DLL not found")
	endif()
	if(SDL2_MIXER_DLL STREQUAL "SDL2_MIXER_DLL-NOTFOUND")
		message(FATAL_ERROR "SDL_mixer DLL not found")
	endif()
	if(SDL2_NET_DLL STREQUAL "SDL2_NET_DLL-NOTFOUND")
		message(FATAL_ERROR "SDL_net DLL not found")
	endif()
	if(GLEW_DLL STREQUAL "GLEW_DLL-NOTFOUND")
		message(FATAL_ERROR "glew DLL not found")
	endif()
	if(FREETYPE_DLL STREQUAL "FREETYPE_DLL-NOTFOUND")
		message(FATAL_ERROR "freetype DLL not found")
	endif()
	if(JPEG_DLL STREQUAL "JPEG_DLL-NOTFOUND")
		message(FATAL_ERROR "libjpeg DLL not found")
	endif()
	if(PNG_DLL STREQUAL "PNG_DLL-NOTFOUND")
		message(FATAL_ERROR "libpng DLL not found")
	endif()
	if(TIFF_DLL STREQUAL "TIFF_DLL-NOTFOUND")
		message(FATAL_ERROR "libtiff DLL not found")
	endif()
	if(WEBP_DLL STREQUAL "WEBP_DLL-NOTFOUND")
		message(FATAL_ERROR "webp DLL not found")
	endif()
	if(ZLIB_DLL STREQUAL "ZLIB_DLL-NOTFOUND")
		message(FATAL_ERROR "zlib DLL not found")
	endif()
	if(LZMA_DLL STREQUAL "LZMA_DLL-NOTFOUND")
		message(FATAL_ERROR "lzma DLL not found")
	endif()
	if(BZ2_DLL STREQUAL "BZ2_DLL-NOTFOUND")
		message(FATAL_ERROR "bz2 DLL not found")
	endif()
	if(MAD_DLL STREQUAL "MAD_DLL-NOTFOUND")
		message(FATAL_ERROR "libmad DLL not found")
	endif()
	if(WINPTHREAD_DLL STREQUAL "WINPTHREAD_DLL-NOTFOUND")
		message(FATAL_ERROR "winpthread DLL not found")
	endif()
	if(GCC_DLL STREQUAL "GCC_DLL-NOTFOUND")
		message(FATAL_ERROR "libgcc DLL not found")
	endif()
	if(STDLIB_DLL STREQUAL "STDLIB_DLL-NOTFOUND")
		message(FATAL_ERROR "stdlib DLL not found")
	endif()
	if(HARFBUZZ_DLL STREQUAL "HARFBUZZ_DLL-NOTFOUND")
		message(FATAL_ERROR "harfbuzz DLL not found")
	endif()
	if(GLIB_DLL STREQUAL "GLIB_DLL-NOTFOUND")
		message(FATAL_ERROR "glib DLL not found")
	endif()
	if(INTL_DLL STREQUAL "INTL_DLL-NOTFOUND")
		message(FATAL_ERROR "intl DLL not found")
	endif()
	if(ICONV_DLL STREQUAL "ICONV_DLL-NOTFOUND")
		message(FATAL_ERROR "ivonv DLL not found")
	endif()
	if(PCRE_DLL STREQUAL "PCRE_DLL-NOTFOUND")
		message(FATAL_ERROR "PCRE DLL not found")
	endif()
	if(GRAPHITE_DLL STREQUAL "GRAPHITE_DLL-NOTFOUND")
		message(FATAL_ERROR "graphite DLL not found")
	endif()

	install(FILES
		${SDL2_DLL}
		${SDL2_IMAGE_DLL}
		${SDL2_MIXER_DLL}
		${SDL2_NET_DLL}
		${GLEW_DLL}
		${FREETYPE_DLL}
		${JPEG_DLL}
		${PNG_DLL}
		${TIFF_DLL}
		${WEBP_DLL}
		${ZLIB_DLL}
		${LZMA_DLL}
		${BZ2_DLL}
		${MAD_DLL}
		${WINPTHREAD_DLL}
		${GCC_DLL}
		${STDLIB_DLL}
		${HARFBUZZ_DLL}
		${GLIB_DLL}
		${INTL_DLL}
		${ICONV_DLL}
		${PCRE_DLL}
		${GRAPHITE_DLL}
		DESTINATION ${TA3D_BINARIES_INSTALL_PATH})
endif()

install(DIRECTORY ../../docs DESTINATION ${TA3D_RESOURCES_INSTALL_PATH} PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)
install(DIRECTORY fonts DESTINATION ${TA3D_RESOURCES_INSTALL_PATH} PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)
install(DIRECTORY gfx DESTINATION ${TA3D_RESOURCES_INSTALL_PATH} PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)
install(DIRECTORY gui DESTINATION ${TA3D_RESOURCES_INSTALL_PATH} PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)
install(DIRECTORY intro DESTINATION ${TA3D_RESOURCES_INSTALL_PATH} PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)
install(DIRECTORY languages DESTINATION ${TA3D_RESOURCES_INSTALL_PATH} PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)
install(DIRECTORY maps DESTINATION ${TA3D_RESOURCES_INSTALL_PATH} PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)
install(DIRECTORY scripts DESTINATION ${TA3D_RESOURCES_INSTALL_PATH} PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE PATTERN "__campaign_script.lua" EXCLUDE)
install(DIRECTORY shaders DESTINATION ${TA3D_RESOURCES_INSTALL_PATH} PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)
install(DIRECTORY sky DESTINATION ${TA3D_RESOURCES_INSTALL_PATH} PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)

# copy the file that will add TA3D to the desktop menu
install(FILES distrib/linux/ta3d DESTINATION share/menu)
install(FILES distrib/linux/ta3d.desktop DESTINATION share/applications)
install(FILES distrib/linux/ta3d_cfg.desktop DESTINATION share/applications)
install(FILES ota_resources_installer.sh DESTINATION ${TA3D_BINARIES_INSTALL_PATH})
install(FILES distrib/linux/ta3d_installer.desktop DESTINATION share/applications)
install(FILES ta3d.png DESTINATION ${TA3D_RESOURCES_INSTALL_PATH})
install(FILES ta3d_cfg.png DESTINATION ${TA3D_RESOURCES_INSTALL_PATH})
install(FILES readme.html DESTINATION ${TA3D_RESOURCES_INSTALL_PATH})
install(FILES readme_fr.html DESTINATION ${TA3D_RESOURCES_INSTALL_PATH})
install(FILES ../../TODO DESTINATION ${TA3D_RESOURCES_INSTALL_PATH})
install(FILES ../../AUTHORS DESTINATION ${TA3D_RESOURCES_INSTALL_PATH})
install(FILES ../../README.md DESTINATION ${TA3D_RESOURCES_INSTALL_PATH})
install(FILES ../../NEWS DESTINATION ${TA3D_RESOURCES_INSTALL_PATH})
install(FILES ta3d.mod DESTINATION ${TA3D_RESOURCES_INSTALL_PATH})
install(FILES ../../ChangeLog DESTINATION ${TA3D_RESOURCES_INSTALL_PATH})

#
# --- CPACK configuration ---
#
string(TOLOWER ${CMAKE_BUILD_TYPE} BTYPE)

if(WIN32)
	set(CPACK_GENERATOR "ZIP")
else()
	set(CPACK_GENERATOR "STGZ;DEB;RPM")
endif()

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "TA3D (Total Annihilation 3D), RTS Engine")
set(CPACK_PACKAGE_VENDOR "the TA3D team")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../../COPYING")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PACKAGE_STRING}")
set(CPACK_PACKAGE_CONTACT "${TA3D_MAIL_BUGREPORT}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Roland Brochard")
set(CPACK_DEBIAN_PACKAGE_SECTION "games")
if(NOT ${BTYPE} STREQUAL "debug")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl1.2debian, libsdl-image1.2, libsdl-mixer1.2, libsdl-net1.2, libz1, libglew1.5, libfreetype6, p7zip-full, p7zip-rar")
else()
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl1.2debian, libsdl-image1.2, libsdl-mixer1.2, libsdl-net1.2, libz1, libglew1.5, libfreetype6, p7zip-full, p7zip-rar, gdb")
endif()

if(NOT WIN32 AND UNIX)
	if(NOT ${BTYPE} STREQUAL "debug")
		set(CPACK_STRIP_FILES "ta3d")
	endif()
	set(CPACK_SOURCE_STRIP_FILES "")
endif()
set(CPACK_PACKAGE_EXECUTABLES "ta3d")
include(CPack)



#
# --- config.h ---
#
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/config.h )




# The source code
add_subdirectory(src)

