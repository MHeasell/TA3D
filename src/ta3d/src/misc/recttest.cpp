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
#include "recttest.h"
#include <sdl.h>

namespace TA3D
{

	RectTest::RectTest(Camera& cam, const Rect<float>& rectangle)
		: cam(cam),
		  x1(Math::Min(rectangle.x1, rectangle.x2)),
		  y1(Math::Min(rectangle.y1, rectangle.y2)),
		  x2(Math::Max(rectangle.x1, rectangle.x2)),
		  y2(Math::Max(rectangle.y1, rectangle.y2))
	{
	}

	bool RectTest::contains(const Vector3D& point) const
	{
		// construct the rays
		auto bottomLeftRay = cam.screenToWorldRay(Vector2D(x1, y1));
		auto bottomRightRay = cam.screenToWorldRay(Vector2D(x2, y1));
		auto topLeftRay = cam.screenToWorldRay(Vector2D(x1, y2));
		auto topRightRay = cam.screenToWorldRay(Vector2D(x2, y2));

		// construct the clipping planes
		auto topPlane = Plane3D::fromPoints(topLeftRay.pointAt(0.0f), topLeftRay.pointAt(1.0f), topRightRay.pointAt(0.0f));
		auto bottomPlane = Plane3D::fromPoints(bottomLeftRay.pointAt(0.0f), bottomRightRay.pointAt(0.0f), bottomLeftRay.pointAt(1.0f));
		auto leftPlane = Plane3D::fromPoints(bottomLeftRay.pointAt(0.0f), bottomLeftRay.pointAt(1.0f), topLeftRay.pointAt(0.0f));
		auto rightPlane = Plane3D::fromPoints(bottomRightRay.pointAt(0.0f), topRightRay.pointAt(0.0f), bottomRightRay.pointAt(1.0f));

		// test that the point is on the correct side of all planes
		return topPlane.isInFront(point)
			   && bottomPlane.isInFront(point)
			   && leftPlane.isInFront(point)
			   && rightPlane.isInFront(point);
	}

} // namespace TA3D
