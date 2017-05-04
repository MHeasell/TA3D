/*  TA3D, a remake of Total Annihilation
	Copyright (C) 2005  Roland BROCHARD

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA*/
#include "battle.h"
#include "menus/waitroom.h"
#include <network/network.h>
#include <logs/logs.h>
#include <gfx/gfx.h>
#include <TA3D_NameSpace.h>
#include <languages/i18n.h>
#include <languages/table.h>
#include <mesh/textures.h>
#include <mesh/instancing.h>
#include <mesh/mesh.h>
#include <tdf.h>
#include "weapons/weapons.h"
#include <fbi.h>
#include "players.h"
#include <UnitEngine.h>
#include <sounds/manager.h>
#include <gfx/fx.manager.h>
#include <tnt.h>
#include <misc/paths.h>
#include <misc/files.h>
#include <input/mouse.h>
#include "menus/loading.h"
#include <cache.h>
#include <memory>
#include <fstream>

namespace TA3D
{

	Battle::Result Battle::Execute(GameData* g)
	{
		Battle battle(g);
		return battle.execute();
	}

	Battle::Battle(GameData* g)
		: pResult(brUnknown),
		  pGameData(g),
		  pNetworkEnabled(false),
		  pNetworkIsServer(false),
		  map(nullptr),
		  cam(gfx->width / 2.0f, gfx->height / 2.0f),
		  escMenuWasVisible(false),
		  bShowPing(false)
	{
		LOG_INFO(LOG_PREFIX_BATTLE << "Preparing a new battle...");
		grab_mouse(lp_CONFIG->grab_inputs);
		pInstance = this;
	}

	Battle::~Battle()
	{
		pInstance = NULL;

		LOG_INFO(LOG_PREFIX_BATTLE << "Releasing unused resources...");

		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for 3d models");
		model_manager.destroy();
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for particle engine");
		particle_engine.destroy();
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for ingame units");
		units.destroy();
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for units");
		unit_manager.destroy();
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for weapons");
		weapons.destroy();
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for sound");
		sound_manager->stopMusic();
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for fx");
		fx_manager.destroy();
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for weapon manager");
		weapon_manager.destroy();
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for features");
		feature_manager.destroy();
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for ingame features");
		features.destroy();
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for textures");
		texture_manager.destroy();
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for players...");
		players.destroy();

		// Network
		if (g_ta3d_network)
		{
			LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing memory used for network object...");
			g_ta3d_network = NULL;
		}
		// Reset the VFS manager
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Reloading VFS manager...");
		VFS::Instance()->reload();

		// If we're in developer mode, clear the cache
		if (lp_CONFIG->developerMode)
			Cache::Clear(true);

		LOG_DEBUG(LOG_PREFIX_BATTLE << "Reinitializing 2D menus environment...");
		gfx->set_2D_mode();
		gfx->ReInitTexSys();
		LOG_INFO(LOG_PREFIX_BATTLE << "Done.");

		if (!lp_CONFIG->fullscreen)
			grab_mouse(false);
	}

	bool Battle::loadFromGameData(GameData* g)
	{
		pResult = brUnknown;
		if (!g)
			return true;

		loading = new Menus::Loading;
		auto spLoading = std::unique_ptr<Menus::Loading>(loading);

		pause_tex.destroy();

		// We don't want to load things we won't be able to use
		gfx->checkConfig();

		// Here we go
		uint64 startTime = MILLISECONDS_SINCE_INIT;
		uint64 timer[20];

		timer[0] = MILLISECONDS_SINCE_INIT;
		if (!initPreflight(g))
			return false;
		timer[1] = MILLISECONDS_SINCE_INIT;
		if (!initTextures())
			return false;
		timer[2] = MILLISECONDS_SINCE_INIT;
		if (!init3DModels())
			return false;
		timer[3] = MILLISECONDS_SINCE_INIT;
		if (!initGraphicalFeatures())
			return false;
		timer[4] = MILLISECONDS_SINCE_INIT;
		if (!initWeapons())
			return false;
		timer[5] = MILLISECONDS_SINCE_INIT;
		if (!initUnits())
			return false;
		timer[6] = MILLISECONDS_SINCE_INIT;
		if (!initIntermediateCleanup())
			return false;
		timer[7] = MILLISECONDS_SINCE_INIT;
		if (!initEngine())
			return false;
		timer[8] = MILLISECONDS_SINCE_INIT;
		if (!initPlayers())
			return false;
		timer[9] = MILLISECONDS_SINCE_INIT;
		if (!initRestrictions())
			return false;
		timer[10] = MILLISECONDS_SINCE_INIT;
		if (!initGUI())
			return false;
		timer[11] = MILLISECONDS_SINCE_INIT;
		if (!initTheMap())
			return false;
		timer[12] = MILLISECONDS_SINCE_INIT;
		if (!initTheSun())
			return false;
		timer[13] = MILLISECONDS_SINCE_INIT;
		if (!initAllTextures())
			return false;
		timer[14] = MILLISECONDS_SINCE_INIT;
		if (!initTheCamera())
			return false;
		timer[15] = MILLISECONDS_SINCE_INIT;
		if (!initTheWind())
			return false;
		timer[16] = MILLISECONDS_SINCE_INIT;
		if (!initParticules())
			return false;
		timer[17] = MILLISECONDS_SINCE_INIT;
		if (!initPostFlight())
			return false;
		timer[18] = MILLISECONDS_SINCE_INIT;

		unit_manager.waitUntilReady();
		timer[19] = MILLISECONDS_SINCE_INIT;

		// The loading has finished
		(*loading)(100.0f, I18N::Translate("Load finished"));
		LOG_INFO(LOG_PREFIX_BATTLE << "Loading time: " << ((float)(MILLISECONDS_SINCE_INIT - startTime) * 0.001f) << " sec.");
#define TA3D_LOADING_STATS
#ifdef TA3D_LOADING_STATS
		LOG_INFO(LOG_PREFIX_BATTLE << "statistics:");
		const char* functionName[] = {
			"initPreflight(g)",
			"initTextures()",
			"init3DModels()",
			"initGraphicalFeatures()",
			"initWeapons()",
			"initUnits()",
			"initIntermediateCleanup()",
			"initEngine()",
			"initPlayers()",
			"initRestrictions()",
			"initGUI()",
			"initTheMap()",
			"initTheSun()",
			"initAllTextures()",
			"initTheCamera()",
			"initTheWind()",
			"initParticules()",
			"initTheWater()",
			"initPostFlight()",
			"waitUntilReady()"};
		for (int i = 0; i < 19; ++i)
			LOG_INFO(LOG_PREFIX_BATTLE << functionName[i] << " done in " << timer[i + 1] - timer[i] << " msec.");
#endif

		TranslationTable::Update();

		return true;
	}

	bool Battle::initPreflight(GameData* g)
	{
		gfx->unset_2D_mode();
		// The game data
		pGameData = g;
		// Network
		pNetworkEnabled = network_manager.isConnected();
		pNetworkIsServer = network_manager.isServer();

		// To have mouse sensibility undependent from the resolution
		gfx->SCREEN_W_TO_640 = 1.0f;
		gfx->SCREEN_H_TO_480 = 1.0f;

		// The GUI
		pCurrentGUI.clear();
		updateCurrentGUICacheNames();

		// FPS
		fps.countSinceLastTime = 0;
		fps.average = 0;
		fps.lastTime = MILLISECONDS_SINCE_INIT;
		fps.toStr.clear();

		// Misc
		pMouseRectSelection.reset();
		pMouseSelecting = false;
		pCacheShowGameStatus = false;

		return true;
	}

	bool Battle::initTextures()
	{
		LOG_INFO(LOG_PREFIX_BATTLE << "Loading textures...");
		(*loading)(0.0f, I18N::Translate("Loading textures"));
		texture_manager.all_texture();
		return true;
	}

	bool Battle::init3DModels()
	{
		LOG_INFO(LOG_PREFIX_BATTLE << "Loading 3D Models...");
		(*loading)(100.0f / 7.0f, I18N::Translate("Loading 3D Models"));
		model_manager.init();
		model_manager.load_all(loading);
		return true;
	}

	bool Battle::initGraphicalFeatures()
	{
		LOG_INFO(LOG_PREFIX_BATTLE << "Loading graphical features...");
		(*loading)(200.0f / 7.0f, I18N::Translate("Loading graphical features"));
		load_features(loading);
		feature_manager.clean();
		model_manager.compute_ids();
		return true;
	}

	bool Battle::initWeapons()
	{
		LOG_INFO(LOG_PREFIX_BATTLE << "Loading weapons...");
		(*loading)(250.0f / 7.0f, I18N::Translate("Loading weapons"));
		load_weapons(loading);
		weapons.init();
		return true;
	}

	bool Battle::initUnits()
	{
		LOG_INFO(LOG_PREFIX_BATTLE << "Loading units...");
		(*loading)(300.0f / 7.0f, I18N::Translate("Loading units"));
		unit_manager.load_all_units(loading);
		return true;
	}

	bool Battle::initIntermediateCleanup()
	{
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Freeing unused memory");
		(*loading)(400.0f / 7.0f, I18N::Translate("Free unused memory"));
		texture_manager.destroy();
		return true;
	}

	bool Battle::initEngine()
	{
		LOG_INFO(LOG_PREFIX_BATTLE << "Initializing the engine...");
		(*loading)(500.0f / 7.0f, I18N::Translate("Initialising engine"));
		gfx->SetDefState();
		particle_engine.init();
		return true;
	}

	bool Battle::initPlayers()
	{
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Adding players...");
		players.init(); // Object containing data about players
		for (int i = 0; i < pGameData->nb_players; ++i)
		{
			players.add(pGameData->player_names[i], pGameData->player_sides[i],
				pGameData->player_control[i],
				pGameData->energy[i], pGameData->metal[i],
				pGameData->ai_level[i], pGameData->team[i]); // add a player
		}

		if (players.local_human_id >= 0)
		{
			String intgaf;
			for (int i = 0; i < ta3dSideData.nb_side; ++i)
			{
				if (ta3dSideData.side_name[i] == pGameData->player_sides[players.local_human_id])
				{
					intgaf = ta3dSideData.side_int[i];
					break;
				}
			}
			if (!intgaf.empty())
				unit_manager.load_panel_texture(intgaf);
		}
		TA3D::MAX_UNIT_PER_PLAYER = pGameData->max_unit_per_player;
		units.init(true);
		return true;
	}

	bool Battle::initRestrictions()
	{
		if (!pGameData->use_only.empty()) // We are told not to use all units !!
		{
			LOG_DEBUG(LOG_PREFIX_BATTLE << "Loading restrictions...");
			TDFParser useonly_parser(pGameData->use_only, false, false, true); // In gadgets mode so we can read the special key :)
			for (int i = 0; i < unit_manager.nb_unit; i++)
				unit_manager.unit_type[i]->not_used = true;
			String unit_name;
			int i = 0;
			while (!(unit_name = useonly_parser.pullAsString(String("gadget") << i)).empty())
			{
				int idx = unit_manager.get_unit_index(unit_name);
				if (idx >= 0)
					unit_manager.unit_type[idx]->not_used = false;
				++i;
			}
		}
		return true;
	}

	bool Battle::initGUI()
	{
		LOG_INFO(LOG_PREFIX_BATTLE << "Loading the GUI...");
		(*loading)(550.0f / 7.0f, I18N::Translate("Loading GUI"));
		pArea.load_tdf("gui/game.area");

		try
		{
			pArea.load_window(String(ta3dSideData.guis_dir) << ta3dSideData.side_pref[players.side_view] << "gen.gui"); // Load the order interface
			pArea.msg(String(ta3dSideData.side_pref[players.side_view]) << "gen.hide");									// Hide it
			pArea.msg(String(ta3dSideData.side_pref[players.side_view]) << "gen.enableScrolling");						// Enable scrolling
		}
		catch (...)
		{
			LOG_WARNING(LOG_PREFIX_BATTLE << "`gen.gui` is missing or can not be loaded");
		}

		try
		{
			pArea.load_window(String(ta3dSideData.guis_dir) << ta3dSideData.side_pref[players.side_view] << "dl.gui"); // Load the default build interface
			pArea.msg(String(ta3dSideData.side_pref[players.side_view]) << "dl.hide");								   // Hide it
			pArea.msg(String(ta3dSideData.side_pref[players.side_view]) << "dl.enableScrolling");					   // Enable scrolling
		}
		catch (...)
		{
			LOG_WARNING(LOG_PREFIX_BATTLE << "`dl.gui` is missing or can not be loaded");
		}

		const String sideName = ToLower(ta3dSideData.side_name[players.side_view]);
		for (int i = 0; i < unit_manager.nb_unit; ++i)
		{
			if (!(i & 0xF))
				(*loading)((550.0f + 50.0f * (float)i / float(unit_manager.nb_unit + 1)) / 7.0f, I18N::Translate("Loading GUI"));
			if (ToLower(unit_manager.unit_type[i]->side) == sideName)
			{
				int e(1);
				while (VFS::Instance()->fileExists(String(ta3dSideData.guis_dir) << unit_manager.unit_type[i]->Unitname << e << ".gui"))
				{
					toBeLoadedMenuSet.insert(ToLower(String(ta3dSideData.guis_dir) << unit_manager.unit_type[i]->Unitname << e << ".gui"));
					++e;
				}
			}
		}

		for (unsigned int i = 0; i < players.count(); ++i)
		{
			Gui::GUIOBJ::Ptr obj = pArea.get_object(String("playerstats.team") << i);
			if (obj)
			{
				obj->current_state = (byte)TA3D::Math::Log2(players.team[i]);
				obj->Flag &= ~FLAG_HIDDEN;
				obj->Flag &= ~FLAG_CAN_BE_CLICKED;
			}
		}
		return true;
	}

	bool Battle::initTheMap()
	{
		LOG_INFO(LOG_PREFIX_BATTLE << "Loading the map...");
		(*loading)(600.0f / 7.0f, I18N::Translate("Loading the map"));
		LOG_DEBUG(LOG_PREFIX_BATTLE << "Extracting `" << pGameData->map_filename << "`...");

		File* map_file = VFS::Instance()->readFile(pGameData->map_filename);
		if (!map_file)
			return false;
		LOG_DEBUG(LOG_PREFIX_BATTLE << "`" << pGameData->map_filename << "` extracted");
		LOG_DEBUG(LOG_PREFIX_BATTLE << "loading map data ...");
		map.reset(load_tnt_map(map_file));
		LOG_DEBUG(LOG_PREFIX_BATTLE << "map data loaded");
		delete map_file;

		LOG_INFO(LOG_PREFIX_BATTLE << "Initialising the Fog Of War...");
		map->clear_FOW(pGameData->fog_of_war);

		units.map = map.get(); // Setup some useful information

		pGameData->map_filename = Paths::Files::ReplaceExtension(pGameData->map_filename, ".ota");

		LOG_DEBUG(LOG_PREFIX_BATTLE << "Extracting `" << pGameData->map_filename << "`...");
		map_file = VFS::Instance()->readFile(pGameData->map_filename);
		if (map_file)
		{
			LOG_INFO(LOG_PREFIX_BATTLE << "Loading map informations...");
			map->ota_data.load(map_file);

			delete map_file;
		}
		pGameData->map_filename = Paths::Files::ReplaceExtension(pGameData->map_filename, "");

		LOG_INFO(LOG_PREFIX_BATTLE << "Computing walkable areas...");
		(*loading)(650.0f / 7.0f, I18N::Translate("Computing walkable areas"));
		Pathfinder::instance()->computeWalkableAreas();

		return true;
	}

	bool Battle::initTheSun()
	{
		pSun.Att = 0.0f;
		// Direction
		pSun.Dir.x = -1.0f;
		pSun.Dir.y = 2.0f;
		pSun.Dir.z = 1.0f;
		pSun.Dir.normalize();
		// Lights
		pSun.LightAmbient[0] = 0.25f;
		pSun.LightAmbient[1] = 0.25f;
		pSun.LightAmbient[2] = 0.25f;
		pSun.LightAmbient[3] = 0.25f;
		pSun.LightDiffuse[0] = 1.0f;
		pSun.LightDiffuse[1] = 1.0f;
		pSun.LightDiffuse[2] = 1.0f;
		pSun.LightDiffuse[3] = 1.0f;
		pSun.LightSpecular[0] = 0.0f;
		pSun.LightSpecular[1] = 0.0f;
		pSun.LightSpecular[2] = 0.0f;
		pSun.LightSpecular[3] = 0.0f;
		// Direction
		pSun.Directionnal = true;
		return true;
	}

	bool Battle::initAllTextures()
	{
		pause_tex.load("gfx/pause.png");
		return true;
	}

	bool Battle::initTheCamera()
	{
		Camera::inGame = &cam;
		return true;
	}

	bool Battle::initTheWind()
	{
		wind_t = 0.0f; // To handle wind variations
		wind_change = false;
		if (map->ota_data.maxwindspeed != map->ota_data.minwindspeed)
			map->wind = Math::randomFloat(map->ota_data.minwindspeed, map->ota_data.maxwindspeed);
		return true;
	}

	bool Battle::initParticules()
	{
		fire = particle_engine.addtex("gfx/fire.tga");
		build_part = particle_engine.addtex("gfx/part.tga");
		fx_manager.loadData();
		return true;
	}

	bool Battle::initPostFlight()
	{
		deltaTime = 0.0f;
		gameTime = 0.0f;
		lastFrameTime = MILLISECONDS_SINCE_INIT;

		mx = my = 0;
		cur_sel = -1;
		old_gui_sel = -1;
		old_sel = false;
		selected = false;
		build = -1;
		build_order_given = false;
		cur_sel_index = -1;

		speed_limit = lp_CONFIG->fps_limit;
		delayBetweenFrames = (speed_limit < 1.0f) ? 0.0f : (1.0f / speed_limit);
		done = false;

		show_model = false; // Affichage les noms des sous objets du modèle 3D de l'unité sélectionnée
		cheat_metal = false;
		cheat_energy = false;
		internal_name = false;
		internal_idx = false;
		ia_debug = false;
		view_dbg = false;
		show_mission_info = false;
		show_timefactor = 0.0f;
		show_gamestatus = 0.0f;
		unit_info_id = -1;

		lp_CONFIG->pause = false;
		video_timer = MILLISECONDS_SINCE_INIT; // To handle video
		video_shoot = false;
		current_order = SIGNAL_ORDER_NONE;

		// Interface
		IsOnGUI = false;
		IsOnMinimap = false;
		can_be_there = false;

		// Detect current screen shot number
		nb_shoot = -1;
		shoot = false;
		do
		{
			nb_shoot = (nb_shoot + 1) % 1000000;
		} while (TA3D::Paths::Exists(String(TA3D::Paths::Screenshots) << String().format("ta3d-shoot%.6d.tga", nb_shoot)) && nb_shoot != 999999);

		return true;
	}

	void Battle::updateCurrentGUICacheNames()
	{
		// Reset
		for (int i = 0; i < cgcEnd; ++i)
			pCurrentGUICache[i] = pCurrentGUI;
		// Each item
		pCurrentGUICache[cgcDot] << ".";
		pCurrentGUICache[cgcShow] << ".show";
		pCurrentGUICache[cgcHide] << ".hide";
	}

	void Battle::waitForNetworkPlayers()
	{
		g_ta3d_network = TA3DNetwork::Ptr(new TA3DNetwork(&pArea, pGameData));
		if (pNetworkEnabled)
		{
			players.set_network(g_ta3d_network);
			if (!network_manager.isServer()) // Only server is able to save a game
				pArea.msg("esc_menu.b_save.disable");
		}
		sound_manager->playMusic();
		Menus::WaitRoom::Execute(pGameData);
	}

} // namespace TA3D
