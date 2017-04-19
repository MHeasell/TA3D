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
		cam_h = cam.rpos.y - map->get_unit_h(cam.rpos.x, cam.rpos.z);

		updateZFAR();

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
		if (rotate_light)
		{
			pSun.Dir.x = -1.0f;
			pSun.Dir.y = 1.0f;
			pSun.Dir.z = 1.0f;
			pSun.Dir.unit();
			Vector3D Dir(-pSun.Dir);
			Dir.x = cosf(light_angle);
			Dir.z = sinf(light_angle);
			Dir.unit();
			pSun.Dir = -Dir;
			units.draw_shadow(render_time, Dir);
		}
		else
		{
			pSun.Dir.x = -1.0f;
			pSun.Dir.y = 1.0f;
			pSun.Dir.z = 1.0f;
			pSun.Dir.unit();
			units.draw_shadow(render_time, -pSun.Dir);
		}
	}


	void Battle::renderWorld()
	{
		gfx->SetDefState();
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		gfx->clearDepth(); // Clear screen

		cam.setView();

		pSun.Set(cam);
		pSun.Enable();

		cam.setView();

		cam.zfar *= 100.0f;
		cam.setView();
		glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);

		gfx->clearScreen();

		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glEnable(GL_LIGHTING);
		updateZFAR();

		if (lp_CONFIG->wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		map->draw(&cam, byte(1 << players.local_human_id));

		if (lp_CONFIG->wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		cam.setView(true);

		features.draw(render_time); // Dessine les éléments "2D"

		/*----------------------------------------------------------------------------------------------*/

		// Dessine les unités sous l'eau / Draw units which are under water
		cam.setView(true);
		units.draw(true, false, true, lp_CONFIG->height_line);

		// Dessine les objets produits par les armes sous l'eau / Draw weapons which are under water
		weapons.draw(true);

		particle_engine.drawUW();

		cam.setView(true);
		// Dessine les unités non encore dessinées / Draw units which have not been drawn
		units.draw(false, false, true, lp_CONFIG->height_line);

		// Dessine les objets produits par les armes n'ayant pas été dessinés / Draw weapons which have not been drawn
		weapons.draw(false);
	}

	void Battle::renderInfo()
	{
		if (build >= 0 && !IsOnGUI) // Display the building we want to build (with nice selection quads)
		{
			Vector3D target(cursorOnMap(cam, *map));
			pMouseRectSelection.x2 = ((int)(target.x) + map->map_w_d) >> 3;
			pMouseRectSelection.y2 = ((int)(target.z) + map->map_h_d) >> 3;

			if (mouse_b != 1 && omb3 != 1)
			{
				pMouseRectSelection.x1 = pMouseRectSelection.x2;
				pMouseRectSelection.y1 = pMouseRectSelection.y2;
			}

			int d = Math::Max(abs(pMouseRectSelection.x2 - pMouseRectSelection.x1), abs(pMouseRectSelection.y2 - pMouseRectSelection.y1));

			int ox = pMouseRectSelection.x1 + 0xFFFF;
			int oy = pMouseRectSelection.y1 + 0xFFFF;

			for (int c = 0; c <= d; ++c)
			{
				target.x = float(pMouseRectSelection.x1 + (pMouseRectSelection.x2 - pMouseRectSelection.x1) * c / Math::Max(d, 1));
				target.z = float(pMouseRectSelection.y1 + (pMouseRectSelection.y2 - pMouseRectSelection.y1) * c / Math::Max(d, 1));

				if (abs(ox - (int)target.x) < unit_manager.unit_type[build]->FootprintX && abs(oy - (int)target.z) < unit_manager.unit_type[build]->FootprintZ)
					continue;
				ox = (int)target.x;
				oy = (int)target.z;

				target.y = map->get_max_rect_h((int)target.x, (int)target.z, unit_manager.unit_type[build]->FootprintX, unit_manager.unit_type[build]->FootprintZ);
				if (unit_manager.unit_type[build]->floatting())
					target.y = Math::Max(target.y, map->sealvl + ((float)unit_manager.unit_type[build]->AltFromSeaLevel - (float)unit_manager.unit_type[build]->WaterLine) * H_DIV);
				target.x = target.x * 8.0f - (float)map->map_w_d;
				target.z = target.z * 8.0f - (float)map->map_h_d;

				can_be_there = can_be_built(target, build, players.local_human_id);

				cam.setView();

				glTranslatef(target.x, target.y, target.z);
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

					double eqn[4] = {0.0f, -1.0f, 0.0f, map->sealvl - target.y};
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
				cam.setView();
				glTranslatef(target.x, Math::Max(target.y, map->sealvl), target.z);
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
		}

		if ((selected || units.last_on >= 0) && TA3D_SHIFT_PRESSED)
		{
			cam.setView();
			bool builders = false;
			const float t = (float)msec_timer * 0.001f;
			const float mt = std::fmod(0.5f * t, 1.0f);
			for (unsigned int e = 0; e < units.index_list_size; ++e)
			{
				const int i = units.idx_list[e];
				if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && (units.unit[i].sel || i == units.last_on))
				{
					const int type_id = units.unit[i].type_id;
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
					if (units.unit[i].sel)
						units.unit[i].show_orders(); // Dessine les ordres reçus par l'unité / Draw given orders
				}
			}

			if (builders)
			{
				for (unsigned int e = 0; e < units.index_list_size; ++e)
				{
					const int i = units.idx_list[e];
					const int type_id = units.unit[i].type_id;
					if (type_id < 0)
						continue;
					if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && !units.unit[i].sel && unit_manager.unit_type[type_id]->Builder && unit_manager.unit_type[type_id]->BMcode)
					{
						units.unit[i].show_orders(true); // Dessine les ordres reçus par l'unité / Draw given orders
					}
				}
			}
		}
		if ((selected || units.last_on >= 0) && TA3D_CTRL_PRESSED)
		{
			cam.setView();
			const float t = (float)msec_timer * 0.001f;
			const float mt = std::fmod(0.5f * t, 1.0f);
			const float mt2 = std::fmod(0.5f * t + 0.5f, 1.0f);
			for (unsigned int e = 0; e < units.index_list_size; ++e)
			{
				const int i = units.idx_list[e];
				if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && (units.unit[i].sel || i == units.last_on))
				{
					const int type_id = units.unit[i].type_id;
					if (type_id >= 0)
					{
						const UnitType* const pType = unit_manager.unit_type[type_id];

						const float x = units.unit[i].render.Pos.x;
						const float z = units.unit[i].render.Pos.z;
						if (!TA3D_SHIFT_PRESSED)
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
			cam.setView();
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
		cam.znear = -512.0f;

		renderWorld();

		renderStencilShadow();

		renderInfo();

		renderPostEffects();
	}

	void Battle::makePoster(int w, int h)
	{
		bool previous_pause_state = lp_CONFIG->pause;
		lp_CONFIG->pause = true;

		while (!lp_CONFIG->paused)
			rest(100); // Wait for the engine to enter in pause mode so we can assemble several shots
					   // of the same game tick

		Camera camBak = cam;

		cam.znear = -255.0f;
		SDL_Surface* poster = gfx->create_surface_ex(24, w, h);
		SDL_Surface* buf = gfx->create_surface_ex(24, SCREEN_W, SCREEN_H);

		for (int z = 0; z < h; z += SCREEN_H / 2)
		{
			for (int x = 0; x < w; x += SCREEN_W / 2)
			{
				// Set camera to current part of the scene
				cam.rpos = camBak.rpos + camBak.zoomFactor * (float(x - w / 2 - SCREEN_W / 4) * camBak.side + float(z - h / 2 - SCREEN_H / 4) * camBak.up);
				if (!Math::AlmostZero(camBak.dir.y))
					cam.rpos = cam.rpos + ((camBak.rpos - cam.rpos).y / camBak.dir.y) * camBak.dir;

				// Render this part of the scene
				gfx->clearAll();
				initRenderer();
				renderScene();

				// Read the pixels
				glReadPixels(0, 0, SCREEN_W, SCREEN_H, GL_BGR, GL_UNSIGNED_BYTE, buf->pixels);

				// Fill current part of the poster
				blit(buf, poster, SCREEN_W / 4, SCREEN_H / 4, x, z, Math::Min(SCREEN_W / 2, poster->w - x), Math::Min(SCREEN_H / 2, poster->h - z));
			}
		}

		vflip_bitmap(poster);
		save_TGA(String(TA3D::Paths::Screenshots) << "poster.tga", poster);

		SDL_FreeSurface(buf);
		SDL_FreeSurface(poster);

		cam = camBak;

		lp_CONFIG->pause = previous_pause_state;
	}

} // namespace TA3D
