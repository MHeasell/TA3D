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
#include "paths.h"
#include <vfs/realfile.h>
#include <fstream>

namespace TA3D
{
	namespace Paths
	{
		namespace Files
		{

			template <class T>
			bool getline(T& file, String& s)
			{
				s.clear();

				if (file.eof())
					return false;
				while (!file.eof())
				{
					char c;
					file.get(c);
					if (c == '\n')
						break;
					s << c;
				}
				return true;
			}

			template <class T>
			bool TmplLoadFromFile(T& out, const String& filename, const uint32 sizeLimit, const bool emptyListBefore)
			{
				if (emptyListBefore)
					out.clear();
				std::ifstream file(filename.c_str(), std::ios::binary);
				if (!file.is_open())
				{
					LOG_WARNING("Impossible to open the file `" << filename << "`");
					return false;
				}
				if (sizeLimit)
				{
					file.seekg(0);
					ssize_t begin_pos = file.tellg();
					file.seekg(0, file.end);
					if (static_cast<uint32>((file.tellg() - begin_pos)) > sizeLimit)
					{
						LOG_WARNING("Impossible to read the file `" << filename << "` (size > " << sizeLimit << ")");
						return false;
					}
					file.seekg(0);
				}
				String line;
				while (getline(file, line))
					out.push_back(line);
				return true;
			}

			bool Load(String::List& out, const String& filename, const uint32 sizeLimit, const bool emptyListBefore)
			{
				return TmplLoadFromFile<String::List>(out, filename, sizeLimit, emptyListBefore);
			}

			bool Load(String::Vector& out, const String& filename, const uint32 sizeLimit, const bool emptyListBefore)
			{
				return TmplLoadFromFile<String::Vector>(out, filename, sizeLimit, emptyListBefore);
			}

			bool Size(const String& filename, uint64& size)
			{
				// This is a hack as tellg technically isn't guaranteed
				// to return the position in the file in bytes,
				// but in practice it does behave this way
				// on the platforms we care about.
				std::ifstream file(filename.c_str(), std::ios::binary);
				if (file.is_open())
				{
					uint64 start = file.tellg();
					file.seekg(0, file.end);
					uint64 end = file.tellg();
					size = end - start;
					return true;
				}

				return false;
			}

			uint64 Size(const String& filename)
			{
				uint64 size;
				return Size(filename, size) ? size : 0;
			}

			bool Exists(const String& filename)
			{
				std::ifstream file(filename.c_str(), std::ios::binary);
				return file.good();
			}

			File* LoadContentInMemory(const String& filename, const uint64 hardlimit)
			{
				uint64 size;
				if (Size(filename, size))
				{
					if (0 == size)
						return NULL;
					if (size > hardlimit)
					{
						LOG_ERROR("Impossible to load the file `" << filename << "` in memory. Its size exceeds << "
																  << hardlimit / 1204 << "Ko");
						return NULL;
					}
					return new RealFile(filename);
				}
				return NULL;
			}

			bool SaveToFile(const String& filename, const String& content)
			{
				std::ofstream file(filename.c_str(), std::ios::trunc);
				if (!file.is_open())
				{
					return false;
				}

				file << content;
				file.close();
				return true;
			}

			String ReplaceExtension(const String& filename, const String& newExt)
			{
				if (filename.empty())
					return String();
				String::size_type p = filename.find_last_of('.');
				if (p == String::npos)
					return String(filename) << newExt;
				String::size_type s = filename.find_last_of("\\/");
				if (s != String::npos && p < s)
					return String(filename) << newExt;
				return Substr(filename, 0, p) << newExt;
			}

			bool Copy(const String& from, const String& to, const bool overwrite)
			{
				if (overwrite && Exists(to))
				{
					return false;
				}

				std::ifstream in(from.c_str(), std::ios::binary);
				if (!in.is_open())
				{
					return false;
				}

				std::ofstream out(to.c_str(), std::ios::binary);
				if (!out.is_open())
				{
					return false;
				}

				out << in.rdbuf();

				in.close();
				out.close();

				return true;
			}

		} // namespace Files
	}	 // namespace Paths
} // namespace TA3D
