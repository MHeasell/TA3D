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

#ifndef __TA3D__BASE__H
#define __TA3D__BASE__H

#include "gaf.h"				// read pictures/animations from GAF files
#include "gui.h"				// Graphical User Interface
#include "TA3D_hpi.h"			// HPI handler
#include "ingame/gamedata.h"


#define TA3D_SHIFT_PRESSED	( key[KEY_LSHIFT] || key[KEY_RSHIFT] )
#define TA3D_CTRL_PRESSED	( key[KEY_LCONTROL] || key[KEY_RCONTROL] )


namespace TA3D
{
    namespace VARS
    {
        extern SDL_Color *pal;

        extern UTILS::HPI::cHPIHandler *HPIManager;
    }
}

using namespace TA3D::VARS;



extern float	player_color[30];
extern byte		player_color_map[10];

extern int expected_players;

#ifndef TA3D_MSEC_TIMER
#define TA3D_MSEC_TIMER
#define msec_timer  (SDL_GetTicks())
#endif

extern int start;

extern int fire;
extern int build_part;

#endif
