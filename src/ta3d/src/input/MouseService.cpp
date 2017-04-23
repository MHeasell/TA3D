#include "MouseService.h"
#include "mouse.h"

namespace TA3D
{

	void MouseService::poll_inputs()
	{
		TA3D::poll_inputs();
	}

	void MouseService::position_mouse(int x, int y)
	{
		TA3D::position_mouse(x, y);
	}

	void MouseService::get_mouse_mickeys(int* mx, int* my)
	{
		TA3D::get_mouse_mickeys(mx, my);
	}

	void MouseService::draw_cursor()
	{
		TA3D::draw_cursor();
	}

	int MouseService::anim_cursor(const int type)
	{
		return TA3D::anim_cursor(type);
	}

	void MouseService::init_mouse()
	{
		TA3D::init_mouse();
	}

	void MouseService::grab_mouse(bool grab)
	{
		TA3D::grab_mouse(grab);
	}
}