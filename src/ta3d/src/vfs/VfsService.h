#ifndef __TA3D_VFSSERVICE_H__
#define __TA3D_VFSSERVICE_H__

#include "vfs.h"
#include "file.h"

namespace TA3D
{
	/**
	 * Service providing the virtual file system.
	 * This is currently a wrapper around the global VFS,
	 * however the implementation may change after the application
	 * has migrated to this class.
	 */
	class VfsService
	{
	public:
		/**
		 * Reloads all archives
		 */
		void reload();

		uint32 getFilelist(String pattern, String::List& li);
		uint32 getFilelist(String pattern, String::Vector& li);

		uint32 getDirlist(String pattern, String::List& li);
		uint32 getDirlist(String pattern, String::Vector& li);

		UTILS::File* readFile(const String& filename);

		UTILS::File* readFileRange(const String& filename, const uint32 start, const uint32 length);

		bool fileExists(String filename);

		int filePriority(const String& filename);

		String extractFile(const String& filename);
	};
}


#endif // __TA3D_VFSSERVICE_H__
