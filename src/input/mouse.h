#ifndef __TA3D_MOUSE_H__
#define __TA3D_MOUSE_H__

#include "../gaf.h"

namespace TA3D
{
    namespace VARS
    {
		TA3D_API_E int                              mouse_x, mouse_y, mouse_z, mouse_b;
    }

	/*!
	** \brief poll mouse events
	*/
	void poll_mouse();

	/*!
	** \brief set mouse position
	*/
	void position_mouse(int x, int y);

	/*!
	** \brief return mouse move since last call
	*/
	void get_mouse_mickeys(int *mx, int *my);

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
#define CURSOR_DEFAULT		CURSOR_BLUE
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
