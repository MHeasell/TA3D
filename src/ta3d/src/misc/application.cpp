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
#include "application.h"
#include "osinfo.h"
#include "paths.h"
#include <logs/logs.h>
#include <languages/i18n.h>
#include "resources.h"
#include <stdlib.h>
#include <TA3D_NameSpace.h>
#include "settings.h"

namespace TA3D
{

	void Finalize()
	{
		LOG_INFO("Aborting now. Releasing all resources...");

		if (TA3D::VARS::lp_CONFIG)
			delete TA3D::VARS::lp_CONFIG;
		TA3D::VARS::lp_CONFIG = NULL;

		I18N::Destroy();
		logs.info() << "Exiting now.";
	}

	void Initialize(int argc, char* argv[])
	{
		// Load and prepare output directories
		if (!TA3D::Paths::Initialize(argc, argv, "ta3d"))
			exit(1);
		TA3D::Resources::Initialize();

		// Install our atexit function
		atexit(Finalize);

		// Interface Manager
		InterfaceManager = IInterfaceManager::Ptr(new IInterfaceManager());

		// Load settings early only to get current mod name
		TA3D::Settings::Load();

		// Load additionnal resource paths
		String::Vector cfgPaths;
		lp_CONFIG->resourcePaths.explode(cfgPaths, ',', true, true, true);
		for (String::Vector::iterator i = cfgPaths.begin(); i != cfgPaths.end(); ++i)
			TA3D::Resources::AddSearchPath(*i);

		// Display usefull infos for debugging
		System::DisplayInformations();
	}

} // namespace TA3D
