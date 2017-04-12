#ifndef __TA3D_KEYBOARDSERVICE_H__
#define __TA3D_KEYBOARDSERVICE_H__

#include "keyboard.h"

namespace TA3D
{
	/**
	 * Service providing keyboard interaction.
	 * This currently just wraps the equivalent global functions.
	 * Once the application has migrated to this class
	 * this implementation may change.
	 */
	class KeyboardService
	{
	public:

		/**
		 * Returns true if the key for the given keycode
		 * is currently being held down, otherwise false.
		 */
		bool isKeyDown(KeyCode keycode);

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
	};
}


#endif // __TA3D_KEYBOARDSERVICE_H__
