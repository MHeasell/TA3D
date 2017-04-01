
string(STRIP "${LIBS}" LIBS)
link_libraries("${LIBS}")

set(CMAKE_CXX_STANDARD 11)

# Yuni Settins
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Yuni_CXXFLAGS}")
link_libraries("${Yuni_LIBS}")

include_directories("../../libs/utfcpp/source")
include_directories("../../libs/fmt-3.0.1")
include_directories("../../libs/catch")

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})


#
# STDAFX
#
set(SRC_STDAFX stdafx.h)

#
#
#
set(SRC_FILEFORMATS
		gaf.cpp gaf.h)

#
# Meshs
#
set(SRC_MESHES
		mesh/mesh.cpp mesh/mesh.h
		mesh/3do.cpp mesh/3do.h
		mesh/textures.cpp mesh/textures.h
		mesh/instancing.cpp mesh/instancing.h
		mesh/joins.cpp mesh/joins.h)

set(SRC_TA3D_STRING
		ta3d_string.h ta3d_string.cpp)

#
# Logs
#
set(SRC_LOGS
		logs/logs.h logs/logs.hxx logs/logs.cpp)

#
# Languages
set(SRC_I18N
		languages/table.h languages/table.cpp
		languages/i18n.h languages/i18n.hxx languages/i18n.cpp)


#
# Threads
#
set(SRC_THREADS
		threads/mutex.h threads/mutex.cpp
		threads/thread.h threads/thread.cpp)

#
# Misc
# TODO Must of those files should be removed/cleaned
#
set(SRC_MISC
		misc/string.h misc/string.cpp
		misc/stack.hxx
		misc/progressnotifier.h
		misc/paths.h misc/paths.cpp
		misc/files.h misc/files.cpp
		misc/resources.h misc/resources.cpp
		misc/settings.h misc/settings.cpp
		misc/interface.h misc/interface.cpp
		misc/hash_table.h
		misc/osinfo.h misc/osinfo.cpp
		misc/application.h misc/application.cpp
		misc/point.h misc/rect.h
		misc/kdtree.h misc/kdtree.hxx
		misc/bvh.h misc/bvh.hxx
		misc/bitmap.h
		misc/mempool.h

		misc/vector.cpp misc/vector.h
		misc/matrix.h misc/matrix.cpp
		misc/material.light.h misc/material.light.cpp
		misc/camera.h misc/camera.cpp
		misc/recttest.h misc/recttest.cpp

		misc/usectimer.h misc/usectimer.cpp
		misc/grid.h misc/grid.hxx misc/grid.cpp
		misc/math.h misc/math.cpp
		misc/tdf.h misc/tdf.hxx misc/tdf.cpp

		misc/suspend.h misc/suspend.cpp)
		

#
# Mods
set(SRC_MODS
		mods/mods.h mods/mods.cpp
		mods/modinfo.h mods/modinfo.cpp)


#
# Scripts
#
set(SRC_SCRIPTS
		scripts/cob.cpp scripts/cob.h
		scripts/cob.vm.h scripts/cob.vm.cpp
		scripts/lua.thread.h scripts/lua.thread.cpp
		scripts/lua.env.h scripts/lua.env.cpp
		scripts/lua.chunk.h scripts/lua.chunk.hxx scripts/lua.chunk.cpp
		scripts/lua.data.h scripts/lua.data.cpp
		scripts/draw.list.h scripts/draw.list.cpp
		scripts/unit.script.h scripts/unit.script.cpp
		scripts/unit.script.func.cpp
		scripts/script.interface.h scripts/script.interface.cpp
		scripts/unit.script.interface.h scripts/unit.script.interface.cpp
		scripts/ai.script.h scripts/ai.script.cpp
		scripts/script.data.h scripts/script.data.cpp
		scripts/script.h scripts/script.cpp
		scripts/noscript.h scripts/noscript.cpp)

#
# GFX
#
set(SRC_GFX
		gfx/gfx.h gfx/gfx.cpp
		gfx/gfx.toolkit.h gfx/gfx.toolkit.cpp
		gfx/font.cpp gfx/font.h
		gfx/glfunc.h gfx/glfunc.cpp
		gfx/fx.base.h gfx/fx.base.cpp
		gfx/fx.particle.h gfx/fx.particle.cpp
		gfx/fx.electric.h gfx/fx.electric.cpp
		gfx/fx.manager.h gfx/fx.manager.cpp
		gfx/fx.h
		gfx/shader.h gfx/shader.cpp
		gfx/sky.h gfx/sky.cpp
		gfx/video.h gfx/video.cpp
		gfx/gl.extensions.h
		gfx/texture.cpp gfx/texture.h)

#
# GFX - GUI
#
set(SRC_GFX_GUI
		gfx/gui/base.h gfx/gui/base.cpp
		gfx/gui/object.h gfx/gui/object.cpp
		gfx/gui/skin.h gfx/gui/skin.cpp
		gfx/gui/skin.object.h gfx/gui/skin.object.cpp
		gfx/gui/text.color.h gfx/gui/text.color.cpp
		gfx/gui/skin.manager.h gfx/gui/skin.manager.cpp
		gfx/gui/wnd.h gfx/gui/wnd.cpp
		gfx/gui/area.h gfx/gui/area.cpp)

set(SRC_GFX_PARTICLES
		gfx/particles/particles.h gfx/particles/particle.h
		gfx/particles/particlessystem.h gfx/particles/particlessystem.cpp
		gfx/particles/particlesengine.h gfx/particles/particlesengine.cpp
)

#
# Sound
#
set(SRC_SOUND
		sounds/manager.h sounds/manager.cpp
		)


#
# Debug
#
set(SRC_DEBUG
		backtrace.cpp backtrace.h
		)


#
# Console
#
set(SRC_CONSOLE
		console/console.api.cpp console/console.api.h
		console/console.cpp console/console.h)

#
# VFS (HPI, archives)
#
set(SRC_VFS
		vfs/vfs.cpp vfs/vfs.h vfs/vfs.hxx
		vfs/archive.cpp vfs/archive.h
		vfs/hpi.cpp vfs/hpi.h
		vfs/realfs.cpp vfs/realfs.h
		vfs/file.h
		vfs/realfile.cpp vfs/realfile.h
		vfs/virtualfile.cpp vfs/virtualfile.h)


set(SRC_ENGINE
		UnitEngine.cpp UnitEngine.h
		EngineClass.cpp EngineClass.h
		engine/weapondata.h  engine/weapondata.hxx
		engine/unit.h  engine/unit.cpp engine/unit.defines.h
		engine/mission.h engine/mission.hxx engine/mission.cpp
		engine.cpp engine.h
		)


set(SRC_INPUT
		input/mouse.h input/mouse.cpp
		input/keyboard.cpp input/keyboard.h)

set(SRC_NETWORK
		network/network.cpp network/network.h
		network/socket.cpp network/socket.h
		network/socket.tcp.cpp network/socket.tcp.h
		network/socket.broadcast.cpp network/socket.broadcast.h
		network/socket.udp.cpp network/socket.udp.h
		network/ta3dsock.cpp network/ta3dsock.h
		network/networkutils.h network/networkutils.cpp
		network/socketlist.h network/socketlist.cpp
		network/TA3D_Network.cpp network/TA3D_Network.h
		network/netclient.cpp network/netclient.h network/netclient.hxx
		network/http.cpp network/http.h)

set(SRC_AI
		ai/ai.h ai/ai.hxx ai/ai.cpp
		ai/weight.h ai/weight.cpp
		ai/ai.controller.h ai/ai.controller.cpp
		ai/pathfinding.h ai/pathfinding.cpp	)


set(SRC_INGAME
		ingame/sidedata.h ingame/sidedata.cpp
		ingame/gamedata.h ingame/gamedata.cpp
		ingame/players.h ingame/players.hxx ingame/players.cpp
		ingame/battle.h ingame/battle.init.cpp
		ingame/battle.preflight.cpp ingame/battle.cpp ingame/battle.tools.cpp ingame/battle.renderer.cpp
		ingame/fps.h ingame/fps.cpp
		)

set(SRC_INGAME_WEAPONS
		ingame/weapons/weapons.h ingame/weapons/weapons.cpp
		ingame/weapons/weapons.def.h ingame/weapons/weapons.def.cpp
		ingame/weapons/weapons.ingame.h ingame/weapons/weapons.ingame.cpp
		ingame/weapons/weapons.manager.h ingame/weapons/weapons.manager.cpp
		ingame/weapons/weapons.single.h ingame/weapons/weapons.single.cpp)

set(SRC_INGAME_MENUS
		ingame/menus/base.h ingame/menus/base.cpp
		ingame/menus/splash.h ingame/menus/splash.cpp
		ingame/menus/intro.h ingame/menus/intro.cpp
		ingame/menus/loading.h ingame/menus/loading.cpp
		ingame/menus/solo.h ingame/menus/solo.cpp
		ingame/menus/mapselector.h ingame/menus/mapselector.cpp
		ingame/menus/unitselector.h ingame/menus/unitselector.cpp
		ingame/menus/statistics.h ingame/menus/statistics.cpp
		ingame/menus/mainmenu.h ingame/menus/mainmenu.cpp
		ingame/menus/netmenu.h ingame/menus/netmenu.cpp
		ingame/menus/multimenu.h ingame/menus/multimenu.cpp
		ingame/menus/campaignmainmenu.h ingame/menus/campaignmainmenu.cpp
		ingame/menus/briefscreen.h ingame/menus/briefscreen.cpp
		ingame/menus/waitroom.h ingame/menus/waitroom.cpp
		ingame/menus/networkroom.h ingame/menus/networkroom.cpp
		ingame/menus/config.h ingame/menus/config.cpp
		ingame/menus/setupgame.h ingame/menus/setupgame.cpp)



# --------------------------------------------------------------------
# --- TA3D
# --------------------------------------------------------------------
set(TA3D_LIB libta3d)
set(TA3D_BIN ta3d)
set(TA3D_TEST_BIN ta3d_test)

set(TA3D_LIB_SOURCES
		${SRC_STDAFX} ${SRC_TA3D_STRING} ${SRC_LOGS} ${SRC_I18N} ${SRC_MISC}
		${SRC_THREADS} ${SRC_DEBUG} ${SRC_CONSOLE}
		${SRC_GFX} ${SRC_GFX_GUI}
		${SRC_GFX_PARTICLES}
		${SRC_MESHES}
		${SRC_MODS}
		${SRC_SOUND}
		${SRC_SCRIPTS}
		${SRC_ENGINE}
		${SRC_INPUT}
		${SRC_FILEFORMATS}
		${SRC_VFS}
		${SRC_NETWORK}
		${SRC_AI}
		${SRC_INGAME} ${SRC_INGAME_WEAPONS} ${SRC_INGAME_MENUS}
		install.cpp
		restore.cpp restore.h
		fbi.cpp fbi.h
		tdf.cpp tdf.h tnt.cpp tnt.h
		TA3D_NameSpace.cpp TA3D_NameSpace.h ta3dbase.h
		cache.h cache.cpp
		read_file_parameter.h read_file_parameter.cpp
		sdl.h
	)

set(TA3D_SOURCES main.cpp)

set(TA3D_TEST_SOURCES test_main.cpp ta3d_string_test.cpp)






add_library(${TA3D_LIB} ${TA3D_LIB_SOURCES})

add_executable(${TA3D_BIN} ${TA3D_SOURCES})

add_executable(${TA3D_TEST_BIN} ${TA3D_TEST_SOURCES})

add_test(NAME ta3d_tests COMMAND ${TA3D_TEST_BIN})


#
# --- Internal libraries ---
#
set(LIBS  "${LIBS} ${FREETYPE_LIBRARIES}")
if(WIN32)
	set(${LIBS} "${LIBS} -llua -lgdi32 -lopengl32 -lglu32")
endif()

string(STRIP "${LIBS}" LIBS)
target_link_libraries(${TA3D_BIN} ${TA3D_LIB} fmt ${LIBS})
target_link_libraries(${TA3D_TEST_BIN} ${TA3D_LIB} fmt ${LIBS})


#
# Install
#
install(TARGETS ${TA3D_BIN} RUNTIME DESTINATION ${TA3D_BINARIES_INSTALL_PATH})

