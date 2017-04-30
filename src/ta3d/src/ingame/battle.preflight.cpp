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
#include <sounds/manager.h>
#include <UnitEngine.h>
#include <input/mouse.h>
#include <input/keyboard.h>
#include <cmath>

namespace TA3D
{

	void Battle::preflightVars()
	{
		can_be_there = false;

		// Used to handle on mini map commands
		IsOnMinimap = mouse_x < 128 && mouse_y < 128;
		// Priority given to game interface
		IsOnGUI = (mouse_x < 128 && (mouse_y >= gfx->height - 64 || mouse_y < 128)) || mouse_y < 32 || mouse_y >=
																										gfx->height - 32;

		IsOnGUI |= (pArea.check() != 0);

		IsOnGUI |= mouse_x < 128; // Priority given to game interface

		if (IsOnMinimap) // Check if we can project the cursor position on the map
		{
			if (std::abs(float(mouse_x - 64) * 252.0f / 128.0f) > (float)map->mini_w * 0.5f)
				IsOnMinimap = false;
			else
			{
				if (std::abs(float(mouse_y - 64) * 252.0f / 128.0f) > (float)map->mini_h * 0.5f)
					IsOnMinimap = false;
			}
		}

		if (IsOnMinimap)
			units.pick_minimap(); // Precompute this, we'll need it

		if (video_shoot)
		{
			if (MILLISECONDS_SINCE_INIT - video_timer >= 1000 / 15)
			{
				video_timer = MILLISECONDS_SINCE_INIT;
				shoot = true;
			}
		}

		// Restore the default cursor
		cursor_type = CURSOR_DEFAULT;
	}

	void Battle::preflightChangeWindSpeedAndDirection()
	{
		wind_t = gameTime;
		map->wind += float((Math::RandomTable() % 2001) - 1000);

		if (map->wind < map->ota_data.minwindspeed)
			map->wind = (float)map->ota_data.minwindspeed;
		else
		{
			if (map->wind > map->ota_data.maxwindspeed)
				map->wind = (float)map->ota_data.maxwindspeed;
		}

		map->wind_dir += float(Math::RandomTable() % 901) * 0.1f - 45.0f;

		if (map->wind_dir < 0.0f)
			map->wind_dir += 360.0f;
		else
		{
			if (map->wind_dir >= 360.0f)
				map->wind_dir -= 360.0f;
		}

		map->wind_vec.y = 0.0f;
		map->wind_vec.x = 0.01f * map->wind * cosf(map->wind_dir * DEG2RAD);
		map->wind_vec.z = 0.01f * map->wind * sinf(map->wind_dir * DEG2RAD);
		units.set_wind_change();
	}

	void Battle::preflightUpdate3DSounds()
	{
		if (units.nb_attacked / (units.nb_attacked + units.nb_built + 1) >= 0.75f)
			sound_manager->setMusicMode(true);
		else
		{
			if (units.nb_attacked / (units.nb_attacked + units.nb_built + 1) <= 0.25f)
				sound_manager->setMusicMode(false);
		}
		sound_manager->setListenerPos(cam.rpos);
		sound_manager->update3DSound();
	}
} // namespace TA3D
