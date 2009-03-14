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
#include "../UnitEngine.h"
#include "cob.vm.h"

/*!
 * \brief Display the executed code if enabled
 */
#define DEBUG_USE_PRINT_CODE 0

#if DEBUG_USE_PRINT_CODE == 1
#   define DEBUG_PRINT_CODE(X)  if (print_code) LOG_DEBUG(X)
#else
#   define DEBUG_PRINT_CODE(X)
#endif


#define SQUARE(X)  ((X)*(X))

#define MAX_CODE_PER_TICK			100

namespace TA3D
{
    void COB_VM::load( SCRIPT_DATA *data )
    {
        COB_SCRIPT *p_script = dynamic_cast<COB_SCRIPT*>(data);

        destroy();

        if (p_script)
        {
            global_env = new SCRIPT_ENV;
            script = p_script;
        }
    }

    COB_VM::COB_VM()
    {
        init();
    }

    COB_VM::~COB_VM()
    {
        destroy();
        if (caller == NULL && global_env)
            delete global_env;
    }

    void COB_VM::init()
    {
        caller = NULL;
        script = NULL;
        global_env = NULL;
        sStack.clear();
        local_env.clear();
    }

    void COB_VM::destroy()
    {
        script = NULL;
        sStack.clear();
        local_env.clear();
    }

    int COB_VM::run(float dt, bool alone)  // Run the script
    {
        return run(dt, alone, NULL, 0);
    }

    int COB_VM::run(float dt, bool alone, int *pParam, int nParam)  // Run the script
    {
        if (script == NULL)
        {
            running = false;
            cur.clear();
            local_env.clear();
            return -1;	// No associated script !!
        }

        MutexLocker mLocker( pMutex );

        if (caller == NULL && !alone)
        {
            clean();
            for(int i = childs.size() - 1 ; i >= 0 ; i--)
            {
                int sig = childs[i]->run(dt);
                if (sig > 0 || sig < -3)
                    return sig;
            }
        }

        if (!is_running())   return -1;
        if (!running)   return 0;
        if (waiting)    return -3;

        if (sleeping)
        {
            sleep_time -= dt;
            if (sleep_time <= 0.0f)
                sleeping = false;
            if (sleeping)
                return -2; 			// Keep sleeping
        }

        if (cur.empty())        // Call stack empty
        {
            running = false;
            cur.clear();
            local_env.clear();
            return -1;
        }

        sint16 script_id = (cur.top() & 0xFF);			// Récupère l'identifiant du script en cours d'éxecution et la position d'éxecution
        sint16 pos = (cur.top() >> 8);
        if (script_id < 0 || script_id >= script->nb_script)
        {
            running = false;
            cur.clear();
            local_env.clear();
            return -1;		// Erreur, ce n'est pas un script repertorié
        }

        UNIT *pUnit = &(units.unit[ unitID ]);

        float divisor(I2PWR16);
        float div = 0.5f * divisor;
        bool done = false;
        int nb_code = 0;

#if DEBUG_USE_PRINT_CODE == 1
//        bool print_code = false;
        //bool	print_code = String::ToLower( unit_manager.unit_type[type_id]->Unitname ) == "armtship" && (String::ToLower( script->name[script_id] ) == "transportpickup" || String::ToLower( script->name[script_id] ) == "boomcalc" );
        bool	print_code = script->names[script_id] == "MOTIONCONTROL";
#endif

        do
        {
            //			uint32 code = script->script_code[script_id][pos];			// Lit un code
            //			pos++;
            nb_code++;
            if (nb_code >= MAX_CODE_PER_TICK) done = true;			// Pour éviter que le programme ne fige à cause d'un script
            //			switch(code)			// Code de l'interpréteur
            switch(script->script_code[script_id][pos++])
            {
                case SCRIPT_MOVE_OBJECT:
                    {
                        DEBUG_PRINT_CODE("MOVE_OBJECT");
                        int obj = script->script_code[script_id][pos++];
                        int axis = script->script_code[script_id][pos++];
                        int v1 = sStack.pop();
                        int v2 = sStack.pop();
                        pUnit->script_move_object(obj, axis, v1 * div, v2 * div);
                        break;
                    }
                case SCRIPT_WAIT_FOR_TURN:
                    {
                        DEBUG_PRINT_CODE("WAIT_FOR_TURN");
                        int obj = script->script_code[script_id][pos++];
                        int axis = script->script_code[script_id][pos++];
                        if (pUnit->script_is_turning(obj, axis))
                        {
                            pos -= 3;
                            done = true;
                        }
                        break;
                    }
                case SCRIPT_RANDOM_NUMBER:
                    {
                        DEBUG_PRINT_CODE("RANDOM_NUMBER");
                        int high = sStack.pop();
                        int low = sStack.pop();
                        sStack.push(((sint32)(Math::RandFromTable() % (high - low + 1))) + low);
                        break;
                    }
                case SCRIPT_GREATER_EQUAL:
                    {
                        DEBUG_PRINT_CODE("GREATER_EQUAL");
                        int v2 = sStack.pop();
                        int v1 = sStack.pop();
                        sStack.push(v1 >= v2 ? 1 : 0);
                        break;
                    }
                case SCRIPT_GREATER:
                    {
                        DEBUG_PRINT_CODE("GREATER");
                        int v2 = sStack.pop();
                        int v1 = sStack.pop();
                        sStack.push(v1 > v2 ? 1 : 0);
                        break;
                    }
                case SCRIPT_LESS:
                    {
                        DEBUG_PRINT_CODE("LESS");
                        int v2 = sStack.pop();
                        int v1 = sStack.pop();
                        sStack.push(v1 < v2 ? 1 : 0);
                        break;
                    }
                case SCRIPT_EXPLODE:
                    {
                        DEBUG_PRINT_CODE("EXPLODE");
                        int obj = script->script_code[script_id][pos++];
                        int explosion_type = sStack.pop();
                        pUnit->script_explode(obj, explosion_type);
                        break;
                    }
                case SCRIPT_TURN_OBJECT:
                    {
                        DEBUG_PRINT_CODE("TURN_OBJECT");
                        int obj = script->script_code[script_id][pos++];
                        int axis = script->script_code[script_id][pos++];
                        int v1 = sStack.pop();
                        int v2 = sStack.pop();
                        pUnit->script_turn_object(obj, axis, v1 * TA2DEG, v2 * TA2DEG);
                        break;
                    }
                case SCRIPT_WAIT_FOR_MOVE:
                    {
                        DEBUG_PRINT_CODE("WAIT_FOR_MOVE");
                        int obj = script->script_code[script_id][pos++];
                        int axis = script->script_code[script_id][pos++];
                        if (pUnit->script_is_moving(obj, axis))
                        {
                            pos -= 3;
                            done = true;
                        }
                        break;
                    }
                case SCRIPT_CREATE_LOCAL_VARIABLE:
                    {
                        DEBUG_PRINT_CODE("CREATE_LOCAL_VARIABLE");
                        break;
                    }
                case SCRIPT_SUBTRACT:
                    {
                        DEBUG_PRINT_CODE("SUBSTRACT");
                        int v1 = sStack.pop();
                        int v2 = sStack.pop();
                        sStack.push(v2 - v1);
                        break;
                    }
                case SCRIPT_GET_VALUE_FROM_PORT:
                    {
                        DEBUG_PRINT_CODE("GET_VALUE_FROM_PORT:");
                        int value = sStack.pop();
                        DEBUG_PRINT_CODE(value);
                        int param[2];
                        switch(value)
                        {
                        case ATAN:
                            param[1] = sStack.pop();
                            param[0] = sStack.pop();
                            break;
                        case HYPOT:
                            param[1] = sStack.pop();
                            param[0] = sStack.pop();
                            break;
                        };
                        sStack.push( pUnit->script_get_value_from_port(value, param) );
                    }
                    break;
                case SCRIPT_LESS_EQUAL:
                    {
                        DEBUG_PRINT_CODE("LESS_EQUAL");
                        int v2 = sStack.pop();
                        int v1 = sStack.pop();
                        sStack.push(v1 <= v2 ? 1 : 0);
                        break;
                    }
                case SCRIPT_SPIN_OBJECT:
                    {
                        DEBUG_PRINT_CODE("SPIN_OBJECT");
                        int obj = script->script_code[script_id][pos++];
                        int axis = script->script_code[script_id][pos++];
                        int v1 = sStack.pop();
                        int v2 = sStack.pop();
                        pUnit->script_spin_object(obj, axis, v1 * TA2DEG, v2 * TA2DEG);
                    }
                    break;
                case SCRIPT_SLEEP:
                    {
                        DEBUG_PRINT_CODE("SLEEP");
                        pause( sStack.pop() * 0.001f );
                        done = true;
                        break;
                    }
                case SCRIPT_MULTIPLY:
                    {
                        DEBUG_PRINT_CODE("MULTIPLY");
                        int v1 = sStack.pop();
                        int v2 = sStack.pop();
                        sStack.push(v1 * v2);
                        break;
                    }
                case SCRIPT_CALL_SCRIPT:
                    {
                        DEBUG_PRINT_CODE("CALL_SCRIPT");
                        int function_id = script->script_code[script_id][pos++];			// Lit un code
                        int num_param = script->script_code[script_id][pos++];			// Lit un code
                        cur.top() = script_id + (pos<<8);
                        cur.push( function_id );
                        local_env.push( SCRIPT_ENV() );
                        local_env.top().resize( num_param );
                        for(int i = num_param - 1 ; i >= 0 ; i--)		// Lit les paramètres
                            local_env.top()[i] = sStack.pop();
                        pos = 0;
                        script_id = function_id;
                        break;
                    }
                case SCRIPT_SHOW_OBJECT:
                    {
                        DEBUG_PRINT_CODE("SHOW_OBJECT");
                        pUnit->script_show_object(script->script_code[script_id][pos++]);
                        break;
                    }
                case SCRIPT_EQUAL:
                    {
                        DEBUG_PRINT_CODE("EQUAL");
                        int v1 = sStack.pop();
                        int v2 = sStack.pop();
                        sStack.push(v1 == v2 ? 1 : 0);
                        break;
                    }
                case SCRIPT_NOT_EQUAL:
                    {
                        DEBUG_PRINT_CODE("NOT_EQUAL");
                        int v1 = sStack.pop();
                        int v2 = sStack.pop();
                        sStack.push(v1 != v2 ? 1 : 0);
                        break;
                    }
                case SCRIPT_IF:
                    {
                        DEBUG_PRINT_CODE("IF");
                        if (sStack.pop() != 0)
                            pos++;
                        else
                        {
                            int target_offset = script->script_code[script_id][pos];        // Lit un code
                            pos = target_offset - script->dec_offset[script_id];            // Déplace l'éxecution
                        }
                        break;
                    }
                case SCRIPT_HIDE_OBJECT:
                    {
                        DEBUG_PRINT_CODE("HIDE_OBJECT");
                        pUnit->script_hide_object(script->script_code[script_id][pos++]);
                        break;
                    }
                case SCRIPT_SIGNAL:
                    {
                        DEBUG_PRINT_CODE("SIGNAL");
                        cur.top() = script_id + (pos<<8);	    // Sauvegarde la position
                        processSignal(sStack.pop());                 // Tue tout les processus utilisant ce signal
                        if (!running)
                            return 0;
                        break;
                    }
                case SCRIPT_DONT_CACHE:
                    {
                        DEBUG_PRINT_CODE("DONT_CACHE");
                        pUnit->script_dont_cache(script->script_code[script_id][pos++]);
                        break;
                    }
                case SCRIPT_SET_SIGNAL_MASK:
                    {
                        DEBUG_PRINT_CODE("SET_SIGNAL_MASK");
                        setSignalMask( sStack.pop() );
                        break;
                    }
                case SCRIPT_NOT:
                    {
                        DEBUG_PRINT_CODE("NOT");
                        sStack.push(!sStack.pop());
                        break;
                    }
                case SCRIPT_DONT_SHADE:
                    {
                        DEBUG_PRINT_CODE("DONT_SHADE");
                        pUnit->script_dont_shade(script->script_code[script_id][pos++]);
                        break;
                    }
                case SCRIPT_EMIT_SFX:
                    {
                        DEBUG_PRINT_CODE("EMIT_SFX:");
                        int smoke_type = sStack.pop();
                        int from_piece = script->script_code[script_id][pos++];
                        DEBUG_PRINT_CODE("smoke_type " << smoke_type << " from " << from_piece);
                        pUnit->script_emit_sfx( smoke_type, from_piece );
                    }
                    break;
                case SCRIPT_PUSH_CONST:
                    {
                        DEBUG_PRINT_CODE("PUSH_CONST (" << script->script_code[script_id][pos] << ")");
                        sStack.push(script->script_code[script_id][pos]);
                        ++pos;
                        break;
                    }
                case SCRIPT_PUSH_VAR:
                    {
                        DEBUG_PRINT_CODE("PUSH_VAR (" << script->script_code[script_id][pos] << ") = "
                                         << local_env.top()[script->script_code[script_id][pos]]);
                        sStack.push(local_env.top()[script->script_code[script_id][pos]]);
                        ++pos;
                        break;
                    }
                case SCRIPT_SET_VAR:
                    {
                        DEBUG_PRINT_CODE("SET_VAR (" << script->script_code[script_id][pos] << ")");
                        int v_id = script->script_code[script_id][pos];
                        if (local_env.top().size() <= v_id)
                            local_env.top().resize(v_id+1);
                        local_env.top()[v_id] = sStack.pop();
                        ++pos;
                        break;
                    }
                case SCRIPT_PUSH_STATIC_VAR:
                    {
                        DEBUG_PRINT_CODE("PUSH_STATIC_VAR");
                        if (script->script_code[script_id][pos] >= global_env->size() )
                            global_env->resize( script->script_code[script_id][pos] + 1 );
                        sStack.push((*global_env)[script->script_code[script_id][pos]]);
                        ++pos;
                        break;
                    }
                case SCRIPT_SET_STATIC_VAR:
                    {
                        DEBUG_PRINT_CODE("SET_STATIC_VAR");
                        if (script->script_code[script_id][pos] >= global_env->size() )
                            global_env->resize( script->script_code[script_id][pos] + 1 );
                        (*global_env)[script->script_code[script_id][pos]] = sStack.pop();
                        ++pos;
                        break;
                    }
                case SCRIPT_OR:
                    {
                        DEBUG_PRINT_CODE("OR");
                        int v1 = sStack.pop(), v2 = sStack.pop();
                        sStack.push(v1 | v2);
                        break;
                    }
                case SCRIPT_START_SCRIPT:				// Transfère l'éxecution à un autre script
                    {
                        DEBUG_PRINT_CODE("START_SCRIPT");
                        int function_id = script->script_code[script_id][pos++];			// Lit un code
                        int num_param = script->script_code[script_id][pos++];			// Lit un code
                        COB_VM *p_cob = fork();
                        if (p_cob)
                        {
                            p_cob->call(function_id, NULL, 0);
                            p_cob->local_env.top().resize( num_param );
                            for(int i = num_param - 1 ; i >= 0 ; i--)		// Lit les paramètres
                                p_cob->local_env.top()[i] = sStack.pop();
                        }
                        else
                        {
                            for (int i = 0 ; i < num_param ; ++i)		// Enlève les paramètres de la pile
                                sStack.pop();
                        }
                        break;
                    }
                case SCRIPT_RETURN:		// Retourne au script précédent
                    {
                        DEBUG_PRINT_CODE("RETURN");
                        cur.pop();

                        if (cur.empty() && pParam)              // Get back parameter values
                            for(int i = 0 ; i < nParam ; i++)
                                pParam[i] = local_env.top().size() > i ? local_env.top()[i] : 0;

                        local_env.pop();
                        int value = sStack.pop();		// Enlève la valeur retournée / Pops the return value
                        setReturnValue( script->names[script_id], value );      // Set the return value
                        if (!cur.empty())
                        {
                            script_id = (cur.top() & 0xFF);			// Récupère l'identifiant du script en cours d'éxecution et la position d'éxecution
                            pos = (cur.top() >> 8);
                        }
                        else
                            done = true;
                        break;
                    }
                case SCRIPT_JUMP:						// Commande de saut
                    {
                        DEBUG_PRINT_CODE("JUMP");
                        int target_offset = script->script_code[script_id][pos];        // Lit un code
                        pos = target_offset - script->dec_offset[script_id];            // Déplace l'éxecution
                        break;
                    }
                case SCRIPT_ADD:
                    {
                        DEBUG_PRINT_CODE("ADD");
                        int v1 = sStack.pop();
                        int v2 = sStack.pop();
                        sStack.push(v1 + v2);
                        break;
                    }
                case SCRIPT_STOP_SPIN:
                    {
                        DEBUG_PRINT_CODE("STOP_SPIN");
                        int obj = script->script_code[script_id][pos++];
                        int axis = script->script_code[script_id][pos++];
                        int v = sStack.pop();
                        pUnit->script_stop_spin(obj, axis, v * TA2DEG);
                        break;
                    }
                case SCRIPT_DIVIDE:
                    {
                        DEBUG_PRINT_CODE("DIVIDE");
                        int v1 = sStack.pop();
                        int v2 = sStack.pop();
                        sStack.push(v2 / v1);
                        break;
                    }
                case SCRIPT_MOVE_PIECE_NOW:
                    {
                        DEBUG_PRINT_CODE("MOVE_PIECE_NOW");
                        int obj = script->script_code[script_id][pos++];
                        int axis = script->script_code[script_id][pos++];
                        int pos = sStack.pop();
                        pUnit->script_move_piece_now(obj, axis, pos * div);
                        break;
                    }
                case SCRIPT_TURN_PIECE_NOW:
                    {
                        DEBUG_PRINT_CODE("TURN_PIECE_NOW");
                        int obj = script->script_code[script_id][pos++];
                        int axis = script->script_code[script_id][pos++];
                        int v = sStack.pop();
                        pUnit->script_turn_piece_now(obj, axis, v * TA2DEG);
                        break;
                    }
                case SCRIPT_CACHE:
                    DEBUG_PRINT_CODE("CACHE");
                    pUnit->script_cache(script->script_code[script_id][pos++]);
                    break;	//added
                case SCRIPT_COMPARE_AND:
                    {
                        DEBUG_PRINT_CODE("COMPARE_AND");
                        int v1 = sStack.pop();
                        int v2 = sStack.pop();
                        sStack.push(v1 && v2);
                        break;
                    }
                case SCRIPT_COMPARE_OR:
                    {
                        DEBUG_PRINT_CODE("COMPARE_OR");
                        int v1 = sStack.pop();
                        int v2 = sStack.pop();
                        sStack.push(v1 || v2);
                        break;
                    }
                case SCRIPT_CALL_FUNCTION:
                    {
                        DEBUG_PRINT_CODE("CALL_FUNCTION");
                        int function_id = script->script_code[script_id][pos++];			// Lit un code
                        int num_param = script->script_code[script_id][pos++];			// Lit un code
                        cur.top() = script_id + (pos << 8);
                        local_env.push( SCRIPT_ENV() );
                        for(int i = num_param - 1 ; i >= 0 ; i--)		// Lit les paramètres
                            local_env.top()[i] = sStack.pop();
                        done = true;
                        pos = 0;
                        script_id = function_id;
                        cur.push( script_id + (pos << 8) );
                        break;
                    }
                case SCRIPT_GET:
                    {
                        DEBUG_PRINT_CODE("GET *");
                        sStack.pop();
                        sStack.pop();
                        int v2 = sStack.pop();
                        int v1 = sStack.pop();
                        int val = sStack.pop();
                        sStack.push( pUnit->script_get(val, v1, v2) );
                        break;	//added
                    }
                case SCRIPT_SET_VALUE:
                    {
                        DEBUG_PRINT_CODE("SET_VALUE *:");
                        int v1 = sStack.pop();
                        int v2 = sStack.pop();
                        DEBUG_PRINT_CODE(v1 << " " << v2);
                        pUnit->script_set_value( v2, v1 );
                    }
                    break;	//added
                case SCRIPT_ATTACH_UNIT:
                    {
                        DEBUG_PRINT_CODE("ATTACH_UNIT");
                        /*int v3 =*/ sStack.pop();
                        int v2 = sStack.pop();
                        int v1 = sStack.pop();
                        pUnit->script_attach_unit(v1, v2);
                        break;	//added
                    }
                case SCRIPT_DROP_UNIT:
                    {
                        DEBUG_PRINT_CODE("DROP_UNIT *");
                        int v1 = sStack.pop();
                        DEBUG_PRINT_CODE("Dropping " << v1);
                        pUnit->script_drop_unit(v1);
                        break;	//added
                    }
                default:
                    LOG_ERROR("UNKNOWN " << script->script_code[script_id][--pos] << ", Stopping script");
                    {
                        cur.pop();

                        if (cur.empty() && !local_env.empty() && pParam)              // Get back parameter values
                            for(int i = 0 ; i < nParam ; i++)
                                pParam[i] = local_env.top().size() > i ? local_env.top()[i] : 0;

                        local_env.pop();
                    }
                    if (!cur.empty())
                    {
                        script_id = (cur.top() & 0xFF);			// Récupère l'identifiant du script en cours d'éxecution et la position d'éxecution
                        pos = (cur.top() >> 8);
                    }
                    else
                        running = false;
                    done = true;
            };
        } while(!done);

        if (!cur.empty())
            cur.top() = script_id + (pos << 8);
        return 0;
    }

    void COB_VM::call(const int functionID, int *parameters, int nb_params)
    {
        if (!script || functionID < 0 || functionID >= script->nb_script || !cur.empty())
            return;

        cur.push( functionID );
        local_env.push( SCRIPT_ENV() );
        running = true;

        if (nb_params > 0 && parameters != NULL)
        {
            local_env.top().resize(nb_params);
            for(int i = 0 ; i < nb_params ; i++)
                local_env.top()[i] = parameters[i];
        }
    }

    COB_VM *COB_VM::fork()
    {
        pMutex.lock();

        if (!running && caller == NULL)
        {
            cur.clear();
            local_env.clear();
            waiting = false;
            sleeping = false;
            sleep_time = 0.0f;
            signal_mask = 0;
            pMutex.unlock();
            return this;
        }

        COB_VM *newThread = new COB_VM();

        newThread->script = script;
        newThread->running = false;
        newThread->waiting = false;
        newThread->sleeping = false;
        newThread->sleep_time = 0.0f;
        newThread->caller = (caller != NULL) ? caller : this;
        newThread->global_env = global_env;
        newThread->setUnitID( unitID );
        addThread(newThread);

        pMutex.unlock();
        return newThread;
    }

    COB_VM *COB_VM::fork(const String &functionName, int *parameters, int nb_params)
    {
        pMutex.lock();

        COB_VM *newThread = fork();
        if (newThread)
            newThread->call(functionName, parameters, nb_params);

        pMutex.unlock();
        return newThread;
    }

    void COB_VM::call(const String &functionName, int *parameters, int nb_params)
    {
        call( script->findFromName( functionName ), parameters, nb_params );
    }

    int COB_VM::execute(const String &functionName, int *parameters, int nb_params)
    {
        int params[1] = {-1};
        if (parameters == NULL || nb_params == 0)
        {
            parameters = params;
            nb_params = 1;
        }
        COB_VM *cob_thread = fork( functionName, parameters, nb_params);
        if (cob_thread)
        {
            int res = -1;
            while( cob_thread->running )
                res = cob_thread->run(0.0f, true, parameters, nb_params);
            cob_thread->kill();
            return parameters[nb_params-1];
        }
        return 0;
    }

    void COB_VM::setUnitID(uint32 ID)
    {
        unitID = ID;
    }

    int COB_VM::getNbPieces()
    {
        if (script == NULL)
            return 0;
        return script->nb_piece;
    }

    void COB_VM::dumpDebugInfo()
    {
        if (caller)
        {
            caller->dumpDebugInfo();
            return;
        }
        LOG_DEBUG(LOG_PREFIX_SCRIPT << "COB_VM::dumpDebufInfo :");
        LOG_DEBUG(LOG_PREFIX_SCRIPT << childs.size() << " child threads");
        if (running)
            LOG_DEBUG(LOG_PREFIX_SCRIPT << "main thread running : " << script->names[cur.top() & 0xFF]);
        for(int i = 0 ; i < childs.size() ; i++)
            if (childs[i]->is_running())
            {
                String state;
                if (childs[i]->is_waiting())
                    state << " (waiting)";
                if (childs[i]->is_sleeping())
                    state << " (sleeping = " << (dynamic_cast<COB_VM*>(childs[i]))->sleep_time << ")";
                LOG_DEBUG(LOG_PREFIX_SCRIPT << "child thread " << i << " running : " << script->names[(dynamic_cast<COB_VM*>(childs[i]))->cur.top() & 0xFF] << state);
            }
    }

    void COB_VM::save_thread_state(gzFile file)
    {
        if (caller == NULL)
        {
            gzputc(file, 1);
            int t = global_env->size();
            gzwrite(file, &t, sizeof(t));
            for(int i = 0 ; i < t ; i++)
                gzwrite(file, &((*global_env)[i]), sizeof(int));
        }
        else
            gzputc(file, 0);

        int t = cur.size();
        gzwrite(file, &t, sizeof(t));
        for(int i = 0 ; i < t ; i++)
            gzwrite(file, &(cur[i]), sizeof(int));

        t = sStack.size();
        gzwrite(file, &t, sizeof(t));
        for(int i = 0 ; i < t ; i++)
            gzwrite(file, &(sStack[i]), sizeof(int));

        t = local_env.size();
        gzwrite(file, &t, sizeof(t));
        for(int i = 0 ; i < t ; i++)
        {
            int f = local_env[i].size();
            gzwrite(file, &f, sizeof(t));
            for(int e = 0 ; e < f ; e++)
                gzwrite(file, &(local_env[i][e]), sizeof(int));
        }
    }

    void COB_VM::restore_thread_state(gzFile file)
    {
        if (gzgetc(file))
        {
            int t;
            gzread(file, &t, sizeof(t));
            if (global_env == NULL)
                global_env = new SCRIPT_ENV;
            global_env->resize(t);
            for(int i = 0 ; i < t ; i++)
                gzread(file, &((*global_env)[i]), sizeof(int));
        }

        int t;
        gzread(file, &t, sizeof(t));
        cur.clear();
        for(int i = 0 ; i < t ; i++)
        {
            int v;
            gzread(file, &v, sizeof(int));
            cur.push(v);
        }

        gzread(file, &t, sizeof(t));
        sStack.clear();
        for(int i = 0 ; i < t ; i++)
        {
            int v;
            gzread(file, &v, sizeof(int));
            sStack.push(v);
        }

        gzread(file, &t, sizeof(t));
        local_env.clear();
        for(int i = 0 ; i < t ; i++)
        {
            int f;
            gzread(file, &f, sizeof(t));
            local_env.push( SCRIPT_ENV() );
            local_env.top().resize(f);
            for(int e = 0 ; e < f ; e++)
                gzread(file, &(local_env.top()[e]), sizeof(int));
        }
    }
}
