#ifndef _TA3D_TOOLS_PATHS_H__
# define _TA3D_TOOLS_PATHS_H__

# include "../stdafx.h"



namespace TA3D
{


    /*! \class Paths
    **
    ** \brief Tools to manage paths for TA3D
    **
    ** This tool must be initialized using Initialize()
    **
    ** \see TA3D::Paths::Initialize()
    */
    class Paths
    {
    public:
        //! Folder separator according to the platform
        static char Separator;
        static String SeparatorAsString;

        //! Folder for resources
        static String Resources;

        //! Folder for Caches
        static String Caches;
        
        //! Folder for Savegames
        static String Savegames;

        //! Folder for logs
        static String Logs;

        //! Folder for preferences
        static String Preferences;

        //! Configuration file
        static String ConfigFile;

        //! Folder for screenshots
        static String Screenshots;


    public:
        /*!
        ** \brief Test if a file/folder exists
        ** \param p The folder/filename to test
        ** \return True if it exists, false otherwise
        */
        static bool Exists(const String& p);


        /*!
        ** \brief Create Path Recursively
        **
        ** \param p The path to create if it does not exist
        ** return True if the operation succeeded, false otherwise
        */
        static bool MakeDir(const String& p);

        /*!
        ** \brief Load all informations about paths
        **
        ** return False if any error has occured
        */
        static bool Initialize();


    }; // class Paths



} // namespace TA3D



#endif // _TA3D_TOOLS_PATHS_H__
