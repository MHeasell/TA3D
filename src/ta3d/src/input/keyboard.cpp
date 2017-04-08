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
		KeyCode asciiToKeyCode[256];
		bool keyState[MAX_KEYCODE];
		bool previousKeyState[MAX_KEYCODE];
		std::deque<KeyboardBufferItem> keyboardBuffer;
		KeyCode keyCodeMap[MAX_KEYCODE];
	}

	bool isKeyDown(KeyCode keycode)
	{
		return keyState[keycode];
	}

	bool isAsciiCharacterKeyDown(byte c)
	{
		return isKeyDown(asciiToKeyCode[c]);
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

	void clearKeyboardBuffer()
	{
		keyboardBuffer.clear();
	}

	void init_keyboard()
	{
		// Initialize the SDL Stuff
		SDL_EnableUNICODE(1);
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

		// We need some remapping hack to support some keyboards (french keyboards don't access KEY_0..9)
		memset(keyCodeMap, 0, MAX_KEYCODE * sizeof(int));

		keyCodeMap[KEY_ENTER_PAD] = KEY_ENTER;
		keyCodeMap[38] = KEY_1;
		keyCodeMap[233] = KEY_2;
		keyCodeMap[34] = KEY_3;
		keyCodeMap[39] = KEY_4;
		keyCodeMap[40] = KEY_5;
		keyCodeMap[45] = KEY_6;
		keyCodeMap[232] = KEY_7;
		keyCodeMap[95] = KEY_8;
		keyCodeMap[231] = KEY_9;
		keyCodeMap[224] = KEY_0;

		memset(VARS::keyState, 0, MAX_KEYCODE * sizeof(bool));
		keyboardBuffer.clear();

		// Initializing the ascii to scancode table
		memset(asciiToKeyCode, 0, 256 * sizeof(int));

		asciiToKeyCode[int('a')] = KEY_A;
		asciiToKeyCode[int('b')] = KEY_B;
		asciiToKeyCode[int('c')] = KEY_C;
		asciiToKeyCode[int('d')] = KEY_D;
		asciiToKeyCode[int('e')] = KEY_E;
		asciiToKeyCode[int('f')] = KEY_F;
		asciiToKeyCode[int('g')] = KEY_G;
		asciiToKeyCode[int('h')] = KEY_H;
		asciiToKeyCode[int('i')] = KEY_I;
		asciiToKeyCode[int('j')] = KEY_J;
		asciiToKeyCode[int('k')] = KEY_K;
		asciiToKeyCode[int('l')] = KEY_L;
		asciiToKeyCode[int('m')] = KEY_M;
		asciiToKeyCode[int('n')] = KEY_N;
		asciiToKeyCode[int('o')] = KEY_O;
		asciiToKeyCode[int('p')] = KEY_P;
		asciiToKeyCode[int('q')] = KEY_Q;
		asciiToKeyCode[int('r')] = KEY_R;
		asciiToKeyCode[int('s')] = KEY_S;
		asciiToKeyCode[int('t')] = KEY_T;
		asciiToKeyCode[int('u')] = KEY_U;
		asciiToKeyCode[int('v')] = KEY_V;
		asciiToKeyCode[int('w')] = KEY_W;
		asciiToKeyCode[int('x')] = KEY_X;
		asciiToKeyCode[int('y')] = KEY_Y;
		asciiToKeyCode[int('z')] = KEY_Z;

		for (int i = 0; i < 26; ++i)
			asciiToKeyCode[int('A' + i)] = asciiToKeyCode[int('a' + i)];

		asciiToKeyCode[int('0')] = KEY_0;
		asciiToKeyCode[int('1')] = KEY_1;
		asciiToKeyCode[int('2')] = KEY_2;
		asciiToKeyCode[int('3')] = KEY_3;
		asciiToKeyCode[int('4')] = KEY_4;
		asciiToKeyCode[int('5')] = KEY_5;
		asciiToKeyCode[int('6')] = KEY_6;
		asciiToKeyCode[int('7')] = KEY_7;
		asciiToKeyCode[int('8')] = KEY_8;
		asciiToKeyCode[int('9')] = KEY_9;

		asciiToKeyCode[int(' ')] = KEY_SPACE;
		asciiToKeyCode[int('\n')] = KEY_ENTER;
		asciiToKeyCode[27] = KEY_ESC;
	}

	void setKeyDown(KeyCode keycode)
	{
		if (keycode >= MAX_KEYCODE)
			return;

		if (keyCodeMap[keycode])
			VARS::keyState[keyCodeMap[keycode]] = true;
		VARS::keyState[keycode] = true;
	}

	void setKeyUp(KeyCode keycode)
	{
		if (keycode >= MAX_KEYCODE)
			return;

		if (keyCodeMap[keycode])
			VARS::keyState[keyCodeMap[keycode]] = false;
		VARS::keyState[keycode] = false;
	}

	bool didKeyGoDown(KeyCode keycode)
	{
		if (keycode >= MAX_KEYCODE)
			return false;

		if (!previousKeyState[keycode] && keyState[keycode])
		{
			previousKeyState[keycode] = true;
			return true;
		}
		previousKeyState[keycode] = keyState[keycode];
		return false;
	}

	KeyCode sdlToKeyCode(SDLKey key)
	{
		return key;
	}
} // namespace TA3D
