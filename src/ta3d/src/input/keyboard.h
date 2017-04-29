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

#ifndef __TA3D_KEYBOARD_H__
#define __TA3D_KEYBOARD_H__

#include <deque>
#include <stdafx.h>
#include <SDL_keycode.h>
#include <unordered_set>
#include <unordered_map>

namespace TA3D
{
	typedef SDL_Keycode KeyCode;

	struct KeyboardBufferItem {
		KeyCode keyCode;
		CodePoint codePoint;

		KeyboardBufferItem(KeyCode keyCode, CodePoint codePoint): keyCode(keyCode), codePoint(codePoint) {}
	};

	/**
	 * Mapping of ASCII characters to key codes.
	 */
	extern KeyCode asciiToKeyCode[256];

	/**
	 * Set recording the state of each key,
	 * identified by its keycode.
	 * If the key is down, the keycode is in the set.
	 * Otherwise, if the key is up, the keycode is not in the set.
	 */
	extern std::unordered_set<KeyCode> keyState;

	/**
	 * Array recording the previous state of each key.
	 * This is used by didKeyGoDown.
	 */
	extern std::unordered_set<KeyCode> previousKeyState;

	/**
	 * A buffer that holds the keys received from key down events.
	 */
	extern std::deque<KeyboardBufferItem> keyboardBuffer;

	/**
	 * A mapping of key codes to other key codes.
	 * Key codes in this mapping will be translated
	 * to the key code they are mapped to before processing.
	 */
	extern std::unordered_map<KeyCode, KeyCode> keyCodeMap;

	/**
	 * Returns true if the key for the given keycode
	 * is currently being held down, otherwise false.
	 */
	bool isKeyDown(KeyCode keycode);

	/**
	 * Returns true if either of the left or right shift keys are down,
	 * otherwise false.
	 */
	bool isShiftKeyDown();

	/**
	 * Returns true if either of the left or right control keys are down,
	 * otherwise false.
	 */
	bool isControlKeyDown();

	/**
	 * Returns true if a key that emits the given ASCII character is down,
	 * otherwise false.
	 *
	 * For letters the case is ignored, that is,
	 * when the 'A' key is pressed, both 'a' and 'A'
	 * are considered to be down.
	 */
	bool isAsciiCharacterKeyDown(byte c);

	/**
	 * Returns true if there are characters waiting in the keyboard buffer,
	 * otherwise false.
	 */
	bool keyboardBufferContainsElements();

	/**
	 * Appends an item to the global keyboard key buffer.
	 */
	void appendKeyboardBufferElement(KeyCode keyCode, CodePoint codePoint);

	/**
	 * Reads the next key in the keyboard input buffer.
	 * If there are no elements in the buffer,
	 * the behaviour of this function is undefined.
	 */
	KeyboardBufferItem getNextKeyboardBufferElement();

	/**
	 * Clears the keyboard buffer.
	 */
	void clearKeyboardBuffer();

	/**
	 * Initializes the keyboard handler.
	 */
	void initializeKeyboard();

	/**
	 * Sets the given keycode as up.
	 */
	void setKeyUp(KeyCode keycode);

	/**
	 * Sets the given keycode as down.
	 */
	void setKeyDown(KeyCode keycode);

	/**
	 * Returns true if the given key went down
	 * since the key was last checked with this function,
	 * otherwise false.
	 */
	bool didKeyGoDown(KeyCode keycode);

	/**
	 * Converts an SDL keycode into a TA3D keycode.
	 */
	KeyCode sdlToKeyCode(SDL_Keycode key);
}

#define KEY_UNKNOWN SDLK_UNKNOWN

#define KEY_ENTER SDLK_RETURN
#define KEY_SPACE SDLK_SPACE
#define KEY_LEFT SDLK_LEFT
#define KEY_RIGHT SDLK_RIGHT
#define KEY_UP SDLK_UP
#define KEY_DOWN SDLK_DOWN
#define KEY_TAB SDLK_TAB
#define KEY_LSHIFT SDLK_LSHIFT
#define KEY_RSHIFT SDLK_RSHIFT
#define KEY_LCONTROL SDLK_LCTRL
#define KEY_RCONTROL SDLK_RCTRL
#define KEY_ESC SDLK_ESCAPE
#define KEY_BACKSPACE SDLK_BACKSPACE
#define KEY_DEL SDLK_DELETE
#define KEY_ALT SDLK_LALT
#define KEY_PAUSE SDLK_PAUSE

#define KEY_F1 SDLK_F1
#define KEY_F2 SDLK_F2
#define KEY_F3 SDLK_F3
#define KEY_F4 SDLK_F4
#define KEY_F5 SDLK_F5
#define KEY_F6 SDLK_F6
#define KEY_F7 SDLK_F7
#define KEY_F8 SDLK_F8
#define KEY_F9 SDLK_F9
#define KEY_F10 SDLK_F10
#define KEY_F11 SDLK_F11
#define KEY_F12 SDLK_F12

#define KEY_0 SDLK_0
#define KEY_1 SDLK_1
#define KEY_2 SDLK_2
#define KEY_3 SDLK_3
#define KEY_4 SDLK_4
#define KEY_5 SDLK_5
#define KEY_6 SDLK_6
#define KEY_7 SDLK_7
#define KEY_8 SDLK_8
#define KEY_9 SDLK_9

#define KEY_PLUS SDLK_PLUS
#define KEY_MINUS SDLK_MINUS
#define KEY_PLUS_PAD SDLK_KP_PLUS
#define KEY_MINUS_PAD SDLK_KP_MINUS

#define KEY_A SDLK_a
#define KEY_B SDLK_b
#define KEY_C SDLK_c
#define KEY_D SDLK_d
#define KEY_E SDLK_e
#define KEY_F SDLK_f
#define KEY_G SDLK_g
#define KEY_H SDLK_h
#define KEY_I SDLK_i
#define KEY_J SDLK_j
#define KEY_K SDLK_k
#define KEY_L SDLK_l
#define KEY_M SDLK_m
#define KEY_N SDLK_n
#define KEY_O SDLK_o
#define KEY_P SDLK_p
#define KEY_Q SDLK_q
#define KEY_R SDLK_r
#define KEY_S SDLK_s
#define KEY_T SDLK_t
#define KEY_U SDLK_u
#define KEY_V SDLK_v
#define KEY_W SDLK_w
#define KEY_X SDLK_x
#define KEY_Y SDLK_y
#define KEY_Z SDLK_z
#define KEY_PAGEUP SDLK_PAGEUP
#define KEY_PAGEDOWN SDLK_PAGEDOWN

// This is used to show/hide the console
#ifdef TA3D_PLATFORM_WINDOWS
#define KEY_TILDE SDLK_F8
#else
#define KEY_TILDE SDLK_RIGHTPAREN
#endif

#define KEY_CAPSLOCK SDLK_CAPSLOCK

#define KEY_END SDLK_END
#define KEY_HOME SDLK_HOME
#define KEY_ENTER_PAD SDLK_KP_ENTER

#endif
