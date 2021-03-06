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
  |                                         3do.cpp                                    |
  |  ce fichier contient les structures, classes et fonctions nécessaires à la lecture |
  | des fichiers 3do de total annihilation qui sont les fichiers contenant les modèles |
  | 3d des objets du jeu.                                                              |
  |                                                                                    |
  \-----------------------------------------------------------------------------------*/

#include <stdafx.h>
#include <misc/matrix.h>
#include <TA3D_NameSpace.h>
#include <ta3dbase.h>
#include "3do.h"
#include <misc/ta3d_math.h>
#include <misc/paths.h>
#include <misc/files.h>
#include <logs/logs.h>
#include <gfx/gl.extensions.h>
#include "textures.h"

namespace TA3D
{
	static bool coupe(int x1, int y1, int dx1, int dy1, int x2, int y2, int dx2, int dy2)
	{
		int u1 = x1, v1 = y1, u2 = x2 + dx2, v2 = y2 + dy2;
		if (u1 > x2)
			u1 = x2;
		if (v1 > y2)
			v1 = y2;
		if (x1 + dx1 > u2)
			u2 = x1 + dx1;
		if (y1 + dy1 > v2)
			v2 = y1 + dy1;
		return (u2 - u1 + 1 < dx1 + dx2 && v2 - v1 + 1 < dy1 + dy2);
	}

	void Mesh3DO::init3DO()
	{
		init();
		selprim = -1;
	}

	void Mesh3DO::destroy3DO()
	{
		destroy();
		init3DO();
	}

	int Mesh3DO::load(File* file, int dec, const String& filename)
	{
		if (nb_vtx > 0)
			destroy3DO(); // Au cas où l'objet ne serait pas vierge

		if (file == NULL)
			return -1;

		int offset = file->tell();

		tagObject header; // Lit l'en-tête
		*file >> header;

		if (header.NumberOfVertexes + offset < 0)
			return -1;
		if (header.NumberOfPrimitives + offset < 0)
			return -1;
		if (header.OffsetToChildObject + offset < 0)
			return -1;
		if (header.OffsetToSiblingObject + offset < 0)
			return -1;
		if (header.OffsetToVertexArray + offset < 0)
			return -1;
		if (header.OffsetToPrimitiveArray + offset < 0)
			return -1;
		if (header.OffsetToObjectName + offset < 0 || header.OffsetToObjectName > 102400)
			return -1;
		int i;

		nb_vtx = (short)header.NumberOfVertexes;
		nb_prim = (short)header.NumberOfPrimitives;
		file->seek(header.OffsetToObjectName);
		name = file->getString();
		if (header.OffsetToChildObject) // Charge récursivement les différents objets du modèle
		{
			Mesh3DO* pChild = new Mesh3DO;
			child = pChild;
			file->seek(header.OffsetToChildObject);
			if (pChild->load(file, dec + 1, filename))
			{
				destroy();
				return -1;
			}
		}
		if (header.OffsetToSiblingObject) // Charge récursivement les différents objets du modèle
		{
			Mesh3DO* pNext = new Mesh3DO;
			next = pNext;
			file->seek(header.OffsetToSiblingObject);
			if (pNext->load(file, dec, filename))
			{
				destroy();
				return -1;
			}
		}
		points = new Vector3D[nb_vtx]; // Alloue la mémoire nécessaire pour stocker les points
		const float div = 0.5f / 65536.0f;
		pos_from_parent.x = (float)header.XFromParent * div;
		pos_from_parent.y = (float)header.YFromParent * div;
		pos_from_parent.z = -(float)header.ZFromParent * div;
		file->seek(header.OffsetToVertexArray);

		for (i = 0; i < nb_vtx; ++i) // Lit le tableau de points stocké dans le fichier
		{
			tagVertex vertex;
			*file >> vertex;
			points[i].x = (float)vertex.x * div;
			points[i].y = (float)vertex.y * div;
			points[i].z = -(float)vertex.z * div;
		}

		int n_index = 0;
		selprim = -1; //header.OffsetToselectionPrimitive;
		sel[0] = sel[1] = sel[2] = sel[3] = 0;
		for (i = 0; i < nb_prim; ++i) // Compte le nombre de primitive de chaque sorte
		{
			tagPrimitive primitive;
			file->seek(header.OffsetToPrimitiveArray + i * (int)sizeof(tagPrimitive));
			*file >> primitive;

			switch (primitive.NumberOfVertexIndexes)
			{
				case 0:
					break;
				case 1:
					++nb_p_index;
					break;
				case 2:
					nb_l_index = (short)(nb_l_index + 2);
					break;
				default:
					if (i == header.OffsetToselectionPrimitive)
					{
						selprim = 1; //nb_t_index;
						break;
					}
					else
					{
						if (primitive.IsColored && primitive.ColorIndex == 1)
							break;
						file->seek(primitive.OffsetToTextureName);
						if (!primitive.IsColored && (!primitive.OffsetToTextureName || !file->getc()))
							break;
					}
					n_index += primitive.NumberOfVertexIndexes;
					++nb_t_index;
			}
		}

		if (nb_p_index > 0) // Alloue la mémoire nécessaire pour stocker les primitives
			p_index = new GLushort[nb_p_index];
		if (nb_l_index > 0)
			l_index = new GLushort[nb_l_index];
		int* tex = NULL;
		byte* usetex = NULL;
		if (nb_t_index > 0)
		{
			tex = new int[nb_t_index];
			usetex = new byte[nb_t_index];
			nb_index = new short[nb_t_index];
			t_index = new GLushort[n_index];
		}

		int pos_p = 0;
		int pos_l = 0;
		int pos_t = 0;
		int cur = 0;
		int nb_diff_tex = 0;
		int* index_tex = new int[nb_prim];
		int t_m = 0;
		for (i = 0; i < nb_prim; ++i) // Compte le nombre de primitive de chaque sorte
		{
			tagPrimitive primitive;
			file->seek(header.OffsetToPrimitiveArray + i * (int)sizeof(tagPrimitive));
			*file >> primitive;

			switch (primitive.NumberOfVertexIndexes)
			{
				case 0:
					break;
				case 1:
					file->seek(primitive.OffsetToVertexIndexArray);
					{
						short s;
						*file >> s;
						p_index[pos_p++] = s;
					}
					break;
				case 2:
					file->seek(primitive.OffsetToVertexIndexArray);
					{
						short s;
						*file >> s;
						l_index[pos_l++] = s;
						*file >> s;
						l_index[pos_l++] = s;
					}
					break;
				default:
					if (i != header.OffsetToselectionPrimitive)
					{
						if (primitive.IsColored && primitive.ColorIndex == 1)
							break;
						file->seek(primitive.OffsetToTextureName);
						if (!primitive.IsColored && (!primitive.OffsetToTextureName || !file->getc()))
							break;
					}
					else
					{
						file->seek(primitive.OffsetToVertexIndexArray);
						for (int e = 0; e < primitive.NumberOfVertexIndexes && e < 4; ++e)
						{
							short s;
							*file >> s;
							sel[e] = s;
						}
						break;
					}
					nb_index[cur] = (short)primitive.NumberOfVertexIndexes;
					file->seek(primitive.OffsetToTextureName);
					tex[cur] = t_m = texture_manager.get_texture_index(file->getString());
					usetex[cur] = 1;
					if (t_m == -1)
					{
						if (primitive.ColorIndex >= 0 && primitive.ColorIndex < 256)
						{
							usetex[cur] = 1;
							tex[cur] = t_m = primitive.ColorIndex;
						}
						else
							usetex[cur] = 0;
					}
					if (t_m >= 0)
					{ // Code pour la création d'une texture propre à chaque modèle
						bool al_in = false;
						int indx = t_m;
						for (int e = 0; e < nb_diff_tex; ++e)
							if (index_tex[e] == indx)
							{
								al_in = true;
								break;
							}
						if (!al_in)
							index_tex[nb_diff_tex++] = indx;
					}
					file->seek(primitive.OffsetToVertexIndexArray);
					for (int e = 0; e < nb_index[cur]; ++e)
					{
						short s;
						*file >> s;
						t_index[pos_t++] = s;
					}
					++cur;
			}
		}

		/*------------------------------Création de la texture unique pour l'unité--------------*/
		int* px = new int[nb_diff_tex];
		int* py = new int[nb_diff_tex]; // Pour placer les différentes mini-textures sur une grande texture
		int mx = 0;
		int my = 0;

		for (i = 0; i < nb_diff_tex; ++i)
		{
			int dx = texture_manager.tex[index_tex[i]].bmp[0]->w;
			int dy = texture_manager.tex[index_tex[i]].bmp[0]->h;
			px[i] = py[i] = 0;
			if (i != 0)
				for (int e = 0; e < i; ++e)
				{
					int fx = texture_manager.tex[index_tex[e]].bmp[0]->w, fy = texture_manager.tex[index_tex[e]].bmp[0]->h;
					bool found[3];
					found[0] = found[1] = found[2] = true;
					int j;

					px[i] = px[e] + fx;
					py[i] = py[e];
					for (j = 0; j < i; ++j)
					{
						int gx = texture_manager.tex[index_tex[j]].bmp[0]->w, gy = texture_manager.tex[index_tex[j]].bmp[0]->h;
						if (coupe(px[i], py[i], dx, dy, px[j], py[j], gx, gy))
						{
							found[0] = false;
							break;
						}
					}

					px[i] = px[e];
					py[i] = py[e] + fy;
					for (j = 0; j < i; ++j)
					{
						int gx = texture_manager.tex[index_tex[j]].bmp[0]->w, gy = texture_manager.tex[index_tex[j]].bmp[0]->h;
						if (coupe(px[i], py[i], dx, dy, px[j], py[j], gx, gy))
						{
							found[2] = false;
							break;
						}
					}
					px[i] = px[e] + fx;
					py[i] = 0;

					for (j = 0; j < i; ++j)
					{
						int gx = texture_manager.tex[index_tex[j]].bmp[0]->w, gy = texture_manager.tex[index_tex[j]].bmp[0]->h;
						if (coupe(px[i], py[i], dx, dy, px[j], py[j], gx, gy))
						{
							found[1] = false;
							break;
						}
					}
					bool deborde = false;
					bool found_one = false;
					int deb = 0;

					if (found[1])
					{
						px[i] = px[e] + fx;
						py[i] = 0;
						deborde = false;
						if (px[i] + dx > mx || py[i] + dy > my)
							deborde = true;
						deb = Math::Max(mx, px[i] + dx) * Math::Max(py[i] + dy, my) - mx * my;
						found_one = true;
					}
					if (found[0] && (!found_one || deborde))
					{
						px[i] = px[e] + fx;
						py[i] = py[e];
						deborde = false;
						if (px[i] + dx > mx || py[i] + dy > my)
							deborde = true;
						deb = Math::Max(mx, px[i] + dx) * Math::Max(py[i] + dy, my) - mx * my;
						found_one = true;
					}
					if (found[2] && deborde)
					{
						int ax = px[i], ay = py[i];
						px[i] = px[e];
						py[i] = py[e] + fy;
						deborde = false;
						if (px[i] + dx > mx || py[i] + dy > my)
							deborde = true;
						int deb2 = Math::Max(mx, px[i] + dx) * Math::Max(py[i] + dy, my) - mx * my;
						if (found_one && deb < deb2)
						{
							px[i] = ax;
							py[i] = ay;
						}
						else
							found_one = true;
					}
					if (found_one) // On a trouvé une position qui convient
						break;
				}
			if (px[i] + dx > mx)
				mx = px[i] + dx;
			if (py[i] + dy > my)
				my = py[i] + dy;
		}

		SDL_Surface* bmp = gfx->create_surface_ex(32, mx, my);
		if (bmp != NULL && mx != 0 && my != 0)
		{
			gfx->set_texture_format(gfx->defaultTextureFormat_RGB());
			SDL_FillRect(bmp, NULL, 0);
			tex_cache_name.clear();
			gltex.clear();
			int nb_sprites = 0;
			for (i = 0; i < nb_diff_tex; ++i)
			{
				nb_sprites = Math::Max((int)texture_manager.tex[index_tex[i]].nb_bmp, nb_sprites);
				fixed_textures |= texture_manager.tex[index_tex[i]].logo;
			}
			gltex.resize(nb_sprites);
			for (short e = 0; e < nb_sprites; ++e)
			{
				String cache_filename = !filename.empty() ? String(filename) << '-' << (!name.empty() ? name : "none") << '-' << e << ".bin" : String();
				cache_filename.replace('/', 'S');
				cache_filename.replace('\\', 'S');

				gltex[e] = 0;

				for (i = 0; i < nb_diff_tex; ++i)
				{
					int f = e % texture_manager.tex[index_tex[i]].nb_bmp;
					blit(texture_manager.tex[index_tex[i]].bmp[f], bmp,
						0, 0, px[i], py[i],
						texture_manager.tex[index_tex[i]].bmp[f]->w,
						texture_manager.tex[index_tex[i]].bmp[f]->h);
				}
				cache_filename = TA3D::Paths::Files::ReplaceExtension(cache_filename, ".tex");
				if (!TA3D::Paths::Exists(String(TA3D::Paths::Caches) << cache_filename))
					SaveTex(bmp, String(TA3D::Paths::Caches) << cache_filename);

				tex_cache_name.push_back(cache_filename);
			}
		}
		else
			gltex.clear();
		if (bmp)
			SDL_FreeSurface(bmp);

		int nb_total_point = 0;
		for (i = 0; i < nb_t_index; ++i)
			nb_total_point += nb_index[i];

		nb_total_point += nb_l_index;
		if (selprim >= 0)
			nb_total_point += 4;

		Vector3D* p = new Vector3D[nb_total_point << 1]; // *2 pour le volume d'ombre
		int prim_dec = selprim >= 0 ? 4 : 0;
		for (i = 0; i < nb_total_point - nb_l_index - prim_dec; ++i)
		{
			p[i + nb_total_point] = p[i] = points[t_index[i]];
			t_index[i] = (GLushort)i;
		}
		if (selprim >= 0)
		{
			p[nb_total_point - nb_l_index - prim_dec] = points[sel[0]];
			sel[0] = (GLushort)(nb_total_point - nb_l_index - prim_dec);
			p[nb_total_point - nb_l_index - prim_dec + 1] = points[sel[1]];
			sel[1] = (GLushort)(nb_total_point - nb_l_index - prim_dec + 1);
			p[nb_total_point - nb_l_index - prim_dec + 2] = points[sel[2]];
			sel[2] = (GLushort)(nb_total_point - nb_l_index - prim_dec + 2);
			p[nb_total_point - nb_l_index - prim_dec + 3] = points[sel[3]];
			sel[3] = (GLushort)(nb_total_point - nb_l_index - prim_dec + 3);
		}
		for (i = nb_total_point - nb_l_index; i < nb_total_point; ++i)
		{
			const int e = i - nb_total_point + nb_l_index;
			p[i + nb_total_point] = p[i] = points[l_index[e]];
			l_index[e] = (GLushort)i;
		}
		if (nb_l_index == 2)
		{
			if (p[l_index[0]].x < 0.0f)
			{
				const GLushort tmp = l_index[0];
				l_index[0] = l_index[1];
				l_index[1] = tmp;
			}
		}
		DELETE_ARRAY(points);
		points = p;
		nb_vtx = (short)nb_total_point;

		int nb_triangle = 0;
		for (i = 0; i < nb_t_index; ++i)
			nb_triangle += nb_index[i] - 2;
		GLushort* index = new GLushort[nb_triangle * 3];
		tcoord = new float[nb_vtx << 1];
		cur = 0;
		int curt = 0;
		pos_t = 0;
		for (i = 0; i < nb_t_index; ++i)
		{
			int indx = 0;
			for (int f = 0; f < nb_diff_tex; ++f)
			{
				if (tex[i] == index_tex[f])
				{
					indx = f;
					break;
				}
			}
			for (int e = 0; e < nb_index[i]; ++e)
			{
				if (e < 3)
					index[pos_t++] = t_index[cur];
				else
				{
					index[pos_t] = index[pos_t - 3];
					++pos_t;
					index[pos_t] = index[pos_t - 2];
					++pos_t;
					index[pos_t++] = t_index[cur];
				}
				tcoord[curt] = 0.0f;
				tcoord[curt + 1] = 0.0f;

				if (usetex[i])
				{
					switch (e & 3)
					{
						case 1:
							tcoord[curt] += (float)(texture_manager.tex[tex[i]].bmp[0]->w - 1) / float(mx - 1);
							break;
						case 2:
							tcoord[curt] += (float)(texture_manager.tex[tex[i]].bmp[0]->w - 1) / float(mx - 1);
							tcoord[curt + 1] += (float)(texture_manager.tex[tex[i]].bmp[0]->h - 1) / float(my - 1);
							break;
						case 3:
							tcoord[curt + 1] += (float)(texture_manager.tex[tex[i]].bmp[0]->h - 1) / float(my - 1);
							break;
					};
					tcoord[curt] += ((float)px[indx] + 0.5f) / float(mx - 1);
					tcoord[curt + 1] += ((float)py[indx] + 0.5f) / float(my - 1);
				}
				++cur;
				curt += 2;
			}
		}
		for (cur = 0; cur < pos_t; cur += 3) // Petite inversion pour avoir un affichage correct
		{
			GLushort t = index[cur + 1];
			index[cur + 1] = index[cur + 2];
			index[cur + 2] = t;
		}
		nb_t_index = short(nb_triangle * 3);
		DELETE_ARRAY(t_index);
		t_index = index;
		DELETE_ARRAY(usetex);
		DELETE_ARRAY(tex);
		/*--------------------------------------------------------------------------------------*/

		if (nb_t_index > 0) // Calcule les normales pour l'éclairage
		{
			N = new Vector3D[nb_vtx << 1];
			F_N = new Vector3D[nb_t_index / 3];
			memset(N, 0, nb_vtx * 2 * sizeof(Vector3D));
			int e = 0;
			for (i = 0; i < nb_t_index; i += 3)
			{
				Vector3D AB, AC, Normal;
				AB = points[t_index[i + 1]] - points[t_index[i]];
				AC = points[t_index[i + 2]] - points[t_index[i]];
				Normal = AB * AC;
				Normal.normalize();
				F_N[e++] = Normal;
				for (int e = 0; e < 3; ++e)
					N[t_index[i + e]] = N[t_index[i + e]] + Normal;
			}
			for (i = 0; i < nb_vtx; ++i)
				N[i].normalize();
			for (i = nb_vtx; i < (nb_vtx << 1); ++i)
				N[i] = N[i - nb_vtx];
		}
		DELETE_ARRAY(px);
		DELETE_ARRAY(py);
		DELETE_ARRAY(index_tex);
		return 0;
	}

	bool Mesh3DO::draw(float t, AnimationData* data_s, bool sel_primitive, bool alset, bool notex, PlayerId side, bool chg_col, bool exploding_parts)
	{
		bool culling = glIsEnabled(GL_CULL_FACE);
		glEnable(GL_CULL_FACE);
		bool explodes = script_index >= 0 && data_s && (data_s->data[script_index].flag & FLAG_EXPLODE);
		bool hide = false;
		bool set = false;
		float color_factor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		if (!tex_cache_name.empty())
		{
			for (uint32 i = 0; i < tex_cache_name.size(); ++i)
				load_texture_id(i);
			tex_cache_name.clear();
		}

		if (!(explodes && !exploding_parts))
		{
			glPushMatrix();

			glTranslatef(pos_from_parent.x, pos_from_parent.y, pos_from_parent.z);
			if (script_index >= 0 && data_s)
			{
				if (!explodes ^ exploding_parts)
				{
					glTranslatef(data_s->data[script_index].axe[0].pos, data_s->data[script_index].axe[1].pos, data_s->data[script_index].axe[2].pos);
					glRotatef(data_s->data[script_index].axe[0].angle, 1.0f, 0.0f, 0.0f);
					glRotatef(data_s->data[script_index].axe[1].angle, 0.0f, 1.0f, 0.0f);
					glRotatef(data_s->data[script_index].axe[2].angle, 0.0f, 0.0f, 1.0f);
				}
				hide = data_s->data[script_index].flag & FLAG_HIDE;
			}

			hide |= explodes ^ exploding_parts;
			if (chg_col)
				glGetFloatv(GL_CURRENT_COLOR, color_factor);
			int texID = player_color_map[side];
			if (script_index >= 0 && data_s && (data_s->data[script_index].flag & FLAG_ANIMATED_TEXTURE) && !fixed_textures && !gltex.empty())
				texID = (int)(((int)(t * 10.0f)) % gltex.size());
			if ((int)gl_dlist.size() > texID && gl_dlist[texID] && !hide && !chg_col && !notex && false)
			{
				glCallList(gl_dlist[texID]);
				alset = false;
				set = false;
			}
			else if (!hide)
			{
				bool creating_list = false;
				if ((int)gl_dlist.size() <= texID)
					gl_dlist.resize(texID + 1);
				if (!chg_col && !notex && gl_dlist[texID] == 0 && false)
				{
					gl_dlist[texID] = glGenLists(1);
					glNewList(gl_dlist[texID], GL_COMPILE_AND_EXECUTE);
					alset = false;
					set = false;
					creating_list = true;
				}
				if (nb_t_index > 0 && nb_vtx > 0 && t_index != NULL)
				{
					if (!alset)
					{
						glEnableClientState(GL_VERTEX_ARRAY); // Les sommets
						glEnableClientState(GL_NORMAL_ARRAY);
						if (notex)
							glDisableClientState(GL_TEXTURE_COORD_ARRAY);
						else
							glEnableClientState(GL_TEXTURE_COORD_ARRAY);
						glEnable(GL_LIGHTING);
						if (notex)
							glDisable(GL_TEXTURE_2D);
						else
							glEnable(GL_TEXTURE_2D);
						alset = true;
					}
					if (chg_col || !notex)
					{
						if (chg_col && color_factor[3] != 1.0f) // La transparence
						{
							glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
							glEnable(GL_BLEND);
						}
						else
							glDisable(GL_BLEND);
					}
					set = true;
					if (gltex.empty())
					{
						alset = false;
						glDisable(GL_TEXTURE_2D);
						glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					}
					if (!notex && !gltex.empty())
					{
						if (texID < (int)gltex.size() && texID >= 0)
							glBindTexture(GL_TEXTURE_2D, gltex[texID]);
						else
							glBindTexture(GL_TEXTURE_2D, gltex[0]);
						glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
					}
					glVertexPointer(3, GL_FLOAT, 0, points);
					glNormalPointer(GL_FLOAT, 0, N);
					switch (type)
					{
						case MESH_TYPE_TRIANGLES:
							glDrawRangeElements(GL_TRIANGLES, 0, nb_vtx - 1, nb_t_index, GL_UNSIGNED_SHORT, t_index); // draw everything
							break;
						case MESH_TYPE_TRIANGLE_STRIP:
							glDisable(GL_CULL_FACE);
							glDrawRangeElements(GL_TRIANGLE_STRIP, 0, nb_vtx - 1, nb_t_index, GL_UNSIGNED_SHORT, t_index); // draw everything
							glEnable(GL_CULL_FACE);
							break;
					};
				}
				if (creating_list)
					glEndList();
			}

			if (sel_primitive && selprim >= 0 && nb_vtx > 0) // && (data_s==NULL || (data_s!=NULL && !data_s->explode))) {
			{
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisable(GL_LIGHTING);
				glDisable(GL_TEXTURE_2D);
				if (!set)
					glVertexPointer(3, GL_FLOAT, 0, points);
				glColor3ub(0, 0xFF, 0);
				glTranslatef(0.0f, 2.0f, 0.0f);
				glDrawRangeElements(GL_LINE_LOOP, 0, nb_vtx - 1, 4, GL_UNSIGNED_SHORT, sel); // dessine la primitive de sélection
				glTranslatef(0.0f, -2.0f, 0.0f);
				if (notex)
				{
					const int var = abs(0xFF - (MILLISECONDS_SINCE_INIT % 1000) * 0x200 / 1000);
					glColor3ub(0, GLubyte(var), 0);
				}
				else
					glColor3ub(0xFF, 0xFF, 0xFF);
				alset = false;
			}
			if (chg_col)
				glColor4fv(color_factor);
			if (child && !(explodes && !exploding_parts))
				alset = child->draw(t, data_s, sel_primitive, alset, notex, side, chg_col, exploding_parts && !explodes);
			glPopMatrix();
		}
		if (next)
			alset = next->draw(t, data_s, sel_primitive, alset, notex, side, chg_col, exploding_parts);

		if (!culling)
			glDisable(GL_CULL_FACE);
		return alset;
	}

	bool Mesh3DO::draw_nodl(bool alset)
	{
		bool culling = glIsEnabled(GL_CULL_FACE);
		glPushMatrix();

		glTranslatef(pos_from_parent.x, pos_from_parent.y, pos_from_parent.z);

		if (nb_t_index > 0 && nb_vtx > 0 && t_index != NULL)
		{
			if (!alset)
			{
				glEnableClientState(GL_VERTEX_ARRAY); // Les sommets
				glEnableClientState(GL_NORMAL_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glEnable(GL_LIGHTING);
				glEnable(GL_TEXTURE_2D);
				alset = true;
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
			}
			if (gltex.empty())
			{
				alset = false;
				glDisable(GL_TEXTURE_2D);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			if (!gltex.empty())
			{
				glBindTexture(GL_TEXTURE_2D, gltex[0]);
				glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
			}
			glVertexPointer(3, GL_FLOAT, 0, points);
			glNormalPointer(GL_FLOAT, 0, N);
			switch (type)
			{
				case MESH_TYPE_TRIANGLES:
					glDrawRangeElements(GL_TRIANGLES, 0, nb_vtx - 1, nb_t_index, GL_UNSIGNED_SHORT, t_index); // draw everything
					break;
				case MESH_TYPE_TRIANGLE_STRIP:
					glDisable(GL_CULL_FACE);
					glDrawRangeElements(GL_TRIANGLE_STRIP, 0, nb_vtx - 1, nb_t_index, GL_UNSIGNED_SHORT, t_index); // draw everything
					glEnable(GL_CULL_FACE);
					break;
			};
		}
		if (child)
			alset = child->draw_nodl(alset);
		glPopMatrix();
		if (next)
			alset = next->draw_nodl(alset);
		if (!culling)
			glDisable(GL_CULL_FACE);

		return alset;
	}

	Model* Mesh3DO::load(const String& filename)
	{
		File* file = VFS::Instance()->readFile(filename);
		if (!file)
		{
			LOG_ERROR(LOG_PREFIX_3DO << "could not read file '" << filename << "'");
			return NULL;
		}

		Mesh3DO* mesh = new Mesh3DO;
		mesh->load(file, 0, filename);
		delete file;

		Model* model = new Model;
		model->mesh = mesh;
		model->postLoadComputations();
		return model;
	}

	const char* Mesh3DO::getExt()
	{
		return ".3do";
	}
} // namespace TA3D
