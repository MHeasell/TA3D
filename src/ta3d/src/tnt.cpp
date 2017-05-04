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

	MAP* load_tnt_map(File* file) // Charge une map au format TA, extraite d'une archive HPI/UFO
	{
		LOG_DEBUG("MAP: creating MAP object ...");
		MAP* map = new MAP; // Crée une nouvelle carte

		the_map = map;

		LOG_DEBUG("MAP: initializing map data ...");

		map->init();
		map->tnt = true;
		TNTHEADER_U header_u; // Structure pour l'en-tête du fichier

		LOG_DEBUG("MAP: reading header");

		*file >> header_u;
		TNTHEADER& header = header_u.header; // Structure pour l'en-tête du fichier

		LOG_DEBUG("MAP: reading TDF table");
		int* TDF_index = new int[header.tileanims];

		int i, x, y;
		for (i = 0; i < header.tileanims; ++i) // Crée le tableau pour la correspondance des éléments
		{
			file->seek(header.PTRtileanim + 4 + (i * 132));
			String fname = file->getString();
			TDF_index[i] = feature_manager.get_feature_index(fname);
			if (TDF_index[i] == -1)
				LOG_ERROR("tdf not found: " << fname);
		}

		map->sealvl = float(header.sealevel) * H_DIV;
		// Lit la minimap
		LOG_DEBUG("MAP: reading mini map");
		int event_timer = MILLISECONDS_SINCE_INIT;
		int w, h;
		file->seek(header.PTRminimap);
		*file >> w;
		*file >> h;
		map->mini_w = w;
		map->mini_h = h;
		map->mini = gfx->create_surface_ex(8, 252, 252);
		for (y = 0; y < 252; ++y)
			file->read((char*)map->mini->pixels + y * map->mini->pitch, 252);
		map->mini = convert_format(map->mini);
		map->mini_w = 251;
		map->mini_h = 251;
		uint32 mask = makecol(0xFC, 0xFC, 0xFC);
		while (map->mini_w > 0 && ((SurfaceInt(map->mini, map->mini_w, 0) & mask) == makecol(120, 148, 252) || SurfaceInt(map->mini, map->mini_w, 0) == 0))
			--(map->mini_w);
		while (map->mini_h > 0 && ((SurfaceInt(map->mini, 0, map->mini_h) & mask) == makecol(120, 148, 252) || SurfaceInt(map->mini, 0, map->mini_h) == 0))
			--(map->mini_h);
		++(map->mini_w);
		++(map->mini_h);
		gfx->set_texture_format(gfx->defaultTextureFormat_RGB());
		map->glmini = gfx->make_texture(map->mini, FILTER_LINEAR, true);

		LOG_INFO("minimap read in " << float(MILLISECONDS_SINCE_INIT - event_timer) * 0.001f << "s.");

		// Lit les différents morceaux
		LOG_DEBUG("MAP: reading blocs data");
		event_timer = MILLISECONDS_SINCE_INIT;
		int n_bmp = (header.tiles + 0x3F) >> 5; // Nombre de textures 1024x32 nécessaires pour mémoriser tout les morceaux
		SDL_Surface** bmp_tex = new SDL_Surface*[n_bmp];
		for (i = 0; i < n_bmp; ++i)
			bmp_tex[i] = gfx->create_surface_ex(8, 1024, 32);

		file->seek(header.PTRtilegfx);
		for (i = 0; i < header.tiles; ++i) // Lit tout les morceaux
		{
			int tex_num = i >> 5;	 // Numéro de la texture associée
			int tx = (i & 0x1F) << 5; // Coordonnées sur la texture
			for (y = 0; y < 32; ++y)  // Lit le morceau
				file->read((char*)bmp_tex[tex_num]->pixels + y * bmp_tex[tex_num]->pitch + tx, 32);
		}

		LOG_DEBUG("MAP: allocating map memory");
		map->widthInGraphicalTiles = header.Width / 2;
		map->heightInGraphicalTiles = header.Height / 2;
		map->widthInHeightmapTiles = map->widthInGraphicalTiles * 2;
		map->heightInHeightmapTiles = map->heightInGraphicalTiles * 2;

		// These are known to be wrong (they should be twice as big)
		// but other code depends on this for now.
		map->heightInPixels = map->heightInGraphicalTiles * 16;
		map->widthInPixels = map->widthInGraphicalTiles * 16;
		map->halfHeightInPixels = map->heightInGraphicalTiles * 8;
		map->halfWidthInPixels = map->widthInGraphicalTiles * 8;

		map->bmap.resize(map->widthInGraphicalTiles, map->heightInGraphicalTiles);
		map->view.resize(map->widthInGraphicalTiles, map->heightInGraphicalTiles);
		map->map_data.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);
		map->path.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);
		map->energy.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);
		map->slope.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);
		map->obstacles.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);
		map->obstacles.clear();

		LOG_DEBUG("MAP: creating FOW maps");
		map->sight_map.resize(map->widthInGraphicalTiles, map->heightInGraphicalTiles); // FOW maps
		map->view_map.resize(map->widthInGraphicalTiles, map->heightInGraphicalTiles);
		map->radar_map.resize(map->widthInGraphicalTiles, map->heightInGraphicalTiles);
		map->sonar_map.resize(map->widthInGraphicalTiles, map->heightInGraphicalTiles);
		map->view_map.clear(0);
		map->sight_map.clear(0);
		map->radar_map.clear(0);
		map->sonar_map.clear(0);

		LOG_DEBUG("MAP: allocating height maps");
		map->h_map.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);
		map->ph_map.resize(map->widthInHeightmapTiles, map->heightInHeightmapTiles);

		LOG_DEBUG("MAP: initialising map data");
		map->path.clear();

		map->view.clear(0);
		map->nbbloc = header.tiles;		   // Nombre de blocs nécessaires
		map->bloc = new BLOC[map->nbbloc]; // Alloue la mémoire pour les blocs
		map->ntex = short(n_bmp);
		map->tex = new GLuint[n_bmp]; // Tableau d'indices de texture OpenGl

		for (i = 0; i < map->nbbloc; i++) // Crée les blocs
		{
			map->bloc[i].init();
			const int tex_num = i >> 5;		// Numéro de la texture associée
			const int tx = (i & 0x1F) << 5; // Coordonnées sur la texture
			int r = 0, g = 0, b = 0;
			for (y = 0; y < 32; ++y)
			{
				for (x = tx; x < tx + 32; ++x)
				{
					const int c = SurfaceByte(bmp_tex[tex_num], x, y);
					r += pal[c].r;
					g += pal[c].g;
					b += pal[c].b;
				}
			}
			r >>= 10;
			g >>= 10;
			b >>= 10;
			map->bloc[i].lava = (r > 4 && g < (r >> 2) && b < (r >> 2));
			map->bloc[i].tex_x = byte(tx >> 5);
		}

		LOG_INFO("Blocs read in " << float(MILLISECONDS_SINCE_INIT - event_timer) * 0.001f << "s.");
		event_timer = MILLISECONDS_SINCE_INIT;

		LOG_DEBUG("MAP: creating textures");

		gfx->set_texture_format(gfx->defaultTextureFormat_RGB());
		for (i = 0; i < n_bmp; ++i) // Finis de charger les textures et détruit les objets SDL_Surface
		{
			SDL_Surface* tmp = convert_format_24_copy(bmp_tex[i]);
			map->tex[i] = gfx->make_texture(tmp);
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
			// Numéro de texture
			int t_n = i >> 5;
			// Position sur la texture
			float t_x = ((float)(i & 0x1F)) / 32.f;

			map->bloc[i].tex = map->tex[t_n];
			map->bloc[i].nbpoint = 9;
			map->bloc[i].nbindex = 12;
			map->bloc[i].texcoord = new float[map->bloc[i].nbpoint << 1];

			const float c = (1.f / 32.f) - (1.f / 1024.f);
			t_x += 1.f / 2048.f;
			map->bloc[i].texcoord[0] = t_x;
			map->bloc[i].texcoord[1] = 1.0f / 64.0f;
			map->bloc[i].texcoord[2] = t_x + c * 0.5f;
			map->bloc[i].texcoord[3] = 1.0f / 64.0f;
			map->bloc[i].texcoord[4] = t_x + c;
			map->bloc[i].texcoord[5] = 1.0f / 64.0f;
			map->bloc[i].texcoord[6] = t_x;
			map->bloc[i].texcoord[7] = 0.5f;
			map->bloc[i].texcoord[8] = t_x + c * 0.5f;
			map->bloc[i].texcoord[9] = 0.5f;
			map->bloc[i].texcoord[10] = t_x + c;
			map->bloc[i].texcoord[11] = 0.5f;
			map->bloc[i].texcoord[12] = t_x;
			map->bloc[i].texcoord[13] = 63.0f / 64.0f;
			map->bloc[i].texcoord[14] = t_x + c * 0.5f;
			map->bloc[i].texcoord[15] = 63.0f / 64.0f;
			map->bloc[i].texcoord[16] = t_x + c;
			map->bloc[i].texcoord[17] = 63.0f / 64.0f;
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

		for (i = 0; i < n_bmp; ++i) // Delete SDL_Surface textures
			SDL_FreeSurface(bmp_tex[i]);

		event_timer = MILLISECONDS_SINCE_INIT;

		gfx->set_texture_format(gfx->defaultTextureFormat_RGB());

		LOG_DEBUG("MAP: computing height data (step 1)");
		// Charge d'autres données sur les blocs
		map->water = false;
		file->seek(header.PTRmapattr);
		for (y = 0; y < (map->heightInGraphicalTiles << 1); ++y)
		{
			for (x = 0; x < (map->widthInGraphicalTiles << 1); ++x)
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
		for (y = 0; y < (map->heightInGraphicalTiles << 1); ++y)
		{
			for (x = 0; x < (map->widthInGraphicalTiles << 1); ++x) // Projete la carte du relief
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
		for (y = 0; y < (map->heightInGraphicalTiles << 1); ++y)
		{
			for (x = 0; x < (map->widthInGraphicalTiles << 1); ++x) // Lisse la carte du relief projeté
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
						while (cy < (map->heightInGraphicalTiles << 1) && Math::AlmostEquals(map->ph_map(x, cy), -1.f))
							++cy;

						if (cy >= (map->heightInGraphicalTiles << 1))
							cy = (map->heightInGraphicalTiles << 1) - 1;

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

		for (int y = 0; y < (map->heightInGraphicalTiles << 1); ++y) // Compute slopes on the map using height map and projected datas
		{
			for (int x = 0; x < (map->widthInGraphicalTiles << 1); ++x)
			{
				float dh = 0.0f;
				if (y > 0)
				{
					float dz = fabsf(map->get_zdec(x, y) - map->get_zdec(x, y - 1) + 8.0f);
					dz = (Math::AlmostZero(dz)) ? 100000000.f : (8.0f / dz);
					dh = Math::Max(dh, std::abs(map->h_map(x, y) - map->h_map(x, y - 1)) * dz);
				}

				if (y + 1 < (map->heightInGraphicalTiles << 1))
				{
					float dz = fabsf(map->get_zdec(x, y + 1) - map->get_zdec(x, y) + 8.0f);
					dz = (Math::AlmostZero(dz)) ? 100000000.f : (8.0f / dz);
					dh = Math::Max(dh, std::abs(map->h_map(x, y) - map->h_map(x, y + 1)) * dz);
				}

				if (x > 0)
					dh = Math::Max(dh, std::abs(map->h_map(x, y) - map->h_map(x - 1, y)));
				if (x + 1 < (map->widthInGraphicalTiles << 1))
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
		for (y = 0; y < (map->heightInGraphicalTiles << 1); ++y)
			for (x = 0; x < (map->widthInGraphicalTiles << 1); ++x)
				map->map_data(x, y).stuff = -1;
		features.destroy();
		for (y = 0; y < (map->heightInGraphicalTiles << 1); ++y)
		{
			for (x = 0; x < (map->widthInGraphicalTiles << 1); ++x)
			{
				unsigned short type;
				*file >> type;
				if (type <= header.tileanims)
				{
					Vector3D Pos;
					Pos.x = float((x << 3) - map->halfWidthInPixels) + 8.0f;
					Pos.z = float((y << 3) - map->halfHeightInPixels) + 8.0f;
					const Feature* const feature = feature_manager.getFeaturePointer(TDF_index[type]);
					if (feature && !feature->m3d)
						Pos.y = map->get_max_rect_h(x, y, feature->footprintx, feature->footprintz);
					else
						Pos.y = map->get_unit_h(Pos.x, Pos.z);
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
		DELETE_ARRAY(bmp_tex);

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
