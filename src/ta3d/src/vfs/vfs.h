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
**  File: vfs.h
** Notes:
**   Zuzuf:  This module implements an abstraction layer over the filesystem
**           and the archives loaded by the game. It doesn't handle archives itself
**           it only maps a path/filename to the corresponding data.
*/

#ifndef __TA3D_UTILS_VFS_H__
#define __TA3D_UTILS_VFS_H__

#include <list>
#include <vector>
#include "archive.h"
#include <misc/hash_table.h>
#include <misc/string.h>
#include <sdl.h>
#include <threads/mutex.h>
#include "file.h"
#include <threads/policy.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

namespace TA3D
{
	namespace UTILS
	{
		class VFS : public ObjectLevelLockable
		{
		private:
			typedef VFS ThreadingPolicy;

			struct CacheFileData
			{
				uint32 length;
				byte* data;
				String name;
			}; // class CacheFileData

		public:
			/**
			 * Returns the global VFS instance
			 */
			static inline VFS* Instance()
			{
				static VFS instance;
				return &instance;
			}

		public:
			/**
			 * Reloads all archives
			 */
			void reload();

			uint32 getFilelist(String pattern, String::List& li);
			uint32 getFilelist(String pattern, String::Vector& li);

			uint32 getDirlist(String pattern, String::List& li);
			uint32 getDirlist(String pattern, String::Vector& li);

			File* readFile(const String& filename);

			File* readFileRange(const String& filename, const uint32 start, const uint32 length);

			bool fileExists(String filename);

			int filePriority(const String& filename);

			String extractFile(const String& filename);

		private:
			VFS();

			~VFS();

			/**
			 * Loads all archives
			 */
			void load();

			/**
			 * Unloads all archives
			 */
			void unload();

			/**
			 * Adds the files within the given archive to the VFS file table.
			 *
			 * @param filename The path to the archive file.
			 * @param priority
			 */
			void addArchive(const String& filename, const int priority);

			void locateAndReadArchives(const String& path, const int priority);

			void putInCache(const String& filename, File* file);

			CacheFileData* isInCache(const String& filename);

			/**
			 * BUilds the table of all dirs from the list of all files
			 */
			void buildDirMap();

		protected:
			void loadWL();
			void unloadWL();

			CacheFileData* isInCacheWL(const String& filename);

			File* isInDiskCacheWL(const String& filename);

		private:
			/**
			 * A list of paths to search when looking for archive files
			 * to load from the underlying file system.
			 */
			String::Vector pPaths;

			typedef TA3D::UTILS::HashMap<Archive::FileInfo*>::Dense FileInfoMap;

			/**
			 * A map of file paths to FileInfo objects.
			 * A file path represents a file within the VFS.
			 * The corresponding FileInfo object indicates
			 * where the file can be found on the underlying filesystem.
			 */
			FileInfoMap pFiles;

			typedef TA3D::UTILS::HashMap<TA3D::UTILS::HashMap<bool>::Sparse>::Dense DirMap;
			DirMap pDirs;

			/**
			 * A list of cached files.
			 * The cache is used to avoid hitting the disk
			 * when a file is requested multiple times.
			 */
			std::list<CacheFileData> fileCache;

			/**
			 * A list of Archive*, needed only for cleanup.
			 */
			std::vector<Archive*> archives;

		}; // class VFS;

		bool load_palette(SDL_Color* pal, const String& filename = "palettes\\palette.pal");

		bool loadFromFile(String::List& out, const String& filename, const uint32 sizeLimit, const bool emptyListBefore);
		bool loadFromFile(String::Vector& out, const String& filename, const uint32 sizeLimit, const bool emptyListBefore);

	} // namespace utils
} // namespace TA3D

#endif // __TA3D_UTILS_VFS_H__
