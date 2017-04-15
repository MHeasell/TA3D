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
		updateFOG();

		render_time = ((float)units.current_tick) / TICKS_PER_SEC;

		// Copy unit data from simulation
		units.renderTick();

		glActiveTextureARB(GL_TEXTURE7_ARB);
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		gfx->setShadowMapMode(false);
		if (g_useProgram)
			glUseProgramObjectARB(0);
	}

	void Battle::renderShadowMap()
	{
		if (lp_CONFIG->shadow_quality > 0 && cam.rpos.y <= gfx->low_def_limit)
		{
			switch (lp_CONFIG->shadow_quality)
			{
				case 3:
				case 2: // Render the shadow map
					gfx->setShadowMapMode(true);
					gfx->SetDefState();
					gfx->renderToTextureDepth(gfx->get_shadow_map());
					gfx->clearDepth();
					pSun.SetView(map->get_visible_volume());

					// We'll need this matrix later (when rendering with shadows)
					gfx->readShadowMapProjectionMatrix();

					glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
					glDisable(GL_FOG);
					glShadeModel(GL_FLAT);

					glEnable(GL_POLYGON_OFFSET_FILL);
					glPolygonOffset(3.0f, 1.0f);

					// Render all visible features from light's point of view
					for (std::vector<int>::const_iterator i = features.list.begin(); i != features.list.end(); ++i)
						features.feature[*i].draw = true;
					features.draw(render_time, true);

					glEnable(GL_POLYGON_OFFSET_FILL);
					glPolygonOffset(3.0f, 1.0f);
					// Render all visible units from light's point of view
					units.draw(true, false, true, false);
					units.draw(false, false, true, false);

					// Render all visible weapons from light's point of view
					weapons.draw(true);
					weapons.draw(false);

					glDisable(GL_POLYGON_OFFSET_FILL);
					glPolygonOffset(0.0f, 0.0f);

					gfx->renderToTextureDepth(0);
					glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

					glActiveTextureARB(GL_TEXTURE7_ARB);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, gfx->get_shadow_map());
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
					glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);

					glActiveTextureARB(GL_TEXTURE0_ARB);
					gfx->setShadowMapMode(false);
					break;
			};
		}
	}

	void Battle::renderStencilShadow()
	{
		if (lp_CONFIG->shadow_quality > 0 && cam.rpos.y <= gfx->low_def_limit)
		{
			switch (lp_CONFIG->shadow_quality)
			{
				case 1: // Stencil Shadowing (shadow volumes)
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
					break;
				case 2: // Shadow mapping
				case 3:
					break;
			}
		}
	}


	void Battle::renderWorld()
	{
		gfx->SetDefState();
		glClearColor(FogColor[0], FogColor[1], FogColor[2], FogColor[3]);
		gfx->clearDepth(); // Clear screen

		cam.setView();

		pSun.Set(cam);
		pSun.Enable();

		cam.setView();

		cam.zfar *= 100.0f;
		cam.setView();
		glDisable(GL_FOG);
		glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		if (lp_CONFIG->render_sky)
		{
			glDisable(GL_LIGHTING);
			glDepthMask(GL_FALSE);
			glTranslatef(cam.rpos.x, cam.rpos.y + cam.shakeVector.y, cam.rpos.z);
			glRotatef(sky_angle, 0.0f, 1.0f, 0.0f);
			if (lp_CONFIG->ortho_camera)
			{
				const float scale = cam.zoomFactor / 800.0f * std::sqrt(float(SCREEN_H * SCREEN_H + SCREEN_W * SCREEN_W));
				glScalef(scale, scale, scale);
			}
			sky.draw();
		}
		else
			gfx->clearScreen();

		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glEnable(GL_LIGHTING);
		glEnable(GL_FOG);
		updateZFAR();

		if (lp_CONFIG->wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		map->draw(&cam, byte(1 << players.local_human_id), false, 0.0f, t, dt * units.apparent_timefactor);

		if (lp_CONFIG->wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		cam.setView(lp_CONFIG->shadow_quality < 2);

		features.draw(render_time); // Dessine les éléments "2D"

		/*----------------------------------------------------------------------------------------------*/

		// Dessine les unités sous l'eau / Draw units which are under water
		cam.setView(lp_CONFIG->shadow_quality < 2);
		if (cam.rpos.y <= gfx->low_def_limit)
		{
			if (lp_CONFIG->shadow_quality >= 2)
				glFogi(GL_FOG_COORD_SRC, GL_FOG_COORD);
			units.draw(true, false, true, lp_CONFIG->height_line);
			glFogi(GL_FOG_COORD_SRC, GL_FRAGMENT_DEPTH);
		}

		// Dessine les objets produits par les armes sous l'eau / Draw weapons which are under water
		weapons.draw(true);

		if (lp_CONFIG->particle)
			particle_engine.drawUW();

		// Render map object icons (if in tactical mode)
		if (cam.rpos.y > gfx->low_def_limit)
		{
			cam.setView(true);
			features.draw_icons();
		}

		cam.setView(lp_CONFIG->shadow_quality < 2);
		if (lp_CONFIG->shadow_quality >= 2)
			glFogi(GL_FOG_COORD_SRC, GL_FOG_COORD);
		// Dessine les unités non encore dessinées / Draw units which have not been drawn
		units.draw(false, false, true, lp_CONFIG->height_line);
		glFogi(GL_FOG_COORD_SRC, GL_FRAGMENT_DEPTH);

		// Dessine les objets produits par les armes n'ayant pas été dessinés / Draw weapons which have not been drawn
		weapons.draw(false);
	}

	void Battle::renderInfo()
	{
		if (build >= 0 && !IsOnGUI) // Display the building we want to build (with nice selection quads)
		{
			glDisable(GL_FOG);
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

					const bool old_mode = gfx->getShadowMapMode();
					gfx->setShadowMapMode(true);
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
					gfx->setShadowMapMode(old_mode);
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
			glEnable(GL_FOG);
		}

		if ((selected || units.last_on >= 0) && TA3D_SHIFT_PRESSED)
		{
			glDisable(GL_FOG);
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
			glEnable(GL_FOG);
		}
		if ((selected || units.last_on >= 0) && TA3D_CTRL_PRESSED)
		{
			glDisable(GL_FOG);
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
			glEnable(GL_FOG);
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
		if (lp_CONFIG->ortho_camera)
			cam.znear = -512.0f;
		else
			cam.znear = 1.0f;

		renderShadowMap();

		renderWorld();

		renderStencilShadow();

		renderInfo();

		renderPostEffects();
	}

	void Battle::makePoster(int w, int h)
	{
		bool previous_pause_state = lp_CONFIG->pause;
		bool prevCameraType = lp_CONFIG->ortho_camera;
		lp_CONFIG->pause = true;
		lp_CONFIG->ortho_camera = true;

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
		lp_CONFIG->ortho_camera = prevCameraType;
	}

} // namespace TA3D
