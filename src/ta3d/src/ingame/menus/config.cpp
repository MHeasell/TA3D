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

#include "config.h"
#include <input/keyboard.h>
#include <input/mouse.h>
#include <TA3D_NameSpace.h>
#include <mods/mods.h>
#include <misc/paths.h>
#include <cache.h>
#include <sounds/manager.h>
#include <ingame/sidedata.h>
#include <misc/settings.h>
#include <misc/suspend.h>
#include <SDL_video.h>

namespace TA3D
{
	namespace Menus
	{

		bool Config::Execute()
		{
			Config m;
			return m.execute();
		}

		Config::Config()
			: Abstract()
		{
			saved_config = new TA3DCONFIG;
		}

		Config::~Config()
		{
			delete saved_config;
		}

		void Config::doFinalize()
		{
			// Wait for user to release ESC
			reset_mouse();
			while (isKeyDown(KEY_ESC))
			{
				SuspendMilliSeconds(TA3D_MENUS_RECOMMENDED_TIME_MS_FOR_RESTING);
				poll_inputs();
			}
			clearKeyboardBuffer();

			bool ask_for_quickrestart = lp_CONFIG->quickrestart;

			if (!save)
				*lp_CONFIG = *saved_config;
			else
			{
				sound_manager->savePlaylist(); // Save the playlist

				// Check we're not trying to crash the game :P
				gfx->checkConfig();

				if (lp_CONFIG->screen_width != saved_config->screen_width || lp_CONFIG->screen_height != saved_config->screen_height || lp_CONFIG->fsaa != saved_config->fsaa || (lp_CONFIG->fullscreen != saved_config->fullscreen)) // Need to restart
				{
					lp_CONFIG->quickrestart = true;
				}

				lp_CONFIG->player_name = pArea->caption("*.player_name");

				if (lp_CONFIG->last_MOD != TA3D_CURRENT_MOD) // Refresh the file structure
				{
					TA3D_CURRENT_MOD = lp_CONFIG->last_MOD;
					Cache::Clear(true); // Force cache reset

					VFS::Instance()->reload();
					ta3dSideData.loadData(); // Refresh side data so we load the correct values
					sound_manager->reset();
					sound_manager->loadTDFSounds(true);
					sound_manager->loadTDFSounds(false);
				}

				if ((lp_CONFIG->developerMode ^ saved_config->developerMode) || lp_CONFIG->unitTextureQuality != saved_config->unitTextureQuality)
					Cache::Clear(true); // Force cache reset
			}

			grab_mouse(lp_CONFIG->grab_inputs);
			sound_manager->setVolume(lp_CONFIG->sound_volume);
			sound_manager->setMusicVolume(lp_CONFIG->music_volume);

			lp_CONFIG->quickrestart |= ask_for_quickrestart;

			pArea->destroy();

			I18N::CurrentLanguage(lp_CONFIG->Lang);

			TA3D::Settings::Save(); // Keep settings :)
		}

		bool Config::doInitialize()
		{
			LOG_DEBUG(LOG_PREFIX_MENU_MULTIMENU << "Entering...");

			// Loading the area
			loadAreaFromTDF("config", "gui/config.area");

			if (lp_CONFIG->restorestart)
			{
				lp_CONFIG->restorestart = false;
				lp_CONFIG->quickstart = false;
			}

			*saved_config = *lp_CONFIG;

			if (pArea->get_object("*.fps_limit"))
			{
				fps_limits = pArea->get_object("*.fps_limit")->Text;
				fps_limits.erase(fps_limits.begin());
			}
			else
			{
				fps_limits.push_back("50");
				fps_limits.push_back("60");
				fps_limits.push_back("70");
				fps_limits.push_back("80");
				fps_limits.push_back("90");
				fps_limits.push_back("100");
				fps_limits.push_back("no limit");
			}
			I18N::Translate(fps_limits);

			// assumes we are on display 0
			int number_of_display_modes = SDL_GetNumDisplayModes(0);
			if (number_of_display_modes < 1)
			{
				LOG_ERROR(String("Failed to get the number of display modes: ") << SDL_GetError());
			}

			ScreenResolution lastRes(0, 0);
			for (int i = 0; i < number_of_display_modes; ++i)
			{
				SDL_DisplayMode mode;
				if (SDL_GetDisplayMode(0, i, &mode) != 0) {
					LOG_ERROR(String("Couldn't get display mode") << i << ": " << SDL_GetError());
					continue;
				}
				ScreenResolution res(mode.w, mode.h);

				// filter out repeats of the same resolution
				if (res == lastRes)
				{
					continue;
				}

				// filter out too-small resolutions
				if (res.width < 640 || res.height < 480)
				{
					continue;
				}

				availableScreenResolutions.push_back(res);
				lastRes = res;
			}
			if (availableScreenResolutions.empty())
			{
				LOG_ERROR("No suitable resolution choices found!");
			}

			pArea->set_state("*.showfps", lp_CONFIG->showfps);
			pArea->caption("*.fps_limit", fps_limits[fps_limits.size() - 1]);
			for (String::Vector::const_iterator i = fps_limits.begin(); i != fps_limits.end(); ++i)
			{
				if ((String() << (int)lp_CONFIG->fps_limit) == *i)
					pArea->caption("*.fps_limit", *i);
			}
			pArea->set_value("*.texture_quality", lp_CONFIG->unitTextureQuality);
			pArea->set_value("*.interface_transparency", 0);
			pArea->set_value("*.shadow_map_size", 0);
			pArea->set_state("*.grab_inputs", lp_CONFIG->grab_inputs);
			pArea->set_value("*.sound_volume", lp_CONFIG->sound_volume);
			pArea->set_value("*.music_volume", lp_CONFIG->music_volume);
			pArea->set_state("*.far_sight", false);
			pArea->set_value("*.anisotropy", lp_CONFIG->anisotropy);
			pArea->set_value("*.mouse_sensitivity", 100);
			pArea->set_state("*.disable_perspective", true);
			pArea->set_state("*.right_click_interface", lp_CONFIG->right_click_interface);
			pArea->set_state("*.disable_GLSL", true);
			pArea->set_state("*.underwater_bright", false);
			pArea->set_state("*.use_texture_compression", false);
			pArea->set_state("*.low_definition_map", false);
			pArea->set_state("*.sky", false);
			pArea->set_state("*.particle", true);
			pArea->set_state("*.explosion_particles", false);
			pArea->set_state("*.waves", false);
			pArea->set_state("*.height_line", lp_CONFIG->height_line);
			pArea->set_state("*.detail_tex", false);
			pArea->set_state("*.use_texture_cache", false);
			pArea->set_state("*.draw_console_loading", lp_CONFIG->draw_console_loading);
			pArea->set_state("*.fullscreen", lp_CONFIG->fullscreen);
			pArea->set_state("*.developer_mode", lp_CONFIG->developerMode);
			pArea->set_state("*.tool_tips", false);
			I18N::Instance()->retrieveAllLanguages(languageList);
			if (pArea->get_object("*.LANG"))
			{
				Gui::GUIOBJ::Ptr objLang = pArea->get_object("*.LANG");
				objLang->Text.clear();
				I18N::Language* l = I18N::Instance()->language(lp_CONFIG->Lang);
				if (l)
					objLang->Text.push_back(l->caption());
				else
					objLang->Text.push_back(lp_CONFIG->Lang);
				for (unsigned int i = 0; i < languageList.size(); i++)
					objLang->Text.push_back(languageList[i].caption());
			}
			if (pArea->get_object("*.camera_zoom"))
				pArea->caption("*.camera_zoom", pArea->get_object("*.camera_zoom")->Text[0]);
			pArea->caption("*.camera_def_angle", String("TA"));
			pArea->caption("*.camera_def_h", String("TA"));
			pArea->caption("*.camera_zoom_speed", String("None"));
			if (pArea->get_object("*.screenres"))
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.screenres");
				obj->Text.clear();

				// put the current resolution first
				obj->Text.push_back(String().format("%dx%d", lp_CONFIG->screen_width, lp_CONFIG->screen_height));
				for (auto it = availableScreenResolutions.begin(); it != availableScreenResolutions.end(); ++it)
				{
					obj->Text.push_back(String().format("%dx%d", it->width, it->height));
				}
			}
			Gui::GUIOBJ::Ptr tmpO = pArea->get_object("*.shadow_quality");
			if (tmpO)
			{
				const unsigned int indx = 0;
				if (indx < tmpO->Text.size())
					pArea->caption("*.shadow_quality", tmpO->Text[indx]);
			}

			pArea->caption("*.timefactor", String() << (int)lp_CONFIG->timefactor);
			switch (lp_CONFIG->fsaa)
			{
				case 2:
					pArea->caption("*.fsaa", "x2");
					break;
				case 4:
					pArea->caption("*.fsaa", "x4");
					break;
				case 6:
					pArea->caption("*.fsaa", "x6");
					break;
				case 8:
					pArea->caption("*.fsaa", "x8");
					break;
				default:
					pArea->caption("*.fsaa", I18N::Translate("no fsaa"));
			}

			if (pArea->get_object("*.mod"))
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.mod");

				if (obj->Text.size() >= 2)
					obj->Text[0] = obj->Text[1];
				else
					obj->Text.resize(1);

				String current_selection = TA3D_CURRENT_MOD.length() > 6 ? Substr(TA3D_CURRENT_MOD, 5, TA3D_CURRENT_MOD.length() - 6) : "";
				String::List mod_list = Mods::instance()->getModNameList(Mods::MOD_INSTALLED);
				mod_list.sort();
				mod_list.unique();
				for (String::List::iterator i = mod_list.begin(); i != mod_list.end(); ++i)
				{
					obj->Text.push_back(*i);
					if (ToLower(*i) == ToLower(current_selection))
						obj->Text[0] = *i;
				}
			}

			pArea->caption("*.player_name", lp_CONFIG->player_name);

			if (pArea->get_object("*.skin"))
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.skin");

				obj->Text.resize(1);
				obj->Text[0] = I18N::Translate("default.skn");

				String::Vector skin_list;
				VFS::Instance()->getFilelist("gui\\*.skn", skin_list);

				String skin_name;
				const String::Vector::iterator end = skin_list.end();
				for (String::Vector::iterator i = skin_list.begin(); i != end; ++i)
				{
					skin_name = Paths::ExtractFileName(*i, false);
					obj->Text.push_back(skin_name);
					if (String("gui/") << ToLower(skin_name) == ToLower(lp_CONFIG->skin_name))
						obj->Text[0] = skin_name;
				}
			}

			if (pArea->get_object("*.l_files"))
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.l_files");
				sound_manager->getPlayListFiles(obj->Text);
			}

			if (lp_CONFIG->quickstart)
				I_Msg(TA3D::TA3D_IM_GUI_MSG, "config_confirm.show");

			save = false;
			timer = MILLISECONDS_SINCE_INIT;

			return true;
		}

		void Config::waitForEvent()
		{
			time_out = false;
			do
			{
				// Grab user events
				pArea->check();

				// Wait to reduce CPU consumption
				wait();

				if (lp_CONFIG->quickstart)
				{
					Gui::GUIOBJ::Ptr pbar = pArea->get_object("config_confirm.p_wait");
					if (pbar)
					{
						const uint32 new_value = (MILLISECONDS_SINCE_INIT - timer) / 50;
						if (new_value != pbar->Data)
						{
							pbar->Data = new_value;
							if (new_value >= 100)
								time_out = true;
							break;
						}
					}
				}
			} while (pMouseX == mouse_x && pMouseY == mouse_y && pMouseZ == mouse_z && pMouseB == mouse_b && mouse_b == 0 && !isKeyDown(KEY_ENTER) && !isKeyDown(KEY_ESC) && !isKeyDown(KEY_SPACE) && !isKeyDown(KEY_C) && !pArea->key_pressed && !pArea->scrolling);
		}

		bool Config::maySwitchToAnotherMenu()
		{
			if (lp_CONFIG->quickstart)
			{
				if (time_out || pArea->get_state("config_confirm.b_cancel_changes") || isKeyDown(KEY_ESC))
				{
					I_Msg(TA3D::TA3D_IM_GUI_MSG, "config_confirm.hide");
					TA3D::Settings::Restore(TA3D::Paths::ConfigFile);
					TA3D::Settings::Load();
					save = false;
					lp_CONFIG->quickstart = false;
					lp_CONFIG->quickrestart = true;
					lp_CONFIG->restorestart = true;
					*saved_config = *lp_CONFIG;
					return true;
				}
				else if (pArea->get_state("config_confirm.b_confirm"))
				{
					I_Msg(TA3D::TA3D_IM_GUI_MSG, "config_confirm.hide");
					lp_CONFIG->quickstart = false;
					saved_config->quickstart = false;
					TA3D::Settings::Save(); // Keep settings :)
				}
			}

			if (pArea->get_state("*.b_activate"))
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.l_files");
				if (obj && obj->Text.size() > obj->Pos)
				{
					sound_manager->setPlayListFileMode(obj->Pos, false, false);
					obj->Text[obj->Pos][1] = '*';
				}
			}
			if (pArea->get_state("*.b_deactivate"))
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.l_files");
				if (obj && obj->Text.size() > obj->Pos)
				{
					sound_manager->setPlayListFileMode(obj->Pos, false, true);
					obj->Text[obj->Pos][1] = ' ';
				}
			}
			if (pArea->get_state("*.b_battle"))
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.l_files");
				if (obj && obj->Text.size() > obj->Pos)
				{
					sound_manager->setPlayListFileMode(obj->Pos, true, false);
					obj->Text[obj->Pos][1] = 'B';
				}
			}

			if (pArea->get_state("*.b_ok"))
			{
				save = true;
				return true; // On "OK", leave the menu
			}
			if (pArea->get_state("*.b_cancel")) // On "cancel", leave
				return true;

			lp_CONFIG->showfps = pArea->get_state("*.showfps");
			if (pArea->get_value("*.fps_limit") >= 0)
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.fps_limit");
				if (obj && obj->Data != uint32(-1))
				{
					obj->Text[0] = fps_limits[obj->Value];
					switch (obj->Value)
					{
						case 0:
							lp_CONFIG->fps_limit = 50;
							break;
						case 1:
							lp_CONFIG->fps_limit = 60;
							break;
						case 2:
							lp_CONFIG->fps_limit = 70;
							break;
						case 3:
							lp_CONFIG->fps_limit = 80;
							break;
						case 4:
							lp_CONFIG->fps_limit = 90;
							break;
						case 5:
							lp_CONFIG->fps_limit = 100;
							break;
						default:
							lp_CONFIG->fps_limit = -1;
					}
				}
			}

			if (lp_CONFIG->grab_inputs != pArea->get_state("*.grab_inputs"))
			{
				lp_CONFIG->grab_inputs = pArea->get_state("*.grab_inputs");
				grab_mouse(lp_CONFIG->grab_inputs);
			}
			if (lp_CONFIG->sound_volume != pArea->get_value("*.sound_volume"))
			{
				lp_CONFIG->sound_volume = pArea->get_value("*.sound_volume");
				sound_manager->setVolume(lp_CONFIG->sound_volume);
			}
			if (lp_CONFIG->music_volume != pArea->get_value("*.music_volume"))
			{
				lp_CONFIG->music_volume = pArea->get_value("*.music_volume");
				sound_manager->setMusicVolume(lp_CONFIG->music_volume);
			}
			lp_CONFIG->unitTextureQuality = pArea->get_value("*.texture_quality");
			lp_CONFIG->anisotropy = sint16(pArea->get_value("*.anisotropy"));
			lp_CONFIG->right_click_interface = pArea->get_state("*.right_click_interface");
			lp_CONFIG->height_line = pArea->get_state("*.height_line");
			lp_CONFIG->draw_console_loading = pArea->get_state("*.draw_console_loading");
			lp_CONFIG->fullscreen = pArea->get_state("*.fullscreen");
			lp_CONFIG->developerMode = pArea->get_state("*.developer_mode");
			if (pArea->get_value("*.LANG") >= 0)
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.LANG");
				if (obj && obj->Value != -1)
				{
					obj->Text[0] = obj->Text[1 + obj->Value];
					lp_CONFIG->Lang = languageList[obj->Value].englishCaption();
				}
			}
			if (pArea->get_value("*.screenres") >= 0)
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.screenres");
				if (obj && obj->Value != -1)
				{
					obj->Text[0] = obj->Text[1 + obj->Value];
					lp_CONFIG->screen_width = availableScreenResolutions[obj->Value].width;
					lp_CONFIG->screen_height = availableScreenResolutions[obj->Value].height;
				}
			}
			if (pArea->get_value("*.timefactor") >= 0)
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.timefactor");
				if (obj && obj->Value != -1)
				{
					obj->Text[0] = obj->Text[1 + obj->Value];
					lp_CONFIG->timefactor = float(obj->Value + 1);
				}
			}
			if (pArea->get_value("*.fsaa") >= 0)
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.fsaa");
				if (obj && obj->Value != -1)
				{
					obj->Text[0] = obj->Text[1 + obj->Value];
					lp_CONFIG->fsaa = sint16(obj->Value << 1);
				}
			}
			if (pArea->get_value("*.mod") >= 0)
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.mod");
				if (obj && obj->Value != -1)
				{
					obj->Text[0] = obj->Text[1 + obj->Value];
					lp_CONFIG->last_MOD = obj->Value > 0 ? String("mods/") << obj->Text[0] << '/' : String();
				}
			}
			if (pArea->get_value("*.skin") >= 0)
			{
				Gui::GUIOBJ::Ptr obj = pArea->get_object("*.skin");
				if (obj && obj->Value != -1)
				{
					obj->Text[0] = obj->Text[1 + obj->Value];
					lp_CONFIG->skin_name = obj->Value >= 0 ? String("gui/") << obj->Text[0] : String();
				}
			}

			if (isKeyDown(KEY_ESC)) // Leave menu on ESC
				return true;

			return false;
		}
	}
}
