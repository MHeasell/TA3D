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

#ifndef __TA3D_SCRIPTS_LUA_CHUNK_H__
# define __TA3D_SCRIPTS_LUA_CHUNK_H__

# include <stdafx.h>
# include <misc/string.h>
# include "../lua/lua.hpp"
# include "script.data.h"


namespace TA3D
{



    /*!
    ** This class represents a basic Lua thread without specialization
	** To use it, create a new class that inherits LUA_THREAD
	*/
	class LuaChunk : public ScriptData
	{
	public:
		//! The most suitable smart pointer for the class
		typedef SmartPtr<LuaChunk>	Ptr;

	public:
		//! \name Constructors & Destructor
		//@{
		/*!
		** \brief Default constructor
		*/
		LuaChunk();
		/*!
		** \brief Constructor with default values
		*/
		LuaChunk(lua_State *L, const String& name);
		//! Destructor
		virtual ~LuaChunk();
		//@}

		/*virtual*/ void load(const String &filename);                    // Load a lua chunk
		void save(const String &filename);                    // Save the lua chunk

		int load(lua_State *L);

		void dump(lua_State *L, const String& name);
		const String& getName() const;

		virtual int identify(const String &name);

	protected:
		byte* buffer;
		uint32 size;
		String name;
		String::Vector  piece_name;     // Nom des pièces de l'objet 3d concerné / Name of pieces

	private:
		void init();
		void destroy();

		static int WriterFunc(lua_State* L, const void* p, size_t size, void* u);
	};






} // namespace TA3D

# include "lua.chunk.hxx"

#endif // __TA3D_SCRIPTS_LUA_CHUNK_H__
