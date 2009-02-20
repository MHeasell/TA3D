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
#ifndef __TA3D_INGAME_MENUS_XX_LOADING_H__
# define __TA3D_INGAME_MENUS_XX_LOADING_H__

# include "../../stdafx.h"
# include "../../threads/mutex.h"
# include <list>

namespace TA3D
{
namespace Menus
{


    /*! \class Loading
    **
    ** \brief The window loading for TA3D
    **
    ** This class is thread-safe
    */
    class Loading
    {
    public:
        //! \name Constructors & Destructor
        //@{
        //! Default constructor
        Loading();
        /*!
        ** \brief Constructor
        ** \param the count of tasks to complete
        */
        Loading(const float maxTasks);
        //! Destructor
        ~Loading();
        //@}


        /*!
        ** \brief Set the count of tasks to complete
        */
        void maxTasks(const float v);
        /*!
        ** \brief Get the count of tasks to complete
        */
        int maxTasks();

        /*!
        ** \brief Get the percent completed
        */
        float percent();

        /*!
        ** \brief Get the caption
        */
        String caption();
        /*!
        ** \brief Set the caption
        */
        void caption(const String& s);

        /*!
        ** \brief Indicate that one or more tasks have been completed
        **
        ** \param n The count of completed tasks
        ** \param relative The count of completed tasks is an absolute value if equals to false
        */
        void progress(const float progression = 1, const bool relative = true);

        /*!
        ** \brief Indicate that one or more tasks have been completed
        **
        ** \param n The count of completed tasks
        ** \param relative The count of completed tasks is an absolute value if equals to false
        */
        void progress(const String& info, const float progression = 1, const bool relative = true);


        //! \name SDL interaction
        //@{

        /*!
        ** \brief Re Draw the entire screen
        **
        ** Nothing will be done if there was no changes since the last call
        ** to this method.
        **
        ** This method must be called from the main thread.
        **
        ** Data could be safely loaded from another thread and the display
        ** done by the main thread.
        */
        void draw();

        //@}


        /*!
        ** \brief Get if informations should be broadcasted to other players
        */
        bool broadcastInfosAboutLoading();
        /*!
        ** \brief Set if informations should be broadcasted to other players
        */
        void broadcastInfosAboutLoading(const bool v);

    private:
        /*!
        ** \brief Indicate that one or more tasks have been completed
        **
        ** This method is not thread-safe
        */
        void doProgress(const float progression, const bool relative);

        /*!
        ** \brief Notice other connected players about the progress
        **
        ** This method must be called from the main thread and is not thread-safe
        */
        void doNoticeOtherPlayers();

        /*!
        ** \brief Load the background texture
        */
        void loadTheBackgroundTexture();

        void initializeDrawing();

        void finalizeDrawing();

    private:
        //! Mutex
        Mutex pMutex;

        //! The number of tasks completed
        float pNbTasksCompleted;
        //! The maximum count of
        float pMaxTasksCompleted;
        //! The current percentage of progression
        float pPercent;
        //! The percent value
        float pLastPercent;

        //! Should broadcast informations
        bool pBroadcastInformations;

        //! The current caption
        String pCaption;
        //! All messages
        String::List pMessages;

        //! The background texture
        GLuint pBackgroundTexture;
        //! The height of the font
        float pCurrentFontHeight;

        //! Start time
        int pStartTime;

    }; // class Loading



} // namespace Menus
} // namespace TA3D

#endif // __TA3D_INGAME_MENUS_XX_LOADING_H__
