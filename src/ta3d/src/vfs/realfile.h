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
**  File: realfile.h
** Notes:
**   Zuzuf:  This module implements a File object for real files
*/

#ifndef __TA3D_VFS_REALFILE_H__
#define __TA3D_VFS_REALFILE_H__

#include "file.h"
#include <fstream>

namespace TA3D
{
	class RealFile : public File
	{
	public:
		RealFile();
		RealFile(const String& filename);

		//! This is an abstract class, its destructor must be virtual
		virtual ~RealFile();

		//! Read s bytes and write them in memory pointed by p
		virtual int read(void* p, int s);
		//! Returns the size of the file
		virtual int size();
		//! Returns true if end of file has been reached
		virtual bool eof();
		//! Returns current position in file
		virtual int tell();
		//! Set absolute position in file
		virtual void seek(int pos);
		//! Read a single line from the file, returns true while end of file has not been reached
		virtual bool readLine(String& line);
		//! Returns true if the file is opened
		virtual bool isOpen();
		//! Returns a pointer to a memory buffer containing the file
		virtual const char* data();
		//! Close the file
		virtual void close();
		//! Tell if this is a real file (useful for use with external libraries)
		virtual bool isReal() const;
		//! Returns the real filename (if it's a real file)
		virtual const String& getRealFilename() const;

		void open(const String& filename);

	private:
		virtual File& operator=(const File&) { return *this; }

	private:
		std::ifstream sFile;
		char* buffer;
		String realFilename;
	};
}
#endif
