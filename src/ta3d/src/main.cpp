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

/*
 **  File: main.cpp
 ** Notes: The applications main entry point.
 */

#include <vfs/realfs.h>
#include <vfs/hpi.h>
#include "vfs/VfsService.h"
#include "input/KeyboardService.h"
#include "stdafx.h"			// standard pch inheritance.
#include "TA3D_NameSpace.h" // our namespace, a MUST have.
#include "engine.h"			// The engine class.
//#include "ta3dbase.h"				// Just for the LANG var
#include "EngineClass.h"
#include "backtrace.h" // Some debugging tools
#include "misc/paths.h"
#include "misc/resources.h"
#include "logs/logs.h"
#include "misc/settings.h"
#include "ingame/sidedata.h"
#include "ingame/menus/splash.h"
#include "ingame/menus/intro.h"
#include "ingame/menus/mainmenu.h"
#include "languages/i18n.h"
#include "languages/table.h"
#include "misc/application.h"
#include "sounds/manager.h"
#include "cache.h"
#include "ingame/menus/setupgame.h"
#include "SdlContextManager.h"

using namespace TA3D;

/*
** Function: ParseCommandLine
**    Notes: this will eventually break down any command line arguments passed to
**              the application at run time.  It don't do anything yet, but eventually
**              we will be adding lots of command parms.
**           If something goes wrong you can safely throw a string for an error.
**             The call to this function is tried, but it only catches exceptions
**             and strings, ie throw( "LoadConfigFile: some error occured" );
**           Remember if you throw an error, or generate one, you are responsible for
**             cleaning up what you initialized!
*/
static int ParseCommandLine(int argc, char* argv[])
{
	if (argc > 1)
	{
		// Argument converted to a String
		String arg;

		for (int i = 1; i < argc; ++i)
		{
			arg = argv[i];
			if ("--quick-start" == arg) // Quick restart mecanism (bypass the intro screen)
				lp_CONFIG->quickstart = true;
			else
			{
				if ("--file-param" == arg) // Pass a file as parameter, used for complex things
				{
					if (i + 1 < argc)
					{
						++i;
						lp_CONFIG->file_param = argv[i]; // Copy the file name
					}
				}
				else
				{
					if ("--no-sound" == arg) // Disable sound
						lp_CONFIG->no_sound = true;
				}
			}
		}
	}
	return 0;
}

static void InitializeTheEngine(TA3D::Engine& engine)
{
	// Engine: Start the loading of data in background (thread)
	engine.start();
	// Make the user wait while loading
	Menus::Splash::Execute(engine);

	LOG_INFO("The engine is ready.");
}

int main(int argc, char* argv[])
{
	// initialize SDL
	SdlContextManager sdlContextManager;

	// Constructing config
	TA3D::VARS::lp_CONFIG = new TA3D::TA3DCONFIG;

	// Initialize all modules used by ta3d
	TA3D::Initialize(argc, argv);

	logSdlVersions();

	TA3D::Cache::Clear();

	if (ParseCommandLine(argc, argv))
		return 1;

	try
	{
		// register archives loaders which VFS will use
		TA3D::UTILS::Archive::registerArchiveFinder(Hpi::finder);
		TA3D::UTILS::Archive::registerArchiveLoader(Hpi::loader);
		TA3D::UTILS::Archive::registerArchiveFinder(RealFS::finder);
		TA3D::UTILS::Archive::registerArchiveLoader(RealFS::loader);

		// set up services
		KeyboardService keyboardService;
		keyboardService.initializeKeyboard();

		VfsService vfsService;
		vfsService.reload();

		I18N* i18nService = I18N::Instance();

		// Initializing the TA3D Engine
		TA3D::Engine engine(&keyboardService, &vfsService, i18nService);
		InitializeTheEngine(engine);

		// ok, if we are here, our thread in engine class is running
		// and doing some work loading up alot of crap so while its doing that
		// we are going to show our intro, but first we need to start our timer.
		start = msec_timer; // Initalize timer.

		// Make some initialization which must be done in the main thread only
		engine.initializationFromTheMainThread();

		// while our engine does some loading and intializing, lets show our intro.
		if (!lp_CONFIG->quickstart && lp_CONFIG->file_param.empty())
			Menus::Intro::Execute();

		// The main menu call will eventually not be here, instead
		// we will turn control over to our engine, but for now we'll call the
		// menu this way.
		TA3D::Menus::MainMenu::Execute();

		// if we get here its time to exit, so delete the engine, the engine should clean itself
		//   up and we are outa here, but first lets save our config file, we
		//   need to try/catch this but no worries for now since its not doing anything.
		TA3D::Settings::Save();
	}
	catch (const char* msg)
	{
		criticalMessage(msg);
		return 1;
	}
	catch (const String& msg)
	{
		criticalMessage(msg);
		return 1;
	}
	catch (const std::exception& e)
	{
		criticalMessage(String("Uncaught exception: ") << e.what());
		return 1;
	}

	return 0;
}
