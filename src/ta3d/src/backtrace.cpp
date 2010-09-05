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
#include <iostream>
#include "misc/string.h"


// Signals should be disabled under OS X, since the system already produces a crash report
// More information are available here :
// http://developer.apple.com/technotes/tn2004/tn2123.html
#ifndef TA3D_PLATFORM_DARWIN


# include "TA3D_NameSpace.h"
# include "misc/paths.h"
# include "gfx/gui/area.h"
# include "backtrace.h"

# ifdef TA3D_PLATFORM_LINUX
#	define TA3D_BACKTRACE_SUPPORT
# endif

# ifdef TA3D_BACKTRACE_SUPPORT
#	include <execinfo.h>
# endif

# include <signal.h>
# include <stdio.h>
# include <stdlib.h>
# include <yuni/core/io/file/stream.h>
# include "sdl.h"
# include <SDL/SDL_sgui.h>

/*!
 * \brief Obtain a backtrace and print it to stdout.
 *
 * Only if TA3D_BACKTRACE_SUPPORT is defined, a backtrace is obtained
 * then writen into a log file. It will be displayed in stdout in any case.
 * After this call, the program will exit with a exit status code equals
 * to `-1`.
 *
 * \param signum Which signal was received
 */
void backtrace_handler (int signum)
{
    # ifdef TA3D_BACKTRACE_SUPPORT

    // Retrieving a backtrace
	void *array[400];
	size_t size = backtrace (array, 400);
	char** strings = backtrace_symbols(array, size);

    // Try to log it
	Yuni::Core::IO::File::Stream m_File(TA3D::Paths::Logs + "backtrace.txt", Yuni::Core::IO::OpenMode::write);
	if(m_File.opened())
    {
		m_File << "received signal " << strsignal( signum ) << "\n";
		m_File << "Obtained " << size << " stack frames.\n";
		for (TA3D::uint32 i = 0; i < size; ++i)
			m_File << strings[i] << "\n";
		m_File.flush();
		m_File.close();

		printf("received signal %s\n", strsignal( signum ));
		printf ("Obtained %zd stack frames.\n", size);
		for (TA3D::uint32 i = 0; i < size; ++i)
			printf ("%s\n", strings[i]);

        String szErrReport = "An error has occured.\nDebugging information have been logged to:\n"
            + TA3D::Paths::Logs
            + "backtrace.txt\nPlease report to our forums (http://www.ta3d.org/)\nand keep this file, it'll help us debugging.\n";

		criticalMessage(szErrReport);
	}
	else
    {
        // The file is not opened
        // The backtrace will be directly to stdout instead.
		printf("received signal %s\n", strsignal(signum));
		printf("couldn't open file for writing!!\n");
		printf ("Obtained %zd stack frames.\n", size);
		for (TA3D::uint32 i = 0; i < size; ++i)
			printf ("%s\n", strings[i]);
	}
	free(strings);

    # else // ifdef TA3D_BACKTRACE_SUPPORT

        // The backtrace support is disabled: warns the user
		String szErrReport = "An error has occured.\nDebugging information could not be logged.\nPlease report to our forums (http://www.ta3d.org/) so we can fix it.";
		criticalMessage(szErrReport);

    # endif // ifdef TA3D_BACKTRACE_SUPPORT
	exit(-1);
}

#endif // ifdef TA3D_PLATFORM_DARWIN





int init_signals (void)
{
	// Signals should be disabled under OS X, since the system already produces a crash report
	// More information are available here :
	// http://developer.apple.com/technotes/tn2004/tn2123.html
	#ifndef TA3D_PLATFORM_DARWIN

	# ifdef TA3D_PLATFORM_WINDOWS
		int signum[] = { SIGFPE, SIGILL, SIGSEGV, SIGABRT };
		int nb_signals = 4;
	# else
		int signum[] = { SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGABRT, SIGIOT, SIGTRAP, SIGSYS };
		int nb_signals = 8;
	# endif // ifdef TA3D_PLATFORM_WINDOWS
	for (int i = 0; i < nb_signals; ++i)
	{
		if (signal (signum[i], backtrace_handler) == SIG_IGN)
			signal (signum[i], SIG_IGN);
	}

	#endif // ifdef TA3D_PLATFORM_DARWIN
	return 0; // TODO missing value ?
}


void criticalMessage(const String &msg)
{
	std::cerr << msg << std::endl;      // Output error message to stderr

	Gui::Utils::message("TA3D - Critical Error", msg.c_str());
}


