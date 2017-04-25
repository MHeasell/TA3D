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
#include "splash.h"
#include <misc/paths.h>
#include <vector>
#include <misc/resources.h>
#include <misc/files.h>
#include <gfx/gui/skin.h>
#include <input/mouse.h>
#include <input/keyboard.h>
#include <languages/i18n.h>
#include <TA3D_NameSpace.h>

namespace TA3D
{
	namespace Menus
	{

		bool Splash::Execute(Engine& engine)
		{
			Menus::Splash m(engine);
			return m.execute();
		}

		Splash::Splash(Engine& engine)
			: Abstract(), pEngine(engine), pBackgroundTexture(0)
		{
		}

		Splash::~Splash()
		{
		}

		bool Splash::doInitialize()
		{
			LOG_ASSERT(NULL != gfx);
			LOG_DEBUG(LOG_PREFIX_MENU_INTRO << "Entering...");

			// Load the background
			loadBackgroundTexture();
			// 2D Mode
			gfx->set_2D_mode();
			return true;
		}

		void Splash::doFinalize()
		{
			ResetTexture(pBackgroundTexture);
			LOG_DEBUG(LOG_PREFIX_MENU_INTRO << "Done.");
		}

		void Splash::waitForEvent()
		{
			// Do nothing
			wait(TA3D_MENUS_RECOMMENDED_TIME_MS_FOR_RESTING * 3);
		}

		bool Splash::maySwitchToAnotherMenu()
		{
			// We can switch away from the splash screen
			// once the background loading thread has finished.
			return !pEngine.isRunning();
		}

		void Splash::loadBackgroundTexture()
		{
			LOG_ASSERT(NULL != gfx);

			String filename;
			// The background
			filename << "gfx" << Paths::SeparatorAsString << "splash" << Paths::SeparatorAsString << "loading.jpg";
			ResetTexture(pBackgroundTexture, gfx->load_texture(filename));
		}

		void Splash::redrawTheScreen()
		{
			// Clear the screen
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glLoadIdentity();
			// Background
			gfx->drawtexture(pBackgroundTexture, 0.0f, 0.0f, float(SCREEN_W), float(SCREEN_H));
			// Flip
			gfx->flip();
		}

	} // namespace Menus
} // namespace TA3D
