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

/*-----------------------------------------------------------------------------\
|                                      ai.h                                    |
|       Ce module est responsable de l'intelligence artificielle               |
|                                                                              |
\-----------------------------------------------------------------------------*/

#ifndef TA3D_XX_AI_H__
# define TA3D_XX_AI_H__

# include "ai.controller.h"
# include "../scripts/ai.script.h"

# define TA3D_AI_FILE_EXTENSION  ".ai"


namespace TA3D
{

#define	AI_TYPE_EASY		0x0
#define AI_TYPE_MEDIUM		0x1
#define AI_TYPE_HARD		0x2
#define AI_TYPE_BLOODY		0x3
#define AI_TYPE_LUA         0x4

    class AI_PLAYER			// Class to manage players controled by AI
    {
    private:
        String			name;			// Attention faudrait pas qu'il se prenne pour quelqu'un!! -> indique aussi le fichier correspondant à l'IA (faut sauvegarder les cervelles)
        AI_CONTROLLER   *ai_controller;
        AiScript        *ai_script;
        int             type;
        int             ID;
        String          AI;

    public:

        AI_PLAYER();

        ~AI_PLAYER();

        void monitor();

        void stop();
        void destroy();

        void setType(int type);
        int getType();

        void setAI(const String &AI);

        void setPlayerID(int id);
        int getPlayerID();

        void changeName(const String& newName);		// Change AI's name (-> creates a new file)

        void save();
        void load(const String& filename, const int id = 0);

        static String::Vector getAvailableAIs();
    };


} // namespace TA3D

#endif // TA3D_XX_AI_H__
