#include <stdafx.h>
#include <TA3D_NameSpace.h>
#include <misc/paths.h>
#include "textures.h"

namespace TA3D
{



	TEXTURE_MANAGER	texture_manager;

	TEXTURE_MANAGER::TEXTURE_MANAGER() : nbtex(0), tex(NULL)
	{
		tex_hashtable.set_empty_key(String());
	}



	void TEXTURE_MANAGER::init()
	{
		tex_hashtable.clear();
		nbtex = 0;
		tex = NULL;
	}

	void TEXTURE_MANAGER::destroy()
	{
		DELETE_ARRAY(tex);
		init();
	}



	int TEXTURE_MANAGER::get_texture_index(const String& texture_name)
	{
		if (nbtex == 0)
			return -1;
		return tex_hashtable[ texture_name ] - 1;
	}


	GLuint TEXTURE_MANAGER::get_gl_texture(const String& texture_name, const int frame)
	{
		int index = get_texture_index(texture_name);
		return (index == -1) ? 0 : tex[index].glbmp[frame];
	}


	SDL_Surface* TEXTURE_MANAGER::get_bmp_texture(const String& texture_name, const int frame)
	{
		int index = get_texture_index(texture_name);
		return (index== -1) ? NULL : tex[index].bmp[frame];
	}


	int TEXTURE_MANAGER::all_texture()
	{
		// Crée des textures correspondant aux couleurs de la palette de TA
		nbtex = 256;
		tex = new Gaf::Animation[nbtex];
		for (int i = 0; i < 256; ++i)
		{
			tex[i].nb_bmp = 1;
			tex[i].bmp.resize(1, NULL);
			tex[i].glbmp.resize(1, 0);
			tex[i].ofs_x.resize(1, 0);
			tex[i].ofs_y.resize(1, 0);
			tex[i].w.resize(1, 0);
			tex[i].h.resize(1, 0);
			tex[i].name = String('_') << i;

			tex[i].ofs_x[0] = 0;
			tex[i].ofs_y[0] = 0;
			tex[i].w[0] = 16;
			tex[i].h[0] = 16;
			tex[i].bmp[0] = gfx->create_surface_ex(32,16,16);
			SDL_FillRect(tex[i].bmp[0], NULL, makeacol(pal[i].r, pal[i].g, pal[i].b, 0xFF));

			tex_hashtable[tex[i].name] = i + 1;
		}

		String::Vector file_list;
		VFS::Instance()->getFilelist("textures\\*.gaf", file_list);
		const String::Vector::const_iterator end = file_list.end();
		for (String::Vector::const_iterator cur_file = file_list.begin(); cur_file != end; ++cur_file)
		{
			File *file = VFS::Instance()->readFile(*cur_file);
			load_gaf(file, String::ToUpper(Paths::ExtractFileName(*cur_file)) == "LOGOS.GAF");
			delete file;
		}
		return 0;
	}


	int TEXTURE_MANAGER::load_gaf(File* file, bool logo)
	{
		sint32 nb_entry = Gaf::RawDataEntriesCount(file);
		int n_nbtex = nbtex + nb_entry;
		Gaf::Animation* n_tex = new Gaf::Animation[n_nbtex];
		for (int i = 0; i < nbtex; ++i)
		{
			n_tex[i] = tex[i];
			tex[i].init();
		}
		DELETE_ARRAY(tex);
		tex = n_tex;
		for (int i = 0; i < nb_entry; ++i)
		{
			tex[nbtex + i].loadGAFFromRawData(file, i, false);
			tex[nbtex + i].logo = logo;
			tex_hashtable[tex[nbtex + i].name] = nbtex + i + 1;
		}
		nbtex += nb_entry;
		return 0;
	}



} // namespace TA3D
