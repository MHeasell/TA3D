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
		SideData* sideDataService
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
		  sideDataService(sideDataService)
	{
		// How many CPU we've got ?
		LOG_INFO("CPU: " << std::thread::hardware_concurrency());

		LOG_INFO(TA3D_ENGINE_VERSION << " initializing started:");
		LOG_INFO("Build info : " << __DATE__ << " , " << __TIME__);

		if (!vfsService->fileExists("gamedata\\sidedata.tdf") || !vfsService->fileExists("gamedata\\allsound.tdf") || !vfsService->fileExists("gamedata\\sound.tdf"))
		{
			showError("RESOURCES ERROR");
			exit(1);
		}

		// Display informations about OpenGL
		displayInfosAboutOpenGL();
	}

	Engine::~Engine(void)
	{
		join();
		cursor.clear();
		ta3dSideData.destroy();
	}

	void Engine::initializationFromTheMainThread()
	{
		// Load the default textures
		graphicsService->loadDefaultTextures();

		// Load fonts (it crashes on some systems if not run from the main thread)
		graphicsService->loadFonts();

		// Initialize the mouse handler
		LOG_INFO("Initializing the mouse device handler");
		init_mouse();
		// Initialize the keyboard handler
		LOG_INFO("Initializing the keyboard device handler");
		keyboardService->initializeKeyboard();

		if (!audioService->isRunning() && !config->quickstart)
			showWarning("FMOD WARNING");
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

	void Engine::displayInfosAboutOpenGL() const
	{
		logs.checkpoint() << "OpenGL Informations :";
		logs.info() << "Vendor: " << (const char*)glGetString(GL_VENDOR);
		logs.info() << "Renderer: " << (const char*)glGetString(GL_RENDERER);
		logs.info() << "Version: " << (const char*)glGetString(GL_VERSION);
		if (graphicsService->atiWorkaround())
			LOG_WARNING("ATI or SIS card detected ! Using workarounds for ATI/SIS cards");
		LOG_INFO(LOG_PREFIX_OPENGL << "Texture compression: " << (g_useTextureCompression ? "Yes" : "No"));
		LOG_INFO(LOG_PREFIX_OPENGL << "Stencil Two Side: " << (g_useStencilTwoSide ? "Yes" : "No"));
		LOG_INFO(LOG_PREFIX_OPENGL << "FBO: " << (g_useFBO ? "Yes" : "No"));
		LOG_INFO(LOG_PREFIX_OPENGL << "Multi texturing: " << (MultiTexturing ? "Yes" : "No"));
	}

	void Engine::showError(const String& s, const String& additional) const
	{
		LOG_ERROR(i18nService->Translate(s));
		criticalMessage(i18nService->Translate(s) << additional);
	}

	void Engine::showWarning(const String& s, const String& additional) const
	{
		LOG_WARNING(i18nService->Translate(s));
		auto pArea = std::unique_ptr<Gui::AREA>(new Gui::AREA());
		pArea->load_tdf("gui/empty.area");
		pArea->popup(i18nService->Translate("Warning"), i18nService->Translate(s) << additional);
	}
} // namespace TA3D
