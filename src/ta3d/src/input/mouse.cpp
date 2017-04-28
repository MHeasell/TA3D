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

#include <stdafx.h>
#include <TA3D_NameSpace.h>
#include "mouse.h"
#include "keyboard.h"
#include "../misc/math.h"

int TA3D::mouse_x = 0;
int TA3D::mouse_y = 0;
int TA3D::mouse_z = 0;
int TA3D::mouse_b = 0;
int TA3D::previousMouseState = 0;
int CURSOR_MOVE;
int CURSOR_GREEN;
int CURSOR_CROSS;
int CURSOR_RED;
int CURSOR_LOAD;
int CURSOR_UNLOAD;
int CURSOR_GUARD;
int CURSOR_PATROL;
int CURSOR_REPAIR;
int CURSOR_ATTACK;
int CURSOR_BLUE;
int CURSOR_AIR_LOAD;
int CURSOR_BOMB_ATTACK;
int CURSOR_BALANCE;
int CURSOR_RECLAIM;
int CURSOR_WAIT;
int CURSOR_CANT_ATTACK;
int CURSOR_CROSS_LINK;
int CURSOR_CAPTURE;
int CURSOR_REVIVE;

TA3D::Gaf::AnimationList cursor;

int cursor_type = CURSOR_DEFAULT;

namespace TA3D
{
	void poll_inputs()
	{
		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
				{
					SDL_Keycode codePoint = event.key.keysym.sym;
					KeyCode keyCode = sdlToKeyCode(event.key.keysym.sym);

					setKeyDown(keyCode);

					if (keyCode == KEY_ENTER_PAD)
					{
						keyCode = KEY_ENTER;
						codePoint = '\n';
					}

					// Keycodes for keys with printable chraacters
					// are represented by their character byte.
					// Other keycodes use their scancode ORed with (1 << 30).
					// We want to filter out these non-printable keys.
					if ((codePoint & (1 << 30)) == 0) {
						appendKeyboardBufferElement(keyCode, CodePoint(codePoint));
					}
				}
				break;
				case SDL_MOUSEWHEEL:
				{
					mouse_z += event.wheel.y;
				}
				break;
				case SDL_KEYUP:
					setKeyUp(event.key.keysym.sym);
					break;
			};
		}

		int newMouseX;
		int newMouseY;
		uint32 mouseButtonState = SDL_GetMouseState(&newMouseX, &newMouseY);

		// update mouse button state
		previousMouseState = mouse_b;
		mouse_b = 0;
		if (mouseButtonState & SDL_BUTTON(SDL_BUTTON_LEFT))  // left mouse button
			mouse_b |= LeftMouseButton;
		if (mouseButtonState & SDL_BUTTON(SDL_BUTTON_RIGHT))  // right mouse button
			mouse_b |= RightMouseButton;
		if (mouseButtonState & SDL_BUTTON(SDL_BUTTON_MIDDLE))  // middle mouse button
			mouse_b |= MiddleMouseButton;

		// update mouse position
		mouse_x = newMouseX;
		mouse_y = newMouseY;

		// handle ALT-TAB
		if (lp_CONFIG->fullscreen && isKeyDown(KEY_ALT) && isKeyDown(KEY_TAB) && (SDL_GetWindowFlags(screen) & SDL_WINDOW_SHOWN))
			SDL_MinimizeWindow(screen);
	}

	static uint32 start = 0;

	int anim_cursor(const int type)
	{
		return (type < 0)
			? ((MILLISECONDS_SINCE_INIT - start) / 100) % cursor[cursor_type].nb_bmp
			: ((MILLISECONDS_SINCE_INIT - start) / 100) % cursor[type].nb_bmp;
	}

	void draw_cursor()
	{
		int curseur = anim_cursor();
		if (curseur < 0 || curseur >= cursor[cursor_type].nb_bmp)
		{
			curseur = 0;
			start = MILLISECONDS_SINCE_INIT;
		}
		int dx = cursor[cursor_type].ofs_x[curseur];
		int dy = cursor[cursor_type].ofs_y[curseur];
		int sx = cursor[cursor_type].bmp[curseur]->w;
		int sy = cursor[cursor_type].bmp[curseur]->h;
		gfx->set_color(0xFFFFFFFF);
		gfx->set_alpha_blending();
		gfx->drawtexture(cursor[cursor_type].glbmp[curseur],
			float(mouse_x - dx),
			float(mouse_y - dy),
			float(mouse_x - dx + sx),
			float(mouse_y - dy + sy));
		gfx->unset_alpha_blending();
	}

	void init_mouse()
	{
		grab_mouse(lp_CONFIG->grab_inputs);

		mouse_b = 0;
		previousMouseState = 0;

		SDL_ShowCursor(SDL_DISABLE);

		// Loading and creating cursors
		File* file = VFS::Instance()->readFile("anims\\cursors.gaf"); // Load cursors
		if (file)
		{
			cursor.loadGAFFromRawData(file, false);
			cursor.convert(false, false);

			CURSOR_MOVE = cursor.findByName("cursormove"); // Match cursor variables with cursor anims
			CURSOR_GREEN = cursor.findByName("cursorgrn");
			CURSOR_CROSS = cursor.findByName("cursorselect");
			CURSOR_RED = cursor.findByName("cursorred");
			CURSOR_LOAD = cursor.findByName("cursorload");
			CURSOR_UNLOAD = cursor.findByName("cursorunload");
			CURSOR_GUARD = cursor.findByName("cursordefend");
			CURSOR_PATROL = cursor.findByName("cursorpatrol");
			CURSOR_REPAIR = cursor.findByName("cursorrepair");
			CURSOR_ATTACK = cursor.findByName("cursorattack");
			CURSOR_BLUE = cursor.findByName("cursornormal");
			CURSOR_AIR_LOAD = cursor.findByName("cursorpickup");
			CURSOR_BOMB_ATTACK = cursor.findByName("cursorairstrike");
			CURSOR_BALANCE = cursor.findByName("cursorunload");
			CURSOR_RECLAIM = cursor.findByName("cursorreclamate");
			CURSOR_WAIT = cursor.findByName("cursorhourglass");
			CURSOR_CANT_ATTACK = cursor.findByName("cursortoofar");
			CURSOR_CROSS_LINK = cursor.findByName("pathicon");
			CURSOR_CAPTURE = cursor.findByName("cursorcapture");
			CURSOR_REVIVE = cursor.findByName("cursorrevive");
			if (CURSOR_REVIVE == -1) // If you don't have the required cursors, then resurrection won't work
				CURSOR_REVIVE = cursor.findByName("cursorreclamate");
			delete file;
		}
		else
		{
			LOG_DEBUG("RESOURCES ERROR : (anims\\cursors.gaf not found)");
			exit(2);
		}
		cursor_type = CURSOR_DEFAULT;
	}

	void grab_mouse(bool grab)
	{
		SDL_SetWindowGrab(screen, grab ? SDL_TRUE : SDL_FALSE);
	}

	bool isMouseButtonDown(MouseButtonFlag button)
	{
		return (mouse_b & button) != 0;
	}

	bool isMouseButtonUp(MouseButtonFlag button)
	{
		return (mouse_b & button) == 0;
	}

	bool didMouseButtonGoDown(MouseButtonFlag button)
	{
		return (mouse_b & button) && !(previousMouseState & button);
	}

	bool didMouseButtonGoUp(MouseButtonFlag button)
	{
		return (previousMouseState & button) && !(mouse_b & button);
	}
}
