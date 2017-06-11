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

	// One world unit is equivalent to two pixels on screen.
	const int MAP::GraphicalTileWidthInWorldUnits = 16;
	const int MAP::GraphicalTileHeightInWorldUnits = 16;
	const int MAP::HeightmapTileWidthInWorldUnits = 8;
	const int MAP::HeightmapTileHeightInWorldUnits = 8;

	// Texture size of the graphical tiles
	const int MAP::GraphicalTileWidthInPixels = 32;
	const int MAP::GraphicalTileHeightInPixels = 32;

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
		numberOfTileSheets = 0;
		tileSheets = NULL;
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
		previousX1 = previousX2 = previousY1 = previousY2 = 0;
	}

	float MAP::get_unit_h(float x, float y) const
	{
		if (isNaN(x) || isNaN(y))
			return 0.0f;

		// convert the coordinates
		auto heightSpacePosition = worldToHeightmapSpace(x, y);
		x = heightSpacePosition.x;
		y = heightSpacePosition.y;

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

	float MAP::get_max_h(Point<int> index) const
	{
		return get_max_h(index.x, index.y);
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

		if (numberOfTileSheets > 0)
		{
			for (int i = 0; i < numberOfTileSheets; i++)
				gfx->destroy_texture(tileSheets[i]);
			DELETE_ARRAY(tileSheets);
		}
		if (lvl)
		{
			for (int i = 0; i < heightInGraphicalTiles * widthInGraphicalTiles; i++)
				DELETE_ARRAY(lvl[i]);
			DELETE_ARRAY(lvl);
		}
		if (bloc && nbbloc > 0)
		{
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

	void MAP::draw_mini(int x1, int y1, int w, int h, Camera* cam, PlayerMask player_mask) // Draw the mini-map
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
			float projectedX = ((it->x + (widthInWorldUnits / 2.0f)) / widthInWorldUnits) * rw;
			float projectedY = ((it->z + (heightInWorldUnits / 2.0f)) / heightInWorldUnits) * rh;
			glVertex2f(projectedX, projectedY);
		}
		glEnd();
		glColor3ub(0xFF, 0xFF, 0xFF);
		glEnable(GL_TEXTURE_2D);
	}

	void MAP::update_player_visibility(PlayerId player_id, int px, int py, int r, int rd, int sn, int rd_j, int sn_j, bool jamming, bool black)
	{
		gfx->lock();

		// convert from heightmap tile index to graphical tile index
		px /= 2;
		py /= 2;

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
			const PlayerMask mask = toPlayerMask(player_id);
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

	void MAP::draw(Camera* cam, PlayerMask player_mask)
	{
		cam->applyToOpenGl();

		draw_HD(cam, player_mask);
	}

	void MAP::draw_HD(Camera* cam, PlayerMask player_mask)
	{
		glPushMatrix();

		gfx->lock();

		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);

		if (numberOfTileSheets > 0)
			gfx->ReInitAllTex(true);
		else
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		// ------------------------------------------------------------------
		// Beginning of visible area calculations
		// ------------------------------------------------------------------

		Vector3D cameraExtents(cam->viewportWidth() / 2.0f, 0.0f, cam->viewportHeight() / 2.0f);
		auto topLeft = worldToGraphicalTileIndex(cam->position() - cameraExtents);
		auto bottomRight = worldToGraphicalTileIndex(cam->position() + cameraExtents);
		int x1 = Math::Clamp(topLeft.x, 0, widthInGraphicalTiles - 2);
		int y1 = Math::Clamp(topLeft.y, 0, heightInGraphicalTiles - 2);
		int x2 = Math::Clamp(bottomRight.x, 0, widthInGraphicalTiles - 2);
		int y2 = Math::Clamp(bottomRight.y, 0, heightInGraphicalTiles - 2);

		// ------------------------------------------------------------------
		// End of visible area calculations
		// ------------------------------------------------------------------

		glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);

		if (numberOfTileSheets > 0)
		{
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glEnable(GL_TEXTURE_2D);
			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		for (int y = previousY1; y <= previousY2; ++y)
			memset(&(view(previousX1, y)), 0, previousX2 - previousX1 + 1);
		features.list.clear();
		previousX1 = x1;
		previousX2 = x2;
		previousY1 = y1;
		previousY2 = y2;

		for (int y = y1; y <= y2; ++y) // Scans blocks that can be seen to draw those that are visible
		{
			const int Y = y * 2;

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

				// Draw the tile quad
				const int tileIndex = bmap(x, y);

				auto tilePosition = graphicalTileIndexToWorldCorner(x, y);
				float tileX = tilePosition.x;
				float tileY = tilePosition.y;

				glBindTexture(GL_TEXTURE_2D, bloc[tileIndex].tex);

				glBegin(GL_QUADS);

				// bottom-left
				if (fog_of_war == FOW_DISABLED)
				{
					glColor3f(1.0f, 1.0f, 1.0f);
				}
				else if (!isDiscoveredBy(player_mask, x, y + 1))
				{
					glColor3f(0.0f, 0.0f, 0.0f);
				}
				else if (!isInSightOf(player_mask, x, y + 1))
				{
					glColor3f(0.5f, 0.5f, 0.5f);
				}
				else
				{
					glColor3f(1.0f, 1.0f, 1.0f);
				}

				glTexCoord2f(bloc[tileIndex].texcoord[0], bloc[tileIndex].texcoord[1]);
				glVertex3f(tileX, 0.0f, tileY + GraphicalTileHeightInWorldUnits);

				// bottom-right
				if (fog_of_war == FOW_DISABLED)
				{
					glColor3f(1.0f, 1.0f, 1.0f);
				}
				else if (!isDiscoveredBy(player_mask, x + 1, y + 1))
				{
					glColor3f(0.0f, 0.0f, 0.0f);
				}
				else if (!isInSightOf(player_mask, x + 1, y + 1))
				{
					glColor3f(0.5f, 0.5f, 0.5f);
				}
				else
				{
					glColor3f(1.0f, 1.0f, 1.0f);
				}

				glTexCoord2f(bloc[tileIndex].texcoord[2], bloc[tileIndex].texcoord[3]);
				glVertex3f(tileX + GraphicalTileWidthInWorldUnits, 0.0f, tileY + GraphicalTileHeightInWorldUnits);

				// top-right
				if (fog_of_war == FOW_DISABLED)
				{
					glColor3f(1.0f, 1.0f, 1.0f);
				}
				else if (!isDiscoveredBy(player_mask, x + 1, y))
				{
					glColor3f(0.0f, 0.0f, 0.0f);
				}
				else if (!isInSightOf(player_mask, x + 1, y))
				{
					glColor3f(0.5f, 0.5f, 0.5f);
				}
				else
				{
					glColor3f(1.0f, 1.0f, 1.0f);
				}

				glTexCoord2f(bloc[tileIndex].texcoord[4], bloc[tileIndex].texcoord[5]);
				glVertex3f(tileX + GraphicalTileWidthInWorldUnits, 0.0f, tileY);

				// top-left
				if (fog_of_war == FOW_DISABLED)
				{
					glColor3f(1.0f, 1.0f, 1.0f);
				}
				else if (!isDiscoveredBy(player_mask, x, y))
				{
					glColor3f(0.0f, 0.0f, 0.0f);
				}
				else if (!isInSightOf(player_mask, x, y))
				{
					glColor3f(0.5f, 0.5f, 0.5f);
				}
				else
				{
					glColor3f(1.0f, 1.0f, 1.0f);
				}

				glTexCoord2f(bloc[tileIndex].texcoord[6], bloc[tileIndex].texcoord[7]);
				glVertex3f(tileX, 0.0f, tileY);

				glEnd();
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

	bool MAP::isInSightOf(PlayerMask player_mask, unsigned int x, unsigned int z) const
	{
		return (sight_map(x, z) & player_mask) != 0;
	}

	bool MAP::isDiscoveredBy(PlayerMask player_mask, unsigned int x, unsigned int z) const
	{
		return (view_map(x, z) & player_mask) != 0;
	}

	boost::optional<Vector3D> MAP::findIntersectWithTerrain(const Ray3D& ray) const
	{
		// * find the point where the ray enters the map bounding box

		BoundingBox3D mapBoundingBox(
			Vector3D(0.0f, 63.75f, 0.0f),
			Vector3D(halfWidthInWorldUnits, 63.75f, halfHeightInWorldUnits));

		auto intersect = mapBoundingBox.intersect(ray);
		if (!intersect)
		{
			return boost::none;
		}

		auto startPoint = ray.pointAt(intersect->enter);
		auto endPoint = ray.pointAt(intersect->exit);

		return findIntersectWithTerrainLine(startPoint, endPoint);
	}

	boost::optional<Vector3D> MAP::findIntersectWithTerrainLine(const Vector3D& startPoint, const Vector3D& endPoint) const
	{
		auto startCell = worldToHeightmapIndex(startPoint);

		Ray3D ray(startPoint, endPoint - startPoint);

		int xDirection = ray.direction.x > 0 ? 1 : -1;
		int zDirection = ray.direction.z > 1 ? 1 : -1;
		float xPlaneOffset = (HeightmapTileWidthInWorldUnits / 2.0f) * xDirection;
		float zPlaneOffset = (HeightmapTileHeightInWorldUnits / 2.0f) * zDirection;

		while (true)
		{
			if (startCell.x >= 0
				&& startCell.y >= 0
				&& startCell.x < widthInHeightmapTiles - 1
				&& startCell.y < heightInHeightmapTiles - 1)
			{
				auto cellIntersect = findIntersectWithHeightmapCell(startPoint, endPoint, startCell.x, startCell.y);
				if (cellIntersect)
				{
					return *cellIntersect;
				}
			}

			auto cellPos = heightmapIndexToWorld(startCell);

			Plane3D xPlane(Vector3D(cellPos.x + xPlaneOffset, 0.0f, 0.0f), Vector3D(1.0f, 0.0f, 0.0f));
			float xIntersect = xPlane.intersect(ray).get_value_or(std::numeric_limits<float>::infinity());

			Plane3D zPlane(Vector3D(0.0f, 0.0f, cellPos.y + zPlaneOffset), Vector3D(0.0f, 0.0f, 1.0f));
			float zIntersect = zPlane.intersect(ray).get_value_or(std::numeric_limits<float>::infinity());

			if (xIntersect < zIntersect)
			{
				if (xIntersect > 1.0f)
				{
					return boost::none;
				}

				startCell.x += xDirection;
			}
			else
			{
				if (zIntersect > 1.0f)
				{
					return boost::none;
				}

				startCell.y += zDirection;
			}
		}
	}

	boost::optional<Vector3D> MAP::findIntersectWithHeightmapCell(const Vector3D& start, const Vector3D& end, int x, int y) const
	{
		float topLeftHeight = h_map(x, y);
		Vector2D xzPosTopLeft = heightmapIndexToWorldCorner(x, y);
		Vector3D posTopLeft(xzPosTopLeft.x, topLeftHeight, xzPosTopLeft.y);

		float topRightHeight = h_map(x + 1, y);
		Vector2D xzPosTopRight = heightmapIndexToWorldCorner(x + 1, y);
		Vector3D posTopRight(xzPosTopRight.x, topRightHeight, xzPosTopRight.y);

		float bottomLeftHeight = h_map(x, y + 1);
		Vector2D xzPosBottomLeft = heightmapIndexToWorldCorner(x, y + 1);
		Vector3D posBottomLeft(xzPosBottomLeft.x, bottomLeftHeight, xzPosBottomLeft.y);

		float bottomRightHeight = h_map(x + 1, y + 1);
		Vector2D xzPosBottomRight = heightmapIndexToWorldCorner(x + 1, y + 1);
		Vector3D posBottomRight(xzPosBottomRight.x, bottomRightHeight, xzPosBottomRight.y);

		float midHeight = (topLeftHeight + topRightHeight + bottomLeftHeight + bottomRightHeight) / 4.0f;
		Vector2D xzPosMiddle = heightmapIndexToWorld(x, y);
		Vector3D posMiddle(xzPosMiddle.x, midHeight, xzPosMiddle.y);

		Triangle3D left, bottom, right, top;

		// For robust collision testing under floating point arithmetic,
		// we ensure that the direction of any edge shared by two triangles
		// is the same in both triangles.
		// When this is true, the intersectLine intersection test
		// guarantees that a line passing exactly through the edge
		// will intersect at least one of the triangles.
		if (std::abs(y - x) % 2 == 0)  // checkerboard pattern
		{
			left = Triangle3D(posTopLeft, posMiddle, posBottomLeft);
			bottom = Triangle3D(posBottomLeft, posBottomRight, posMiddle);
			right = Triangle3D(posBottomRight, posMiddle, posTopRight);
			top = Triangle3D(posTopRight, posTopLeft, posMiddle);
		}
		else
		{
			left = Triangle3D(posTopLeft, posBottomLeft, posMiddle);
			bottom = Triangle3D(posBottomLeft, posMiddle, posBottomRight);
			right = Triangle3D(posBottomRight, posTopRight, posMiddle);
			top = Triangle3D(posTopRight, posMiddle, posTopLeft);
		}

		auto result = left.intersectLine(start, end);
		result = closestTo(start, result, bottom.intersectLine(start, end));
		result = closestTo(start, result, right.intersectLine(start, end));
		result = closestTo(start, result, top.intersectLine(start, end));

		return result;
	}

	boost::optional<Vector3D> MAP::hitLine(const Vector3D& start, const Vector3D& end, bool water) const
	{
		auto result = findIntersectWithTerrainLine(start, end);

		if (water)
		{
			Plane3D waterPlane(Vector3D(0.0f, sealvl, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));
			auto waterResult = waterPlane.intersectLine(start, end);
			result = closestTo(start, result, waterResult);
		}

		return result;
	}

	boost::optional<Vector3D> MAP::hit2(const Vector3D& Pos, const Vector3D& Dir, bool water) const
	{
		Ray3D ray(Pos, Dir);
		auto result = findIntersectWithTerrain(ray);

		if (water)
		{
			Plane3D waterPlane(Vector3D(0.0f, sealvl, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));
			auto waterResult = waterPlane.intersect(ray);
			if (waterResult)
			{
				auto waterPoint = ray.pointAt(*waterResult);

				// accept whichever point is the least distance along the ray
				if (!result || ray.isLessFar(waterPoint, *result))
				{
					result = waterPoint;
				}
			}
		}

		return result;
	}

	Vector3D MAP::hit(Vector3D Pos, Vector3D Dir, bool water, float length, bool allow_out) const
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
		const float dwm = (float)halfWidthInWorldUnits;
		const float dhm = (float)halfHeightInWorldUnits;
		Dir = (1.0f * step) * Dir;
		float len_step = Dir.length();
		while (((sealvl < Pos.y && water) || !water) && get_max_h(worldToHeightmapIndex(Pos)) < Pos.y)
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

		int newX = static_cast<int>((x + halfWidthInWorldUnits) / HeightmapTileWidthInWorldUnits);
		int newZ = static_cast<int>((z + halfHeightInWorldUnits) / HeightmapTileHeightInWorldUnits);

		return Point<int>(newX, newZ);
	}

	Point<int> MAP::worldToHeightmapIndex(const Vector3D& position) const
	{
		return worldToHeightmapIndex(Vector2D(position.x, position.z));
	}

	Point<int> MAP::worldToHeightmapIndex(float x, float z) const
	{
		return worldToHeightmapIndex(Vector2D(x, z));
	}

	Vector2D MAP::worldToHeightmapSpace(float x, float z) const
	{
		return Vector2D(
			(x + halfWidthInWorldUnits) / HeightmapTileWidthInWorldUnits,
			(z + halfHeightInWorldUnits) / HeightmapTileHeightInWorldUnits);
	}

	Vector2D MAP::heightmapIndexToWorld(const Point<int>& heightmapIndex) const
	{
		return heightmapIndexToWorld(heightmapIndex.x, heightmapIndex.y);
	}

	Vector2D MAP::heightmapIndexToWorld(int x, int y) const
	{
		int newX = (x * HeightmapTileWidthInWorldUnits) + (HeightmapTileWidthInWorldUnits / 2) - the_map->halfWidthInWorldUnits;
		int newY = (y * HeightmapTileHeightInWorldUnits) + (HeightmapTileHeightInWorldUnits / 2) - the_map->halfHeightInWorldUnits;
		return Vector2D(newX, newY);
	}

	Vector2D MAP::heightmapIndexToWorldCorner(const Point<int>& heightmapIndex) const
	{
		int x = (heightmapIndex.x * HeightmapTileWidthInWorldUnits) - the_map->halfWidthInWorldUnits;
		int y = (heightmapIndex.y * HeightmapTileHeightInWorldUnits) - the_map->halfHeightInWorldUnits;
		return Vector2D(x, y);
	}

	Vector2D MAP::heightmapIndexToWorldCorner(int x, int y) const
	{
		return heightmapIndexToWorldCorner(Point<int>(x, y));
	}

	Vector2D MAP::graphicalTileIndexToWorldCorner(const Point<int>& graphicalTileIndex) const
	{
		int x = (graphicalTileIndex.x * GraphicalTileWidthInWorldUnits) - the_map->halfWidthInWorldUnits;
		int y = (graphicalTileIndex.y * GraphicalTileHeightInWorldUnits) - the_map->halfHeightInWorldUnits;
		return Vector2D(x, y);
	}

	Vector2D MAP::graphicalTileIndexToWorldCorner(int x, int y) const
	{
		return graphicalTileIndexToWorldCorner(Point<int>(x, y));
	}

	Point<int> MAP::worldToGraphicalTileIndex(const Vector2D& xzPosition) const
	{
		float x = xzPosition.x;
		float z = xzPosition.y;

		int newX = static_cast<int>((x + halfWidthInWorldUnits) / GraphicalTileWidthInWorldUnits);
		int newZ = static_cast<int>((z + halfHeightInWorldUnits) / GraphicalTileHeightInWorldUnits);

		return Point<int>(newX, newZ);
	}

	Vector2D MAP::worldToNormalizedMinimapCoordinates(const Vector2D& xzPosition) const
	{
		return Vector2D(
			(xzPosition.x + halfWidthInWorldUnits) / widthInWorldUnits,
			(xzPosition.y + halfHeightInWorldUnits) / heightInWorldUnits
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

	Vector3D MAP::snapToBuildCenter(const Vector3D& position, int unitTypeId) const
	{
		return snapToBuildCenter(position.x, position.z, unitTypeId);
	}

	Vector3D MAP::snapToBuildCenter(float x, float z, int unitTypeId) const
	{
		auto unitType = unit_manager.unit_type[unitTypeId];

		float halfFootprintX = (unitType->FootprintX * HeightmapTileWidthInWorldUnits) / 2.0f;
		float halfFootprintZ = (unitType->FootprintZ * HeightmapTileHeightInWorldUnits) / 2.0f;

		x -= halfFootprintX;
		z -= halfFootprintZ;

		auto heightIndex = worldToNearestHeightmapCorner(x, z);

		float buildingHeight = get_max_rect_h(heightIndex.x, heightIndex.y, unitType->FootprintX, unitType->FootprintZ);
		if (unitType->floatting())
		{
			float floatingHeight = sealvl - (unitType->WaterLine / 2.0f);
			buildingHeight = Math::Max(buildingHeight, floatingHeight);
		}

		auto snappedPosition = heightmapIndexToWorldCorner(heightIndex);

		return Vector3D(snappedPosition.x + halfFootprintX, buildingHeight, snappedPosition.y + halfFootprintZ);
	}

	Point<int> MAP::worldToNearestHeightmapCorner(float x, float z) const
	{
		x += HeightmapTileWidthInWorldUnits / 2.0f;
		z += HeightmapTileHeightInWorldUnits / 2.0f;
		auto heightIndex = worldToHeightmapIndex(x, z);
		return heightIndex;
	}
} // namespace TA3D
