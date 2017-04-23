#ifndef __TA3D_MOUSESERVICE_H__
#define __TA3D_MOUSESERVICE_H__


namespace TA3D
{
	class MouseService
	{
	public:
		/*!
		** \brief poll mouse/keyboard events
		*/
		void poll_inputs();

		/*!
		** \brief set mouse position
		*/
		void position_mouse(int x, int y);

		/*!
		** \brief return mouse move since last call
		*/
		void get_mouse_mickeys(int* mx, int* my);

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

		/*!
		** \brief enable grabing mouse inside the window when in windowed mode
		*/
		void grab_mouse(bool);
	};
}


#endif // __TA3D_MOUSESERVICE_H__
