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
#ifndef __TA3D__INGAM__BATTLE_H__
#define __TA3D__INGAM__BATTLE_H__

#include <stdafx.h>
#include <misc/string.h>
#include "gamedata.h"
#include <memory>
#include <gfx/gui/area.h>
#include <gfx/texture.h>
#include <tdf.h>
#include <EngineClass.h>
#include <misc/rect.h>
#include <misc/material.light.h>
#include <scripts/script.h>
#include "fps.h"

#ifndef SCROLL_SPEED
#define SCROLL_SPEED 400.0f
#endif

namespace TA3D
{
	namespace Menus
	{
		class Loading;
	}

	class Battle
	{
		friend class CAPI;
		friend class Console;

	public:
		//! Results about the battle
		enum Result
		{
			brUnknown,
			brVictory,
			brDefeat,
			brPat
		};

		/*!
		** \brief Launch a game
		** \param g Data about the game
		** \return The result of the game
		*/
		static Result Execute(GameData* g);

	public:
		//! \name Constructors & Destructor
		//@{
		//! Constructor with a GameData
		Battle(GameData* g);
		//! Destructor
		~Battle();
		//@}

		/*!
		** \brief Launch the battle
		*/
		Result execute();

		/*!
		** \brief The result of the battle
		*/
		Result result() const { return pResult; }

		void setCameraDirection(const Vector3D& dir);

		void setShowPing(bool b) { bShowPing = b; }
		bool getShowPing() const { return bShowPing; }

		void setTimeFactor(const float f);

	private:
		/*!
		** \brief Reset the cache for the GUI name
		**
		** To avoid multiple and expensive string manipulations
		*/
		void updateCurrentGUICacheNames();

		/*!
		** \brief update camera zfar parameter
		*/
		void updateZFAR();

		//! \name Preparing all data about a battle
		//@{
		/*!
		** \brief Load Game Data
		*/
		bool loadFromGameData(GameData* g);
		//! PreInitialization
		bool initPreflight(GameData* g);
		//! Load textures
		bool initTextures();
		//! Load 3D models
		bool init3DModels();
		//! Load graphical features
		bool initGraphicalFeatures();
		//! Load Weapons
		bool initWeapons();
		//! Load units
		bool initUnits();
		//! Intermediate clean up
		bool initIntermediateCleanup();
		//! Init the engine
		bool initEngine();
		//! Add players
		bool initPlayers();
		//! Init restrictions
		bool initRestrictions();
		//! Load the GUI
		bool initGUI();
		//! Init the map
		bool initTheMap();
		//! Init the sun
		bool initTheSun();
		//! Init all textures
		bool initAllTextures();
		//! Init the camera
		bool initTheCamera();
		//! Init the wind
		bool initTheWind();
		//! Init particules
		bool initParticules();
		//! Init miscellaneous stuff
		bool initPostFlight();
		//@}

		void waitForNetworkPlayers();

		//! \name Preflight
		//@{

		/*!
		** \brief Reinit vars
		*/
		void preflightVars();

		/*!
		** \brief Change the speed and the direction of the wind
		*/
		void preflightChangeWindSpeedAndDirection();

		/*!
		** \brief Update the 3D sounds
		*/
		void preflightUpdate3DSounds();

		/**
		 * Processes inputs for the camera and updates the camera state.
		 */
		void preflightAutomaticCamera();

		/*!
		** \brief Pre Execute
		*/
		bool preExecute(LuaProgram& gameScript);
		//@}

		//! \name 2D Objects && User interaction
		//@{
		void draw2DObjects();
		void draw2DMouseUserSelection();
		//@}

		//! \name Renderer
		//@{
		void initRenderer();
		void renderScene();
		void renderStencilShadow();
		void renderWorld();
		void renderInfo();
		void renderPostEffects();
		void makePoster(int w, int h);
		//@}

		//! \name Evens in Game
		//@{
		//@}

		/**
		 * Returns the point on the map terrain that the user's cursor is hovering over.
		 *
		 * If on_mini_map is set to true,
		 * the user's cursor is assumed to be on the minimap area of the screen
		 * and the corresponding point on terrain is calculated from that.
		 */
		Vector3D cursorOnMap(const Camera& cam, MAP& map, bool on_mini_map = false);

		void handleGameStatusEvents();
		/*!
		** \brief Display some game informations (key `SPACE`)
		*/
		void showGameStatus();

		void keyArrowsNotInFreeCam();

	private:
		//! Results
		Result pResult;
		//! Data about the current game
		GameData* pGameData;

		//! Use network communications
		bool pNetworkEnabled;
		//! Host a game
		bool pNetworkIsServer;

		//! The game script
		LuaProgram game_script;

	private:
		enum CurrentGUICache
		{
			cgcDot,
			cgcShow,
			cgcHide,
			cgcEnd
		};
		//! The current GUI
		String pCurrentGUI;
		//!
		String pCurrentGUICache[cgcEnd];
		//!
		HashSet<>::Sparse toBeLoadedMenuSet;

	private:
		//!

		//!
		struct DebugInfo
		{
			//!
			LuaThread* process;
		};

	private:
		//! The area
		Gui::AREA pArea;
		//! Informations about FPS
		FPSInfos fps;
		//! The map of the game
		std::unique_ptr<MAP> map;

		//! Indicates whether the user is currently dragging out a selection box.
		bool pMouseSelecting;
		//! The bounding box of the current mouse selection (if pMouseSelecting == true)
		Rect<int> pMouseRectSelection;

		//! The sun
		HWLight pSun;

		//! \name Textures
		//@{
		//!
		Interfaces::GfxTexture pause_tex;
		//@}

		//! \name Camera
		//@{
		//!
		Camera cam;
		//!
		Vector3D cam_target;
		//! The position of the camera on the virtual "rail"
		float camera_zscroll;
		//!
		int cam_target_mx;
		//!
		int cam_target_my;
		//!
		bool cam_has_target;
		//!
		bool escMenuWasVisible;
		//! Just to see if the cam has been long enough at the default angle
		int cam_def_timer;
		/**
		 * The unit ID of the unit currently being tracked by the camera.
		 * If no unit is currently being tracked, this value is -1.
		 */
		int unitBeingTracked;
		//!
		bool last_time_activated_track_mode;

		//! Indicates whether unit health bars should be shown on the screen
		bool showHealthBars;
		//!
		float r1;
		//!
		float r2;
		//!
		float r3;
		//!
		float render_time;
		//@}

		//! \name Unknown vars
		//@{
		//!
		bool show_model;
		//!
		bool cheat_metal;
		//!
		bool cheat_energy;
		//!
		bool internal_name;
		//!
		bool internal_idx;
		//!
		bool ia_debug;
		//!
		bool view_dbg;
		//!
		bool show_mission_info;
		//!
		float show_timefactor;
		//!
		float show_gamestatus;
		//!
		int unit_info_id;
		//!
		float speed_limit;
		/**
		 * The time in (fractional) seconds that should elapse between frames.
		 * This is used to limit the number of frames rendered per second.
		 */
		float delayBetweenFrames;
		//!
		int nb_shoot;
		//!
		bool shoot;
		//!
		bool tilde;
		//!
		bool done;
		//!
		int mx;
		//!
		int my;
		//!
		int cur_sel;
		//!
		int old_gui_sel;
		//!
		bool old_sel;

		/**
		 * Indicates whether any units are currently selected.
		 * A value of true indicates that at least one unit
		 * is currently selected.
		 */
		bool selected;

		/**
		 * Indicates the type of unit (structure)
		 * that the user is currently attempting to build.
		 * If set, the user is in build mode trying to place the structure.
		 * A value of -1 indicates that the user is not in build mode.
		 */
		int build;
		//!
		bool build_order_given;
		//!
		int cur_sel_index;
		//!
		int omz;
		//!
		float cam_h;

		//! Elapsed time since the last frame in (fractional) seconds.
		float deltaTime;

		/**
		 * The current time in the game world since the simulation started, in seconds.
		 * This is the time from the perspective of units within the game world,
		 * so it doesn't include time spent with the game paused.
		 */
		float gameTime;

		/**
		 * The time in milliseconds when the last frame was processed,
		 * relative to program initialization.
		 */
		int lastFrameTime;
		//!
		int video_timer;
		//!
		bool video_shoot;
		//!
		int current_order;
		//@}

		//! \name Wind
		//@{
		//!
		float wind_t;
		//!
		bool wind_change;
		//@}

		//! \name Interface
		//@{
		//! Indicates whether the user's cursor is over a GUI element (including the minimap)
		bool IsOnGUI;
		//! Indicates whether the user's cursor is over the minimap
		bool IsOnMinimap;

		/**
		 * True if the user is currently attempting to build a structure
		 * and it is legal to build the structure at the currently hovered location.
		 */
		bool can_be_there;
		//@}

	private:
		bool pCacheShowGameStatus;

		//! \name Debug information
		DebugInfo debugInfo;

		//! pointer to loading screen
		Menus::Loading* loading;

		//! show players ping
		bool bShowPing;

	private:
		static Battle* pInstance;

	public:
		static Battle* Instance();

	private:

		/**
		 * Toggles the self-destruct order on the selected units.
		 * Triggered by the user when they issue the self-destruct command.
		 */
		void selfDestructSelectedUnits();

		/**
		 * Toggles the display of unit health bars on the screen.
		 * Triggered by the user when they press the toggle key.
		 */
		void toggleHealthBars();

		/**
		 * If the game is running below maximum speed, increases it.
		 * Triggered by the user when they press the "increase speed" key.
		 */
		void increaseGameSpeed();

		/**
		 * If the game is running above the minimum speed, decreases it.
		 * Triggered by the user when they press the "decrease speed" key.
		 */
		void decreaseGameSpeed();
	}; // class Battle

} // namespace TA3D

#endif // __TA3D__INGAM__BATTLE_H__
