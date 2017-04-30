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
#include <TA3D_NameSpace.h>
#include "camera.h"
#include "math.h"

namespace TA3D
{

	Camera* Camera::inGame = NULL;

	Camera::Camera() // Initialise la caméra
	{
		reset();
	}

	void Camera::reset()
	{
		// Pos
		pos.x = 0.0f;
		pos.y = 0.0f;
		pos.z = 30.0f;

		rpos = pos;
		dir = up = pos;
		dir.z = -1.0f; // direction
		up.y = 1.0f;   // Haut
		zfar = 140000.0f;
		znear = 1.0f;
		side = dir * up;
		zfar2 = zfar * zfar;
		zoomFactor = 0.5f;
	}

	void Camera::setMatrix(const Matrix& v)
	{
		dir.reset();
		up = dir;
		dir.z = -1.0f;
		up.y = 1.0f;
		dir = dir * v;
		up = up * v;
		side = dir * up;
	}

	void Camera::setView(bool classic)
	{
		zfar2 = zfar * zfar;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glOrtho(-0.5f * zoomFactor * float(gfx->width), 0.5f * zoomFactor * float(gfx->width), -0.5f * zoomFactor * float(
			gfx->height), 0.5f * zoomFactor * float(
			gfx->height), znear, zfar);

		if (classic)
		{
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		}

		pos = rpos;
		Vector3D FP(pos);
		FP += dir;
		gluLookAt(pos.x, pos.y, pos.z,
			FP.x, FP.y, FP.z,
			up.x, up.y, up.z);

		if (!classic)
		{
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		}
	}

	Matrix Camera::getMatrix() const
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glOrtho(-0.5f * zoomFactor * float(gfx->width), 0.5f * zoomFactor * float(gfx->width), -0.5f * zoomFactor * float(
			gfx->height), 0.5f * zoomFactor * float(
			gfx->height), -512.0f, zfar);

		const Vector3D FP(rpos + dir);
		gluLookAt(pos.x, pos.y, pos.z,
			FP.x, FP.y, FP.z,
			up.x, up.y, up.z);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		Matrix mProj;
		GLfloat tmp[16];
		glGetFloatv(GL_PROJECTION_MATRIX, tmp);
		for (int i = 0; i < 16; ++i)
			mProj.E[i % 4][i / 4] = tmp[i];
		return mProj;
	}

	void Camera::getFrustum(std::vector<Vector3D>& list)
	{
		const Vector3D wside = static_cast<float>(gfx->width) * side;
		const Vector3D hup = static_cast<float>(gfx->height) * up;
		list.push_back(rpos + znear * dir + 0.5f * zoomFactor * (-wside + hup));
		list.push_back(rpos + znear * dir + 0.5f * zoomFactor * (wside + hup));
		list.push_back(rpos + znear * dir + 0.5f * zoomFactor * (-wside - hup));
		list.push_back(rpos + znear * dir + 0.5f * zoomFactor * (wside - hup));

		list.push_back(rpos + zfar * dir + 0.5f * zoomFactor * (-wside + hup));
		list.push_back(rpos + zfar * dir + 0.5f * zoomFactor * (wside + hup));
		list.push_back(rpos + zfar * dir + 0.5f * zoomFactor * (-wside - hup));
		list.push_back(rpos + zfar * dir + 0.5f * zoomFactor * (wside - hup));
	}

} // namespace TA3D
