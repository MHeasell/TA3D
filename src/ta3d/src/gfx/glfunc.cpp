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

/*--------------------------------------------------------------\
|                          glfunc.h                             |
|      contains functions and variables to set up an OpenGL     |
|  environnement using some OpenGL extensions.                  |
\--------------------------------------------------------------*/

#include <stdafx.h>
#include <TA3D_NameSpace.h>
#include "glfunc.h"
#include <logs/logs.h>

#define CHECK_OPENGL_FUNCTION(extension, function, var)                                                            \
	if ((function) == NULL)                                                                                        \
	{                                                                                                              \
		LOG_WARNING(LOG_PREFIX_OPENGL << "OpenGL reports supporting " #extension " but " #function " is lacking"); \
		(var) = false;                                                                                             \
	}

namespace TA3D
{

	bool MultiTexturing;
	bool g_useTextureCompression = false;
	bool g_useStencilTwoSide = false;
	bool g_useFBO = false;
	bool g_useGenMipMaps = false;
	bool g_useNonPowerOfTwoTextures = false;

	static void checkOpenGLExtensionsPointers()
	{
		if (MultiTexturing)
		{
			CHECK_OPENGL_FUNCTION(MultiTexturing, glActiveTextureARB, MultiTexturing)
			CHECK_OPENGL_FUNCTION(MultiTexturing, glMultiTexCoord2fARB, MultiTexturing)
			CHECK_OPENGL_FUNCTION(MultiTexturing, glClientActiveTextureARB, MultiTexturing)
			if (!MultiTexturing)
				LOG_WARNING(LOG_PREFIX_OPENGL << "MultiTexturing support will be disbaled");
		}
		if (g_useFBO)
		{
			CHECK_OPENGL_FUNCTION(FBO, glDeleteFramebuffersEXT, g_useFBO)
			CHECK_OPENGL_FUNCTION(FBO, glDeleteRenderbuffersEXT, g_useFBO)
			CHECK_OPENGL_FUNCTION(FBO, glBindFramebufferEXT, g_useFBO)
			CHECK_OPENGL_FUNCTION(FBO, glFramebufferTexture2DEXT, g_useFBO)
			CHECK_OPENGL_FUNCTION(FBO, glFramebufferRenderbufferEXT, g_useFBO)
			if (!g_useFBO)
				LOG_WARNING(LOG_PREFIX_OPENGL << "FBO support will be disbaled");
		}
		if (g_useStencilTwoSide)
		{
			CHECK_OPENGL_FUNCTION(StencilTwoSide, glActiveStencilFaceEXT, g_useStencilTwoSide)
			if (!g_useStencilTwoSide)
				LOG_WARNING(LOG_PREFIX_OPENGL << "StencilTwoSide support will be disbaled");
		}
	}

	void installOpenGLExtensions()
	{
		GLenum err = glewInit();
		if (err == GLEW_OK)
			LOG_DEBUG(LOG_PREFIX_OPENGL << "GLEW initialization successful");
		else
		{
			LOG_WARNING(LOG_PREFIX_OPENGL << "GLEW initialization failed!");
			LOG_WARNING(LOG_PREFIX_OPENGL << "GLEW error: " << (const char*)glewGetErrorString(err));
		}
		LOG_DEBUG(LOG_PREFIX_OPENGL << "Using GLEW " << (const char*)glewGetString(GLEW_VERSION));

		MultiTexturing = GLEW_ARB_multitexture;

		g_useStencilTwoSide = GLEW_EXT_stencil_two_side;
		g_useFBO = GLEW_EXT_framebuffer_object;
		g_useGenMipMaps = GLEW_SGIS_generate_mipmap;
		g_useNonPowerOfTwoTextures = GLEW_ARB_texture_non_power_of_two;

		checkOpenGLExtensionsPointers();

		// Extension: multitexturing
		if (glActiveTextureARB != NULL && glMultiTexCoord2fARB != NULL && glClientActiveTextureARB != NULL)
			MultiTexturing = true;
	}

} // namespace TA3D
