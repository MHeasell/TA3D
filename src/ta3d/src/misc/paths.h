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

#ifndef _TA3D_TOOLS_PATHS_H__
#define _TA3D_TOOLS_PATHS_H__

#include <stdafx.h>
#include "string.h"
#include <vector>
#include <list>

namespace TA3D
{

	/*!
	 * \brief Path manipulation functions
	 */
	namespace Paths
	{
		//! \name Relevant paths used by TA3D
		//@{

		//! Absolute path where the application is located (extracted from argv)
		extern String ApplicationRoot;

		//! Folder for Caches
		extern String Caches;

		//! Folder for saved games
		extern String Savegames;

		//! Folder for logs
		extern String Logs;

		/*!
		 * \brief Current log file
		 * @see logs::writeToFile()
		 */
		extern String LogFile;

		//! Folder for preferences
		extern String Preferences;

		//! Configuration file
		extern String ConfigFile;

		//! Folder for screenshots
		extern String Screenshots;

		//! Folder for game resources
		extern String Resources;

		//@}

		//! \name System-dependent variables
		//@{

#ifdef TA3D_PLATFORM_WINDOWS
		//! The path-separator character according to the platform
		const char Separator = '\\';
#else
		//! The path-separator character according to the platform
		const char Separator = '/';
#endif

		const char AllSeparators[] = {'/', '\\'};

#ifdef TA3D_PLATFORM_WINDOWS
		//! The path-separator character according to the platform (stored in a string instead of a char)
		const String SeparatorAsString = "\\";
#else
		//! The path-separator character according to the platform (stored in a string instead of a char)
		const String SeparatorAsString = "/";
#endif

#ifdef TA3D_PLATFORM_WINDOWS
		//! The user's Roaming AppData folder (Windows only)
		extern String AppData;
#endif

		//@}

		//! \name Folders handling
		//@{

		/*!
		 * \brief Test if a file/folder exists
		 * \param p The folder/filename to test
		 * \return True if it exists, false otherwise
		 */
		bool Exists(const String& p);

		/*!
		 * \brief Create Path Recursively
		 *
		 * \param p The path to create if it does not exist
		 * \return True if the operation succeeded, false otherwise
		 */
		bool MakeDir(const String& p);

		/*!
		 * \brief Remove Folder Recursively
		 *
		 * \param p The folder to remove
		 */
		void RemoveDir(const String& p);

		/*!
		 * \brief Retrieve the current directory
		 */
		String CurrentDirectory();

		/*!
		 * \brief Extract the path part of a filename
		 *
		 * The path part will be extracted according the system-dependant path-separator
		 *
		 * \param systemDependant Consider only the system-dependant path-separator
		 *
		 * \code
		 *      std::cout << Paths::ExtractFilePath("/tmp/foo.txt") std::endl; // write `/tmp/`
		 *      std::cout << Paths::ExtractFilePath("/tmp/") std::endl; // write `/tmp/`
		 *      std::cout << Paths::ExtractFilePath("/tmp") std::endl; // write `/`
		 * \endcode
		 *
		 * \see Paths::Separator
		 */
		String ExtractFilePath(const String& p, const bool systemDependant = false);

		/*!
		 * \brief Extract the bare file name
		 *
		 * The file name will be extracted according the last occurence
		 * of the system-dependant path-separator (if systemDependant = true)
		 *
		 * \param systemDependant Consider only the system-dependant path-separator
		 *
		 * \see Paths::Separator
		 */
		String ExtractFileName(const String& p, const bool systemDependant = false);
		void ExtractFileName(String::List& p, const bool systemDependant = false);
		void ExtractFileName(String::Vector& p, const bool systemDependant = false);

		void ExtractFilePathIndependent(const String& p, String& out);
		void ExtractFilePathDependent(const String& p, String& out);

		/*!
		 * \brief Extract the bare file name without its extension
		 *
		 * The file name will be extracted according the last occurence
		 * of the system-dependant path-separator
		 *
		 * \see Paths::Separator
		 */
		String ExtractFileNameWithoutExtension(const String& p, const bool systemDependant = false);

		/*!
		 * \brief Extract the extention of a file name
		 * \param s Filename
		 * \return The extenion of the filename (with the leading '.') in lowercase, empty if no extension is present
		 *
		 * \code
		 *     std::cout << Paths::Files::ExtractFileExt("foo.exe") << std::endl; // '.exe'
		 *     std::cout << Paths::Files::ExtractFileExt("/usr/folder.foo/file") << std::endl; // ''
		 * \endcode
		 */
		String ExtractFileExt(const String& s);

		/*!
		 * \brief Get if a path is absolute
		 *
		 * \param p The path or the filename to test
		 * \return True if the path is an absolute path or empty, false otherwise
		 */
		bool IsAbsolute(const String& p);

		//@}

		//! \name Globbing
		//@{

		/*!
		 * \brief Find pathnames matching a pattern
		 *
		 * \param[out] out The list of file that has been found
		 * \param pattern The pattern to use
		 * \param emptyListBefore True to empty the list before performing the glob function
		 * \return True if the operation succeeded and the list is not empty,
		 * false othewise
		 *
		 * \code
		 * String::Vector list;
		 * if (Paths::Glob(list, Paths::Savegames + "*.sav"))
		 * {
		 *      for (String::Vector::const_iterator i = list.begin(); i != list.end(); ++i)
		 *          std::cout << "Savegame found: `" << *i << std::endl;
		 * }
		 * else
		 * {
		 *      std::cerr << "No savegame found." << std::endl;
		 * }
		 * \endcode
		 */
		bool Glob(String::Vector& out, const String& pattern, const bool emptyListBefore = true, const bool relative = false);
		bool Glob(String::List& out, const String& pattern, const bool emptyListBefore = true, const bool relative = false);

		/*!
		 * \brief Find files matching a pattern
		 *
		 * \param[out] out The list of file that has been found
		 * \param pattern The pattern to use
		 * \param emptyListBefore True to empty the list before performing the glob function
		 * \return True if the operation succeeded and the list is not empty,
		 * false othewise
		 *
		 * \code
		 * String::Vector list;
		 * if (Paths::GlobFiles(list, Paths::Savegames + "*.sav"))
		 * {
		 *      for (String::Vector::const_iterator i = list.begin(); i != list.end(); ++i)
		 *          std::cout << "Savegame found: `" << *i << std::endl;
		 * }
		 * else
		 * {
		 *      std::cerr << "No savegame found." << std::endl;
		 * }
		 * \endcode
		 */
		bool GlobFiles(String::Vector& out, const String& pattern, const bool emptyListBefore = true, const bool relative = false);
		bool GlobFiles(String::List& out, const String& pattern, const bool emptyListBefore = true, const bool relative = false);

		/*!
		 * \brief Find directories matching a pattern
		 *
		 * \param[out] out The list of directory that has been found
		 * \param pattern The pattern to use
		 * \param emptyListBefore True to empty the list before performing the glob function
		 * \return True if the operation succeeded and the list is not empty,
		 * false othewise
		 *
		 * \code
		 * String::Vector list;
		 * if (Paths::GlobDirs(list, Paths::LocalData))
		 * {
		 *      for (String::Vector::const_iterator i = list.begin(); i != list.end(); ++i)
		 *          std::cout << "Sub directory found: `" << *i << std::endl;
		 * }
		 * else
		 * {
		 *      std::cerr << "No sub directory found." << std::endl;
		 * }
		 * \endcode
		 */
		bool GlobDirs(String::Vector& out, const String& pattern, const bool emptyListBefore = true, const bool relative = false);
		bool GlobDirs(String::List& out, const String& pattern, const bool emptyListBefore = true, const bool relative = false);

		//@} // Globbing

		/*!
		 * \brief Load all informations about paths
		 *
		 * \param programName Arbitrary name for the program. It will be used
		 * to create the log file.
		 *
		 * return False if any error has occured
		 */
		bool Initialize(int argc, char* argv[], const String& programName);

	} // namespace Paths
} // namespace TA3D

#endif // _TA3D_TOOLS_PATHS_H__
