#ifndef __TA3D_KEYBOARD_H__
#define __TA3D_KEYBOARD_H__

namespace TA3D
{
    namespace VARS
    {
		TA3D_API_E int								ascii_to_scancode[ 256 ];
		TA3D_API_E int                              key[0x1000];
		TA3D_API_E std::list<uint32>                keybuf;
    }

	/*!
	** \brief return true is there are key codes waiting in the buffer, false otherwise
	*/
	inline bool keypressed()    {   return !VARS::keybuf.empty(); }

	/*!
	** \brief return the next key code in the key buffer
	*/
	uint32 readkey();

	/*!
	** \brief clears the key code buffer
	*/
	void clear_keybuf();

	/*!
	** \brief poll keyboard events
	*/
	void poll_keyboard();

	/*!
	** \brief initialize keyboard handler
	*/
	void init_keyboard();
}

#define KEY_ENTER       SDLK_RETURN
#define KEY_SPACE       SDLK_SPACE
#define KEY_LEFT        SDLK_LEFT
#define KEY_RIGHT       SDLK_RIGHT
#define KEY_UP          SDLK_UP
#define KEY_DOWN        SDLK_DOWN
#define KEY_TAB         SDLK_TAB
#define KEY_LSHIFT      SDLK_LSHIFT
#define KEY_RSHIFT      SDLK_RSHIFT
#define KEY_LCONTROL    SDLK_LCTRL
#define KEY_RCONTROL    SDLK_RCTRL
#define KEY_ESC         SDLK_ESCAPE
#define KEY_BACKSPACE   SDLK_BACKSPACE
#define KEY_DEL         SDLK_DELETE
#define KEY_ALT         SDLK_LALT

#define KEY_F1          SDLK_F1
#define KEY_F2          SDLK_F2
#define KEY_F3          SDLK_F3
#define KEY_F4          SDLK_F4
#define KEY_F5          SDLK_F5
#define KEY_F6          SDLK_F6
#define KEY_F7          SDLK_F7
#define KEY_F8          SDLK_F8
#define KEY_F9          SDLK_F9
#define KEY_F10         SDLK_F10
#define KEY_F11         SDLK_F11
#define KEY_F12         SDLK_F12

#define KEY_0           SDLK_0
#define KEY_1           SDLK_1
#define KEY_2           SDLK_2
#define KEY_3           SDLK_3
#define KEY_4           SDLK_4
#define KEY_5           SDLK_5
#define KEY_6           SDLK_6
#define KEY_7           SDLK_7
#define KEY_8           SDLK_8
#define KEY_9           SDLK_9

#define KEY_PLUS        SDLK_PLUS
#define KEY_MINUS       SDLK_MINUS
#define KEY_PLUS_PAD    SDLK_KP_PLUS
#define KEY_MINUS_PAD   SDLK_KP_MINUS

#define KEY_A           SDLK_a
#define KEY_B           SDLK_b
#define KEY_C           SDLK_c
#define KEY_D           SDLK_d
#define KEY_E           SDLK_e
#define KEY_F           SDLK_f
#define KEY_G           SDLK_g
#define KEY_H           SDLK_h
#define KEY_I           SDLK_i
#define KEY_J           SDLK_j
#define KEY_K           SDLK_k
#define KEY_L           SDLK_l
#define KEY_M           SDLK_m
#define KEY_N           SDLK_n
#define KEY_O           SDLK_o
#define KEY_P           SDLK_p
#define KEY_Q           SDLK_q
#define KEY_R           SDLK_r
#define KEY_S           SDLK_s
#define KEY_T           SDLK_t
#define KEY_U           SDLK_u
#define KEY_V           SDLK_v
#define KEY_W           SDLK_w
#define KEY_X           SDLK_x
#define KEY_Y           SDLK_y
#define KEY_Z           SDLK_z

#define KEY_TILDE       SDLK_RIGHTPAREN

#define KEY_CAPSLOCK    SDLK_CAPSLOCK

#endif
