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

#include <stdafx.h>
#include "paths.h"
#ifndef TA3D_PLATFORM_WINDOWS
#include <stdlib.h>
#else
#include <windows.h>
#include <shlobj.h>
#endif
#include <sys/stat.h>
#include <TA3D_NameSpace.h>
#include <logs/logs.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string>
#include <cwchar>

#ifdef TA3D_PLATFORM_WINDOWS
#define FA_FILE 1
#define FA_DIREC 2
#else
#define FA_FILE DT_REG
#define FA_DIREC DT_DIR
#endif

#define FA_ALL (FA_FILE | FA_DIREC)

namespace TA3D
{
	namespace Paths
	{

		String ApplicationRoot;
		String Caches;
		String Savegames;
		String Logs;
		String LogFile;
		String Preferences;
		String ConfigFile;
		String Screenshots;
		String Resources;
#ifdef TA3D_PLATFORM_WINDOWS
		String LocalData;
		String AppData;
#endif

		String CurrentDirectory()
		{
			char* c = getcwd(NULL, 0);
			String ret(c);
			free(c);
			return String(ret);
		}

		namespace
		{

#ifdef TA3D_PLATFORM_WINDOWS

			/*!
			 * \brief Returns the full path to the user's roaming AppData folder
			 * with no trailing slash.
			 */
			String roamingAppData()
			{
				std::unique_ptr<wchar_t[]> pathBuffer(new wchar_t[MAX_PATH + 1]);
				pathBuffer[MAX_PATH] = '\0';

				HRESULT status = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, pathBuffer.get());
				if (status != S_OK) {
					throw std::runtime_error("Failed to locate AppData folder");
				}
				std::wstring utf16Path(pathBuffer.get());

				std::string path;
				utf8::utf16to8(utf16Path.begin(), utf16Path.end(), std::back_inserter(path));
				return String(path);
			}


			void initForWindows()
			{
				AppData = roamingAppData();
				AppData += Separator;

				String userDir = String(AppData) << "TA3D\\";

				Resources = String(userDir) << "resources\\";
				Caches = String(userDir) << "cache\\";
				Savegames = String(userDir) << "savegames\\";
				Logs = String(userDir) << "logs\\";

				Preferences = String(userDir) << "settings\\";
				Screenshots = String(userDir) << "screenshots\\";
			}

#else // ifdef TA3D_PLATFORM_WINDOWS

			void initForDefaultUnixes()
			{
				String home = getenv("HOME");
				home << "/.ta3d/";
				Resources = String(home) << "resources/";
				Caches = String(home) << "cache/";
				Savegames = String(home) << "savegames/";
				Logs = String(home) << "log/";

				Preferences = home;
				Screenshots = String(home) << "screenshots/";
			}

#endif // ifdef TA3D_PLATFORM_WINDOWS

			/*!
			** \brief Initialize the ApplicationRoot variable
			** \param argv0 Equivalent to argv[0] from the main
			*/
			void initApplicationRootPath(const char* argv0)
			{
				LOG_ASSERT(NULL != argv0);

				if (IsAbsolute(argv0))
					ApplicationRoot = ExtractFilePath(argv0);
				else
				{
					ApplicationRoot.clear();
					String r;
					r << CurrentDirectory() << Separator << argv0;
					if (!r.empty())
					{
						ApplicationRoot = ExtractFilePath(r);
					}
				}
				if (ApplicationRoot.endsWith("/./"))
					ApplicationRoot.chop(2);
				else if (ApplicationRoot.endsWith("/."))
					ApplicationRoot.chop(1);
				else if (!ApplicationRoot.empty() && ApplicationRoot.last() != '/' && ApplicationRoot.last() != '\\')
					ApplicationRoot << '/';
			}

		} // namespace

		String ExtractFilePath(const String& p, const bool systemDependant)
		{
			String out;
			if (systemDependant)
			{
				ExtractFilePathDependent(p, out);
			}
			else
			{
				ExtractFilePathIndependent(p, out);
			}
			return out;
		}

		void ExtractFilePathIndependent(const String& p, String& out)
		{
			if (p.empty())
				out.clear();
			String::size_type pos = p.find_last_of(AllSeparators);
			if (String::npos == pos)
				out.clear();
			else
				out.assign(p, pos);
		}

		void ExtractFilePathDependent(const String& p, String& out)
		{
			if (p.empty())
				out.clear();
			String::size_type pos = p.find_last_of(Separator);
			if (String::npos == pos)
				out.clear();
			else
				out.assign(p, pos);
		}

		void ExtractFileNameIndependent(const String& p, String& out)
		{
			if (p.notEmpty())
				out.clear();
			String::size_type pos = p.find_last_of(AllSeparators);
			if (String::npos == pos)
				out.clear();
			else
				out.assign(p.c_str() + pos + 1);
		}

		void ExtractFileNameDependent(const String& p, String& out)
		{
			if (p.notEmpty())
				out.clear();
			String::size_type pos = p.find_last_of(Separator);
			if (String::npos == pos)
				out.clear();
			else
				out.assign(p.c_str() + pos + 1);
		}

		void ExtractFileName(const String& p, String& out, const bool systemDependant)
		{
			if (systemDependant)
			{
				ExtractFileNameDependent(p, out);
			}
			else
			{
				ExtractFileNameIndependent(p, out);
			}
		}

		String ExtractFileName(const String& p, const bool systemDependant)
		{
			String out;
			ExtractFileName(p, out, systemDependant);
			return out;
		}

		void ExtractFileName(String::List& p, const bool systemDependant)
		{
			for (String::List::iterator i = p.begin(); i != p.end(); ++i)
			{
				String out;
				ExtractFileName(out, *i, systemDependant);
				*i = out;
			}
		}

		void ExtractFileName(String::Vector& p, const bool systemDependant)
		{
			for (String::Vector::iterator i = p.begin(); i != p.end(); ++i)
			{
				String out;
				ExtractFileName(out, *i, systemDependant);
				*i = out;
			}
		}

		void ExtractFileNameWithoutExtensionIndependent(const String& p, String& out)
		{
			String::size_type pos = p.find_last_of(AllSeparators);
			String::size_type n = p.find_last_of('.');

			if (String::npos == n && String::npos == pos)
			{
				out = p;
				return;
			}
			if (n == pos)
			{
				out.clear();
				return;
			}
			if (n == String::npos && n > pos + 1)
			{
				if (String::npos == pos)
				{
					out = p;
					return;
				}
				out.assign(p.c_str() + pos + 1);
				return;
			}
			if (pos == String::npos)
			{
				out.assign(p, n);
				return;
			}
			out.assign(p.c_str() + pos + 1, n - pos - 1);
		}

		void ExtractFileNameWithoutExtensionDependent(const String& p, String& out)
		{
			String::size_type pos = p.find_last_of(Separator);
			String::size_type n = p.find_last_of('.');

			if (String::npos == n && String::npos == pos)
			{
				out = p;
				return;
			}
			if (n == pos)
			{
				out.clear();
				return;
			}
			if (n == String::npos && n > pos + 1)
			{
				if (String::npos == pos)
				{
					out = p;
					return;
				}
				out.assign(p.c_str() + pos + 1);
				return;
			}
			if (pos == String::npos)
			{
				out.assign(p, n);
				return;
			}
			out.assign(p.c_str() + pos + 1, n - pos - 1);
		}

		void ExtractFileNameWithoutExtension(const String& p, String& out, const bool systemDependant)
		{
			if (systemDependant)
			{
				ExtractFileNameWithoutExtensionDependent(p, out);
			}
			else
			{
				ExtractFileNameWithoutExtensionIndependent(p, out);
			}
		}

		String ExtractFileNameWithoutExtension(const String& p, const bool systemDependant)
		{
			String out;
			ExtractFileNameWithoutExtension(p, out, systemDependant);
			return out;
		}

		bool ExtractExtension(const String& filename, String& out)
		{
			out.clear();

			// If the string is empty, the buffer may be invalid (NULL)
			if (filename.size())
			{
				String::size_type i = filename.size();
				do
				{
					--i;
					switch (filename[i])
					{
						case '.':
						{
							out.append(filename.c_str() + i, filename.size() - i);
							return true;
						}
						case '/':
						case '\\':
							return false;
					}
				} while (i != 0);
			}
			return false;
		}

		String ExtractFileExt(const String& s)
		{
			String t;
			ExtractExtension(s, t);
			return t;
		}

		bool Initialize(int /*argc*/, char* argv[], const String& programName)
		{
			LOG_ASSERT(NULL != argv);
			LOG_ASSERT(!programName.empty());

			initApplicationRootPath(argv[0]);

#ifdef TA3D_PLATFORM_WINDOWS
			initForWindows();
#else
			initForDefaultUnixes();
#endif

			// Initialize the logging mecanism
			ResetTheLoggingMecanism(String(Paths::Logs) << programName << ".log");

			// Welcome Commander !
			logs.checkpoint() << "Welcome to TA3D";
			logs.checkpoint() << "Version: " << TA3D_VERSION_HI << "." << TA3D_VERSION_LO << "." << TA3D_VERSION_PATCH
							  << " (commit " << TA3D_CURRENT_COMMIT << ")";
			logs.info();

			LOG_INFO(LOG_PREFIX_PATHS << "Started from: `" << ApplicationRoot << "`");
			ConfigFile = Preferences;
			ConfigFile += "ta3d.cfg";
			LOG_INFO(LOG_PREFIX_PATHS << "Preferences: `" << Preferences << "`");
			LOG_INFO(LOG_PREFIX_PATHS << "Cache: `" << Caches << "`");
			LOG_INFO(LOG_PREFIX_PATHS << "Savegames: `" << Savegames << "`");
			LOG_INFO(LOG_PREFIX_PATHS << "Screenshots: `" << Screenshots << "`");
			LOG_INFO(LOG_PREFIX_PATHS << "Logs: `" << Logs << "`");

			// Informations about the log file
			if (!logs.logFileIsOpened())
				logs.error() << "Logs: Impossible to open `" << logs.logfile() << "`";
			else
				logs.info() << "Opened the log file: `" << logs.logfile() << "`";

			bool res = MakeDir(Caches) && MakeDir(Savegames) && MakeDir(Logs) && MakeDir(Preferences) && MakeDir(Screenshots) && MakeDir(Resources) && MakeDir(String(Savegames) << "multiplayer" << Paths::Separator);
			if (!res)
			{
				logs.fatal() << "Some paths are missing. Aborting now...";
				exit(120);
			}

			return res;
		}

		bool Exists(const String& p)
		{
			if (p.empty())
				return false;
#ifdef TA3D_PLATFORM_WINDOWS
			// ugly workaround with stat under Windows
			// FIXME: Find a better way to find driver letters
			if (p.size() == 2 && ':' == p[1])
				return true;
			struct _stat s;
			if ('\\' != p[p.size() - 1])
				return (_stat(p.c_str(), &s) == 0);
			return (_stat(String(p, 0, p.size() - 1).c_str(), &s) == 0);
#else
			struct stat s;
			return (stat(p.c_str(), &s) == 0);
#endif
		}


#ifdef TA3D_PLATFORM_WINDOWS

		bool Remove(const char* path)
		{
			String pathString(path);
			if (pathString.empty())
				return false;
			int cr;

			std::wstring widePathString;
			utf8::utf8to16(pathString.begin(), pathString.end(), std::back_inserter(widePathString));

			// construct a double null-terminated string
			// to pass to the Windows API.
			wchar_t* fromBuffer = new wchar_t[widePathString.size() + 2];
			wcscpy(fromBuffer, widePathString.c_str());
			fromBuffer[widePathString.size()] = L'\0';
			fromBuffer[widePathString.size() + 1] = L'\0';

			SHFILEOPSTRUCTW shf;
			shf.hwnd = NULL;

			shf.wFunc = FO_DELETE;
			shf.pFrom = fromBuffer;
			shf.pTo = NULL;
			shf.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI;

			cr = SHFileOperationW(&shf);

			delete[] fromBuffer;

			return (!cr);
		}

#else


		namespace // Anonymous namespace
		{

			bool RmDirRecursiveInternal(const char path[])
			{
				DIR* dp;
				struct dirent* ep;
				String buffer;
				struct stat st;

				if (NULL != (dp = ::opendir(path)))
				{
					while (NULL != (ep = ::readdir(dp)))
					{
						buffer.clear() << path << Separator << (const char*)ep->d_name;
						if (0 == ::stat(buffer.c_str(), &st))
						{
							if (S_ISDIR(st.st_mode))
							{
								if (strcmp(".", (ep->d_name)) != 0 && strcmp("..", (ep->d_name)) != 0)
								{
									RmDirRecursiveInternal(buffer.c_str());
									::rmdir(buffer.c_str());
								}
							}
							else
								::unlink(buffer.c_str());
						}
					}
					(void)::closedir(dp);
				}
				return (0 == rmdir(path));
			}

		} // anonymous namespace


		bool Remove(const char path[])
		{
			if (NULL == path || '\0' == *path)
				return true;
			return RmDirRecursiveInternal(path);
		}


#endif


		void RemoveDir(const String& p)
		{
			Remove(p.c_str());
		}

		bool MakeDir(const String& p)
		{
			if (p.empty())
				return true;
			// TODO Use the boost library, which has a better implementation that this one
			String::Vector parts;
			p.explode(parts, SeparatorAsString, false);
			String pth;
			bool hasBeenCreated(false);
			if (p[0] == '/' || p[0] == '\\')
				pth += Separator;

			for (String::Vector::const_iterator i = parts.begin(); i != parts.end(); ++i)
			{
				pth += *i;
#ifndef TA3D_PLATFORM_WINDOWS
				pth += Separator;
#endif
				if (!Exists(pth))
				{
					LOG_DEBUG(LOG_PREFIX_PATHS << "`" << pth << "` does not exist !");
#ifdef TA3D_PLATFORM_WINDOWS
					if (mkdir(pth.c_str()))
#else
					if (mkdir(pth.c_str(), 0755))
#endif
					{
						// TODO Use the logging system instead
						LOG_ERROR(LOG_PREFIX_PATHS << "Impossible to create the folder `" << pth << "`");
						return false;
					}
					else
						hasBeenCreated = true;
				}
#ifdef TA3D_PLATFORM_WINDOWS
				pth += Separator;
#endif
			}
			if (hasBeenCreated)
				LOG_INFO(LOG_PREFIX_PATHS << "Created folder: `" << p << "`");
			return true;
		}

		template <class T>
		bool TmplGlob(T& out, const String& pattern, const bool emptyListBefore, const uint32 fileAttribs = FA_ALL, const uint32 required = 0, const bool relative = false)
		{
			if (emptyListBefore)
				out.clear();

			String root = ExtractFilePath(pattern);
			String root_path = root;
			if (root.size() > 1 && (root.last() == '/' || root.last() == '\\'))
				root_path.removeLast();
			else if (!root.empty())
				root << Separator;

#ifdef TA3D_PLATFORM_WINDOWS
			String strFilePath;				 // Filepath
			String strExtension;			 // Extension
			HANDLE hFile;					 // Handle to file
			WIN32_FIND_DATA FileInformation; // File information

			hFile = ::FindFirstFile(pattern.c_str(), &FileInformation);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (FileInformation.cFileName[0] != '.')
					{
						String name = (const char*)FileInformation.cFileName;

						if ((FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (fileAttribs & FA_DIREC) && !(required & FA_FILE))
						{
							if (relative)
								out.push_back(name);
							else
								out.push_back(String(root) << name);
						}
						else if (!(required & FA_DIREC) && (fileAttribs & FA_FILE))
						{
							if (relative)
								out.push_back(name);
							else
								out.push_back(String(root) << name);
						}
					}
				} while (::FindNextFile(hFile, &FileInformation) == TRUE);

				// Close handle
				::FindClose(hFile);
			}

#else /* ifdef WINDOWS */

			(void)fileAttribs;
			String filename_pattern = ExtractFileName(pattern);
			filename_pattern.toUpper();
			DIR* dp;
			struct dirent* dirp;
			if ((dp = opendir(root_path.c_str())) == NULL)
			{
				// Following line is commented out because it may be useful later, but for now it only floods the logs
				//            LOG_ERROR( LOG_PREFIX_PATHS << "opening " << root << " failed: " << strerror( errno ) );
				return true;
			}

			while ((dirp = readdir(dp)) != NULL)
			{
				String name = (char*)(dirp->d_name);
				if (dirp->d_type == 0)
				{
					DIR* dp2;
					if ((dp2 = opendir((String(root) << name).c_str())))
					{
						closedir(dp2);
						dirp->d_type |= FA_DIREC;
					}
					else
						dirp->d_type |= FA_FILE;
				}

				if ((dirp->d_type & required) == required && name != "." && name != ".." && ToUpper(name).glob(filename_pattern))
				{
					if (relative)
						out.push_back(name);
					else
						out.push_back(String(root) << name);
				}
			}
			closedir(dp);
#endif

			return !out.empty();
		}

		bool Glob(String::List& out, const String& pattern, const bool emptyListBefore, const bool relative)
		{
			return TmplGlob<String::List>(out, pattern, emptyListBefore, FA_ALL, 0, relative);
		}

		bool Glob(String::Vector& out, const String& pattern, const bool emptyListBefore, const bool relative)
		{
			return TmplGlob<String::Vector>(out, pattern, emptyListBefore, FA_ALL, 0, relative);
		}

		bool GlobFiles(String::List& out, const String& pattern, const bool emptyListBefore, const bool relative)
		{
			return TmplGlob<String::List>(out, pattern, emptyListBefore, FA_FILE, FA_FILE, relative);
		}

		bool GlobFiles(String::Vector& out, const String& pattern, const bool emptyListBefore, const bool relative)
		{
			return TmplGlob<String::Vector>(out, pattern, emptyListBefore, FA_FILE, FA_FILE, relative);
		}

		bool GlobDirs(String::List& out, const String& pattern, const bool emptyListBefore, const bool relative)
		{
			return TmplGlob<String::List>(out, pattern, emptyListBefore, FA_ALL, FA_DIREC, relative);
		}

		bool GlobDirs(String::Vector& out, const String& pattern, const bool emptyListBefore, const bool relative)
		{
			return TmplGlob<String::Vector>(out, pattern, emptyListBefore, FA_ALL, FA_DIREC, relative);
		}

		bool IsAbsolute(const String& p)
		{
#ifdef TA3D_PLATFORM_WINDOWS
			return (p.empty() || (p.size() > 2 && ':' == p[1] && ('\\' == p[2] || '/' == p[2])));
#else
			return ('/' == p.first());
#endif
		}

	} // namespace Paths
} // namespace TA3D
