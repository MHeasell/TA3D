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

#ifndef __TA3D_MISC_TA3D_MATH_H__
#define __TA3D_MISC_TA3D_MATH_H__

#include <cstdlib>
#include <cmath>

#include "../stdafx.h"

#define TA3D_MATH_RANDOM_TABLE_SIZE 0x100000U
#define TA3D_MATH_RANDOM_TABLE_MASK 0x0FFFFFU

#define TA3D_DEFAULT_DOUBLE_EPSILON (1.e-6)
#define TA3D_DEFAULT_FLOAT_EPSILON (1.e-6f)

namespace TA3D
{

	/*!
	 * \brief Convenient routines for Mathematics calculations
	 */
	namespace Math
	{

		template <typename T>
		inline T Max(const T a, const T b)
		{
			return (a > b) ? a : b;
		}

		template <typename T>
		inline T Min(const T a, const T b)
		{
			return (a > b) ? b : a;
		}

		/**
		 * Clamps the input value to within the given range.
		 * @param v The value to clamp
		 * @param m The minimum allowed value
		 * @param M The maximum allowed value
		 * @return v, or min or max if v was out of range.
		 */
		template <typename T>
		inline T Clamp(const T& v, const T min, const T max)
		{
			return (v > max) ? max : v < min ? min : v;
		}

		inline bool IsPowerOfTwo(const int x)
		{
			return !(x & (x - 1)) && x;
		}

		template <typename T>
		inline T Deg2Rad(const T deg)
		{
			return (deg * 0.017453292f);
		}

		template <typename T>
		inline T Rad2Deg(const T rad)
		{
			return (rad * 57.29578122f);
		}

		template <typename T>
		inline int Sgn(const T a)
		{
			return (a < 0) ? -1 : ((a > 0) ? 1 : 0);
		}

		inline bool AlmostEquals(float a, float b, float epsilon)
		{
			return std::abs(a - b) < epsilon;
		}

		inline bool AlmostEquals(float a, float b)
		{
			return AlmostEquals(a, b, TA3D_DEFAULT_FLOAT_EPSILON);
		}

		inline bool AlmostEquals(double a, double b, double epsilon)
		{
			return std::abs(a - b) < epsilon;
		}

		inline bool AlmostEquals(double a, double b)
		{
			return AlmostEquals(a, b, TA3D_DEFAULT_DOUBLE_EPSILON);
		}

		inline bool AlmostZero(float n, float epsilon)
		{
			return std::abs(n) < epsilon;
		}

		inline bool AlmostZero(float n)
		{
			return AlmostZero(n, TA3D_DEFAULT_FLOAT_EPSILON);
		}

		inline bool AlmostZero(double n, double epsilon)
		{
			return std::abs(n) < epsilon;
		}

		inline bool AlmostZero(double n)
		{
			return AlmostZero(n, TA3D_DEFAULT_DOUBLE_EPSILON);
		}

		/**
		 * Quantizes a value to the nearest multiple of the given interval.
		 *
		 * @param value The value to quantize.
		 * @param interval The interval. Must be a positive (>0) number.
		 * @return The quantized value.
		 */
		float snapToInterval(float value, float interval);

		/**
		 * Generates a random floating point number x,
		 * where min <= x < max.
		 */
		float randomFloat(float min, float max);

		/**
		 * Generates a random integer x,
		 * where min <= x < max.
		 * The difference between min and max must not be greater than RAND_MAX.
		 */
		int randomInt(int min, int max);

		/*!
		 * \brief Get log2(n)
		 */
		uint32 Log2(uint32 n);

		//! \name Random table
		//@{
		//! A table for precached random numbers
		class PreCachedRandomNumbers
		{
		public:
			PreCachedRandomNumbers();

			void reset();

			inline int operator()() const
			{
				return pCache[++pOffset & TA3D_MATH_RANDOM_TABLE_MASK];
			}

		private:
			mutable size_t pOffset;
			int pCache[TA3D_MATH_RANDOM_TABLE_SIZE];
		};

		/*!
		 * \brief A pre-cached table for random numbers
		 */
		extern PreCachedRandomNumbers RandomTable;
		//@}

		template <typename T>
		inline T Modf(const T a)
		{
			return a - T(int(a));
		}

	} // namespace Math
} // namespace TA3D

//! Alias to the isnan() function
#define isNaN(x) std::isnan(x)

/*
 * The below functions don't exists within windows math routines.
 */
#if defined TA3D_PLATFORM_WINDOWS && defined TA3D_PLATFORM_MSVC

inline const float asinh(const float f)
{
	return (float)logf((float)(f + sqrtf(f * f + 1)));
}

inline const float acosh(const float f)
{
	return (float)logf((float)(f + sqrtf(f * f - 1)));
}

inline const float atanh(const float f)
{
	return (float)logf((float)((float)(1.0f / f + 1.0f) / (float)(1.0f / f - 1.0f))) / 2.0f;
}

#endif

// The rand() function on Windows platforms should be replaced by something
// that fits a 32bits integer (it would be slower of course)
#ifdef TA3D_PLATFORM_WINDOWS
#define TA3D_RAND() (rand() | (rand() << 16))
#else
#define TA3D_RAND() rand()
#endif

// Pi, our favorite number
#ifdef PI
#undef PI
#endif
#define PI 3.141592653589793238462643383279502884197169399375105f
#define DB_PI 6.28318530717958647693f

// Square root of 2
#ifdef SQRT2
#undef SQRT2
#endif
#define SQRT2 1.414213562373095048801688724209698078569671875376948f

// Square root of 2 over 2
#ifdef SQRT2OVER2
#undef SQRT2OVER2
#endif
#define SQRT2OVER2 0.707106781186547524400844362104849039284835937688474f

// Square root of 3
#ifdef SQRT3
#undef SQRT3
#endif
#define SQRT3 1.732050807568877293527446341505872366942805253810381f

// Square root of 3 over 2
#ifdef SQRT3OVER2
#undef SQRT3OVER2
#endif
#define SQRT3OVER2 0.866025403784438646763723170752936183471402626905190f

// The natural number "e"
#ifdef NAT_E
#undef NAT_E
#endif
#define NAT_E 2.718281828459045235360287471352662497757247093699959f

#define DEG2RAD (PI / 180.0f)
#define RAD2DEG (180.0f / PI)

#define DEG2TA (65536.0f / 360.0f)
#define TA2DEG (360.0f / 65536.0f)

#define RAD2TA (RAD2DEG * DEG2TA)
#define TA2RAD (TA2DEG * DEG2RAD)

#define I2PWR16 (1.0f / 65536.0f)

#endif // __TA3D_MISC_TA3D_MATH_H__
