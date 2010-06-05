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
  |                                    menu.cpp                                  |
  |       Ce module contient les routines du menu de TA3D                        |
  |                                                                              |
  \-----------------------------------------------------------------------------*/

#include "stdafx.h"
#include <vector>
#include <list>
#include "misc/matrix.h"
#include "TA3D_NameSpace.h"
#include "ta3dbase.h"
#include "scripts/cob.h"                    // Pour la lecture et l'éxecution des scripts
#include "EngineClass.h"            // Inclus le moteur
#include "fbi.h"                    // Pour la gestion des unités
#include "tnt.h"                    // Inclus le chargeur de cartes
#include "menu.h"
#include "restore.h"
#include "misc/settings.h"
#include "misc/paths.h"
#include "misc/files.h"
#include "misc/resources.h"
#include "logs/logs.h"
#include "ingame/gamedata.h"
#include "ingame/menus/mapselector.h"
#include "ingame/menus/unitselector.h"
#include "languages/i18n.h"
#include "misc/math.h"
#include "sounds/manager.h"
#include "ingame/players.h"
#include "scripts/script.h"
#include "ingame/battle.h"
#include "ai/ai.h"
#include "input/mouse.h"
#include "input/keyboard.h"
#include "cache.h"
#include "mods/mods.h"
#include "network/netclient.h"
#include <yuni/core/math.h>
#include <yuni/core/sleep.h>



#define p_size          10.0f

// Some functions from main.cpp used to deal with config file



namespace TA3D
{


	void ReadFileParameter();


	static inline String FixBlank(const String& s)
	{
		String t(s);
		t.replace(' ', char(1));
		return t;
	}

	static inline String UnfixBlank(const String& s)
	{
		String t(s);
		t.replace(char(1), ' ');
		return t;
	}

	void setup_game(bool client, const String& host, String saved_game, bool bNetServer, bool instantStart)
	{
		int my_player_id = -1;
		String status;
		if (!host.empty())
		{
			if (!client)
			{
				if (!bNetServer)
					network_manager.InitBroadcast(1234);      // broadcast mode
				network_manager.HostGame(host, 4242);
			}
			else
				network_manager.Connect(host, 4242);

			my_player_id = network_manager.getMyID();           // Get player id

			if (client)
			{
				status = network_manager.getStatus();
				LOG_DEBUG("client received game status : " << status);
				if (!status.empty())
				{
					status = Paths::Savegames + "multiplayer" + Paths::Separator + status;
					saved_game = status;
				}
				else
				{
					network_manager.sendSpecial("NOTIFY NEW_PLAYER " + FixBlank(lp_CONFIG->player_name));
					SleepMilliSeconds(10);
					network_manager.sendSpecial( "REQUEST GameData" );
				}
			}
		}

		cursor_type = CURSOR_DEFAULT;

		uint16  player_str_n = 4;
		String  player_str[4] = { lp_CONFIG->player_name, I18N::Translate("computer"), I18N::Translate("open"), I18N::Translate("closed") };
		byte    player_control[4] = { PLAYER_CONTROL_LOCAL_HUMAN, PLAYER_CONTROL_LOCAL_AI, PLAYER_CONTROL_NONE, PLAYER_CONTROL_CLOSED };
		String::Vector  side_str;
		String::Vector  AI_list = AI_PLAYER::getAvailableAIs();
		AI_list.resize(AI_list.size() + 1);
		for (int i = int(AI_list.size()) - 1 ; i > 0 ; --i)
			AI_list[i] = AI_list[i-1];

		side_str.resize( ta3dSideData.nb_side);
		for (int i = 0; i < ta3dSideData.nb_side; ++i)
			side_str[i] = ta3dSideData.side_name[i];

		GameData game_data;
		game_data.unserialize(lp_CONFIG->serializedGameData);

		if (!VFS::Instance()->fileExists(game_data.map_filename))
		{
			String::Vector map_list;
			const uint32 n = VFS::Instance()->getFilelist("maps\\*.tnt", map_list);

			if (n == 0)
			{
				network_manager.Disconnect();
				TA3D::Gui::AREA::current()->popup(I18N::Translate("Error"),I18N::Translate("No maps found"));
				LOG_ERROR("No map has been found !");
				reset_mouse();
				return;
			}
			game_data.map_filename = *(map_list.begin());
			map_list.clear();
		}
		if (!VFS::Instance()->fileExists(game_data.game_script) || String::ToLower(Paths::ExtractFileExt(game_data.game_script)) != ".lua")
		{
			if (VFS::Instance()->fileExists("scripts\\game\\default.lua"))
				game_data.game_script = "scripts\\game\\default.lua";
			else
			{
				String::Vector script_list;
				uint32 n = VFS::Instance()->getFilelist("scripts\\game\\*.lua", script_list);

				if (n == 0)
				{
					network_manager.Disconnect();
					TA3D::Gui::AREA::current()->popup(I18N::Translate("Error"),I18N::Translate("No scripts found"));
					LOG_ERROR("No script has been found!!");
					reset_mouse();
					return;
				}
				for (String::Vector::iterator i = script_list.begin() ; i != script_list.end() ; ++i)
				{
					game_data.game_script = *i;
					if (i->size() > 1 && (*i)[0] != '_')            // Avoid selecting a special file as default script if possible
						break;
				}
				script_list.clear();
			}
		}

		if (lp_CONFIG->serializedGameData.empty() || client || !host.empty())
		{
			for (short int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
			{
				game_data.player_names[i] = player_str[2];
				game_data.player_sides[i] = side_str[0];
				game_data.player_control[i] = player_control[2];
				game_data.ai_level[i] = AI_list.empty() ? String("none") : AI_list[0];
			}

			if (lp_CONFIG->serializedGameData.empty())
			{
				if (!client)
				{
					game_data.player_names[0] = player_str[0];
					game_data.player_sides[0] = side_str[0];
					game_data.player_control[0] = player_control[0];
					game_data.player_network_id[0] = my_player_id;
					game_data.ai_level[0] = AI_list.empty() ? String("none") : AI_list[0];

					if (!host)
					{
						game_data.player_names[1] = player_str[1];
						game_data.player_sides[1] = side_str[1];
						game_data.player_control[1] = player_control[1];
						game_data.ai_level[1] = AI_list.empty() ? String("none") : AI_list[0];
					}
				}
			}
		}

		int net_id_table[10];           // Table used to identify players joining a multiplayer saved game

		if (saved_game.notEmpty())             // We're loading a multiplayer game !!
		{
			load_game_data( saved_game, &game_data, true);      // Override server only access to game information, we're loading now
			int my_old_id = -1;
			for (int i = 0 ; i < 10 ; i++)          // Build the reference table
			{
				net_id_table[i] = game_data.player_network_id[i];
				if (game_data.player_control[i] == PLAYER_CONTROL_LOCAL_HUMAN)
					my_old_id = net_id_table[i];
			}
			network_manager.sendSpecial( String("NOTIFY PLAYER_BACK ") << my_old_id );
			SleepMilliSeconds(10);
			network_manager.sendSpecial( "REQUEST GameData" );
		}

		int dx, dy;
		GLuint glimg = load_tnt_minimap_fast(game_data.map_filename,dx,dy);
		MAP_OTA map_data;
		map_data.load(Paths::Files::ReplaceExtension(game_data.map_filename, ".ota"));
		float ldx = float(dx) * 70.0f / 252.0f;
		float ldy = float(dy) * 70.0f / 252.0f;

		Gui::AREA setupgame_area("setup");
		setupgame_area.load_tdf("gui/setupgame.area");
		if (!setupgame_area.background)
			setupgame_area.background = gfx->glfond;
		for (int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
		{
			setupgame_area.caption( String("gamesetup.name") << i, game_data.player_names[i]);

			Gui::GUIOBJ::Ptr guiobj = setupgame_area.get_object(String("gamesetup.side") << i);
			if (guiobj)
			{
				guiobj->Text.clear();
				guiobj->Text.push_back( game_data.player_sides[i] );
				guiobj->Text.insert(guiobj->Text.end(), side_str.begin(), side_str.end());
			}
			AI_list[0] = (game_data.player_control[i] & PLAYER_CONTROL_FLAG_AI) ? game_data.ai_level[i] : String();
			setupgame_area.set_entry( String("gamesetup.ai") << i, AI_list);
			guiobj = setupgame_area.get_object( String("gamesetup.color") << i);
			if (guiobj)
			{
				guiobj->Flag |= (game_data.player_control[i] == PLAYER_CONTROL_NONE ? FLAG_HIDDEN : 0);
				guiobj->Data = gfx->makeintcol(player_color[player_color_map[i]*3], player_color[player_color_map[i]*3+1], player_color[player_color_map[i]*3+2]);
			}
			guiobj = setupgame_area.get_object( String("gamesetup.team") << i );
			if (guiobj)
				guiobj->current_state = Math::Log2(game_data.team[i]);
			setupgame_area.caption( String("gamesetup.energy") << i, String() << game_data.energy[i]);
			setupgame_area.caption( String("gamesetup.metal") << i, String() << game_data.metal[i]);
		}

		if (setupgame_area.get_object("gamesetup.max_units"))
		{
			Gui::GUIOBJ::Ptr obj = setupgame_area.get_object("gamesetup.max_units");
			obj->Text[0] = String() << game_data.max_unit_per_player;
		}

		Gui::GUIOBJ::Ptr minimap_obj = setupgame_area.get_object( "gamesetup.minimap");
		float mini_map_x1 = 0.0f;
		float mini_map_y1 = 0.0f;
		float mini_map_x2 = 0.0f;
		float mini_map_y2 = 0.0f;
		float mini_map_x = 0.0f;
		float mini_map_y = 0.0f;
		if (minimap_obj)
		{
			mini_map_x1 = minimap_obj->x1;
			mini_map_y1 = minimap_obj->y1;
			mini_map_x2 = minimap_obj->x2;
			mini_map_y2 = minimap_obj->y2;
			ldx = float(dx) * ( mini_map_x2 - mini_map_x1 ) / 504.0f;
			ldy = float(dy) * ( mini_map_y2 - mini_map_y1 ) / 504.0f;

			mini_map_x = (mini_map_x1 + mini_map_x2) * 0.5f;
			mini_map_y = (mini_map_y1 + mini_map_y2) * 0.5f;

			minimap_obj->Data = glimg;
			minimap_obj->x1 = mini_map_x - ldx;
			minimap_obj->y1 = mini_map_y - ldy;
			minimap_obj->x2 = mini_map_x + ldx;
			minimap_obj->y2 = mini_map_y + ldy;
			minimap_obj->u2 = float(dx) / 252.0f;
			minimap_obj->v2 = float(dy) / 252.0f;
		}

		Gui::GUIOBJ::Ptr guiobj = setupgame_area.get_object( "scripts.script_list");
		if (guiobj)
		{
			String::Vector script_list;
			VFS::Instance()->getFilelist("scripts\\game\\*.lua", script_list);
			guiobj->Text.clear();
			for (String::Vector::const_iterator i_script = script_list.begin(); i_script != script_list.end(); ++i_script)
				guiobj->Text.push_back(*i_script);
		}
		setupgame_area.caption( "gamesetup.script_name", game_data.game_script);
		{
			Gui::GUIOBJ::Ptr obj = setupgame_area.get_object( "gamesetup.FOW");
			if (obj )
				obj->Text[0] = obj->Text[ 1 + game_data.fog_of_war ];
		}

		{
			String map_info;
			if (!map_data.missionname.empty())
				map_info << map_data.missionname << "\n";
			if (!map_data.numplayers.empty())
				map_info << map_data.numplayers << "\n";
			if (!map_data.missiondescription.empty())
				map_info << map_data.missiondescription;
			setupgame_area.caption("gamesetup.map_info", map_info);
		}

		if (!host)
			for (short int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
				setupgame_area.msg(String("gamesetup.ready") << i << ".hide");
		uint32 player_timer[TA3D_PLAYERS_HARD_LIMIT];
		for (short int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
			player_timer[i] = msec_timer;

		bool done = instantStart;

		if (host.notEmpty() && my_player_id == -1) // Leave now, we aren't connected but we're in network mode
		{
			LOG_ERROR("in network mode without being connected");
			done = true;
		}

		if (saved_game.notEmpty() && game_data.saved_file.empty())     // We're trying to load a multiplayer game we didn't save
		{
			LOG_ERROR("trying to load a multiplayer game we didn't play");
			done = true;
		}

		bool start_game = instantStart;

		int amx = -1;
		int amy = -1;
		int amz = -1;
		int amb = -1;

		String set_map;
		String previous_tnt_port;
		String previous_ota_port;
		String previous_lua_port;

		if (host.notEmpty() && client)
		{
			setupgame_area.msg("gamesetup.b_ok.disable");
			setupgame_area.msg("gamesetup.b_units.disable");
			setupgame_area.msg("gamesetup.max_units.disable");
		}
		else if (saved_game.notEmpty())
		{
			setupgame_area.msg("gamesetup.b_units.disable");
			setupgame_area.msg("gamesetup.change_map.disable");
			setupgame_area.msg("gamesetup.FOW.disable");
			for (short int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
			{
				Gui::GUIOBJ::Ptr obj = setupgame_area.get_object(String("gamesetup.team") << i);
				if (obj)
					obj->Flag &= ~FLAG_CAN_BE_CLICKED;
			}
		}

		int progress_timer = msec_timer;
		int ping_timer = msec_timer;                    // Used to send simple PING requests in order to detect when a connection fails

		bool statusUpdateRequired = true;

		do
		{
			if (host.notEmpty())
			{
				for (int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
				{
					Gui::GUIOBJ::Ptr obj = setupgame_area.get_object(String("gamesetup.ready") << i);
					if (obj)
					{
						if (game_data.player_control[i] != PLAYER_CONTROL_LOCAL_HUMAN
							&& game_data.player_control[i] != PLAYER_CONTROL_REMOTE_HUMAN)
							obj->Flag |= FLAG_HIDDEN;
						else
							obj->Flag &= ~FLAG_HIDDEN;
						obj->Etat = game_data.ready[i];
					}
				}
			}

			if (statusUpdateRequired && !client)
			{
				statusUpdateRequired = false;
				if (bNetServer)
				{
					uint16 nb_open = 0;
					for (int f = 0; f < TA3D_PLAYERS_HARD_LIMIT; ++f)
					{
						if (setupgame_area.caption(String("gamesetup.name") << f) == player_str[2])
							++nb_open;
					}
					NetClient::instance()->sendMessage("SERVER MAP \"" + Escape(Paths::ExtractFileNameWithoutExtension(game_data.map_filename)) + "\" SLOTS " + String(nb_open));
				}
			}

			if (saved_game.notEmpty())
			{
				for (int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
				{
					if (game_data.player_control[i] == PLAYER_CONTROL_LOCAL_HUMAN)
					{
						if (!game_data.ready[i])
						{
							setupgame_area.set_state(String("gamesetup.ready") << i,true);
							game_data.ready[i] = true;
							network_manager.sendSpecial("NOTIFY UPDATE");
						}
					}
				}
			}

			bool key_is_pressed = false;
			String broadcast_msg;
			String chat_msg;
			String special_msg;
			struct chat received_chat_msg;
			struct chat received_special_msg;
			bool playerDropped = false;
			do
			{
				if (bNetServer)
					NetClient::instance()->receive();
				playerDropped = network_manager.getPlayerDropped();
				broadcast_msg = network_manager.getNextBroadcastedMessage();
				if (network_manager.getNextChat(&received_chat_msg ) == 0)
					chat_msg = (char*)received_chat_msg.message;
				if (network_manager.getNextSpecial(&received_special_msg ) == 0)
					special_msg = (char*)received_special_msg.message;
				if (host.notEmpty() && !network_manager.isConnected()) // We're disconnected !!
				{
					LOG_DEBUG("disconnected from server");
					done = true;
					break;
				}
				setupgame_area.check();
				key_is_pressed = setupgame_area.key_pressed;
				SleepMilliSeconds(TA3D_MENUS_RECOMMENDED_TIME_MS_FOR_RESTING);
				if (msec_timer - progress_timer >= 500 && Yuni::Math::Equals(network_manager.getFileTransferProgress(), 100.0f))
					break;
			} while (amx == mouse_x && amy == mouse_y && amz == mouse_z && amb == mouse_b && mouse_b == 0 && !key[ KEY_ENTER ] && !key[ KEY_ESC ] && !done
				&& !key_is_pressed && !setupgame_area.scrolling && broadcast_msg.empty() && chat_msg.empty() && special_msg.empty() && !playerDropped
				&& ( msec_timer - ping_timer < 2000 || host.empty() || client ));

			//-------------------------------------------------------------- Network Code : syncing information --------------------------------------------------------------

			if (host.notEmpty() && !client && msec_timer - ping_timer >= 2000) // Send a ping request
			{
				statusUpdateRequired = true;
				network_manager.sendPing();
				ping_timer = msec_timer;

				for (int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i) // ping time out
				{
					if (game_data.player_network_id[i] > 0 && msec_timer - player_timer[i] > 10000)
					{
						LOG_DEBUG("dropping player " << game_data.player_network_id[i] << "[" << i << "] from " << __FILE__ << " l." << __LINE__);
						network_manager.dropPlayer(game_data.player_network_id[i]);
						playerDropped = true;
						player_timer[i] = msec_timer;
					}
				}
			}

            if (network_manager.getFileTransferProgress() < 100.0f)
			{
				progress_timer = msec_timer;
				Gui::GUIOBJ::Ptr obj = setupgame_area.get_object( "gamesetup.p_transfer");
				if (obj)
				{
					obj->Flag &= ~FLAG_HIDDEN;
					int progress = (int)network_manager.getFileTransferProgress();
					obj->Data = progress;
				}
			}
			else
			{
				Gui::GUIOBJ::Ptr obj = setupgame_area.get_object( "gamesetup.p_transfer");
				if (obj)
					obj->Flag |= FLAG_HIDDEN;
			}

			if (playerDropped)
			{
				for (int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
				{
					if (game_data.player_network_id[i] > 0 && !network_manager.pollPlayer( game_data.player_network_id[i]))
					{
						if (saved_game.notEmpty())
						{
							setupgame_area.set_state(String("gamesetup.ready") << i, false);     // He's not there
							game_data.ready[i] = false;
						}
						else
						{
							game_data.player_names[i] = player_str[2];
							game_data.player_sides[i] = side_str[0];
							game_data.player_control[i] = player_control[2];
							game_data.ai_level[i] = AI_TYPE_EASY;
							game_data.player_network_id[i] = -1;

							setupgame_area.caption( String("gamesetup.name") << i,game_data.player_names[i]);                                 // Update gui
							AI_list[0] = (game_data.player_control[i] & PLAYER_CONTROL_FLAG_AI) ? game_data.ai_level[i] : String();
							setupgame_area.set_entry( String("gamesetup.ai") << i, AI_list);
							setupgame_area.caption( String("gamesetup.side") << i, side_str[0]);                           // Update gui

							Gui::GUIOBJ::Ptr guiobj =  setupgame_area.get_object( String("gamesetup.color") << i );
							if (guiobj)
								guiobj->Flag |= FLAG_HIDDEN;
						}
					}
				}
				network_manager.sendSpecial("NOTIFY UPDATE");
			}

			while (!special_msg.empty()) // Special receiver (sync config data)
			{
				int from = received_special_msg.from;
				String::Vector params;
				LOG_DEBUG(LOG_PREFIX_NET << "parsing '" << (char*)(received_special_msg.message) << "'");
				String((char*)(received_special_msg.message)).explode(params, ' ', true, false, true);
				if (params.size() == 1)
				{
					if (params[0] == "PONG")
					{
						int player_id = game_data.net2id(from);
						if (player_id >= 0 )
							player_timer[ player_id ] = msec_timer;
					}
				}
				else
					if (params.size() == 2)
					{
						if (params[0] == "REQUEST")
						{
							if (params[1] == "PLAYER_ID")                  // Sending player's network ID
								network_manager.sendSpecial( String("RESPONSE PLAYER_ID ") << from, -1, from);
							else
							{
								if (params[1] == "GameData") // Sending game information
								{
									network_manager.sendSpecial(String("SET UNIT LIMIT ") << game_data.max_unit_per_player);
									for (short int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i) // Send player information
									{
										if (client && game_data.player_network_id[i] != my_player_id )  continue;       // We don't send updates about things we won't update
										String msg;                             // SYNTAX: PLAYER_INFO player_id network_id side_id ai_level metal energy player_name ready_flag
										int side_id = String::FindInList(side_str, game_data.player_sides[i]);
										msg << "PLAYER_INFO " << i << " " << game_data.player_network_id[i] << " "
											<< side_id << " "
											<< ((game_data.player_control[i] == PLAYER_CONTROL_NONE || game_data.player_control[i] == PLAYER_CONTROL_CLOSED || game_data.ai_level[i].empty()) ? String("[C]") : FixBlank(game_data.ai_level[i]))
											<< " " << game_data.metal[i] << " " << game_data.energy[i] << " "
											<< FixBlank(game_data.player_names[i]) << " " << (int)game_data.ready[i];
										network_manager.sendSpecial( msg, -1, from);

										Gui::GUIOBJ::Ptr guiobj =  setupgame_area.get_object( String("gamesetup.team") << i);
										if (guiobj)
										{
											msg.clear();
											msg << "CHANGE TEAM " << i << " " << (int)(guiobj->current_state);
											network_manager.sendSpecial( msg, -1, from);
										}
									}
									if (!client)  // Send server to client specific information (player colors, map name, ...)
									{
										String msg("PLAYERCOLORMAP");
										for (short int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
											msg += String(" ") << int(player_color_map[i]);
										network_manager.sendSpecial( msg, -1, from);

										network_manager.sendSpecial(String("SET FOW ") << int(game_data.fog_of_war), -1, from);
										network_manager.sendSpecial("SET SCRIPT " + FixBlank( game_data.game_script), -1, from);
										network_manager.sendSpecial("SET MAP " + FixBlank( game_data.map_filename), -1, from);
									}
								}
								else if (params[1] == "STATUS")
								{
									if (saved_game.notEmpty())
										network_manager.sendSpecial("STATUS SAVED " + FixBlank( Paths::ExtractFileName(saved_game) ), -1, from);
									else
										network_manager.sendSpecial("STATUS NEW", -1, from);
								}
							}
						}
						else
							if (params[0] == "NOTIFY")
							{
								if (params[1] == "UPDATE")
									network_manager.sendSpecial( "REQUEST GameData");           // We're told there are things to update, so ask for update
								else
								{
									if (params[1] == "PLAYER_LEFT")
									{
										LOG_DEBUG("dropping player " << from << " from " << __FILE__ << " l." << __LINE__);
										network_manager.dropPlayer(from);
										network_manager.sendSpecial( "REQUEST GameData");           // We're told there are things to update, so ask for update
										for (short int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
										{
											if (game_data.player_network_id[i] == from)
											{
												if (saved_game.notEmpty())
												{
													setupgame_area.set_state(String("gamesetup.ready") << i,false);
													game_data.ready[i] = false;
												}
												else
												{
													game_data.player_network_id[i] = -1;
													game_data.player_control[i] = player_control[2];
													game_data.player_names[i] = player_str[2];

													setupgame_area.caption(String("gamesetup.name") << i, game_data.player_names[i]);

													Gui::GUIOBJ::Ptr guiobj =  setupgame_area.get_object( String("gamesetup.color") << i );
													if (guiobj)
														guiobj->Flag |= FLAG_HIDDEN;
												}
												break;
											}
										}
									}
									else
									{
										if (params[1] == "START") // Game is starting ...
										{
											clear_keybuf();
											done = true;      // If user click "OK" or hit enter then leave the window
											start_game = true;
										}
									}
								}
							}
					}
					else
						if (params.size() == 3)
						{
							if (params[0] == "NOTIFY")
							{
								if (params[1] == "NEW_PLAYER" && !saved_game) // Add new player
								{
									int slot = -1;
									for (int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
									{
										if (game_data.player_control[i] == PLAYER_CONTROL_NONE)
										{
											slot = i;
											break;
										}
									}
									if (slot >= 0)
									{
										player_timer[ slot ] = msec_timer;              // If we forget this, player will be droped immediately
										game_data.player_network_id[slot] = from;
										game_data.player_control[slot] = PLAYER_CONTROL_REMOTE_HUMAN;
										game_data.player_names[slot] = UnfixBlank( params[2] );
										setupgame_area.caption( String("gamesetup.name") << slot, game_data.player_names[slot]);                      // Update gui

										Gui::GUIOBJ::Ptr guiobj =  setupgame_area.get_object( String("gamesetup.color") << slot);
										if (guiobj)
										{
											guiobj->Data = gfx->makeintcol( player_color[player_color_map[slot] * 3],
																			player_color[player_color_map[slot] * 3 + 1],
																			player_color[player_color_map[slot] * 3 + 2]);           // Update gui
											guiobj->Flag &= ~FLAG_HIDDEN;
										}
										network_manager.sendSpecial( "NOTIFY UPDATE", from);            // Tell others that things have changed
									}
									else
									{
										LOG_DEBUG("dropping player " << from << " from " << __FILE__ << " l." << __LINE__);
										network_manager.dropPlayer(from);      // No more room for this player !!
									}
								}
								else if (params[1] == "PLAYER_BACK" && saved_game.notEmpty()) // A player is back in the game :), let's find who it is
								{
									LOG_DEBUG("received identifier from " << from << " : " << params[2].to<sint32>());
									int slot = -1;
									for (int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
									{
										if (net_id_table[i] == params[2].to<sint32>())
										{
											slot = i;
											break;
										}
									}
									if (slot >= 0)
									{
										player_timer[ slot ] = msec_timer;              // If we forget this, player will be droped immediately
										game_data.player_network_id[slot] = from;
										game_data.player_control[slot] = PLAYER_CONTROL_REMOTE_HUMAN;

										network_manager.sendSpecial( "NOTIFY UPDATE", from);            // Tell others that things have changed
									}
									else
									{
										LOG_DEBUG("dropping player " << from << " from " << __FILE__ << " l." << __LINE__ << " because it couldn't be identified");
										network_manager.dropPlayer(from);      // No more room for this player !!
									}
								}
								else if (params[1] == "COLORCHANGE")
								{
									int i = params[2].to<sint32>();
									if (!client) // From client to server only
									{
										byte e = player_color_map[i];
										int f = -1;
										for (int g = 0; g < TA3D_PLAYERS_HARD_LIMIT; ++g) // Look for the next color
										{
											if ((game_data.player_control[g] == PLAYER_CONTROL_NONE || game_data.player_control[g] == PLAYER_CONTROL_CLOSED) && player_color_map[g] > e && (f == -1 || player_color_map[g] < player_color_map[f]) )
												f = g;
										}
										if (f == -1)
										{
											for (int g = 0; g < TA3D_PLAYERS_HARD_LIMIT; ++g)
											{
												if ((game_data.player_control[g] == PLAYER_CONTROL_NONE || game_data.player_control[g] == PLAYER_CONTROL_CLOSED) && (f == -1 || player_color_map[g] < player_color_map[f]) )
													f = g;
											}
										}
										if (f != -1)
										{
											byte g = player_color_map[f];
											player_color_map[i] = g;                                // update game data
											player_color_map[f] = e;

											guiobj =  setupgame_area.get_object( String("gamesetup.color") << i);
											if (guiobj)
												guiobj->Data = gfx->makeintcol(player_color[player_color_map[i]*3],player_color[player_color_map[i]*3+1],player_color[player_color_map[i]*3+2]);            // Update gui
											guiobj =  setupgame_area.get_object( String("gamesetup.color") << f);
											if (guiobj)
												guiobj->Data = gfx->makeintcol(player_color[player_color_map[f]*3],player_color[player_color_map[f]*3+1],player_color[player_color_map[f]*3+2]);            // Update gui
										}
										network_manager.sendSpecial("NOTIFY UPDATE");
									}
								}
							}
							else if (params[0] == "SET")
							{
								if (params[1] == "FOW")
								{
									int value = params[2].to<sint32>();
									Gui::GUIOBJ::Ptr obj = setupgame_area.get_object( "gamesetup.FOW");
									if (obj && value >= 0 && value < 4)
									{
										obj->Value = value;
										obj->Text[0] = obj->Text[1 + obj->Value];
										game_data.fog_of_war = uint8(obj->Value);
									}
								}
								else if (params[1] == "MAP")
								{
									set_map = UnfixBlank( params[2] );
									if (set_map != game_data.map_filename )
									{
										if (!previous_tnt_port.empty() )
											network_manager.stopFileTransfer( previous_tnt_port);
										if (!previous_ota_port.empty())
											network_manager.stopFileTransfer(previous_ota_port);
										previous_ota_port.empty();
										previous_tnt_port.empty();
										String new_map_name = TA3D::Paths::Files::ReplaceExtension(set_map,".tnt");
										if (client && !VFS::Instance()->fileExists( new_map_name ))
										{
											String sMpN(new_map_name);
											sMpN.replace('\\', '/');
											previous_tnt_port = network_manager.getFile( 1, sMpN);
											network_manager.sendSpecial( "REQUEST FILE " + FixBlank(new_map_name) + " " + previous_tnt_port );
										}

										new_map_name = TA3D::Paths::Files::ReplaceExtension(new_map_name,".ota");

										if (client && !VFS::Instance()->fileExists( new_map_name ))
										{
											String sMpN(new_map_name);
											sMpN.replace('\\', '/');

											previous_ota_port = network_manager.getFile( 1, sMpN);
											network_manager.sendSpecial( "REQUEST FILE " + FixBlank(new_map_name) + " " + previous_ota_port );
										}
									}
								}
								else if (params[1] == "SCRIPT")
								{
									String script_name = UnfixBlank( params[2] );
									if (script_name != game_data.game_script)
									{
										setupgame_area.caption( "gamesetup.script_name", script_name);
										game_data.game_script = script_name;

										if (client && !VFS::Instance()->fileExists( script_name ))
										{
											if (!previous_lua_port.empty())
												network_manager.stopFileTransfer( previous_lua_port);

											String sSpS(script_name);
											sSpS.replace('\\', '/');

											previous_lua_port = network_manager.getFile( 1, sSpS);
											network_manager.sendSpecial("REQUEST FILE " + FixBlank(script_name) + " " + previous_lua_port);
										}
									}
								}
							}
						}
						else if (params.size() == 4)
						{
							if (params[0] == "REQUEST") // REQUEST FILE filename port
							{
								if (params[1] == "FILE")
								{
									String file_name = UnfixBlank( params[2] );
									LOG_DEBUG(LOG_PREFIX_NET << "received file request : '" << file_name << "'");
									network_manager.stopFileTransfer( params[3], from);
									network_manager.sendFile( from, file_name, params[3]);
								}
							}
							else if (params[0] == "CHANGE")
							{
								if (params[1] == "TEAM")
								{
									int i = params[2].to<sint32>();
									int n_team = params[3].to<sint32>();
									if (i >= 0 && i < TA3D_PLAYERS_HARD_LIMIT && (client || from == game_data.player_network_id[i])) // Server doesn't accept someone telling him what to do
									{
										Gui::GUIOBJ::Ptr guiobj = setupgame_area.get_object( String("gamesetup.team") << i );
										if (guiobj)
										{
											guiobj->current_state = byte(n_team);
											game_data.team[i] = short(1 << n_team);
										}
									}
								}
							}
							else if (params[0] == "SET")
							{
								if (params[1] == "UNIT" && params[2] == "LIMIT")
								{
									game_data.max_unit_per_player = params[3].to<sint32>();
									Gui::GUIOBJ::Ptr obj = setupgame_area.get_object("gamesetup.max_units");
									if (obj)
										obj->Text[0] = String() << game_data.max_unit_per_player;
								}
							}
						}
						else if (params.size() == 9)
						{
							if (params[0] == "PLAYER_INFO") // We've received player information, let's update :)
							{
								int i = params[1].to<sint32>();
								int n_id = params[2].to<sint32>();
								if (i >= 0 && i < TA3D_PLAYERS_HARD_LIMIT && (client || from == n_id)) // Server doesn't accept someone telling him what to do
								{
									int side_id  = params[3].to<sint32>();
									int metal_q  = params[5].to<sint32>();
									int energy_q = params[6].to<sint32>();
									bool ready   = params[8].to<sint32>();
									game_data.player_network_id[i] = n_id;
									game_data.player_sides[i] = side_str[ side_id ];
									game_data.ai_level[i] = UnfixBlank( params[4] );
									game_data.metal[i] = metal_q;
									game_data.energy[i] = energy_q;
									game_data.player_names[i] = UnfixBlank( params[7] );
									game_data.ready[i] = ready;
									if (n_id < 0 && game_data.ai_level[i].size() >= 4)
										game_data.player_control[i] = PLAYER_CONTROL_REMOTE_AI;     // AIs are on the server, no need to replicate them
									else if (n_id < 0 && game_data.ai_level[i].size() < 4)
										game_data.player_control[i] = PLAYER_CONTROL_NONE;
									else
										game_data.player_control[i] = (n_id == my_player_id) ? PLAYER_CONTROL_LOCAL_HUMAN : PLAYER_CONTROL_REMOTE_HUMAN;

									setupgame_area.caption( String("gamesetup.name") << i, game_data.player_names[i]);                                 // Update gui
									AI_list[0] = (game_data.player_control[i] & PLAYER_CONTROL_FLAG_AI) ? game_data.ai_level[i] : String();
									setupgame_area.set_entry( String("gamesetup.ai") << i, AI_list);
									setupgame_area.caption( String("gamesetup.side") << i, side_str[side_id]);                         // Update gui
									setupgame_area.caption( String("gamesetup.energy") << i, String() << game_data.energy[i]);         // Update gui
									setupgame_area.caption( String("gamesetup.metal") << i, String() << game_data.metal[i]);               // Update gui
									setupgame_area.set_state( String("gamesetup.ready") << i, ready);                                           // Update gui

									Gui::GUIOBJ::Ptr guiobj =  setupgame_area.get_object( String("gamesetup.color") << i);
									if (guiobj)
									{
										guiobj->Data = gfx->makeintcol(player_color[player_color_map[i]*3],player_color[player_color_map[i]*3+1],player_color[player_color_map[i]*3+2]);            // Update gui
										if (game_data.player_control[i] == PLAYER_CONTROL_NONE || game_data.player_control[i] == PLAYER_CONTROL_CLOSED )
											guiobj->Flag |= FLAG_HIDDEN;
										else
											guiobj->Flag &= ~FLAG_HIDDEN;
									}
									if (!client)
										network_manager.sendSpecial("NOTIFY UPDATE", from);
								}
								else
									LOG_ERROR("Packet error : " << received_special_msg.message);
							}
						}
						else if (params.size() == 11)
						{
							if (params[0] == "PLAYERCOLORMAP")
							{
								for (short int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
								{
									player_color_map[i] = byte(params[i + 1].to<sint32>());
									Gui::GUIOBJ::Ptr guiobj =  setupgame_area.get_object( String("gamesetup.color") << i);
									if (guiobj)
										guiobj->Data = gfx->makeintcol(player_color[player_color_map[i]*3],player_color[player_color_map[i]*3+1],player_color[player_color_map[i]*3+2]);            // Update gui
								}
							}
						}

				if (network_manager.getNextSpecial(&received_special_msg) == 0)
					special_msg = (char*)received_special_msg.message;
				else
					special_msg.clear();
			}

			//-------------------------------------------------------------- Network Code : chat system --------------------------------------------------------------

			while (!chat_msg.empty()) // Chat receiver
			{
				Gui::GUIOBJ::Ptr chat_list = setupgame_area.get_object("gamesetup.chat_list");
				if (chat_list)
				{
					const int lastSize = chat_list->Text.size();
					setupgame_area.append("gamesetup.chat_list", chat_msg);
					if (chat_list->Text.size() > 5)
						chat_list->Data += chat_list->Text.size() - lastSize;
					chat_list->Pos = uint32(chat_list->Text.size() - 1);
				}

				if (network_manager.getNextChat( &received_chat_msg ) == 0 )
					chat_msg = (char*)received_chat_msg.message;
				else
					chat_msg.clear();
			}

			//-------------------------------------------------------------- Network Code : advert system --------------------------------------------------------------

			while (!broadcast_msg.empty())  // Broadcast message receiver
			{
				String::Vector params;
				broadcast_msg.explode(params, ' ', true, false, true);
				if (params.size() == 3 && params[0] == "PING" && params[1] == "SERVER")
				{
					if (params[2] == "LIST" && host.notEmpty()) // Sending information about this server
					{
						uint16 nb_open = 0;
						for (int f = 0; f < TA3D_PLAYERS_HARD_LIMIT; ++f)
						{
							if (setupgame_area.caption(String("gamesetup.name") << f) == player_str[2])
								++nb_open;
						}

						String hostFixed(host);
						hostFixed.replace(' ', char(1));

						String engineV(TA3D_ENGINE_VERSION);
						engineV.replace(' ', char(1));
						if (TA3D_CURRENT_MOD.empty())
						{
							network_manager.broadcastMessage(String("PONG SERVER ") << hostFixed << " . " << engineV << " " << nb_open);
						}
						else
						{
							String mod(TA3D_CURRENT_MOD);
							mod.replace(' ', char(1));
							network_manager.broadcastMessage(String("PONG SERVER ") << hostFixed << " " << mod << " " << engineV << " " << nb_open);
						}
					}
				}
				broadcast_msg = network_manager.getNextBroadcastedMessage();
			}

			//-------------------------------------------------------------- End of Network Code --------------------------------------------------------------

			amx = mouse_x;
			amy = mouse_y;
			amz = mouse_z;
			amb = mouse_b;

			if (key[KEY_ENTER] && !setupgame_area.caption("gamesetup.t_chat").empty())
			{
				String message;
				message << "<" << lp_CONFIG->player_name << "> " << setupgame_area.caption("gamesetup.t_chat");
				if (host.notEmpty())
				{
					struct chat msg;
					network_manager.sendChat( strtochat( &msg, message ));
				}
				Gui::GUIOBJ::Ptr chat_list = setupgame_area.get_object("gamesetup.chat_list");

				if (chat_list)
				{
					const int lastSize = chat_list->Text.size();
					setupgame_area.append("gamesetup.chat_list", message);
					if (chat_list->Text.size() > 5)
						chat_list->Data += chat_list->Text.size() - lastSize;
					chat_list->Pos = uint32(chat_list->Text.size() - 1);
				}

				setupgame_area.caption("gamesetup.t_chat", "");
			}

			if (setupgame_area.get_value("gamesetup.FOW") >= 0 && !client && !saved_game)
			{
				Gui::GUIOBJ::Ptr obj = setupgame_area.get_object( "gamesetup.FOW");
				if (obj && obj->Value != -1)
				{
					obj->Text[0] = obj->Text[1 + obj->Value];
					game_data.fog_of_war = uint8(obj->Value);
					if (host.notEmpty())
						network_manager.sendSpecial(String("SET FOW ") << obj->Value);
				}
			}

			if (client || saved_game.notEmpty())
				setupgame_area.msg("scripts.hide"); // Hide the scripts window in client mode

			if (setupgame_area.get_state( "scripts.b_ok" ) && !client && !saved_game)
			{
				guiobj = setupgame_area.get_object( "scripts.script_list");
				if (guiobj && guiobj->Pos < guiobj->num_entries())
				{
					setupgame_area.caption( "gamesetup.script_name", guiobj->Text[ guiobj->Pos ]);
					game_data.game_script = guiobj->Text[ guiobj->Pos ];
					if (host.notEmpty())
						network_manager.sendSpecial("SET SCRIPT " + FixBlank(guiobj->Text[guiobj->Pos]));
				}
			}

			if (setupgame_area.get_state( "gamesetup.b_ok" ) && !client)
			{
				bool ready = true;
				if (host.notEmpty())
				{
					for (int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
					{
						if (game_data.player_control[i] == PLAYER_CONTROL_LOCAL_HUMAN || game_data.player_control[i] == PLAYER_CONTROL_REMOTE_HUMAN )
							ready &= game_data.ready[i];
					}
				}

				if (ready)
				{
					while (key[KEY_ENTER])
					{
						SleepMilliSeconds(TA3D_MENUS_RECOMMENDED_TIME_MS_FOR_RESTING);
						poll_inputs();
					}
					clear_keybuf();
					done = true;      // If user click "OK" or hit enter then leave the window
					start_game = true;
					network_manager.sendSpecial("NOTIFY START");
				}
			}
			if (setupgame_area.get_state("gamesetup.b_cancel"))
			{
				LOG_DEBUG("leaving game room");
				done = true;      // En cas de click sur "retour", on quitte la fenêtre
			}

			if (!saved_game && setupgame_area.get_value("gamesetup.max_units") >= 0 && !client)
			{
				Gui::GUIOBJ::Ptr obj = setupgame_area.get_object("gamesetup.max_units");
				obj->Text[0] = obj->Text[1+obj->Value];
				game_data.max_unit_per_player = obj->Text[0].to<sint32>();
				network_manager.sendSpecial(String("SET UNIT LIMIT ") << game_data.max_unit_per_player);
			}

			for (int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
			{
				if (setupgame_area.get_state( String("gamesetup.ready") << i) != game_data.ready[i])
				{
					if (game_data.player_control[i] == PLAYER_CONTROL_LOCAL_HUMAN && !saved_game)
					{
						network_manager.sendSpecial( "NOTIFY UPDATE");
						game_data.ready[i] = !game_data.ready[i];
					}
					else
						setupgame_area.set_state( String("gamesetup.ready") << i, game_data.ready[i]);
				}
				if (saved_game.notEmpty()) continue;            // We mustn't change any thing for a saved game
				guiobj = setupgame_area.get_object( String("gamesetup.team") << i );
				if (guiobj != NULL && (1 << guiobj->current_state) != game_data.team[i])           // Change team
				{
					if ( ((!client && !(game_data.player_control[i] & PLAYER_CONTROL_FLAG_REMOTE)) || (client && game_data.player_control[i] == PLAYER_CONTROL_LOCAL_HUMAN))
						&& game_data.player_control[i] != PLAYER_CONTROL_NONE && game_data.player_control[i] != PLAYER_CONTROL_CLOSED)
					{
						network_manager.sendSpecial( "NOTIFY UPDATE");
						game_data.team[i] = short(1 << guiobj->current_state);
					}
					else
						guiobj->current_state = byte(Math::Log2(game_data.team[i]));
				}
				if (client && game_data.player_network_id[i] != my_player_id )
					continue;                           // You cannot change other player's settings
				if (setupgame_area.get_state(String("gamesetup.b_name") << i) && !client ) // Change player type
				{
					if (game_data.player_network_id[i] >= 0 && game_data.player_network_id[i] != my_player_id ) // Kick player !!
					{
						network_manager.dropPlayer( game_data.player_network_id[i]);
						network_manager.sendSpecial( "NOTIFY UPDATE");
					}
					int e = 0;
					for (int f = 0; f < player_str_n; ++f)
					{
						if (setupgame_area.caption(String("gamesetup.name") << i) == player_str[f])
						{
							e = f;
							break;
						}
					}
					e = (e + 1) % player_str_n;

					if (player_control[e] == PLAYER_CONTROL_LOCAL_HUMAN)// We can only have one local human player ( or it crashes )
					{
						for (int f = 0; f < TA3D_PLAYERS_HARD_LIMIT; ++f)
						{
							if (f!= i && game_data.player_control[f] == PLAYER_CONTROL_LOCAL_HUMAN) // If we already have a local human player pass this player type value
							{
								e = (e + 1) % player_str_n;
								break;
							}
						}
					}

					game_data.player_names[i] = player_str[e];                              // Update game data
					game_data.player_control[i] = player_control[e];
					if (player_control[e] == PLAYER_CONTROL_LOCAL_HUMAN )
						game_data.player_network_id[i] = my_player_id;
					else
						game_data.player_network_id[i] = -1;

					setupgame_area.caption( String("gamesetup.name") << i, player_str[e]);         // Update gui
					AI_list[0] = (game_data.player_control[i] & PLAYER_CONTROL_FLAG_AI) ? game_data.ai_level[i] : String();
					setupgame_area.set_entry( String("gamesetup.ai") << i, AI_list);
					guiobj = setupgame_area.get_object( String("gamesetup.color") << i);
					if (guiobj)
					{
						if (player_control[e] == PLAYER_CONTROL_NONE || player_control[e] == PLAYER_CONTROL_CLOSED)
							guiobj->Flag |= FLAG_HIDDEN;
						else
							guiobj->Flag &= ~FLAG_HIDDEN;
					}
					if (host.notEmpty())
						network_manager.sendSpecial( "NOTIFY UPDATE");
				}
				if (setupgame_area.get_value( String("gamesetup.side") << i) >= 0) // Change player side
				{
					int pos = setupgame_area.get_value( String("gamesetup.side") << i) + 1;
					Gui::GUIOBJ::Ptr guiobj = setupgame_area.get_object(String("gamesetup.side") << i);
					guiobj->Text[0] = guiobj->Text[pos];

					game_data.player_sides[i] = side_str[pos - 1];                                // update game data
					if (host.notEmpty())
						network_manager.sendSpecial( "NOTIFY UPDATE");
				}
				if (!(game_data.player_control[i] & PLAYER_CONTROL_FLAG_AI))
					setupgame_area.set_state(String("gamesetup.ai") << i, false);
				else if (setupgame_area.get_value( String("gamesetup.ai") << i ) >= 0 ) // Change player level (for AI)
				{
					int pos = setupgame_area.get_value( String("gamesetup.ai") << i ) + 1;
					if (pos >= 1 && pos < (int)AI_list.size())
					{
						String AIlevel = AI_list[pos];
						AI_list[0] = (game_data.player_control[i] & PLAYER_CONTROL_FLAG_AI) ? AIlevel : String("");
						setupgame_area.set_entry( String("gamesetup.ai") << i, AI_list);          // Update gui

						game_data.ai_level[i] = AIlevel;                              // update game data
						if (host.notEmpty())
							network_manager.sendSpecial("NOTIFY UPDATE");
					}
				}
				if (setupgame_area.get_state( String("gamesetup.b_color") << i)) // Change player color
				{
					if (client)
						network_manager.sendSpecial(String("NOTIFY COLORCHANGE ") << i);
					byte e = player_color_map[i];
					int f = -1;
					for (int g = 0; g < TA3D_PLAYERS_HARD_LIMIT; ++g) // Look for the next color
					{
						if ((game_data.player_control[g] == PLAYER_CONTROL_NONE || game_data.player_control[g] == PLAYER_CONTROL_CLOSED)
							&& player_color_map[g] > e && (f == -1 || player_color_map[g] < player_color_map[f]))
						{
							f = g;
						}
					}
					if (f == -1)
					{
						for (int g = 0; g < TA3D_PLAYERS_HARD_LIMIT; ++g)
						{
							if ((game_data.player_control[g] == PLAYER_CONTROL_NONE || game_data.player_control[g] == PLAYER_CONTROL_CLOSED) && (f == -1 || player_color_map[g] < player_color_map[f]))
								f = g;
						}
					}
					if (f != -1)
					{
						byte g = player_color_map[f];
						player_color_map[i] = g;                                // update game data
						player_color_map[f] = e;

						guiobj =  setupgame_area.get_object( String("gamesetup.color") << i);
						if (guiobj )
							guiobj->Data = gfx->makeintcol(player_color[player_color_map[i]*3],player_color[player_color_map[i]*3+1],player_color[player_color_map[i]*3+2]);            // Update gui
						guiobj =  setupgame_area.get_object( String("gamesetup.color") << f);
						if (guiobj )
							guiobj->Data = gfx->makeintcol(player_color[player_color_map[f]*3],player_color[player_color_map[f]*3+1],player_color[player_color_map[f]*3+2]);            // Update gui
					}
					if (host.notEmpty() && !client)
						network_manager.sendSpecial( "NOTIFY UPDATE");
				}
				if (setupgame_area.get_state( String("gamesetup.b_energy") << i ) ) // Change player energy stock
				{
					game_data.energy[i] = (game_data.energy[i] + 500) % 10500;
					if (game_data.energy[i] == 0 ) game_data.energy[i] = 500;

					setupgame_area.caption( String("gamesetup.energy") << i, String() << game_data.energy[i]);         // Update gui
					if (host.notEmpty())
						network_manager.sendSpecial( "NOTIFY UPDATE");
				}
				if (setupgame_area.get_state( String("gamesetup.b_metal") << i ) ) // Change player metal stock
				{
					game_data.metal[i] = (game_data.metal[i] + 500) % 10500;
					if (game_data.metal[i] == 0 ) game_data.metal[i] = 500;

					setupgame_area.caption( String("gamesetup.metal") << i, String() << game_data.metal[i]);           // Update gui
					if (host.notEmpty())
						network_manager.sendSpecial( "NOTIFY UPDATE");
				}
			}

			if (setupgame_area.get_state("gamesetup.b_units") && !client && !saved_game) // Select available units
			{
				Menus::UnitSelector::Execute(game_data.use_only, game_data.use_only);       // Change unit selection
			}

			if (minimap_obj != NULL &&
				( ( ( setupgame_area.get_state( "gamesetup.minimap" ) || setupgame_area.get_state("gamesetup.change_map")) && !client)
				  || ( client && !set_map.empty() ) ) && !saved_game) // Clic on the mini-map or received map set command
			{
				String map_filename;
				String new_map;
				if (!client)
				{
					setupgame_area.caption("popup.msg", I18N::Translate("Loading maps, please wait ..."));       // Show a small popup displaying a wait message
					setupgame_area.title("popup", I18N::Translate("Please wait ..."));
					setupgame_area.msg("popup.show");
					gfx->set_2D_mode();
					setupgame_area.draw();

					glEnable(GL_TEXTURE_2D);
					gfx->set_color(0xFFFFFFFF);
					draw_cursor();

					// Affiche / Show the buffer
					gfx->flip();
					gfx->unset_2D_mode();

					String newMapName;
					Menus::MapSelector::Execute(game_data.map_filename, newMapName);
					new_map = newMapName;
					for (short int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
						player_timer[i] = msec_timer;
					setupgame_area.msg("popup.hide");
				}
				else
					new_map = set_map;

				gfx->SCREEN_W_TO_640 = 1.0f;                // To have mouse sensibility undependent from the resolution
				gfx->SCREEN_H_TO_480 = 1.0f;
				cursor_type=CURSOR_DEFAULT;
				gfx->set_2D_mode();

				if (!new_map.empty() && (set_map.empty() || (client && VFS::Instance()->fileExists( Paths::Files::ReplaceExtension(new_map, ".tnt"))
					&& VFS::Instance()->fileExists( Paths::Files::ReplaceExtension(new_map, ".ota")))))
				{
					set_map.clear();
					if (host.notEmpty() && !client)
					{
						String tmp(new_map);
						tmp.replace(' ', char(1));
						network_manager.sendSpecial("SET MAP " + tmp);
					}

					String new_map_name = new_map;

					gfx->destroy_texture( glimg);

					game_data.map_filename = new_map;
					glimg = load_tnt_minimap_fast(game_data.map_filename,dx,dy);
					ldx = float(dx) * ( mini_map_x2 - mini_map_x1 ) / 504.0f;
					ldy = float(dy) * ( mini_map_y2 - mini_map_y1 ) / 504.0f;
					minimap_obj->x1 = mini_map_x - ldx;
					minimap_obj->y1 = mini_map_y - ldy;
					minimap_obj->x2 = mini_map_x + ldx;
					minimap_obj->y2 = mini_map_y + ldy;
					minimap_obj->u2 = float(dx) / 252.0f;
					minimap_obj->v2 = float(dy) / 252.0f;

					map_data.destroy();
					map_data.load(Paths::Files::ReplaceExtension(game_data.map_filename, ".ota"));
					String map_info;
					if (!map_data.missionname.empty())
						map_info << map_data.missionname << "\n";
					if (!map_data.numplayers.empty())
						map_info << map_data.numplayers << "\n";
					if (!map_data.missiondescription.empty())
						map_info << map_data.missiondescription;
					setupgame_area.caption("gamesetup.map_info", map_info);
				}

				minimap_obj->Data = glimg;      // Synchronize the picture on GUI
			}

			if (key[KEY_ESC])
			{
				LOG_DEBUG("leaving game room");
				done = true;
			}

			setupgame_area.draw();

			glEnable(GL_TEXTURE_2D);
			gfx->set_color(0xFFFFFFFF);
			draw_cursor();

			// Affiche
			gfx->flip();

		} while (!done);

		if (!previous_lua_port.empty() && network_manager.isConnected())
			TA3D::network_manager.stopFileTransfer(previous_lua_port);
		if (!previous_ota_port.empty() && network_manager.isConnected())
			TA3D::network_manager.stopFileTransfer(previous_ota_port);
		if (!previous_tnt_port.empty() && network_manager.isConnected())
			TA3D::network_manager.stopFileTransfer(previous_tnt_port);

		if (setupgame_area.background == gfx->glfond )  setupgame_area.background = 0;
		setupgame_area.destroy();

		map_data.destroy();

		gfx->destroy_texture(glimg);

		reset_mouse();
		while (key[KEY_ESC])
		{
			SleepMilliSeconds(TA3D_MENUS_RECOMMENDED_TIME_MS_FOR_RESTING);
			poll_inputs();
		}

		if (start_game)
		{
			if (!game_data.map_filename.empty() && !game_data.game_script.empty())
			{
				if (!saved_game)            // For a saved game, we already have everything set
				{
					game_data.nb_players = 0;
					for (int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i) // Move players to the top of the vector, so it's easier to access data
					{
						if (game_data.player_control[i] != PLAYER_CONTROL_NONE && game_data.player_control[i] != PLAYER_CONTROL_CLOSED)
						{
							if (i != game_data.nb_players)
							{
								game_data.team[game_data.nb_players] = game_data.team[i];
								game_data.player_control[game_data.nb_players] = game_data.player_control[i];
								game_data.player_names[game_data.nb_players] = game_data.player_names[i];
								game_data.player_sides[game_data.nb_players] = game_data.player_sides[i];
								game_data.ai_level[game_data.nb_players] = game_data.ai_level[i];
								game_data.energy[game_data.nb_players] = game_data.energy[i];
								game_data.metal[game_data.nb_players] = game_data.metal[i];
								byte e = player_color_map[game_data.nb_players];
								player_color_map[game_data.nb_players] = player_color_map[i];
								player_color_map[i] = e;
							}
							game_data.nb_players++;
						}
					}

					lp_CONFIG->serializedGameData = game_data.serialize();		// Remember the last game parameters
					Settings::Save();											// Save it to disk to avoid surprises in case of crash
				}

				Battle::Execute(&game_data);
			}

			while (key[KEY_ESC])
			{
				SleepMilliSeconds(TA3D_MENUS_RECOMMENDED_TIME_MS_FOR_RESTING);
				poll_inputs();
			}
		}
		else
		{
			if (client)
				network_manager.sendSpecial( "NOTIFY PLAYER_LEFT");
		}
		network_manager.Disconnect();
	}

} // namespace TA3D
