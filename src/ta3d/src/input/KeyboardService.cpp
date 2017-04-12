#include "KeyboardService.h"

namespace TA3D
{

	bool KeyboardService::isKeyDown(KeyCode keycode)
	{
		return TA3D::isKeyDown(keycode);
	}

	bool KeyboardService::isAsciiCharacterKeyDown(byte c)
	{
		return TA3D::isAsciiCharacterKeyDown(c);
	}

	bool KeyboardService::keyboardBufferContainsElements()
	{
		return TA3D::keyboardBufferContainsElements();
	}

	void KeyboardService::appendKeyboardBufferElement(KeyCode keyCode, CodePoint codePoint)
	{
		return TA3D::appendKeyboardBufferElement(keyCode, codePoint);
	}

	KeyboardBufferItem KeyboardService::getNextKeyboardBufferElement()
	{
		return TA3D::getNextKeyboardBufferElement();
	}

	void KeyboardService::clearKeyboardBuffer()
	{
		return TA3D::clearKeyboardBuffer();
	}

	void KeyboardService::initializeKeyboard()
	{
		return TA3D::initializeKeyboard();
	}

	void KeyboardService::setKeyUp(KeyCode keycode)
	{
		return TA3D::setKeyUp(keycode);
	}

	void KeyboardService::setKeyDown(KeyCode keycode)
	{
		return TA3D::setKeyDown(keycode);
	}

	bool KeyboardService::didKeyGoDown(KeyCode keycode)
	{
		return TA3D::didKeyGoDown(keycode);
	}

	KeyCode KeyboardService::sdlToKeyCode(SDL_Keycode key)
	{
		return TA3D::sdlToKeyCode(key);
	}
}