#include "VfsService.h"

namespace TA3D
{
	void VfsService::reload()
	{
		return VFS::Instance()->reload();
	}

	uint32 VfsService::getFilelist(String pattern, String::List& li)
	{
		return VFS::Instance()->getFilelist(pattern, li);
	}

	uint32 VfsService::getFilelist(String pattern, String::Vector& li)
	{
		return VFS::Instance()->getFilelist(pattern, li);
	}

	uint32 VfsService::getDirlist(String pattern, String::List& li)
	{
		return VFS::Instance()->getDirlist(pattern, li);
	}

	uint32 VfsService::getDirlist(String pattern, String::Vector& li)
	{
		return VFS::Instance()->getDirlist(pattern, li);
	}

	File* VfsService::readFile(const String& filename)
	{
		return VFS::Instance()->readFile(filename);
	}

	File* VfsService::readFileRange(const String& filename, const uint32 start, const uint32 length)
	{
		return VFS::Instance()->readFileRange(filename, start, length);
	}

	bool VfsService::fileExists(String filename)
	{
		return VFS::Instance()->fileExists(filename);
	}

	int VfsService::filePriority(const String& filename)
	{
		return VFS::Instance()->filePriority(filename);
	}

	String VfsService::extractFile(const String& filename)
	{
		return VFS::Instance()->extractFile(filename);
	}
}