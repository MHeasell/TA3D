/*  TA3D, a remake of Total Annihilation
	Copyright (C) 2006  Roland BROCHARD

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

/*
**  File: TA3D_NameSpace.h
** Notes:
**   Cire: The goal of this file is mainly to introduce our namespace
**           Some constants are also defined in our namespace, function
**           prototypes ect.
**         This file should be included 'second' in all other cpp files, it will
**           give cpp access to our namespace members.
*/

#ifndef __TA3D_NAMESPACE_H__
#define __TA3D_NAMESPACE_H__

#include "gfx/gfx.toolkit.h"
#include "misc/interface.h"
#include "misc/vector.h"
#include "misc/string.h"
#include "vfs/vfs.h"
#include "gfx/gfx.h"
#include "network/network.h"
#include "threads/thread.h"
#include <vector>
#include "SDL.h"

namespace TA3D
{
	struct TA3DCONFIG
	{
		float fps_limit;

		/**
		 * The speed at which the game simulation runs.
		 * A value of 1 is normal speed, but the user can adjust this
		 * to make the game progress faster.
		 */
		float timefactor; // 1.0 -> 10.0

		sint16 fsaa;		   // ?
		String Lang;		   // english, french, german, italian, spanish, japanese, chinese, ...
		sint16 anisotropy;	 // Level of anisotropy for textures (1->16)

		uint16 screen_width;
		uint16 screen_height;

		bool showfps;
		bool wireframe;
		bool height_line;
		bool fullscreen;

		bool draw_console_loading;

		String serializedGameData; // Informations about last game

		String last_MOD;
		String player_name; // Name of local player

		String skin_name; // The skin used ( empty means default )

		String net_server;		 // The server that monitor the game server list

		bool right_click_interface; // Right click interface ?

		int sound_volume; // Self explanatory
		int music_volume; // 0 - 128

		bool grab_inputs; // Capture inputs ?

		String system7zCommand; // Command used by system() to run 7zip (used to extract archives when needed by the mods manager)

		String resourcePaths; // Alternative resource paths to use

		bool developerMode; // Special mode for developers (especially mod developers)
							// which disables file caching

		int unitTextureQuality; // Maximum resolution of unit textures (0->64x64, 1->128x128, 2->256x256, 3->512x512, 4->1024x1024, 5->no limit)

		// Variables used to communicate with all the code
		bool quickrestart;				// Should be false, set to true when need to restart to enable options/parameters
		bool quickstart;				// Tell to speed up the starting process
		bool restorestart;				// Tell it's a restart to restore previous config file
		volatile bool pause;			// Tell the engine it's in pause mode, so wait for this to be false again
		volatile bool paused;			// The engine writes its current state here, used by save/load code
		String file_param;				// File parameter (command line parameter), used to give complex instructions
		volatile bool enable_shortcuts; // Tell the GUI module to react to shortcuts or not (deactivated when in chat mode)
		bool first_start;

		TA3DCONFIG();

		int getMaxTextureSizeAllowed() const;
	};

	extern SDL_Window* screen;
	extern IInterfaceManager* InterfaceManager;
	extern TA3D::GFX::Ptr gfx;

	extern SDL_Color* pal;
	extern TA3D::TA3DCONFIG* lp_CONFIG;

	extern String TA3D_CURRENT_MOD;

	template <class T>
	inline void setFlag(T& out, const int flag) { out = T(out | flag); }

	template <class T>
	inline void unsetFlag(T& out, const int flag) { out = T(out & ~flag); }

	template <class T>
	inline bool isFlagSet(const T& in, const int flag) { return (in & flag) != 0; }
} // namespace TA3D

// number of ticks/sec. to simulate
#define TICKS_PER_SEC 30

#define MILLISECONDS_SINCE_INIT SDL_GetTicks()

#endif // __TA3D_NAMESPACE_H__
