/*  TA3D, a remake of Total Annihilation
    Copyright (C) 2006  Roland BROCHARD

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

/*
**  File: stdafx.h
** Notes:  **** PLEASE SEE README.txt FOR LICENCE AGREEMENT ****
**  Cire: *stdafx.h and stdafx.cpp will generate a pch file
**         (pre compiled headers).
**        *All other cpp files MUST include this file as its first include.
**        *No .h files should include this file.
**        *The goal is to include everything that we need from system, and
**          game libiries, ie everything we need external to our project.
*/

#ifndef __TA3D_STDAFX_H__
#define __TA3D_STDAFX_H__

#include <cstdint>

// Include the config options generated by the configure script
#include "../config.h"

#include "ta3d_string.h"

#if defined(TA3D_PLATFORM_MSVC)
#define strcasecmp(x, xx) _stricmp(x, xx)
#endif

namespace TA3D
{
	// Fundamental types
	typedef uint64_t uint64;
	typedef uint32_t uint32;
	typedef uint16_t uint16;
	typedef uint8_t uint8;
	typedef int64_t sint64;
	typedef int32_t sint32;
	typedef int16_t sint16;
	typedef int8_t sint8;

	typedef uint8 byte;
	typedef unsigned char uchar;
	typedef signed char schar;

	// widely used domain types
	typedef int PlayerId;
}

#define DELETE_ARRAY(x)  \
	do                   \
	{                    \
		if (x)           \
		{                \
			delete[](x); \
			(x) = NULL;  \
		}                \
	} while (false)

#endif // __TA3D_STDAFX_H__
