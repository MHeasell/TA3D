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

#include "../stdafx.h"
#include "application.h"
#include "osinfo.h"
#include "paths.h"
#include "../logs/logs.h"
#include "../languages/i18n.h"
#include "resources.h"
#include <stdlib.h>
#include "../TA3D_NameSpace.h"
#include "settings.h"




namespace TA3D
{


	void Finalize()
	{
		LOG_INFO("Aborting now. Releasing all resources...");

		// SDL_Quit(); // Should not be needed since SDL_Quit is installed as an atexit() routine

		if (TA3D::VARS::lp_CONFIG)
		{
			delete TA3D::VARS::lp_CONFIG;
			TA3D::VARS::lp_CONFIG = NULL;
		}

		I18N::Destroy();
		LOG_INFO("Exit.");

		// Close the log file
		Logs::logger().closeFile();
	}




	void Initialize(int argc, char* argv[])
	{
		Logs::level = LOG_LEVEL_DEBUG;
		for (int i = 1 ; i < argc; i++)
		{
			if (!strcmp(argv[i], "--quiet") || !strcmp(argv[i], "/quiet"))
				Logs::level = LOG_LEVEL_QUIET;
		}

		// Load and prepare output directories
		if (!TA3D::Paths::Initialize(argc, argv, "ta3d"))
			exit(1);
		TA3D::Resources::Initialize();

		// Install our atexit function before SDL
		// Like this, SDL_Quit() will be called before Finalize()
		atexit(Finalize);
		SDL_Init(SDL_INIT_VIDEO);
		atexit(Finalize);

		// Interface Manager
		InterfaceManager = new IInterfaceManager();

		// Load settings early only to get current mod name
		TA3D::Settings::Load();

		// Display usefull infos for debugging
		System::DisplayInformations();
		System::DisplayInformationsAboutSDL();
	}




} // namespace TA3D
