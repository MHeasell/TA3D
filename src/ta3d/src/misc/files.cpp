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

#include "files.h"
#include <logs/logs.h>
#include <fstream>
#include <sys/stat.h>
#include "paths.h"


namespace TA3D
{
namespace Paths
{
namespace Files
{


	template<class T>
	bool TmplLoadFromFile(T& out, const String& filename, const uint32 sizeLimit, const bool emptyListBefore)
	{
		if (emptyListBefore)
			out.clear();
		std::ifstream file(filename.c_str());
		if (!file)
		{
			LOG_WARNING("Impossible to open the file `" << filename << "`");
			return false;
		}
		if (sizeLimit)
		{
			file.seekg(0, std::ios_base::beg);
			std::ifstream::pos_type begin_pos = file.tellg();
			file.seekg(0, std::ios_base::end);
			if ((file.tellg() - begin_pos) > sizeLimit)
			{
				LOG_WARNING("Impossible to read the file `" << filename << "` (size > " << sizeLimit << ")");
				return false;
			}
			file.seekg(0, std::ios_base::beg);
		}
		std::string line;
		while (std::getline(file, line))
			out.push_back(line);
		return true;
	}


	bool Load(String::List& out, const String& filename, const uint32 sizeLimit, const bool emptyListBefore)
	{
		return TmplLoadFromFile< String::List >(out, filename, sizeLimit, emptyListBefore);
	}

	bool Load(String::Vector& out, const String& filename, const uint32 sizeLimit, const bool emptyListBefore)
	{
		return TmplLoadFromFile< String::Vector >(out, filename, sizeLimit, emptyListBefore);
	}


	bool Size(const String& filename, uint64& size)
	{
		struct stat results;
		if (!filename.empty() && stat(filename.c_str(), &results) == 0)
		{
			size = results.st_size;
			return true;
		}
		size = 0;
		return false;
	}


	char* LoadContentInMemory(const String& filename, const uint64 hardlimit)
	{
		uint64 s;
		return LoadContentInMemory(filename, s, hardlimit);
	}


	char* LoadContentInMemory(const String& filename, uint64& size, const uint64 hardlimit)
	{
		if (Size(filename, size))
		{
			if (0 == size)
			{
				char* ret = new char[1];
				LOG_ASSERT(ret != NULL);
				*ret = '\0';
				return ret;
			}
			if (size > hardlimit)
			{
				LOG_ERROR("Impossible to load the file `" << filename << "` in memory. Its size exceeds << "
						  << hardlimit / 1204 << "Ko");
				return NULL;
			}
			std::ifstream f;
			f.open(filename.c_str(), std::ios::in | std::ios::binary);
			if (f.is_open())
			{
				char* ret = new char[size + 1];
				LOG_ASSERT(ret != NULL);
				f.read((char*)ret, size);
				f.close();
				ret[size] = '\0';
				return ret;
			}
		}
		return NULL;
	}


	bool SaveToFile(const String& filename, const String& content)
	{
		std::ofstream dst(filename.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
		if (dst.is_open())
		{
			dst << content;
			return true;
		}
		return false;
	}


	String ReplaceExtension(const String& filename, const String& newExt)
	{
		if (filename.empty())
			return String();
		String::size_type p = filename.find_last_of('.');
		if (p == String::npos)
			return filename + newExt;
		String::size_type s = filename.find_last_of("\\/");
		if (s != String::npos && p < s)
			return filename + newExt;
		return filename.substr(0, p) + newExt;
	}



	bool Copy(const String& from, const String& to, const bool overwrite)
	{
		if (!Paths::Exists(from))
		{
			LOG_ERROR("[copy] Impossible to find the source file `" << from << "`");
			return false;
		}
		if (!overwrite && Paths::Exists(to))
			return true;
		std::ifstream src(from.c_str(), std::ios::in | std::ios::binary);
		if (src.is_open())
		{
			std::ofstream dst(to.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
			if (dst.is_open())
			{
				dst << src.rdbuf();
				return true;
			}
			LOG_ERROR("[copy] Impossible to create the target file `" << to << "`");
			return false;
		}
		LOG_ERROR("[copy] Impossible to open the source file `" << from << "`");
		return false;
	}



} // namespace Files
} // namespace Paths
} // namespace TA3D

