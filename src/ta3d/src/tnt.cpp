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

/*-----------------------------------------------------------------------------------\
  |                                         tnt.cp                                     |
  |  ce fichier contient les structures, classes et fonctions nécessaires à la lecture |
  | des fichiers tnt de total annihilation qui sont les fichiers contenant les cartes  |
  | du jeu.                                                                            |
  |                                                                                    |
  \-----------------------------------------------------------------------------------*/

#include "stdafx.h"
#include "misc/matrix.h"
#include "TA3D_NameSpace.h"
#include "ta3dbase.h"
#include "EngineClass.h"
#include "tdf.h"
#include "tnt.h"
#include "misc/ta3d_math.h"
#include "logs/logs.h"
#include <cmath>

namespace TA3D
{

	namespace
	{

		SDL_Surface* load_tnt_minimap_bmp(TNTMINIMAP* minimap, int* sw, int* sh)
		{
			// Copy the mini-map into an 8-bit SDL_Surface
			SDL_Surface* mini = gfx->create_surface_ex(8, TNTMINIMAP_WIDTH, TNTMINIMAP_HEIGHT);
			for (int y = 0; y < TNTMINIMAP_HEIGHT; ++y)
				memcpy((char*)mini->pixels + y * mini->pitch, minimap->map[y], TNTMINIMAP_WIDTH);

			// Apply the palette -- increase the color depth
			mini = convert_format(mini);

			// Examine the image for a blank-looking bottom or right edge
			int mini_w = TNTMINIMAP_WIDTH;
			int mini_h = TNTMINIMAP_HEIGHT;
			uint32 blank_color = makecol(120, 148, 252); // approximately
			uint32 mask = makecol(0xFC, 0xFC, 0xFC);	 // XXX this assumes 24- or 32-bit pixels
			do
			{
				--mini_w;
			} while (mini_w > 0 && ((SurfaceInt(mini, mini_w, 0) & mask) == blank_color || SurfaceInt(mini, mini_w, 0) == 0));
			do
			{
				--mini_h;
			} while (mini_h > 0 && ((SurfaceInt(mini, 0, mini_h) & mask) == blank_color || SurfaceInt(mini, 0, mini_h) == 0));
			mini_w++;
			mini_h++;

			if (sw)
				*sw = mini_w;
			if (sh)
				*sh = mini_h;

			return mini;
		}

		SDL_Surface* load_tnt_minimap_fast_raw_bmp(const String& filename, int& sw, int& sh)
		{
			File* headerBytes = VFS::Instance()->readFileRange(filename, 0, sizeof(TNTHEADER));
			if (headerBytes == NULL)
				return 0;

			TNTHEADER* header = &((TNTHEADER_U*)headerBytes->data())->header;

			File* minimapdata = VFS::Instance()->readFileRange(filename, header->PTRminimap, sizeof(TNTMINIMAP));
			if (!minimapdata)
			{
				delete headerBytes;
				return 0;
			}

			minimapdata->seek(header->PTRminimap);
			TNTMINIMAP minimap;
			*minimapdata >> minimap;
			SDL_Surface* bitmap = load_tnt_minimap_bmp(&minimap, &sw, &sh);

			delete headerBytes;
			delete minimapdata;

			return bitmap;
		}

	} // unnamed namespace

	struct MinimapInfo
	{
		int width;
		int height;
		TextureHandle texture;

		MinimapInfo(int width, int height, TextureHandle& texture): width(width), height(height), texture(std::move(texture)) {}
	};

	MinimapInfo loadMinimap(File* file)
	{
		int width;
		int height;
		*file >> width;
		*file >> height;

		SDL_Surface* minimapSurface = gfx->create_surface_ex(8, width, height);

		file->read(minimapSurface->pixels, width * height);
		minimapSurface = convert_format(minimapSurface);

		gfx->set_texture_format(gfx->defaultTextureFormat_RGB());
		TextureHandle texture(gfx, gfx->make_texture(minimapSurface, FILTER_LINEAR, true));

		SDL_FreeSurface(minimapSurface);

		return MinimapInfo(width, height, texture);
	}

	MAP* load_tnt_map(File* file)
	{
		LOG_DEBUG("MAP: creating MAP object ...");
		MAP* map = new MAP; // Create a new map

		the_map = map;

		LOG_DEBUG("MAP: initializing map data ...");

		map->init();
		map->tnt = true;
		TNTHEADER_U header_u; // Structure for the file header

		LOG_DEBUG("MAP: reading header");

		*file >> header_u;
		TNTHEADER& header = header_u.header; // Structure for the file header

		LOG_DEBUG("MAP: reading TDF table");
		int* TDF_index = new int[header.tileanims];

		int i, x, y;
		for (i = 0; i < header.tileanims; ++i) // Create the feature mapping table
		{
			file->seek(header.PTRtileanim + 4 + (i * 132));
			String fname = file->getString();
			TDF_index[i] = feature_manager.get_feature_index(fname);
			if (TDF_index[i] == -1)
				LOG_ERROR("tdf not found: " << fname);
		}

		map->sealvl = float(header.sealevel) * H_DIV;

		// Read the minimap
		LOG_DEBUG("MAP: reading mini map");
		int event_timer = MILLISECONDS_SINCE_INIT;
		file->seek(header.PTRminimap);
		auto minimapInfo = loadMinimap(file);
		map->mini_w = minimapInfo.width;
		map->mini_h = minimapInfo.height;
		map->glmini = std::move(minimapInfo.texture);
		LOG_INFO("minimap read in " << float(MILLISECONDS_SINCE_INIT - event_timer) * 0.001f << "s.");

		// Read the tile bitmaps
		LOG_DEBUG("MAP: reading blocs data");
		event_timer = MILLISECONDS_SINCE_INIT;

		// A "tile sheet" is a texture containing multiple tile bitmaps.
		// Sheets are 32 tiles wide and 1 tile high (1024x32)

		// Compute the number of tile sheets necessary to store all the tile bitmaps
		int tilesPerSheet = 32;
		int numberOfTileSheets = (header.tiles / tilesPerSheet) + ((header.tiles % tilesPerSheet == 0) ? 0 : 1);

		SDL_Surface** tileSheets = new SDL_Surface*[numberOfTileSheets];
		for (i = 0; i < numberOfTileSheets; ++i)
		{
			tileSheets[i] = gfx->create_surface_ex(8, tilesPerSheet * MAP::GraphicalTileWidthInPixels, MAP::GraphicalTileHeightInPixels);
		}

		// Read the tile bitmaps into the sheets
		file->seek(header.PTRtilegfx);
		for (i = 0; i < header.tiles; ++i)
		{
			int sheetNumber = i / tilesPerSheet;
			int offset = (i % tilesPerSheet) * MAP::GraphicalTileWidthInPixels;
			for (y = 0; y < MAP::GraphicalTileHeightInPixels; ++y)
			{
				int rowOffset = (y * tileSheets[sheetNumber]->pitch) + offset;
				file->read((byte*)tileSheets[sheetNumber]->pixels + rowOffset, MAP::GraphicalTileWidthInPixels);
			}
		}

		LOG_DEBUG("MAP: allocating map memory");
		map->widthInGraphicalTiles = header.Width / 2;
		map->heightInGraphicalTiles = header.Height / 2;
		map->widthInHeightmapTiles = map->widthInGraphicalTiles * 2;
		map->heightInHeightmapTiles = map->heightInGraphicalTiles * 2;

		// These are known to be wrong (they should be twice as big)
		// but other code depends on this for now.
		map->heightInWorldUnits = map->heightInGraphicalTiles * MAP::GraphicalTileHeightInWorldUnits;
		map->widthInWorldUnits = map->widthInGraphicalTiles * MAP::GraphicalTileWidthInWorldUnits;
		map->halfHeightInWorldUnits = map->heightInWorldUnits / 2;
		map->halfWidthInWorldUnits = map->widthInWorldUnits / 2;

		map->bmap.resize(map->widthInGraphicalTiles, map->heightInGraphicalTiles);
		map->view.resize(map->widthInGraphicalTiles, map->heightInGraphicalTiles);
		map->map_data.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);
		map->path.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);
		map->energy.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);
		map->slope.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);
		map->obstacles.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);
		map->obstacles.fill(false);

		LOG_DEBUG("MAP: creating FOW maps");
		map->sight_map.resize(map->widthInGraphicalTiles, map->heightInGraphicalTiles); // FOW maps
		map->view_map.resize(map->widthInGraphicalTiles, map->heightInGraphicalTiles);
		map->radar_map.resize(map->widthInGraphicalTiles, map->heightInGraphicalTiles);
		map->sonar_map.resize(map->widthInGraphicalTiles, map->heightInGraphicalTiles);
		map->view_map.fill(0);
		map->sight_map.fill(0);
		map->radar_map.fill(0);
		map->sonar_map.fill(0);

		LOG_DEBUG("MAP: allocating height maps");
		map->h_map.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);
		map->ph_map.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);

		LOG_DEBUG("MAP: initialising map data");
		map->path.fill(0);

		map->view.fill(0);
		map->nbbloc = header.tiles;		   // Nombre de blocs nécessaires
		map->bloc = new BLOC[map->nbbloc]; // Alloue la mémoire pour les blocs
		map->numberOfTileSheets = numberOfTileSheets;
		map->tileSheets = new GLuint[numberOfTileSheets]; // Tableau d'indices de texture OpenGl

		// Figure out which blocks are lava
		for (i = 0; i < map->nbbloc; i++)
		{
			int sheetNumber = i / tilesPerSheet;
			int offset = (i % tilesPerSheet) * MAP::GraphicalTileWidthInPixels;

			// Figure out if the block is lava
			// by looking at the colour(!!)
			// If the block is red enough we'll call it lava.
			int r = 0;
			int g = 0;
			int b = 0;
			for (y = 0; y < MAP::GraphicalTileHeightInPixels; ++y)
			{
				for (x = offset; x < offset + MAP::GraphicalTileWidthInPixels; ++x)
				{
					const int c = SurfaceByte(tileSheets[sheetNumber], x, y);
					r += pal[c].r;
					g += pal[c].g;
					b += pal[c].b;
				}
			}
			r /= 1024;
			g /= 1024;
			b /= 1024;
			map->bloc[i].lava = (r > 4 && g < (r / 4) && b < (r / 4));
		}

		LOG_INFO("Blocs read in " << float(MILLISECONDS_SINCE_INIT - event_timer) * 0.001f << "s.");
		event_timer = MILLISECONDS_SINCE_INIT;

		LOG_DEBUG("MAP: creating textures");

		gfx->set_texture_format(gfx->defaultTextureFormat_RGB());
		for (i = 0; i < numberOfTileSheets; ++i) // Finis de charger les textures et détruit les objets SDL_Surface
		{
			SDL_Surface* tmp = convert_format_24_copy(tileSheets[i]);
			map->tileSheets[i] = gfx->make_texture(tmp);
			SDL_FreeSurface(tmp);
		}
		LOG_INFO("Textures for blocks in " << float(MILLISECONDS_SINCE_INIT - event_timer) * 0.001f << "s.");

		event_timer = MILLISECONDS_SINCE_INIT;

		map->lvl = new Vector3D*[map->widthInGraphicalTiles * map->heightInGraphicalTiles];
		for (i = 0; i < map->widthInGraphicalTiles * map->heightInGraphicalTiles; ++i)
			map->lvl[i] = NULL;

		LOG_DEBUG("MAP: creating blocs texture coordinates");
		for (i = 0; i < map->nbbloc; ++i) // Crée les blocs
		{
			int sheetNumber = i / tilesPerSheet;
			float uOffset = (i % tilesPerSheet) / static_cast<float>(tilesPerSheet);

			map->bloc[i].tex = map->tileSheets[sheetNumber];

			float uvTileWidth = 1.0f / tilesPerSheet;

			float minU = uOffset;
			float maxU = uOffset + uvTileWidth;
			float minV = 0.0f;
			float maxV = 1.0f;


			map->bloc[i].texcoord[0] = minU;
			map->bloc[i].texcoord[1] = maxV;

			map->bloc[i].texcoord[2] = maxU;
			map->bloc[i].texcoord[3] = maxV;

			map->bloc[i].texcoord[4] = maxU;
			map->bloc[i].texcoord[5] = minV;

			map->bloc[i].texcoord[6] = minU;
			map->bloc[i].texcoord[7] = minV;
		}

		file->seek(header.PTRmapdata);
		file->read(map->bmap.getData(), map->bmap.getSize());
		for (y = 0; y < map->heightInGraphicalTiles; ++y)
		{
			for (x = 0; x < map->widthInGraphicalTiles; ++x)
			{
				if (map->bmap(x, y) >= map->nbbloc) // To add some security
					map->bmap(x, y) = 0;
			}
		}

		for (i = 0; i < numberOfTileSheets; ++i) // Delete SDL_Surface textures
			SDL_FreeSurface(tileSheets[i]);

		event_timer = MILLISECONDS_SINCE_INIT;

		gfx->set_texture_format(gfx->defaultTextureFormat_RGB());

		LOG_DEBUG("MAP: computing height data (step 1)");
		// Charge d'autres données sur les blocs
		map->water = false;
		file->seek(header.PTRmapattr);
		for (y = 0; y < map->heightInHeightmapTiles; ++y)
		{
			for (x = 0; x < map->widthInHeightmapTiles; ++x)
			{
				const int c = byte(file->getc());
				if (c < header.sealevel)
					map->water = true;
				map->h_map(x, y) = map->ph_map(x, y) = float(c) * H_DIV;
				file->getc();
				file->getc();
				file->getc();
			}
		}

		LOG_DEBUG("MAP: computing height data (step 2)");

		for (int y = 0; y < map->heightInHeightmapTiles; ++y) // Calcule les informations complémentaires sur la carte
		{
			for (int x = 0; x < map->widthInHeightmapTiles; ++x)
			{
				map->map_data(x, y).init();
				map->map_data(x, y).setUnderwater(map->h_map(x, y) < map->sealvl);
				map->map_data(x, y).setLava(map->bmap(x >> 1, y >> 1) < map->nbbloc ? map->bloc[map->bmap(x >> 1, y >> 1)].lava : false);
				if (!map->map_data(x, y).isLava() && (x >> 1) + 1 < map->widthInGraphicalTiles && map->bmap((x >> 1) + 1, y >> 1) < map->nbbloc)
				{
					map->map_data(x, y).setLava(map->bloc[map->bmap((x >> 1) + 1, y >> 1)].lava);
					if (!map->map_data(x, y).isLava() && (y >> 1) + 1 < map->heightInGraphicalTiles && map->bmap((x >> 1) + 1, (y >> 1) + 1) < map->nbbloc)
						map->map_data(x, y).setLava(map->bloc[map->bmap((x >> 1) + 1, (y >> 1) + 1)].lava);
					if (!map->map_data(x, y).isLava() && (y >> 1) - 1 >= 0 && map->bmap((x >> 1) + 1, (y >> 1) - 1) < map->nbbloc)
						map->map_data(x, y).setLava(map->bloc[map->bmap((x >> 1) + 1, (y >> 1) - 1)].lava);
				}
				if (!map->map_data(x, y).isLava() && (x >> 1) - 1 >= 0 && map->bmap((x >> 1) - 1, y >> 1) < map->nbbloc)
				{
					map->map_data(x, y).setLava(map->bloc[map->bmap((x >> 1) - 1, y >> 1)].lava);
					if (!map->map_data(x, y).isLava() && (y >> 1) + 1 < map->heightInGraphicalTiles && map->bmap((x >> 1) - 1, (y >> 1) + 1) < map->nbbloc)
						map->map_data(x, y).setLava(map->bloc[map->bmap((x >> 1) - 1, (y >> 1) + 1)].lava);
					if (!map->map_data(x, y).isLava() && (y >> 1) - 1 >= 0 && map->bmap((x >> 1) - 1, (y >> 1) - 1) < map->nbbloc)
						map->map_data(x, y).setLava(map->bloc[map->bmap((x >> 1) - 1, (y >> 1) - 1)].lava);
				}
				if (!map->map_data(x, y).isLava() && (y >> 1) + 1 < map->heightInGraphicalTiles && map->bmap(x >> 1, (y >> 1) + 1) < map->nbbloc)
					map->map_data(x, y).setLava(map->bloc[map->bmap(x >> 1, (y >> 1) + 1)].lava);
				if (!map->map_data(x, y).isLava() && (y >> 1) - 1 >= 0 && map->bmap(x >> 1, (y >> 1) - 1) < map->nbbloc)
					map->map_data(x, y).setLava(map->bloc[map->bmap(x >> 1, (y >> 1) - 1)].lava);
			}
		}

		LOG_INFO("Env created in " << float(MILLISECONDS_SINCE_INIT - event_timer) * 0.001f << "s.");
		event_timer = MILLISECONDS_SINCE_INIT;

		LOG_DEBUG("MAP: computing height data (step 3)");
		for (y = 0; y < map->heightInHeightmapTiles; ++y)
		{
			for (x = 0; x < map->widthInHeightmapTiles; ++x) // Projete la carte du relief
			{
				float h = map->ph_map(x, y);
				int rec_y = y + int(0.5f - tnt_transform * h / 16.f);
				for (int cur_y = rec_y + 1; cur_y <= y; cur_y++)
				{
					if (cur_y >= 0)
						map->ph_map(x, cur_y) = -1.f; // Valeur non spécifiée (un trou que l'on comblera plus tard)
				}
				if (rec_y >= 0)
					map->ph_map(x, rec_y) = h;
			}
		}

		LOG_DEBUG("MAP: computing height data (step 4)");
		for (y = 0; y < map->heightInHeightmapTiles; ++y)
		{
			for (x = 0; x < map->widthInHeightmapTiles; ++x) // Lisse la carte du relief projeté
			{
				if (!y && Math::AlmostEquals(map->ph_map(x, y), -1.f))
				{
					int cy = 0;
					while (Math::AlmostEquals(map->ph_map(x, cy), -1.f))
						++cy;
					const float h = map->ph_map(x, cy);
					cy = 0;
					while (Math::AlmostEquals(map->ph_map(x, cy), -1.f))
					{
						map->ph_map(x, cy) = h;
						++cy;
					}
				}
				else
				{
					if (Math::AlmostEquals(map->ph_map(x, y), -1.f))
					{
						float h1 = map->ph_map(x, y - 1);
						int cy = y;
						while (cy < map->heightInHeightmapTiles && Math::AlmostEquals(map->ph_map(x, cy), -1.f))
							++cy;

						if (cy >= map->heightInHeightmapTiles)
							cy = map->heightInHeightmapTiles - 1;

						float h2 = map->ph_map(x, cy);
						if (Math::AlmostEquals(h2, -1.f))
							h2 = h1;
						for (int ry = y; ry < cy; ++ry)
							map->ph_map(x, ry) = h1 + (h2 - h1) * float(ry - y + 1) / float(cy - y + 1);
					}
				}
			}
		}

		LOG_DEBUG("MAP: computing height data (step 5)");

		for (int y = 0; y < map->heightInHeightmapTiles; ++y) // Compute slopes on the map using height map and projected datas
		{
			for (int x = 0; x < map->widthInHeightmapTiles; ++x)
			{
				float dh = 0.0f;
				if (y > 0)
				{
					float dz = fabsf(map->get_zdec(x, y) - map->get_zdec(x, y - 1) + 8.0f);
					dz = (Math::AlmostZero(dz)) ? 100000000.f : (8.0f / dz);
					dh = Math::Max(dh, std::abs(map->h_map(x, y) - map->h_map(x, y - 1)) * dz);
				}

				if (y + 1 < map->heightInHeightmapTiles)
				{
					float dz = fabsf(map->get_zdec(x, y + 1) - map->get_zdec(x, y) + 8.0f);
					dz = (Math::AlmostZero(dz)) ? 100000000.f : (8.0f / dz);
					dh = Math::Max(dh, std::abs(map->h_map(x, y) - map->h_map(x, y + 1)) * dz);
				}

				if (x > 0)
					dh = Math::Max(dh, std::abs(map->h_map(x, y) - map->h_map(x - 1, y)));
				if (x + 1 < map->widthInHeightmapTiles)
					dh = Math::Max(dh, std::abs(map->h_map(x, y) - map->h_map(x + 1, y)));

				map->slope(x, y) = dh;
				map->energy(x, y) = dh;
			}
		}
		LOG_DEBUG("MAP: computing height data (step 6)");
		gaussianFilter(map->energy, 3.0f);

		LOG_INFO("relief : " << float(MILLISECONDS_SINCE_INIT - event_timer) * 0.001f << "s.");
		event_timer = MILLISECONDS_SINCE_INIT;

		LOG_DEBUG("MAP: reading map features data");
		// Ajoute divers éléments(végétation,...)
		file->seek(header.PTRmapattr + 1);
		for (y = 0; y < map->heightInHeightmapTiles; ++y)
			for (x = 0; x < map->widthInHeightmapTiles; ++x)
				map->map_data(x, y).stuff = -1;
		features.destroy();
		for (y = 0; y < map->heightInHeightmapTiles; ++y)
		{
			for (x = 0; x < map->widthInHeightmapTiles; ++x)
			{
				unsigned short type;
				*file >> type;
				if (type <= header.tileanims)
				{
					const Feature* const feature = feature_manager.getFeaturePointer(TDF_index[type]);
					auto xzPos = map->heightmapIndexToWorldCorner(x, y);
					xzPos.x += (feature->footprintx * MAP::HeightmapTileWidthInWorldUnits) / 2.0f;
					xzPos.y += (feature->footprintz * MAP::HeightmapTileHeightInWorldUnits) / 2.0f;
					float height = map->get_unit_h(xzPos.x, xzPos.y);

					// Quantize the height so sprite-based features are aligned with screen pixels.
					// 1 pixel is 0.5 world units, but height contributes half due to the projection,
					// so we quantize to whole world units.
					height = Math::truncateToInterval(height, 1.0f);

					Vector3D Pos(xzPos.x, height, xzPos.y);
					if (x + 1 < map->widthInHeightmapTiles && y + 1 < map->heightInHeightmapTiles)
					{
						map->map_data(x + 1, y + 1).stuff = features.add_feature(Pos, TDF_index[type]);
						features.drawFeatureOnMap(map->map_data(x + 1, y + 1).stuff); // Feature index is checked by drawFeatureOnMap so this is secure
						if (map->map_data(x + 1, y + 1).stuff >= 0 && feature->model) // Rotate only 3D models
						{
							if (feature_manager.getFeaturePointer(TDF_index[type])->category == "trees"		// Randomize trees angle for more realism
								|| feature_manager.getFeaturePointer(TDF_index[type])->category == "rocks") // Randomize rocks angle for more realism
								features.feature[map->map_data(x + 1, y + 1).stuff].angle = Math::randomFloat(0.0f, 360.0f);
						}
					}
				}
				// Read 2 more bytes
				*file >> type;
			}
		}
		LOG_INFO("Decors : " << float(MILLISECONDS_SINCE_INIT - event_timer) * 0.001f << "s.");

		LOG_DEBUG("MAP: freeing temporary allocated memory");

		DELETE_ARRAY(TDF_index);
		DELETE_ARRAY(tileSheets);

		return map;
	}

	GLuint load_tnt_minimap_fast(const String& filename, int& sw, int& sh) // Charge une minimap d'une carte contenue dans une archive HPI/UFO
	{
		SDL_Surface* bitmap = load_tnt_minimap_fast_raw_bmp(filename, sw, sh);

		if (bitmap == NULL)
			return 0;

		// Convert to a GL texture
		gfx->set_texture_format(gfx->defaultTextureFormat_RGB());
		GLuint texture = gfx->make_texture(bitmap, FILTER_LINEAR, true);

		SDL_FreeSurface(bitmap);
		return texture;
	}
} // namespace TA3D
