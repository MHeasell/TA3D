
string(STRIP "${LIBS}" LIBS)
link_libraries("${LIBS}")

set(CMAKE_CXX_STANDARD 17)

include_directories("../../libs/utfcpp/source")
include_directories("../../libs/fmt-3.0.1")
include_directories("../../libs/catch")

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

# --------------------------------------------------------------------
# --- TA3D
# --------------------------------------------------------------------
set(TA3D_LIB libta3d)
set(TA3D_BIN ta3d)
set(TA3D_TEST_BIN ta3d_test)

set(TA3D_LIB_SOURCES
		EngineClass.cpp
		EngineClass.h
		SdlContextManager.cpp
		SdlContextManager.h
		TA3D_NameSpace.cpp
		TA3D_NameSpace.h
		UnitEngine.cpp
		UnitEngine.h
		ai/ai.controller.cpp
		ai/ai.controller.h
		ai/ai.cpp
		ai/ai.h
		ai/ai.hxx
		ai/pathfinding.cpp
		ai/pathfinding.h
		ai/weight.cpp
		ai/weight.h
		backtrace.cpp
		backtrace.h
		cache.cpp
		cache.h
		console/console.api.cpp
		console/console.api.h
		console/console.cpp
		console/console.h
		engine.cpp
		engine.h
		engine/mission.cpp
		engine/mission.h
		engine/unit.cpp
		engine/unit.defines.h
		engine/unit.h
		engine/weapondata.h
		engine/weapondata.hxx
		fbi.cpp
		fbi.h
		gaf.cpp
		gaf.h
		gfx/font.cpp
		gfx/font.h
		gfx/fx.base.cpp
		gfx/fx.base.h
		gfx/fx.electric.cpp
		gfx/fx.electric.h
		gfx/fx.h
		gfx/fx.manager.cpp
		gfx/fx.manager.h
		gfx/fx.particle.cpp
		gfx/fx.particle.h
		gfx/gfx.cpp
		gfx/gfx.h
		gfx/gfx.toolkit.cpp
		gfx/gfx.toolkit.h
		gfx/gl.extensions.h
		gfx/glfunc.cpp
		gfx/glfunc.h
		gfx/gui/area.cpp
		gfx/gui/area.h
		gfx/gui/base.cpp
		gfx/gui/base.h
		gfx/gui/object.cpp
		gfx/gui/object.h
		gfx/gui/skin.cpp
		gfx/gui/skin.h
		gfx/gui/skin.manager.cpp
		gfx/gui/skin.manager.h
		gfx/gui/skin.object.cpp
		gfx/gui/skin.object.h
		gfx/gui/text.color.cpp
		gfx/gui/text.color.h
		gfx/gui/wnd.cpp
		gfx/gui/wnd.h
		gfx/particles/particle.h
		gfx/particles/particles.h
		gfx/particles/particlesengine.cpp
		gfx/particles/particlesengine.h
		gfx/particles/particlessystem.cpp
		gfx/particles/particlessystem.h
		gfx/texture.cpp
		gfx/texture.h
		ingame/battle.cpp
		ingame/battle.h
		ingame/battle.init.cpp
		ingame/battle.preflight.cpp
		ingame/battle.renderer.cpp
		ingame/battle.tools.cpp
		ingame/fps.cpp
		ingame/fps.h
		ingame/gamedata.cpp
		ingame/gamedata.h
		ingame/menus/base.cpp
		ingame/menus/base.h
		ingame/menus/briefscreen.cpp
		ingame/menus/briefscreen.h
		ingame/menus/campaignmainmenu.cpp
		ingame/menus/campaignmainmenu.h
		ingame/menus/config.cpp
		ingame/menus/config.h
		ingame/menus/loading.cpp
		ingame/menus/loading.h
		ingame/menus/mainmenu.cpp
		ingame/menus/mainmenu.h
		ingame/menus/mapselector.cpp
		ingame/menus/mapselector.h
		ingame/menus/multimenu.cpp
		ingame/menus/multimenu.h
		ingame/menus/netmenu.cpp
		ingame/menus/netmenu.h
		ingame/menus/networkroom.cpp
		ingame/menus/networkroom.h
		ingame/menus/setupgame.cpp
		ingame/menus/setupgame.h
		ingame/menus/solo.cpp
		ingame/menus/solo.h
		ingame/menus/splash.cpp
		ingame/menus/splash.h
		ingame/menus/statistics.cpp
		ingame/menus/statistics.h
		ingame/menus/unitselector.cpp
		ingame/menus/unitselector.h
		ingame/menus/waitroom.cpp
		ingame/menus/waitroom.h
		ingame/players.cpp
		ingame/players.h
		ingame/sidedata.cpp
		ingame/sidedata.h
		ingame/weapons/weapons.cpp
		ingame/weapons/weapons.def.cpp
		ingame/weapons/weapons.def.h
		ingame/weapons/weapons.h
		ingame/weapons/weapons.ingame.cpp
		ingame/weapons/weapons.ingame.h
		ingame/weapons/weapons.manager.cpp
		ingame/weapons/weapons.manager.h
		ingame/weapons/weapons.single.cpp
		ingame/weapons/weapons.single.h
		input/KeyboardService.cpp
		input/KeyboardService.h
		input/keyboard.cpp
		input/keyboard.h
		input/mouse.cpp
		input/mouse.h
		install.cpp
		languages/i18n.cpp
		languages/i18n.h
		languages/table.cpp
		languages/table.h
		logs/logs.cpp
		logs/logs.h
		logs/logs.hxx
		mesh/3do.cpp
		mesh/3do.h
		mesh/instancing.cpp
		mesh/instancing.h
		mesh/mesh.cpp
		mesh/mesh.h
		mesh/textures.cpp
		mesh/textures.h
		misc/application.cpp
		misc/application.h
		misc/bitmap.h
		misc/bvh.h
		misc/bvh.hxx
		misc/camera.cpp
		misc/camera.h
		misc/files.cpp
		misc/files.h
		misc/grid.cpp
		misc/grid.h
		misc/hash_table.h
		misc/interface.cpp
		misc/interface.h
		misc/kdtree.h
		misc/kdtree.hxx
		misc/material.light.cpp
		misc/material.light.h
		misc/ta3d_math.cpp
		misc/ta3d_math.h
		misc/matrix.cpp
		misc/matrix.h
		misc/mempool.h
		misc/osinfo.cpp
		misc/osinfo.h
		misc/paths.cpp
		misc/paths.h
		misc/point.h
		misc/progressnotifier.h
		misc/rect.h
		misc/recttest.cpp
		misc/recttest.h
		misc/resources.cpp
		misc/resources.h
		misc/settings.cpp
		misc/settings.h
		misc/stack.hxx
		misc/string.cpp
		misc/string.h
		misc/suspend.cpp
		misc/suspend.h
		misc/tdf.cpp
		misc/tdf.h
		misc/tdf.hxx
		misc/usectimer.cpp
		misc/usectimer.h
		misc/vector.cpp
		misc/vector.h
		mods/modinfo.cpp
		mods/modinfo.h
		mods/mods.cpp
		mods/mods.h
		network/TA3D_Network.cpp
		network/TA3D_Network.h
		network/http.cpp
		network/http.h
		network/netclient.cpp
		network/netclient.h
		network/netclient.hxx
		network/network.cpp
		network/network.h
		network/networkutils.cpp
		network/networkutils.h
		network/socket.broadcast.cpp
		network/socket.broadcast.h
		network/socket.cpp
		network/socket.h
		network/socket.tcp.cpp
		network/socket.tcp.h
		network/socket.udp.cpp
		network/socket.udp.h
		network/socketlist.cpp
		network/socketlist.h
		network/ta3dsock.cpp
		network/ta3dsock.h
		read_file_parameter.cpp
		read_file_parameter.h
		restore.cpp
		restore.h
		scripts/ai.script.cpp
		scripts/ai.script.h
		scripts/cob.cpp
		scripts/cob.h
		scripts/cob.vm.cpp
		scripts/cob.vm.h
		scripts/draw.list.cpp
		scripts/draw.list.h
		scripts/lua.chunk.cpp
		scripts/lua.chunk.h
		scripts/lua.chunk.hxx
		scripts/lua.data.cpp
		scripts/lua.data.h
		scripts/lua.env.cpp
		scripts/lua.env.h
		scripts/lua.thread.cpp
		scripts/lua.thread.h
		scripts/noscript.cpp
		scripts/noscript.h
		scripts/script.cpp
		scripts/script.data.cpp
		scripts/script.data.h
		scripts/script.h
		scripts/script.interface.cpp
		scripts/script.interface.h
		scripts/unit.script.cpp
		scripts/unit.script.func.cpp
		scripts/unit.script.h
		scripts/unit.script.interface.cpp
		scripts/unit.script.interface.h
		sdl.h
		sounds/manager.cpp
		sounds/manager.h
		stdafx.h
		ta3d_string.cpp
		ta3d_string.h
		ta3dbase.h
		tdf.cpp
		tdf.h
		threads/mutex.cpp
		threads/mutex.h
		threads/thread.cpp
		threads/thread.h
		tnt.cpp
		tnt.h
		vfs/VfsService.cpp
		vfs/VfsService.h
		vfs/archive.cpp
		vfs/archive.h
		vfs/file.h
		vfs/hpi.cpp
		vfs/hpi.h
		vfs/realfile.cpp
		vfs/realfile.h
		vfs/realfs.cpp
		vfs/realfs.h
		vfs/vfs.cpp
		vfs/vfs.h
		vfs/virtualfile.cpp
		vfs/virtualfile.h
		input/MouseService.cpp input/MouseService.h
		misc/geometry.cpp misc/geometry.h)

set(TA3D_SOURCES main.cpp)

set(TA3D_TEST_SOURCES
		test_main.cpp
		ta3d_string_test.cpp
		SdlContextManager_test.cpp
		misc/geometry_test.cpp
		misc/vector_test.cpp misc/matrix_test.cpp misc/camera_test.cpp misc/point_test.cpp misc/ta3d_math_test.cpp misc/grid_test.cpp)

add_library(${TA3D_LIB} ${TA3D_LIB_SOURCES})
add_dependencies(${TA3D_LIB} lua)

add_executable(${TA3D_BIN} ${TA3D_SOURCES})

add_executable(${TA3D_TEST_BIN} ${TA3D_TEST_SOURCES})

add_test(NAME ta3d_tests COMMAND ${TA3D_TEST_BIN})


#
# --- Internal libraries ---
#
if(WIN32)
	set(LIBS "${LIBS} -llua -lgdi32 -lopengl32 -lglu32")
endif()

string(STRIP "${LIBS}" LIBS)
target_link_libraries(${TA3D_BIN} ${TA3D_LIB} fmt ${LIBS})
target_link_libraries(${TA3D_TEST_BIN} ${TA3D_LIB} fmt ${LIBS})


#
# Install
#
install(TARGETS ${TA3D_BIN} RUNTIME DESTINATION ${TA3D_BINARIES_INSTALL_PATH})

