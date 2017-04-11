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
#include "stdafx.h"
#include "TA3D_NameSpace.h" // our namespace, a MUST have.
#include "ta3dbase.h"
#include "misc/math.h"
#include "misc/paths.h"
#include <memory>
#include <fstream>

namespace TA3D
{

	void install_TA_files(String HPI_file, String filename)
	{
		auto archive = std::unique_ptr<Archive>(Archive::load(HPI_file));
		if (!archive)
		{
			LOG_ERROR("archive not found : '" << HPI_file << "'");
			return;
		}
		std::deque<Archive::FileInfo*> lFiles;
		archive->getFileList(lFiles);
		File* file = archive->readFile(filename); // Extract the file
		if (file)
		{
			String output_filename = String(Paths::Resources) << Paths::ExtractFileName(filename);
			std::ofstream dst(output_filename.c_str(), std::ios::binary);

			if (dst.is_open())
			{
				dst.write(file->data(), file->size());

				dst.flush();
				dst.close();
			}
			delete file;
		}
	}

} // namespace TA3D
