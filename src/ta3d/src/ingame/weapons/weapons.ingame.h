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
#ifndef __TA3D_InGameWeapons_ING_H__
# define __TA3D_InGameWeapons_ING_H__

# include "../../stdafx.h"
# include "../../threads/thread.h"
# include "weapons.h"
# include "weapons.single.h"
# include "../../misc/camera.h"


namespace TA3D
{


	/*! \class InGameWeapons
    **
    ** \brief
    */
	class InGameWeapons : public ObjectSync, public Thread
    {
    public:
        //! \name Constructor & Destructor
        //@{
        //! Default constructor
		InGameWeapons();
        //! Destructor
		~InGameWeapons();
        //@}


        /*!
        ** \brief
        ** \param map
        */
        void set_data(MAP* map);

        /*!
        ** \brief
        ** \param real
        */
        void init(bool real = true);

        /*!
        ** \brief
        */
        void destroy();


        /*!
        ** \brief
        ** \param weapon_id
        ** \param shooter
        */
        int add_weapon(int weapon_id,int shooter);

        /*!
        ** \brief
        */
        void move(const float dt,MAP *map);

        /*!
        ** \brief
        */
        void draw(MAP *map = NULL, bool underwater = false);

        /*!
        ** \brief
        */
        void draw_mini(float map_w, float map_h, int mini_w, int mini_h); // Repère les unités sur la mini-carte


    public:
        //! Weapons count
        uint32 nb_weapon;			// Nombre d'armes
        //!
		std::vector< Weapon > weapon;			// Tableau regroupant les armes
        //!
        Gaf::Animation nuclogo;			// Logos des armes atomiques sur la minicarte / Logo of nuclear weapons on minimap

        //!
        std::vector< uint32 > idx_list;
        //!
        std::vector< uint32 > free_idx;

    protected:
        //!
        bool thread_running;
        //!
        bool thread_ask_to_stop;
        //!
        MAP* p_map;
        //!
        void proc(void*);
        //!
        void signalExitThread();

	}; // class InGameWeapons



	extern InGameWeapons weapons;


}

#endif // __TA3D_InGameWeapons_ING_H__
