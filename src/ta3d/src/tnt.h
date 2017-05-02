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
|                                         tnt.h                                      |
|  ce fichier contient les structures, classes et fonctions nécessaires à la lecture |
| des fichiers tnt de total annihilation qui sont les fichiers contenant les cartes  |
| du jeu.                                                                            |
|                                                                                    |
\-----------------------------------------------------------------------------------*/

#ifndef __TA3D_TNT_H__
#define __TA3D_TNT_H__

#include "misc/string.h"
#include "EngineClass.h"

namespace TA3D
{

	/**
	 * Structure of the TNT file header
	 */
	struct TNTHEADER
	{
        /**
         * version/file type marker.
         * It is always 0x2000 in valid TNT files.
         */
		int IDversion;

		/**
		 * The width of the map in 16-pixel units as used for feature/height information.
		 * To get the width in 32-pixel units (the size of a tile), divide by 2.
		 */
		int Width;

		/**
		 * The height of the map in 16-pixel units as used for feature/height information.
		 * To get the height in 32-pixel units (the size of a tile), divide by 2.
		 */
		int Height;

		/**
		 * Offset from the beginning of the file to the tile indices array.
		 * This array contains unsigned shorts
		 * that are the indices in the tile array
		 * of the 32x32 pixel tile to be displayed at that point.
		 */
		int PTRmapdata;

		/**
         * Offset from the beginning of the file to the map attributes array.
         * This array holds the height and feature information
         * for every 16x16 pixel square on the map.
         *
         * NB: There are 4 of these squares for each square
         * in the tile indices array.
		 */
		int PTRmapattr;

		/**
         * This is the array of tiles referenced by the tile indices array.
         * One tile consists of unsigned char[32*32].
		 */
		int PTRtilegfx;

		/**
		 * The length of the array of tiles, i.e. the number of tiles.
		 */
		int tiles;

		/**
		 * The length of the array of tile animations (features).
         * i.e. the number of features.
		 */
		int tileanims;

		/**
         * This is the array of features referenced by the feature indices array.
         * Each entry is 0x84 bytes long and consists of an unsigned long
         * that always seems to equal the index of the feature in this array
         * and a 128-character null-terminated string
         * that contains the name of the feature to be placed in the location
         * as found between the square brackets "[]" in a TDF feature file.
         *
         * NB: The index seems to be totally useless.
         * I have noticed that while Cavedog maps include them,
         * Annihilator doesn't even bother to include them.
         */
		int PTRtileanim;

		/**
         * Any value in the heights array less than this value
         * is deemed to be "under water"
         * (i.e. water modifiers and effects apply).
         */
		int sealevel;

		/**
         * Obviously enough, this is where the minimap image is found.
         * At the offset, the first things you find are 2 unsigned long's:
         * the minimap's width and height (which always seem to be 252).
         * What follows is width*height of pixel data.
         * The difficult part about minimaps
         * is that they are padded by 0xDD's
         * (the standard blue used as transparent in TA).
         */
		int PTRminimap;

		/**
         * No-one knows what this is used for.
         * It may just be here for alignment.
         */
		int unknown1;

		/**
         * No-one knows what this is used for.
         * It may just be here for alignment.
         */
		int pad1;

		/**
         * No-one knows what this is used for.
         * It may just be here for alignment.
         */
		int pad2;

		/**
         * No-one knows what this is used for.
         * It may just be here for alignment.
         */
		int pad3;

		/**
         * No-one knows what this is used for.
         * It may just be here for alignment.
         */
		int pad4;
	};
	union TNTHEADER_U // For strict-aliasing safety
	{
		byte bytes[sizeof(TNTHEADER)];
		TNTHEADER header;
	};

#define TNTMINIMAP_WIDTH 252
#define TNTMINIMAP_HEIGHT 252

	struct TNTMINIMAP
	{
		//! The width of the minimap in pixels
		int w;

		//! The height of the minimap in pixels
		int h;

		/**
		 * The minimap data. Each byte is one pixel
		 * and the data is an index into the TA color pallete.
		 */
		byte map[TNTMINIMAP_HEIGHT][TNTMINIMAP_WIDTH];
	};

	// Load a map in TNT format extracted from a HPI archive
	MAP* load_tnt_map(File* file);

	// Load a minimap from a map file extracted from a HPI archive
	GLuint load_tnt_minimap_fast(const String& filename, int& sw, int& sh);

} // namespace TA3D

#endif // __TA3D_TNT_H__
