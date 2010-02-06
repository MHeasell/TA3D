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
|                                          3dm.h                                     |
|  ce fichier contient les structures, classes et fonctions nécessaires à la lecture |
| des fichiers 3dm de TA3D qui sont les fichiers contenant les modèles 3D des objets |
| du jeu.                                                                            |
|                                                                                    |
\-----------------------------------------------------------------------------------*/

#ifndef __CLASSE_3DM
#define __CLASSE_3DM

# include <stdafx.h>
# include <misc/string.h>
# include <misc/hash_table.h>
# include <ta3dbase.h>
# include <gaf.h>
# include <vector>
# include <list>
# include <misc/matrix.h>
# include <gfx/glfunc.h>
# include <gfx/shader.h>
# include <scripts/script.data.h>
# include "mesh.h"

namespace TA3D
{

#define SURFACE_ADVANCED                0x01            // Tell it is not a 3Do surface
#define SURFACE_REFLEC                  0x02            // Reflection
#define SURFACE_LIGHTED                 0x04            // Lighting
#define SURFACE_TEXTURED                0x08            // Texturing
#define SURFACE_GOURAUD                 0x10            // Gouraud shading
#define SURFACE_BLENDED                 0x20            // Alpha Blending
#define SURFACE_PLAYER_COLOR            0x40            // The color is the owner's color
#define SURFACE_GLSL                    0x80            // Use a shader to create a surface effect
#define SURFACE_ROOT_TEXTURE            0X100           // Use only the textures of the root object (all objects share the same texture set)

    class MESH_3DM : public MESH        // Classe pour la gestion des (sous-)objets des modèles 3do
    {
    public:
        uint32  Color;
        uint32  RColor;
        uint32  Flag;
        String  frag_shader_src;
        String  vert_shader_src;
        Shader  s_shader;
        GLuint  glColorTexture;     // This is a small texture filled with color Color (just to prevent rendering color from being changed)
        MESH_3DM *root;
    public:

		void load(File *file, const String &filename, MESH_3DM *root = NULL);

		virtual bool draw(float t, AnimationData *data_s = NULL, bool sel_primitive = false, bool alset = false, bool notex = false, int side = 0, bool chg_col = true, bool exploding_parts = false);
        virtual bool draw_nodl(bool alset = false);

        void init3DM();

        inline MESH_3DM() {init3DM();}

        void destroy3DM();

        virtual ~MESH_3DM() {destroy3DM();}

		virtual bool has_animation_data();
	public:
        static MODEL *load(const String &filename);
    };
} // namespace TA3D

#endif
