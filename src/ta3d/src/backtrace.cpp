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
#include <SDL/SDL_sgui.h>
#include "TA3D_NameSpace.h"
#include "misc/paths.h"
#include <exception>
#include <fstream>

#include "gfx/gui/area.h"
#include "backtrace.h"
#include "network/socket.tcp.h"

#ifdef TA3D_PLATFORM_LINUX
#define TA3D_BUILTIN_BACKTRACE_SUPPORT
#endif

#ifdef TA3D_BUILTIN_BACKTRACE_SUPPORT
#include <execinfo.h>
#endif

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include "sdl.h"
#include "misc/osinfo.h"

/*! \brief a small function implementing an automatic bug reporter
 * Declaration of the bug reporter. It's here because it should be visible only
 * from this module.
 */
void bug_reporter(const TA3D::String& trace);

/*!
 * \brief Obtain a backtrace and print it to stdout.
 *
 * If GDB can be used to get a backtrace then we use it, otherwise and only
 * if TA3D_BUILTIN_BACKTRACE_SUPPORT is defined, a backtrace is obtained
 * then writen into a log file. It will be displayed in stdout when gdb is missing.
 * After this call, the program will exit with a exit status code equals
 * to `-1`.
 *
 * \param signum Which signal was received
 */
void backtrace_handler(int signum)
{
	// Some functions called at exit may crash, so we must disable signals in order
	// to prevent overwriting a useful log
	clear_signals();

	// Get TA3D's PID
	pid_t mypid = getpid();
	// Try to get a stack trace from GDB
	TA3D::String::Vector threads;
	TA3D::System::run_command(TA3D::String("gdb --pid=") << mypid << " -ex \"info threads\" --batch").split(threads, "\n");
	if (!threads.empty())
	{
		TA3D::String cmd;
		cmd << "gdb --pid="
			<< mypid
			<< " -ex \"info threads\"";
		for (size_t i = 0; i < threads.size(); ++i)
		{
			TA3D::String& line = threads[i];
			if (line.startsWith('[') || line.startsWith("0x") || line.startsWith('#'))
				continue;
			if (line.startsWith('*'))
			{
				line[0] = ' ';
				line.trimLeft(' ');
			}
			const int id = line.to_int();
			if (id <= 0)
				continue;
			cmd << " -ex \"thread " << id << "\" -ex bt";
		}
		cmd << " --batch";
		const TA3D::String trace = TA3D::System::run_command(cmd);
		if (!trace.empty())
		{
			bug_reporter(trace);
			exit(-1);
		}
	}

// If GDB is not available or returned an error we must find another way ... this is now platform dependent

#ifdef TA3D_BUILTIN_BACKTRACE_SUPPORT
	// Retrieving a backtrace
	void* array[400];
	int size = backtrace(array, 400);
	char** strings = backtrace_symbols(array, size);

	// Try to log it
	TA3D::String backtrace_filename = TA3D::String(TA3D::Paths::Logs) << "backtrace.txt";
	std::ofstream m_File(backtrace_filename.c_str());
	if (m_File.is_open())
	{
		m_File << "received signal " << strsignal(signum) << "\n";
		m_File << "Obtained " << size << " stack frames.\n";
		for (int i = 0; i < size; ++i)
			m_File << strings[i] << "\n";
		m_File.flush();
		m_File.close();

		printf("received signal %s\n", strsignal(signum));
		printf("Obtained %d stack frames.\n", static_cast<int>(size));
		for (int i = 0; i < size; ++i)
			printf("%s\n", strings[i]);

		TA3D::String szErrReport;
		szErrReport << "An error has occured.\nDebugging information have been logged to:\n"
					<< TA3D::Paths::Logs
					<< "backtrace.txt\nPlease report to our forums (http://www.ta3d.org/)\nand keep this file, it'll help us debugging.\n";

		criticalMessage(szErrReport);
	}
	else
	{
		// The file is not opened
		// The backtrace will be directly to stdout instead.
		printf("received signal %s\n", strsignal(signum));
		printf("couldn't open file for writing!!\n");
		printf("Obtained %d stack frames.\n", static_cast<int>(size));
		for (int i = 0; i < size; ++i)
			printf("%s\n", strings[i]);
	}
	free(strings);

#else // ifdef TA3D_BUILTIN_BACKTRACE_SUPPORT

	// The backtrace support is disabled: warns the user
	TA3D::String szErrReport = "An error has occured.\nDebugging information could not be logged.\nPlease report to our forums (http://www.ta3d.org/) so we can fix it.";
	criticalMessage(szErrReport);

#endif // ifdef TA3D_BUILTIN_BACKTRACE_SUPPORT
	exit(-1);
}

class sigpipe_exception : public std::exception
{
public:
	virtual ~sigpipe_exception() throw() {}
	virtual const char* sigpipe_what() const throw()
	{
		return "broken pipe";
	}
};

void sigpipe_handler(int /*signum*/)
{
	throw sigpipe_exception();
}

void init_signals(void)
{
// On all platforms that supports it, SIGPIPE must
// not end the program since it's likely to happen
// when sockets are disconnected, so let's convert
// it to an exception
#ifdef TA3D_PLATFORM_LINUX
	signal(SIGPIPE, sigpipe_handler);
#endif

// On Linux, get the command of the parent process.
// If we're running in GDB, then don't override it!!
#ifdef TA3D_PLATFORM_LINUX
	// Get TA3D's PID
	pid_t mypid = getpid();
	const TA3D::String ppid = TA3D::System::run_command(TA3D::String("ps -o ppid -p ") << mypid << " | tail -n 1");
	const TA3D::String parent = TA3D::System::run_command(TA3D::String("ps -o command -p ") << ppid);
	if (parent.contains("gdb"))
	{
		std::cerr << "Running under GDB, not overriding signals handlers" << std::endl;
		return;
	}
#endif

#ifdef TA3D_PLATFORM_WINDOWS
	int signum[] = {SIGFPE, SIGILL, SIGSEGV, SIGABRT};
	int nb_signals = 4;
#else
	int signum[] = {SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGABRT, SIGIOT, SIGTRAP, SIGSYS};
	int nb_signals = 8;
#endif // ifdef TA3D_PLATFORM_WINDOWS
	for (int i = 0; i < nb_signals; ++i)
	{
		if (signal(signum[i], backtrace_handler) == SIG_IGN)
			signal(signum[i], SIG_IGN);
	}
}

void clear_signals(void)
{
#ifdef TA3D_PLATFORM_WINDOWS
	int signum[] = {SIGFPE, SIGILL, SIGSEGV, SIGABRT};
	int nb_signals = 4;
#else
	int signum[] = {SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGABRT, SIGIOT, SIGTRAP, SIGSYS};
	int nb_signals = 8;
#endif // ifdef TA3D_PLATFORM_WINDOWS
	for (int i = 0; i < nb_signals; ++i)
		signal(signum[i], SIG_IGN);
}

void criticalMessage(const TA3D::String& msg)
{
	std::cerr << msg << std::endl; // Output error message to stderr

	SDL_SetVideoMode(0, 0, 0, 0);
	Gui::Utils::message("TA3D - Critical Error", msg.c_str());
}

/*!
 * \brief Display a window presenting the crash report to the user
 * The crash report window tells the user something went wrong. It shows the content of the
 * crash report that would be sent to the bug report server. The user can accept to send the
 * report or not.
 */
void bug_reporter(const TA3D::String& trace)
{
	bool bSendReport = false;
	std::string report;

	// Engine version
	report += TA3D_ENGINE_VERSION;
#ifdef TA3D_CURRENT_COMMIT
	report += " g";
	report += TA3D_CURRENT_COMMIT;
#endif
	report += '\n';

	// Current mod
	if (!TA3D_CURRENT_MOD.empty())
	{
		report += "MOD: ";
		report += TA3D_CURRENT_MOD.c_str();
		report += '\n';
	}

	// System info
	report += "\nSystem info:\n";
#ifdef TA3D_PLATFORM_LINUX
	report += "OS: linux\n";
#elif defined TA3D_PLATFORM_WINDOWS
	report += "OS: windows\n";
#else
	report += "OS: unknown\n";
#endif

	// OpenGL info
	report += "OpenGL Informations :\n";
	(report += "Vendor: ") += (const char*)glGetString(GL_VENDOR);
	(report += "\nRenderer: ") += (const char*)glGetString(GL_RENDERER);
	(report += "\nVersion: ") += (const char*)glGetString(GL_VERSION);
	report += "\nExtensions:\n";
	const char* ext = (const char*)glGetString(GL_EXTENSIONS);
	for (; *ext; ++ext)
		report += *ext == ' ' ? '\n' : *ext;
	report += '\n';
	report += '\n';

	report += "\nstacktrace:\n";
	report += trace.c_str();

	Gui::Window wnd("Bug report", 640, 240, Gui::Window::MOVEABLE);
	wnd.addChild(Gui::TabWidget_("tabs") / (Gui::Spacer_(false) | Gui::Button_("ok", " send report ") | Gui::Spacer_(false) | Gui::Button_("cancel", " don't send ") | Gui::Spacer_(false)));

	TABWIDGET(tabs)->addTab("info", Gui::Label_("info") / Gui::Spacer_(true) / Gui::Label_("size"));
	TABWIDGET(tabs)->addTab("report", Gui::ScrollArea_("scroll"));

	SCROLLAREA(scroll)->setCentralWidget(Gui::Label_("text", report));

	BUTTON(ok)->addListener(Gui::Utils::actionSetBool(bSendReport));
	BUTTON(ok)->addListener(Gui::Utils::actionCloseWindow());
	BUTTON(cancel)->addListener(Gui::Utils::actionCloseWindow());

	LABEL(info)->setCaption("An error has occured.\n"
							"A bug report has been prepared. You can review it in the 'report' tab.\n"
							"It contains information about your version of TA3D, OS, OpenGL renderer\n"
							"and a stack trace to help us find what's wrong.\n"
							"\n"
							"Do you want to send the bug report ?");
	TA3D::String buf;
	buf << "(report size = " << report.size() << " bytes)";
	LABEL(size)->setCaption(buf.c_str());

	wnd();

	if (!bSendReport)
		return;

	// Send the bug report to the bug server
	TA3D::SocketTCP sock;
	sock.open("bugs.ta3d.org", 1905);
	if (!sock.isOpen())
	{
		Gui::Utils::message("Socket error", "Error: could not connect to server.");
		return;
	}
	sock.send("BUG REPORT\n");
	sock.send(report);
	sock.send("DISCONNECT");
	// Wait a bit to make sure all data has been received
	SDL_Delay(2000);
	sock.close();
	Gui::Utils::message("Success", "Bug report has been sent to server.");
}
