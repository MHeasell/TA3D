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

#include "battle.h"
#include "players.h"
#include <UnitEngine.h>
#include <gfx/fx.h>
#include <gfx/gfx.toolkit.h>
#include <input/keyboard.h>
#include <input/mouse.h>
#include <misc/paths.h>

namespace TA3D
{

	void Battle::initRenderer()
	{
		gfx->SetDefState();

		render_time = ((float)units.current_tick) / TICKS_PER_SEC;

		// Copy unit data from simulation
		units.renderTick();

		glActiveTextureARB(GL_TEXTURE7_ARB);
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	void Battle::renderStencilShadow()
	{
		pSun.Dir.x = -1.0f;
		pSun.Dir.y = 1.0f;
		pSun.Dir.z = 1.0f;
		pSun.Dir.normalize();
		units.draw_shadow(render_time, -pSun.Dir);
	}


	void Battle::renderWorld()
	{
		gfx->SetDefState();
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		gfx->clearDepth(); // Clear screen

		cam.applyToOpenGl();

		pSun.Set(cam);
		pSun.Enable();

		cam.applyToOpenGl();

		glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);

		gfx->clearScreen();

		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glEnable(GL_LIGHTING);

		if (lp_CONFIG->wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		map->draw(&cam, toPlayerMask(players.local_human_id));

		if (lp_CONFIG->wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		cam.applyToOpenGl();

		features.draw(render_time); // Dessine les éléments "2D"

		/*----------------------------------------------------------------------------------------------*/

		// Dessine les unités sous l'eau / Draw units
		cam.applyToOpenGl();
		units.draw(lp_CONFIG->height_line);

		// Dessine les objets produits par les armes sous l'eau / Draw weapons which are under water
		weapons.draw(true);

		particle_engine.drawUW();

		cam.applyToOpenGl();

		// Dessine les objets produits par les armes n'ayant pas été dessinés / Draw weapons which have not been drawn
		weapons.draw(false);
	}

	void Battle::renderInfo()
	{
		if (build >= 0 && !IsOnGUI) // Display the building we want to build (with nice selection quads)
		{
			Vector3D target(cursorOnMap(cam, *map));

			Vector3D buildingPosition = map->snapToBuildCenter(target, build);

			can_be_there = can_be_built(buildingPosition, build, players.local_human_id);

			cam.applyToOpenGl();

			glTranslatef(buildingPosition.x, buildingPosition.y, buildingPosition.z);
			glScalef(unit_manager.unit_type[build]->Scale, unit_manager.unit_type[build]->Scale, unit_manager.unit_type[build]->Scale);
			const float DX = float(unit_manager.unit_type[build]->FootprintX << 2);
			const float DZ = float(unit_manager.unit_type[build]->FootprintZ << 2);
			if (unit_manager.unit_type[build]->model)
			{
				glEnable(GL_CULL_FACE);
				gfx->ReInitAllTex(true);
				if (can_be_there)
					glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
				else
					glColor4ub(0xFF, 0, 0, 0xFF);
				glDepthFunc(GL_GREATER);
				unit_manager.unit_type[build]->model->draw(0.0f, NULL, false, false, false, 0, NULL, NULL, NULL, 0.0f, NULL, false, players.local_human_id, false);
				glDepthFunc(GL_LESS);
				unit_manager.unit_type[build]->model->draw(0.0f, NULL, false, false, false, 0, NULL, NULL, NULL, 0.0f, NULL, false, players.local_human_id, false);

				double eqn[4] = {0.0f, -1.0f, 0.0f, map->sealvl - buildingPosition.y};
				glClipPlane(GL_CLIP_PLANE2, eqn);

				glEnable(GL_CLIP_PLANE2);

				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				glDepthFunc(GL_EQUAL);
				glColor4ub(0x7F, 0x7F, 0x7F, 0x7F);
				unit_manager.unit_type[build]->model->draw(0.0f, NULL, false, true, false, 0, NULL, NULL, NULL, 0.0f, NULL, false, players.local_human_id, false);
				glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
				glDepthFunc(GL_LESS);
				glDisable(GL_BLEND);

				glDisable(GL_CLIP_PLANE2);
			}
			cam.applyToOpenGl();
			glTranslatef(buildingPosition.x, Math::Max(buildingPosition.y, map->sealvl), buildingPosition.z);
			byte red = 0xFF, green = 0x00;
			if (can_be_there)
			{
				green = 0xFF;
				red = 0x00;
			}
			glDisable(GL_CULL_FACE);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_LIGHTING);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBegin(GL_QUADS);
			glColor4ub(red, green, 0x00, 0xFF);
			glVertex3f(-DX, 0.0f, -DZ); // First quad
			glVertex3f(DX, 0.0f, -DZ);
			glColor4ub(red, green, 0x00, 0x00);
			glVertex3f(DX + 2.0f, 5.0f, -DZ - 2.0f);
			glVertex3f(-DX - 2.0f, 5.0f, -DZ - 2.0f);

			glColor4ub(red, green, 0x00, 0xFF);
			glVertex3f(-DX, 0.0f, -DZ); // Second quad
			glVertex3f(-DX, 0.0f, DZ);
			glColor4ub(red, green, 0x00, 0x00);
			glVertex3f(-DX - 2.0f, 5.0f, DZ + 2.0f);
			glVertex3f(-DX - 2.0f, 5.0f, -DZ - 2.0f);

			glColor4ub(red, green, 0x00, 0xFF);
			glVertex3f(DX, 0.0f, -DZ); // Third quad
			glVertex3f(DX, 0.0f, DZ);
			glColor4ub(red, green, 0x00, 0x00);
			glVertex3f(DX + 2.0f, 5.0f, DZ + 2.0f);
			glVertex3f(DX + 2.0f, 5.0f, -DZ - 2.0f);

			glColor4ub(red, green, 0x00, 0xFF);
			glVertex3f(-DX, 0.0f, DZ); // Fourth quad
			glVertex3f(DX, 0.0f, DZ);
			glColor4ub(red, green, 0x00, 0x00);
			glVertex3f(DX + 2.0f, 5.0f, DZ + 2.0f);
			glVertex3f(-DX - 2.0f, 5.0f, DZ + 2.0f);
			glEnd();
			glDisable(GL_BLEND);
			glEnable(GL_LIGHTING);
			glEnable(GL_CULL_FACE);
		}

		if ((selected || units.last_on >= 0) && isShiftKeyDown())
		{
			cam.applyToOpenGl();
			bool builders = false;
			const float t = (float)MILLISECONDS_SINCE_INIT * 0.001f;
			const float mt = std::fmod(0.5f * t, 1.0f);
			for (unsigned int e = 0; e < units.index_list_size; ++e)
			{
				const int i = units.idx_list[e];
				if (units.unit[i].isAlive() && units.unit[i].isOwnedBy(players.local_human_id) && (units.unit[i].isSelected || i == units.last_on))
				{
					const int type_id = units.unit[i].typeId;
					if (type_id >= 0)
					{
						const UnitType* const pType = unit_manager.unit_type[type_id];
						builders |= pType->Builder;

						const float x = units.unit[i].render.Pos.x;
						const float z = units.unit[i].render.Pos.z;
						if (pType->kamikaze)
						{
							the_map->drawCircleOnMap(x, z, pType->kamikazedistance, makeacol(0xFF, 0x0, 0x0, 0xFF), 1.0f);
							const int idx = weapon_manager.get_weapon_index(pType->SelfDestructAs);
							const WeaponDef* const pWeapon = idx >= 0 && idx < weapon_manager.nb_weapons ? &(weapon_manager.weapon[idx]) : NULL;
							if (pWeapon)
								the_map->drawCircleOnMap(x, z, (float)pWeapon->areaofeffect * 0.25f * mt, makeacol(0xFF, 0x0, 0x0, 0xFF), 1.0f);
						}
						if (pType->mincloakdistance && units.unit[i].cloaked)
							the_map->drawCircleOnMap(x, z, (float)pType->mincloakdistance, makeacol(0xFF, 0xFF, 0xFF, 0xFF), 1.0f);
					}
					if (units.unit[i].isSelected)
						units.unit[i].show_orders(); // Dessine les ordres reçus par l'unité / Draw given orders
				}
			}

			if (builders)
			{
				for (unsigned int e = 0; e < units.index_list_size; ++e)
				{
					const int i = units.idx_list[e];
					const int type_id = units.unit[i].typeId;
					if (type_id < 0)
						continue;
					if (units.unit[i].isAlive() && units.unit[i].isOwnedBy(players.local_human_id) && !units.unit[i].isSelected && unit_manager.unit_type[type_id]->Builder && unit_manager.unit_type[type_id]->BMcode)
					{
						units.unit[i].show_orders(true); // Dessine les ordres reçus par l'unité / Draw given orders
					}
				}
			}
		}
		if ((selected || units.last_on >= 0) && isControlKeyDown())
		{
			cam.applyToOpenGl();
			const float t = (float)MILLISECONDS_SINCE_INIT * 0.001f;
			const float mt = std::fmod(0.5f * t, 1.0f);
			const float mt2 = std::fmod(0.5f * t + 0.5f, 1.0f);
			for (unsigned int e = 0; e < units.index_list_size; ++e)
			{
				const int i = units.idx_list[e];
				if (units.unit[i].isAlive() && units.unit[i].isOwnedBy(players.local_human_id) && (units.unit[i].isSelected || i == units.last_on))
				{
					const int type_id = units.unit[i].typeId;
					if (type_id >= 0)
					{
						const UnitType* const pType = unit_manager.unit_type[type_id];

						const float x = units.unit[i].render.Pos.x;
						const float z = units.unit[i].render.Pos.z;
						if (!isShiftKeyDown())
						{
							if (pType->kamikaze)
							{
								the_map->drawCircleOnMap(x, z, pType->kamikazedistance, makeacol(0xFF, 0x0, 0x0, 0xFF), 1.0f);
								const int idx = weapon_manager.get_weapon_index(pType->SelfDestructAs);
								const WeaponDef* const pWeapon = idx >= 0 && idx < weapon_manager.nb_weapons ? &(weapon_manager.weapon[idx]) : NULL;
								if (pWeapon)
									the_map->drawCircleOnMap(x, z, (float)pWeapon->areaofeffect * 0.25f * mt, makeacol(0xFF, 0x0, 0x0, 0xFF), 1.0f);
							}
							if (pType->mincloakdistance && units.unit[i].cloaked)
								the_map->drawCircleOnMap(x, z, (float)pType->mincloakdistance, makeacol(0xFF, 0xFF, 0xFF, 0xFF), 1.0f);
						}
						if (!pType->onoffable || units.unit[i].port[ACTIVATION])
						{
							if (pType->RadarDistance)
								the_map->drawCircleOnMap(x, z, (float)pType->RadarDistance * mt, makeacol(0x00, 0x00, 0xFF, 0xFF), 1.0f);
							if (pType->RadarDistanceJam)
								the_map->drawCircleOnMap(x, z, (float)pType->RadarDistanceJam * mt, makeacol(0x00, 0x00, 0x00, 0xFF), 1.0f);
							if (pType->SonarDistance)
								the_map->drawCircleOnMap(x, z, (float)pType->SonarDistance * mt2, makeacol(0xFF, 0xFF, 0xFF, 0xFF), 1.0f);
							if (pType->SonarDistanceJam)
								the_map->drawCircleOnMap(x, z, (float)pType->SonarDistanceJam * mt2, makeacol(0x7F, 0x7F, 0x7F, 0xFF), 1.0f);
						}
					}
				}
			}
		}
		if (showHealthBars)
		{
			cam.applyToOpenGl();
			units.drawHealthBars();
		}
	}

	void Battle::renderPostEffects()
	{
		particle_engine.draw(&cam); // Dessine les particules

		if (!map->water)
			fx_manager.draw(cam, map->sealvl, true); // Effets spéciaux en surface
		fx_manager.draw(cam, map->sealvl);			 // Effets spéciaux en surface
	}

	void Battle::renderScene()
	{
		renderWorld();

		renderStencilShadow();

		renderInfo();

		renderPostEffects();
	}
} // namespace TA3D
