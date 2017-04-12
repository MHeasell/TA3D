#ifndef __TA3D_CONFIG_FILE_FROM_CMAKE_H__
# define __TA3D_CONFIG_FILE_FROM_CMAKE_H__


//! Define to the address where bug reports for this package should be sent
# define PACKAGE_BUGREPORT "@PACKAGE_BUGREPORT@"

//! Define to the full name and version of this package
# define PACKAGE_STRING "@PACKAGE_STRING@"

//! Define to the version of this package
# define PACKAGE_VERSION "@PACKAGE_VERSION@"

//! Define to the version string of this package
# define TA3D_ENGINE_VERSION "@TA3D_ENGINE_VERSION@"


//! The hi version for TA3D
# define TA3D_VERSION_HI  @TA3D_VERSION_HI@
//! The lo version for TA3D
# define TA3D_VERSION_LO  @TA3D_VERSION_LO@
//! The patch version for TA3D
# define TA3D_VERSION_PATCH @TA3D_VERSION_PATCH@

//! The current git commit
# define TA3D_CURRENT_COMMIT   "@GIT_COMMIT@"


//! The website
# define TA3D_WEBSITE                  "@TA3D_WEBSITE@"
//! The forum
# define TA3D_WEBSITE_FORUMS           "@TA3D_WEBSITE_FORUM@"
//! URL for a new bug report
# define TA3D_WEBSITE_NEW_BUGREPORT    "@TA3D_WEBSITE_NEW_BUGREPORT@"


//! Default server hostname
# define TA3D_DEFAULT_SERVER_HOSTNAME  "netserver.ta3d.org"



/*!
** \brief The hard limit for the number of players
*/
# define TA3D_PLAYERS_HARD_LIMIT   10


#cmakedefine __FTGL__lower__

#endif // __TA3D_CONFIG_FILE_FROM_CMAKE_H__
