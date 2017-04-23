/*  TA3D, a remake of Total Annihilation
	Copyright (C) 2006  Roland BROCHARD

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

#include <thread>
#include <memory>
#include <vfs/hpi.h>
#include <vfs/realfs.h>

#include "stdafx.h"
#include "TA3D_NameSpace.h"
#include "threads/thread.h"
#include "engine.h"
#include "ta3dbase.h"

#include "UnitEngine.h" // The Unit Engine

#include "gfx/fx.h" // Special FX engine
#include "languages/i18n.h"
#include "sounds/manager.h"
#include "input/mouse.h"
#include "input/keyboard.h"
#include "backtrace.h"

namespace TA3D
{
	Synchronizer Engine::synchronizer(4);

	Engine::Engine(
		KeyboardService* keyboardService,
		TA3D::VfsService* vfsService,
		I18N* i18nService,
		GFX* graphicsService,
		Audio::Manager* audioService,
		TA3DCONFIG* config,
		ModelManager* modelManager,
		UnitManager* unitManager,
		FeatureManager* featureManager,
		WeaponManager* weaponManager,
		FXManager* fxManager,
		Math::PreCachedRandomNumbers* randomTable,
		SideData* sideDataService,
		MouseService* mouseService
	)
		: keyboardService(keyboardService),
		  i18nService(i18nService),
		  graphicsService(graphicsService),
		  audioService(audioService),
		  config(config),
		  modelManager(modelManager),
		  unitManager(unitManager),
		  featureManager(featureManager),
		  weaponManager(weaponManager),
		  fxManager(fxManager),
		  randomTable(randomTable),
		  sideDataService(sideDataService),
		  mouseService(mouseService)
	{
	}

	Engine::~Engine(void)
	{
		join();
		cursor.clear();
		sideDataService->destroy();
	}

	void Engine::initializationFromTheMainThread()
	{
		// Load the default textures
		graphicsService->loadDefaultTextures();

		// Load fonts (it crashes on some systems if not run from the main thread)
		graphicsService->loadFonts();

		// Initialize the mouse handler
		LOG_INFO("Initializing the mouse device handler");
		mouseService->init_mouse();
		// Initialize the keyboard handler
		LOG_INFO("Initializing the keyboard device handler");
		keyboardService->initializeKeyboard();
	}

	void Engine::proc(void* /* param */)
	{
		// Creating translation manager
		i18nService->loadFromFile("gamedata\\translate.tdf", true, true);
		i18nService->loadFromResources();

		// Apply settings for the current language (required since it failed when loading settings because languages were not loaded)
		if (!config->Lang.empty())
			i18nService->currentLanguage(config->Lang);
		else
		{
			LOG_INFO(LOG_PREFIX_I18N << "language not set, guessing from system config");
			if (!i18nService->tryToDetermineTheLanguage())
			{
				LOG_INFO(LOG_PREFIX_I18N << "language detection failed, language set to 'english'");
				config->Lang = "english";
				i18nService->currentLanguage(config->Lang);
			}
			else
				config->Lang = i18nService->currentLanguage()->englishCaption();
		}

		audioService->loadTDFSounds(true);
		audioService->loadTDFSounds(false);

		modelManager->init();
		unitManager->init();
		featureManager->init();
		weaponManager->init();
		fxManager->init();
		randomTable->reset();

		sideDataService->init();
		sideDataService->loadData();

		audioService->loadTDFSounds(false);
	}

	void rest(uint32 msec)
	{
		::SDL_Delay(msec);
	}
} // namespace TA3D
