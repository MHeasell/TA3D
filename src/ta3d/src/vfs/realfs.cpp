#include <misc/files.h>
#include <misc/paths.h>
#include <misc/ta3d_math.h>
#include <misc/string.h>
#include <logs/logs.h>
#include "realfs.h"
#include "realfile.h"

namespace TA3D
{
	void RealFS::finder(String::List& fileList, const String& path)
	{
		fileList.push_back(path); // We consider a path to a directory as an archive of the real filesystem
	}

	Archive* RealFS::loader(const String& filename)
	{
		if (!filename.empty() && (filename.last() == '/' || filename.last() == '\\'))
			return new RealFS(filename);
		return NULL;
	}

	RealFS::RealFS(const String& filename)
	{
		open(filename);
	}

	RealFS::~RealFS()
	{
		close();
	}

	void RealFS::open(const String& filename)
	{
		Archive::name = filename;
	}

	void RealFS::close()
	{
		Archive::name.clear();
		if (!files.empty())
		{
			for (HashMap<RealFile*>::Sparse::iterator i = files.begin(); i != files.end(); ++i)
				delete *i;
			files.clear();
		}
	}

	void RealFS::getFileList(std::deque<FileInfo*>& lFiles)
	{
		if (files.empty())
		{
			String root = name;
			root.removeTrailingSlash();
			String::List dirs;
			dirs.push_back(root);
			String::List fileList;
			while (!dirs.empty())
			{
				String current;
				current << dirs.front() << Paths::Separator << "*";
				dirs.pop_front();

				Paths::GlobFiles(fileList, current, false, false);

				Paths::GlobDirs(dirs, current, false, false);
			}

			for (String::List::iterator i = fileList.begin(); i != fileList.end(); ++i)
			{
				if (i->size() <= root.size() + 1 || i->last() == '~' || Paths::ExtractFileExt(*i).toLower() == ".bak") // Don't take useless files into account
					continue;
				*i = Substr(*i, root.size() + 1); // Remove root path + path separator since we don't need them in VFS name and we can add them when accessing the files
				if (!i->empty())
				{
					const String::size_type s = i->find_first_not_of("/\\");
					if (s != String::npos)
						*i = Substr(*i, s);
				}

				if (i->find("cache") != String::npos) // Don't include cache folders (they are huge and useless to us here)
					continue;

				RealFile* file = new RealFile;
				file->pathToFile = *i; // Store full path here
				// make VFS path
				i->convertSlashesIntoBackslashes();
				i->toLower();
				file->setName(*i);
				file->setParent(this);
				file->setPriority(0xFFFF);
				HashMap<RealFile*>::Sparse::iterator it = files.find(*i);
				if (it != files.end()) // On some platform we can have files with the same VFS name (because of different cases resulting in different file names)
					delete *it;
				files[*i] = file;
			}
		}
		for (HashMap<RealFile*>::Sparse::iterator i = files.begin(); i != files.end(); ++i)
			lFiles.push_back(*i);
	}

	File* RealFS::readFile(const String& filename)
	{
		if (!files.empty())
		{
			HashMap<RealFile*>::Sparse::iterator file = files.find(filename);
			if (file != files.end())
				return readFile(*file);
		}
		return NULL;
	}

	File* RealFS::readFile(const FileInfo* file)
	{
		String unixFilename = ((const RealFile*)file)->pathToFile;
		unixFilename.convertBackslashesIntoSlashes();

		String root = name;
		root.removeTrailingSlash();

		unixFilename = String(root) << Paths::SeparatorAsString << unixFilename;

		return new TA3D::RealFile(unixFilename);
	}

	File* RealFS::readFileRange(const String& filename, const uint32 start, const uint32 length)
	{
		HashMap<RealFile*>::Sparse::iterator file = files.find(filename);
		if (file != files.end())
			return readFileRange(*file, start, length);
		else
			return NULL;
	}

	File* RealFS::readFileRange(const FileInfo* file, const uint32, const uint32)
	{
		String unixFilename = ((const RealFile*)file)->pathToFile;
		unixFilename.convertBackslashesIntoSlashes();

		String root = name;
		root.removeTrailingSlash();

		unixFilename = String(root) << Paths::SeparatorAsString << unixFilename;

		return new TA3D::RealFile(unixFilename);
	}

	bool RealFS::needsCaching()
	{
		return false;
	}
}
