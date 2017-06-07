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

/*----------------------------------------------------------\
  |                      EngineClass.h                        |
  |    Contient toutes les classes nécessaires au moteur 3D   |
  | et au moteur physique.                                    |
  |                                                           |
  \----------------------------------------------------------*/

#ifndef __TA3D_ENGINE_CL_H__
#define __TA3D_ENGINE_CL_H__

#include "tdf.h" // Pour le gestionnaire de sprites
#include "threads/thread.h"
#include "gfx/particles/particles.h"
#include "ai/ai.h"
#include "network/TA3D_Network.h"
#include "misc/camera.h"
#include <vector>
#include "misc/ta3d_math.h"
#include "misc/string.h"
#include "misc/grid.h"

#define H_DIV 0.5f

namespace TA3D
{

// tnt_transform = 1.0f / tanf(63.44f * DEG2RAD) / H_DIV;
#define tnt_transform 0.99977961f
// tnt_transform_H_DIV = 1.0f / tanf(63.44f * DEG2RAD);
#define tnt_transform_H_DIV 0.49988981f

	/**
	 * Structure holding information about the terrain
	 * (variations in altitude, underwater, amount of metal, etc.)
	 */
	class SECTOR
	{
	public:
		//! Indicates the graphical element present on this sector
		sint32 stuff;

		//! Index of the unit that is on this sector
		sint32 unit_idx;

		//! underwater, lava
		uint32 flags;	// underwater, lava

		//! Is the sector underwater?
		inline bool isUnderwater() const { return flags & 1U; }

		//! Is the sector lava? Used for pathfinding.
		inline bool isLava() const { return flags & 2U; }

		inline void setUnderwater(bool b) { flags = b ? (flags | 1U) : (flags & ~1U); }
		inline void setLava(bool b) { flags = b ? (flags | 2U) : (flags & ~2U); }

		void init();
	};
}

#include "ai/pathfinding.h" // Algorithme de pathfinding

// FIXME !

namespace TA3D
{

	class MAP_OTA
	{
	public:
		MAP_OTA() { init(); }
		~MAP_OTA() { destroy(); }

		void init();

		void destroy();

		void load(File* data);
		void load(const String& filename);

	public:
		String missionname;
		String planet;
		String missiondescription;
		String glamour;
		int tidalstrength;
		int solarstrength;
		bool lavaworld;
		short killmul;
		int minwindspeed;
		int maxwindspeed;
		float gravity;
		String numplayers;
		String map_size;
		int SurfaceMetal;
		int MohoMetal;
		int startX[TA3D_PLAYERS_HARD_LIMIT];
		int startZ[TA3D_PLAYERS_HARD_LIMIT];
		bool waterdoesdamage;
		int waterdamage;
		bool network;
		bool whitefog;

	}; // class MAP_OTA

	/**
	 * Blocks composing the map
	 */
	struct BLOC
	{
		//! Texture coordinates
		float texcoord[8];

		//! OpenGL texture handle
		GLuint tex;

		//! Is that a lava block?
		bool lava;
	};

	//! Map details
	class MAP : public ObjectSync
	{
	public:
		static const int GraphicalTileWidthInWorldUnits;
		static const int GraphicalTileHeightInWorldUnits;
		static const int HeightmapTileWidthInWorldUnits;
		static const int HeightmapTileHeightInWorldUnits;

		static const int GraphicalTileWidthInPixels;
		static const int GraphicalTileHeightInPixels;

		//! Indicates whether the texture is loaded and must be destroyed
		int numberOfTileSheets;

		//! Surface texture
		GLuint* tileSheets;

		//! Number of blocks
		int nbbloc;

		//! Blocks composing the terrain
		BLOC* bloc;

		// Table of indices to blocks
		Grid<uint16> bmap;

		// Table of terrain heights
		Grid<float> h_map;

		//! Table of the projected relief for the inverse computation (projection) during the display
		Grid<float> ph_map;

		//! Table of information about the terrain
		Grid<SECTOR> map_data;

		//!
		/**
		 * Indicates which plots of land are visible on the screen.
		 * The following values can be set:
		 *
		 * 1: in sight
		 * 2: not in sight, (under)water
		 * 3: not in sight, land
		 */
		Grid<byte> view;

		//! Table for pathfinding
		Grid<int> path;

		//! Maximum derivative of the height map
		Grid<float> slope;

		/**
		 * Energy of the map used by the pathfinder and units when following a path.
		 * One tile corresponds to one heightmap tile.
		 */
		Grid<float> energy;

		//! A map used by the pathfinder to detect blocking objects. One tile corresponds to one heightmap tile.
		Grid<bool> obstacles;

		//! Map of what has been discovered. One view map tile corresponds to one graphical tile.
		Grid<PlayerMask> view_map;

		//! Map of what tiles are in sight of each player. One sight tile corresponds to one graphical tile.
		Grid<PlayerMask> sight_map;

		//! Radar map. One radar tile corresponds to one graphical tile.
		Grid<PlayerMask> radar_map;

		//! Sonar map. One sonar tile corresponds to one graphical tile.
		Grid<PlayerMask> sonar_map;

		//! Width of the map in TA3D game units.
		int widthInWorldUnits;

		//! Height of the map in TA3D game units.
		int heightInWorldUnits;

		//! Half the width of the map in TA3D game units.
		int halfWidthInWorldUnits;

		//! Half the height of the map in TA3D game units.
		int halfHeightInWorldUnits;

		//! Width in 32-pixel graphical tiles
		int widthInGraphicalTiles;

		//! Height in 32-pixel graphical tiles
		int heightInGraphicalTiles;

		//! Width in 16-pixel heightmap tiles
		int widthInHeightmapTiles;

		// Height in 16-pixel heightmap tiles
		int heightInHeightmapTiles;

		//! Minimap
		SDL_Surface* mini;

		//! OpenGL texture for the minimap
		TextureHandle glmini;
		int mini_w;
		int mini_h;

		//! Sea level
		float sealvl;

		//! Block of floats for the relief of the map
		Vector3D** lvl;

		//! Indicates to draw the sea
		bool water;

		//! Indicates whether the map is in tnt format (Total Annihilation format)
		bool tnt;

		// Coordinates of the last drawn map window
		int previousX1, previousX2;
		int previousY1, previousY2;

		//! Data read from the ota file
		MAP_OTA ota_data;

		//! To handle wind
		float wind;
		float wind_dir;
		Vector3D wind_vec;

		uint8 fog_of_war;

		void clear_FOW(sint8 FOW_flags = -1);

		void init();

		MAP() { init(); }

		void destroy();

		//! Used to remove all objects when loading a saved game
		void clean_map();

		~MAP() { destroy(); }

		/**
		 * Converts the given XZ coordinates in world space
		 * to the corresponding cell in the heightmap grid that contains it.
		 *
		 * This method is valid on inputs that are not inside map boundaries,
		 * but the resulting index returned may be invalid,
		 * i.e. outside the grid.
		 * It is the caller's job to check this if necessary.
		 */
		Point<int> worldToHeightmapIndex(const Vector2D& xzPosition) const;

		Point<int> worldToHeightmapIndex(const Vector3D& position) const;

		Point<int> worldToHeightmapIndex(float x, float z) const;

		/**
		 * Converts the given XZ coordinates in world space
		 * to the corresponding cell in the heightmap grid
		 * whose top-left corner this position is closest to.
		 */
		Point<int> worldToNearestHeightmapCorner(float x, float z) const;

		/**
		 * Converts the given XZ coordinates in world space
		 * to the corresponding coordinate in heightmap space.
		 * This can (and probably will) be a fractional coordinate
		 * if the position is part-way through a tile.
		 *
		 * This method is valid on inputs that are not inside map boundaries,
		 * but the resulting coordinate may be outside the grid.
		 */
		Vector2D worldToHeightmapSpace(float x, float z) const;

		/**
		 * Returns the XZ centre of the given heightmap tile in world space.
		 */
		Vector2D heightmapIndexToWorld(const Point<int>& heightmapIndex) const;

		Vector2D heightmapIndexToWorld(int x, int y) const;

		/**
		 * Returns the XZ position of the top-left corner of the given heightmap tile in world space.
		 */
		Vector2D heightmapIndexToWorldCorner(const Point<int>& heightmapIndex) const;

		Vector2D heightmapIndexToWorldCorner(int x, int y) const;

		/**
		 * Returns the XZ position of the top-left corner of the given graphical tile in world space.
		 */
		Vector2D graphicalTileIndexToWorldCorner(const Point<int>& graphicalTileIndex) const;

		Vector2D graphicalTileIndexToWorldCorner(int x, int y) const;

		/**
		 * Converts the given XZ coordinates in world space
		 * to the corresponding cell in the graphical tile grid that contains it.
		 *
		 * This method is valid on inputs that are not inside map boundaries,
		 * but the resulting index returned may be invalid,
		 * i.e. outside the grid.
		 * It is the caller's job to check this if necessary.
		 */
		Point<int> worldToGraphicalTileIndex(const Vector2D& xzPosition) const;

		Point<int> worldToGraphicalTileIndex(const Vector3D& position) const;

		/**
		 * Converts a point in world (map pixel) coordinates
		 * into normalized minimap coordinates (between 0 and 1).
		 */
		Vector2D worldToNormalizedMinimapCoordinates(const Vector2D& xzPosition) const;

		Vector2D worldToNormalizedMinimapCoordinates(const Vector3D& position) const;

		/**
		 * Returns the centre point of the building footprint
		 * if you were to build the given type of unit at this location.
		 */
		Vector3D snapToBuildCenter(const Vector3D& position, int unitTypeId) const;

		Vector3D snapToBuildCenter(float x, float z, int unitTypeId) const;

		void drawCircleOnMap(const float x, const float y, const float radius, const uint32 color, const float thickness = 5.0f) const;

		/**
		 *
		 * @param player_id
		 * @param px X position on the heightmap grid
		 * @param py Y position on the heightmap grid
		 * @param r vision range in graphical tiles
		 * @param rd radar range in graphical tiles
		 * @param sn sonar range in graphical tiles
		 * @param rd_j radar jam range in graphical tiles
		 * @param sn_j sonar jam range in graphical tiles
		 * @param jamming
		 * @param black
		 */
		void update_player_visibility(PlayerId player_id, int px, int py, int r, int rd, int sn, int rd_j, int sn_j, bool jamming = false, bool black = false); // r -> sight, rd -> radar range, sn -> sonar range, j for jamming ray

		void draw_mini(int x1 = 0, int y1 = 0, int w = 252, int h = 252, Camera* cam = NULL, PlayerMask player_mask = 0xFF); // Dessine la mini-carte

		float get_unit_h(float x, float y) const;

		inline float get_h(const int x, const int y) const;

		float get_max_h(int x, int y) const;

		float get_max_h(Point<int> index) const;

		float get_max_rect_h(int x, int y, int w, int h) const;

		inline float get_zdec(const int x, const int y) const;

		inline int get_zdec_notest(const int x, const int y) const;

		inline float get_nh(const int x, const int y) const;

		void rect(int x1, int y1, int w, int h, int c, const String& yardmap = String(), bool open = false);
		void obstaclesRect(int x1, int y1, int w, int h, bool b, const String& yardmap = String(), bool open = false);

		bool check_rect(int x1, int y1, int w, int h, const int c) const;

		bool check_rect_discovered(int x1, int y1, int w, int h, const int c) const; // Check if the area has been fully discovered

		float check_rect_dh(int x1, int y1, int w, int h) const;

		float check_max_depth(int x1, int y1, int w, int h) const;

		float check_min_depth(int x1, int y1, int w, int h) const;

		bool check_vents(int x1, int y1, int w, int h, const String& yard_map) const;

		bool check_lava(int x1, int y1, int w, int h) const;

		int check_metal(int x1, int y1, int unit_idx, int* stuff_id = NULL) const;

		void draw_HD(Camera* cam, PlayerMask player_mask);
		void draw(Camera* cam, PlayerMask player_mask);

		//! Calculates the intersection of a ray with the map (the ray starts from the top of the map)
		Vector3D hit(Vector3D Pos, Vector3D Dir, bool water = true, float length = 200000.0f, bool allow_out = false) const;

		struct IntersectResult
		{
			bool hit;
			Vector3D v;
			IntersectResult(): hit(false) {}
			explicit IntersectResult(const Vector3D& v): hit(true), v(v) {}
		};

		IntersectResult hit2(Vector3D Pos, Vector3D Dir, bool water = true, float length = 200000.0f, bool allow_out = false) const;

		IntersectResult findIntersectWithTerrain(const Ray3D& ray) const;

		IntersectResult findIntersectWithHeightmapCell(const Vector3D& start, const Vector3D& end, int x, int y) const;

		/**
		 * Returns true if the graphical tile at the given index
		 * has been discovered by any players matching the given mask.
		 */
		bool isDiscoveredBy(PlayerMask player_mask, unsigned int x, unsigned int z) const;

		/**
		 * Returns true if the graphical tile at the given index
		 * is in line of sight of any players matching the given mask.
		 */
		bool isInSightOf(PlayerMask player_mask, unsigned int x, unsigned int z) const;
	};

	inline float MAP::get_nh(const int x, const int y) const { return ph_map(Math::Clamp(x, 0, widthInHeightmapTiles - 2), Math::Clamp(y, 0, heightInHeightmapTiles - 2)); }
	inline float MAP::get_zdec(const int x, const int y) const { return ph_map(Math::Clamp(x, 0, widthInHeightmapTiles - 2), Math::Clamp(y, 0, heightInHeightmapTiles - 2)) * tnt_transform_H_DIV; }
	inline int MAP::get_zdec_notest(const int x, const int y) const { return static_cast<int>(ph_map(x, y) * (0.125f * tnt_transform_H_DIV) + 0.5f); }
	inline float MAP::get_h(const int x, const int y) const { return h_map(Math::Clamp(x, 0, widthInHeightmapTiles - 2), Math::Clamp(y, 0, heightInHeightmapTiles - 2)); }

	extern MAP* the_map;
}

#endif // __TA3D_ENGINE_CL_H__
