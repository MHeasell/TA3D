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

//
//	the gfx module
//
//	it contains basic gfx commands like font management, background management, etc...
//

#include <stdafx.h>
#include <TA3D_NameSpace.h>
#include "glfunc.h"
#include <gaf.h>
#include "gfx.h"
#include "gui/skin.manager.h"
#include <misc/paths.h>
#include <logs/logs.h>
#include <strings.h>
#include <misc/math.h>
#include <input/keyboard.h>
#include <input/mouse.h>
#include "gui/base.h"
#include <backtrace.h>
#include <fstream>
#include <iostream>
#include <cassert>

namespace TA3D
{

	void GFX::set_texture_format(GLuint gl_format)
	{
		texture_format = gl_format == 0 ? defaultRGBTextureFormat : gl_format;
	}

	void GFX::use_mipmapping(bool use)
	{
		build_mipmaps = use;
	}

	void GFX::detectDefaultSettings()
	{
		const GLubyte* glStr = glGetString(GL_VENDOR);
		String glVendor = glStr ? ToUpper((const char*)glStr) : String();

		enum VENDOR
		{
			Unknown,
			Ati,
			Nvidia,
			Sis,
			Intel
		};

		VENDOR glVendorID = Unknown;
		if (glVendor.find("ATI") != String::npos)
			glVendorID = Ati;
		else if (glVendor.find("NVIDIA") != String::npos)
			glVendorID = Nvidia;
		else if (glVendor.find("SIS") != String::npos)
			glVendorID = Sis;
		else if (glVendor.find("INTEL") != String::npos)
			glVendorID = Intel;

#ifdef TA3D_PLATFORM_LINUX
		switch (glVendorID)
		{
			case Ati:
				lp_CONFIG->fsaa = 0;
				lp_CONFIG->anisotropy = 1;

				lp_CONFIG->use_texture_cache = false;

				break;

			case Nvidia:
				lp_CONFIG->fsaa = 0;
				lp_CONFIG->anisotropy = 1;

				lp_CONFIG->use_texture_cache = true;

				break;

			case Sis:
			case Intel:
				lp_CONFIG->fsaa = 0;
				lp_CONFIG->anisotropy = 1;

				lp_CONFIG->use_texture_cache = false;

				break;

			case Unknown:
				lp_CONFIG->fsaa = 0;
				lp_CONFIG->anisotropy = 1;

				lp_CONFIG->use_texture_cache = false;

				break;
		};
#elif defined TA3D_PLATFORM_WINDOWS
		switch (glVendorID)
		{
			case Ati:
				lp_CONFIG->fsaa = 0;
				lp_CONFIG->anisotropy = 1;

				lp_CONFIG->use_texture_cache = false;

				break;

			case Nvidia:
				lp_CONFIG->fsaa = 0;
				lp_CONFIG->anisotropy = 1;

				lp_CONFIG->use_texture_cache = true;

				break;

			case Sis:
			case Intel:
				lp_CONFIG->fsaa = 0;
				lp_CONFIG->anisotropy = 1;

				lp_CONFIG->use_texture_cache = false;

				break;

			case Unknown:
				lp_CONFIG->fsaa = 0;
				lp_CONFIG->anisotropy = 1;

				lp_CONFIG->use_texture_cache = false;

				break;
		};
#endif
	}

	void GFX::initSDL()
	{
		static bool GLloaded = false;

		if (!GLloaded)
		{
			// First we need to load the OpenGL library
			if (SDL_GL_LoadLibrary(NULL))
			{
				LOG_CRITICAL(LOG_PREFIX_OPENGL << "could not load OpenGL library!");
			}
			else
				GLloaded = true;
		}

		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, TA3D::lp_CONFIG->fsaa > 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, TA3D::lp_CONFIG->fsaa);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);

		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

		SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
		SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
		SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
		SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);

		uint32 flags = SDL_WINDOW_OPENGL;
		if (TA3D::lp_CONFIG->fullscreen)
			flags |= SDL_WINDOW_FULLSCREEN;

		screen = SDL_CreateWindow(
			"Total Annihilation 3D",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			TA3D::lp_CONFIG->screen_width,
			TA3D::lp_CONFIG->screen_height,
			flags);

		if (screen == NULL)
		{
			LOG_WARNING(LOG_PREFIX_GFX << "SDL_CreateWindow failed : " << SDL_GetError());
			LOG_WARNING(LOG_PREFIX_GFX << "retrying with GL_DEPTH_SIZE = 16");
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
			screen = SDL_CreateWindow(
				"TA3D SDL Window",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				TA3D::lp_CONFIG->screen_width,
				TA3D::lp_CONFIG->screen_height,
				flags);
		}

		if (screen == NULL)
		{
			LOG_ERROR(LOG_PREFIX_GFX << "Impossible to set OpenGL video mode!");
			LOG_ERROR(LOG_PREFIX_GFX << "SDL_GetError() = " << SDL_GetError());
			criticalMessage(String("Impossible to set OpenGL video mode! SDL_GetError() = '") << SDL_GetError() << "'");
			exit(1);
			return;
		}

		// The context is returned here but we don't store it in a variable.
		// This is technically a leak, but the context will live
		// until the program terminates anyway.
		SDL_GL_CreateContext(screen);

		SDL_Surface* icon = load_image("gfx\\icon.png");
		if (icon)
		{
			SDL_SetWindowIcon(screen, icon);
			SDL_FreeSurface(icon);
		}

		preCalculations();
		// Install OpenGL extensions
		installOpenGLExtensions();

		defaultRGBTextureFormat = GL_RGB8;
		defaultRGBATextureFormat = GL_RGBA8;

		// Check everything is supported
		checkConfig();

		set_texture_format(defaultRGBTextureFormat);
		glViewport(0, 0, width, height);

		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
	}

	void GFX::checkConfig() const
	{
	}

	bool GFX::checkVideoCardWorkaround() const
	{
		// Check for ATI workarounds (if an ATI card is present)
		bool workaround = Substr(ToUpper((const char*)glGetString(GL_VENDOR)), 0, 3) == "ATI";
		// Check for SIS workarounds (if an SIS card is present) (the same than ATI)
		workaround |= ToUpper((const char*)glGetString(GL_VENDOR)).find("SIS") != String::npos;
		return workaround;
	}

	GFX::GFX()
		: width(0), height(0), normal_font(NULL), small_font(NULL), TA_font(NULL), ta3d_gui_font(NULL), big_font(NULL), SCREEN_W_HALF(0), SCREEN_H_HALF(0), SCREEN_W_INV(0.), SCREEN_H_INV(0.), SCREEN_W_TO_640(0.), SCREEN_H_TO_480(0.), textureFBO(0), textureDepth(0), textureColor(0), ati_workaround(false), max_tex_size(0), default_texture(0), alpha_blending_set(false), texture_format(0), build_mipmaps(false), defaultRGBTextureFormat(GL_RGB8), defaultRGBATextureFormat(GL_RGBA8)
	{
		// Initialize the GFX Engine
		if (lp_CONFIG->first_start)
		{
			initSDL();
			// Guess best settings
			detectDefaultSettings();
			lp_CONFIG->first_start = false;
		}
		initSDL();
		ati_workaround = checkVideoCardWorkaround();

		LOG_DEBUG("Allocating palette memory...");
		TA3D::pal = new SDL_Color[256]; // Allocate a new palette

		LOG_DEBUG("Loading TA's palette...");
		bool palette = TA3D::load_palette(pal);
		if (!palette)
			LOG_WARNING("Failed to load the palette");

		InitInterface();
	}

	GFX::~GFX()
	{
		DeleteInterface();

		if (textureFBO)
			glDeleteFramebuffersEXT(1, &textureFBO);
		if (textureDepth)
			glDeleteRenderbuffersEXT(1, &textureDepth);
		destroy_texture(textureColor);
		destroy_texture(default_texture);

		DELETE_ARRAY(TA3D::pal);

		normal_font = NULL;
		small_font = NULL;
		TA_font = NULL;
		ta3d_gui_font = NULL;

		font_manager.destroy();
		Gui::skin_manager.destroy();
	}

	void GFX::loadDefaultTextures()
	{
		LOG_DEBUG(LOG_PREFIX_GFX << "Loading default texture...");
		default_texture = load_texture("gfx/default.png");

		alpha_blending_set = false;
	}

	void GFX::loadFonts()
	{
		LOG_DEBUG(LOG_PREFIX_GFX << "Creating a normal font...");
		normal_font = font_manager.find("FreeSans", 10, Font::typeTexture);

		LOG_DEBUG(LOG_PREFIX_GFX << "Creating a small font...");
		small_font = font_manager.find("FreeMono", 8, Font::typeTexture);
		small_font->setBold(true);

		LOG_DEBUG(LOG_PREFIX_GFX << "Loading a big font...");
		TA_font = font_manager.find("FreeSans", 16, Font::typeTexture);

		LOG_DEBUG(LOG_PREFIX_GFX << "Loading the GUI font...");
		ta3d_gui_font = font_manager.find("FreeSerif", 10 * height / 480, Font::typeTexture);
		Gui::gui_font = ta3d_gui_font;

		LOG_DEBUG(LOG_PREFIX_GFX << "Loading a big scaled font...");
		big_font = font_manager.find("FreeSans", 16 * height / 480, Font::typeTexture);
	}

	void GFX::set_2D_mode()
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, height, 0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glDisable(GL_LIGHTING);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glEnable(GL_TEXTURE_2D);

		glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
	}

	void GFX::set_2D_clip_rectangle(int x, int y, int w, int h)
	{
		if (w == -1 || h == -1)
		{
			glDisable(GL_SCISSOR_TEST);
		}
		else
		{
			glScissor(x, height - (y + h), w, h);
			glEnable(GL_SCISSOR_TEST);
		}
	}

	void GFX::unset_2D_mode()
	{
		glPopAttrib();
	}

	void GFX::line(const float x1, const float y1, const float x2, const float y2) // Basic drawing routines
	{
		float points[4] = {x1, y1, x2, y2};
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, points);
		glDrawArrays(GL_LINES, 0, 2);
	}
	void GFX::line(const float x1, const float y1, const float x2, const float y2, const uint32 col)
	{
		set_color(col);
		line(x1, y1, x2, y2);
	}

	void GFX::rect(const float x1, const float y1, const float x2, const float y2)
	{
		float points[8] = {x1, y1, x2, y1, x2, y2, x1, y2};
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, points);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
	}
	void GFX::rect(const float x1, const float y1, const float x2, const float y2, const uint32 col)
	{
		set_color(col);
		rect(x1, y1, x2, y2);
	}

	void GFX::rectfill(const float x1, const float y1, const float x2, const float y2)
	{
		float points[8] = {x1, y1, x2, y1, x2, y2, x1, y2};
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, points);
		glDrawArrays(GL_QUADS, 0, 4);
	}
	void GFX::rectfill(const float x1, const float y1, const float x2, const float y2, const uint32 col)
	{
		set_color(col);
		rectfill(x1, y1, x2, y2);
	}

	void GFX::circle_zoned(const float x, const float y, const float r, const float mx, const float my, const float Mx, const float My)
	{
		float d_alpha = DB_PI / (r + 1.0f);
		int n = (int)(DB_PI / d_alpha) + 2;
		float* points = new float[n * 2];
		int i = 0;
		for (float alpha = 0.0f; alpha <= DB_PI; alpha += d_alpha)
		{
			float rx = x + r * cosf(alpha);
			float ry = y + r * sinf(alpha);
			if (rx < mx)
				rx = mx;
			else if (rx > Mx)
				rx = Mx;
			if (ry < my)
				ry = my;
			else if (ry > My)
				ry = My;
			points[i++] = rx;
			points[i++] = ry;
		}
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, points);
		glDrawArrays(GL_LINE_LOOP, 0, i >> 1);
		DELETE_ARRAY(points);
	}

	void GFX::dot_circle_zoned(const float t, const float x, const float y, const float r, const float mx, const float my, const float Mx, const float My)
	{
		float d_alpha = DB_PI / (r + 1.0f);
		int n = (int)(DB_PI / d_alpha) + 2;
		float* points = new float[n * 2];
		int i = 0;
		for (float alpha = 0.0f; alpha <= DB_PI; alpha += d_alpha)
		{
			float rx = x + r * cosf(alpha + t);
			float ry = y + r * sinf(alpha + t);
			if (rx < mx)
				rx = mx;
			else if (rx > Mx)
				rx = Mx;
			if (ry < my)
				ry = my;
			else if (ry > My)
				ry = My;
			points[i++] = rx;
			points[i++] = ry;
		}
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, points);
		glDrawArrays(GL_LINES, 0, i >> 1);
		DELETE_ARRAY(points);
	}

	void GFX::dot_circle_zoned(const float t, const float x, const float y, const float r, const float mx, const float my, const float Mx, const float My, const uint32 col)
	{
		set_color(col);
		dot_circle_zoned(t, x, y, r, mx, my, Mx, My);
	}

	void GFX::circle_zoned(const float x, const float y, const float r, const float mx, const float my, const float Mx, const float My, const uint32 col)
	{
		set_color(col);
		circle_zoned(x, y, r, mx, my, Mx, My);
	}

	void GFX::rectdot(const float x1, const float y1, const float x2, const float y2)
	{
		glLineStipple(1, 0x5555);
		glEnable(GL_LINE_STIPPLE);
		rect(x1, y1, x2, y2);
		glDisable(GL_LINE_STIPPLE);
	}
	void GFX::rectdot(const float x1, const float y1, const float x2, const float y2, const uint32 col)
	{
		set_color(col);
		rectdot(x1, y1, x2, y2);
	}

	void GFX::drawtexture(const GLuint& tex, const float x1, const float y1, const float x2, const float y2, const float u1, const float v1, const float u2, const float v2)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex);

		const float points[8] = {x1, y1, x2, y1, x2, y2, x1, y2};
		const float tcoord[8] = {u1, v1, u2, v1, u2, v2, u1, v2};

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, points);
		glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
		glDrawArrays(GL_QUADS, 0, 4);

		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void GFX::drawtexture(const GLuint& tex, const float x1, const float y1, const float x2, const float y2)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex);

		const float points[8] = {x1, y1, x2, y1, x2, y2, x1, y2};
		static const float tcoord[8] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, points);
		glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
		glDrawArrays(GL_QUADS, 0, 4);

		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void GFX::drawtexture_flip(const GLuint& tex, const float x1, const float y1, const float x2, const float y2)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex);

		const float points[8] = {x1, y1, x2, y1, x2, y2, x1, y2};
		static const float tcoord[8] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f};

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, points);
		glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
		glDrawArrays(GL_QUADS, 0, 4);

		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void GFX::drawtexture(const GLuint& tex, const float x1, const float y1, const float x2, const float y2, const uint32 col)
	{
		set_color(col);
		drawtexture(tex, x1, y1, x2, y2);
	}

	void GFX::drawtexture_flip(const GLuint& tex, const float x1, const float y1, const float x2, const float y2, const uint32 col)
	{
		set_color(col);
		drawtexture_flip(tex, x1, y1, x2, y2);
	}

	void GFX::print(Font* font, const float x, const float y, const float z, const String& text) // Font related routines
	{
		assert(NULL != font);
		if (!text.empty())
		{
			ReInitTexSys(false);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_TEXTURE_2D);
			font->print(x, y, z, text);
		}
	}

	void GFX::print(Font* font, const float x, const float y, const float z, const uint32 col, const String& text) // Font related routines
	{
		set_color(col);
		print(font, x, y, z, text);
	}

	void GFX::print_center(Font* font, const float x, const float y, const float z, const String& text) // Font related routines
	{
		if (font)
		{
			ReInitTexSys(false);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			const float X = x - 0.5f * font->length(text);

			glEnable(GL_TEXTURE_2D);
			font->print(X, y, z, text);
		}
	}

	void GFX::print_center(Font* font, const float x, const float y, const float z, const uint32 col, const String& text) // Font related routines
	{
		set_color(col);
		print_center(font, x, y, z, text);
	}

	void GFX::print_right(Font* font, const float x, const float y, const float z, const String& text) // Font related routines
	{
		if (!font)
			return;

		ReInitTexSys(false);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		float X = x - font->length(text);

		glEnable(GL_TEXTURE_2D);
		font->print(X, y, z, text);
	}

	void GFX::print_right(Font* font, const float x, const float y, const float z, const uint32 col, const String& text) // Font related routines
	{
		set_color(col);
		print_right(font, x, y, z, text);
	}

	GLuint GFX::make_texture(SDL_Surface* bmp, int filter_type, bool clamp)
	{
		if (bmp == NULL)
		{
			LOG_WARNING(LOG_PREFIX_GFX << "make_texture used with empty SDL_Surface");
			return 0;
		}
		MutexLocker locker(pMutex);

		if (bmp->w > max_tex_size || bmp->h > max_tex_size)
		{
			SDL_Surface* tmp = create_surface_ex(bmp->format->BitsPerPixel,
				Math::Min(bmp->w, max_tex_size), Math::Min(bmp->h, max_tex_size));
			stretch_blit(bmp, tmp, 0, 0, bmp->w, bmp->h, 0, 0, tmp->w, tmp->h);
			GLuint tex = make_texture(tmp, filter_type, clamp);
			SDL_FreeSurface(tmp);
			return tex;
		}

		if (!g_useNonPowerOfTwoTextures && (!Math::IsPowerOfTwo(bmp->w) || !Math::IsPowerOfTwo(bmp->h)))
		{
			int w = 1 << Math::Log2(bmp->w);
			int h = 1 << Math::Log2(bmp->h);
			if (w < bmp->w)
				w <<= 1;
			if (h < bmp->h)
				h <<= 1;
			SDL_Surface* tmp = create_surface_ex(bmp->format->BitsPerPixel, w, h);
			stretch_blit_smooth(bmp, tmp, 0, 0, bmp->w, bmp->h, 0, 0, tmp->w, tmp->h);
			GLuint tex = make_texture(tmp, filter_type, clamp);
			SDL_FreeSurface(tmp);
			return tex;
		}

		if (ati_workaround && filter_type != FILTER_NONE && (!Math::IsPowerOfTwo(bmp->w) || !Math::IsPowerOfTwo(bmp->h)))
			filter_type = FILTER_LINEAR;

		if (filter_type == FILTER_NONE || filter_type == FILTER_LINEAR)
			use_mipmapping(false);
		else
			use_mipmapping(true);

		bool can_useGenMipMaps = g_useGenMipMaps && (g_useNonPowerOfTwoTextures || (Math::IsPowerOfTwo(bmp->w) && Math::IsPowerOfTwo(bmp->h)));

		GLuint gl_tex = 0;
		glGenTextures(1, &gl_tex);

		glBindTexture(GL_TEXTURE_2D, gl_tex);

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);

		if (clamp)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		if (GLEW_EXT_texture_filter_anisotropic)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, lp_CONFIG->anisotropy);

		switch (filter_type)
		{
			case FILTER_NONE:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				break;
			case FILTER_LINEAR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				break;
			case FILTER_BILINEAR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				break;
			case FILTER_TRILINEAR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				break;
		}

#ifdef TA3D_PLATFORM_WINDOWS
		bool softMipMaps = build_mipmaps;
#else
		bool softMipMaps = false;
#endif

		//Upload image data to OpenGL
		if (!softMipMaps)
		{
			if (can_useGenMipMaps) // Automatic mipmaps generation
			{
				if (!build_mipmaps || glGenerateMipmapEXT)
					glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);
				else
					glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
			}
			switch (bmp->format->BitsPerPixel)
			{
				case 8:
					if (build_mipmaps && !can_useGenMipMaps) // Software mipmaps generation
						gluBuild2DMipmaps(GL_TEXTURE_2D, texture_format, bmp->w, bmp->h, GL_LUMINANCE, GL_UNSIGNED_BYTE, bmp->pixels);
					else
						glTexImage2D(GL_TEXTURE_2D, 0, texture_format, bmp->w, bmp->h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, bmp->pixels);
					break;
				case 24:
					if (build_mipmaps && !can_useGenMipMaps) // Software mipmaps generation
						gluBuild2DMipmaps(GL_TEXTURE_2D, texture_format, bmp->w, bmp->h, GL_RGB, GL_UNSIGNED_BYTE, bmp->pixels);
					else
						glTexImage2D(GL_TEXTURE_2D, 0, texture_format, bmp->w, bmp->h, 0, GL_RGB, GL_UNSIGNED_BYTE, bmp->pixels);
					break;
				case 32:
					if (build_mipmaps && !can_useGenMipMaps) // Software mipmaps generation
						gluBuild2DMipmaps(GL_TEXTURE_2D, texture_format, bmp->w, bmp->h, GL_RGBA, GL_UNSIGNED_BYTE, bmp->pixels);
					else
						glTexImage2D(GL_TEXTURE_2D, 0, texture_format, bmp->w, bmp->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, bmp->pixels);
					break;
				default:
					LOG_DEBUG("SDL_Surface format not supported by texture loader: " << (int)bmp->format->BitsPerPixel << " bpp");
			}

			if (g_useGenMipMaps && glGenerateMipmapEXT && build_mipmaps)
				glGenerateMipmapEXT(GL_TEXTURE_2D);
		}
		else // Generate mipmaps here since other methods are unreliable
		{
			int w = bmp->w << 1;
			int h = bmp->h << 1;
			int level = 0;
			if (w >= 1 && h >= 1)
				do
				{
					w = Math::Max(w / 2, 1);
					h = Math::Max(h / 2, 1);
					SDL_Surface* tmp = create_surface_ex(bmp->format->BitsPerPixel, w, h);
					stretch_blit(bmp, tmp, 0, 0, bmp->w, bmp->h, 0, 0, w, h);
					switch (tmp->format->BitsPerPixel)
					{
						case 8:
							glTexImage2D(GL_TEXTURE_2D, level, texture_format, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tmp->pixels);
							break;
						case 24:
							glTexImage2D(GL_TEXTURE_2D, level, texture_format, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, tmp->pixels);
							break;
						case 32:
							glTexImage2D(GL_TEXTURE_2D, level, texture_format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp->pixels);
							break;
						default:
							LOG_DEBUG("SDL_Surface format not supported by texture loader: " << (int)tmp->format->BitsPerPixel << " bpp");
					}
					SDL_FreeSurface(tmp);
					++level;
				} while (w > 1 || h > 1);
		}

		if (filter_type == FILTER_NONE || filter_type == FILTER_LINEAR)
			use_mipmapping(true);

		return gl_tex;
	}

	GLuint GFX::create_texture(int w, int h, int filter_type, bool clamp)
	{
		MutexLocker locker(pMutex);

		if (w > max_tex_size || h > max_tex_size)
			return create_texture(Math::Min(w, max_tex_size), Math::Min(h, max_tex_size), filter_type, clamp);

		if (ati_workaround && filter_type != FILTER_NONE && (!Math::IsPowerOfTwo(w) || !Math::IsPowerOfTwo(h)))
			filter_type = FILTER_LINEAR;

		GLuint gl_tex = 0;
		glGenTextures(1, &gl_tex);

		glBindTexture(GL_TEXTURE_2D, gl_tex);

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);

		if (clamp)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}

		switch (filter_type)
		{
			case FILTER_NONE:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				break;
			case FILTER_LINEAR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				break;
			case FILTER_BILINEAR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				break;
			case FILTER_TRILINEAR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				break;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, texture_format, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

		return gl_tex;
	}

	SDL_Surface* GFX::load_image(const String& filename)
	{
		File* vfile = VFS::Instance()->readFile(filename);
		if (vfile)
		{
			SDL_RWops* file = SDL_RWFromMem((void*)vfile->data(), vfile->size());
			SDL_Surface* img = NULL;
			if (Paths::ExtractFileExt(filename).toLower() == ".tga")
				img = IMG_LoadTGA_RW(file);
			else
				img = IMG_Load_RW(file, 0);
			SDL_RWclose(file);

			delete vfile;

			if (img)
			{
				if (img->format->Amask)
					img = convert_format(img);
				else
					img = convert_format_24(img);
			}
			else
				LOG_ERROR(LOG_PREFIX_GFX << "could not load image file: " << filename << " (vfs)");
			return img;
		}
		else
			LOG_ERROR(LOG_PREFIX_GFX << "could not read image file: " << filename << " (vfs)");
		return NULL;
	}

	GLuint GFX::load_texture(const String& file, int filter_type, uint32* width, uint32* height, bool clamp, GLuint texFormat, bool* useAlpha, bool checkSize)
	{
		if (!VFS::Instance()->fileExists(file)) // The file doesn't exist
			return 0;

		const String upfile = ToUpper(file) << " (" << texFormat << "-" << (int)filter_type << ')';
		HashMap<Interfaces::GfxTexture>::Sparse::iterator it = textureIDs.find(upfile);
		if (it != textureIDs.end() && it->tex > 0) // File already loaded
		{
			if (textureLoad[it->tex] > 0)
			{
				++textureLoad[it->tex];
				if (width)
					*width = it->getWidth();
				if (height)
					*height = it->getHeight();
				if (useAlpha)
					*useAlpha = textureAlpha.contains(it->tex);
				return it->tex;
			}
		}

		const bool compressible = texFormat == GL_COMPRESSED_RGB || texFormat == GL_COMPRESSED_RGBA || texFormat == 0;
		String cache_filename = !file.empty() ? String(file) << ".bin" : String();
		cache_filename.replace('/', 'S');
		cache_filename.replace('\\', 'S');
		if (compressible)
		{
			GLuint gltex = load_texture_from_cache(cache_filename, filter_type, width, height, clamp, useAlpha);
			if (gltex)
				return gltex;
		}

		SDL_Surface* bmp = load_image(file);
		if (bmp == NULL)
		{
			LOG_ERROR("Failed to load texture `" << file << "`");
			return 0;
		}

		if (width)
			*width = bmp->w;
		if (height)
			*height = bmp->h;
		bmp = convert_format(bmp);

		if (checkSize)
		{
			const int maxTextureSizeAllowed = lp_CONFIG->getMaxTextureSizeAllowed();
			if (std::max(bmp->w, bmp->h) > maxTextureSizeAllowed)
			{
				SDL_Surface* tmp = shrink(bmp, std::min(bmp->w, maxTextureSizeAllowed), std::min(bmp->h, maxTextureSizeAllowed));
				SDL_FreeSurface(bmp);
				bmp = tmp;
			}
		}

		String ext = Paths::ExtractFileExt(file);
		ext.toLower();
		bool with_alpha = (ext == ".tga" || ext == ".png" || ext == ".tif");
		if (with_alpha)
		{
			with_alpha = false;
			for (int y = 0; y < bmp->h && !with_alpha; ++y)
			{
				for (int x = 0; x < bmp->w && !with_alpha; ++x)
					with_alpha |= geta(SurfaceInt(bmp, x, y)) != 255;
			}
		}
		if (useAlpha)
			*useAlpha = with_alpha;
		if (texFormat == 0)
		{
			set_texture_format(with_alpha ? defaultRGBATextureFormat : defaultRGBTextureFormat);
		}
		else
			set_texture_format(texFormat);
		GLuint gl_tex = make_texture(bmp, filter_type, clamp);
		if (gl_tex)
		{
			textureIDs[upfile] = Interfaces::GfxTexture(gl_tex, bmp->w, bmp->h);
			textureFile[gl_tex] = upfile;
			textureLoad[gl_tex] = 1;
			if (with_alpha)
				textureAlpha.insert(gl_tex);

			if (compressible)
				save_texture_to_cache(cache_filename, gl_tex, bmp->w, bmp->h, with_alpha);
		}
		SDL_FreeSurface(bmp);
		return gl_tex;
	}

	GLuint GFX::load_texture_mask(const String& file, uint32 level, int filter_type, uint32* width, uint32* height, bool clamp)
	{
		if (!VFS::Instance()->fileExists(file)) // The file doesn't exist
			return 0;

		SDL_Surface* bmp = load_image(file);
		if (bmp == NULL)
			return 0; // Operation failed
		if (width)
			*width = bmp->w;
		if (height)
			*height = bmp->h;
		if (bmp->format->BitsPerPixel != 32)
			bmp = convert_format(bmp);
		bool with_alpha = (Paths::ExtractFileExt(file).toLower() == "tga");
		if (with_alpha)
		{
			with_alpha = false;
			for (int y = 0; y < bmp->h && !with_alpha; y++)
				for (int x = 0; x < bmp->w && !with_alpha; x++)
					with_alpha |= (geta(SurfaceInt(bmp, x, y)) != 255);
		}
		else
		{
			for (int y = 0; y < bmp->h; y++)
				for (int x = 0; x < bmp->w; x++)
					SurfaceInt(bmp, x, y) |= makeacol(0, 0, 0, 255);
		}

		for (int y = 0; y < bmp->h; ++y)
		{
			for (int x = 0; x < bmp->w; ++x)
			{
				const uint32 c = SurfaceInt(bmp, x, y);
				if (getr(c) < level && getg(c) < level && getb(c) < level)
				{
					SurfaceInt(bmp, x, y) = makeacol(getr(c), getg(c), getb(c), 0);
					with_alpha = true;
				}
			}
		}
		set_texture_format(with_alpha ? defaultRGBATextureFormat : defaultRGBTextureFormat);
		GLuint gl_tex = make_texture(bmp, filter_type, clamp);
		SDL_FreeSurface(bmp);
		return gl_tex;
	}

	bool GFX::is_texture_in_cache(const String& file)
	{
		return false;
	}

	GLuint GFX::load_texture_from_cache(const String& file, int filter_type, uint32* width, uint32* height, bool clamp, bool* useAlpha)
	{
		return 0;
	}

	void GFX::save_texture_to_cache(String file, GLuint tex, uint32 width, uint32 height, bool useAlpha)
	{
		return;
	}

	uint32 GFX::texture_width(const GLuint gltex)
	{
		GLint width;
		glBindTexture(GL_TEXTURE_2D, gltex);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		return width;
	}

	uint32 GFX::texture_height(const GLuint gltex)
	{
		GLint height;
		glBindTexture(GL_TEXTURE_2D, gltex);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
		return height;
	}

	void GFX::destroy_texture(GLuint& gltex)
	{
		if (gltex) // Test if the texture exists
		{
			// Look for it in the texture table
			HashMap<int, GLuint>::Sparse::iterator it = textureLoad.find(gltex);
			if (it != textureLoad.end())
			{
				--(*it);	 // Update load info
				if (*it > 0) // Still used elsewhere, so don't delete it
				{
					gltex = 0;
					return;
				}
				textureLoad.erase(it);
				textureAlpha.erase(gltex);

				HashMap<String, GLuint>::Sparse::iterator file_it = textureFile.find(gltex);
				if (file_it != textureFile.end())
				{
					if (!file_it->empty())
						textureIDs.erase(*file_it);
					textureFile.erase(file_it);
				}
			}
			glDeleteTextures(1, &gltex); // Delete the OpenGL object
		}
		gltex = 0; // The texture is destroyed
	}

	void GFX::set_alpha_blending()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		alpha_blending_set = true;
	}

	void GFX::unset_alpha_blending()
	{
		glDisable(GL_BLEND);
		alpha_blending_set = false;
	}

	void GFX::ReInitTexSys(bool matrix_reset)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		if (matrix_reset)
		{
			glMatrixMode(GL_TEXTURE);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
		}
	}

	void GFX::ReInitAllTex(bool disable)
	{
		if (MultiTexturing)
		{
			for (unsigned int i = 0; i < 7; ++i)
			{
				glActiveTextureARB(GL_TEXTURE0_ARB + i);
				ReInitTexSys();
				glClientActiveTexture(GL_TEXTURE0_ARB + i);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				if (disable)
					glDisable(GL_TEXTURE_2D);
			}
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glClientActiveTexture(GL_TEXTURE0_ARB);
		}
	}

	void GFX::SetDefState()
	{
		glClearColor(0, 0, 0, 0);
		glShadeModel(GL_SMOOTH);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDepthFunc(GL_LESS);
		glEnable(GL_DEPTH_TEST);
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
		glDisable(GL_BLEND);
		glEnable(GL_LIGHTING);
		ReInitTexSys();
		alpha_blending_set = false;
	}

	uint32 GFX::InterfaceMsg(const uint32 MsgID, const String&)
	{
		if (MsgID != TA3D_IM_GFX_MSG)
			return INTERFACE_RESULT_CONTINUE;
		return INTERFACE_RESULT_CONTINUE;
	}

	void GFX::clearAll()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void GFX::clearScreen()
	{
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void GFX::clearDepth()
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	void GFX::disable_texturing()
	{
		glDisable(GL_TEXTURE_2D);
	}

	void GFX::enable_texturing()
	{
		glEnable(GL_TEXTURE_2D);
	}

	void GFX::preCalculations()
	{
		SDL_GetWindowSize(screen, &width, &height);
		SCREEN_W_HALF = width >> 1;
		SCREEN_H_HALF = height >> 1;
		SCREEN_W_INV = 1.0f / float(width);
		SCREEN_H_INV = 1.0f / float(height);
		SCREEN_W_TO_640 = 640.0f / float(width);
		SCREEN_H_TO_480 = 480.0f / float(height);
	}

	void GFX::PutTextureInsideRect(const GLuint texture, const float x1, const float y1, const float x2, const float y2)
	{
		glBindTexture(GL_TEXTURE_2D, texture);
		glBegin(GL_QUADS);
		//
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(x1, y1);

		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(x2, y1);

		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(x2, y2);

		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(x1, y2);
		//
		glEnd();
	}

	SDL_Surface* GFX::create_surface_ex(int bpp, int w, int h)
	{
		SDL_Surface* surface;

		if (bpp == 8)
		{
			surface = SDL_CreateRGBSurface(0, w, h, 8, 0, 0, 0, 0);
		}
		else
		{
			surface = SDL_CreateRGBSurface(0, w, h, bpp,
			    0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
		}


		if (surface == NULL)
		{
			LOG_CRITICAL(String("Error creating SDL RGB surface: ") << SDL_GetError());
		}

		return surface;
	}

	SDL_Surface* GFX::create_surface(int w, int h)
	{
		return create_surface_ex(32, w, h);
	}

	void reset_keyboard()
	{
		clearKeyboardBuffer();
	}
	void reset_mouse()
	{
	}

	SDL_Surface* GFX::LoadMaskedTextureToBmp(const String& file, const String& filealpha)
	{
		// Load the texture (32Bits)
		SDL_Surface* bmp = gfx->load_image(file);
		LOG_ASSERT(bmp != NULL);

		// Load the mask
		SDL_Surface* alpha = gfx->load_image(filealpha);
		LOG_ASSERT(alpha != NULL);

		// Apply the mask, pixel by pixel
		for (int y = 0; y < bmp->h; ++y)
		{
			for (int x = 0; x < bmp->w; ++x)
				SurfaceByte(bmp, (x << 2) + 3, y) = byte(SurfaceInt(alpha, x, y));
		}

		SDL_FreeSurface(alpha);
		return bmp;
	}

	void GFX::flip() const
	{
		SDL_ShowCursor(SDL_DISABLE);
		SDL_GL_SwapWindow(screen);
	}

	void GFX::multMatrix(const Matrix& m)
	{
		float buf[16];
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				buf[(i * 4) + j] = m.E[i][j];
			}
		}

		glMultMatrixf(buf);
	}

} // namespace TA3D
