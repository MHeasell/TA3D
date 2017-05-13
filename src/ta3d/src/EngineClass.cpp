/*  TA3D, a remake of Total Annihilation
	Copyright (C) 2005   Roland BROCHARD

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

/*----------------------------------------------------------\
  |                      EngineClass.cpp                      |
  |    Contient toutes les classes nécessaires au moteur 3D   |
  | et au moteur physique.                                    |
  |                                                           |
  \----------------------------------------------------------*/

#include "stdafx.h"
#include "misc/matrix.h"
#include "TA3D_NameSpace.h"
#include "ta3dbase.h"
#include "scripts/cob.h" // Pour la lecture et l'éxecution des scripts
#include "tdf.h"		 // Pour la gestion des éléments du jeu
#include "EngineClass.h"
#include "UnitEngine.h"
#include "gfx/fx.h"
#include "misc/ta3d_math.h"
#include "logs/logs.h"
#include "misc/tdf.h"

namespace TA3D
{
	unsigned int player_color_map[TA3D_PLAYERS_HARD_LIMIT] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	const float player_color[TA3D_PLAYERS_HARD_LIMIT * 3] = {0.11f, 0.28f, 0.91f,
		0.83f, 0.17f, 0.0f,
		1.0f, 1.0f, 1.0f,
		0.11f, 0.62f, 0.07f,
		0.03f, 0.12f, 0.48f,
		0.5f, 0.34f, 0.62f,
		1.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		0.61f, 0.8f, 0.87f,
		0.67f, 0.67f, 0.51f};

	MAP* the_map = NULL;

	// these are known to be wrong (they should be twice as big)
	// but other code currently has these values hard-coded.
	const int MAP::GraphicalTileWidthInPixels = 16;
	const int MAP::GraphicalTileHeightInPixels = 16;
	const int MAP::HeightmapTileWidthInPixels = 8;
	const int MAP::HeightmapTileHeightInPixels = 8;

	void SECTOR::init()
	{
		stuff = -1;
		unit_idx = -1;
		flags = 0U;
	}

	void MAP_OTA::init()
	{
		network = false;
		planet.clear();
		glamour.clear();
		missionname.clear();
		missiondescription.clear();
		numplayers.clear();
		map_size.clear();
		for (int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
			startX[i] = startZ[i] = 0;
		tidalstrength = 0;
		solarstrength = 22;
		lavaworld = false;
		killmul = 50;
		minwindspeed = 0;
		maxwindspeed = 0;
		gravity = 9.8f;
		SurfaceMetal = 0;
		MohoMetal = 0;
		waterdamage = 0;
		waterdoesdamage = false;
	}

	void MAP_OTA::destroy()
	{
		glamour.clear();
		planet.clear();
		missionname.clear();
		missiondescription.clear();
		numplayers.clear();
		map_size.clear();
		init();
	}

	void MAP::init()
	{
		view_map.fill(0);
		sight_map.fill(0);
		radar_map.fill(0);
		sonar_map.fill(0);

		wind = 0.0f;
		wind_dir = 0.0f;
		wind_vec.reset();
		ota_data.init();
		mini_w = mini_h = 252;
		ntex = 0;
		tex = NULL;
		nbbloc = 0;
		bloc = NULL;
		bmap.resize(0, 0);
		h_map.resize(0, 0);
		ph_map.resize(0, 0);
		map_data.resize(0, 0);
		sealvl = 0.0f;
		glmini.reset();
		lvl = NULL;
		water = true;
		tnt = false; // Laisse la possibilité de créer un autre format de cartes
		view.resize(0, 0);
		ox1 = ox2 = oy1 = oy2 = 0;
		int buf_size = 0;
		for (int i = 0; i < 6500; ++i)
		{
			buf_i[i++] = GLushort(0 + buf_size);
			buf_i[i++] = GLushort(1 + buf_size);
			buf_i[i++] = GLushort(3 + buf_size);
			buf_i[i++] = GLushort(4 + buf_size);
			buf_i[i++] = GLushort(6 + buf_size);
			buf_i[i++] = GLushort(7 + buf_size);
			buf_i[i++] = GLushort(7 + buf_size);
			buf_i[i++] = GLushort(8 + buf_size);
			buf_i[i++] = GLushort(4 + buf_size);
			buf_i[i++] = GLushort(5 + buf_size);
			buf_i[i++] = GLushort(1 + buf_size);
			buf_i[i++] = GLushort(2 + buf_size);
			buf_i[i] = GLushort(2 + buf_size);
			buf_size += 9;
		}
	}

	float MAP::get_unit_h(float x, float y) const
	{
		if (isNaN(x) || isNaN(y))
			return 0.0f;
		x = (x + (float)halfWidthInPixels) * 0.125f; // Convert the coordinates
		y = (y + (float)halfHeightInPixels) * 0.125f;
		const int lx = widthInHeightmapTiles - 1;
		const int ly = heightInHeightmapTiles - 1;
		x = Math::Clamp(x, 0.0f, float(widthInHeightmapTiles - 2));
		y = Math::Clamp(y, 0.0f, float(heightInHeightmapTiles - 2));
		float h[4];
		const int X = Math::Clamp((int)x, 0, lx - 1), Y = Math::Clamp((int)y, 0, ly - 1);
		const float dx = x - (float)X;
		const float dy = y - (float)Y;
		h[0] = h_map(X, Y);
		if (X + 1 < lx)
			h[1] = h_map(X + 1, Y) - h[0];
		else
			h[1] = 0.0f;

		if (Y + 1 < ly)
		{
			h[2] = h_map(X, Y + 1);
			if (X + 1 < lx)
				h[3] = h_map(X + 1, Y + 1) - h[2];
			else
				h[3] = 0.0f;
		}
		else
		{
			h[2] = h[0];
			h[3] = h[1];
		}
		h[0] = h[0] + h[1] * dx;
		return h[0] + (h[2] + h[3] * dx - h[0]) * dy;
	}

	float MAP::get_max_h(int x, int y) const
	{
		x = Math::Clamp(x, 0, widthInHeightmapTiles - 2);
		y = Math::Clamp(y, 0, heightInHeightmapTiles - 2);
		float h = h_map(x, y);
		if (x < widthInHeightmapTiles - 2)
			h = Math::Max(h, h_map(x + 1, y));
		if (y < heightInHeightmapTiles - 2)
		{
			h = Math::Max(h, h_map(x, y + 1));
			if (x < widthInHeightmapTiles - 2)
				h = Math::Max(h, h_map(x + 1, y + 1));
		}
		return h;
	}

	float MAP::get_max_rect_h(int x, int y, int w, int h) const
	{
		const int x1 = Math::Clamp(x - (w >> 1), 0, widthInHeightmapTiles - 2);
		const int x2 = Math::Clamp(x1 + w, 0, widthInHeightmapTiles - 2);
		const int y1 = Math::Clamp(y - (h >> 1), 0, heightInHeightmapTiles - 2);
		const int y2 = Math::Clamp(y1 + h, 0, heightInHeightmapTiles - 2);
		float max_h = h_map(x1, y1);
		for (int Y = y1; Y <= y2; ++Y)
			for (int X = x1; X <= x2; ++X)
				max_h = std::max(max_h, h_map(X, Y));
		return max_h;
	}

	void MAP::obstaclesRect(int x1, int y1, int w, int h, bool b, const String& yardmap, bool open)
	{
		if (yardmap.empty())
		{
			const int x2 = Math::Min(x1 + w, widthInHeightmapTiles);
			const int y2 = Math::Min(y1 + h, heightInHeightmapTiles);
			y1 = Math::Max(y1, 0);
			x1 = Math::Max(x1, 0);
			for (int y = y1; y < y2; ++y)
				for (int x = x1; x < x2; ++x)
					obstacles(x, y) = b;
		}
		else
		{
			const int x2 = Math::Min(x1 + w, widthInHeightmapTiles);
			const int y2 = Math::Min(y1 + h, heightInHeightmapTiles);
			const int oy1 = y1;
			const int ox1 = x1;
			y1 = Math::Max(y1, 0);
			x1 = Math::Max(x1, 0);
			uint32 i = (y1 - oy1) * w + x1 - ox1;
			const uint32 s = w - (x2 - x1);
			const uint32 l = yardmap.size();
			for (int y = y1; y < y2 && l > i; ++y)
			{
				for (int x = x1; x < x2 && l > i; ++x, ++i)
				{
					switch (yardmap[i])
					{
						case 'G':
						case 'o':
						case 'w':
						case 'f':
							obstacles(x, y) = b;
							break;
						case 'c':
						case 'C':
							if (!open)
								obstacles(x, y) = b;
							break;
						case 'O':
							if (open)
								obstacles(x, y) = b;
							break;
					};
				}
				i += s;
			}
		}
	}

	void MAP::rect(int x1, int y1, int w, int h, int c, const String& yardmap, bool open)
	{
		if (yardmap.empty())
		{
			const int y2 = std::min(y1 + h, heightInHeightmapTiles - 1);
			const int x2 = std::min(x1 + w, widthInHeightmapTiles - 1);
			y1 = std::max(y1, 0);
			x1 = std::max(x1, 0);
			if (y2 <= y1 || x2 <= x1)
				return;
			pMutex.lock();
			for (int y = y1; y < y2; ++y)
				for (int x = x1; x < x2; ++x)
					map_data(x, y).unit_idx = c;
			pMutex.unlock();
		}
		else
		{
			uint32 i = 0;
			const int y2 = std::min(y1 + h, heightInHeightmapTiles - 1);
			const int x2 = std::min(x1 + w, widthInHeightmapTiles - 1);
			if (y1 < 0)
				i -= y1 * w;
			if (x1 < 0)
				i -= x1;
			y1 = std::max(y1, 0);
			x1 = std::max(x1, 0);
			if (y2 <= y1 || x2 <= x1)
				return;

			const int dw = w - (x2 - x1);
			const unsigned int l = yardmap.size();
			pMutex.lock();
			for (int y = y1; y < y2; ++y)
			{
				for (int x = x1; x < x2; ++x)
				{
					if (l <= i)
					{
						pMutex.unlock();
						return;
					}
					switch (yardmap[i])
					{
						case 'G':
						case 'o':
						case 'w':
						case 'f':
							map_data(x, y).unit_idx = c;
							break;
						case 'c':
						case 'C':
							if (!open)
								map_data(x, y).unit_idx = c;
							break;
						case 'O':
							if (open)
								map_data(x, y).unit_idx = c;
							break;
					};
					++i;
				}
				i += dw;
			}
			pMutex.unlock();
		}
	}

	bool MAP::check_rect(int x1, int y1, int w, int h, const int c) const
	{
		if (w == 0 || h == 0)
			return true;
		const int y2 = std::min(y1 + h, heightInHeightmapTiles - 1);
		const int x2 = std::min(x1 + w, widthInHeightmapTiles - 1);
		y1 = std::max(y1, 0);
		x1 = std::max(x1, 0);
		if (y2 <= y1 || x2 <= x1)
			return false;
		for (int y = y1; y < y2; ++y)
			for (int x = x1; x < x2; ++x)
			{
				const int idx = map_data(x, y).unit_idx;
				if (idx != c && idx != -1)
					return false;
			}
		return true;
	}

	bool MAP::check_rect_discovered(int x1, int y1, int w, int h, const int c) const // Check if the area has been fully discovered
	{
		const int y2 = std::min((y1 + h + 1) >> 1, heightInGraphicalTiles - 1);
		const int x2 = std::min((x1 + w + 1) >> 1, widthInGraphicalTiles - 1);
		x1 >>= 1;
		y1 >>= 1;
		y1 = std::max(y1, 0);
		x1 = std::max(x1, 0);
		if (y2 <= y1 || x2 <= x1)
			return false;
		for (int y = y1; y < y2; ++y)
			for (int x = x1; x < x2; ++x)
				if (!(view_map(x, y) & c))
					return false;
		return true;
	}

	float MAP::check_rect_dh(int x1, int y1, int w, int h) const
	{
		const int y2 = std::min(y1 + h, heightInHeightmapTiles - 1);
		const int x2 = std::min(x1 + w, widthInHeightmapTiles - 1);
		y1 = std::max(y1, 0);
		x1 = std::max(x1, 0);
		if (y2 <= y1 || x2 <= x1)
			return 0.0f;
		float max_dh = 0.0f;
		bool on_water = false;
		for (int y = y1; y < y2; ++y)
			for (int x = x1; x < x2; ++x)
			{
				max_dh = Math::Max(max_dh, slope(x, y));
				on_water |= map_data(x, y).isUnderwater();
			}
		if (on_water)
			max_dh = -max_dh;
		return max_dh;
	}

	float MAP::check_max_depth(int x1, int y1, int w, int h) const
	{
		const int y2 = std::min(y1 + h, heightInHeightmapTiles - 1);
		const int x2 = std::min(x1 + w, widthInHeightmapTiles - 1);
		y1 = std::max(y1, 0);
		x1 = std::max(x1, 0);
		if (y2 <= y1 || x2 <= x1)
			return 0.0f;
		float depth = -sealvl;
		for (int y = y1; y < y2; ++y)
			for (int x = x1; x < x2; ++x)
				depth = std::max(depth, -h_map(x, y));
		return depth + sealvl;
	}

	float MAP::check_min_depth(int x1, int y1, int w, int h) const
	{
		const int y2 = std::min(y1 + h, heightInHeightmapTiles - 1);
		const int x2 = std::min(x1 + w, widthInHeightmapTiles - 1);
		y1 = std::max(y1, 0);
		x1 = std::max(x1, 0);
		if (y2 <= y1 || x2 <= x1)
			return 255.0f;
		float depth = 255.0f - sealvl;
		for (int y = y1; y < y2; ++y)
			for (int x = x1; x < x2; ++x)
				depth = std::min(depth, -h_map(x, y));
		return depth + sealvl;
	}

	bool MAP::check_vents(int x1, int y1, int w, int h, const String& yard_map) const
	{
		if (yard_map.empty())
			return true;
		const int y2 = std::min(y1 + h, heightInHeightmapTiles - 1);
		const int x2 = std::min(x1 + w, widthInHeightmapTiles - 1);
		y1 = std::max(y1, 0);
		x1 = std::max(x1, 0);

		if (y2 <= y1 || x2 <= x1)
			return false;

		const int dw = w - (x2 - x1);
		const unsigned int l = yard_map.size();
		int i = 0;
		bool ok = true;
		for (int y = y1; y < y2; ++y)
		{
			for (int x = x1; x < x2; ++x)
			{
				if (l <= (unsigned int)i)
					return ok;
				if (yard_map[i] == 'G')
				{
					ok = false;
					const int feature_id = map_data(x, y).stuff;
					if (feature_id >= 0)
					{
						const Feature* const pFeature = feature_manager.getFeaturePointer(features.feature[feature_id].type);
						if (pFeature && pFeature->geothermal)
							return true;
					}
				}
				++i;
			}
			i += dw;
		}
		return ok;
	}

	bool MAP::check_lava(int x1, int y1, int w, int h) const
	{
		const int y2 = std::min(y1 + h, heightInHeightmapTiles - 1);
		const int x2 = std::min(x1 + w, widthInHeightmapTiles - 1);
		y1 = std::max(y1, 0);
		x1 = std::max(x1, 0);

		if (y2 <= y1 || x2 <= x1)
			return false;
		for (int y = y1; y < y2; ++y)
		{
			for (int x = x1; x < x2; ++x)
			{
				if (bloc[bmap(x, y)].lava)
					return true;
			}
		}
		return false;
	}

	void MAP::clean_map() // Used to remove all objects when loading a saved game
	{
		for (int y = 0; y < heightInHeightmapTiles; ++y)
		{
			for (int x = 0; x < widthInHeightmapTiles; ++x)
			{
				map_data(x, y).stuff = -1;
				map_data(x, y).unit_idx = -1;
			}
		}
	}

	void MAP::destroy()
	{
		ota_data.destroy();

		if (ntex > 0)
		{
			for (int i = 0; i < ntex; i++)
				gfx->destroy_texture(tex[i]);
			DELETE_ARRAY(tex);
		}
		if (lvl)
		{
			for (int i = 0; i < heightInGraphicalTiles * widthInGraphicalTiles; i++)
				DELETE_ARRAY(lvl[i]);
			DELETE_ARRAY(lvl);
		}
		if (bloc && nbbloc > 0)
		{
			for (int i = 0; i < nbbloc; i++)
			{
				bloc[i].point = NULL;
				bloc[i].destroy();
			}
			DELETE_ARRAY(bloc);
		}
		init();

		the_map = NULL;
	}

	void MAP::clear_FOW(sint8 FOW_flags)
	{
		if (FOW_flags < 0)
			FOW_flags = fog_of_war;
		fog_of_war = FOW_flags;

		if (fog_of_war & FOW_BLACK)
			view_map.fill(0);
		else
			view_map.fill(0xFF);
		if (fog_of_war & FOW_GREY)
			sight_map.fill(0);
		else
			sight_map.fill(0xFF);

		if (fog_of_war == FOW_DISABLED)
		{
			radar_map.fill(0xFF);
			sonar_map.fill(0xFF);
		}
	}

	void MAP_OTA::load(const String& filename)
	{
		File* file = VFS::Instance()->readFile(filename);
		if (file)
		{
			load(file);
			delete file;
		}
	}

	void MAP_OTA::load(File* file)
	{
		destroy();

		TDFParser parser;
		parser.loadFromMemory("OTA", file->data(), file->size(), false, true, false);
		file->close();

		missionname = parser.pullAsString("GlobalHeader.missionname");
		planet = parser.pullAsString("GlobalHeader.planet");
		glamour = parser.pullAsString("GlobalHeader.glamour");
		missiondescription = parser.pullAsString("GlobalHeader.missiondescription");
		tidalstrength = parser.pullAsInt("GlobalHeader.tidalstrength");
		solarstrength = parser.pullAsInt("GlobalHeader.solarstrength");
		lavaworld = parser.pullAsBool("GlobalHeader.lavaworld");
		killmul = (short)parser.pullAsInt("GlobalHeader.killmul", 50);
		minwindspeed = parser.pullAsInt("GlobalHeader.minwindspeed");
		maxwindspeed = parser.pullAsInt("GlobalHeader.maxwindspeed");
		gravity = (float)parser.pullAsInt("GlobalHeader.gravity") * 0.1f;
		numplayers = parser.pullAsString("GlobalHeader.numplayers");
		map_size = parser.pullAsString("GlobalHeader.size");
		SurfaceMetal = parser.pullAsInt("GlobalHeader.Schema 0.SurfaceMetal");
		MohoMetal = parser.pullAsInt("GlobalHeader.Schema 0.MohoMetal");
		for (int s = 0; parser.exists(String("globalheader.schema 0.specials.special") << s); s++)
		{
			String key = String("GlobalHeader.Schema 0.specials.special") << s << '.';
			String specialWhat = parser.pullAsString(String(key) << "specialwhat");
			specialWhat.toLower();
			if (specialWhat.startsWith("startpos"))
			{
				int index = Substr(specialWhat, 8, specialWhat.size() - 8).to_int() - 1;
				startX[index] = parser.pullAsInt(String(key) << "xpos");
				startZ[index] = parser.pullAsInt(String(key) << "zpos");
			}
		}
		waterdoesdamage = parser.pullAsBool("GlobalHeader.waterdoesdamage");
		waterdamage = parser.pullAsInt("GlobalHeader.waterdamage");
		String tmp = parser.pullAsString("GlobalHeader.Schema 0.type");
		tmp.toLower();
		network = tmp.startsWith("network");
		// Special flag for slate world which has a white fog background
		whitefog = String(planet).toUpper() == "SLATE";

		if (waterdamage == 0)
			waterdoesdamage = false;
	}

	void MAP::draw_mini(int x1, int y1, int w, int h, Camera* cam, byte player_mask) // Draw the mini-map
	{
		if (!glmini)
			return; // Check if it exists

		gfx->set_color(0xFFFFFFFF);

		const int rw = w * mini_w / 252;
		const int rh = h * mini_h / 252;
		x1 += (w - rw) >> 1;
		y1 += (h - rh) >> 1;
		const float lw = (float)mini_w / 252.0f;
		const float lh = (float)mini_h / 252.0f;
		gfx->drawtexture(glmini.get(), (float)x1, (float)y1, (float)x1 + (float)rw, (float)y1 + (float)rh, 0.0f, 0.0f, lw, lh);

		if (rh == 0 || rw == 0)
			return;

		if (fog_of_war != FOW_DISABLED)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR); // Special blending function
			glDisable(GL_TEXTURE_2D);

			int MY = 0;
			const int DY = 0x10000 * (heightInHeightmapTiles - 2) / rh;
			const int DX = 0x10000 * (widthInHeightmapTiles - 2) / rw;

			std::vector<Vector2D> lines;
			std::vector<uint32> colors;
			uint32 col = 0U;

			gfx->lock();

			for (int y = 0; y < rh; ++y)
			{
				const int my = MY >> 17;
				MY += DY;
				int old_col = -1;
				int old_x = -1;
				int MX = 0;
				for (int x = 0; x < rw; ++x)
				{
					const int mx = MX >> 17;
					MX += DX;
					if (!(view_map(mx, my) & player_mask))
					{
						if (old_col != 0)
						{
							if (old_x != -1)
							{
								lines.push_back(Vector2D(float(x1 + old_x), float(y1 + y)));
								lines.push_back(Vector2D(float(x1 + x), float(y1 + y)));
								colors.push_back(col);
								colors.push_back(col);
							}
							col = 0x00000000U;
							old_col = 0;
							old_x = x;
						}
					}
					else
					{
						if (!(sight_map(mx, my) & player_mask))
						{
							if (old_col != 1)
							{
								if (old_x != -1)
								{
									lines.push_back(Vector2D(float(x1 + old_x), float(y1 + y)));
									lines.push_back(Vector2D(float(x1 + x), float(y1 + y)));
									colors.push_back(col);
									colors.push_back(col);
								}
								col = 0x7F7F7F7FU;
								old_x = x;
								old_col = 1;
							}
						}
						else
						{
							if (old_col != 2)
							{
								if (old_x != -1)
								{
									lines.push_back(Vector2D(float(x1 + old_x), float(y1 + y)));
									lines.push_back(Vector2D(float(x1 + x), float(y1 + y)));
									colors.push_back(col);
									colors.push_back(col);
								}
								old_x = -1;
								old_col = 2;
							}
						}
					}
				}
				if (old_x != -1)
				{
					lines.push_back(Vector2D(float(x1 + old_x), float(y1 + y)));
					lines.push_back(Vector2D(float(x1 + rw), float(y1 + y)));
					colors.push_back(col);
					colors.push_back(col);
				}
			}

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, &(lines.front()));
			glColorPointer(4, GL_UNSIGNED_BYTE, 0, &(colors.front()));

			glDrawArrays(GL_LINES, 0, (GLsizei)lines.size());

			glDisableClientState(GL_COLOR_ARRAY);
			glDisable(GL_BLEND);

			gfx->unlock();
		}

		if (!cam)
			return;

		std::vector<Vector3D> viewportCoords = cam->getProjectionShadow();

		glDisable(GL_TEXTURE_2D);
		glColor3ub(0xE5, 0xE5, 0x66);
		glBegin(GL_LINE_LOOP);
		for (auto it = viewportCoords.begin(); it != viewportCoords.end(); ++it)
		{
			// project from world space to minimap space
			float projectedX = ((it->x + (widthInPixels / 2.0f)) / widthInPixels) * rw;
			float projectedY = ((it->z + (heightInPixels / 2.0f)) / heightInPixels) * rh;
			glVertex2f(projectedX, projectedY);
		}
		glEnd();
		glColor3ub(0xFF, 0xFF, 0xFF);
		glEnable(GL_TEXTURE_2D);
	}

	void MAP::update_player_visibility(int player_id, int px, int py, int r, int rd, int sn, int rd_j, int sn_j, bool jamming, bool black)
	{
		gfx->lock();

		px >>= 1;
		py >>= 1;

		// Update jamming maps
		if (jamming)
		{
			if (rd_j > 0)
				radar_map.circlefill(px, py, rd_j, 0);
			if (sn_j > 0)
				sonar_map.circlefill(px, py, sn_j, 0);
		}
		else
		{
			const byte mask = byte(1 << player_id);
			const int r2 = r * r;
			const int rd2 = rd * rd;
			const int sn2 = sn * sn;
			// Update detector maps
			if (sn > 0)
				for (int y = 0; y <= sn; ++y) // Update sonar data
				{
					const int x = (int)(0.5f + std::sqrt((float)(sn2 - y * y)));
					int ry = py - y;
					if (ry >= 0 && ry < sonar_map.getHeight())
					{
						int rx = std::max<int>(0, px - x);
						const int lx = std::min<int>(px + x, sonar_map.getWidth() - 1);
						for (; rx <= lx; ++rx)
							sonar_map(rx, ry) |= mask;
					}
					if (y != 0)
					{
						ry = py + y;
						if (ry >= 0 && ry < sonar_map.getHeight())
						{
							int rx = std::max<int>(0, px - x);
							const int lx = std::min<int>(px + x, sonar_map.getWidth() - 1);
							for (; rx <= lx; ++rx)
								sonar_map(rx, ry) |= mask;
						}
					}
				}
			if (rd > 0)
				for (int y = 0; y <= rd; ++y) // Update radar data
				{
					const int x = (int)(0.5f + std::sqrt((float)(rd2 - y * y)));
					int ry = py - y;
					if (ry >= 0 && ry < radar_map.getHeight())
					{
						int rx = std::max<int>(0, px - x);
						const int lx = std::min<int>(px + x, radar_map.getWidth() - 1);
						for (; rx <= lx; ++rx)
							radar_map(rx, ry) |= mask;
					}
					if (y != 0)
					{
						ry = py + y;
						if (ry >= 0 && ry < radar_map.getHeight())
						{
							int rx = std::max<int>(0, px - x);
							const int lx = std::min<int>(px + x, radar_map.getWidth() - 1);
							for (; rx <= lx; ++rx)
								radar_map(rx, ry) |= mask;
						}
					}
				}
			if (fog_of_war & FOW_GREY)
				for (int y = 0; y <= r; ++y) // Update view data
				{
					const int x = (int)(0.5f + std::sqrt((float)(r2 - y * y)));
					int ry = py - y;
					if (ry >= 0 && ry < sight_map.getHeight())
					{
						int rx = std::max<int>(0, px - x);
						const int lx = std::min<int>(px + x, sight_map.getWidth() - 1);
						for (; rx <= lx; ++rx)
							sight_map(rx, ry) |= mask;
					}
					if (y != 0)
					{
						ry = py + y;
						if (ry >= 0 && ry < sight_map.getHeight())
						{
							int rx = std::max<int>(0, px - x);
							const int lx = std::min<int>(px + x, sight_map.getWidth() - 1);
							for (; rx <= lx; ++rx)
								sight_map(rx, ry) |= mask;
						}
					}
				}
			if (black && (fog_of_war & FOW_BLACK))
				for (int y = 0; y <= r; ++y) // Update view data
				{
					const int x = (int)(0.5f + std::sqrt((float)(r2 - y * y)));
					int ry = py - y;
					if (ry >= 0 && ry < view_map.getHeight())
					{
						int rx = std::max<int>(0, px - x);
						const int lx = std::min<int>(px + x, view_map.getWidth() - 1);
						for (; rx <= lx; ++rx)
							view_map(rx, ry) |= mask;
					}
					if (y != 0)
					{
						ry = py + y;
						if (ry >= 0 && ry < view_map.getHeight())
						{
							int rx = std::max<int>(0, px - x);
							const int lx = std::min<int>(px + x, view_map.getWidth() - 1);
							for (; rx <= lx; ++rx)
								view_map(rx, ry) |= mask;
						}
					}
				}
		}
		gfx->unlock();
	}

	void MAP::draw(Camera* cam, byte player_mask)
	{
		cam->applyToOpenGl();

		draw_HD(cam, player_mask);
	}

	void renderLine(std::vector<int>& xMin, std::vector<int>& xMax, int x0, int y0, int x1, int y1, int xmax)
	{
		if (y0 == y1) // We don't need to handle this case here
			return;
		if (y0 > y1) // We want y0 < y1
		{
			x0 ^= x1;
			x1 ^= x0;
			x0 ^= x1;
			y0 ^= y1;
			y1 ^= y0;
			y0 ^= y1;
		}
		int ymax = Math::Min((int)y1, (int)xMin.size() - 1);
		for (int y = Math::Max(y0, 0); y <= ymax; y++)
		{
			int x = x0 + (y - y0) * (x1 - x0) / (y1 - y0);
			if (x < 0)
				x = 0;
			else if (x > xmax)
				x = xmax;
			if (xMin[y] > x || xMin[y] == -1)
				xMin[y] = x;
			if (xMax[y] < x || xMax[y] == -1)
				xMax[y] = x;
		}
	}

	void MAP::draw_HD(Camera* cam, byte player_mask)
	{
		glPushMatrix();

		gfx->lock();

		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);

		if (ntex > 0)
			gfx->ReInitAllTex(true);
		else
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		// ------------------------------------------------------------------
		// Beginning of visible area calculations
		// ------------------------------------------------------------------

		Vector3D cameraExtents(cam->viewportWidth() / 2.0f, 0.0f, cam->viewportHeight() / 2.0f);
		auto topLeft = worldToGraphicalTileIndex(cam->position() - cameraExtents);
		auto bottomRight = worldToGraphicalTileIndex(cam->position() + cameraExtents);
		int x1 = Math::Clamp(topLeft.x, 0, widthInGraphicalTiles - 1);
		int y1 = Math::Clamp(topLeft.y, 0, heightInGraphicalTiles - 1);
		int x2 = Math::Clamp(bottomRight.x, 0, widthInGraphicalTiles - 1);
		int y2 = Math::Clamp(bottomRight.y, 0, heightInGraphicalTiles - 1);

		// ------------------------------------------------------------------
		// End of visible area calculations
		// ------------------------------------------------------------------

		glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);

		if (ntex > 0)
		{
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glEnable(GL_TEXTURE_2D);
			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		GLuint old_tex = bloc[0].tex;
		glBindTexture(GL_TEXTURE_2D, old_tex);

		for (int y = oy1; y <= oy2; ++y)
			memset(&(view(ox1, y)), 0, ox2 - ox1 + 1);
		features.list.clear();
		ox1 = x1;
		ox2 = x2;
		oy1 = y1;
		oy2 = y2;

		Vector3D buf_p[4500]; // Tampon qui accumule les blocs pour les dessiner en chaîne
		float buf_t[9000];
		uint8 buf_c[18000];
		short buf_size = 0; // in blocs
		uint16 index_size = 0;
		bool was_flat = false;
		glDisableClientState(GL_NORMAL_ARRAY); // we don't need normal data
		glEnableClientState(GL_VERTEX_ARRAY);  // vertex coordinates
		glEnableClientState(GL_COLOR_ARRAY);   // Colors(for fog of war)
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, buf_c);
		glVertexPointer(3, GL_FLOAT, 0, buf_p);

		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		glTexCoordPointer(2, GL_FLOAT, 0, buf_t);

		int ox = x1;

		Vector3D T;
		Vector3D V;
		for (int y = y1; y <= y2; ++y) // Scans blocks that can be seen to draw those that are visible
		{
			const int pre_y = y * 16;
			const int Y = y * 2;
			const int pre_y2 = y * widthInGraphicalTiles;
			T.x = (float)-halfWidthInPixels;
			T.y = 0.0f;
			T.z = float(pre_y - halfHeightInPixels);
			buf_size = 0;
			ox = x1;
			bool was_clean = false;

			for (int x = x1; x <= x2; ++x)
			{
				int X = x * 2;

				if (!isDiscoveredBy(player_mask, x, y))
				{
					if (water)
					{
						if (map_data(X, Y).isUnderwater() && map_data(X, Y + 1).isUnderwater() && map_data(X + 1, Y).isUnderwater() && map_data(X + 1, Y + 1).isUnderwater())
							view(x, y) = 2;
						else
							view(x, y) = 3;
					}
				}
				else
				{
					if (!isInSightOf(player_mask, x, y))
					{
						if (map_data(X, Y).isUnderwater() || map_data(X, Y + 1).isUnderwater() || map_data(X + 1, Y).isUnderwater() || map_data(X + 1, Y + 1).isUnderwater())
							view(x, y) = 2;
						else
							view(x, y) = 3;
					}
					else
						view(x, y) = 1;

					if (map_data(X, Y).stuff >= 0 && map_data(X, Y).stuff < features.max_features) // Flag are visible objects in that bloc
					{
						if (features.feature[map_data(X, Y).stuff].type < 0)
							map_data(X, Y).stuff = -1;
						else
						{
							features.feature[map_data(X, Y).stuff].draw = true;
							features.feature[map_data(X, Y).stuff].grey = (view(x, y) & 2) == 2;
							features.list.push_back(map_data(X, Y).stuff);
						}
					}
					if (map_data(X + 1, Y).stuff >= 0 && map_data(X + 1, Y).stuff < features.max_features)
					{
						if (features.feature[map_data(X + 1, Y).stuff].type < 0)
							map_data(X + 1, Y).stuff = -1;
						else
						{
							features.feature[map_data(X + 1, Y).stuff].draw = true;
							features.feature[map_data(X + 1, Y).stuff].grey = (view(x, y) & 2) == 2;
							features.list.push_back(map_data(X + 1, Y).stuff);
						}
					}
					if (map_data(X, Y + 1).stuff >= 0 && map_data(X, Y + 1).stuff < features.max_features)
					{
						if (features.feature[map_data(X, Y + 1).stuff].type < 0)
							map_data(X, Y + 1).stuff = -1;
						else
						{
							features.feature[map_data(X, Y + 1).stuff].draw = true;
							features.feature[map_data(X, Y + 1).stuff].grey = (view(x, y) & 2) == 2;
							features.list.push_back(map_data(X, Y + 1).stuff);
						}
					}
					if (map_data(X + 1, Y + 1).stuff >= 0 && map_data(X + 1, Y + 1).stuff < features.max_features)
					{
						if (features.feature[map_data(X + 1, Y + 1).stuff].type < 0)
							map_data(X + 1, Y + 1).stuff = -1;
						else
						{
							features.feature[map_data(X + 1, Y + 1).stuff].draw = true;
							features.feature[map_data(X + 1, Y + 1).stuff].grey = (view(x, y) & 2) == 2;
							features.list.push_back(map_data(X + 1, Y + 1).stuff);
						}
					}
				}

				// If the player can not see this piece, it is not drawn in clear
				T.x += float(x * 16);
				const int i = bmap(x, y);

				bloc[i].point = lvl[pre_y2 + x];
				if (bloc[i].point == NULL)
				{
					lvl[pre_y2 + x] = bloc[i].point = new Vector3D[9];
					if (tnt)
					{
						bloc[i].point[0].x = T.x;
						bloc[i].point[0].z = get_zdec(X, Y) + T.z;
						bloc[i].point[1].x = 8.0f + T.x;
						bloc[i].point[1].z = get_zdec(X | 1, Y) + T.z;
						bloc[i].point[2].x = 16.0f + T.x;
						bloc[i].point[2].z = get_zdec(X + 2, Y) + T.z;
						bloc[i].point[3].x = T.x;
						bloc[i].point[3].z = 8.0f + get_zdec(X, Y | 1) + T.z;
						bloc[i].point[4].x = 8.0f + T.x;
						bloc[i].point[4].z = 8.0f + get_zdec(X | 1, Y | 1) + T.z;
						bloc[i].point[5].x = 16.0f + T.x;
						bloc[i].point[5].z = 8.0f + get_zdec(X + 2, Y | 1) + T.z;
						bloc[i].point[6].x = T.x;
						bloc[i].point[6].z = 16.0f + get_zdec(X, Y + 2) + T.z;
						bloc[i].point[7].x = 8.0f + T.x;
						bloc[i].point[7].z = 16.0f + get_zdec(X | 1, Y + 2) + T.z;
						bloc[i].point[8].x = 16.0f + T.x;
						bloc[i].point[8].z = 16.0f + get_zdec(X + 2, Y + 2) + T.z;
						bloc[i].point[0].y = get_nh(X, Y);
						bloc[i].point[1].y = get_nh(X | 1, Y);
						bloc[i].point[2].y = get_nh(X + 2, Y);
						bloc[i].point[3].y = get_nh(X, Y | 1);
						bloc[i].point[4].y = get_nh(X | 1, Y | 1);
						bloc[i].point[5].y = get_nh(X + 2, Y | 1);
						bloc[i].point[6].y = get_nh(X, Y + 2);
						bloc[i].point[7].y = get_nh(X | 1, Y + 2);
						bloc[i].point[8].y = get_nh(X + 2, Y + 2);
					}
					else
					{
						bloc[i].point[0].x = T.x;
						bloc[i].point[0].z = T.z;
						bloc[i].point[1].x = 8.0f + T.x;
						bloc[i].point[1].z = T.z;
						bloc[i].point[2].x = 16.0f + T.x;
						bloc[i].point[2].z = T.z;
						bloc[i].point[3].x = T.x;
						bloc[i].point[3].z = 8.0f + T.z;
						bloc[i].point[4].x = 8.0f + T.x;
						bloc[i].point[4].z = 8.0f + T.z;
						bloc[i].point[5].x = 16.0f + T.x;
						bloc[i].point[5].z = 8.0f + T.z;
						bloc[i].point[6].x = T.x;
						bloc[i].point[6].z = 16.0f + T.z;
						bloc[i].point[7].x = 8.0f + T.x;
						bloc[i].point[7].z = 16.0f + T.z;
						bloc[i].point[8].x = 16.0f + T.x;
						bloc[i].point[8].z = 16.0f + T.z;
						bloc[i].point[0].y = get_h(X, Y);
						bloc[i].point[1].y = get_h(X | 1, Y);
						bloc[i].point[2].y = get_h(X + 2, Y);
						bloc[i].point[3].y = get_h(X, Y | 1);
						bloc[i].point[4].y = get_h(X | 1, Y | 1);
						bloc[i].point[5].y = get_h(X + 2, Y | 1);
						bloc[i].point[6].y = get_h(X, Y + 2);
						bloc[i].point[7].y = get_h(X | 1, Y + 2);
						bloc[i].point[8].y = get_h(X + 2, Y + 2);
					}
					map_data(X, Y).setFlat();
					for (int f = 1; f < 9; ++f) // Check if it's flat
					{
						if (!Math::AlmostEquals(bloc[i].point[0].y, bloc[i].point[f].y))
						{
							map_data(X, Y).unsetFlat();
							break;
						}
					}
				}

				if (bloc[i].tex != old_tex || buf_size >= 500 || ox + 1 < x)
				{
					if (buf_size > 0)
						glDrawRangeElements(GL_TRIANGLE_STRIP, 0, buf_size * 9, index_size, GL_UNSIGNED_SHORT, buf_i); // dessine le tout
					buf_size = 0;
					index_size = 0;
					was_flat = false;
					if (old_tex != bloc[i].tex)
					{
						old_tex = bloc[i].tex;
						glBindTexture(GL_TEXTURE_2D, bloc[i].tex);
					}
				}
				ox = x;

				size_t buf_pos = buf_size * 9U;

				for (int e = 0; e < 9; ++e) // Copie le bloc
					buf_p[buf_pos + e] = bloc[i].point[e];

				uint8* color = buf_c + (buf_pos * 4);

				for (int e = 0; e < 36; e += 4)
					color[e] = color[e + 1] = color[e + 2] = color[e + 3] = 255;

				bool is_clean = true;
				if (fog_of_war != FOW_DISABLED)
				{
					int Z;
					int grey = 0;
					int black = 0;
					Z = Y + get_zdec_notest(X, Y);
					if (Z >= heightInHeightmapTiles - 1)
						Z = heightInHeightmapTiles - 2;
					if (!isDiscoveredBy(player_mask, x, Z / 2))
					{
						color[0] = color[1] = color[2] = 0;
						++black;
					}
					else if (!isInSightOf(player_mask, x, Z / 2))
					{
						color[0] = color[1] = color[2] = 127;
						++grey;
					}
					if (X + 2 < widthInHeightmapTiles)
					{
						Z = Y + get_zdec_notest(X + 2, Y);
						if (Z >= heightInHeightmapTiles - 1)
							Z = heightInHeightmapTiles - 2;
						if (!isDiscoveredBy(player_mask, x + 1, Z / 2))
						{
							color[8] = color[9] = color[10] = 0;
							++black;
						}
						else if (!isInSightOf(player_mask, x + 1, Z / 2))
						{
							color[8] = color[9] = color[10] = 127;
							++grey;
						}
					}
					if (Y + 2 < heightInHeightmapTiles)
					{
						Z = Y + 2 + get_zdec_notest(X, Y + 2);
						if (Z >= heightInHeightmapTiles - 1)
							Z = heightInHeightmapTiles - 2;
						if (!isDiscoveredBy(player_mask, x, Z / 2))
						{
							color[24] = color[25] = color[26] = 0;
							++black;
						}
						else if (!isInSightOf(player_mask, x, Z / 2))
						{
							color[24] = color[25] = color[26] = 127;
							++grey;
						}
						if (X + 2 < widthInHeightmapTiles)
						{
							Z = Y + 2 + get_zdec_notest(X + 2, Y + 2);
							if (Z >= heightInHeightmapTiles - 1)
								Z = heightInHeightmapTiles - 2;
							if (!isDiscoveredBy(player_mask, x + 1, Z / 2))
							{
								color[32] = color[33] = color[34] = 0;
								++black;
							}
							else if (!isInSightOf(player_mask, x + 1, Z / 2))
							{
								color[32] = color[33] = color[34] = 127;
								++grey;
							}
						}
					}
					is_clean = grey == 4 || black == 4 || (grey == 0 && black == 0);
					if (!map_data(X, Y).isFlat())
					{
						color[4] = color[5] = color[6] = uint8((color[0] + color[8]) / 2);
						color[12] = color[13] = color[14] = uint8((color[0] + color[24]) / 2);
						color[20] = color[21] = color[22] = uint8((color[8] + color[32]) / 2);
						color[16] = color[17] = color[18] = uint8((color[12] + color[20]) / 2);
						color[28] = color[29] = color[30] = uint8((color[24] + color[32]) / 2);
					}
				}

				if (map_data(X, Y).isFlat())
				{
					if (was_flat && bloc[i].tex_x == bloc[bmap(x - 1, y)].tex_x + 1 && is_clean && was_clean && map_data(X, Y).isFlat())
					{
						buf_i[index_size - 4] = GLushort(2 + buf_pos);
						buf_i[index_size - 2] = GLushort(8 + buf_pos);
						buf_i[index_size - 1] = GLushort(2 + buf_pos);
					}
					else
					{
						buf_i[index_size++] = GLushort(buf_pos);
						buf_i[index_size++] = GLushort(2 + buf_pos);
						buf_i[index_size++] = GLushort(6 + buf_pos);
						buf_i[index_size++] = GLushort(8 + buf_pos);
						buf_i[index_size++] = GLushort(2 + buf_pos);
						was_flat = map_data(X, Y).isFlat(); // If it's only lp_CONFIG->low_definition_map, it cannot be considered flat
					}
				}
				else
				{
					was_flat = false;
					buf_i[index_size++] = GLushort(buf_pos);
					buf_i[index_size++] = GLushort(1 + buf_pos);
					buf_i[index_size++] = GLushort(3 + buf_pos);
					buf_i[index_size++] = GLushort(4 + buf_pos);
					buf_i[index_size++] = GLushort(6 + buf_pos);
					buf_i[index_size++] = GLushort(7 + buf_pos);
					buf_i[index_size++] = GLushort(7 + buf_pos);
					buf_i[index_size++] = GLushort(8 + buf_pos);
					buf_i[index_size++] = GLushort(4 + buf_pos);
					buf_i[index_size++] = GLushort(5 + buf_pos);
					buf_i[index_size++] = GLushort(1 + buf_pos);
					buf_i[index_size++] = GLushort(2 + buf_pos);
				}
				was_clean = is_clean;
				T.x -= float(x * 16);
				memcpy(buf_t + (buf_pos * 2), bloc[i].texcoord, 72); // texture

				++buf_size;
			}
			if (buf_size > 0)
			{
				glDrawRangeElements(GL_TRIANGLE_STRIP, 0, buf_size * 9, index_size, GL_UNSIGNED_SHORT, buf_i); // dessine le tout
				was_flat = false;
				index_size = 0;
				buf_size = 0;
			}
		}
		glDisableClientState(GL_COLOR_ARRAY); // Colors (for fog of war)

		gfx->unlock();

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glEnable(GL_TEXTURE_2D);
		glClientActiveTextureARB(GL_TEXTURE0_ARB);

		glPopMatrix();
	}

	bool MAP::isInSightOf(byte player_mask, unsigned int x, unsigned int z) const
	{
		return (sight_map(x, z) & player_mask) != 0;
	}

	bool MAP::isDiscoveredBy(byte player_mask, unsigned int x, unsigned int z) const
	{
		return (view_map(x, z) & player_mask) != 0;
	}

	Vector3D MAP::hit(Vector3D Pos, Vector3D Dir, bool water, float length, bool allow_out) const // Calculates the intersection of a ray with the map (the ray starts from the top of the map)
	{
		if (Math::AlmostZero(Dir.x) && Math::AlmostZero(Dir.z)) // Trivial solution
		{
			Pos.y = get_unit_h(Pos.x, Pos.z);
			return Pos;
		}

		if (get_unit_h(Pos.x, Pos.z) > Pos.y) // Unhandled case
			return Pos;

		float step = 1.0f;
		if (!Math::AlmostZero(Dir.x) && !Math::AlmostZero(Dir.z))
		{
			if (fabsf(Dir.x) < fabsf(Dir.z))
				step = 1.0f / fabsf(Dir.x);
			else
				step = 1.0f / fabsf(Dir.z);
		}
		int nb = 0;
		int nb_limit = (int)(Pos.y) + 1000;
		const float dwm = (float)halfWidthInPixels;
		const float dhm = (float)halfHeightInPixels;
		Dir = (1.0f * step) * Dir;
		float len_step = Dir.length();
		while (((sealvl < Pos.y && water) || !water) && get_max_h((int)(Pos.x + (float)halfWidthInPixels) >> 3, (int)(Pos.z + (float)halfHeightInPixels) >> 3) < Pos.y)
		{
			if (nb >= nb_limit || length < 0.0f)
				return Pos;
			length -= len_step;
			nb++;
			Pos += Dir;
			if (!allow_out && (fabsf(Pos.x) > dwm || fabsf(Pos.z) > dhm)) // No result
				return Pos;
		}
		length += len_step;
		Pos -= Dir;

		while (((sealvl < Pos.y && water) || !water) && get_unit_h(Pos.x, Pos.z) < Pos.y)
		{
			if (nb >= nb_limit || length < 0.0f)
				return Pos;
			length -= len_step;
			++nb;
			Pos += Dir;
			if (!allow_out && (fabsf(Pos.x) > dwm || fabsf(Pos.z) > dhm)) // No result
				return Pos;
		}

		for (int i = 0; i < 7; ++i)
		{
			length += len_step;
			Pos -= Dir; // Repeat the last operation but with more precision
			Dir *= 0.5f;
			len_step *= 0.5f;
			nb = 0;
			while (((sealvl < Pos.y && water) || !water) && get_unit_h(Pos.x, Pos.z) < Pos.y)
			{
				if (nb >= 2 || length < 0.0f)
					return Pos;
				length -= len_step;
				++nb;
				Pos += Dir;
			}
		}
		return Pos; // Best approximate solution found
	}

	int MAP::check_metal(int x1, int y1, int unit_idx, int* stuff_id) const
	{
		if (unit_idx < 0 || unit_idx >= unit_manager.nb_unit)
			return 0;

		const int w = unit_manager.unit_type[unit_idx]->FootprintX;
		const int h = unit_manager.unit_type[unit_idx]->FootprintZ;
		int metal_base = 0;
		const int end_y = y1 + (h >> 1);
		const int end_x = x1 + (w >> 1);
		const int start_x = x1 - (w >> 1);
		for (int ry = y1 - (h >> 1); ry <= end_y; ++ry)
		{
			if (ry >= 0 && ry < heightInHeightmapTiles)
			{
				for (int rx = start_x; rx <= end_x; ++rx)
				{
					if (rx >= 0 && rx < widthInHeightmapTiles)
					{
						if (map_data(rx, ry).stuff >= 0)
						{
							features.lock();
							const int ID = map_data(rx, ry).stuff;
							if (ID >= 0) // We have to recheck this in case it has changed before locking
							{
								const int type = features.feature[ID].type;
								const Feature* const feature = feature_manager.getFeaturePointer(type);
								if (feature && !feature->reclaimable && !feature->blocking)
								{
									metal_base += feature->metal;
									if (stuff_id) // We need to know where to put metal extractors, so it'll give the impression the AI is clever :P
										*stuff_id = ID;
								}
							}
							features.unlock();
						}
					}
				}
			}
		}
		if (metal_base == 0)
			metal_base = ota_data.SurfaceMetal;
		return metal_base;
	}

	void MAP::drawCircleOnMap(const float x, const float y, const float radius, const uint32 color, const float thickness) const
	{
		const int steps = std::max(5, int(radius * 2.0f));
		const float f = 2.0f * (float)M_PI / (float)steps;
		std::vector<Vector3D> vertices0, vertices1;
		std::vector<uint32> colors0, colors1;
		vertices0.reserve(steps * 2 + 2);
		vertices1.reserve(steps * 2 + 2);
		colors0.reserve(steps * 2 + 2);
		colors1.reserve(steps * 2 + 2);
		const uint32 mask = makeacol(0xFF, 0xFF, 0xFF, 0x0);
		for (int i = 0; i <= steps; ++i)
		{
			const float _cos = std::cos((float)i * f);
			const float _sin = std::sin((float)i * f);
			const float tx = x + (radius + thickness) * _cos;
			const float tz = y + (radius + thickness) * _sin;
			const float ty = get_unit_h(tx, tz) + 1.0f;
			const float mx = x + radius * _cos;
			const float mz = y + radius * _sin;
			const float my = get_unit_h(mx, mz) + 1.0f;
			const float bx = x + (radius - thickness) * _cos;
			const float bz = y + (radius - thickness) * _sin;
			const float by = get_unit_h(bx, bz) + 1.0f;
			const Vector3D T(tx, ty, tz);
			const Vector3D M(mx, my, mz);
			const Vector3D B(bx, by, bz);

			vertices0.push_back(T);
			colors0.push_back(color & mask);
			vertices0.push_back(M);
			colors0.push_back(color);

			vertices1.push_back(M);
			colors1.push_back(color);
			vertices1.push_back(B);
			colors1.push_back(color & mask);
		}
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_CULL_FACE);
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		glVertexPointer(3, GL_FLOAT, 0, &(vertices0.front()));
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, &(colors0.front()));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)vertices0.size());

		glVertexPointer(3, GL_FLOAT, 0, &(vertices1.front()));
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, &(colors1.front()));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)vertices1.size());

		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	Point<int> MAP::worldToHeightmapIndex(const Vector2D& xzPosition) const
	{
		float x = xzPosition.x;
		float z = xzPosition.y;

		unsigned int newX = static_cast<unsigned int>((x + halfWidthInPixels) / HeightmapTileWidthInPixels);
		unsigned int newZ = static_cast<unsigned int>((z + halfHeightInPixels) / HeightmapTileHeightInPixels);

		return Point<int>(newX, newZ);
	}

	Point<int> MAP::worldToHeightmapIndex(const Vector3D& position) const
	{
		return worldToHeightmapIndex(Vector2D(position.x, position.z));
	}

	Vector2D MAP::heightmapIndexToWorld(const Point<int>& heightmapIndex) const
	{
		int x = (heightmapIndex.x * HeightmapTileWidthInPixels) + (HeightmapTileWidthInPixels / 2) - the_map->halfWidthInPixels;
		int y = (heightmapIndex.y * HeightmapTileHeightInPixels) + (HeightmapTileHeightInPixels / 2) - the_map->halfHeightInPixels;
		return Vector2D(x, y);
	}

	Point<int> MAP::worldToGraphicalTileIndex(const Vector2D& xzPosition) const
	{
		float x = xzPosition.x;
		float z = xzPosition.y;

		int newX = static_cast<int>((x + halfWidthInPixels) / GraphicalTileWidthInPixels);
		int newZ = static_cast<int>((z + halfHeightInPixels) / GraphicalTileHeightInPixels);

		return Point<int>(newX, newZ);
	}

	Vector2D MAP::worldToNormalizedMinimapCoordinates(const Vector2D& xzPosition) const
	{
		return Vector2D(
			(xzPosition.x + halfWidthInPixels) / widthInPixels,
			(xzPosition.y + halfHeightInPixels) / heightInPixels
		);
	}

	Vector2D MAP::worldToNormalizedMinimapCoordinates(const Vector3D& position) const
	{
		return worldToNormalizedMinimapCoordinates(Vector2D(position.x, position.z));
	}

	Point<int> MAP::worldToGraphicalTileIndex(const Vector3D& position) const
	{
		return worldToGraphicalTileIndex(Vector2D(position.x, position.z));
	}
} // namespace TA3D
