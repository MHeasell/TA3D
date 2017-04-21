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
#ifndef __TA3D_XX_MISC_CAMERA_H__
#define __TA3D_XX_MISC_CAMERA_H__

#include <vector>
#include "matrix.h"
#include "vector.h"

namespace TA3D
{

	//! Class for managing the camera
	class Camera
	{
	public:
		static Camera* inGame;

	public:
		Camera();

		/*!
		** \brief Set the matrix
		*/
		void setMatrix(const Matrix& v);

		/*!
		** \brief Get the camera matrix
		*/
		Matrix getMatrix() const;

		/*!
		** \brief Replace the OpenGL camera
		*/
		void setView(bool classic = false);

		/*!
		** \brief Reset all data
		*/
		void reset();

		/*!
		** \brief Returns the 8 points defining the frustum volume
		*/
		void getFrustum(std::vector<Vector3D>& list);

	public:
		//! Top of the camera
		Vector3D up;

		//! Side of the camera (optimization for particles)
		Vector3D side;

		//! Position
		Vector3D pos;

		//! Position of the camera
		Vector3D rpos;
		//! Direction of the camera
		Vector3D dir;

		//! For the visible volume
		float zfar;
		float znear;

		//! Square of the maximum distance
		float zfar2;

		float zoomFactor;
	}; // class Camera

} // namespace TA3D

#endif // __TA3D_XX_MISC_CAMERA_H__
