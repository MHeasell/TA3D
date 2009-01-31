#include "../stdafx.h"
#include "keyboard.h"
#include "mouse.h"

int						TA3D::VARS::ascii_to_scancode[ 256 ];
int                     TA3D::VARS::key[0x1000];
std::list<uint32>       TA3D::VARS::keybuf;

using namespace TA3D::VARS;

namespace TA3D
{
	uint32 readkey()
	{
	    uint32 res = VARS::keybuf.front();
	    VARS::keybuf.pop_front();
	    return res;
    }

	bool keypressed()
	{
	    return !VARS::keybuf.empty();
    }

	void clear_keybuf()
	{
	    VARS::keybuf.clear();
	}

    void poll_keyboard()
    {
        poll_mouse();
    }

    void init_keyboard()
    {
        SDL_EnableUNICODE(1);

        SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

        for(int i = 0 ; i < 0x1000 ; i++)
            VARS::key[i] = 0;
	    VARS::keybuf.clear();

        // Initializing the ascii to scancode table
        for (int i = 0; i < 256; ++i)
            ascii_to_scancode[i] = 0;

        ascii_to_scancode[ 'a' ] = KEY_A;
        ascii_to_scancode[ 'b' ] = KEY_B;
        ascii_to_scancode[ 'c' ] = KEY_C;
        ascii_to_scancode[ 'd' ] = KEY_D;
        ascii_to_scancode[ 'e' ] = KEY_E;
        ascii_to_scancode[ 'f' ] = KEY_F;
        ascii_to_scancode[ 'g' ] = KEY_G;
        ascii_to_scancode[ 'h' ] = KEY_H;
        ascii_to_scancode[ 'i' ] = KEY_I;
        ascii_to_scancode[ 'j' ] = KEY_J;
        ascii_to_scancode[ 'k' ] = KEY_K;
        ascii_to_scancode[ 'l' ] = KEY_L;
        ascii_to_scancode[ 'm' ] = KEY_M;
        ascii_to_scancode[ 'n' ] = KEY_N;
        ascii_to_scancode[ 'o' ] = KEY_O;
        ascii_to_scancode[ 'p' ] = KEY_P;
        ascii_to_scancode[ 'q' ] = KEY_Q;
        ascii_to_scancode[ 'r' ] = KEY_R;
        ascii_to_scancode[ 's' ] = KEY_S;
        ascii_to_scancode[ 't' ] = KEY_T;
        ascii_to_scancode[ 'u' ] = KEY_U;
        ascii_to_scancode[ 'v' ] = KEY_V;
        ascii_to_scancode[ 'w' ] = KEY_W;
        ascii_to_scancode[ 'x' ] = KEY_X;
        ascii_to_scancode[ 'y' ] = KEY_Y;
        ascii_to_scancode[ 'z' ] = KEY_Z;

        for (int i = 0; i < 26; ++i)
            ascii_to_scancode[ 'A' + i ] = ascii_to_scancode[ 'a' + i ];

        ascii_to_scancode[ '0' ] = KEY_0;
        ascii_to_scancode[ '1' ] = KEY_1;
        ascii_to_scancode[ '2' ] = KEY_2;
        ascii_to_scancode[ '3' ] = KEY_3;
        ascii_to_scancode[ '4' ] = KEY_4;
        ascii_to_scancode[ '5' ] = KEY_5;
        ascii_to_scancode[ '6' ] = KEY_6;
        ascii_to_scancode[ '7' ] = KEY_7;
        ascii_to_scancode[ '8' ] = KEY_8;
        ascii_to_scancode[ '9' ] = KEY_9;

        ascii_to_scancode[ ' ' ] = KEY_SPACE;
        ascii_to_scancode[ '\n' ] = KEY_ENTER;
        ascii_to_scancode[ 27 ] = KEY_ESC;
    }
}
