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

#ifndef __TA3D_ENGINE_H__
#define __TA3D_ENGINE_H__

#include <TA3D_NameSpace.h>
#include <languages/i18n.h>
#include <sounds/manager.h>
#include <mesh/mesh.h>
#include <gfx/fx.h>
#include <input/MouseService.h>
#include "threads/thread.h"
#include "input/KeyboardService.h"
#include "vfs/VfsService.h"
#include "fbi.h"

namespace TA3D
{

	class Engine : public Thread
	{
		// These classes need access to the Synchronizer object
		friend class INGAME_UNITS;
		friend class InGameWeapons;
		friend class PARTICLE_ENGINE;
		friend class PLAYERS;
		friend class Battle;

	public:
		//! \name Constructor & Destructor
		//@{
		//! Default Constructor
		Engine(
			KeyboardService* keyboardService,
			VfsService* vfsService,
			I18N* i18nService,
			GFX* graphicsService,
			AudioManager* audioService,
			TA3DCONFIG* config,
			ModelManager* modelManager,
			UnitManager* unitManager,
			FeatureManager* featureManager,
			WeaponManager* weaponManager,
			FXManager* fxManager,
			Math::PreCachedRandomNumbers* randomTable,
			SideData* sideDataService,
			MouseService* mouseService
		);
		//! Destructor
		virtual ~Engine();
		//@}

		/*!
		** \brief Make initialization (from the main thread only) after the engine was initialized from another thread
		*/
		void initializationFromTheMainThread();

	protected:
		virtual void proc(void* param);

	private:
		KeyboardService* keyboardService;
		I18N* i18nService;
		GFX* graphicsService;
		AudioManager* audioService;
		TA3DCONFIG* config;
		ModelManager* modelManager;
		UnitManager* unitManager;
		FeatureManager* featureManager;
		WeaponManager* weaponManager;
		FXManager* fxManager;
		Math::PreCachedRandomNumbers* randomTable;
		SideData* sideDataService;
		MouseService* mouseService;

	private:
		static void sync()
		{
			synchronizer.sync();
		}

	private:
		static Synchronizer synchronizer;
	}; // class Engine

} // namespace TA3D

#endif // __TA3D_ENGINE_H__
