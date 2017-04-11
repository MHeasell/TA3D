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
**  File: hpi.h
** Notes:
**   Zuzuf:  This module implements the HPI interface through the Archive layer
*/

#ifndef __TA3D_UTILS_VFS_HPI_H__
#define __TA3D_UTILS_VFS_HPI_H__

#include "archive.h"
#include <misc/hash_table.h>
#include <fstream>

#define HEX_HAPI 0x49504148
#define HPI_V1 0x00010000

namespace TA3D
{
	namespace UTILS
	{
		class Hpi : public Archive
		{
		private:
#pragma pack(1) // Byte alignment.

			struct HPIVERSION
			{
				sint32 HPIMarker; /* Must be HEX_HAPI */
				sint32 Version; /* Must be HPI_V1 */
			};

			struct HPIHEADER
			{
				HPIHEADER() : DirectorySize(0), Key(0), Start(0) {}

				sint32 DirectorySize; /* Directory size */

				sint32 Key; /* Decode key */

				sint32 Start; /* Directory offset */

			};

			struct HPIENTRY
			{
				sint32 NameOffset;
				sint32 CountOffset;
				sint8 Flag;
			};

			struct HPICHUNK
			{
				//! always 0x48535153 (SQSH)
				sint32 Marker;
				//! I have no idea what these mean
				sint8 Unknown1;
				//! 1 = lz77, 2 = zlib
				sint8 CompMethod;
				//! Is the chunk encrypted ?
				sint8 Encrypt;
				//! The length of the compressed data
				sint32 CompressedSize;
				//! The length of the decompressed data
				sint32 DecompressedSize;
				//! Checksum
				sint32 Checksum;

			};

			/**
			 * Used for strict-aliasing safety
			 */
			union HPICHUNK_U {
				HPICHUNK chunk;
				byte bytes[sizeof(HPICHUNK)];
			};

#pragma pack()
		public:
			class HpiFile : public Archive::FileInfo
			{
			public:
				HPIENTRY entry;

				uint64 size;

			public:
				virtual ~HpiFile() {}
				inline void setName(const String& name) { Archive::FileInfo::name = name; }
				inline void setParent(Archive* parent) { Archive::FileInfo::parent = parent; }
			};

		public:
			Hpi(const String& filename);

			virtual ~Hpi();

			/*!
            ** \brief Loads an archive
            */
			virtual void open(const String& filename);

			/*!
            ** \brief Just close the opened archive
            */
			virtual void close();

			/*!
            ** \brief Return the list of all files in the archive
            */
			virtual void getFileList(std::deque<FileInfo*>& lFiles);

			virtual File* readFile(const String& filename);
			virtual File* readFile(const FileInfo* file);

			virtual File* readFileRange(const String& filename, const uint32 start, const uint32 length);
			virtual File* readFileRange(const FileInfo* file, const uint32 start, const uint32 length);

			/*!
            ** \brief returns true if using the cache is a good idea (real FS will return false)
            ** \return
            */
			virtual bool needsCaching();

		public:
			static void finder(String::List& fileList, const String& path);
			static Archive* loader(const String& filename);

		private:
			String m_cDir;

			HPIHEADER header;

			sint8 key;

			sint8* directory;

			std::ifstream HPIFile;

			HashMap<HpiFile*>::Sparse files;

			int priority;

		private:
			void processRoot(const String& startPath, const sint32 offset);

			void processSubDir(HPIENTRY* base);

			sint32 readAndDecrypt(sint32 fpos, byte* buff, sint32 buffsize);

			sint32 ZLibDecompress(byte* out, byte* in, HPICHUNK* Chunk);

			sint32 LZ77Decompress(byte* out, byte* in);

			sint32 decompress(byte* out, byte* in, HPICHUNK* Chunk);
		}; // class Hpi
	}	  // namespace utils
} // namespace TA3D

#endif // __TA3D_UTILS_VFS_HPI_H__
