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
#include "weapons.ingame.h"
#include "../../EngineClass.h"
#include "../../misc/camera.h"
#include "../../gfx/gfx.h"



namespace TA3D
{

	INGAME_WEAPONS weapons;


	void INGAME_WEAPONS::set_data(MAP* map)
	{
		p_map = map;
	}

	void INGAME_WEAPONS::init(bool real)
	{
		pMutex.lock();

		thread_running = false;
		thread_ask_to_stop = false;

		idx_list.clear();
		free_idx.clear();
		nb_weapon = 0;
		weapon.clear();
		nuclogo.init();
		if (real)
		{
			byte *data = VFS::Instance()->readFile("anims\\fx.gaf");
			if (data)
			{
				nuclogo.loadGAFFromRawData(data, Gaf::RawDataGetEntryIndex(data, "nuclogo"));
				nuclogo.convert();
				nuclogo.clean();
				delete[] data;
			}
		}
		pMutex.unlock();
	}


	void INGAME_WEAPONS::destroy()
	{
		destroyThread();

		pMutex.lock();
		idx_list.clear();
		free_idx.clear();
		weapon.clear();
		nuclogo.destroy();
		pMutex.unlock();
		init(false);
	}


	INGAME_WEAPONS::INGAME_WEAPONS()
	{
		init(false);
	}

	INGAME_WEAPONS::~INGAME_WEAPONS()
	{
		destroy();
	}


	int INGAME_WEAPONS::add_weapon(int weapon_id,int shooter)
	{
		if (weapon_id < 0)
			return -1;

		MutexLocker locker(pMutex);

		if (nb_weapon < weapon.size())// S'il y a encore de la place
		{
			uint32 i = free_idx.back();
			free_idx.pop_back();
			idx_list.push_back(i);
			++nb_weapon;
			weapon[i].init();
			weapon[i].weapon_id = short(weapon_id);
			weapon[i].shooter_idx = short(shooter);
			weapon[i].idx = i;
			weapon[i].f_time = weapon_manager.weapon[weapon_id].flighttime;
			return i;
		}
		weapon.resize( weapon.size() + 1 );

		weapon[ weapon.size()-1 ].weapon_id = -1;
		uint32 index = uint32(weapon.size() - 1);
		idx_list.push_back( index );
		++nb_weapon;
		weapon[index].init();
		weapon[index].weapon_id = short(weapon_id);
		weapon[index].shooter_idx = short(shooter);
		weapon[index].idx=index;
		weapon[index].f_time=weapon_manager.weapon[weapon_id].flighttime;
		return index;
	}

	void INGAME_WEAPONS::move(float dt,MAP *map)
	{
		pMutex.lock();
		if (nb_weapon <= 0 || weapon.size() <= 0)
		{
			pMutex.unlock();
			return;
		}

		for (uint32 e = 0 ; e < idx_list.size() ; )
		{
			// TODO Check if it is really necessary by now
			pMutex.unlock();// Pause to give the renderer the time to work and to go at the given engine speed (in ticks per sec.)
			pMutex.lock();
			if (e >= idx_list.size())
				break;

			uint32 i = idx_list[e];
			weapon[i].move(dt,map);
			if (weapon[i].weapon_id < 0) // Remove it from the "alive" list
			{
				--nb_weapon;
				free_idx.push_back( i );
				idx_list[e] = idx_list.back();
				idx_list.pop_back();
			}
			else
				++e;
		}
		pMutex.unlock();
	}



	void INGAME_WEAPONS::draw(MAP* map, bool underwater)
	{
		pMutex.lock();
		if(nb_weapon<=0 || weapon.size()<=0)
		{
			pMutex.unlock();
			return;
		}

		gfx->lock();

		for(std::vector<uint32>::iterator e = idx_list.begin() ; e != idx_list.end() ; ++e)
		{
			uint32 i = *e;
			if((weapon[i].Pos.y < map->sealvl && underwater) || (weapon[i].Pos.y >= map->sealvl && !underwater))
				weapon[i].draw(map);
		}

		gfx->unlock();
		pMutex.unlock();
	}



	void INGAME_WEAPONS::draw_mini(float map_w,float map_h,int mini_w,int mini_h)				// Repère les unités sur la mini-carte
	{
		MutexLocker locker(pMutex);

		if(nb_weapon <= 0 || weapon.size() <= 0)
			return;

		float rw = 128.0f * float(mini_w) / (252.0f * map_w);
		float rh = 128.0f * float(mini_h) / (252.0f * map_h);

		Vector2D *points = new Vector2D[ idx_list.size() ];
		uint32 n(0);

		glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
		glEnable(GL_TEXTURE_2D);
		for (std::vector<uint32>::iterator e = idx_list.begin() ; e != idx_list.end() ; ++e)
		{
			uint32 i = *e;
			if(weapon_manager.weapon[weapon[i].weapon_id].cruise || weapon_manager.weapon[weapon[i].weapon_id].interceptor)
			{
				glEnable(GL_TEXTURE_2D);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
				int idx = weapon[i].owner;
				GFX::PutTextureInsideRect(nuclogo.glbmp[idx], weapon[i].Pos.x * rw + 64.0f - float(nuclogo.ofs_x[idx]),
										  weapon[i].Pos.z * rh + 64.0f - float(nuclogo.ofs_y[idx]),
										  weapon[i].Pos.x * rw + 63.0f - float(nuclogo.ofs_x[idx] + nuclogo.w[idx]),
										  weapon[i].Pos.z * rh + 63.0f - float(nuclogo.ofs_y[idx] + nuclogo.h[idx]));
				glDisable(GL_BLEND);
			}
			else
			{
				points[n].x = weapon[i].Pos.x*rw+64.0f;
				points[n].y = weapon[i].Pos.z*rh+64.0f;
				++n;
			}
		}
		glDisable(GL_TEXTURE_2D);

		glDisableClientState(GL_NORMAL_ARRAY);              // Render all points in one pass
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer( 2, GL_FLOAT, 0, points);

		glDrawArrays( GL_POINTS, 0, n );

		glEnable(GL_TEXTURE_2D);

		delete[] points;
	}


	void INGAME_WEAPONS::proc(void*)
	{
		thread_running = true;
		float dt = 1.0f / TICKS_PER_SEC;
		int weapon_timer = msec_timer;
		int counter = 0;

		weapon_engine_thread_sync = 0;

		while (!thread_ask_to_stop)
		{
			++counter;
			move(dt, p_map);					// Animate weapons
			features.move_forest(dt);			// Animate the forest

			ThreadSynchroniser->lock();
			ThreadSynchroniser->unlock();

			weapon_engine_thread_sync = 1;

			while (weapon_engine_thread_sync && !thread_ask_to_stop)
				rest(1);			// Wait until other thread sync with this one
		}
		thread_running = false;
		thread_ask_to_stop = false;
		LOG_INFO("Weapon engine: " << (float)(counter * 1000) / float(msec_timer - weapon_timer) << " ticks/sec");
	}


	void INGAME_WEAPONS::signalExitThread()
	{
		if (thread_running)
			thread_ask_to_stop = true;
	}



}
