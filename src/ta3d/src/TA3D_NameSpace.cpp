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

#include "stdafx.h"
#include "TA3D_NameSpace.h"
#ifdef TA3D_PLATFORM_LINUX
#include <sys/stat.h>
#endif
#include "misc/paths.h"

namespace TA3D
{

	// global variables:
	TA3D::TA3DCONFIG::Ptr TA3D::VARS::lp_CONFIG = NULL;
	TA3D::GFX::Ptr TA3D::VARS::gfx = NULL; // The gfx object we will use to draw basic things and manage fonts, textures, ...
	SDL_Color* TA3D::VARS::pal = NULL;
	String TA3D::VARS::TA3D_CURRENT_MOD = ""; // This string stores the path to current mod
	SDL_Window* TA3D::VARS::screen = NULL;

	TA3DCONFIG::TA3DCONFIG()
	{
		unitTextureQuality = 3;

		developerMode = false;

		first_start = false;

		no_sound = false;

		resourcePaths.clear();

		system7zCommand = "7z";

		grab_inputs = false;

		sound_volume = music_volume = 128;

		anisotropy = 1;

		mouse_sensivity = 1.0f;

		right_click_interface = false;

		use_texture_compression = true;

		enable_shortcuts = true;

		net_server = TA3D_DEFAULT_SERVER_HOSTNAME;

		file_param.clear();

		use_texture_cache = false;

		pause = false;

		skin_name = "";

		camera_zoom = ZOOM_NORMAL;
		camera_def_angle = 63.44f; // TA angle
		camera_def_h = 200.0f;
		camera_zoom_speed = 1.0f;

		player_name = "player";
		serializedGameData.clear();

		draw_console_loading = false; // default set to false since it's a developer feature

		fps_limit = -1.0f;
		timefactor = 1.0f;
		priority_level = 0;
		fsaa = 0;
		Lang = "english"; // English
		screen_width = 800;
		screen_height = 600;
		showfps = false;
		wireframe = false;
		height_line = false;
		fullscreen = false; // For now, later will be true when we will reach a beta release

		last_MOD = "";

		quickrestart = false;
		quickstart = false;
		restorestart = false;
	}

	int TA3DCONFIG::getMaxTextureSizeAllowed() const
	{
		if (unitTextureQuality < 0 || unitTextureQuality >= 5)
			return 16384;
		return 64 << unitTextureQuality;
	}

} // namespace TA3D
