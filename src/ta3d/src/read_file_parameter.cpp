#include <logs/logs.h>
#include <misc/tdf.h>
#include <ingame/sidedata.h>
#include <sounds/manager.h>
#include <ingame/menus/setupgame.h>
#include <languages/table.h>
#include "read_file_parameter.h"
#include "cache.h"

namespace TA3D
{
	/*
	** Function: ReadFileParameter
	**    Notes: This function will eventually load a file given as command line parameter
	**             and run given commands. This is used to start a multiplayer game from
	**             an external Lobby client
	*/
	void ReadFileParameter()
	{
		if (!TA3D::lp_CONFIG || TA3D::lp_CONFIG->file_param.empty())
			return;

		LOG_DEBUG("Reading file parameter `" << TA3D::lp_CONFIG->file_param << "`...");

		TDFParser parser(TA3D::lp_CONFIG->file_param);

		String current_mod = TA3D::TA3D_CURRENT_MOD;

		TA3D_CURRENT_MOD = TA3D::lp_CONFIG->last_MOD = parser.pullAsString("TA3D.MOD", current_mod);
		lp_CONFIG->serializedGameData = parser.pullAsString("TA3D.Game Data", TA3D::lp_CONFIG->serializedGameData);

		if (current_mod != TA3D::TA3D_CURRENT_MOD) // Refresh file structure
		{
			Cache::Clear();

			VFS::Instance()->reload();
			ta3dSideData.loadData(); // Refresh side data so we load the correct values
			sound_manager->reset();
			sound_manager->stopMusic();
			sound_manager->loadTDFSounds(true);
			sound_manager->loadTDFSounds(false);
		}

		if (parser.pullAsBool("TA3D.Network game"))
		{
			if (parser.pullAsBool("TA3D.Server")) // Server code
			{
				const String& host_name = parser.pullAsString("TA3D.Server name", TA3D::lp_CONFIG->player_name);
				Menus::SetupGame::Execute(false, host_name); // Start the game in networking mode as server
			}
			else // Client code
			{
				const String& host_name = parser.pullAsString("TA3D.Server name");
				Menus::SetupGame::Execute(true, host_name); // Start the game in networking mode as server
			}
		}
		else if (parser.pullAsBool("TA3D.Local game"))
			Menus::SetupGame::Execute(false, String(), String(), false, parser.pullAsBool("TA3D.Instant start")); // Start the game in local mode

		TA3D::TA3D_CURRENT_MOD = TA3D::lp_CONFIG->last_MOD = current_mod;

		if (current_mod != TA3D::TA3D_CURRENT_MOD) // Refresh file structure
		{
			Cache::Clear(); // Clear the cache

			VFS::Instance()->reload();
			ta3dSideData.loadData(); // Refresh side data so we load the correct values
			sound_manager->reset();
			sound_manager->loadTDFSounds(true);
			sound_manager->loadTDFSounds(false);
		}

		// Update the translation table
		TranslationTable::Update();
	}
}
