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

#include "vector.h"

namespace TA3D
{

	void Vector2D::unit()
	{
		if (!isNull()) // Si le vecteur n'est pas nul
		{
			float n = 1.0f / sqrtf(x * x + y * y); // Inverse de la norme du vecteur
			x *= n;
			y *= n;
		}
	}

} // namespace TA3D
