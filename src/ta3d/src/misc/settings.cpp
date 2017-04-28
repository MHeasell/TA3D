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

#include "settings.h"
#include <logs/logs.h>
#include "paths.h"
#include "files.h"
#include <TA3D_NameSpace.h>
#include <ta3dbase.h>
#include <languages/i18n.h>
#include "tdf.h"

namespace TA3D
{
	namespace Settings
	{

		bool Backup(const String& filename)
		{
			LOG_INFO(LOG_PREFIX_SETTINGS << "Making a backup for `" << filename << "`...");
			if (Paths::Files::Copy(filename, String(filename) << ".bak"))
			{
				LOG_INFO(LOG_PREFIX_SETTINGS << "The backup is done.");
				return true;
			}
			LOG_WARNING(LOG_PREFIX_SETTINGS << "Impossible to make the backup.");
			return false;
		}

		bool Save()
		{
			if (!TA3D::lp_CONFIG)
				return false;

			// Make a copy that can be restored if TA3D does not start any more
			TA3D::Settings::Backup(TA3D::Paths::ConfigFile);

			TA3D::lp_CONFIG->last_MOD = TA3D::TA3D_CURRENT_MOD;

			String s;
			s << "// TA3D Settings\n"
			  << "\n"
			  << "[TA3D]\n"
			  << "{\n"
			  << "                  Version = " << TA3D_VERSION_HI << "." << TA3D_VERSION_LO << ";\n"
			  << "                FPS Limit = " << TA3D::lp_CONFIG->fps_limit << "; // <= 0 means `unlimited`\n"
			  << "              Time Factor = " << TA3D::lp_CONFIG->timefactor << ";\n"
			  << "                     FSAA = " << TA3D::lp_CONFIG->fsaa << ";\n"
			  << "               Anisotropy = " << TA3D::lp_CONFIG->anisotropy << ";\n"
			  << "                 Language = " << TA3D::lp_CONFIG->Lang << ";\n"
			  << "             Screen Width = " << TA3D::lp_CONFIG->screen_width << ";\n"
			  << "            Screen Height = " << TA3D::lp_CONFIG->screen_height << ";\n"
			  << "                 Show FPS = " << TA3D::lp_CONFIG->showfps << ";\n"
			  << "           Show Wireframe = " << TA3D::lp_CONFIG->wireframe << ";\n"
			  << "        Show Height Lines = " << TA3D::lp_CONFIG->height_line << ";\n"
			  << "          Show FullScreen = " << TA3D::lp_CONFIG->fullscreen << ";\n"
			  << "     Draw Console Loading = " << TA3D::lp_CONFIG->draw_console_loading << ";\n"
			  << "                Game Data = " << TA3D::lp_CONFIG->serializedGameData << ";\n"
			  << "                 Last MOD = " << TA3D::lp_CONFIG->last_MOD << ";\n"
			  << "              Player name = " << TA3D::lp_CONFIG->player_name << ";\n"
			  << "         Camera Zoom Mode = " << (int)TA3D::lp_CONFIG->camera_zoom << ";\n"
			  << "     Camera Default Angle = " << TA3D::lp_CONFIG->camera_def_angle << ";\n"
			  << "    Camera Default Height = " << TA3D::lp_CONFIG->camera_def_h << ";\n"
			  << "        Camera Zoom Speed = " << TA3D::lp_CONFIG->camera_zoom_speed << ";\n"
			  << "                     Skin = " << TA3D::lp_CONFIG->skin_name << ";\n"
			  << "        Use Texture Cache = " << TA3D::lp_CONFIG->use_texture_cache << ";\n"
			  << "               Net Server = " << TA3D::lp_CONFIG->net_server << "; // default: " << TA3D_DEFAULT_SERVER_HOSTNAME << "\n"
			  << "  Use Texture Compression = " << TA3D::lp_CONFIG->use_texture_compression << ";\n"
			  << "    Right Click Interface = " << TA3D::lp_CONFIG->right_click_interface << ";\n"
			  << "             Sound Volume = " << TA3D::lp_CONFIG->sound_volume << ";\n"
			  << "             Music Volume = " << TA3D::lp_CONFIG->music_volume << ";\n"
			  << "              Grab Inputs = " << TA3D::lp_CONFIG->grab_inputs << ";\n"
			  << "               7z command = " << TA3D::lp_CONFIG->system7zCommand << ";\n"
			  << "           Resource Paths = " << TA3D::lp_CONFIG->resourcePaths << ";\n"
			  << "           Developer Mode = " << TA3D::lp_CONFIG->developerMode << ";\n"
			  << "          Texture Quality = " << TA3D::lp_CONFIG->unitTextureQuality << ";\n"
			  << "}\n";

			if (Paths::Files::SaveToFile(TA3D::Paths::ConfigFile, s))
			{
				LOG_INFO(LOG_PREFIX_SETTINGS << "The settings has been saved.");
				return true;
			}
			LOG_ERROR(LOG_PREFIX_SETTINGS << "Impossible to write settings: `" << TA3D::Paths::ConfigFile << "`");
			return false;
		}

		bool Restore(const String& filename)
		{
			LOG_INFO(LOG_PREFIX_SETTINGS << "Restoring the backup for `" << filename << "`...");
			if (Paths::Files::Copy(String(filename) << ".bak", filename))
			{
				LOG_INFO(LOG_PREFIX_SETTINGS << "The settings have been restored.");
				return true;
			}
			LOG_WARNING(LOG_PREFIX_SETTINGS << "Impossible to restore the settings.");
			return false;
		}

		bool Load()
		{
			TDFParser cfgFile;
			if (!cfgFile.loadFromFile(TA3D::Paths::ConfigFile, false, false, false, true)) // Load this from real file system since it has nothing to do with game content
			{
				LOG_ERROR(LOG_PREFIX_SETTINGS << "Impossible to load the settings from `" << TA3D::Paths::ConfigFile << "`");

				lp_CONFIG->Lang = "english";   // Set default language to English
				lp_CONFIG->first_start = true; // No config file -> guess best settings
				// Apply settings for the current language
				I18N::Instance()->currentLanguage(lp_CONFIG->Lang);

				return false;
			}

			TA3D::lp_CONFIG->fps_limit = cfgFile.pullAsFloat("TA3D.FPS Limit");
			TA3D::lp_CONFIG->timefactor = cfgFile.pullAsFloat("TA3D.Time Factor");

			TA3D::lp_CONFIG->fsaa = sint16(cfgFile.pullAsInt("TA3D.FSAA"));
			TA3D::lp_CONFIG->anisotropy = sint16(cfgFile.pullAsInt("TA3D.Anisotropy", 1));
			TA3D::lp_CONFIG->Lang = cfgFile.pullAsString("TA3D.Language").toLower();
			TA3D::lp_CONFIG->screen_width = uint16(cfgFile.pullAsInt("TA3D.Screen Width"));
			TA3D::lp_CONFIG->screen_height = uint16(cfgFile.pullAsInt("TA3D.Screen Height"));

			TA3D::lp_CONFIG->showfps = cfgFile.pullAsBool("TA3D.Show FPS");
			TA3D::lp_CONFIG->wireframe = cfgFile.pullAsBool("TA3D.Show Wireframe");
			TA3D::lp_CONFIG->height_line = cfgFile.pullAsBool("TA3D.Show Height Lines");
			TA3D::lp_CONFIG->fullscreen = cfgFile.pullAsBool("TA3D.Show FullScreen", false);
			TA3D::lp_CONFIG->draw_console_loading = cfgFile.pullAsBool("TA3D.Draw Console Loading");

			TA3D::lp_CONFIG->serializedGameData = cfgFile.pullAsString("TA3D.Game Data", String());
			TA3D::lp_CONFIG->last_MOD = cfgFile.pullAsString("TA3D.Last MOD", "");

			TA3D::lp_CONFIG->camera_zoom = uint8(cfgFile.pullAsInt("TA3D.Camera Zoom Mode", ZOOM_NORMAL));
			TA3D::lp_CONFIG->camera_def_angle = cfgFile.pullAsFloat("TA3D.Camera Default Angle", 63.44f);
			TA3D::lp_CONFIG->camera_def_h = cfgFile.pullAsFloat("TA3D.Camera Default Height", 200.0f);
			TA3D::lp_CONFIG->camera_zoom_speed = cfgFile.pullAsFloat("TA3D.Camera Zoom Speed", 1.0f);

			TA3D::lp_CONFIG->use_texture_cache = cfgFile.pullAsBool("TA3D.Use Texture Cache", false);

			TA3D::lp_CONFIG->skin_name = cfgFile.pullAsString("TA3D.Skin", "");

			TA3D::lp_CONFIG->net_server = cfgFile.pullAsString("TA3D.Net Server", TA3D_DEFAULT_SERVER_HOSTNAME);

			TA3D::TA3D_CURRENT_MOD = TA3D::lp_CONFIG->last_MOD;

			TA3D::lp_CONFIG->player_name = cfgFile.pullAsString("TA3D.Player name", "player");

			TA3D::lp_CONFIG->use_texture_compression = cfgFile.pullAsBool("TA3D.Use Texture Compression", true);

			TA3D::lp_CONFIG->right_click_interface = cfgFile.pullAsBool("TA3D.Right Click Interface", false);

			TA3D::lp_CONFIG->sound_volume = cfgFile.pullAsInt("TA3D.Sound Volume", 128);
			TA3D::lp_CONFIG->music_volume = cfgFile.pullAsInt("TA3D.Music Volume", 128);

			TA3D::lp_CONFIG->grab_inputs = cfgFile.pullAsBool("TA3D.Grab Inputs", false);

			TA3D::lp_CONFIG->system7zCommand = cfgFile.pullAsString("TA3D.7z command", "7z");
			TA3D::lp_CONFIG->resourcePaths = cfgFile.pullAsString("TA3D.Resource Paths");

			TA3D::lp_CONFIG->developerMode = cfgFile.pullAsBool("TA3D.Developer Mode");

			TA3D::lp_CONFIG->unitTextureQuality = cfgFile.pullAsInt("TA3D.Texture Quality", 3);

			String cfg_version = cfgFile.pullAsString("TA3D.Version");
			String ref_version;
			ref_version << TA3D_VERSION_HI << "." << TA3D_VERSION_LO;
			if (cfg_version != ref_version) // Update ?
			{
				lp_CONFIG->net_server = TA3D_DEFAULT_SERVER_HOSTNAME;
				if (cfg_version.empty()) // Pre-SDL versions
				{
					lp_CONFIG->first_start = true; // First start of a >= 0.6 release
					int langID = lp_CONFIG->Lang.to_int(); // TA3D used to store language ID instead of language
					switch (langID)
					{
						case 0:
							lp_CONFIG->Lang = "english";
							break;
						case 1:
							lp_CONFIG->Lang = "french";
							break;
						case 2:
							lp_CONFIG->Lang = "german";
							break;
						case 3:
							lp_CONFIG->Lang = "spanish";
							break;
						case 4:
							lp_CONFIG->Lang = "italian";
							break;
						case 5:
							lp_CONFIG->Lang = "japanese";
							break;
						default:
							lp_CONFIG->Lang = "english";
					};
				}
			}

			// Apply settings for the current language
			if (!lp_CONFIG->Lang.empty())
				I18N::Instance()->currentLanguage(lp_CONFIG->Lang);

			LOG_INFO(LOG_PREFIX_SETTINGS << "Loaded from `" << TA3D::Paths::ConfigFile << "`");
			return true;
		}

	} // namespace Settings
} // namespace TA3D
