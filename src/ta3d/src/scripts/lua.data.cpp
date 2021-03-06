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

#include <stdafx.h>
#include <TA3D_NameSpace.h>
#include "lua.data.h"
#include "unit.script.h"
#include "lua.thread.h"

namespace TA3D
{
    LuaData::LuaData()
    {
        init();
    }

    LuaData::~LuaData()
    {
        destroy();
    }

	const String &LuaData::getName() const
    {
        return name;
    }

    void LuaData::load(const String &filename)                    // Load a lua chunk
    {
        destroy();

		UnitScript::load(filename);
        lua_State *L = UnitScript::luaVM();

		try
		{
			lua_call(L, 0, 0);
		}
		catch(...)
		{
			LOG_ERROR(LOG_PREFIX_LUA << "error loading '" << filename << "'");
			if (lua_gettop(L) > 0 && !lua_isnoneornil(L, -1) && lua_tostring( L, -1 ) != NULL && strlen(lua_tostring( L, -1 )) > 0)
			{
				LOG_ERROR(LOG_PREFIX_LUA << __FILE__ << " l." << __LINE__);
				LOG_ERROR(LOG_PREFIX_LUA << lua_tostring(L, -1));
			}
			return;
		};

        lua_getglobal(L, "__name");
        name = lua_isstring(L, -1) ? String(lua_tostring(L, -1)) : String();
        lua_pop(L, 1);

        lua_getglobal(L, name.c_str());
        lua_getfield(L, -1, "__piece_list");
        if (lua_istable(L, -1))
        {
            piece_name.resize(lua_objlen(L, -1));
			for(uint32 i = 1 ; i <= piece_name.size() ; ++i)
            {
                lua_rawgeti(L, -1, i);
				piece_name[i - 1] = ToLower( lua_tostring(L, -1) );
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 2);
    }

    void LuaData::init()
    {
        name.clear();
        piece_name.clear();
    }

    void LuaData::destroy()
    {
        init();
    }

    int LuaData::identify(const String &name)
    {
		const String query = ToLower(name);
		for(uint32 i = 0 ; i < piece_name.size() ; ++i)
            if (piece_name[i] == query)
                return i;
        return -1;
    }
}
