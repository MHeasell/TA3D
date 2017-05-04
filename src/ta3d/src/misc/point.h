#ifndef __TA3D_MISC_POINT_H__
#define __TA3D_MISC_POINT_H__

namespace TA3D
{
	/**
	 * Structure holding values for 2D-coordinates
	 */
	template <typename T>
	struct Point
	{
		//! X Coordinate
		T x;
		//! Y Coordinate
		T y;

		Point(): x(0), y(0) {}

		Point(T x, T y): x(x), y(y) {}
	};

} // namespace TA3D

#endif // __TA3D_MISC_POINT_H__
