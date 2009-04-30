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
#ifndef __TA3D_INGAME_MENUS_SOLO_H__
# define __TA3D_INGAME_MENUS_SOLO_H__

# include "base.h"


namespace TA3D
{
namespace Menus
{


    /*! \class Solo
    **
    ** \brief The Solo menu, to launch a solo campaign, a skirmish, or load a savegame
    */
    class Solo : public Abstract
    {
    public:
        /*!
        ** \brief Execute an instance of MainMenu
        */
        static bool Execute();

    public:
        //! \name Constructor & Destructor
        //@{
        //! Default constructor
        Solo();
        //! Destructor
        virtual ~Solo();
        //@}

    protected:
        virtual bool doInitialize();
        virtual void doFinalize();
        virtual void waitForEvent();
        virtual bool maySwitchToAnotherMenu();

    private:
        bool doDisplayAllSavegames();
        /*!
        ** \brief Go to the loading room
        ** \return Always equals to false
        */
        bool doGoMenuLoadSingleGame();

        /*!
        ** \brief Go to Campaign menu
        ** \return Always equals to false
        */
        bool doGoMenuCompaign();

        /*!
        ** \brief Go to the Skirmish menu
        ** \return Always equals to false
        */
        bool doGoMenuSkirmish();

    }; // class Solo


} // namespace Menus
} // namespace TA3D

#endif // __TA3D_INGAME_MENUS_SOLO_H__
