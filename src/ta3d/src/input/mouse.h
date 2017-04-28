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
#ifndef __TA3D_MOUSE_H__
#define __TA3D_MOUSE_H__

#include <gaf.h>

namespace TA3D
{
	enum MouseButtonFlag
	{
		LeftMouseButton = 1,
		RightMouseButton = 2,
		MiddleMouseButton = 4,
	};

	namespace VARS
	{
		extern int mouse_x;
		extern int mouse_y;
		extern int mouse_z;

		/**
		 * Indicates which mouse buttons are currently held down.
		 * This is a bitmask with the following flags:
		 *
		 * 1: left mouse button
		 * 2: right mouse button
		 * 4: middle mouse button
		 */
		extern int mouse_b;

		extern float fmouse_x, fmouse_y;
	}

	/*!
	** \brief poll mouse/keyboard events
	*/
	void poll_inputs();

	/*!
	** \brief draw mouse cursor at mouse position
	*/
	void draw_cursor();

	/*!
	** \brief compute cursor animation
	*/
	int anim_cursor(const int type = -1);

	/*!
	** \brief initialize mouse handler
	*/
	void init_mouse();

	/*!
	** \brief enable grabing mouse inside the window when in windowed mode
	*/
	void grab_mouse(bool);
}

extern int CURSOR_MOVE;
extern int CURSOR_GREEN;
extern int CURSOR_CROSS;
extern int CURSOR_RED;
extern int CURSOR_LOAD;
extern int CURSOR_UNLOAD;
extern int CURSOR_GUARD;
extern int CURSOR_PATROL;
extern int CURSOR_REPAIR;
extern int CURSOR_ATTACK;
extern int CURSOR_BLUE;
#define CURSOR_DEFAULT CURSOR_BLUE
extern int CURSOR_AIR_LOAD;
extern int CURSOR_BOMB_ATTACK;
extern int CURSOR_BALANCE;
extern int CURSOR_RECLAIM;
extern int CURSOR_WAIT;
extern int CURSOR_CANT_ATTACK;
extern int CURSOR_CROSS_LINK;
extern int CURSOR_CAPTURE;
extern int CURSOR_REVIVE;

extern TA3D::Gaf::AnimationList cursor;

extern int cursor_type;

#endif
