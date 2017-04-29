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

namespace TA3D
{
	KeyCode asciiToKeyCode[256];
	std::unordered_set<KeyCode> keyState;
	std::unordered_set<KeyCode> previousKeyState;
	std::deque<KeyboardBufferItem> keyboardBuffer;
	std::unordered_map<KeyCode, KeyCode> keyCodeMap;

	bool isKeyDown(KeyCode keycode)
	{
		return keyState.find(keycode) != keyState.end();
	}

	bool isShiftKeyDown()
	{
		return isKeyDown(KEY_LSHIFT) || isKeyDown(KEY_RSHIFT);
	}

	bool isControlKeyDown()
	{
		return isKeyDown(KEY_LCONTROL) || isKeyDown(KEY_RCONTROL);
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

	void appendKeyboardBufferElement(KeyCode keyCode, CodePoint codePoint)
	{
		keyboardBuffer.push_back(KeyboardBufferItem(keyCode, codePoint));
	}

	void clearKeyboardBuffer()
	{
		keyboardBuffer.clear();
	}

	void initializeKeyboard()
	{
		// We need some remapping hack to support some keyboards (french keyboards don't access KEY_0..9)
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

		keyboardBuffer.clear();

		// Initializing the ascii to scancode table
		memset(asciiToKeyCode, 0, 256 * sizeof(KeyCode));

		asciiToKeyCode['a'] = KEY_A;
		asciiToKeyCode['b'] = KEY_B;
		asciiToKeyCode['c'] = KEY_C;
		asciiToKeyCode['d'] = KEY_D;
		asciiToKeyCode['e'] = KEY_E;
		asciiToKeyCode['f'] = KEY_F;
		asciiToKeyCode['g'] = KEY_G;
		asciiToKeyCode['h'] = KEY_H;
		asciiToKeyCode['i'] = KEY_I;
		asciiToKeyCode['j'] = KEY_J;
		asciiToKeyCode['k'] = KEY_K;
		asciiToKeyCode['l'] = KEY_L;
		asciiToKeyCode['m'] = KEY_M;
		asciiToKeyCode['n'] = KEY_N;
		asciiToKeyCode['o'] = KEY_O;
		asciiToKeyCode['p'] = KEY_P;
		asciiToKeyCode['q'] = KEY_Q;
		asciiToKeyCode['r'] = KEY_R;
		asciiToKeyCode['s'] = KEY_S;
		asciiToKeyCode['t'] = KEY_T;
		asciiToKeyCode['u'] = KEY_U;
		asciiToKeyCode['v'] = KEY_V;
		asciiToKeyCode['w'] = KEY_W;
		asciiToKeyCode['x'] = KEY_X;
		asciiToKeyCode['y'] = KEY_Y;
		asciiToKeyCode['z'] = KEY_Z;

		for (int i = 0; i < 26; ++i)
			asciiToKeyCode['A' + i] = asciiToKeyCode['a' + i];

		asciiToKeyCode['0'] = KEY_0;
		asciiToKeyCode['1'] = KEY_1;
		asciiToKeyCode['2'] = KEY_2;
		asciiToKeyCode['3'] = KEY_3;
		asciiToKeyCode['4'] = KEY_4;
		asciiToKeyCode['5'] = KEY_5;
		asciiToKeyCode['6'] = KEY_6;
		asciiToKeyCode['7'] = KEY_7;
		asciiToKeyCode['8'] = KEY_8;
		asciiToKeyCode['9'] = KEY_9;

		asciiToKeyCode[' '] = KEY_SPACE;
		asciiToKeyCode['\n'] = KEY_ENTER;
		asciiToKeyCode[27] = KEY_ESC;
	}

	void setKeyDown(KeyCode keycode)
	{
		auto mappedKeyCode = keyCodeMap.find(keycode);
		if (mappedKeyCode != keyCodeMap.end())
		{
			keyState.insert(mappedKeyCode->second);
		}

		keyState.insert(keycode);
	}

	void setKeyUp(KeyCode keycode)
	{
		auto mappedKeyCode = keyCodeMap.find(keycode);
		if (mappedKeyCode != keyCodeMap.end())
		{
			keyState.erase(mappedKeyCode->second);
		}

		keyState.erase(keycode);
	}

	bool didKeyGoDown(KeyCode keycode)
	{
		bool isDown = keyState.find(keycode) != keyState.end();
		bool wasDown = previousKeyState.find(keycode) != previousKeyState.end();

		if (isDown && !wasDown)
		{
			previousKeyState.insert(keycode);
			return true;
		}

		if (!isDown && wasDown) {
			previousKeyState.erase(keycode);
			return false;
		}

		return false;
	}

	KeyCode sdlToKeyCode(SDL_Keycode key)
	{
		return key;
	}
} // namespace TA3D
