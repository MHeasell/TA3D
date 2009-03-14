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

#include "../stdafx.h"
#include "unit.script.h"
#include "../UnitEngine.h"

namespace TA3D
{
    UNIT_SCRIPT::UNIT_SCRIPT()
    {
    }

    UNIT_SCRIPT::~UNIT_SCRIPT()
    {
        destroyThread();
    }

    UNIT *lua_currentUnit(lua_State *L)
    {
        lua_getfield(L, LUA_REGISTRYINDEX, "unitID");
        UNIT *p = &(units.unit[lua_tointeger( L, -1 )]);
        lua_pop(L, 1);
        return p;
    }

    int unit_is_turning( lua_State *L )        // is_turning(obj_id, axis_id)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, 1);
        int axis = lua_tointeger(L, 2);
        lua_pushboolean(L, pUnit->script_is_turning(obj, axis));
        return 1;
    }

    int unit_is_moving( lua_State *L )        // is_moving(obj_id, axis_id)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, 1);
        int axis = lua_tointeger(L, 2);
        lua_pushboolean(L, pUnit->script_is_moving(obj, axis));
        return 1;
    }

    int unit_move( lua_State *L )           // move(obj_id, axis_id, target_pos, speed)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, -4);
        int axis = lua_tointeger(L, -3);
        float pos = (float) lua_tonumber(L, -2);
        float speed = (float) lua_tonumber(L, -1);
        lua_pop(L, 4);
        pUnit->script_move_object(obj, axis, pos, speed);
        return 0;
    }

    int unit_explode( lua_State *L )        // explode(obj_id, explosion_type)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, -2);
        int type = lua_tointeger(L, -1);
        lua_pop(L, 2);
        pUnit->script_explode(obj, type);
        return 0;
    }

    int unit_turn( lua_State *L )        // turn(obj_id, axis, angle, speed)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, -4);
        int type = lua_tointeger(L, -3);
        float angle = (float) lua_tonumber(L, -2);
        float speed = (float) lua_tonumber(L, -1);
        lua_pop(L, 4);
        pUnit->script_turn_object(obj, type, angle, speed);
        return 0;
    }

    int unit_get_value_from_port( lua_State *L )        // get_value_from_port(port)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int port = lua_tointeger(L, -1);
        lua_pop(L, 1);

        lua_pushinteger(L, pUnit->script_get_value_from_port(port));
        return 1;
    }

    int unit_show( lua_State *L )        // show(obj_id)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, -1);
        lua_pop(L, 1);
        pUnit->script_show_object(obj);
        return 0;
    }

    int unit_hide( lua_State *L )        // hide(obj_id)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, -1);
        lua_pop(L, 1);
        pUnit->script_hide_object(obj);
        return 0;
    }

    int unit_cache( lua_State *L )          // cache(obj_id)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, -1);
        lua_pop(L, 1);
        pUnit->script_cache(obj);
        return 0;
    }

    int unit_dont_cache( lua_State *L )        // dont_cache(obj_id)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, -1);
        lua_pop(L, 1);
        pUnit->script_dont_cache(obj);
        return 0;
    }

    int unit_dont_shade( lua_State *L )          // dont_shade(obj_id)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, -1);
        lua_pop(L, 1);
        pUnit->script_dont_shade(obj);
        return 0;
    }

    int unit_shade( lua_State *L )          // shade(obj_id)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, -1);
        lua_pop(L, 1);
        pUnit->script_shade(obj);
        return 0;
    }

    int unit_emit_sfx( lua_State *L )           // emit_sfx(smoke_type, from_piece)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int smoke_type = lua_tointeger(L, 1);
        int from_piece = lua_tointeger(L, 2);
        lua_pop(L, 2);
        pUnit->script_emit_sfx(smoke_type, from_piece);
        return 0;
    }

    int unit_spin( lua_State *L )               // spin(obj, axis, speed, (accel))
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, 1);
        int axis = lua_tointeger(L, 2);
        float speed = (float) lua_tonumber(L, 3);
        float accel = lua_isnoneornil(L, 4) ? 0.0f : (float) lua_tonumber(L, 4);
        lua_pop(L, 4);
        pUnit->script_spin_object(obj, axis, speed, accel);
        return 0;
    }

    int unit_stop_spin( lua_State *L )               // stop_spin(obj, axis, (speed))
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, 1);
        int axis = lua_tointeger(L, 2);
        float speed = lua_isnoneornil(L,3) ? 0.0f : (float) lua_tonumber(L, 3);
        lua_pop(L, 3);
        pUnit->script_stop_spin(obj, axis, speed);
        return 0;
    }

    int unit_move_piece_now( lua_State *L )           // move_piece_now(obj, axis, pos)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, -3);
        int axis = lua_tointeger(L, -2);
        float pos = (float) lua_tonumber(L, -1);
        lua_pop(L, 3);
        pUnit->script_move_piece_now(obj, axis, pos);
        return 0;
    }

    int unit_turn_piece_now( lua_State *L )           // turn_piece_now(obj, axis, angle)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int obj = lua_tointeger(L, -3);
        int axis = lua_tointeger(L, -2);
        float angle = (float) lua_tonumber(L, -1);
        lua_pop(L, 3);
        pUnit->script_turn_piece_now(obj, axis, angle);
        return 0;
    }

    int unit_get( lua_State *L )           // get(type, v1, v2)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int type = lua_tointeger(L, 1);
        int v1 = lua_isnoneornil(L,2) ? 0 : lua_tointeger(L, 2);
        int v2 = lua_isnoneornil(L,3) ? 0 : lua_tointeger(L, 3);
        lua_pop(L, lua_gettop(L));
        lua_pushinteger( L, pUnit->script_get(type, v1, v2) );
        return 1;
    }

    int unit_set_value( lua_State *L )           // set_value(type, v)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int type = lua_tointeger(L, 1);
        int v = lua_isboolean(L,2) ? lua_toboolean(L, 2) : lua_tointeger(L, 2);
        lua_pop(L, lua_gettop(L));
        pUnit->script_set_value(type, v);
        return 0;
    }

    int unit_attach_unit( lua_State *L )           // attach_unit(unit_id, piece_id)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int unit_id = lua_tointeger(L, 1);
        int piece_id = lua_tointeger(L, 2);
        lua_pop(L, 2);
        pUnit->script_attach_unit(unit_id, piece_id);
        return 0;
    }

    int unit_drop_unit( lua_State *L )           // drop_unit(unit_id)
    {
        UNIT *pUnit = lua_currentUnit(L);
        int unit_id = lua_tointeger(L, 1);
        lua_pop(L, 1);
        pUnit->script_drop_unit(unit_id);
        return 0;
    }

    int unit_unit_position( lua_State *L )           // unit_position(unit_id)
    {
        int unit_id = lua_tointeger(L, 1);
        lua_pop(L, 1);
        if (unit_id >= 0 && unit_id < units.max_unit && units.unit[unit_id].flags)
            lua_pushvector( L, units.unit[unit_id].Pos );
        else
            lua_pushvector( L, Vector3D() );
        return 1;
    }

    int unit_set_script_value( lua_State *L )       // set_script_value(script_name, value)
    {
        UNIT *pUnit = lua_currentUnit(L);
        String scriptName = lua_isstring(L,1) ? String(lua_tostring(L,1)) : String();
        int value = lua_isboolean(L,2) ? lua_toboolean(L,2) : lua_tointeger(L, 2);
        lua_pop(L, 2);
        if (pUnit && pUnit->script)
            pUnit->script->setReturnValue(scriptName, value);
        return 0;
    }

    int unit_unit_ID( lua_State *L )           // unit_ID()
    {
        UNIT *pUnit = lua_currentUnit(L);
        lua_pushinteger( L, pUnit->idx );
        return 1;
    }

    void UNIT_SCRIPT::register_functions()
    {
        lua_register(L, "is_turning", unit_is_turning );                    // is_turning(obj_id, axis_id)
        lua_register(L, "is_moving", unit_is_moving );                      // is_moving(obj_id, axis_id)
        lua_register(L, "move", unit_move );                                // move(obj_id, axis_id, target_pos, speed)
        lua_register(L, "explode", unit_explode );                          // explode(obj_id, explosion_type)
        lua_register(L, "turn", unit_turn );                                // turn(obj_id, axis, angle, speed)
        lua_register(L, "get_value_from_port", unit_get_value_from_port );  // get_value_from_port(port)
        lua_register(L, "spin", unit_spin );                                // spin(obj_id, axis, speed, (accel))
        lua_register(L, "stop_spin", unit_stop_spin );                      // stop_spin(obj_id, axis, (speed))
        lua_register(L, "show", unit_show );                                // show(obj_id)
        lua_register(L, "hide", unit_hide );                                // hide(obj_id)
        lua_register(L, "emit_sfx", unit_emit_sfx );                        // emit_sfx(smoke_type, from_piece)
        lua_register(L, "move_piece_now", unit_move_piece_now );            // move_piece_now(obj, axis, pos)
        lua_register(L, "turn_piece_now", unit_turn_piece_now );            // turn_piece_now(obj, axis, angle)
        lua_register(L, "get", unit_get );                                  // get(type, v1, v2)
        lua_register(L, "set_value", unit_set_value );                      // set_value(type, v)
        lua_register(L, "set", unit_set_value );                            // set(type, v)
        lua_register(L, "attach_unit", unit_attach_unit );                  // attach_unit(unit_id, piece_id)
        lua_register(L, "drop_unit", unit_drop_unit );                      // drop_unit(unit_id)
        lua_register(L, "unit_position", unit_unit_position );              // unit_position(unit_id) = vector(x,y,z)
        lua_register(L, "unit_ID", unit_unit_ID );                          // unit_ID()
        lua_register(L, "cache", unit_cache );                              // cache(obj_id)
        lua_register(L, "dont_cache", unit_dont_cache );                    // dont_cache(obj_id)
        lua_register(L, "shade", unit_shade );                              // shade(obj_id)
        lua_register(L, "dont_shade", unit_dont_shade );                    // dont_shade(obj_id)
        lua_register(L, "set_script_value", unit_set_script_value );        // set_script_value(script_name, value)
    }

    void UNIT_SCRIPT::register_info()
    {
        lua_pushinteger(L, unitID);
        lua_setfield(L, LUA_REGISTRYINDEX, "unitID");
    }

    void UNIT_SCRIPT::setUnitID(uint32 ID)
    {
        unitID = ID;
        lua_pushinteger(L, unitID);
        lua_setfield(L, LUA_REGISTRYINDEX, "unitID");
    }

    int UNIT_SCRIPT::getNbPieces()
    {
        int nb_piece = 0;
        lua_getglobal(L, "__piece_list");
        if (!lua_isnil(L, -1))
            nb_piece = lua_objlen(L, -1);
        lua_pop(L, 1);
        return nb_piece;
    }

    void UNIT_SCRIPT::load(SCRIPT_DATA *data)
    {
        LUA_THREAD::load(data);
        LUA_THREAD::run();              // We need this to register all the functions and pieces ...
    }

    int UNIT_SCRIPT::run(float dt, bool alone)                  // Run the script
    {
        return LUA_THREAD::run(dt, alone);
    }

    //! functions used to call/run Lua functions
    void UNIT_SCRIPT::call(const String &functionName, int *parameters, int nb_params)
    {
        LUA_THREAD::call(functionName, parameters, nb_params);
    }

    int UNIT_SCRIPT::execute(const String &functionName, int *parameters, int nb_params)
    {
        return LUA_THREAD::execute(functionName, parameters, nb_params);
    }

    //! functions used to create new threads sharing the same environment
    LUA_THREAD *UNIT_SCRIPT::fork()
    {
        return LUA_THREAD::fork();
    }

    LUA_THREAD *UNIT_SCRIPT::fork(const String &functionName, int *parameters, int nb_params)
    {
        return LUA_THREAD::fork(functionName, parameters, nb_params);
    }

    //! functions used to save/restore scripts state
    void UNIT_SCRIPT::save_thread_state(gzFile file)
    {
        gzwrite(file, &unitID, sizeof(unitID));
        LUA_THREAD::save_thread_state(file);
    }

    void UNIT_SCRIPT::restore_thread_state(gzFile file)
    {
        gzread(file, &unitID, sizeof(unitID));
        LUA_THREAD::restore_thread_state(file);
    }
}
