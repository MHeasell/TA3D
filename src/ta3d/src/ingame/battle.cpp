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

#include "../TA3D_NameSpace.h"
#include "../misc/matrix.h"

#include "../intro.h"			// Introduction
#include "../menu.h"			// Game menus
#include "../restore.h"		// Save/Load mecanisms
#include "../network/TA3D_Network.h"	// Network functionnalities such as chat
#include "../gfx/fx.h"
#include "../misc/paths.h"
#include "../misc/files.h"
#include "../misc/camera.h"
#include "../languages/i18n.h"
#include <list>
#include <vector>
#include "menus/statistics.h"
#include "../misc/math.h"
#include "../sounds/manager.h"
#include "../logs/logs.h"
#include "../console.h"
#include "weapons/weapons.h"
#include "../fbi.h"
#include "../UnitEngine.h"
#include "../tnt.h"
#include "../scripts/script.h"
#include "../scripts/lua.env.h"
#include "../gfx/shader.h"
#include "players.h"
#include "../engine/mission.h"
#include "../input/keyboard.h"
#include "../input/mouse.h"





#define PICK_TOLERANCE  5

namespace TA3D
{
	int expected_players;

	int fire;
	int build_part;

	int start=0;



	static inline int CursorFromSignalOrder(const int order, int defaultValue)
	{
		switch (order)
		{
			case SIGNAL_ORDER_CAPTURE:	return CURSOR_CAPTURE;
			case SIGNAL_ORDER_MOVE:		return CURSOR_MOVE;
			case SIGNAL_ORDER_PATROL:	return CURSOR_PATROL;
			case SIGNAL_ORDER_GUARD:	return CURSOR_GUARD;
			case SIGNAL_ORDER_DGUN:		return CURSOR_ATTACK;
			case SIGNAL_ORDER_ATTACK:	return CURSOR_ATTACK;
			case SIGNAL_ORDER_RECLAM:	return CURSOR_RECLAIM;
			case SIGNAL_ORDER_LOAD:		return CURSOR_LOAD;
			case SIGNAL_ORDER_UNLOAD:	return CURSOR_UNLOAD;
			case SIGNAL_ORDER_REPAIR:	return CURSOR_REPAIR;
		}
		return defaultValue;
	}


	bool Battle::preExecute(LuaProgram& gameScript)
	{
		if (!pGameData) // no gamedata, nothing to do
			return false;

		if (pNetworkEnabled) // prepare the network connections if any
			network_manager.cleanQueues();

		if (!loadFromGameData(pGameData)) // Reinit data
			return false;

		// Network synchronization
		waitForNetworkPlayers();

		// Script Management
		if (!pNetworkEnabled || pNetworkIsServer)
		{
			gameScript.load(pGameData->game_script);	// Load the script
			if (!pGameData->saved_file.empty()) 		// We have something to load, so let's run initialization code in passive mode
			{
				LuaProgram::passive = true;            // So deactivate unit creation (at least neutralize network creation events)
				gameScript.run(0.0f);
			}
			LuaProgram::passive = false;
			gameScript.start();                        // Start game script thread
		}

		if (!pGameData->saved_file.empty()) 			// We have something to load
		{
			load_game(pGameData);
			done = !pGameData->saved_file.empty();		// If loading the game fails, then exit
		}

		// Code Related to Threads
		unit_engine_thread_sync = 0;
		weapon_engine_thread_sync = 0;
		particle_engine_thread_sync = 0;

		// Start the Unit engine
		units.start();

		// Start the particle engine
		particle_engine.set_data( map->ota_data.gravity, map->wind_vec);
		particle_engine.start();

		// Start the weapon engine
		weapons.set_data(map.get());
		// NB: the feature engine runs in the weapon thread to avoid having too much thread to synchronise
		features.set_data(map->wind_vec);
		weapons.start();

		// Players
		players.start();

		return true;
	}





	Battle::Result Battle::execute()
	{
		LuaProgram game_script;

		if (!preExecute(game_script))
			return pResult;

		// Here we go Commander !
		LOG_INFO(LOG_PREFIX_BATTLE << "*** The game has started - Good luck Commander ! ***");
		// Reinit the counter for FPS
		fps.lastTime = msec_timer;

		do
		{
			// Prepare events and reInit some vars
			preflightVars();

			// Wind - Make a change every 10 sec. (simulation time)
			if ((wind_change = (t - wind_t >= 10.0f)))
				preflightChangeWindSpeedAndDirection();

			// Update 3D sounds
			preflightUpdate3DSounds();


			if (!freecam)
				preflightAutomaticCamera();
			else
				preflightFreeCamera();


			bool rope_selection = pMouseSelecting && (abs(pMouseRectSelection.x1 - pMouseRectSelection.x2) >= PICK_TOLERANCE || abs(pMouseRectSelection.y1 - pMouseRectSelection.y2) >= PICK_TOLERANCE);
			if (selected && build < 0 && (!IsOnGUI || IsOnMinimap) && !rope_selection)
			{
				for (int i = 0; i < units.index_list_size; ++i)
				{
					uint32 e = units.idx_list[i];
					if ((units.unit[e].flags & 1) && units.unit[e].owner_id == players.local_human_id
						&& units.unit[e].sel && unit_manager.unit_type[units.unit[e].type_id]->canmove)
					{
						cursor_type = CURSOR_MOVE;
						break;
					}
				}
			}
			else
				cursor_type = CURSOR_DEFAULT;


			dt = (msec_timer - count) * Conv; // Regulate frame rate
			while (dt < delay)
			{
				switch (lp_CONFIG->priority_level)
				{
					case 0: rest(1); break;
					case 1: rest(0); break;
				}
				dt = (msec_timer - count) * Conv;
			}
			count = msec_timer;

			if (pSkyIsSpherical)
				sky_angle += pSkyData->rotation_speed * dt * units.apparent_timefactor;

			unit_info -= dt;
			if (!lp_CONFIG->pause)
			{
				light_angle+=dt*units.apparent_timefactor;
				t += dt * units.apparent_timefactor;
			}


			/*------------bloc regroupant ce qui est relatif aux commandes----------------*/

			if (players.local_human_id >= 0 && !console.activated() && !pArea.get_state("chat"))
				handleGameStatusEvents();

			if (TA3D_CTRL_PRESSED && key[KEY_D])
			{
				if (!ordered_destruct)
				{
					for (unsigned int e = 0; e < units.index_list_size; ++e)
					{
						int i = units.idx_list[e];
						if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && units.unit[i].sel)
							units.unit[i].toggle_self_destruct();
					}
					ordered_destruct = true;
				}
			}
			else
				ordered_destruct = false;

			if (key[KEY_PLUS_PAD] && !console.activated())
			{
				if (!speed_changed && lp_CONFIG->timefactor < 10.0f)
				{
					lp_CONFIG->timefactor++;
					show_timefactor = 1.0f;
				}
				speed_changed = true;
			}
			else
			{
				if ((key[KEY_MINUS] || key[KEY_MINUS_PAD]) && !console.activated())
				{
					if (!speed_changed && lp_CONFIG->timefactor > 1.0f)
					{
						lp_CONFIG->timefactor--;
						show_timefactor = 1.0f;
					}
					speed_changed = true;
				}
				else
					speed_changed = false;
			}

			if (track_mode >= 0 && track_mode <= units.max_unit)
			{
				if (!(units.unit[ track_mode ].flags & 1)) // Leave tracking mode
				{
					track_mode = -1;
					cam_has_target = false;
				}
				else
				{
					cam_has_target = true;
					cam_target = units.unit[ track_mode ].Pos;
					cam_target_mx = gfx->SCREEN_W_HALF;
					cam_target_my = gfx->SCREEN_H_HALF;
				}
			}
			else
				track_mode = -1;

			if (track_mode >= 0 && ( cur_sel_index < 0 || cur_sel_index >= units.max_unit || track_mode != cur_sel_index))
				track_mode = -1;

			if (key[KEY_T] && !console.activated() && cur_sel_index>=0 && cur_sel_index<units.max_unit)
			{
				if (!last_time_activated_track_mode)
					track_mode = track_mode == cur_sel_index ? -1 : cur_sel_index;
				last_time_activated_track_mode = true;
			}
			else
			{
				if (key[KEY_T] && !console.activated())
				{
					track_mode = -1;
					cam_has_target = false;
				}
				else
					last_time_activated_track_mode = false;
			}

			if (key[KEY_F1] && units.last_on >= 0 && units.unit[ units.last_on ].type_id >= 0)
			{
				unit_info_id = units.unit[ units.last_on ].type_id;
				unit_info = 1.0f;
			}
			else
			{
				if (key[KEY_F1] && cur_sel >= 0)
				{
					unit_info_id = cur_sel;
					unit_info = 1.0f;
				}
			}

			if (mouse_x < 128.0f && mouse_y < 128.0f && mouse_x >= 0.0f && mouse_y >= 0.0f
				&& ((mouse_b == 2 && !lp_CONFIG->right_click_interface) || (mouse_b == 1 && lp_CONFIG->right_click_interface)))
			{
				cam.rpos.x = (mouse_x - 64) * map->map_w / 128.0f * 252.0f / map->mini_w;
				cam.rpos.z = (mouse_y - 64) * map->map_h / 128.0f * 252.0f / map->mini_h;
				cam_has_target = false;
			}
			if (mouse_x < 1)
			{
				Vector3D move_dir(cam.side);
				move_dir.y = 0.0f;
				move_dir.unit();
				cam.rpos = cam.rpos - (SCROLL_SPEED * dt * cam_h / 151.0f) * move_dir;
				cam_has_target = false;
			}
			else
			{
				if (mouse_x >= SCREEN_W - 1)
				{
					Vector3D move_dir(cam.side);
					move_dir.y = 0.0f;
					move_dir.unit();
					cam.rpos = cam.rpos + (SCROLL_SPEED * dt * cam_h / 151.0f) * move_dir;
					cam_has_target = false;
				}
			}
			if (mouse_y < 1)
			{
				Vector3D move_dir(cam.up);
				if (Yuni::Math::Zero(move_dir.x) && Yuni::Math::Zero(move_dir.z))
					move_dir = cam.dir;
				move_dir.y = 0.f;
				move_dir.unit();
				cam.rpos = cam.rpos+ (SCROLL_SPEED * dt * cam_h / 151.0f) * move_dir;
				cam_has_target = false;
			}
			else
			{
				if (mouse_y >= SCREEN_H - 1)
				{
					Vector3D move_dir(cam.up);
					if (Yuni::Math::Zero(move_dir.x) && Yuni::Math::Zero(move_dir.z))
						move_dir = cam.dir;

					move_dir.y = 0.f;
					move_dir.unit();
					cam.rpos = cam.rpos - (SCROLL_SPEED * dt * cam_h / 151.0f) * move_dir;
					cam_has_target = false;
				}
			}

			if (freecam)
			{
				if (mouse_b == 4)
				{
					get_mouse_mickeys(&mx,&my);
					if (omb == mouse_b)
					{
						r2 -= mx;
						r1 -= my;
					}
					position_mouse(gfx->SCREEN_W_HALF, gfx->SCREEN_H_HALF);
				}
				else
				{
					if (omb == 4)
						position_mouse(gfx->SCREEN_W_HALF, gfx->SCREEN_H_HALF);
				}
			}
			else
			{
				if (!TA3D_CTRL_PRESSED)
				{
					if (mouse_b == 4)
					{
						get_mouse_mickeys(&mx, &my);
						if (omb == mouse_b)
						{
							cam.rpos.x += mx * cam_h / 151.0f;
							cam.rpos.z += my * cam_h / 151.0f;
							cam_has_target = false;
						}
						position_mouse(gfx->SCREEN_W_HALF, gfx->SCREEN_H_HALF);
					}
					else
					{
						if (omb == 4)
						{
							position_mouse(gfx->SCREEN_W_HALF, gfx->SCREEN_H_HALF);
							cam_has_target = false;
						}
					}
				}
			}
			omb = mouse_b;

			if (!freecam)
			{
				if (!console.activated())
					keyArrowsNotInFreeCam();

				float h = map->get_unit_h(cam.rpos.x, cam.rpos.z);
				if (h < map->sealvl)
					h = map->sealvl;
				for (int i = 0; i < 20; ++i) // Increase precision
				{
					for (float T = 0.0f; T < dt ; T += 0.1f)
						cam.rpos.y += (h + cam_h - cam.rpos.y) * Math::Min(dt - T, 0.1f);
				}
			}
			else
			{
				if (!console.activated())
					keyArrowsInFreeCam();
			}

			if (cam.rpos.x < -map->map_w_d)
			{
				cam.rpos.x = -map->map_w_d;
				cam_has_target = false;
			}
			if (cam.rpos.x > map->map_w_d)
			{
				cam.rpos.x = map->map_w_d;
				cam_has_target = false;
			}
			if (cam.rpos.z < -map->map_h_d + 200.0f)
			{
				cam.rpos.z = -map->map_h_d + 200.0f;
				cam_has_target = false;
			}
			if (cam.rpos.z > map->map_h_d && !cam_has_target)
				cam.rpos.z = map->map_h_d;
			if (cam.rpos.z > map->map_h_d + 200.0f)
				cam.rpos.z = map->map_h_d + 200.0f;

			Matrix Rotation;
			if (lp_CONFIG->camera_zoom == ZOOM_NORMAL)
				Rotation = RotateX( r1 * DEG2RAD) * RotateY( r2 * DEG2RAD) * RotateZ( r3 * DEG2RAD);
			else
				Rotation = RotateX( -lp_CONFIG->camera_def_angle * DEG2RAD) * RotateY( r2 * DEG2RAD) * RotateZ( r3 * DEG2RAD);

			cam.setMatrix(Rotation);
			cam.updateShake(dt);

			if (cam_has_target)
			{
				Vector3D cur_dir;
				Vector3D m_pos;
				if (lp_CONFIG->ortho_camera)
				{
					cur_dir = cam.dir;
					m_pos = cam.pos + cam.zoomFactor * ( (cam_target_mx - gfx->SCREEN_W_HALF) * cam.side - (cam_target_my - gfx->SCREEN_H_HALF) * cam.up ) + cam.znear * cam.dir;
				}
				else
				{
					cur_dir = cam.dir + cam.widthFactor * 2.0f * (cam_target_mx - gfx->SCREEN_W_HALF) * gfx->SCREEN_W_INV * cam.side - 1.5f * (cam_target_my - gfx->SCREEN_H_HALF) * gfx->SCREEN_H_INV * cam.up;
					m_pos = cam.rpos;
				}
				cur_dir.unit();		// Direction pointée par le curseur
				Vector3D moving_target(cam_target - m_pos);
				moving_target = moving_target - (moving_target % cur_dir) * cur_dir;
				if (lp_CONFIG->ortho_camera)
				{
					float d = moving_target.sq();
					moving_target.y = 0.0f;
					float D = moving_target.sq();
					if (Yuni::Math::Zero(D))
						cam_has_target = false;
					else
						moving_target = d / D * moving_target;
				}
				cam.rpos = moving_target + cam.rpos;
			}

			if (!selected)
				current_order = SIGNAL_ORDER_NONE;

			if (current_order != SIGNAL_ORDER_NONE && abs( pMouseRectSelection.x1 - pMouseRectSelection.x2) < PICK_TOLERANCE && abs( pMouseRectSelection.y1 - pMouseRectSelection.y2) < PICK_TOLERANCE)
				pMouseSelecting=false;

			rope_selection = pMouseSelecting && ( abs( pMouseRectSelection.x1 - pMouseRectSelection.x2) >= PICK_TOLERANCE || abs( pMouseRectSelection.y1 - pMouseRectSelection.y2) >= PICK_TOLERANCE);

			bool order_removed = false;

			bool right_click_activation = lp_CONFIG->right_click_interface && mouse_b != 2 && omb3 == 2 && current_order == SIGNAL_ORDER_NONE;
			bool left_click_activation = mouse_b != 1 && omb3 == 1 && ((!lp_CONFIG->right_click_interface && current_order == SIGNAL_ORDER_NONE)
																	   || current_order != SIGNAL_ORDER_NONE);
			bool click_activation = right_click_activation || left_click_activation;
			bool click_activated = false;

			if (selected && (!IsOnGUI || IsOnMinimap))
			{
				bool builders = false;
				bool canattack = false;
				bool canreclamate = false;
				bool canresurrect = false;
				for (unsigned int e = 0; e < units.index_list_size; ++e)
				{
					int i = units.idx_list[e];
					if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && units.unit[i].sel)
					{
						builders|=unit_manager.unit_type[units.unit[i].type_id]->Builder;
						canattack|=unit_manager.unit_type[units.unit[i].type_id]->canattack;
						canreclamate|=unit_manager.unit_type[units.unit[i].type_id]->CanReclamate;
						canresurrect|=unit_manager.unit_type[units.unit[i].type_id]->canresurrect;
					}
				}
				int pointing = 0;
				if (!IsOnGUI)
				{
					pointing = units.pick(cam);		// Sur quoi le curseur est-il pointé??
					if (pointing == -1) 				// Is the cursor on a rock, tree, ...?
					{
						Vector3D cur_pos(cursorOnMap(cam, *map, IsOnMinimap));
						int px = ((int)(cur_pos.x + map->map_w_d)) >> 3;
						int py = ((int)(cur_pos.z + map->map_h_d)) >> 3;

						if (px >= 0 && px < map->bloc_w_db && py >= 0 && py < map->bloc_h_db
							&& (SurfaceByte(map->view_map, px>>1, py>>1) & (1<<players.local_human_id)) )
						{
							int idx = -map->map_data[py][px].unit_idx - 2;				// Basic check
							if (idx<0 || features.feature[idx].type<0)
							{
								units.last_on = -1;
								for (int dy = -7 ; dy < 8 ; ++dy)	// Look for things like metal patches
								{
									if (py + dy >= 0 && py + dy < map->bloc_h_db)
									{
										for (int dx = -7 ; dx < 8; ++dx)
										{
											if (px + dx >= 0 && px + dx < map->bloc_w_db)
											{
                                                int feature_idx = map->map_data[py+dy][px+dx].stuff;    // This is thread-safe
                                                if (feature_idx >= 0)
												{
                                                    idx = feature_idx;
													Feature *feature = feature_manager.getFeaturePointer(features.feature[idx].type);
													if (feature
														&& feature->footprintx + 1 >= (abs(dx) << 1)
														&& feature->footprintz + 1 >= (abs(dy) << 1))
													{
														units.last_on = -idx - 2;
														dy = 800;
														break;
													}
												}
											}
										}
									}
								}
							}
							else
								units.last_on = -idx - 2;
						}
						pointing = units.last_on;
					}
				}
				else
					pointing = units.pick_minimap();


				if (!rope_selection)
				{
					if (pointing < -1)
					{
						Feature *feature = feature_manager.getFeaturePointer(features.feature[ -pointing - 2 ].type);
						if (feature && canreclamate && feature->reclaimable && build == -1)	cursor_type = CURSOR_RECLAIM;
						if (feature && canresurrect && feature->reclaimable && build == -1 && CURSOR_REVIVE != CURSOR_RECLAIM)	cursor_type = CURSOR_REVIVE;
					}
				}

				if (pointing >= 0 && !rope_selection) 	// S'il y a quelque chose sous le curseur
				{
					cursor_type = CURSOR_CROSS;
					bool can_be_captured = false;
					if (!(players.team[units.unit[pointing].owner_id] & players.team[players.local_human_id]))      // Not allied == enemy
					{
						can_be_captured = true;
						if (canattack)
							cursor_type=CURSOR_ATTACK;
						else
						{
							if (canreclamate)
								cursor_type = CURSOR_RECLAIM;
							else
								cursor_type = CURSOR_CANT_ATTACK;
						}
					}
					else
					{
						if (units.unit[pointing].port[BUILD_PERCENT_LEFT]>0.0f && builders)
							cursor_type = CURSOR_REPAIR;
					}

					cursor_type = CursorFromSignalOrder(current_order, cursor_type);

					if (cursor_type!=CURSOR_DEFAULT && click_activation && !IsOnGUI && TA3D_SHIFT_PRESSED) // Remove commands from queue
					{
						Vector3D target(cursorOnMap(cam, *map));
						target.x = ((int)(target.x) + map->map_w_d) >> 3;
						target.z = ((int)(target.z) + map->map_h_d) >> 3;
						target.x = target.x * 8.0f - map->map_w_d;
						target.z = target.z * 8.0f - map->map_h_d;
						target.y = Math::Max(map->get_unit_h(target.x, target.z), map->sealvl);
						order_removed = units.remove_order(players.local_human_id, target);
					}

					if (click_activation && !order_removed)
					{
						order_removed = true;
						if (cursor_type==CURSOR_ATTACK)
						{
							for (unsigned int e = 0; e < units.index_list_size; ++e)
							{
								uint32 commandfire = current_order == SIGNAL_ORDER_DGUN ? MISSION_FLAG_COMMAND_FIRE : 0;
								int i = units.idx_list[e];
								units.unit[i].lock();
								if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && units.unit[i].sel && unit_manager.unit_type[units.unit[i].type_id]->canattack
									&& ( unit_manager.unit_type[units.unit[i].type_id]->BMcode || !unit_manager.unit_type[units.unit[i].type_id]->Builder))
								{
									for (unsigned int f = 0; f < unit_manager.unit_type[units.unit[i].type_id]->weapon.size(); ++f)
										if (unit_manager.unit_type[units.unit[i].type_id]->weapon[ f ] && unit_manager.unit_type[units.unit[i].type_id]->weapon[ f ]->stockpile)
										{
											commandfire = MISSION_FLAG_COMMAND_FIRE;
											break;
										}
									if (TA3D_SHIFT_PRESSED)
										units.unit[i].add_mission(MISSION_ATTACK,&(units.unit[pointing].Pos),false,0,&(units.unit[pointing]),NULL,commandfire);
									else
										units.unit[i].set_mission(MISSION_ATTACK,&(units.unit[pointing].Pos),false,0,true,&(units.unit[pointing]),NULL,commandfire);
								}
								units.unit[i].unlock();
							}
							if (!TA3D_SHIFT_PRESSED)	current_order=SIGNAL_ORDER_NONE;
							click_activated = true;
						}
						else
							if (cursor_type == CURSOR_CAPTURE && can_be_captured)
							{
								for (unsigned int e = 0 ; e < units.index_list_size ; e++)
								{
									units.lock();
									int i = units.idx_list[e];
									units.unlock();
									units.unit[i].lock();
									if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && units.unit[i].sel && unit_manager.unit_type[units.unit[i].type_id]->CanCapture)
									{
										if (TA3D_SHIFT_PRESSED)
											units.unit[i].add_mission( MISSION_CAPTURE, &(units.unit[pointing].Pos), false, 0, &(units.unit[pointing]), NULL);
										else
											units.unit[i].set_mission( MISSION_CAPTURE, &(units.unit[pointing].Pos), false, 0, true, &(units.unit[pointing]), NULL);
									}
									units.unit[i].unlock();
								}
								if (!TA3D_SHIFT_PRESSED)
									current_order = SIGNAL_ORDER_NONE;
								click_activated = true;
							}
							else
							{
								if (cursor_type==CURSOR_REPAIR)
								{
									for (unsigned int e = 0; e < units.index_list_size; ++e)
									{
										units.lock();
										int i = units.idx_list[e];
										units.unlock();
										units.unit[i].lock();
										if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && units.unit[i].sel
											&& unit_manager.unit_type[units.unit[i].type_id]->Builder && unit_manager.unit_type[units.unit[i].type_id]->BMcode)
										{
											if (!TA3D_SHIFT_PRESSED)
												units.unit[i].playSound("repair");
											if (TA3D_SHIFT_PRESSED)
												units.unit[i].add_mission(MISSION_REPAIR,&(units.unit[pointing].Pos),false,0,&(units.unit[pointing]));
											else
												units.unit[i].set_mission(MISSION_REPAIR,&(units.unit[pointing].Pos),false,0,true,&(units.unit[pointing]));
										}
										units.unit[i].unlock();
									}
									if (!TA3D_SHIFT_PRESSED)
										current_order=SIGNAL_ORDER_NONE;
									click_activated = true;
								}
								else
								{
									if (cursor_type == CURSOR_RECLAIM)
									{
										for (unsigned int e = 0; e < units.index_list_size; ++e)
										{
											units.lock();
											int i = units.idx_list[e];
											units.unlock();
											units.unit[i].lock();
											if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && units.unit[i].sel
												&& unit_manager.unit_type[units.unit[i].type_id]->CanReclamate && unit_manager.unit_type[units.unit[i].type_id]->BMcode)
											{
												if (TA3D_SHIFT_PRESSED)
													units.unit[i].add_mission(MISSION_RECLAIM,&(units.unit[pointing].Pos),false,0,&(units.unit[pointing]));
												else
													units.unit[i].set_mission(MISSION_RECLAIM,&(units.unit[pointing].Pos),false,0,true,&(units.unit[pointing]));
											}
											units.unit[i].unlock();
										}
										if (!TA3D_SHIFT_PRESSED)
											current_order=SIGNAL_ORDER_NONE;
										click_activated = true;
									}
									else
									{
										if (cursor_type==CURSOR_GUARD) // Le curseur donne un ordre
										{
											units.give_order_guard(players.local_human_id,pointing,!TA3D_SHIFT_PRESSED);
											if (!TA3D_SHIFT_PRESSED)
												current_order=SIGNAL_ORDER_NONE;
											click_activated = true;
										}
										else
										{
											if (cursor_type==CURSOR_LOAD) 	// Le curseur donne un ordre
											{
												units.give_order_load(players.local_human_id,pointing,!TA3D_SHIFT_PRESSED);
												if (!TA3D_SHIFT_PRESSED)
													current_order=SIGNAL_ORDER_NONE;
												click_activated = true;
											}
										}
									}
								}
							}
					}
				}
				else
				{
					if (!rope_selection)
					{
						cursor_type = CursorFromSignalOrder(current_order, cursor_type);

						if (left_click_activation)
						{
							if (cursor_type == CURSOR_ATTACK)
							{
								Vector3D cursor_pos(cursorOnMap(cam, *map, IsOnMinimap));
								for (unsigned int e = 0; e < units.index_list_size; ++e)
								{
									uint32 commandfire = current_order == SIGNAL_ORDER_DGUN ? MISSION_FLAG_COMMAND_FIRE : 0;
									int i = units.idx_list[e];
									units.unit[i].lock();
									if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && units.unit[i].sel && unit_manager.unit_type[units.unit[i].type_id]->canattack
										&& ( unit_manager.unit_type[units.unit[i].type_id]->BMcode || !unit_manager.unit_type[units.unit[i].type_id]->Builder))
									{
										for (unsigned int f = 0; f < unit_manager.unit_type[units.unit[i].type_id]->weapon.size(); ++f)
											if (unit_manager.unit_type[units.unit[i].type_id]->weapon[ f ] && unit_manager.unit_type[units.unit[i].type_id]->weapon[ f ]->stockpile)
											{
												commandfire = MISSION_FLAG_COMMAND_FIRE;
												break;
											}
										if (TA3D_SHIFT_PRESSED)
											units.unit[i].add_mission(MISSION_ATTACK,&(cursor_pos),false,0,NULL,NULL,commandfire);
										else
											units.unit[i].set_mission(MISSION_ATTACK,&(cursor_pos),false,0,true,NULL,NULL,commandfire);
									}
									units.unit[i].unlock();
								}
								if (!TA3D_SHIFT_PRESSED)
									current_order = SIGNAL_ORDER_NONE;
								click_activated = true;
							}
						}
					}
				}
			}

			if (cursor_type!=CURSOR_DEFAULT && click_activation && !IsOnGUI && TA3D_SHIFT_PRESSED && !order_removed) // Remove commands from queue
			{
				Vector3D target(cursorOnMap(cam, *map));
				target.x = ((int)(target.x) + map->map_w_d) >> 3;
				target.z = ((int)(target.z) + map->map_h_d) >> 3;
				target.x = target.x * 8.0f - map->map_w_d;
				target.z = target.z * 8.0f - map->map_h_d;
				target.y = Math::Max(map->get_unit_h(target.x, target.z), map->sealvl);
				order_removed = units.remove_order(players.local_human_id, target);
			}

			if (cursor_type == CURSOR_REVIVE && CURSOR_REVIVE != CURSOR_RECLAIM && !rope_selection && click_activation && ( !IsOnGUI || IsOnMinimap) && !order_removed) // The cursor orders to resurrect a wreckage
			{
				Vector3D cur_pos(cursorOnMap(cam, *map, IsOnMinimap));
				int idx = -units.last_on - 2;
				if (idx >= 0 && features.feature[idx].type >= 0 && feature_manager.getFeaturePointer(features.feature[idx].type)->reclaimable)
				{
					for (unsigned int e = 0; e < units.index_list_size; ++e)
					{
						units.lock();
						int i = units.idx_list[e];
						units.unlock();
						units.unit[i].lock();
						if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && units.unit[i].sel
							&& unit_manager.unit_type[units.unit[i].type_id]->canresurrect && unit_manager.unit_type[units.unit[i].type_id]->BMcode)
						{
							if (TA3D_SHIFT_PRESSED)
								units.unit[i].add_mission(MISSION_REVIVE,&cur_pos,false,idx,NULL);
							else
								units.unit[i].set_mission(MISSION_REVIVE,&cur_pos,false,idx,true,NULL);
						}
						units.unit[i].unlock();
					}
				}
				if (!TA3D_SHIFT_PRESSED)
					current_order=SIGNAL_ORDER_NONE;
				click_activated = true;
			}

			// The cursor orders to reclaim something
			if (cursor_type == CURSOR_RECLAIM && !rope_selection && click_activation && (!IsOnGUI || IsOnMinimap) && !order_removed)
			{
				Vector3D cur_pos(cursorOnMap(cam, *map, IsOnMinimap));
				int idx = -units.last_on - 2;
				if (idx >= 0 && features.feature[ idx ].type >= 0 && feature_manager.getFeaturePointer(features.feature[ idx ].type)->reclaimable)
				{
					for (unsigned int e = 0; e < units.index_list_size; ++e)
					{
						units.lock();
						int i = units.idx_list[e];
						units.unlock();
						units.unit[i].lock();
						if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && units.unit[i].sel
							&& unit_manager.unit_type[units.unit[i].type_id]->CanReclamate && unit_manager.unit_type[units.unit[i].type_id]->BMcode)
						{
							if (TA3D_SHIFT_PRESSED)
								units.unit[i].add_mission(MISSION_RECLAIM,&cur_pos,false,idx,NULL);
							else
								units.unit[i].set_mission(MISSION_RECLAIM,&cur_pos,false,idx,true,NULL);
						}
						units.unit[i].unlock();
					}
				}
				if (!TA3D_SHIFT_PRESSED)
					current_order=SIGNAL_ORDER_NONE;
				click_activated = true;
			}

			if (cursor_type==CURSOR_UNLOAD && !rope_selection && click_activation && ( !IsOnGUI || IsOnMinimap) && !order_removed)	// The cursor orders to unload units
			{
				units.give_order_unload(players.local_human_id, cursorOnMap(cam, *map, IsOnMinimap), !TA3D_SHIFT_PRESSED);
				if (!TA3D_SHIFT_PRESSED)
					current_order=SIGNAL_ORDER_NONE;
				click_activated = true;
			}

			if (cursor_type==CURSOR_MOVE && !rope_selection && click_activation && ( !IsOnGUI || IsOnMinimap) && !order_removed) 	// The cursor orders to move
			{
				units.give_order_move(players.local_human_id, cursorOnMap(cam, *map, IsOnMinimap), !TA3D_SHIFT_PRESSED);
				if (!TA3D_SHIFT_PRESSED)
					current_order=SIGNAL_ORDER_NONE;
				click_activated = true;
			}

			// The cursor orders to patrol
			if (cursor_type == CURSOR_PATROL && !rope_selection && click_activation && ( !IsOnGUI || IsOnMinimap) && !order_removed)
			{
				units.give_order_patrol(players.local_human_id, cursorOnMap(cam, *map, IsOnMinimap), !TA3D_SHIFT_PRESSED);
				if (!TA3D_SHIFT_PRESSED)
					current_order = SIGNAL_ORDER_NONE;
				click_activated = true;
			}

			// The cursor orders to build something
			if (build >= 0 && cursor_type == CURSOR_DEFAULT && mouse_b != 1 && omb3 == 1 && !IsOnGUI)
			{
				Vector3D target(cursorOnMap(cam, *map));
				pMouseRectSelection.x2 = ((int)(target.x) + map->map_w_d) >> 3;
				pMouseRectSelection.y2 = ((int)(target.z) + map->map_h_d) >> 3;

				int d = Math::Max(abs( pMouseRectSelection.x2 - pMouseRectSelection.x1), abs( pMouseRectSelection.y2 - pMouseRectSelection.y1));

				int ox = pMouseRectSelection.x1 + 0xFFFF;
				int oy = pMouseRectSelection.y1 + 0xFFFF;

				for (int c = 0; c <= d; ++c)
				{
					target.x = pMouseRectSelection.x1 + (pMouseRectSelection.x2 - pMouseRectSelection.x1) * c / Math::Max(d, 1);
					target.z = pMouseRectSelection.y1 + (pMouseRectSelection.y2 - pMouseRectSelection.y1) * c / Math::Max(d, 1);

					if (abs( ox - (int)target.x) < unit_manager.unit_type[build]->FootprintX
						&& abs( oy - (int)target.z) < unit_manager.unit_type[build]->FootprintZ)	continue;
					ox = (int)target.x;
					oy = (int)target.z;

					target.y = map->get_max_rect_h((int)target.x,(int)target.z, unit_manager.unit_type[build]->FootprintX, unit_manager.unit_type[build]->FootprintZ);
					if (unit_manager.unit_type[build]->floatting())
						target.y = Math::Max(target.y,map->sealvl+(unit_manager.unit_type[build]->AltFromSeaLevel-unit_manager.unit_type[build]->WaterLine)*H_DIV);
					target.x = target.x * 8.0f - map->map_w_d;
					target.z = target.z * 8.0f - map->map_h_d;

					can_be_there = can_be_built(target, map.get(), build, players.local_human_id);

					if (can_be_there)
					{
						units.give_order_build(players.local_human_id,build,target,!( TA3D_SHIFT_PRESSED || c != 0));
						build_order_given = true;
					}
				}
				if (build_order_given)
				{
					if (!TA3D_SHIFT_PRESSED)
						build=-1;
					sound_manager->playTDFSound("OKTOBUILD", "sound" , NULL);
				}
				else
					sound_manager->playTDFSound("NOTOKTOBUILD", "sound" , NULL);
				click_activated = true;
			}
			else
			{
				if (build>=0 && cursor_type==CURSOR_DEFAULT && mouse_b == 1 && omb3 != 1 && !IsOnGUI)// Giving the order to build a row
				{
					Vector3D target(cursorOnMap(cam, *map));
					pMouseRectSelection.x1 = ((int)(target.x) + map->map_w_d) >> 3;
					pMouseRectSelection.y1 = ((int)(target.z) + map->map_h_d) >> 3;
					click_activated = true;
				}
			}

			if (!TA3D_SHIFT_PRESSED && build_order_given)
				build = -1;

			if (build == -1)
				build_order_given = false;

			if (mouse_b != 1 && omb3 == 1 && !TA3D_SHIFT_PRESSED && (!IsOnGUI || IsOnMinimap))
				current_order = SIGNAL_ORDER_NONE;

			//---------------------------------	Code de sélection d'unités

			if (!IsOnGUI)
			{
				if ((mouse_b == 2 && omb3 != 2 && !lp_CONFIG->right_click_interface)
					|| (!click_activated && mouse_b == 1 && omb3 != 1 && current_order == SIGNAL_ORDER_NONE && lp_CONFIG->right_click_interface)) // Secondary mouse button cancels/deselects
				{
					if (current_order != SIGNAL_ORDER_NONE && current_order != SIGNAL_ORDER_MOVE)
						current_order = SIGNAL_ORDER_NONE;
					else
					{
						pMouseSelecting = false;
						if (build >= 0)
						{
							build = -1; // leave build mode
						}
						else
						{
							// Deselect units
							selected = false;
							cur_sel = -1;
							cur_sel_index = -1;
							for (unsigned int e = 0; e < units.index_list_size; ++e)
							{
								int i = units.idx_list[e];
								if (units.unit[i].owner_id == players.local_human_id) // On peut désélectionner les morts, ça ne change rien :-)
									units.unit[i].sel = false;
							}
						}
					}
				}
			}

			if (build == -1 && (!IsOnGUI || (pMouseSelecting && (mouse_y<32 || mouse_y>SCREEN_H-32)) || IsOnMinimap)) // Si le curseur est dans la zone de jeu
			{
				if ((mouse_b!=1 && pMouseSelecting) || ( IsOnMinimap && mouse_b == 1 && omb3 != 1))// Récupère les unités présentes dans la sélection
				{
					bool skip = false;
					if ((abs( pMouseRectSelection.x1 - pMouseRectSelection.x2) < PICK_TOLERANCE && abs(pMouseRectSelection.y1 - pMouseRectSelection.y2) < PICK_TOLERANCE) || IsOnMinimap)
					{
						if (cursor_type == CURSOR_DEFAULT || cursor_type == CURSOR_CROSS)
						{
							int pointing = IsOnMinimap ? units.pick_minimap() : units.pick(cam); // Select a unit from a single click
							if (!TA3D_SHIFT_PRESSED)
							{
								for (unsigned int e = 0; e < units.index_list_size; ++e)
								{
									int i = units.idx_list[e];
									if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id)
										units.unit[i].sel = false;
								}
							}
							if (pointing >= 0 && Yuni::Math::Zero(units.unit[pointing].port[BUILD_PERCENT_LEFT]))	// On ne sélectionne pas les unités en construction
								units.unit[pointing].sel ^= true; // Sélectionne/Désélectionne si l'unité est déjà sélectionnée en appuyant sur SHIFT
							selected = false;
							for (unsigned int e = 0; e < units.index_list_size; ++e)
							{
								int i = units.idx_list[e];
								if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id)
									selected|=units.unit[i].sel;
							}
						}
						else
							skip = true;
					}
					else
						selected = units.selectUnits(RectTest(cam, pMouseRectSelection));		// Séléction au lasso

					if (!skip)
					{
						if (selected)			// In order to refresh GUI
							old_sel = false;
						cur_sel = -1;
						cur_sel_index = -1;
						for (unsigned int e = 0; e < units.index_list_size && cur_sel != -2; ++e)
						{
							int i = units.idx_list[e];
							if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && units.unit[i].sel)
								cur_sel= (cur_sel==-1) ? i : -2;
						}
						if (cur_sel >= 0)
						{
							cur_sel_index=cur_sel;
							cur_sel=units.unit[cur_sel].type_id;
							// Let's do some noise
							units.unit[ cur_sel_index ].playSound( "select1");
						}
					}
				}
				pMouseSelecting = false;
				if (mouse_b==1 && !IsOnMinimap)
				{
					if (omb3 != 1)
					{
						pMouseRectSelection.x1 = mouse_x;
						pMouseRectSelection.y1 = mouse_y;
					}
					pMouseRectSelection.x2 = mouse_x;
					pMouseRectSelection.y2 = mouse_y;
					pMouseSelecting = true;
				}
			}
			else
				pMouseSelecting = false;


			omb3 = mouse_b;
			amx  = mouse_x;
			amy  = mouse_y;

			if (IsOnGUI && !IsOnMinimap)
				cursor_type=CURSOR_DEFAULT;

			if (!IsOnGUI && ( cursor_type == CURSOR_DEFAULT || units.last_on == -1))
			{
				units.pick(cam);		// Let's see what's under the cursor

				if (units.last_on == -1) // Is the cursor on a rock, tree, ...?
				{
					Vector3D cur_pos(cursorOnMap(cam, *map, IsOnMinimap));
					int px = ((int)(cur_pos.x + map->map_w_d)) >> 3;
					int py = ((int)(cur_pos.z + map->map_h_d)) >> 3;
					if (px >= 0 && px < map->bloc_w_db && py >= 0 && py < map->bloc_h_db && (SurfaceByte(map->view_map, px >> 1, py >> 1) & (1 << players.local_human_id)))
					{
						int idx = -map->map_data[py][px].unit_idx - 2;				// Basic check
						if (idx < 0 || features.feature[idx].type < 0)
						{
							units.last_on = -1;
							for (short int dy = -7; dy < 8; ++dy) // Look for things like metal patches
							{
								if (py + dy >= 0 && py + dy < map->bloc_h_db)
								{
									for (short int dx = -7; dx < 8; ++dx)
									{
										if (px + dx >= 0 && px + dx < map->bloc_w_db)
										{
											if (map->map_data[py+dy][px+dx].stuff >= 0)
											{
												idx = map->map_data[py+dy][px+dx].stuff;
												Feature *feature = feature_manager.getFeaturePointer(features.feature[idx].type);
												if (feature
													&& feature->footprintx + 1 >= (abs(dx) << 1)
													&& feature->footprintz + 1 >= (abs(dy) << 1))
												{
													units.last_on = -idx - 2;
													dy = 800;
													break;
												}
											}
										}
									}
								}
							}
						}
						else
							units.last_on = -idx - 2;
					}
				}
			}

			// Select CTRL_* category units
			if (TA3D_CTRL_PRESSED && (key[KEY_C] || key[KEY_F] || key[KEY_V] || key[KEY_B]))
			{
				String check_cat = "CTRL_";
				if (key[KEY_C])
					check_cat += "C";
				else
				{
					if (key[KEY_F])
						check_cat += "F";
					else
					{
						if (key[KEY_V])
							check_cat += "V";
						else
						{
							if (key[KEY_B])
								check_cat += "B";
						}
					}
				}
				for (unsigned int e = 0 ; e < units.index_list_size; ++e)
				{
					int i = units.idx_list[e];
					if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && Yuni::Math::Zero(units.unit[i].build_percent_left))
					{
						if (unit_manager.unit_type[units.unit[i].type_id]->checkCategory( check_cat.c_str()))
							units.unit[i].sel = true;
						else
						{
							if (!TA3D_SHIFT_PRESSED)
								units.unit[i].sel = false;
						}
					}
				}
				cur_sel = -1;
				cur_sel_index = -1;
				build = -1;
				for (unsigned int e = 0; e < units.index_list_size && cur_sel != -2; ++e)
				{
					int i = units.idx_list[e];
					if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && units.unit[i].sel)
						cur_sel= (cur_sel==-1) ? i : -2;
				}
				selected = (cur_sel!=-1);
				if (cur_sel >= 0)
				{
					cur_sel_index=cur_sel;
					cur_sel=units.unit[cur_sel].type_id;
				}
			}
			else
				if (TA3D_CTRL_PRESSED && key[KEY_Z]) // Séletionne toutes les unités dont le type est déjà sélectionné / Select units of the same type
				{
					bool *sel_type = new bool[unit_manager.nb_unit];
					for (int i = 0; i < unit_manager.nb_unit; ++i)
						sel_type[i] = false;
					for (unsigned int e = 0; e < units.index_list_size; ++e)
					{
						int i = units.idx_list[e];
						if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && units.unit[i].sel)
							sel_type[units.unit[i].type_id]=true;
					}
					for (unsigned int e = 0; e < units.index_list_size; ++e)
					{
						int i = units.idx_list[e];
						if ((units.unit[i].flags & 1) && Yuni::Math::Zero(units.unit[i].build_percent_left) && units.unit[i].owner_id==players.local_human_id && sel_type[units.unit[i].type_id])
							units.unit[i].sel=true;
					}
					cur_sel = -1;
					cur_sel_index = -1;
					for (unsigned int e = 0; e < units.index_list_size && cur_sel != -2; ++e)
					{
						int i = units.idx_list[e];
						if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && units.unit[i].sel)
							cur_sel= (cur_sel==-1) ? i : -2;
					}
					selected = (cur_sel != -1);
					build = -1;
					if (cur_sel >= 0)
					{
						cur_sel_index = cur_sel;
						cur_sel = units.unit[cur_sel].type_id;
					}
					delete[] sel_type;
				}
				else
				{
					if (TA3D_CTRL_PRESSED && key[KEY_A]) // Select all the player's units
					{
						for (unsigned int e = 0 ; e < units.index_list_size; ++e)
						{
							int i = units.idx_list[e];
							if ((units.unit[i].flags & 1) && Yuni::Math::Zero(units.unit[i].port[BUILD_PERCENT_LEFT]) && units.unit[i].owner_id == players.local_human_id)
								units.unit[i].sel = true;
						}
						cur_sel = -1;
						cur_sel_index = -1;
						for (unsigned int e = 0; e < units.index_list_size && cur_sel != -2; ++e)
						{
							int i = units.idx_list[e];
							if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && units.unit[i].sel)
								cur_sel= (cur_sel == -1) ? i : -2;
						}
						selected = (cur_sel != -1);
						build = -1;
						if (cur_sel >= 0)
						{
							cur_sel_index = cur_sel;
							cur_sel = units.unit[cur_sel].type_id;
						}
					}
					else
					{
						if (TA3D_CTRL_PRESSED) // Formation de groupes d'unités
						{
							int grpe = -1;
							if (key[KEY_0])	grpe = 0;
							if (key[KEY_1])	grpe = 1;
							if (key[KEY_2])	grpe = 2;
							if (key[KEY_3])	grpe = 3;
							if (key[KEY_4])	grpe = 4;
							if (key[KEY_5])	grpe = 5;
							if (key[KEY_6])	grpe = 6;
							if (key[KEY_7])	grpe = 7;
							if (key[KEY_8])	grpe = 8;
							if (key[KEY_9])	grpe = 9;

							if (grpe >= 0)
							{
								grpe = 1 << grpe;
								for (unsigned int e = 0; e < units.index_list_size; ++e)
								{
									int i = units.idx_list[e];
									if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id)
									{
										if (units.unit[i].sel)
											units.unit[i].groupe|=grpe;
										else if (!TA3D_SHIFT_PRESSED)
											units.unit[i].groupe&=~grpe;
									}
								}
							}
						}
						else
						{
							if (key[KEY_ALT]) // Restauration de groupes d'unités
							{
								int grpe = -1;
								if (key[KEY_0])	grpe = 0;
								if (key[KEY_1])	grpe = 1;
								if (key[KEY_2])	grpe = 2;
								if (key[KEY_3])	grpe = 3;
								if (key[KEY_4])	grpe = 4;
								if (key[KEY_5])	grpe = 5;
								if (key[KEY_6])	grpe = 6;
								if (key[KEY_7])	grpe = 7;
								if (key[KEY_8])	grpe = 8;
								if (key[KEY_9])	grpe = 9;

								if (grpe >= 0)
								{
									build=-1;
									grpe = 1 << grpe;
									for (unsigned int e = 0; e < units.index_list_size; ++e)
									{
										int i = units.idx_list[e];
										if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id)
										{
											if (units.unit[i].groupe&grpe)
												units.unit[i].sel = true;
											else
												if (!TA3D_SHIFT_PRESSED)
													units.unit[i].sel = false;
										}
									}
								}

								cur_sel = -1;
								cur_sel_index = -1;
								for (unsigned int e = 0; e < units.index_list_size && cur_sel != -2; ++e)
								{
									int i = units.idx_list[e];
									if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && units.unit[i].sel)
										cur_sel= (cur_sel==-1) ? i : -2;
								}
								selected = (cur_sel != -1);
								if (cur_sel >= 0)
								{
									cur_sel_index=cur_sel;
									cur_sel=units.unit[cur_sel].type_id;
								}
							}
						}
					}
				}

			/*--------------bloc regroupant ce qui est relatif au temps-------------------*/

			// That code was rewritten multithreaded
			if (!lp_CONFIG->pause)
			{
				const float timetosimulate = dt * units.apparent_timefactor;// Physics calculations take place here
				wind_change = false; // Don't try to run following code in separate thread
				features.move(timetosimulate); // Animate objects
				fx_manager.move(timetosimulate);
			}

			/*----------------------------------------------------------------------------*/

			cam_h = cam.rpos.y - map->get_unit_h(cam.rpos.x, cam.rpos.z);

			updateZFAR();

			gfx->SetDefState();
			updateFOG();

			// Dessine les reflets sur l'eau / Render water reflection
			if (g_useProgram && g_useFBO && lp_CONFIG->water_quality >= 2 && map->water && !map->ota_data.lavaworld && !reflection_drawn_last_time)
			{
				reflection_drawn_last_time = true;

				gfx->clearAll();		// Clear screen

				glViewport(0, 0, 512, 512);

				gfx->ReInitAllTex();
				glColor4f(1.0f,1.0f,1.0f,1.0f);
				glDisable(GL_BLEND);

				double eqn[4]= { 0.0f, 1.0f, 0.0f, -map->sealvl };

				Camera refcam = cam;
				refcam.zfar *= 2.0f;
				refcam.mirror = true;
				refcam.mirrorPos = -2.0f * map->sealvl;

				refcam.setView();
				glClipPlane(GL_CLIP_PLANE1, eqn);
				glEnable(GL_CLIP_PLANE1);

				pSun.Set(refcam);
				pSun.Enable();

				refcam.zfar *= 100.0f;
				refcam.setView();
				glColor4ub(0xFF,0xFF,0xFF,0xFF);
				glEnable(GL_TEXTURE_2D);
				if (lp_CONFIG->render_sky)
				{
					glDisable(GL_FOG);
					glBindTexture(GL_TEXTURE_2D,sky);
					glDisable(GL_LIGHTING);
					glDepthMask(GL_FALSE);
					if (pSkyIsSpherical)
					{
						glCullFace(GL_FRONT);
						glTranslatef(cam.rpos.x,-map->sealvl,cam.rpos.z);
						glRotatef( sky_angle, 0.0f, 1.0f, 0.0f);
						float scale_factor = 15.0f * ( cam.rpos.y + cam.shakeVector.y + sky_obj.w) / sky_obj.w;
						glScalef( scale_factor, scale_factor, scale_factor);
						sky_obj.draw();
					}
					else
					{
						glBegin(GL_QUADS);
						glTexCoord2f(0.0f,map->map_h*0.002f);				glVertex3f(-2.0f*map->map_w,300.0f,2.0f*map->map_h);
						glTexCoord2f(map->map_w*0.002f,map->map_h*0.002f);	glVertex3f(2.0f*map->map_w,300.0f,2.0f*map->map_h);
						glTexCoord2f(map->map_w*0.002f,0.0f);				glVertex3f(2.0f*map->map_w,300.0f,-2.0f*map->map_h);
						glTexCoord2f(0.0f,0.0f);							glVertex3f(-2.0f*map->map_w,300.0f,-2.0f*map->map_h);
						glEnd();
					}
				}
				refcam.zfar = (500.0f + (cam_h - 150.0f) * 2.0f) * 2.0f;
				glDepthMask(GL_TRUE);
				glEnable(GL_CULL_FACE);
				glEnable(GL_LIGHTING);
				glEnable(GL_FOG);
				glCullFace(GL_FRONT);
				refcam.setView();

				if (cam.rpos.y <= gfx->low_def_limit && lp_CONFIG->water_quality >= 4)
				{
					if (lp_CONFIG->wireframe)
						glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);

					map->draw(&refcam, (1 << players.local_human_id),  false, 0.0f, t,
							  dt * units.apparent_timefactor,
							  false, false, false);

					if (lp_CONFIG->wireframe)
						glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

					// Dessine les éléments "2D" / "sprites"
					features.draw();
                    refcam.setView();
                    // Dessine les unités / draw units
					units.draw(map.get(), false, true, false, lp_CONFIG->height_line);

					glDisable(GL_CULL_FACE);
					// Dessine les objets produits par les armes / draw weapons
					weapons.draw(map.get());
					// Dessine les particules
					refcam.setView(true);
					glClipPlane(GL_CLIP_PLANE1, eqn);

					particle_engine.draw(&refcam, map->map_w, map->map_h, map->bloc_w, map->bloc_h, map->view);

					refcam.setView();
					glClipPlane(GL_CLIP_PLANE1, eqn);
					// Effets spéciaux en surface / fx above water
					fx_manager.draw(refcam, map.get(), map->sealvl);
				}

				glDisable(GL_CLIP_PLANE1);

				gfx->ReInitAllTex(true);

				glColor4ub(0xFF,0xFF,0xFF,0xFF);
				glDisable(GL_BLEND);

				glBindTexture(GL_TEXTURE_2D,reflectex);								// Store what's on screen for reflection effect
				glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 512, 512, 0);

				glViewport(0, 0, SCREEN_W, SCREEN_H);

				gfx->SetDefState();
			}
			else
				reflection_drawn_last_time = false;

			if (lp_CONFIG->shadow_quality > 0 && cam.rpos.y <= gfx->low_def_limit)
			{
				switch (lp_CONFIG->shadow_quality)
				{
					case 2:                     // Render the shadow map
						gfx->setShadowMapMode(true);
						gfx->SetDefState();
						gfx->renderToTextureDepth( gfx->get_shadow_map() );
						gfx->clearDepth();
						pSun.SetView( map->get_visible_volume() );

						// We'll need this matrix later (when rendering with shadows)
						gfx->readShadowMapProjectionMatrix();

						glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
						glDisable(GL_FOG);
						glShadeModel (GL_FLAT);

						glEnable(GL_POLYGON_OFFSET_FILL);
						glPolygonOffset(3.0f, 1.0f);

						// Render all visible features from light's point of view
						for(int i=0;i<features.list_size;i++)
							features.feature[features.list[i]].draw = true;
						features.draw(true);

						glEnable(GL_POLYGON_OFFSET_FILL);
						glPolygonOffset(3.0f, 1.0f);
						// Render all visible units from light's point of view
						units.draw(map.get(), true, false, true, false);
						units.draw(map.get(), false, false, true, false);

						// Render all visible weapons from light's point of view
						weapons.draw(map.get(), true);
						weapons.draw(map.get(), false);

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

			gfx->SetDefState();
			glClearColor(FogColor[0],FogColor[1],FogColor[2],FogColor[3]);
			if (pSkyIsSpherical)
				gfx->clearDepth();		// Clear screen
			else
				gfx->clearAll();

			cam.setView();

			pSun.Set(cam);
			pSun.Enable();

			cam.setView();

			glDisable(GL_FOG);
			glColor3ub( 0, 0, 0);				// Black background
			glDisable( GL_TEXTURE_2D);
			glDepthMask(GL_FALSE);
			glBegin( GL_QUADS);
			glVertex3i( -map->map_w, 0, map->map_h);
			glVertex3i( map->map_w, 0, map->map_h);
			glVertex3i( map->map_w, 0, -map->map_h);
			glVertex3i( -map->map_w, 0, -map->map_h);
			glEnd();
			glDepthMask(GL_TRUE);

			cam.zfar *= 100.0f;
			cam.setView();
			glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
			glEnable(GL_TEXTURE_2D);
			if (lp_CONFIG->render_sky)
			{
				glBindTexture(GL_TEXTURE_2D,sky);
				glDisable(GL_LIGHTING);
				glDepthMask(GL_FALSE);
				if (pSkyIsSpherical)
				{
					glTranslatef(cam.rpos.x, cam.rpos.y+cam.shakeVector.y, cam.rpos.z);
					glRotatef(sky_angle, 0.0f, 1.0f, 0.0f);
					if (lp_CONFIG->ortho_camera)
					{
						float scale = cam.zoomFactor / 800.0f * sqrtf(SCREEN_H * SCREEN_H + SCREEN_W * SCREEN_W);
						glScalef( scale, scale, scale );
					}
					sky_obj.draw();
					if (!sky_obj.full)
					{
						glScalef( 1.0f, -1.0f, 1.0f);
						glCullFace( GL_FRONT);
						sky_obj.draw();
						glCullFace( GL_BACK);
					}
				}
				else
				{
					glBegin(GL_QUADS);
					glTexCoord2f(0.0f,0.0f);							glVertex3f(-2.0f*map->map_w,300.0f,-2.0f*map->map_h);
					glTexCoord2f(map->map_w*0.002f,0.0f);				glVertex3f(2.0f*map->map_w,300.0f,-2.0f*map->map_h);
					glTexCoord2f(map->map_w*0.002f,map->map_h*0.002f);	glVertex3f(2.0f*map->map_w,300.0f,2.0f*map->map_h);
					glTexCoord2f(0.0f,map->map_h*0.002f);				glVertex3f(-2.0f*map->map_w,300.0f,2.0f*map->map_h);
					glEnd();
				}
			}
			glDepthMask(GL_TRUE);
			glEnable(GL_CULL_FACE);
			glEnable(GL_LIGHTING);
			glEnable(GL_FOG);
			updateZFAR();

			if (lp_CONFIG->wireframe)
				glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);

			map->draw(&cam,1<<players.local_human_id,false,0.0f,t,dt*units.apparent_timefactor);

			if (lp_CONFIG->wireframe)
				glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

            cam.setView(true);

            features.draw();		// Dessine les éléments "2D"

            /*----------------------------------------------------------------------------------------------*/

			// Dessine les unités sous l'eau / Draw units which are under water
            cam.setView(lp_CONFIG->shadow_quality < 2);
            if (cam.rpos.y <= gfx->low_def_limit)
            {
                if (lp_CONFIG->shadow_quality >= 2)
                    glFogi (GL_FOG_COORD_SRC, GL_FOG_COORD);
                units.draw(map.get(), true, false, true, lp_CONFIG->height_line);
                glFogi (GL_FOG_COORD_SRC, GL_FRAGMENT_DEPTH);
            }

			// Dessine les objets produits par les armes sous l'eau / Draw weapons which are under water
			weapons.draw(map.get(), true);

			if (map->water)
			{
				// Effets spéciaux sous-marins / Draw fx which are under water
				fx_manager.draw(cam, map.get(), map->sealvl, true);
			}


			if (map->water)
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				gfx->ReInitAllTex(true);

				if (!g_useProgram || !g_useFBO || lp_CONFIG->water_quality < 2)
				{
					gfx->set_alpha_blending();
					if (lp_CONFIG->water_quality == 1) // lp_CONFIG->water_quality=1
					{
						glColor4f(1.0f,1.0f,1.0f,0.5f);

						glActiveTextureARB(GL_TEXTURE0_ARB);
						glEnable(GL_TEXTURE_2D);
						glClientActiveTextureARB(GL_TEXTURE0_ARB);

						map->draw(&cam,1,true,map->sealvl,t,dt*lp_CONFIG->timefactor);
					}
					else 	// lp_CONFIG->water_quality=0
					{
						glColor4f(1.0f,1.0f,1.0f,0.5f);
						glDisable(GL_LIGHTING);

						glActiveTextureARB(GL_TEXTURE0_ARB);
						glEnable(GL_TEXTURE_2D);
						glBindTexture(GL_TEXTURE_2D,map->low_tex);

						cam.setView(true);
						glTranslatef(0.0f, map->sealvl, map->sea_dec);
						water_obj->draw(t,false);
						glColor4f(1.0f,1.0f,1.0f,0.75f);

						glEnable(GL_LIGHTING);
						glActiveTextureARB(GL_TEXTURE0_ARB);
						gfx->ReInitTexSys();
						glEnable(GL_TEXTURE_2D);
					}
					gfx->unset_alpha_blending();
				}
				else if (lp_CONFIG->water_quality <= 4)
				{
					glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
					glDisable(GL_LIGHTING);

					// First pass of water rendering, store reflection vector
					glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,water_FBO);
					glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,first_pass,0);

					//			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Efface la texture tampon
					glClear(GL_DEPTH_BUFFER_BIT);		// Efface la texture tampon

					glViewport(0,0,512,512);

					glActiveTextureARB(GL_TEXTURE0_ARB);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D,map->lava_map);
					glClientActiveTextureARB(GL_TEXTURE0_ARB);

					glActiveTextureARB(GL_TEXTURE1_ARB);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D,water);
					glClientActiveTextureARB(GL_TEXTURE1_ARB);

					if (lp_CONFIG->water_quality == 2)
					{
						water_pass1_low.on();
						water_pass1_low.setvar1i("lava",0);
						water_pass1_low.setvar1i("map",1);
						water_pass1_low.setvar1f("t",t);
						water_pass1_low.setvar2f("factor", water_obj->w / map->map_w, water_obj->w / map->map_h);
					}
					else
					{
						water_pass1.on();
						water_pass1.setvar1i("lava",0);
						water_pass1.setvar1i("map",1);
						water_pass1.setvar1f("t",t);
						water_pass1.setvar2f("factor",water_obj->w / map->map_w, water_obj->w / map->map_h);
					}

					cam.setView(true);
					glTranslatef(0.0f,map->sealvl,0.0f);
					water_obj->draw(t, true);

					if (lp_CONFIG->water_quality == 2)
						water_pass1_low.off();
					else
						water_pass1.off();

					glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,second_pass,0);					// Second pass of water rendering, store viewing vector

					//			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Efface la texture tampon
					glClear(GL_DEPTH_BUFFER_BIT);		// Efface la texture tampon

					glActiveTextureARB(GL_TEXTURE0_ARB);
					glDisable(GL_TEXTURE_2D);

					glActiveTextureARB(GL_TEXTURE1_ARB);
					glDisable(GL_TEXTURE_2D);

					water_pass2.on();

					cam.setView(true);
					glTranslatef(0.0f,map->sealvl,0.0f);
					water_obj->draw(t, true);

					water_pass2.off();

					if (lp_CONFIG->water_quality > 2)
					{
						glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,water_color,0);					// Third pass of water rendering, store water color

						glClear(GL_DEPTH_BUFFER_BIT);		// Efface la texture tampon

						glActiveTextureARB(GL_TEXTURE0_ARB);
						glEnable(GL_TEXTURE_2D);
						glBindTexture(GL_TEXTURE_2D,map->low_tex);

						cam.setView();
						glTranslatef( 0.0f, map->sealvl, map->sea_dec);
						water_obj->draw(t, false);
					}

					glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);

					glViewport(0, 0, SCREEN_W, SCREEN_H);

					float logw = logf((float)SCREEN_W) / logf(2.0f);
					float logh = logf((float)SCREEN_H) / logf(2.0f);
					int wx = logw>(int)logw ? (int)logw+1 : (int)logw;
					int wy = logh>(int)logh ? (int)logh+1 : (int)logh;
					wx = 1 << wx;
					wy = 1 << wy;
					glBindTexture(GL_TEXTURE_2D,transtex);								// Store what's on screen for transparency effect
					glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, wx, wy, 0);

					glEnable(GL_STENCIL_TEST);											// Draw basic water in order to have correct texture mapping
					glClear(GL_STENCIL_BUFFER_BIT);
					glStencilFunc(GL_ALWAYS,128, 0xffffffff);
					glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

					glActiveTextureARB(GL_TEXTURE0_ARB);
					glDisable(GL_TEXTURE_2D);
					glClientActiveTextureARB(GL_TEXTURE0_ARB);

					cam.setView(true);
					glTranslatef(0.0f,map->sealvl,0.0f);
					water_obj->draw(t, true);

					glDisable(GL_STENCIL_TEST);

					glMatrixMode(GL_TEXTURE);
					glLoadIdentity();
					glMatrixMode(GL_MODELVIEW);

					glEnable(GL_LIGHTING);

					glActiveTextureARB(GL_TEXTURE0_ARB);
					if (map->ota_data.lavaworld)
						glBindTexture(GL_TEXTURE_2D,sky);
					else
						glBindTexture(GL_TEXTURE_2D,reflectex);
					glEnable(GL_TEXTURE_2D);

					glActiveTextureARB(GL_TEXTURE1_ARB);
					glBindTexture(GL_TEXTURE_2D,transtex);
					glEnable(GL_TEXTURE_2D);

					glActiveTextureARB(GL_TEXTURE2_ARB);
					glBindTexture(GL_TEXTURE_2D,first_pass);
					glEnable(GL_TEXTURE_2D);

					glActiveTextureARB(GL_TEXTURE3_ARB);
					glBindTexture(GL_TEXTURE_2D,second_pass);
					glEnable(GL_TEXTURE_2D);

					if (lp_CONFIG->water_quality == 2)
					{
						water_shader.on();
						water_shader.setvar1i("sky",0);
						water_shader.setvar1i("rtex",1);
						water_shader.setvar1i("bump",2);
						water_shader.setvar1i("view",3);
						water_shader.setvar2f("coef", (float)SCREEN_W / wx, (float)SCREEN_H / wy);
					}
					else
					{
						glActiveTextureARB(GL_TEXTURE4_ARB);
						glBindTexture(GL_TEXTURE_2D,water_color);
						glEnable(GL_TEXTURE_2D);

						water_shader_reflec.on();
						water_shader_reflec.setvar1i("sky",0);
						water_shader_reflec.setvar1i("rtex",1);
						water_shader_reflec.setvar1i("bump",2);
						water_shader_reflec.setvar1i("view",3);
						water_shader_reflec.setvar1i("water_color",4);
						water_shader_reflec.setvar2f("coef", (float)SCREEN_W / wx, (float)SCREEN_H / wy);
					}

					glColor4ub(0xFF,0xFF,0xFF,0xFF);
					glDisable(GL_DEPTH_TEST);

					glMatrixMode(GL_PROJECTION);
					glLoadIdentity();
					glOrtho(0, SCREEN_W, SCREEN_H, 0, -1.0, 1.0);
					glMatrixMode(GL_MODELVIEW);
					glLoadIdentity();

					glEnable(GL_STENCIL_TEST);
					glStencilFunc(GL_NOTEQUAL,0, 0xffffffff);
					glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
					glBegin(GL_QUADS);
					glTexCoord2f(0.0f,1.0f);	glVertex3f(0,0,0);
					glTexCoord2f(1.0f,1.0f);	glVertex3f(SCREEN_W,0,0);
					glTexCoord2f(1.0f,0.0f);	glVertex3f(SCREEN_W,SCREEN_H,0);
					glTexCoord2f(0.0f,0.0f);	glVertex3f(0,SCREEN_H,0);
					glEnd();
					glDisable(GL_STENCIL_TEST);
					glEnable(GL_DEPTH_TEST);

					if (lp_CONFIG->water_quality == 2)
						water_shader.off();
					else
						water_shader_reflec.off();
				}
				else                            // New Ultimate quality water renderer
				{
					// Run water simulation entirely on the GPU
					glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,water_FBO);
					glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,water_sim,0);

					glViewport(0,0,256,256);

					glDisable(GL_DEPTH_TEST);
					glDisable(GL_LIGHTING);

					glMatrixMode (GL_PROJECTION);
					glLoadIdentity ();
					glMatrixMode (GL_MODELVIEW);
					glLoadIdentity();

					glActiveTextureARB(GL_TEXTURE0_ARB);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D,water_sim);

					const float time_step = 0.02f;
					const float time_to_simulate = Math::Min( dt * units.apparent_timefactor, time_step * 3.0f );

					// Simulate water
					for(float real_time = 0.0f ; real_time < time_to_simulate ; real_time += time_step)
					{
						bool refresh = false;
						if (msec_timer - last_water_refresh >= 100000)
						{
							last_water_refresh = msec_timer;
							refresh = true;
						}
						float dt_step = Math::Min( time_to_simulate - real_time, time_step );
						water_simulator_shader.on();
						water_simulator_shader.setvar1i("sim",0);
						water_simulator_shader.setvar1f("fluid",50.0f * dt_step);
						water_simulator_shader.setvar1f("t", refresh ? 1.0f : 0.0f);

						glBegin( GL_QUADS );
						glTexCoord2i( 0, 0 ); glVertex2i( -1, -1 );
						glTexCoord2i( 1, 0 ); glVertex2i( 1, -1 );
						glTexCoord2i( 1, 1 ); glVertex2i( 1, 1 );
						glTexCoord2i( 0, 1 ); glVertex2i( -1, 1 );
						glEnd();

						water_simulator_shader.off();

						water_simulator_shader2.on();
						water_simulator_shader2.setvar1i("sim",0);
						water_simulator_shader2.setvar1f("dt", dt_step);

						glBegin( GL_QUADS );
						glTexCoord2i( 0, 0 ); glVertex2i( -1, -1 );
						glTexCoord2i( 1, 0 ); glVertex2i( 1, -1 );
						glTexCoord2i( 1, 1 ); glVertex2i( 1, 1 );
						glTexCoord2i( 0, 1 ); glVertex2i( -1, 1 );
						glEnd();

						water_simulator_shader2.off();
					}

					glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,water_sim2,0);

					glActiveTextureARB(GL_TEXTURE0_ARB);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D,water_sim);

					water_simulator_shader3.on();
					water_simulator_shader3.setvar1i("sim",0);

					glBegin( GL_QUADS );
					glTexCoord2i( 0, 0 ); glVertex2i( -1, -1 );
					glTexCoord2i( 1, 0 ); glVertex2i( 1, -1 );
					glTexCoord2i( 1, 1 ); glVertex2i( 1, 1 );
					glTexCoord2i( 0, 1 ); glVertex2i( -1, 1 );
					glEnd();

					water_simulator_shader3.off();

					glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
					glDisable(GL_LIGHTING);

					glEnable(GL_DEPTH_TEST);

					// First pass of water rendering, store reflection vector
					glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,water_FBO);
					glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,first_pass,0);

					glClear(GL_DEPTH_BUFFER_BIT);		// Efface la texture tampon

					glViewport(0,0,512,512);

					glActiveTextureARB(GL_TEXTURE0_ARB);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D,map->lava_map);

					water_simulator_shader4.on();
					water_simulator_shader4.setvar1i("lava",0);
					water_simulator_shader4.setvar1f("t",t);
					water_simulator_shader4.setvar2f("factor",water_obj->w / map->map_w, water_obj->w / map->map_h);

					cam.setView(true);
					glTranslatef(0.0f,map->sealvl,0.0f);
					water_obj->draw(t, true);

					water_simulator_shader4.off();

					glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,second_pass,0);					// Second pass of water rendering, store viewing vector

					glClear(GL_DEPTH_BUFFER_BIT);		// Efface la texture tampon

					glActiveTextureARB(GL_TEXTURE0_ARB);
					glDisable(GL_TEXTURE_2D);

					glActiveTextureARB(GL_TEXTURE1_ARB);
					glDisable(GL_TEXTURE_2D);

					water_pass2.on();

					cam.setView(true);
					glTranslatef(0.0f,map->sealvl,0.0f);
					water_obj->draw(t, true);

					water_pass2.off();

					glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,water_color,0);					// Third pass of water rendering, store water color

					glClear(GL_DEPTH_BUFFER_BIT);		// Efface la texture tampon

					glActiveTextureARB(GL_TEXTURE0_ARB);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D,map->low_tex);

					cam.setView(true);
					glTranslatef( 0.0f, map->sealvl, map->sea_dec);
					water_obj->draw(t, false);

					gfx->renderToTexture( 0 );

					float logw = logf((float)SCREEN_W) / logf(2.0f);
					float logh = logf((float)SCREEN_H) / logf(2.0f);
					int wx = logw>(int)logw ? (int)logw+1 : (int)logw;
					int wy = logh>(int)logh ? (int)logh+1 : (int)logh;
					wx = 1 << wx;
					wy = 1 << wy;
					glBindTexture(GL_TEXTURE_2D,transtex);								// Store what's on screen for transparency effect
					glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, wx, wy, 0);

					glEnable(GL_STENCIL_TEST);											// Draw basic water in order to have correct texture mapping
					glClear(GL_STENCIL_BUFFER_BIT);
					glStencilFunc(GL_ALWAYS,128, 0xffffffff);
					glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

					glActiveTextureARB(GL_TEXTURE0_ARB);
					glDisable(GL_TEXTURE_2D);

					cam.setView(true);
					glTranslatef(0.0f,map->sealvl,0.0f);
					water_obj->draw(t, true);

					glDisable(GL_STENCIL_TEST);

					glMatrixMode(GL_TEXTURE);
					glLoadIdentity();
					glMatrixMode(GL_MODELVIEW);

					glEnable(GL_LIGHTING);

					glActiveTextureARB(GL_TEXTURE0_ARB);
					if (map->ota_data.lavaworld)
						glBindTexture(GL_TEXTURE_2D,sky);
					else
						glBindTexture(GL_TEXTURE_2D,reflectex);
					glEnable(GL_TEXTURE_2D);

					glActiveTextureARB(GL_TEXTURE1_ARB);
					glBindTexture(GL_TEXTURE_2D,transtex);
					glEnable(GL_TEXTURE_2D);

					glActiveTextureARB(GL_TEXTURE2_ARB);
					glBindTexture(GL_TEXTURE_2D,first_pass);
					glEnable(GL_TEXTURE_2D);

					glActiveTextureARB(GL_TEXTURE3_ARB);
					glBindTexture(GL_TEXTURE_2D,second_pass);
					glEnable(GL_TEXTURE_2D);

					glActiveTextureARB(GL_TEXTURE4_ARB);
					glBindTexture(GL_TEXTURE_2D,water_color);
					glEnable(GL_TEXTURE_2D);

					glActiveTextureARB(GL_TEXTURE5_ARB);
					glBindTexture(GL_TEXTURE_2D,height_tex);
					glEnable(GL_TEXTURE_2D);

					glActiveTextureARB(GL_TEXTURE6_ARB);
					glBindTexture(GL_TEXTURE_2D,water_sim2);
					glEnable(GL_TEXTURE_2D);

					water_simulator_reflec.on();
					water_simulator_reflec.setvar1i("sky",0);
					water_simulator_reflec.setvar1i("rtex",1);
					water_simulator_reflec.setvar1i("bump",2);
					water_simulator_reflec.setvar1i("view",3);
					water_simulator_reflec.setvar1i("water_color",4);
					water_simulator_reflec.setvar1i("height_map",5);
					water_simulator_reflec.setvar1i("normal_map",6);
					water_simulator_reflec.setvar2f("coef", (float)SCREEN_W / wx, (float)SCREEN_H / wy);
					water_simulator_reflec.setvar1f("cam_h_factor", 1.0f / cam.rpos.y);
					water_simulator_reflec.setvar2f("factor",water_obj->w / map->map_w, water_obj->w / map->map_h);
					water_simulator_reflec.setvar1f("t", t);

					glColor4ub(0xFF,0xFF,0xFF,0xFF);
					glDisable(GL_DEPTH_TEST);

					glMatrixMode(GL_PROJECTION);
					glLoadIdentity();
					glOrtho(0, SCREEN_W, SCREEN_H, 0, -1.0, 1.0);
					glMatrixMode(GL_MODELVIEW);
					glLoadIdentity();

					glEnable(GL_STENCIL_TEST);
					glStencilFunc(GL_NOTEQUAL,0, 0xffffffff);
					glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
					glBegin(GL_QUADS);
					glTexCoord2f(0.0f,1.0f);	glVertex3f(0,0,0);
					glTexCoord2f(1.0f,1.0f);	glVertex3f(SCREEN_W,0,0);
					glTexCoord2f(1.0f,0.0f);	glVertex3f(SCREEN_W,SCREEN_H,0);
					glTexCoord2f(0.0f,0.0f);	glVertex3f(0,SCREEN_H,0);
					glEnd();
					glDisable(GL_STENCIL_TEST);
					glEnable(GL_DEPTH_TEST);

					water_simulator_reflec.off();
				}
				gfx->ReInitAllTex(true);
			}
            cam.setView();

			if (build >= 0 && !IsOnGUI)	// Display the building we want to build (with nice selection quads)
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

				int d = Math::Max(abs(pMouseRectSelection.x2 - pMouseRectSelection.x1), abs( pMouseRectSelection.y2 - pMouseRectSelection.y1));

				int ox = pMouseRectSelection.x1 + 0xFFFF;
				int oy = pMouseRectSelection.y1 + 0xFFFF;

				for (int c = 0; c <= d; ++c)
				{
					target.x = pMouseRectSelection.x1 + (pMouseRectSelection.x2 - pMouseRectSelection.x1) * c / Math::Max(d, 1);
					target.z = pMouseRectSelection.y1 + (pMouseRectSelection.y2 - pMouseRectSelection.y1) * c / Math::Max(d, 1);

					if (abs( ox - (int)target.x) < unit_manager.unit_type[build]->FootprintX
						&& abs( oy - (int)target.z) < unit_manager.unit_type[build]->FootprintZ)
						continue;
					ox = (int)target.x;
					oy = (int)target.z;

					target.y = map->get_max_rect_h((int)target.x,(int)target.z, unit_manager.unit_type[build]->FootprintX, unit_manager.unit_type[build]->FootprintZ);
					if (unit_manager.unit_type[build]->floatting())
						target.y = Math::Max(target.y,map->sealvl+(unit_manager.unit_type[build]->AltFromSeaLevel-unit_manager.unit_type[build]->WaterLine)*H_DIV);
					target.x = target.x * 8.0f - map->map_w_d;
					target.z = target.z * 8.0f - map->map_h_d;

					can_be_there = can_be_built(target, map.get(), build, players.local_human_id);

					cam.setView();

					glTranslatef(target.x,target.y,target.z);
					glScalef(unit_manager.unit_type[build]->Scale,unit_manager.unit_type[build]->Scale,unit_manager.unit_type[build]->Scale);
					float DX = (unit_manager.unit_type[build]->FootprintX<<2);
					float DZ = (unit_manager.unit_type[build]->FootprintZ<<2);
					if (unit_manager.unit_type[build]->model)
					{
						glEnable(GL_CULL_FACE);
						gfx->ReInitAllTex( true);
						if (can_be_there)
							glColor4ub(0xFF,0xFF,0xFF,0xFF);
						else
							glColor4ub(0xFF,0,0,0xFF);
						glDepthFunc( GL_GREATER );
						unit_manager.unit_type[build]->model->draw(0.0f,NULL,false,false,false,0,NULL,NULL,NULL,0.0f,NULL,false,players.local_human_id,false);
						glDepthFunc( GL_LESS );
						unit_manager.unit_type[build]->model->draw(0.0f,NULL,false,false,false,0,NULL,NULL,NULL,0.0f,NULL,false,players.local_human_id,false);

						bool old_mode = gfx->getShadowMapMode();
						gfx->setShadowMapMode(true);
						double eqn[4]= { 0.0f, -1.0f, 0.0f, map->sealvl - target.y };
						glClipPlane(GL_CLIP_PLANE2, eqn);

						glEnable(GL_CLIP_PLANE2);

						glEnable( GL_BLEND );
						glBlendFunc( GL_ONE, GL_ONE );
						glDepthFunc( GL_EQUAL );
						glColor4ub( 0x7F, 0x7F, 0x7F, 0x7F );
						unit_manager.unit_type[build]->model->draw(0.0f,NULL,false,true,false,0,NULL,NULL,NULL,0.0f,NULL,false,players.local_human_id,false);
						glColor4ub( 0xFF, 0xFF, 0xFF, 0xFF );
						glDepthFunc( GL_LESS );
						glDisable( GL_BLEND );

						glDisable(GL_CLIP_PLANE2);
						gfx->setShadowMapMode(old_mode);
					}
					cam.setView();
					glTranslatef(target.x,Math::Max( target.y, map->sealvl ),target.z);
					float red=1.0f, green=0.0f;
					if (can_be_there)
					{
						green = 1.0f;
						red   = 0.0f;
					}
					glDisable(GL_CULL_FACE);
					glDisable(GL_TEXTURE_2D);
					glDisable(GL_LIGHTING);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
					glBegin(GL_QUADS);
					glColor4f(red,green,0.0f,1.0f);
					glVertex3f(-DX,0.0f,-DZ);			// First quad
					glVertex3f(DX,0.0f,-DZ);
					glColor4f(red,green,0.0f,0.0f);
					glVertex3f(DX+2.0f,5.0f,-DZ-2.0f);
					glVertex3f(-DX-2.0f,5.0f,-DZ-2.0f);

					glColor4f(red,green,0.0f,1.0f);
					glVertex3f(-DX,0.0f,-DZ);			// Second quad
					glVertex3f(-DX,0.0f,DZ);
					glColor4f(red,green,0.0f,0.0f);
					glVertex3f(-DX-2.0f,5.0f,DZ+2.0f);
					glVertex3f(-DX-2.0f,5.0f,-DZ-2.0f);

					glColor4f(red,green,0.0f,1.0f);
					glVertex3f(DX,0.0f,-DZ);			// Third quad
					glVertex3f(DX,0.0f,DZ);
					glColor4f(red,green,0.0f,0.0f);
					glVertex3f(DX+2.0f,5.0f,DZ+2.0f);
					glVertex3f(DX+2.0f,5.0f,-DZ-2.0f);

					glColor4f(red,green,0.0f,1.0f);
					glVertex3f(-DX,0.0f,DZ);			// Fourth quad
					glVertex3f(DX,0.0f,DZ);
					glColor4f(red,green,0.0f,0.0f);
					glVertex3f(DX+2.0f,5.0f,DZ+2.0f);
					glVertex3f(-DX-2.0f,5.0f,DZ+2.0f);
					glEnd();
					glDisable(GL_BLEND);
					glEnable(GL_LIGHTING);
					glEnable(GL_CULL_FACE);
				}
                glEnable(GL_FOG);
            }


			if (selected && TA3D_SHIFT_PRESSED)
			{
                glDisable(GL_FOG);
                cam.setView();
				bool builders = false;
				for (unsigned int e = 0; e < units.index_list_size; ++e)
				{
					int i = units.idx_list[e];
					if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && units.unit[i].sel)
					{
						builders |= unit_manager.unit_type[units.unit[i].type_id]->Builder;
						units.unit[i].show_orders();					// Dessine les ordres reçus par l'unité / Draw given orders
					}
				}

				if (builders)
				{
					for (unsigned int e = 0; e < units.index_list_size; ++e)
					{
						int i = units.idx_list[e];
						if ((units.unit[i].flags & 1) && units.unit[i].owner_id==players.local_human_id && !units.unit[i].sel
							&& unit_manager.unit_type[units.unit[i].type_id]->Builder && unit_manager.unit_type[units.unit[i].type_id]->BMcode)
						{
							units.unit[i].show_orders(true);					// Dessine les ordres reçus par l'unité / Draw given orders
						}
					}
				}
                glEnable(GL_FOG);
            }

			cam.setView(lp_CONFIG->shadow_quality < 2);
            if (lp_CONFIG->shadow_quality >= 2)
                glFogi (GL_FOG_COORD_SRC, GL_FOG_COORD);
            // Dessine les unités non encore dessinées / Draw units which have not been drawn
			units.draw(map.get(), false, false, true, lp_CONFIG->height_line);
            glFogi (GL_FOG_COORD_SRC, GL_FRAGMENT_DEPTH);

			// Dessine les objets produits par les armes n'ayant pas été dessinés / Draw weapons which have not been drawn
			weapons.draw(map.get(), false);

			if (lp_CONFIG->shadow_quality > 0 && cam.rpos.y <= gfx->low_def_limit)
			{
				switch (lp_CONFIG->shadow_quality)
				{
					case 1:                     // Stencil Shadowing (shadow volumes)
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
							units.draw_shadow(Dir, map.get());
						}
						else
						{
							pSun.Dir.x = -1.0f;
							pSun.Dir.y = 1.0f;
							pSun.Dir.z = 1.0f;
							pSun.Dir.unit();
							units.draw_shadow(-pSun.Dir, map.get());
						}
						break;
					case 2:                     // Shadow mapping
						break;
				}
			}

			particle_engine.draw(&cam,map->map_w,map->map_h,map->bloc_w,map->bloc_h,map->view);	// Dessine les particules

			if (!map->water)
				fx_manager.draw(cam, map.get(), map->sealvl, true);		// Effets spéciaux en surface
			fx_manager.draw(cam, map.get(), map->sealvl);		// Effets spéciaux en surface

			if (key[KEY_ESC] && !pArea.get_state("esc_menu")) // Enter pause mode if we have to show the menu
			{
				if (!network_manager.isConnected())             // In single player mode we want to pause the game when opening the menu
				{
					lp_CONFIG->pause = true;
					pArea.msg( "esc_menu.b_pause.hide" );
					pArea.msg( "esc_menu.b_resume.show" );
				}
				pArea.msg( "esc_menu.show" );
			}

			if (key_down_event(KEY_PAUSE))      // Toggle pause mode when pressing pause
				lp_CONFIG->pause ^= true;

			if (pArea.get_state("esc_menu.b_return"))
			{
				pArea.set_state("esc_menu.b_return", false);
				if (!network_manager.isConnected())             // In single player mode we want to resume the game when closing the menu
				{
					lp_CONFIG->pause = false;
					pArea.msg( "esc_menu.b_pause.show" );
					pArea.msg( "esc_menu.b_resume.hide" );
				}
			}

			if (pArea.get_state("esc_menu.b_exit"))
			{
				pResult = brUnknown;
				done = true;
			}

			if (pArea.get_state("esc_menu.b_save")) 	// Fill the file list
			{
				pArea.set_state("esc_menu.b_save", false);
				Gui::GUIOBJ::Ptr obj_file_list = pArea.get_object("save_menu.l_file");
				if (obj_file_list)
				{
					String::List file_list;
					if (network_manager.isConnected())
						Paths::Glob(file_list, TA3D::Paths::Savegames + "multiplayer" + Paths::Separator + "*.sav");
					else
						Paths::Glob(file_list, TA3D::Paths::Savegames + "*.sav");
					file_list.sort();
					obj_file_list->Text.clear();
					obj_file_list->Text.reserve(file_list.size());
					for (String::List::const_iterator i = file_list.begin(); i != file_list.end(); ++i)
						obj_file_list->Text.push_back(Paths::ExtractFileName(*i));
				}
			}

			if (pArea.get_state("save_menu.l_file")) // Click on the list
			{
				Gui::GUIOBJ::Ptr obj = pArea.get_object("save_menu.l_file");
				if (obj && obj->Pos < obj->Text.size())
					pArea.caption("save_menu.t_name", obj->Text[obj->Pos]);
			}

			if (pArea.get_state("save_menu.b_save")) // Save the game
			{
				pArea.set_state("save_menu.b_save", false);
				String filename = pArea.caption("save_menu.t_name");
				if (!filename.empty())
				{
					if (network_manager.isServer())          // Ask all clients to save the game too, otherwise they won't be able to load it
					{
						String tmp(filename);
						tmp.replace(' ', char(1));
						network_manager.sendSpecial("SAVE " + tmp);

						// Save multiplayer games in their own folder
						filename = Paths::Savegames + "multiplayer" + Paths::Separator + Paths::Files::ReplaceExtension(filename, ".sav");
					}
					else
						filename = Paths::Savegames + Paths::Files::ReplaceExtension(filename, ".sav");
					save_game(filename, pGameData); // Save the game
				}
				lp_CONFIG->pause = false;
			}

			if (key[KEY_TILDE] && !pArea.get_state("chat"))
			{
				if (!tilde)
					console.toggleShow();
				tilde = true;
			}
			else
				tilde = false;

			gfx->ReInitAllTex(true);
			gfx->set_2D_mode();		// Affiche console, infos,...
			draw2DObjects();

			int signal = 0;
			if (!pNetworkEnabled || pNetworkIsServer)
				signal = game_script.check();
			else
				game_script.check(); // In client mode we only want to display text, pictures, ... everything drawn by the script on the server

			if (pNetworkEnabled || signal == 0)
				signal = g_ta3d_network->get_signal();

			switch(signal)
			{
				case 0:				// Rien de spécial
					break;
				case -1:			// Fin du script
					if (!pNetworkEnabled || pNetworkIsServer)
						game_script.kill();
					break;
				case -2:			// Pause
					break;
				case -3:			// Attente d'un évènement
					break;
				case 1:				// Fin de partie (match nul)
					done = true;
					pResult = brPat;
					break;
				case 2:				// Fin de partie (victoire)
					done = true;
					pResult = brVictory;
					break;
				case 3:				// Fin de partie (défaite)
					done = !pNetworkIsServer;			// Server can't leave, otherwise game stops
					pResult = brDefeat;
					break;
				case 4:				// Caméra en mode normal
					if (freecam)
					{
						freecam = false;
						r2 = 0.0f;
					}
					break;
				case 5:				// Caméra libre
					if (!freecam)
						freecam = true;
					break;
			}

			if (cur_sel_index >= 0 && cur_sel_index < units.max_unit && !(units.unit[cur_sel_index].flags & 1))
			{
				cur_sel = -1;
				cur_sel_index = -1;
				current_order = SIGNAL_ORDER_NONE;
			}

			int n = cur_sel;
			if (n == -1)
				n = -2;
			if (n >= 0 && units.unit[cur_sel_index].port[BUILD_PERCENT_LEFT] > 0.0f) // Unité non terminée
				n = -1;
			int sel = -1;

			// If the game is paused, render the pause image
			if (lp_CONFIG->paused)
			{
				gfx->set_alpha_blending();
				gfx->drawtexture(pause_tex, 145.0f * SCREEN_W / 640.0f, 190 * SCREEN_H / 480.0f, 495 * SCREEN_W / 640.0f, 290 * SCREEN_H / 480.0f, 0xFFFFFFFF);
				gfx->unset_alpha_blending();
			}

			/*------------------- Draw GUI components -------------------------------------------------------*/

			if (pCurrentGUI != String( ta3dSideData.side_pref[players.side_view]) + "gen")
				unit_manager.unit_build_menu(n, omb2, dt, true);	// Draw GUI background
			else
				unit_manager.unit_build_menu(-1, omb2, dt, true);	// Draw GUI background

			pArea.draw();

			/*------------------- End of GUI drawings -------------------------------------------------------*/

			/*WND *current_wnd =*/ //pArea.get_wnd(pCurrentGUI);
			if (pCurrentGUI != String( ta3dSideData.side_pref[players.side_view]) << "gen")
				sel = unit_manager.unit_build_menu(n,omb2,dt);	// Menu correspondant à l'unité / Unit's menu
			else
				sel = unit_manager.unit_build_menu(-1,omb2,dt);	// Menu correspondant à l'unité / Unit's menu
			if (sel == -2) // Crée des armes / build weapons
			{
				if (mouse_b == 1 && omb2 != 1)
				{
					if (TA3D_SHIFT_PRESSED)
						units.unit[cur_sel_index].planned_weapons+=5.0f;
					else
						units.unit[cur_sel_index].planned_weapons+=1.0f;
				}
				else
				{
					if (mouse_b == 2 && omb2 != 2)
					{
						units.unit[cur_sel_index].planned_weapons -= (TA3D_SHIFT_PRESSED) ? 5.0f : 1.0f;
						if (units.unit[cur_sel_index].planned_weapons < 0.0f)
							units.unit[cur_sel_index].planned_weapons = 0.0f;
					}
				}
				sel = -1;
			}

			bool refresh_gui(false);

			if (!selected && !pCurrentGUI.empty())
			{
				pArea.msg(pCurrentGUICache[cgcHide]);	// Hide it
				pCurrentGUI.clear();
				updateCurrentGUICacheNames();
				old_sel = false;
			}
			if ((old_gui_sel >= 0 && old_gui_sel != n) || (!old_sel && !selected)) // Update GUI
			{
				pArea.msg(pCurrentGUICache[cgcHide]);	// Hide it
				if (!pCurrentGUI.empty())
				{
					pCurrentGUI.clear();
					updateCurrentGUICacheNames();
				}
				old_sel = false;
			}
			if (n >= 0 && n != old_gui_sel)
			{
				pArea.msg(pCurrentGUICache[cgcHide]);	// Hide it
				pCurrentGUI.clear();
				pCurrentGUI << unit_manager.unit_type[n]->Unitname << "1";
				if (pArea.get_wnd(pCurrentGUI) == NULL)
				{
					pCurrentGUI.clear();
					if (unit_manager.unit_type[ n ]->nb_unit > 0)				// The default build page
						pCurrentGUI << ta3dSideData.side_pref[players.side_view] << "dl";
					else
						pCurrentGUI << ta3dSideData.side_pref[players.side_view] << "gen";
				}
				updateCurrentGUICacheNames();
				pArea.msg( pCurrentGUICache[cgcShow] );	// Show it
				refresh_gui = true;
			}
			if (n < 0 && ( selected && !old_sel))
			{
				pArea.msg( pCurrentGUICache[cgcHide] );	// Hide it
				old_sel = true;
				pCurrentGUI.clear();
				pCurrentGUI << ta3dSideData.side_pref[players.side_view] << "gen";
				updateCurrentGUICacheNames();
				pArea.msg( pCurrentGUICache[cgcShow] );	// Show it
				refresh_gui = true;
			}
			old_gui_sel = n;

			if (refresh_gui)
			{
				/*------------------- GUI update ----------------------------------------------------------------*/

				bool onoffable = false;
				bool canstop = false;
				bool canpatrol = false;
				bool canmove = false;
				bool canguard = false;
				bool canattack = false;
				bool canreclam = false;
				bool builders = false; // For repair purposes only
				bool canload = false;
				bool cancapture = false;
				bool cancloak  =false;
				bool candgun = false;
				int onoff_state = 0;
				byte sforder = 0;
				byte smorder = 0;

				for (unsigned int e = 0; e < units.index_list_size; ++e)
				{
					units.lock();
					uint32 i = units.idx_list[e];
					units.unlock();
					units.unit[i].lock();
					if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && units.unit[i].sel)
					{
						onoffable  |= unit_manager.unit_type[units.unit[i].type_id]->onoffable;
						canstop    |= unit_manager.unit_type[units.unit[i].type_id]->canstop;
						canmove    |= unit_manager.unit_type[units.unit[i].type_id]->canmove;
						canpatrol  |= unit_manager.unit_type[units.unit[i].type_id]->canpatrol;
						canguard   |= unit_manager.unit_type[units.unit[i].type_id]->canguard;
						canattack  |= unit_manager.unit_type[units.unit[i].type_id]->canattack;
						canreclam  |= unit_manager.unit_type[units.unit[i].type_id]->CanReclamate;
						builders   |= unit_manager.unit_type[units.unit[i].type_id]->Builder;
						canload    |= unit_manager.unit_type[units.unit[i].type_id]->canload;
						cancapture |= unit_manager.unit_type[units.unit[i].type_id]->CanCapture;
						cancloak   |= unit_manager.unit_type[units.unit[i].type_id]->CloakCost > 0;
						candgun    |= unit_manager.unit_type[units.unit[i].type_id]->candgun;

						if (unit_manager.unit_type[units.unit[i].type_id]->canattack)
							sforder |= units.unit[i].port[ STANDINGFIREORDERS ];
						if (unit_manager.unit_type[units.unit[i].type_id]->canmove)
							smorder |= units.unit[i].port[ STANDINGMOVEORDERS ];
						if (unit_manager.unit_type[units.unit[i].type_id]->onoffable)
							onoff_state |= units.unit[ i ].port[ ACTIVATION ] ? 2 : 1;
					}
					units.unit[i].unlock();
				}

				if (onoff_state == 0)
					onoff_state = 3;

				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "STOP", canstop);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "MOVE", canmove);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "PATROL", canpatrol);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "DEFEND", canguard);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "ATTACK", canattack);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "RECLAIM", canreclam);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "LOAD", canload);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "UNLOAD", canload);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "REPAIR", builders);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "ONOFF", onoffable);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "MOVEORD", canmove);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "FIREORD", canattack);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "CAPTURE", cancapture);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "CLOAK", cancloak);
				pArea.set_enable_flag(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "BLAST", candgun);

				pArea.set_enable_flag(pCurrentGUI + ".ARMSTOP", canstop);			// Alternate version to support mods
				pArea.set_enable_flag(pCurrentGUI + ".ARMMOVE", canmove);
				pArea.set_enable_flag(pCurrentGUI + ".ARMPATROL", canpatrol);
				pArea.set_enable_flag(pCurrentGUI + ".ARMDEFEND", canguard);
				pArea.set_enable_flag(pCurrentGUI + ".ARMATTACK", canattack);
				pArea.set_enable_flag(pCurrentGUI + ".ARMRECLAIM", canreclam);
				pArea.set_enable_flag(pCurrentGUI + ".ARMLOAD", canload);
				pArea.set_enable_flag(pCurrentGUI + ".ARMUNLOAD", canload);
				pArea.set_enable_flag(pCurrentGUI + ".ARMREPAIR", builders);
				pArea.set_enable_flag(pCurrentGUI + ".ARMONOFF", onoffable);
				pArea.set_enable_flag(pCurrentGUI + ".ARMMOVEORD", canmove);
				pArea.set_enable_flag(pCurrentGUI + ".ARMFIREORD", canattack);
				pArea.set_enable_flag(pCurrentGUI + ".ARMCAPTURE", cancapture);
				pArea.set_enable_flag(pCurrentGUI + ".ARMCLOAK", cancloak);
				pArea.set_enable_flag(pCurrentGUI + ".ARMBLAST", candgun);

				Gui::GUIOBJ::Ptr onoff_gui = pArea.get_object(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "ONOFF");
				if (!onoff_gui)
					onoff_gui = pArea.get_object(pCurrentGUI + ".ARMONOFF");

				if (onoff_gui)
					onoff_gui->current_state = onoff_state - 1;

				Gui::GUIOBJ::Ptr sorder_gui = pArea.get_object(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "FIREORD");
				if (!sorder_gui)
					sorder_gui = pArea.get_object(pCurrentGUI + ".ARMFIREORD");

				if (sorder_gui)
					sorder_gui->current_state = sforder;

				sorder_gui = pArea.get_object( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "MOVEORD");
				if (!sorder_gui)
					sorder_gui = pArea.get_object( pCurrentGUI + ".ARMMOVEORD");

				if (sorder_gui)
					sorder_gui->current_state = smorder;

				if (canload)
				{
					pArea.msg( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "LOAD.show" );	// Show it
					pArea.msg( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "BLAST.hide" );	// Hide it
					pArea.msg( pCurrentGUI + ".ARMLOAD.show" );	// Show it
					pArea.msg( pCurrentGUI + ".ARMBLAST.hide" );	// Hide it
				}
				else
				{
					pArea.msg( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view ] + "LOAD.hide" );	// Hide it
					pArea.msg( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view ] + "BLAST.show" );	// Show it
					pArea.msg( pCurrentGUI + ".ARMLOAD.hide" );	// Hide it
					pArea.msg( pCurrentGUI + ".ARMBLAST.show" );	// Show it
				}

				if (pCurrentGUI != String(ta3dSideData.side_pref[players.side_view]) + "gen")
				{
					String genGUI;
					genGUI << ta3dSideData.side_pref[players.side_view] << "gen";
					String genGUIwDot(genGUI);
					genGUIwDot += ".";

					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "STOP", canstop);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "MOVE", canmove);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "PATROL", canpatrol);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "DEFEND", canguard);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "ATTACK", canattack);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "RECLAIM", canreclam);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "LOAD", canload);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "UNLOAD", canload);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "REPAIR", builders);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "ONOFF", onoffable);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "MOVEORD", canmove);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "FIREORD", canattack);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "CAPTURE", cancapture);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "CLOAK", cancloak);
					pArea.set_enable_flag( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "BLAST", candgun);

					pArea.set_enable_flag( genGUI + ".ARMSTOP", canstop);
					pArea.set_enable_flag( genGUI + ".ARMMOVE", canmove);
					pArea.set_enable_flag( genGUI + ".ARMPATROL", canpatrol);
					pArea.set_enable_flag( genGUI + ".ARMDEFEND", canguard);
					pArea.set_enable_flag( genGUI + ".ARMATTACK", canattack);
					pArea.set_enable_flag( genGUI + ".ARMRECLAIM", canreclam);
					pArea.set_enable_flag( genGUI + ".ARMLOAD", canload);
					pArea.set_enable_flag( genGUI + ".ARMUNLOAD", canload);
					pArea.set_enable_flag( genGUI + ".ARMREPAIR", builders);
					pArea.set_enable_flag( genGUI + ".ARMONOFF", onoffable);
					pArea.set_enable_flag( genGUI + ".ARMMOVEORD", canmove);
					pArea.set_enable_flag( genGUI + ".ARMFIREORD", canattack);
					pArea.set_enable_flag( genGUI + ".ARMCAPTURE", cancapture);
					pArea.set_enable_flag( genGUI + ".ARMCLOAK", cancloak);
					pArea.set_enable_flag( genGUI + ".ARMBLAST", candgun);

					onoff_gui = pArea.get_object(genGUIwDot + ta3dSideData.side_pref[players.side_view] + "ONOFF");
					if (onoff_gui == NULL)
						onoff_gui = pArea.get_object(genGUI + ".ARMONOFF");

					if (onoff_gui)
						onoff_gui->current_state = onoff_state - 1;

					sorder_gui = pArea.get_object( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "FIREORD");
					if (sorder_gui == NULL)
						sorder_gui = pArea.get_object( genGUI + ".ARMFIREORD");

					if (sorder_gui)
						sorder_gui->current_state = sforder;

					sorder_gui = pArea.get_object( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "MOVEORD");
					if (sorder_gui == NULL)
						sorder_gui = pArea.get_object( genGUI + ".ARMMOVEORD");

					if (sorder_gui)
						sorder_gui->current_state = smorder;

					if (canload)
					{
						pArea.msg( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "LOAD.show" );	// Show it
						pArea.msg( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "BLAST.hide" );	// Hide it
						pArea.msg( genGUI + ".ARMLOAD.show" );	// Show it
						pArea.msg( genGUI + ".ARMBLAST.hide" );	// Hide it
					}
					else
					{
						pArea.msg( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "LOAD.hide" );	// Hide it
						pArea.msg( genGUIwDot + ta3dSideData.side_pref[players.side_view] + "BLAST.show" );	// Show it
						pArea.msg( genGUI + ".ARMLOAD.hide" );	// Hide it
						pArea.msg( genGUI + ".ARMBLAST.show" );	// Show it
					}
				}

				/*------------------- End of GUI update ---------------------------------------------------------*/
			}

			if (!pCurrentGUI.empty() && pCurrentGUI != String(ta3dSideData.side_pref[players.side_view]) + "gen") // Show information about units
				units.complete_menu(cur_sel_index, sel != -1 || units.last_on <= -2, false);
			else
				units.complete_menu(cur_sel_index, sel != -1 || units.last_on <= -2, true);

			int signal_order = 0;
			features.display_info( -units.last_on - 2);
			players.show_resources();

			/*------------------- GUI reacting code ---------------------------------------------------------*/

			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "ORDERS")
				|| pArea.get_state(pCurrentGUI + ".ARMORDERS")) // Go to the order menu
			{
				pArea.set_state(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "ORDERS", false);
				pArea.set_state(pCurrentGUI + ".ARMORDERS", false);				// Because of mod support
				sound_manager->playTDFSound( "ORDERSBUTTON", "sound" , NULL);
				pArea.msg( pCurrentGUICache[cgcHide] );	// Hide it
				pCurrentGUI.clear();
				pCurrentGUI << ta3dSideData.side_pref[players.side_view] << "gen";
				updateCurrentGUICacheNames();
				pArea.msg( pCurrentGUICache[cgcShow] );	// Show it
			}

			if (( pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "BUILD") ||
				  pArea.get_state( pCurrentGUI + ".ARMBUILD")) && old_gui_sel >= 0) // Back to the build menu
			{
				pArea.set_state(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "BUILD", false);
				pArea.set_state(pCurrentGUI + ".ARMBUILD", false);
				sound_manager->playTDFSound( "BUILDBUTTON", "sound" , NULL);
				pArea.msg( pCurrentGUICache[cgcHide] );	// Hide it
				pCurrentGUI.clear();
				pCurrentGUI << unit_manager.unit_type[old_gui_sel]->Unitname << "1";
				if (pArea.get_wnd(pCurrentGUI) == NULL)
				{
					pCurrentGUI.clear();
					if (unit_manager.unit_type[old_gui_sel]->nb_unit > 0) // The default build page
						pCurrentGUI << ta3dSideData.side_pref[players.side_view] << "dl";
					else
						pCurrentGUI << ta3dSideData.side_pref[players.side_view] << "gen";
				}
				updateCurrentGUICacheNames();
				pArea.msg( pCurrentGUICache[cgcShow] );	// Show it
			}

			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "PREV")
				|| pArea.get_state( pCurrentGUI + ".ARMPREV"))
			{
				sound_manager->playTDFSound( "NEXTBUILDMENU", "sound" , NULL);
				if (unit_manager.unit_type[old_gui_sel]->nb_pages > 0)
					unit_manager.unit_type[old_gui_sel]->page = (unit_manager.unit_type[old_gui_sel]->page + unit_manager.unit_type[old_gui_sel]->nb_pages-1)%unit_manager.unit_type[old_gui_sel]->nb_pages;
			}
			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "NEXT")
				|| pArea.get_state( pCurrentGUI + ".ARMNEXT"))
			{
				sound_manager->playTDFSound( "NEXTBUILDMENU", "sound" , NULL);
				if (unit_manager.unit_type[old_gui_sel]->nb_pages > 0)
					unit_manager.unit_type[old_gui_sel]->page = (unit_manager.unit_type[old_gui_sel]->page+1)%unit_manager.unit_type[old_gui_sel]->nb_pages;
			}

			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "ONOFF")
				|| pArea.get_state( pCurrentGUI + ".ARMONOFF")) // Toggle the on/off value
			{
				signal_order = SIGNAL_ORDER_ONOFF;
				Gui::GUIOBJ::Ptr onoff_obj = pArea.get_object( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "ONOFF");
				if (!onoff_obj)
					onoff_obj = pArea.get_object( pCurrentGUI + ".ARMONOFF");
				if (onoff_obj)
				{
					units.lock();
					for (unsigned int e = 0 ; e < units.index_list_size ; ++e)
					{
						unsigned int i = units.idx_list[e];
						units.unlock();
						units.unit[i].lock();
						if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && units.unit[i].sel && unit_manager.unit_type[units.unit[i].type_id]->onoffable)
						{
							if (onoff_obj->nb_stages > 1)
							{
								onoff_obj->current_state &= 1;
								if (onoff_obj->current_state == 0)
									units.unit[i].deactivate();
								else
									units.unit[i].activate();
							}
							else
							{
								if (units.unit[i].port[ACTIVATION])
									units.unit[i].deactivate();
								else
									units.unit[i].activate();
							}
						}
						units.unit[i].unlock();
						units.lock();
					}
					units.unlock();
				}
			}

			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "CLOAK")
				|| pArea.get_state( pCurrentGUI + ".ARMCLOAK")) // Toggle the cloak value
			{
				Gui::GUIOBJ::Ptr cloak_obj = pArea.get_object( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "CLOAK");
				if (!cloak_obj)
					cloak_obj = pArea.get_object( pCurrentGUI + ".ARMCLOAK");
				if (cloak_obj)
				{
					units.lock();
					for (unsigned int e = 0; e < units.index_list_size; ++e)
					{
						unsigned int i = units.idx_list[e];
						units.unlock();
						units.unit[i].lock();
						if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && units.unit[i].sel && unit_manager.unit_type[units.unit[i].type_id]->CloakCost > 0)
						{
							if (cloak_obj->nb_stages > 1)
							{
								cloak_obj->current_state &= 1;
								units.unit[i].cloaking = cloak_obj->current_state != 0;
							}
							else
								units.unit[i].cloaking ^= true;
						}
						units.unit[i].unlock();
						units.lock();
					}
					units.unlock();
				}
			}

			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "FIREORD")
				|| pArea.get_state( pCurrentGUI + ".ARMFIREORD")) // Toggle the fireorder value
			{
				sound_manager->playTDFSound( "SETFIREORDERS", "sound" , NULL);
				Gui::GUIOBJ::Ptr sorder_obj = pArea.get_object(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "FIREORD");
				if (!sorder_obj)
					sorder_obj = pArea.get_object(pCurrentGUI + ".ARMFIREORD");
				if (sorder_obj)
				{
					sorder_obj->current_state %= 3;
					units.lock();
					for (unsigned int e = 0 ; e < units.index_list_size ; ++e)
					{
						unsigned int i = units.idx_list[e];
						units.unlock();
						units.unit[i].lock();
						if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && units.unit[i].sel)
						{
							units.unit[i].port[STANDINGFIREORDERS] = sorder_obj->current_state;
							if (SFORDER_FIRE_AT_WILL != units.unit[i].port[STANDINGFIREORDERS])
							{
								for (unsigned int f = 0; f < units.unit[i].weapon.size(); ++f)
									units.unit[i].weapon[f].state = WEAPON_FLAG_IDLE;
								if (units.unit[i].mission && units.unit[i].mission->mission == MISSION_ATTACK && !( units.unit[i].mission->flags & MISSION_FLAG_COMMAND_FIRE))
									units.unit[i].next_mission();
							}
						}
						units.unit[i].unlock();
						units.lock();
					}
					units.unlock();
				}
			}

			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "MOVEORD")
				|| pArea.get_state( pCurrentGUI + ".ARMMOVEORD")) // Toggle the moveorder value
			{
				sound_manager->playTDFSound("SETMOVEORDERS", "sound" , NULL);
				Gui::GUIOBJ::Ptr sorder_obj = pArea.get_object(pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "MOVEORD");
				if (!sorder_obj)
					sorder_obj = pArea.get_object(pCurrentGUI + ".ARMMOVEORD");
				if (sorder_obj)
				{
					sorder_obj->current_state %= 3;
					units.lock();
					for (unsigned int e = 0 ; e < units.index_list_size; ++e)
					{
						unsigned int i = units.idx_list[e];
						units.unlock();
						units.unit[i].lock();
						if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && units.unit[i].sel)
							units.unit[i].port[ STANDINGMOVEORDERS ] = sorder_obj->current_state;
						units.unit[i].unlock();
						units.lock();
					}
					units.unlock();
				}
			}

			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "MOVE")
				|| pArea.get_state( pCurrentGUI + ".ARMMOVE"))														signal_order = SIGNAL_ORDER_MOVE;
			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "PATROL")
				|| pArea.get_state( pCurrentGUI + ".ARMPATROL"))														signal_order = SIGNAL_ORDER_PATROL;
			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "STOP")
				|| pArea.get_state( pCurrentGUI + ".ARMSTOP"))														signal_order = SIGNAL_ORDER_STOP;
			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "DEFEND")
				|| pArea.get_state( pCurrentGUI + ".ARMDEFEND"))														signal_order = SIGNAL_ORDER_GUARD;
			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "ATTACK")
				|| pArea.get_state( pCurrentGUI + ".ARMATTACK"))														signal_order = SIGNAL_ORDER_ATTACK;
			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "RECLAIM")
				|| pArea.get_state( pCurrentGUI + ".ARMRECLAIM"))														signal_order = SIGNAL_ORDER_RECLAM;
			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "LOAD")
				|| pArea.get_state( pCurrentGUI + ".ARMLOAD"))														signal_order = SIGNAL_ORDER_LOAD;
			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "UNLOAD")
				|| pArea.get_state( pCurrentGUI + ".ARMUNLOAD"))														signal_order = SIGNAL_ORDER_UNLOAD;
			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "REPAIR")
				|| pArea.get_state( pCurrentGUI + ".ARMREPAIR"))														signal_order = SIGNAL_ORDER_REPAIR;
			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "CAPTURE")
				|| pArea.get_state( pCurrentGUI + ".ARMCAPTURE"))														signal_order = SIGNAL_ORDER_CAPTURE;
			if (pArea.get_state( pCurrentGUICache[cgcDot] + ta3dSideData.side_pref[players.side_view] + "BLAST")
				|| pArea.get_state( pCurrentGUI + ".ARMBLAST"))														signal_order = SIGNAL_ORDER_DGUN;

			/*------------------- End of GUI reacting code --------------------------------------------------*/

			switch(signal_order)
			{
				case SIGNAL_ORDER_CAPTURE:
				case SIGNAL_ORDER_DGUN:
				case SIGNAL_ORDER_MOVE:
				case SIGNAL_ORDER_PATROL:
				case SIGNAL_ORDER_GUARD:
				case SIGNAL_ORDER_ATTACK:
				case SIGNAL_ORDER_RECLAM:
				case SIGNAL_ORDER_LOAD:
				case SIGNAL_ORDER_UNLOAD:
				case SIGNAL_ORDER_REPAIR:
					sound_manager->playTDFSound( "IMMEDIATEORDERS", "sound" , NULL);
					current_order = signal_order;
					build = -1;
					break;
				case SIGNAL_ORDER_STOP:
					sound_manager->playTDFSound( "IMMEDIATEORDERS", "sound" , NULL);
					units.lock();
					for (unsigned int e = 0; e < units.index_list_size; ++e)
					{
						unsigned int i = units.idx_list[e];
						units.unlock();
						units.unit[i].lock();
						if ((units.unit[i].flags & 1) && units.unit[i].owner_id == players.local_human_id && units.unit[i].sel && unit_manager.unit_type[units.unit[i].type_id]->canstop)
							units.unit[i].set_mission(MISSION_STOP);
						units.unit[i].unlock();
						units.lock();
					}
					units.unlock();
				case SIGNAL_ORDER_ONOFF:
					current_order=SIGNAL_ORDER_NONE;
					break;
			}
			if (sel>=0 || (mouse_b&2))
				current_order=SIGNAL_ORDER_NONE;

			if (sel >= 0 && mouse_b == 2 && omb2 != 2)
			{
				units.unit[cur_sel_index].lock();
				Mission*  cur = units.unit[cur_sel_index].mission;
				Mission** old = &(units.unit[cur_sel_index].mission);
				int nb(1);
				if (TA3D_SHIFT_PRESSED)
					nb = 5;
				if (cur)
				{
					old = &(cur->next);
					cur = cur->next;
				}
				while (cur)
				{
					if ((cur->mission==MISSION_BUILD && cur->data == sel) || cur->mission == MISSION_STOP) // Efface un ordre
					{
						if (cur->mission==MISSION_BUILD)
							--nb;
						*old = cur->next;
						delete cur;
						cur = *old;
						if (nb == 0)
							break;
						continue;
					}
					old = &(cur->next);
					cur = cur->next;
				}
				cur = units.unit[cur_sel_index].mission;
				if (nb > 0 && cur != NULL && cur->mission == MISSION_BUILD_2 && cur->data==sel)
				{
					sint32 prev = -1;
					for (int i = units.nb_unit - 1; i >= 0; --i)
					{
						if (units.idx_list[i] == ((Unit*)(units.unit[cur_sel_index].mission->p))->idx)
						{
							prev = i;
							break;
						}
					}
					if (prev >= 0)
					{
						int type = units.unit[((Unit*)(units.unit[cur_sel_index].mission->p))->idx].type_id;
						float metal_to_give_back = units.unit[((Unit*)(units.unit[cur_sel_index].mission->p))->idx].build_percent_left * unit_manager.unit_type[type]->BuildCostMetal;
						int p_id = units.unit[cur_sel_index].owner_id;
						units.unit[((Unit*)(units.unit[cur_sel_index].mission->p))->idx].clear_from_map();
						units.unit[((Unit*)(units.unit[cur_sel_index].mission->p))->idx].flags = 0;               // Don't count it as a loss
						units.kill(((Unit*)(units.unit[cur_sel_index].mission->p))->idx, map.get(), prev);
						players.metal[p_id] += metal_to_give_back;          // Give metal back
						players.c_metal[p_id] += metal_to_give_back;
					}
					units.unit[cur_sel_index].next_mission();
				}
				units.unit[cur_sel_index].unlock();
			}

			if (sel >= 0 && mouse_b == 1 && omb2 != 1)
			{
				build = sel;
				sound_manager->playTDFSound( "ADDBUILD", "sound" , NULL);
				if (!unit_manager.unit_type[cur_sel]->BMcode) // S'il s'agit d'un bâtiment
				{
					if (TA3D_SHIFT_PRESSED)
						for (short int i = 0; i < 5; ++i)
							units.give_order_build(players.local_human_id, build, units.unit[cur_sel_index].Pos, false);
					else
						units.give_order_build(players.local_human_id, build, units.unit[cur_sel_index].Pos, false);
					build = -1;
				}
			}

			glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glEnable(GL_TEXTURE_2D);
			gfx->drawtexture(freecam ? freecam_on : freecam_off,32,SCREEN_H-64,95,SCREEN_H);
			glDisable(GL_BLEND);

			if (mouse_x >= 32 && mouse_x <= 95 && mouse_y >= SCREEN_H - 64 && omb2 == 0)
			{
				if (mouse_b==1)
				{
					freecam ^= true;
					if (!freecam)
						r2 = 0.0f;
				}
				else
				{
					if (mouse_b == 2 && !freecam) // Reset default view
					{
						camera_zscroll = -0.00001f;
						r1 = -lp_CONFIG->camera_def_angle - 0.00001f;
					}
				}
			}

			omb2=mouse_b;

			int last_on = units.last_on;

			map->draw_mini(0,0,128,128,&cam,1<<players.local_human_id);	// Mini-carte
			units.draw_mini(map->map_w,map->map_h,map->mini_w,map->mini_h,map->map_data);
			weapons.draw_mini(map->map_w,map->map_h,map->mini_w,map->mini_h);

			if (!freecam && mouse_b == 4) // Moving the cam around
			{
				gfx->set_alpha_blending();
				gfx->set_color( 0xFFFFFFFF);
				gfx->drawtexture( circle_texture, gfx->width * 0.45f, gfx->height * 0.45f, gfx->width * 0.55f, gfx->height * 0.55f);
				gfx->drawtexture( arrow_texture, gfx->width * 0.4f, 0.0f, gfx->width * 0.6f, gfx->height * 0.05f);
				gfx->drawtexture( arrow_texture, gfx->width * 0.4f, gfx->height, gfx->width * 0.6f, gfx->height * 0.95f);
				gfx->drawtexture_flip( arrow_texture, 0.0f, gfx->height * 0.4f, gfx->width * 0.05f, gfx->height * 0.6f);
				gfx->drawtexture_flip( arrow_texture, gfx->width, gfx->height * 0.4f, gfx->width * 0.95f, gfx->height * 0.6f);
				gfx->unset_alpha_blending();
			}

			if (view_dbg)
			{
				glDisable(GL_TEXTURE_2D);
				glColor3f(1.0f,1.0f,1.0f);
				glBegin(GL_POINTS);
				float rw = 128.0f * map->mini_w / 252 / map->bloc_w;
				float rh = 128.0f * map->mini_h / 252 / map->bloc_h;
				float dw = 64.0f - 0.5f * map->bloc_w * rw;
				float dh = 64.0f - 0.5f * map->bloc_h * rh;
				for (int y = 0; y < map->bloc_h; ++y)
				{
					for (int x = 0; x < map->bloc_w; ++x)
					{
						if (map->view[y][x])
							glVertex2f(x*rw+dw,y*rh+dh);
					}
				}
				glEnd();
				glEnable(GL_TEXTURE_2D);
			}

			glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_COLOR);
			glEnable(GL_BLEND);

			if (show_model && cur_sel>=0 && unit_manager.unit_type[cur_sel]->model)
				unit_manager.unit_type[cur_sel]->model->print_struct(32.0f,128.0f,gfx->normal_font);

			if (internal_name && last_on >= 0)
			{
				units.unit[ last_on ].lock();
				if (units.unit[ last_on ].type_id >= 0 && !unit_manager.unit_type[ units.unit[ last_on ].type_id ]->Unitname.empty())
					gfx->print(gfx->normal_font,128.0f,32.0f,0.0f,0xFFFFFFFF,"internal name " + unit_manager.unit_type[ units.unit[ last_on ].type_id ]->Unitname);
				units.unit[ last_on ].unlock();
			}
			else
			{
				if (internal_name && cur_sel >= 0 && !unit_manager.unit_type[cur_sel]->Unitname.empty())
					gfx->print(gfx->normal_font,128.0f,32.0f,0.0f,0xFFFFFFFF,"internal name " + unit_manager.unit_type[cur_sel]->Unitname);
			}

			if (internal_idx && last_on >= 0)
				gfx->print(gfx->normal_font,128.0f,32.0f,0.0f,0xFFFFFFFF, String::Format("idx = %d", last_on));
			else
			{
				if (internal_idx && cur_sel_index >= 0)
					gfx->print(gfx->normal_font,128.0f,32.0f,0.0f,0xFFFFFFFF, String::Format("idx = %d", cur_sel_index));
			}

			if (unit_info>0.0f && unit_info_id>=0)
				unit_manager.unit_type[unit_info_id]->show_info(unit_info,gfx->TA_font);

			if (last_on != -1 && show_mission_info) // Sur les unités sélectionnées
			{
				static const char *unit_info[] =
				{
					"MISSION_STANDBY", "MISSION_VTOL_STANDBY","MISSION_GUARD_NOMOVE",
					"MISSION_MOVE","MISSION_BUILD","MISSION_BUILD_2","MISSION_STOP",
					"MISSION_REPAIR","MISSION_ATTACK",
					"MISSION_PATROL","MISSION_GUARD","MISSION_RECLAIM","MISSION_LOAD",
					"MISSION_UNLOAD","MISSION_STANDBY_MINE"
				};
				float y(32.0f);
				for (int i = 0; i < units.max_unit; ++i)
				{
					units.unit[i].lock();
					if ((units.unit[i].flags & 1) && last_on == i)
					{
						if (units.unit[i].mission != NULL && units.unit[i].mission->mission<=0x0E)
						{
							gfx->print(gfx->normal_font,128.0f,y,0.0f,0xFFFFFFFF, String::Format("MISSION: %s",unit_info[units.unit[i].mission->mission]));
							String flags;
							if (units.unit[i].mission->flags & MISSION_FLAG_CAN_ATTACK)	flags += "CAN_ATTACK; ";
							if (units.unit[i].mission->flags & MISSION_FLAG_SEARCH_PATH)	flags += "SEARCH_PATH; ";
							if (units.unit[i].mission->flags & MISSION_FLAG_TARGET_WEAPON)	flags += "TARGET_WEAPON; ";
							if (units.unit[i].mission->flags & MISSION_FLAG_COMMAND_FIRE)	flags += "COMMAND_FIRE; ";
							if (units.unit[i].mission->flags & MISSION_FLAG_MOVE)	flags += "MOVE; ";
							if (units.unit[i].mission->flags & MISSION_FLAG_REFRESH_PATH)	flags += "REFRESH_PATH; ";
							y += gfx->normal_font->height();
							gfx->print(gfx->normal_font,128.0f,y,0.0f,0xFFFFFFFF, String::Format("FLAGS: %s", flags.c_str()));
						}
						else
							gfx->print(gfx->normal_font,128.0f,y,0.0f,0xFFFFFFFF, String::Format("MISSION: NONE"));
						y += gfx->normal_font->height();
					}
					units.unit[i].unlock();
				}
			}

			glDisable(GL_BLEND);

			if (show_timefactor > 0.0f)
			{
				String value = String::Format("x %f", lp_CONFIG->timefactor);
				if (value.find('.') != String::npos)
					value.truncate(value.find('.') + 2);
				if (show_timefactor > 0.5f)
				{
					gfx->print( gfx->TA_font, (gfx->width - (int)gfx->TA_font->length(value) + 2)>>1, SCREEN_H-79, 0.0f, makeacol32(0,0,0,0xFF), value);
					gfx->print( gfx->TA_font, (gfx->width - (int)gfx->TA_font->length(value))>>1, SCREEN_H-80, 0.0f, 0xFFFFFFFF, value);
				}
				else
				{
					uint32 c = (uint32)(511.0f * show_timefactor) * 0x01010101;
					gfx->print( gfx->TA_font, (gfx->width - (int)gfx->TA_font->length(value) + 2)>>1, SCREEN_H-79, 0.0f, c & makeacol32(0,0,0,0xFF), value);
					gfx->print( gfx->TA_font, (gfx->width - (int)gfx->TA_font->length(value))>>1, SCREEN_H-80, 0.0f, c, value);
				}
				show_timefactor -= dt;
			}

			g_ta3d_network->draw(); // Draw network related stuffs (ie: chat messages, ...)

			String cmd;
			// Draw the console
			if (!shoot || video_shoot)
				cmd = console.draw(Gui::gui_font, dt);

			// Informations about FPS
			if (lp_CONFIG->showfps)
			{
				++fps.countSinceLastTime;
				if (msec_timer - fps.lastTime >= 1000 /* 1s */)
				{
					//fps = (int)(fps.countSinceLastTime / ((msec_timer - fps.lastTime) * Conv));
					fps.average = lrint(fps.countSinceLastTime / ((msec_timer - fps.lastTime) * Conv));
					fps.countSinceLastTime = 0;
					fps.lastTime = msec_timer;
					fps.toStr.clear();
					fps.toStr << "fps: " << fps.average;
				}
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
				gfx->print(gfx->TA_font, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF, fps.toStr);
				glDisable(GL_BLEND);
			}


			if (mouse_b!=4 || TA3D_CTRL_PRESSED)
				draw_cursor();

			if (shoot)
			{
				SDL_Surface *shoot_bmp = gfx->create_surface_ex(24,SCREEN_W,SCREEN_H);
				glReadPixels(0, 0, SCREEN_W, SCREEN_H, GL_BGR, GL_UNSIGNED_BYTE, shoot_bmp->pixels);
				String nom = String::Format("ta3d-shoot%.6d.tga", nb_shoot);
				nb_shoot = (nb_shoot+1)%1000000;
				save_bitmap(TA3D::Paths::Screenshots + nom, shoot_bmp);
				SDL_FreeSurface(shoot_bmp);
				shoot = false;
			}

			gfx->unset_2D_mode();

			gfx->flip();

            parseCommands(cmd);

			if (cheat_metal)
				players.metal[players.local_human_id] = players.c_metal[players.local_human_id]=players.metal_s[players.local_human_id];					// cheat codes
			if (cheat_energy)
				players.energy[players.local_human_id] = players.c_energy[players.local_human_id]=players.energy_s[players.local_human_id];				// cheat codes
			if (key[KEY_F12])
				shoot = true;

			/*------------ Code de gestion du déroulement de la partie -----------------------------------*/

			if ((!pNetworkEnabled || pNetworkIsServer) && signal == -1)	// Si le script est terminé, on reprend les règles standard
			{
				bool win = true;
				for (unsigned int i = 0; i != players.count(); ++i)
				{
					if (!players.annihilated[i] && i != players.local_human_id)
					{
						win = false;
						break;
					}
				}
				if (win)
				{
					done = true;
					pResult = brVictory;
				}
				if (players.annihilated[players.local_human_id])
				{
					done = true;
					pResult = brDefeat;
				}
			}

		} while (!done);


		// Tell to other players the game is over
		if (network_manager.isConnected())
			network_manager.sendSpecial("GONE");
		// Over
		LOG_INFO(LOG_PREFIX_BATTLE << "*** The game is over Commander ***");

		reset_mouse();

		players.destroyThread();				// Shut down the Players thread
		players.stop_threads();
		weapons.destroyThread();				// Shut down the Weapon Engine
		units.destroyThread();					// Shut down the Unit Engine
		particle_engine.destroyThread();		// Shut down the Particle Engine

		sky_obj.destroy();

		LuaProgram::inGame->destroyThread();

		LuaEnv::destroy();

		Camera::inGame = NULL;

		if (g_useProgram && g_useFBO && map->water)
		{
			glDeleteFramebuffersEXT(1, &water_FBO);
			water_pass1.destroy();
			water_pass1_low.destroy();
			water_pass2.destroy();
			water_shader.destroy();
			water_shader_reflec.destroy();
			water_simulator_shader.destroy();
			water_simulator_shader2.destroy();
			water_simulator_shader3.destroy();
			water_simulator_shader4.destroy();
			water_simulator_reflec.destroy();
		}

		gfx->destroy_texture(freecam_on);
		gfx->destroy_texture(freecam_off);
		gfx->destroy_texture(sky);
		gfx->destroy_texture(water);
		gfx->destroy_texture(glow);
		gfx->destroy_texture(arrow_texture);
		gfx->destroy_texture(circle_texture);
		gfx->destroy_texture(water_sim);
		gfx->destroy_texture(water_sim2);
		gfx->destroy_texture(water_color);
		gfx->destroy_texture(first_pass);
		gfx->destroy_texture(second_pass);
		gfx->destroy_texture(reflectex);
		gfx->destroy_texture(transtex);
		gfx->destroy_texture(height_tex);
		gfx->destroy_texture(pause_tex);

		LOG_INFO("Total Models: " << model_manager.nb_models);
		LOG_INFO("Total Units: " << unit_manager.nb_unit);
		LOG_INFO("Total Textures: " << texture_manager.nbtex);

		switch(pResult)
		{
			case brPat:
			case brDefeat:
			case brUnknown: break;
			case brVictory:
							if (pGameData->campaign && !map->ota_data.glamour.empty() && HPIManager->Exists( "bitmaps\\glamour\\" + map->ota_data.glamour + ".pcx"))
							{
								// Disable TA palette since those images have their own palette :)
								disable_TA_palette();
								GLuint	glamour_tex = gfx->load_texture("bitmaps\\glamour\\" + map->ota_data.glamour + ".pcx");
								enable_TA_palette();
								gfx->set_2D_mode();
								gfx->drawtexture( glamour_tex, 0, 0, SCREEN_W, SCREEN_H);
								gfx->destroy_texture( glamour_tex);
								gfx->unset_2D_mode();
								gfx->flip();

								while( !keypressed() && mouse_b == 0) {	rest(1);	poll_inputs();	}
								while( mouse_b)	poll_inputs();
								while( keypressed())	readkey();
							}
							break;
		}

		// Statistics
		if (!pGameData->campaign)
			Menus::Statistics::Execute();

		return pResult;
	}



	void Battle::draw2DObjects()
	{
		if (pMouseSelecting)
			draw2DMouseUserSelection();
	}


	void Battle::draw2DMouseUserSelection()
	{
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		gfx->rect(pMouseRectSelection.x1 + 1, pMouseRectSelection.y1 + 1,
				  pMouseRectSelection.x2 + 1, pMouseRectSelection.y2 + 1);

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		gfx->rect(pMouseRectSelection.x1, pMouseRectSelection.y1,
				  pMouseRectSelection.x2, pMouseRectSelection.y2);
	}



} // namespace TA3D


