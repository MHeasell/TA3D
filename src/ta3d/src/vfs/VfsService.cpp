#include "VfsService.h"

namespace TA3D
{
	void VfsService::reload()
	{
		return UTILS::VFS::Instance()->reload();
	}

	uint32 VfsService::getFilelist(String pattern, String::List& li)
	{
		return UTILS::VFS::Instance()->getFilelist(pattern, li);
	}

	uint32 VfsService::getFilelist(String pattern, String::Vector& li)
	{
		return UTILS::VFS::Instance()->getFilelist(pattern, li);
	}

	uint32 VfsService::getDirlist(String pattern, String::List& li)
	{
		return UTILS::VFS::Instance()->getDirlist(pattern, li);
	}

	uint32 VfsService::getDirlist(String pattern, String::Vector& li)
	{
		return UTILS::VFS::Instance()->getDirlist(pattern, li);
	}

	UTILS::File* VfsService::readFile(const String& filename)
	{
		return UTILS::VFS::Instance()->readFile(filename);
	}

	UTILS::File* VfsService::readFileRange(const String& filename, const uint32 start, const uint32 length)
	{
		return UTILS::VFS::Instance()->readFileRange(filename, start, length);
	}

	bool VfsService::fileExists(String filename)
	{
		return UTILS::VFS::Instance()->fileExists(filename);
	}

	int VfsService::filePriority(const String& filename)
	{
		return UTILS::VFS::Instance()->filePriority(filename);
	}

	String VfsService::extractFile(const String& filename)
	{
		return UTILS::VFS::Instance()->extractFile(filename);
	}
}