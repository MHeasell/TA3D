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
  |                                      weapons.cpp                                   |
  |   Ce module contient les structures, classes et fonctions nécessaires à la lecture |
  | des fichiers tdf du jeu totalannihilation concernant les armes utilisées par les   |
  | unités du jeu.                                                                     |
  |                                                                                    |
  \-----------------------------------------------------------------------------------*/

#ifdef CWDEBUG
#include <libcwd/sys.h>
#include <libcwd/debug.h>
#endif
#include "weapons.h"
#include "../../gfx/fx.h"


namespace TA3D
{


    void load_weapons(void (*progress)(float percent,const String &msg))				// Charge toutes les armes
    {
        List<String> file_list;
        HPIManager->GetFilelist(ta3d_sidedata.weapon_dir + "*.tdf", &file_list);

        int n = 0;

        for (List<String>::const_iterator cur_file = file_list.begin(); cur_file != file_list.end(); ++cur_file)
        {
            if(progress!=NULL && !(n & 0xF))
                progress((250.0f+n*50.0f/(file_list.size()+1))/7.0f,TRANSLATE("Loading weapons"));
            ++n;

            uint32 file_size=0;
            byte *data=HPIManager->PullFromHPI(cur_file->c_str(),&file_size);
            if(data)
            {
                weapon_manager.load_tdf((char*)data,file_size);
                free(data);
            }
        }

        fx_manager.fx_data=HPIManager->PullFromHPI("anims\\fx.gaf");			// Load weapon animation data and stores it into a cache since it's often used
        if(fx_manager.fx_data)
        {
            weapon_manager.cannonshell.load_gaf(fx_manager.fx_data,get_gaf_entry_index(fx_manager.fx_data,"cannonshell"));
            weapon_manager.cannonshell.convert(false,true);
            weapon_manager.cannonshell.clean();
        }
    }



} // namesapce TA3D

