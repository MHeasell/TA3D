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
#include "keyboard.h"
#include "mouse.h"

using namespace TA3D::VARS;

namespace TA3D
{
	namespace VARS
	{
		KeyCode ascii_to_scancode[256];
		bool key[MAX_KEYCODE];
		bool prevkey_down[MAX_KEYCODE];
		std::deque<KeyboardBufferItem> keyboardBuffer;
		int remap[MAX_KEYCODE];
	}

	bool isKeyDown(KeyCode keycode)
	{
		return key[keycode];
	}

	bool isAsciiCharacterKeyDown(byte c)
	{
		return isKeyDown(ascii_to_scancode[c]);
	}

	KeyboardBufferItem getNextKeyboardBufferElement()
	{
		if (keyboardBuffer.empty())
		{
			return KeyboardBufferItem(KEY_UNKNOWN, 0);
		}

		auto element = keyboardBuffer.front();
		keyboardBuffer.pop_front();
		return element;
	}

	bool keyboardBufferContainsElements()
	{
		return !keyboardBuffer.empty();
	}

	void appendKeyboardBufferElement(KeyCode keyCode, uint16 codePoint)
	{
		keyboardBuffer.push_back(KeyboardBufferItem(keyCode, codePoint));
	}

	void clear_keybuf()
	{
		keyboardBuffer.clear();
	}

	void init_keyboard()
	{
		// Initialize the SDL Stuff
		SDL_EnableUNICODE(1);
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

		// We need some remapping hack to support some keyboards (french keyboards don't access KEY_0..9)
		memset(remap, 0, MAX_KEYCODE * sizeof(int));

		remap[KEY_ENTER_PAD] = KEY_ENTER;
		remap[38] = KEY_1;
		remap[233] = KEY_2;
		remap[34] = KEY_3;
		remap[39] = KEY_4;
		remap[40] = KEY_5;
		remap[45] = KEY_6;
		remap[232] = KEY_7;
		remap[95] = KEY_8;
		remap[231] = KEY_9;
		remap[224] = KEY_0;

		memset(VARS::key, 0, MAX_KEYCODE * sizeof(bool));
		keyboardBuffer.clear();

		// Initializing the ascii to scancode table
		memset(ascii_to_scancode, 0, 256 * sizeof(int));

		ascii_to_scancode[int('a')] = KEY_A;
		ascii_to_scancode[int('b')] = KEY_B;
		ascii_to_scancode[int('c')] = KEY_C;
		ascii_to_scancode[int('d')] = KEY_D;
		ascii_to_scancode[int('e')] = KEY_E;
		ascii_to_scancode[int('f')] = KEY_F;
		ascii_to_scancode[int('g')] = KEY_G;
		ascii_to_scancode[int('h')] = KEY_H;
		ascii_to_scancode[int('i')] = KEY_I;
		ascii_to_scancode[int('j')] = KEY_J;
		ascii_to_scancode[int('k')] = KEY_K;
		ascii_to_scancode[int('l')] = KEY_L;
		ascii_to_scancode[int('m')] = KEY_M;
		ascii_to_scancode[int('n')] = KEY_N;
		ascii_to_scancode[int('o')] = KEY_O;
		ascii_to_scancode[int('p')] = KEY_P;
		ascii_to_scancode[int('q')] = KEY_Q;
		ascii_to_scancode[int('r')] = KEY_R;
		ascii_to_scancode[int('s')] = KEY_S;
		ascii_to_scancode[int('t')] = KEY_T;
		ascii_to_scancode[int('u')] = KEY_U;
		ascii_to_scancode[int('v')] = KEY_V;
		ascii_to_scancode[int('w')] = KEY_W;
		ascii_to_scancode[int('x')] = KEY_X;
		ascii_to_scancode[int('y')] = KEY_Y;
		ascii_to_scancode[int('z')] = KEY_Z;

		for (int i = 0; i < 26; ++i)
			ascii_to_scancode[int('A' + i)] = ascii_to_scancode[int('a' + i)];

		ascii_to_scancode[int('0')] = KEY_0;
		ascii_to_scancode[int('1')] = KEY_1;
		ascii_to_scancode[int('2')] = KEY_2;
		ascii_to_scancode[int('3')] = KEY_3;
		ascii_to_scancode[int('4')] = KEY_4;
		ascii_to_scancode[int('5')] = KEY_5;
		ascii_to_scancode[int('6')] = KEY_6;
		ascii_to_scancode[int('7')] = KEY_7;
		ascii_to_scancode[int('8')] = KEY_8;
		ascii_to_scancode[int('9')] = KEY_9;

		ascii_to_scancode[int(' ')] = KEY_SPACE;
		ascii_to_scancode[int('\n')] = KEY_ENTER;
		ascii_to_scancode[27] = KEY_ESC;
	}

	void setKeyDown(KeyCode keycode)
	{
		if (keycode >= MAX_KEYCODE)
			return;

		if (remap[keycode])
			VARS::key[remap[keycode]] = true;
		VARS::key[keycode] = true;
	}

	void setKeyUp(KeyCode keycode)
	{
		if (keycode >= MAX_KEYCODE)
			return;

		if (remap[keycode])
			VARS::key[remap[keycode]] = false;
		VARS::key[keycode] = false;
	}

	bool didKeyGoDown(KeyCode keycode)
	{
		if (keycode >= MAX_KEYCODE)
			return false;

		if (!prevkey_down[keycode] && key[keycode])
		{
			prevkey_down[keycode] = true;
			return true;
		}
		prevkey_down[keycode] = key[keycode];
		return false;
	}

	KeyCode sdlToKeyCode(SDLKey key)
	{
		return key;
	}
} // namespace TA3D
