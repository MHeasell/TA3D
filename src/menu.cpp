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

#ifdef CWDEBUG
#include <libcwd/sys.h>
#include <libcwd/debug.h>
#endif
#include "stdafx.h"
#include "matrix.h"
#include "TA3D_NameSpace.h"
#include "ta3dbase.h"
#include "3do.h"					// Pour la lecture des fichiers 3D
#include "cob.h"					// Pour la lecture et l'éxecution des scripts
#include "EngineClass.h"			// Inclus le moteur
#include "fbi.h"					// Pour la gestion des unités
#include "tnt.h"					// Inclus le chargeur de cartes
#include "menu.h"
#include "gui.h"
#include "taconfig.h"
#include "restore.h"
#include "misc/settings.h"
#include "misc/paths.h"
#include "logs/logs.h"



void generate_script_from_mission( String Filename, cTAFileParser *ota_parser, int schema = 0 );	// To access the script generator in the 'script' module

using namespace TA3D::EXCEPTION;

#define p_size			10.0f
#define MENU_NB_PART	200

// Some functions from main.cpp used to deal with config file


void ReadFileParameter();


void main_menu(void)
{
    GuardEnter( main_menu );

    while( mouse_b || key[ KEY_ENTER ] || key[ KEY_ESC ] || key[ KEY_SPACE ] )	rest( 10 );

    gfx->SetDefState();

    gfx->set_2D_mode();

    gfx->ReInitTexSys();

    gfx->SCREEN_W_TO_640 = 1.0f;				// To have mouse sensibility undependent from the resolution
    gfx->SCREEN_H_TO_480 = 1.0f;

    AREA main_area("main");
    main_area.load_tdf("gui/main.area");
    if( !main_area.background )	main_area.background = gfx->glfond;

    cursor_type = CURSOR_DEFAULT;

    String current_mod = TA3D_CURRENT_MOD.length() > 6 ? TA3D_CURRENT_MOD.substr( 5, TA3D_CURRENT_MOD.length() - 6 ) : "";

    ReadFileParameter();			// If there is a file parameter, read it

    bool done=false;
    bool first=true;

    int amx = -1;
    int amy = -1;
    int amz = -1;
    int amb = -1;

    do
    {
        bool key_is_pressed = false;
        do
        {
            key_is_pressed = keypressed();
            main_area.check();
            rest( 1 );
        } while( amx == mouse_x && amy == mouse_y && amz == mouse_z && amb == mouse_b
                 && mouse_b == 0 && !key[ KEY_ENTER ] && !key[ KEY_ESC ] && !done
                 && !key_is_pressed && !main_area.scrolling && !first );

        first = false;

        amx = mouse_x;
        amy = mouse_y;
        amz = mouse_z;
        amb = mouse_b;

        if(key[KEY_ESC])	done=true;

        main_area.set_caption("main.t_version", TA3D_ENGINE_VERSION );
        if( !current_mod.empty() )
            main_area.set_caption("main.t_mod", "MOD: " + current_mod );
        else
            main_area.set_caption("main.t_mod", "" );

        bool reset = false;

        if( key[KEY_ESC] || main_area.get_state( "main.b_exit" ) )
            done=true;
        if( key[KEY_SPACE] || main_area.get_state( "main.b_options" ) || lp_CONFIG->quickstart )
        {
            glPushMatrix();
            config_menu();
            lp_CONFIG->quickstart = false;
            reset = true;
            glPopMatrix();
        }
        if( key[KEY_ENTER] || main_area.get_state( "main.b_solo" ) )
        {
            glPushMatrix();
            solo_menu();
            glPopMatrix();
            reset = true;
        }
        if( key[KEY_B] || main_area.get_state( "main.b_multi" ) )
        {
            glPushMatrix();
            network_room();
            glPopMatrix();
            reset = true;
        }

        if( reset )
        {
            first = true;

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glEnable(GL_TEXTURE_2D);
            gfx->set_color( 0xFFFFFFFF );

            current_mod = TA3D_CURRENT_MOD.length() > 6 ? TA3D_CURRENT_MOD.substr( 5, TA3D_CURRENT_MOD.length() - 6 ) : "";

            gfx->SCREEN_W_TO_640 = 1.0f;				// To have mouse sensibility undependent from the resolution
            gfx->SCREEN_H_TO_480 = 1.0f;
            cursor_type=CURSOR_DEFAULT;		// Curseur par standard
        }

        main_area.draw();

        glEnable(GL_TEXTURE_2D);
        gfx->set_color(0xFFFFFFFF);
        draw_cursor();

        gfx->flip();

    } while(!done && !lp_CONFIG->quickrestart);

    if( main_area.background == gfx->glfond )	main_area.background = 0;
    main_area.destroy();

    gfx->set_2D_mode();

    GuardLeave();
}



void solo_menu()
{
    cursor_type=CURSOR_DEFAULT;

    reset_keyboard();
    while(key[KEY_ESC])
        rest(1);

    AREA solo_area("solo");
    solo_area.load_tdf("gui/solo.area");
    if( !solo_area.background )	solo_area.background = gfx->glfond;

    bool done=false;

    int amx = -1;
    int amy = -1;
    int amz = -1;
    int amb = -1;

    do
    {
        bool key_is_pressed = false;
        do
        {
            key_is_pressed = keypressed();
            solo_area.check();
            rest( 1 );
        } while( amx == mouse_x && amy == mouse_y && amz == mouse_z && amb == mouse_b
                 && mouse_b == 0 && !key[ KEY_ENTER ] && !key[ KEY_ESC ] && !done
                 && !key_is_pressed && !solo_area.scrolling );

        amx = mouse_x;
        amy = mouse_y;
        amz = mouse_z;
        amb = mouse_b;

        if( solo_area.get_state( "solo.b_skirmish" ) || key[KEY_ENTER] )
        {
            while( key[KEY_ENTER] )
            {
                rest( 20 );
                poll_keyboard();
            }
            clear_keybuf();

            glPushMatrix();
            setup_game();
            glPopMatrix();
        }

        if( solo_area.get_state( "solo.b_campaign" ) || key[KEY_C] )
        {
            while( key[KEY_ESC] )
            {
                rest( 20 );
                poll_keyboard();
            }
            clear_keybuf();

            glPushMatrix();
            campaign_main_menu();
            glPopMatrix();
        }

        if( solo_area.get_state( "solo.b_load" ) && solo_area.get_object("load_menu.l_file") )
        {
            GUIOBJ *obj = solo_area.get_object("load_menu.l_file");
            List<String> file_list = GetFileList(TA3D::Paths::Savegames + "*.sav" );

            file_list.sort();

            obj->Text.clear();
            obj->Text.reserve( file_list.size() );
            foreach( file_list, i )	obj->Text.push_back( *i );
        }

        if( solo_area.get_state( "load_menu.b_load" ) )
        {
            solo_area.set_state( "load_menu.b_load", false );
            GUIOBJ *obj_list = solo_area.get_object( "load_menu.l_file" );
            if( obj_list && obj_list->Pos >= 0 && obj_list->Pos < obj_list->Text.size() )
            {
                GAME_DATA game_data;
                load_game_data(TA3D::Paths::Savegames + obj_list->Text[ obj_list->Pos ], &game_data );

                if( !game_data.saved_file.empty() )
                {
                    gfx->unset_2D_mode();
                    GuardStart( play );
                    play(&game_data);
                    GuardCatch();
                    if( IsExceptionInProgress() ) // if guard threw an error this will be true.
                    {
                        GuardDisplayAndLogError();   // record and display the error.
                        exit(1);                      // we outa here.
                    }
                    gfx->set_2D_mode();
                    gfx->ReInitTexSys();
                }
            }
        }

        if( solo_area.get_state( "solo.b_back" ) || key[KEY_ESC] )
        {
            while( key[KEY_ESC] )
            {
                rest( 20 );
                poll_keyboard();
            }
            clear_keybuf();
            done=true;
        }

        solo_area.draw();

        glEnable(GL_TEXTURE_2D);
        gfx->set_color(0xFFFFFFFF);
        draw_cursor();

        // Affiche
        gfx->flip();
    } while(!done);

    if( solo_area.background == gfx->glfond )
        solo_area.background = 0;
    solo_area.destroy();

    clear_keybuf();

    reset_mouse();
    while(key[KEY_ESC])
    {
        rest(1);
        poll_keyboard();
    }
}

uint32 GetMultiPlayerMapList(std::list<std::string> *li)
{
    LOG_ASSERT(li);

    std::list< String > map_list;
    uint32 n = HPIManager->GetFilelist("maps\\*.tnt",&map_list);
    std::list< String >::iterator i_map;
    uint32 count;

    if( n < 1 )
        return 0;

    MAP_OTA	map_data;											// Using MAP_OTA because it's faster than cTAFileParser that fills a hash_table object
    bool isNetworkGame;
    count = 0;

    for( i_map=map_list.begin(); i_map!=map_list.end(); i_map++ )
    {
        *i_map=i_map->substr(5,i_map->length()-9);

        isNetworkGame = false;

        uint32 ota_size=0;
        byte *data = HPIManager->PullFromHPI( String( "maps\\" ) + *i_map + String( ".ota" ), &ota_size);
        if(data) {
            map_data.load((char*)data,ota_size);
            isNetworkGame = map_data.network;
            free(data);
            map_data.destroy();
        }

        if( isNetworkGame )
        {
            li->push_back( *i_map );
            count++;
        }
    }

    li->sort();
    return count;
} 

char *select_map(String *def_choice)		// Cette fonction affiche un menu permettant à l'utilisateur de choisir une carte dans une liste et de la prévisualiser à l'écran
{
    cursor_type=CURSOR_DEFAULT;

    reset_keyboard();
    while(key[KEY_ESC])	rest(1);

    AREA mapsetup_area("map setup");
    mapsetup_area.load_tdf("gui/mapsetup.area");
    if( !mapsetup_area.background )	mapsetup_area.background = gfx->glfond;

    List< String > map_list;
    uint32 n = GetMultiPlayerMapList( &map_list );
    List< String >::iterator i_map;

    if(n==0)	{
        Popup(TRANSLATE("Error"),TRANSLATE("No maps found"));
        Console->AddEntry("no maps found!!");
        reset_mouse();
        return NULL;
    }

    char *choice=NULL;

    GLuint mini = 0;
    int dx = 0;
    int dy = 0;
    float ldx = dx*70.0f/252.0f;
        float ldy = dy*70.0f/252.0f;


        GUIOBJ *minimap_obj = mapsetup_area.get_object( "mapsetup.minimap" );
    float mini_map_x1 = 0.0f;
    float mini_map_y1 = 0.0f;
    float mini_map_x2 = 0.0f;
    float mini_map_y2 = 0.0f;
    float mini_map_x = 0.0f;
    float mini_map_y = 0.0f;
    if( minimap_obj ) {
        mini_map_x1 = minimap_obj->x1;
        mini_map_y1 = minimap_obj->y1;
        mini_map_x2 = minimap_obj->x2;
        mini_map_y2 = minimap_obj->y2;
        ldx = dx * ( mini_map_x2 - mini_map_x1 ) / 504.0f;
        ldy = dy * ( mini_map_y2 - mini_map_y1 ) / 504.0f;

        mini_map_x = (mini_map_x1 + mini_map_x2) * 0.5f;
        mini_map_y = (mini_map_y1 + mini_map_y2) * 0.5f;

        minimap_obj->Data = 0;
        minimap_obj->x1 = mini_map_x - ldx;
        minimap_obj->y1 = mini_map_y - ldy;
        minimap_obj->x2 = mini_map_x + ldx;
        minimap_obj->y2 = mini_map_y + ldy;
        minimap_obj->u2 = dx / 252.0f;
        minimap_obj->v2 = dy / 252.0f;
    }

    MAP_OTA	map_data;
    int sel_index = -1;
    int o_sel = -1;

    if( def_choice ) {
        *def_choice = def_choice->substr(5,def_choice->length()-9);
        int i = 0;
        GUIOBJ *gui_map_list = mapsetup_area.get_object("mapsetup.map_list");
        if( gui_map_list )
            gui_map_list->Text.resize( map_list.size() );
        for( i_map = map_list.begin() ; i_map != map_list.end() ; i_map++, i++ ) {
            if( gui_map_list )
                gui_map_list->Text[ i ] = *i_map;
            if( *i_map == *def_choice && gui_map_list ) {
                gui_map_list->Pos = i;
                gui_map_list->Data = i;			// Make it visible
                sel_index = i;
            }
        }
    }

    bool done=false;

    int amx = -1;
    int amy = -1;
    int amz = -1;
    int amb = -1;

    do
    {
        bool key_is_pressed = false;
        do {
            key_is_pressed = keypressed();
            mapsetup_area.check();
            rest( 1 );
        } while( amx == mouse_x && amy == mouse_y && amz == mouse_z && amb == mouse_b && mouse_b == 0 && !key[ KEY_ENTER ] && !key[ KEY_ESC ] && !done && !key_is_pressed && !mapsetup_area.scrolling );

        amx = mouse_x;
        amy = mouse_y;
        amz = mouse_z;
        amb = mouse_b;

        if( mapsetup_area.get_state( "mapsetup.b_ok" ) || key[KEY_ENTER] ) {
            while( key[KEY_ENTER] )	{	rest( 20 );	poll_keyboard();	}
            clear_keybuf();
            done=true;		// If user click "OK" or hit enter then leave the window
        }

        if( mapsetup_area.get_state( "mapsetup.b_cancel" ) || key[KEY_ESC] ) {
            while( key[KEY_ESC] )	{	rest( 20 );	poll_keyboard();	}
            clear_keybuf();
            done=true;		// If user click "Cancel" or hit ESC then leave the screen returning NULL
            if( choice )	free( choice );
            choice = NULL;
        }

        if( mapsetup_area.get_object("mapsetup.map_list") )
            sel_index = mapsetup_area.get_object("mapsetup.map_list")->Pos;

        if( sel_index != o_sel && sel_index >= 0) {
            o_sel = sel_index;
            gfx->destroy_texture( mini );
            i_map = map_list.begin();
            for( int i = 0 ; i < sel_index && i_map != map_list.end() ; i++)	i_map++;
            String tmp = String("maps\\") + *i_map + String(".tnt");
            mini = load_tnt_minimap_fast((char*)tmp.c_str(),&dx,&dy);
            if( choice )	free( choice );		// Don't forget to free memory
            choice = strdup(tmp.c_str());													// Copy the map name
            tmp = String("maps\\") + *i_map + String(".ota");								// Read the ota file
            uint32 ota_size = 0;
            byte *data = HPIManager->PullFromHPI(tmp,&ota_size);
            if(data) {
                map_data.load((char*)data,ota_size);
                free(data);
            }
            else
                map_data.destroy();
            if( minimap_obj ) {	// Update the minimap on GUI
                gfx->destroy_texture( minimap_obj->Data );			// Make things clean
                minimap_obj->Data = mini;
                mini = 0;
                ldx = dx * ( mini_map_x2 - mini_map_x1 ) / 504.0f;
                    ldy = dy * ( mini_map_y2 - mini_map_y1 ) / 504.0f;
                    minimap_obj->x1 = mini_map_x-ldx;
                minimap_obj->y1 = mini_map_y-ldy;
                minimap_obj->x2 = mini_map_x+ldx;
                minimap_obj->y2 = mini_map_y+ldy;
                minimap_obj->u2 = dx/252.0f;
                minimap_obj->v2 = dy/252.0f;
            }

            String map_info = "";
            if(map_data.missionname)
                map_info += String( map_data.missionname ) + "\n";
            if(map_data.numplayers)
                map_info += "\n" + TRANSLATE("players: ") + String( map_data.numplayers ) + "\n";
            if(map_data.missiondescription)
                map_info += String( "\n" ) + map_data.missiondescription;
            mapsetup_area.set_caption("mapsetup.map_info", map_info );
        }

        mapsetup_area.draw();

        glEnable(GL_TEXTURE_2D);
        gfx->set_color(0xFFFFFFFF);
        draw_cursor();

        // Affiche
        gfx->flip();
    }while(!done);

    if( mapsetup_area.background == gfx->glfond )	mapsetup_area.background = 0;
    mapsetup_area.destroy();

    gfx->destroy_texture( mini );

    reset_mouse();
    while(key[KEY_ESC]) {	rest(1);	poll_keyboard();	}

    return choice;
}

void config_menu(void)
{
    cursor_type=CURSOR_DEFAULT;

    lp_CONFIG->Lang = LANG;

    if( lp_CONFIG->restorestart )
    {
        lp_CONFIG->restorestart = false;
        lp_CONFIG->quickstart = false;
    }

    TA3DCONFIG	saved_config = *lp_CONFIG;

    AREA config_area("config");
    config_area.load_tdf("gui/config.area");
    if( !config_area.background )	config_area.background = gfx->glfond;

    /*Vector< String > fps_limits = ReadVectorString( "50,60,70,80,90,100,no limit" );
      for( uint32 e = 0 ; e < fps_limits.size() ; e++ )
      fps_limits[ e ] = TRANSLATE( fps_limits[ e ] ); */
    Vector<String> fps_limits;
    fps_limits.push_back(TRANSLATE("15"));
    fps_limits.push_back(TRANSLATE("25"));
    fps_limits.push_back(TRANSLATE("30"));
    fps_limits.push_back(TRANSLATE("40"));
    fps_limits.push_back(TRANSLATE("50"));
    fps_limits.push_back(TRANSLATE("60"));
    fps_limits.push_back(TRANSLATE("70"));
    fps_limits.push_back(TRANSLATE("80"));
    fps_limits.push_back(TRANSLATE("90"));
    fps_limits.push_back(TRANSLATE("100"));
    fps_limits.push_back(TRANSLATE("no limit"));
    int	nb_res = 0;
    int res_width[100];
    int res_height[100];
    int res_bpp[100];

    GFX_MODE_LIST *mode_list = get_gfx_mode_list( GFX_OPENGL_FULLSCREEN );
    if (mode_list)
    {
        for( int i = 0 ; i < mode_list->num_modes ; ++i)
        {
            if( mode_list->mode[ i ].bpp == 32 )
            {
                bool found = mode_list->mode[ i ].width < 640 || mode_list->mode[ i ].height < 480;
                if( !found )
                {
                    for( int e = 0 ; e < nb_res ; e++ )
                    {
                        if( res_width[e] == mode_list->mode[ i ].width && res_height[e] == mode_list->mode[ i ].height )
                        {
                            found = true;
                            break;
                        }
                    }
                }

                if( mode_list->mode[ i ].height == 0 ||
                    ( mode_list->mode[ i ].width * 3 != 4 * mode_list->mode[ i ].height &&
                      mode_list->mode[ i ].width * 9 != 16 * mode_list->mode[ i ].height &&
                      mode_list->mode[ i ].width * 10 != 16 * mode_list->mode[ i ].height &&
                      mode_list->mode[ i ].width * 4 != 5 * mode_list->mode[ i ].height ) )
                {
                    found = true;
                }

                if( !found )
                {
                    res_bpp[ nb_res ] = 16;
                    res_width[ nb_res ] = mode_list->mode[ i ].width;
                    res_height[ nb_res++ ] = mode_list->mode[ i ].height;
                    res_bpp[ nb_res ] = 32;
                    res_width[ nb_res ] = mode_list->mode[ i ].width;
                    res_height[ nb_res++ ] = mode_list->mode[ i ].height;
                }
            }
        }
        destroy_gfx_mode_list( mode_list );
    }

    config_area.set_state("*.showfps", lp_CONFIG->showfps);
    switch( (int)lp_CONFIG->fps_limit )
    {
        case 50:  config_area.set_caption("*.fps_limit", fps_limits[0]); break;
        case 60:  config_area.set_caption("*.fps_limit", fps_limits[1]); break;
        case 70:  config_area.set_caption("*.fps_limit", fps_limits[2]); break;
        case 80:  config_area.set_caption("*.fps_limit", fps_limits[3]); break;
        case 90:  config_area.set_caption("*.fps_limit", fps_limits[4]); break;
        case 100: config_area.set_caption("*.fps_limit", fps_limits[5]); break;
        default:  config_area.set_caption("*.fps_limit", fps_limits[6]);
    };
    config_area.set_state("*.wireframe", lp_CONFIG->wireframe);
    config_area.set_state("*.particle", lp_CONFIG->particle);
    config_area.set_state("*.waves", lp_CONFIG->waves);
    config_area.set_state("*.shadow", lp_CONFIG->shadow);
    config_area.set_state("*.height_line", lp_CONFIG->height_line);
    config_area.set_state("*.detail_tex", lp_CONFIG->detail_tex);
    config_area.set_state("*.use_texture_cache", lp_CONFIG->use_texture_cache);
    config_area.set_state("*.draw_console_loading", lp_CONFIG->draw_console_loading);
    config_area.set_state("*.fullscreen", lp_CONFIG->fullscreen);
    if( config_area.get_object("*.LANG") )
        config_area.set_caption( "*.LANG", config_area.get_object("*.LANG")->Text[1+lp_CONFIG->Lang] );
    if( config_area.get_object("*.camera_zoom") )
        config_area.set_caption( "*.camera_zoom", config_area.get_object("*.camera_zoom")->Text[1+lp_CONFIG->camera_zoom] );
    config_area.set_caption( "*.camera_def_angle", format( "%f", lp_CONFIG->camera_def_angle ) );
    config_area.set_caption( "*.camera_def_h", format( "%f", lp_CONFIG->camera_def_h ) );
    config_area.set_caption( "*.camera_zoom_speed", format( "%f", lp_CONFIG->camera_zoom_speed ) );
    if( config_area.get_object("*.screenres") )
    {
        GUIOBJ *obj = config_area.get_object("*.screenres");
        obj->Text.clear();
        int current = 0;
        while( current < nb_res &&
               ( res_width[ current ] != lp_CONFIG->screen_width
                 || res_height[ current ] != lp_CONFIG->screen_height
                 || res_bpp[ current ] != lp_CONFIG->color_depth ) )
            current++;
        if( current >= nb_res )	current = 0;
        obj->Text.push_back( format( "%dx%dx%d", res_width[ current ], res_height[ current ], res_bpp[ current ] ) );
        for( int i = 0 ; i < nb_res ; i++ )
            obj->Text.push_back( format( "%dx%dx%d", res_width[ i ], res_height[ i ], res_bpp[ i ] ) );
    }
    if( config_area.get_object("*.shadow_quality") )
        config_area.set_caption( "*.shadow_quality", config_area.get_object("*.shadow_quality")->Text[1+min( (lp_CONFIG->shadow_quality-1)/3, 2 ) ] );
    config_area.set_caption("*.timefactor", format( "%d", (int)lp_CONFIG->timefactor ) );
    switch( lp_CONFIG->fsaa )
    {
        case 2:	config_area.set_caption("*.fsaa", "x2" );	break;
        case 4:	config_area.set_caption("*.fsaa", "x4" );	break;
        case 6:	config_area.set_caption("*.fsaa", "x6" );	break;
        case 8:	config_area.set_caption("*.fsaa", "x8" );	break;
        default: config_area.set_caption("*.fsaa", "no fsaa" );
    }
    if( config_area.get_object("*.water_quality") )
    {
        GUIOBJ *obj = config_area.get_object("*.water_quality");
        config_area.set_caption("*.water_quality", obj->Text[ 1 + lp_CONFIG->water_quality ] );
    }

    if( config_area.get_object("*.mod") )
    {
        GUIOBJ *obj = config_area.get_object("*.mod");

        if( obj->Text.size() >= 2 )
            obj->Text[0] = obj->Text[1];
        else
            obj->Text.resize( 1 );

        al_ffblk search;
        String current_selection = TA3D_CURRENT_MOD.length() > 6 ? TA3D_CURRENT_MOD.substr( 5, TA3D_CURRENT_MOD.length() - 6 ) : "";

        if( al_findfirst( "mods/*", &search, FA_RDONLY | FA_DIREC ) == 0 ) 
        {
            do
            {
                if( String( search.name ) != ".." && String( search.name ) != "." ) {		// Have to exclude both .. & . because of windows finding . as something interesting
                    obj->Text.push_back( search.name );
                    if( Lowercase( search.name ) == Lowercase( current_selection ) )
                        obj->Text[0] = search.name;
                }
            } while( al_findnext( &search ) == 0 );

            al_findclose(&search);
        }
    }

    config_area.set_caption("*.player_name", lp_CONFIG->player_name );

    if( config_area.get_object("*.skin") )
    {
        GUIOBJ *obj = config_area.get_object("*.skin");

        obj->Text.resize( 1 );
        obj->Text[ 0 ] = TRANSLATE( "default.skn" );

        List<String> skin_list;
        uint32 n = HPIManager->GetFilelist("gui\\*.skn",&skin_list);

        for( List< String >::iterator i = skin_list.begin() ; i != skin_list.end() ; i++ )
        {
            obj->Text.push_back( i->substr( 4, i->size() - 4 ) );
            if( "gui/" + Lowercase( i->substr( 4, i->size() - 4 ) ) == Lowercase( lp_CONFIG->skin_name ) )
                obj->Text[0] = i->substr( 4, i->size() - 4 );
        }
    }

    if( config_area.get_object("*.l_files") )
    {
        GUIOBJ *obj = config_area.get_object("*.l_files");
        obj->Text = sound_manager->GetPlayListFiles();
    }

    if( lp_CONFIG->quickstart )
        I_Msg( TA3D::TA3D_IM_GUI_MSG, (void*)("config_confirm.show"), NULL, NULL );

    bool done=false;

    bool save=false;

    int amx = -1;
    int amy = -1;
    int amz = -1;
    int amb = -1;
    uint32 timer = msec_timer;

    do
    {
        bool time_out = false;
        bool key_is_pressed = false;
        do
        {
            key_is_pressed = keypressed();
            if( lp_CONFIG->quickstart )
            {
                GUIOBJ *pbar = config_area.get_object( "config_confirm.p_wait" );
                if( pbar )
                {
                    int new_value = (msec_timer - timer) / 50;
                    if( new_value != pbar->Data )
                    {
                        pbar->Data = new_value;
                        key_is_pressed = true;
                        if( new_value == 100 )
                            time_out = true;
                    }
                }
            }
            config_area.check();
            rest( 1 );
        } while( amx == mouse_x && amy == mouse_y && amz == mouse_z && amb == mouse_b && !key[ KEY_ENTER ] && !key[ KEY_ESC ] && !done && !key_is_pressed && !config_area.scrolling );

        amx = mouse_x;
        amy = mouse_y;
        amz = mouse_z;
        amb = mouse_b;

        if( lp_CONFIG->quickstart )
        {
            if( time_out || config_area.get_state("config_confirm.b_cancel_changes" ) || key[KEY_ESC] )
            {
                I_Msg( TA3D::TA3D_IM_GUI_MSG, (void*)("config_confirm.hide"), NULL, NULL );
                TA3D::Settings::Restore(TA3D::Paths::ConfigFile);
                TA3D::Settings::Load();
                done = true;
                save = false;
                lp_CONFIG->quickstart = false;
                lp_CONFIG->quickrestart = true;
                lp_CONFIG->restorestart = true;
                saved_config = *lp_CONFIG;
            }
            else
                if( config_area.get_state("config_confirm.b_confirm" ) )
                {
                    I_Msg( TA3D::TA3D_IM_GUI_MSG, (void*)("config_confirm.hide"), NULL, NULL );
                    lp_CONFIG->quickstart = false;
                    saved_config.quickstart = false;
                }
        }

        if( config_area.get_state( "*.b_activate" ) )
        {
            GUIOBJ *obj = config_area.get_object("*.l_files");
            if( obj && obj->Pos >= 0 && obj->Text.size() > obj->Pos )
            {
                sound_manager->SetPlayListFileMode( obj->Pos, false, false );
                obj->Text[ obj->Pos ][ 1 ] = '*';
            }
        }
        if( config_area.get_state( "*.b_deactivate" ) )
        {
            GUIOBJ *obj = config_area.get_object("*.l_files");
            if( obj && obj->Pos >= 0 && obj->Text.size() > obj->Pos )
            {
                sound_manager->SetPlayListFileMode( obj->Pos, false, true );
                obj->Text[ obj->Pos ][ 1 ] = ' ';
            }
        }
        if( config_area.get_state( "*.b_battle" ) ) {
            GUIOBJ *obj = config_area.get_object("*.l_files");
            if( obj && obj->Pos >= 0 && obj->Text.size() > obj->Pos ) {
                sound_manager->SetPlayListFileMode( obj->Pos, true, false );
                obj->Text[ obj->Pos ][ 1 ] = 'B';
            }
        }


        if( config_area.get_state( "*.b_ok" ) ) {
            done = true;		// En cas de click sur "OK", on quitte la fenêtre
            save = true;
        }
        if( config_area.get_state( "*.b_cancel" ) ) done=true;		// En cas de click sur "retour", on quitte la fenêtre

        lp_CONFIG->showfps = config_area.get_state( "*.showfps" );
        if( config_area.get_value( "*.fps_limit" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.fps_limit" );
            if( obj && obj->Data != -1 ) {
                obj->Text[0] = fps_limits[ obj->Value ];
                switch( obj->Value )
                {
                    case 0:		lp_CONFIG->fps_limit = 50;	break;
                    case 1:		lp_CONFIG->fps_limit = 60;	break;
                    case 2:		lp_CONFIG->fps_limit = 70;	break;
                    case 3:		lp_CONFIG->fps_limit = 80;	break;
                    case 4:		lp_CONFIG->fps_limit = 90;	break;
                    case 5:		lp_CONFIG->fps_limit = 100;	break;
                    default:
                                lp_CONFIG->fps_limit = -1;
                };
            }
        }
        lp_CONFIG->wireframe = config_area.get_state( "*.wireframe" );
        lp_CONFIG->particle = config_area.get_state( "*.particle" );
        lp_CONFIG->waves = config_area.get_state( "*.waves" );
        lp_CONFIG->shadow = config_area.get_state( "*.shadow" );
        lp_CONFIG->height_line = config_area.get_state( "*.height_line" );
        lp_CONFIG->detail_tex = config_area.get_state( "*.detail_tex" );
        lp_CONFIG->draw_console_loading = config_area.get_state( "*.draw_console_loading" );
        lp_CONFIG->fullscreen = config_area.get_state( "*.fullscreen" );
        lp_CONFIG->use_texture_cache = config_area.get_state( "*.use_texture_cache" );
        if( config_area.get_value( "*.camera_zoom" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.camera_zoom" );
            if( obj && obj->Value >= -1 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                lp_CONFIG->camera_zoom = obj->Value;
            }
        }
        if( config_area.get_value( "*.camera_def_angle" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.camera_def_angle" );
            if( obj && obj->Value >= 0 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                lp_CONFIG->camera_def_angle = atof( obj->Text[0].c_str() );
            }
        }
        if( config_area.get_value( "*.camera_def_h" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.camera_def_h" );
            if( obj && obj->Value >= 0 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                lp_CONFIG->camera_def_h = atof( obj->Text[0].c_str() );
            }
        }
        if( config_area.get_value( "*.camera_zoom_speed" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.camera_zoom_speed" );
            if( obj && obj->Value >= 0 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                lp_CONFIG->camera_zoom_speed = atof( obj->Text[0].c_str() );
            }
        }
        if( config_area.get_value( "*.LANG" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.LANG" );
            if( obj && obj->Value != -1 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                lp_CONFIG->Lang = obj->Value;
            }
        }
        if( config_area.get_value( "*.screenres" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.screenres" );
            if( obj && obj->Value != -1 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                lp_CONFIG->screen_width = res_width[ obj->Value ];
                lp_CONFIG->screen_height = res_height[ obj->Value ];
                lp_CONFIG->color_depth = res_bpp[ obj->Value ];
            }
        }
        if( config_area.get_value( "*.shadow_quality" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.shadow_quality" );
            if( obj && obj->Value != -1 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                lp_CONFIG->shadow_quality = obj->Value * 3 + 1;
            }
        }
        if( config_area.get_value( "*.timefactor" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.timefactor" );
            if( obj && obj->Value != -1 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                lp_CONFIG->timefactor = obj->Value + 1;
            }
        }
        if( config_area.get_value( "*.fsaa" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.fsaa" );
            if( obj && obj->Value != -1 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                lp_CONFIG->fsaa = obj->Value << 1;
            }
        }
        if( config_area.get_value( "*.water_quality" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.water_quality" );
            if( obj && obj->Value != -1 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                lp_CONFIG->water_quality = obj->Value;
            }
        }
        if( config_area.get_value( "*.mod" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.mod" );
            if( obj && obj->Value != -1 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                lp_CONFIG->last_MOD = obj->Value > 0 ? "mods/" + obj->Text[0] + "/" : "";
            }
        }
        if( config_area.get_value( "*.skin" ) >= 0 ) {
            GUIOBJ *obj = config_area.get_object( "*.skin" );
            if( obj && obj->Value != -1 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                lp_CONFIG->skin_name = obj->Value > 0 ? "gui/" + obj->Text[0] : "";
            }
        }

        if(key[KEY_ESC]) done=true;			// Quitte si on appuie sur echap

        config_area.draw();

        glEnable(GL_TEXTURE_2D);
        gfx->set_color(0xFFFFFFFF);
        draw_cursor();

        // Affiche
        gfx->flip();
    }while(!done);

    if( config_area.background == gfx->glfond )	config_area.background = 0;

    reset_mouse();
    while(key[KEY_ESC]) {	rest(1);	poll_keyboard();	}

    bool ask_for_quickrestart = lp_CONFIG->quickrestart;

    if(!save)
        *lp_CONFIG = saved_config;
    else {
        sound_manager->SavePlayList();				// Save the playlist

        if( lp_CONFIG->screen_width != saved_config.screen_width ||
            lp_CONFIG->screen_height != saved_config.screen_height ||
            lp_CONFIG->color_depth != saved_config.color_depth ||
            lp_CONFIG->fsaa != saved_config.fsaa ||
            (lp_CONFIG->fullscreen != saved_config.fullscreen) )			// Need to restart
            lp_CONFIG->quickrestart = true;

        lp_CONFIG->player_name = config_area.get_caption( "*.player_name" );

        if( lp_CONFIG->last_MOD != TA3D_CURRENT_MOD ) {			// Refresh the file structure
            TA3D_CURRENT_MOD = lp_CONFIG->last_MOD;
            delete HPIManager;

            TA3D_clear_cache();		// Clear the cache

            HPIManager = new cHPIHandler("");
            ta3d_sidedata.load_data();				// Refresh side data so we load the correct values

            delete sound_manager;
            sound_manager = new TA3D::INTERFACES::cAudio ( 1.0f, 0.0f, 0.0f );
            sound_manager->StopMusic();
            sound_manager->LoadTDFSounds( true );
            sound_manager->LoadTDFSounds( false );
        }
    }

    lp_CONFIG->quickrestart |= ask_for_quickrestart;

    config_area.destroy();

    LANG = lp_CONFIG->Lang;

    i18n.refresh_language();   // refresh the language used by the i18n object
}

void stats_menu(void)
{
    cursor_type=CURSOR_DEFAULT;

    gfx->set_2D_mode();

    gfx->ReInitTexSys();

    reset_keyboard();
    while(key[KEY_ESC])	rest(1);

    AREA statistics_area("statistics");
    statistics_area.load_tdf("gui/statistics.area");
    if( !statistics_area.background )	statistics_area.background = gfx->glfond;

    for( int i = 0 ; i < players.nb_player ; i++ ) {
        GUIOBJ *obj;
        uint32 color = gfx->makeintcol( player_color[ 3 * player_color_map[ i ] ], player_color[ 3 * player_color_map[ i ] + 1 ], player_color[ 3 * player_color_map[ i ] + 2 ] );

        statistics_area.set_caption( format( "statistics.player%d", i ), players.nom[i] );
        obj = statistics_area.get_object( format( "statistics.player%d", i ) );
        if( obj )	obj->Data = color;

        statistics_area.set_caption( format( "statistics.side%d", i ), players.side[i] );
        obj = statistics_area.get_object( format( "statistics.side%d", i ) );
        if( obj )	obj->Data = color;

        statistics_area.set_caption( format( "statistics.losses%d", i ), format( "%d", players.losses[i] ) );
        obj = statistics_area.get_object( format( "statistics.losses%d", i ) );
        if( obj )	obj->Data = color;

        statistics_area.set_caption( format( "statistics.kills%d", i ), format( "%d", players.kills[i] ) );
        obj = statistics_area.get_object( format( "statistics.kills%d", i ) );
        if( obj )	obj->Data = color;

        statistics_area.set_caption( format( "statistics.energy%d", i ), format( "%d", (int)players.energy_total[i] ) );
        obj = statistics_area.get_object( format( "statistics.energy%d", i ) );
        if( obj )	obj->Data = color;

        statistics_area.set_caption( format( "statistics.metal%d", i ), format( "%d", (int)players.metal_total[i] ) );
        obj = statistics_area.get_object( format( "statistics.metal%d", i ) );
        if( obj )	obj->Data = color;
    }

    bool done=false;

    int amx = -1;
    int amy = -1;
    int amz = -1;
    int amb = -1;

    do
    {
        bool key_is_pressed = false;
        do {
            key_is_pressed = keypressed();
            statistics_area.check();
            rest( 1 );
        } while( amx == mouse_x && amy == mouse_y && amz == mouse_z && amb == mouse_b && mouse_b == 0 && !key[ KEY_ENTER ] && !key[ KEY_ESC ] && !done && !key_is_pressed && !statistics_area.scrolling );

        amx = mouse_x;
        amy = mouse_y;
        amz = mouse_z;
        amb = mouse_b;

        if( statistics_area.get_state( "statistics.b_ok" ) || key[KEY_ENTER] ) {
            while( key[KEY_ENTER] )	{	rest( 20 );	poll_keyboard();	}
            clear_keybuf();
            done=true;		// If user click "OK" or hit enter then leave the window
        }

        if(key[KEY_ESC]) done=true;			// Quitte si on appuie sur echap

        statistics_area.draw();

        glEnable(GL_TEXTURE_2D);
        gfx->set_color(0xFFFFFFFF);
        draw_cursor();

        // Affiche
        gfx->flip();
    }while(!done);

    if( statistics_area.background == gfx->glfond )	statistics_area.background = 0;
    statistics_area.destroy();

    gfx->unset_2D_mode();	// Quitte le mode de dessin d'allegro

    reset_mouse();
    while(key[KEY_ESC]) {	rest(1);	poll_keyboard();	}
}

#define INTERNET_AD_COUNTDOWN		150000

void setup_game(bool client, const char *host)
{
    int my_player_id = -1;
    bool advertise = false;
    if( host ) {
        if( !client ) {
            network_manager.InitBroadcast("1234");		// broadcast mode
            network_manager.HostGame( (char*) host, "4242", 2 );
        }
        else
            network_manager.Connect( (char*) host, "4242" );

        my_player_id = network_manager.getMyID();			// Get player id

        if( client ) {
            special msg;
            network_manager.sendSpecial( strtochat( &msg, format( "NOTIFY NEW_PLAYER %s", ReplaceChar( lp_CONFIG->player_name, ' ', 1 ).c_str() ) ) );
            rest(10);
            network_manager.sendSpecial( strtochat( &msg, "REQUEST GAME_DATA" ) );
        }
    }

    cursor_type=CURSOR_DEFAULT;

    uint16	player_str_n = 4;
    uint16	ai_level_str_n = 4;
    String	player_str[4] = { lp_CONFIG->player_name, TRANSLATE("computer"), TRANSLATE("open"), TRANSLATE("closed") };
    byte	player_control[4] = { PLAYER_CONTROL_LOCAL_HUMAN, PLAYER_CONTROL_LOCAL_AI, PLAYER_CONTROL_NONE, PLAYER_CONTROL_CLOSED };
    String	ai_level_str[4] = { TRANSLATE("easy"), TRANSLATE("medium"), TRANSLATE("hard"), TRANSLATE("bloody") };
    uint16	side_str_n = ta3d_sidedata.nb_side;
    Vector<String>	side_str;

    side_str.resize( ta3d_sidedata.nb_side );
    for( int i = 0 ; i < ta3d_sidedata.nb_side ; i++ )			// Get side data
        side_str[ i ] = ta3d_sidedata.side_name[ i ];

    GAME_DATA game_data;

    if( HPIManager->Exists( lp_CONFIG->last_map ) )
        game_data.map_filename = strdup( lp_CONFIG->last_map.c_str() );
    else {
        List<String> map_list;
        uint32 n = HPIManager->GetFilelist("maps\\*.tnt",&map_list);

        if( n == 0 ) {
            network_manager.Disconnect();
            Popup(TRANSLATE("Error"),TRANSLATE("No maps found"));
            Console->AddEntry("no maps found!!");
            reset_mouse();
            return;
        }
        game_data.map_filename = strdup( map_list.begin()->c_str() );
        map_list.clear();
    }
    game_data.nb_players = 2;
    if( HPIManager->Exists( lp_CONFIG->last_script ) && Lowercase( lp_CONFIG->last_script.substr( lp_CONFIG->last_script.length() - 3 , 3 ) ) == "lua" )
        game_data.game_script = strdup( lp_CONFIG->last_script.c_str() );
    else {
        List<String> script_list;
        uint32 n = HPIManager->GetFilelist("scripts\\*.lua",&script_list);

        if( n == 0 ) {
            network_manager.Disconnect();
            Popup(TRANSLATE("Error"),TRANSLATE("No scripts found"));
            Console->AddEntry("no scripts found!!");
            reset_mouse();
            return;
        }
        game_data.game_script = strdup( script_list.begin()->c_str() );
        script_list.clear();
    }
    game_data.fog_of_war = lp_CONFIG->last_FOW;

    for(uint16 i = 0 ; i < 10 ; i++) {
        game_data.player_names[i] = player_str[2];
        game_data.player_sides[i] = side_str[0];
        game_data.player_control[i] = player_control[2];
        game_data.ai_level[i] = AI_TYPE_EASY;
    }

    if( !client ) {
        game_data.player_names[0] = player_str[0];
        game_data.player_sides[0] = side_str[0];
        game_data.player_control[0] = player_control[0];
        game_data.player_network_id[0] = my_player_id;
        game_data.ai_level[0] = AI_TYPE_EASY;

        if( !host ) {
            game_data.player_names[1] = player_str[1];
            game_data.player_sides[1] = side_str[1];
            game_data.player_control[1] = player_control[1];
            game_data.ai_level[1] = AI_TYPE_EASY;
        }
    }

    int dx, dy;
    GLuint glimg = load_tnt_minimap_fast(game_data.map_filename,&dx,&dy);
    char tmp_char[1024];
    MAP_OTA	map_data;
    map_data.load( replace_extension( (char*)tmp_char, game_data.map_filename, "ota", 1024 ) );
    float ldx = dx*70.0f/252.0f;
        float ldy = dy*70.0f/252.0f;

        AREA setupgame_area("setup");
    setupgame_area.load_tdf("gui/setupgame.area");
    if( !setupgame_area.background )	setupgame_area.background = gfx->glfond;
    for(uint16 i = 0 ; i < 10 ; i++ ) {
        setupgame_area.set_caption( format("gamesetup.name%d", i), game_data.player_names[i] );
        setupgame_area.set_caption( format("gamesetup.side%d", i), game_data.player_sides[i] );
        setupgame_area.set_caption( format("gamesetup.ai%d", i), game_data.player_control[i] & PLAYER_CONTROL_FLAG_AI ? ai_level_str[game_data.ai_level[i]].c_str() : "" );
        GUIOBJ *guiobj = setupgame_area.get_object( format("gamesetup.color%d", i) );
        if( guiobj ) {
            guiobj->Flag |= (game_data.player_control[i] == PLAYER_CONTROL_NONE ? FLAG_HIDDEN : 0 );
            guiobj->Data = gfx->makeintcol(player_color[player_color_map[i]*3], player_color[player_color_map[i]*3+1], player_color[player_color_map[i]*3+2]);
        }
        setupgame_area.set_caption( format("gamesetup.energy%d", i), format("%d",game_data.energy[i]) );
        setupgame_area.set_caption( format("gamesetup.metal%d", i), format("%d",game_data.metal[i]) );
    }

    GUIOBJ *minimap_obj = setupgame_area.get_object( "gamesetup.minimap" );
    float mini_map_x1 = 0.0f;
    float mini_map_y1 = 0.0f;
    float mini_map_x2 = 0.0f;
    float mini_map_y2 = 0.0f;
    float mini_map_x = 0.0f;
    float mini_map_y = 0.0f;
    if( minimap_obj ) {
        mini_map_x1 = minimap_obj->x1;
        mini_map_y1 = minimap_obj->y1;
        mini_map_x2 = minimap_obj->x2;
        mini_map_y2 = minimap_obj->y2;
        ldx = dx * ( mini_map_x2 - mini_map_x1 ) / 504.0f;
        ldy = dy * ( mini_map_y2 - mini_map_y1 ) / 504.0f;

        mini_map_x = (mini_map_x1 + mini_map_x2) * 0.5f;
        mini_map_y = (mini_map_y1 + mini_map_y2) * 0.5f;

        minimap_obj->Data = glimg;
        minimap_obj->x1 = mini_map_x - ldx;
        minimap_obj->y1 = mini_map_y - ldy;
        minimap_obj->x2 = mini_map_x + ldx;
        minimap_obj->y2 = mini_map_y + ldy;
        minimap_obj->u2 = dx / 252.0f;
        minimap_obj->v2 = dy / 252.0f;
    }

    GUIOBJ *guiobj = setupgame_area.get_object( "scripts.script_list" );
    if( guiobj ) {
        std::list< String > script_list;
        HPIManager->GetFilelist("scripts\\*.lua",&script_list);
        for(std::list< String >::iterator i_script = script_list.begin() ; i_script != script_list.end() ; i_script++ )
            guiobj->Text.push_back( *i_script );
    }
    setupgame_area.set_caption( "gamesetup.script_name", game_data.game_script );
    {
        GUIOBJ *obj = setupgame_area.get_object( "gamesetup.FOW" );
        if( obj )
            obj->Text[0] = obj->Text[ 1 + game_data.fog_of_war ];
    }

    {
        String map_info = "";
        if(map_data.missionname)
            map_info += String( map_data.missionname ) + "\n";
        if(map_data.numplayers)
            map_info += String( map_data.numplayers ) + "\n";
        if(map_data.missiondescription)
            map_info += map_data.missiondescription;
        setupgame_area.set_caption("gamesetup.map_info", map_info );
    }

    if( !host )
        for( int i = 0 ; i < 10 ; i++ )
            setupgame_area.msg(format("gamesetup.ready%d.hide",i));
    uint32 player_timer[10];
    for( int i = 0 ; i < 10 ; i++ )
        player_timer[i] = msec_timer;

    bool done=false;

    if( host && my_player_id == -1 )	done = true;		// Leave now, we aren't connected but we're in network mode

    bool start_game = false;

    int amx = -1;
    int amy = -1;
    int amz = -1;
    int amb = -1;

    String set_map = "";
    String previous_tnt_port = "";
    String previous_ota_port = "";
    String previous_lua_port = "";

    if( host && client )
        setupgame_area.msg("gamesetup.b_ok.disable");

    if( host && !client )
        setupgame_area.msg("gamesetup.advertise.show");

    int progress_timer = msec_timer;
    int ping_timer = msec_timer;					// Used to send simple PING requests in order to detect when a connection fails

    int internet_ad_timer = msec_timer - INTERNET_AD_COUNTDOWN;	// Resend every 150 sec

    do
    {
        if( host )
            for( int i = 0 ; i < 10 ; i++ ) {
                GUIOBJ *obj = setupgame_area.get_object(format("gamesetup.ready%d",i));
                if( obj ) {
                    if(game_data.player_control[i] != PLAYER_CONTROL_LOCAL_HUMAN && game_data.player_control[i] != PLAYER_CONTROL_REMOTE_HUMAN)
                        obj->Flag |= FLAG_HIDDEN;
                    else
                        obj->Flag &= ~FLAG_HIDDEN;
                    obj->Etat = game_data.ready[i];
                }
            }

        bool key_is_pressed = false;
        String broadcast_msg = "";
        String chat_msg = "";
        String special_msg = "";
        struct chat received_chat_msg;
        struct chat received_special_msg;
        bool playerDropped = false;
        do {
            playerDropped = network_manager.getPlayerDropped();
            broadcast_msg = network_manager.getNextBroadcastedMessage();
            if( network_manager.getNextChat( &received_chat_msg ) == 0 )
                chat_msg = received_chat_msg.message;
            if( network_manager.getNextSpecial( &received_special_msg ) == 0 )
                special_msg = received_special_msg.message;
            if( host && !network_manager.isConnected() ) {					// We're disconnected !!
                done = true;
                break;
            }
            key_is_pressed = keypressed();
            setupgame_area.check();
            rest( 1 );
            if( msec_timer - progress_timer >= 500 && network_manager.getFileTransferProgress() != 100.0f )	break;
        } while( amx == mouse_x && amy == mouse_y && amz == mouse_z && amb == mouse_b && mouse_b == 0 && !key[ KEY_ENTER ] && !key[ KEY_ESC ] && !done
                 && !key_is_pressed && !setupgame_area.scrolling && broadcast_msg.empty() && chat_msg.empty() && special_msg.empty() && !playerDropped
                 && ( (msec_timer - ping_timer < 2000 && (msec_timer - internet_ad_timer >= INTERNET_AD_COUNTDOWN || !advertise) ) || host == NULL || client ) );

        //-------------------------------------------------------------- Network Code : syncing information --------------------------------------------------------------

        if( setupgame_area.get_state( "gamesetup.advertise" ) != advertise ) {
            advertise ^= true;
            if( advertise )
                internet_ad_timer = msec_timer - INTERNET_AD_COUNTDOWN;
            else
                network_manager.registerToNetServer( host, 0 );
        }

        if( host && !client && msec_timer - internet_ad_timer >= INTERNET_AD_COUNTDOWN && advertise ) {		// Advertise the game on the Internet
            internet_ad_timer = msec_timer;	// Resend every 150 sec
            uint16 nb_open = 0;
            for( uint16 f = 0 ; f < 10 ; f++ )
                if( setupgame_area.get_caption(format("gamesetup.name%d", f)) == player_str[2] ) 
                    nb_open++;
            network_manager.registerToNetServer( host, nb_open );
        }

        if( host && !client && msec_timer - ping_timer >= 2000 ) {		// Send a ping request
            network_manager.sendPing();
            ping_timer = msec_timer;

            for( int i = 0 ; i < 10 ; i++ )					// ping time out
                if( game_data.player_network_id[i] > 0 && msec_timer - player_timer[ i ] > 10000 ) {
                    network_manager.dropPlayer( game_data.player_network_id[i] );
                    playerDropped = true;
                }
        }

        if( network_manager.getFileTransferProgress() != 100.0f ) {
            progress_timer = msec_timer;
            GUIOBJ *obj = setupgame_area.get_object( "gamesetup.p_transfer" );
            if( obj ) {
                obj->Flag &= ~FLAG_HIDDEN;
                int progress = (int)network_manager.getFileTransferProgress();
                obj->Data = progress;
            }
        }
        else {
            GUIOBJ *obj = setupgame_area.get_object( "gamesetup.p_transfer" );
            if( obj )	obj->Flag |= FLAG_HIDDEN;
        }

        if( playerDropped ) {
            for( int i = 0 ; i < 10 ; i++ )
                if( game_data.player_network_id[i] > 0 && !network_manager.pollPlayer( game_data.player_network_id[i] ) ) {	// Remove player i
                    game_data.player_names[i] = player_str[2];
                    game_data.player_sides[i] = side_str[0];
                    game_data.player_control[i] = player_control[2];
                    game_data.ai_level[i] = AI_TYPE_EASY;
                    game_data.player_network_id[i] = -1;

                    setupgame_area.set_caption( format( "gamesetup.name%d", i ),game_data.player_names[i]);									// Update gui
                    setupgame_area.set_caption( format( "gamesetup.ai%d", i ), (game_data.player_control[i] & PLAYER_CONTROL_FLAG_AI) ? ai_level_str[game_data.ai_level[i]] : String("") );
                    setupgame_area.set_caption( format("gamesetup.side%d", i) , side_str[0] );							// Update gui

                    GUIOBJ *guiobj =  setupgame_area.get_object( format("gamesetup.color%d", i) );
                    if( guiobj )
                        guiobj->Flag |= FLAG_HIDDEN;
                    guiobj =  setupgame_area.get_object( format("gamesetup.ai%d", i) );
                    if( guiobj )
                        guiobj->Flag |= FLAG_HIDDEN;
                }
        }

        while( !special_msg.empty() ) // Special receiver (sync config data)
        {
            int from = received_special_msg.from;
            Vector<String> params;
            ReadVectorString(params, received_special_msg.message, " ");
            if( params.size() == 1 )
            {
                if( params[0] == "PONG" )
                {
                    int player_id = game_data.net2id( from );
                    if( player_id >= 0 )
                        player_timer[ player_id ] = msec_timer;
                }
            }
            else
                if( params.size() == 2 )
                {
                    if( params[0] == "REQUEST" )
                    {
                        if( params[1] == "PLAYER_ID" )					// Sending player's network ID
                            network_manager.sendSpecial( format( "RESPONSE PLAYER_ID %d", from ), -1, from );
                        else if( params[1] == "GAME_DATA" ) {			// Sending game information
                            for( int i = 0 ; i < 10 ; i++ ) {			// Send player information
                                if( client && game_data.player_network_id[i] != my_player_id )	continue;		// We don't send updates about things we wan't update
                                String msg;								// SYNTAX: PLAYER_INFO player_id network_id side_id ai_level metal energy player_name
                                int side_id = find( side_str, game_data.player_sides[i] );
                                msg = format( "PLAYER_INFO %d %d %d %d %d %d %s %d", 	i, game_data.player_network_id[i],
                                              side_id, (game_data.player_control[i] == PLAYER_CONTROL_NONE || game_data.player_control[i] == PLAYER_CONTROL_CLOSED) ? -1 : game_data.ai_level[i],
                                              game_data.metal[i], game_data.energy[i],
                                              ReplaceChar(game_data.player_names[i], ' ', 1).c_str(), game_data.ready[i] );
                                network_manager.sendSpecial( msg, -1, from );
                            }
                            if( !client ) {			// Send server to client specific information (player colors, map name, ...)
                                String msg = "PLAYERCOLORMAP";
                                for( int i = 0 ; i < 10 ; i++ )
                                    msg += format( " %d", player_color_map[i] );
                                network_manager.sendSpecial( msg, -1, from );

                                network_manager.sendSpecial( format( "SET FOW %d", game_data.fog_of_war ), -1, from );
                                network_manager.sendSpecial( format( "SET SCRIPT %s", ReplaceChar( game_data.game_script, ' ', 1 ).c_str() ), -1, from );
                                network_manager.sendSpecial( format( "SET MAP %s", ReplaceChar( game_data.map_filename, ' ', 1 ).c_str() ), -1, from );
                            }
                        }
                    }
                    else if( params[0] == "NOTIFY" ) {
                        if( params[1] == "UPDATE" )
                            network_manager.sendSpecial( "REQUEST GAME_DATA" );			// We're told there are things to update, so ask for update
                        else if( params[1] == "PLAYER_LEFT" ) {
                            network_manager.dropPlayer( from );
                            network_manager.sendSpecial( "REQUEST GAME_DATA" );			// We're told there are things to update, so ask for update
                            for( int i = 0 ; i < 10 ; i++ )
                                if( game_data.player_network_id[i] == from ) {
                                    game_data.player_network_id[i] = -1;
                                    game_data.player_control[i] = player_control[2];
                                    game_data.player_names[i] = player_str[2];

                                    setupgame_area.set_caption( format( "gamesetup.name%d", i ),game_data.player_names[i]);			// Update gui

                                    GUIOBJ *guiobj =  setupgame_area.get_object( format("gamesetup.color%d", i) );
                                    if( guiobj )	guiobj->Flag |= FLAG_HIDDEN;
                                    break;
                                }
                        }
                        else if( params[1] == "START" ) {				// Game is starting ...
                            clear_keybuf();
                            done=true;		// If user click "OK" or hit enter then leave the window
                            start_game = true;
                        }
                    }
                }
                else if( params.size() == 3 ) {
                    if( params[0] == "NOTIFY" ) {
                        if( params[1] == "NEW_PLAYER" ) {								// Add new player
                            int slot = -1;
                            for( int i = 0 ; i < 10 ; i++ )
                                if( game_data.player_control[i] == PLAYER_CONTROL_NONE ) {
                                    slot = i;
                                    break;
                                }
                            if( slot >= 0 ) {
                                player_timer[ slot ] = msec_timer;				// If we forget this, player will be droped immediately
                                game_data.player_network_id[slot] = from;
                                game_data.player_control[slot] = PLAYER_CONTROL_REMOTE_HUMAN;
                                game_data.player_names[slot] = ReplaceChar( params[2], 1, ' ' );
                                setupgame_area.set_caption( format( "gamesetup.name%d", slot ), game_data.player_names[slot]);						// Update gui

                                GUIOBJ *guiobj =  setupgame_area.get_object( format("gamesetup.color%d", slot) );
                                if( guiobj ) {
                                    guiobj->Data = gfx->makeintcol(player_color[player_color_map[slot]*3],player_color[player_color_map[slot]*3+1],player_color[player_color_map[slot]*3+2]);			// Update gui
                                    guiobj->Flag &= ~FLAG_HIDDEN;
                                }
                                network_manager.sendSpecial( "NOTIFY UPDATE", from );			// Tell others that things have changed
                            }
                            else
                                network_manager.dropPlayer( from );		// No more room for this player !!
                        }
                        else if( params[1] == "COLORCHANGE" ) {
                            int i = atoi( params[2].c_str() );
                            if( !client ) {						// From client to server only
                                sint16 e = player_color_map[i];
                                sint16 f = -1;
                                for( sint16 g = 0; g<10 ; g++ )						// Look for the next color
                                    if( (game_data.player_control[g] == PLAYER_CONTROL_NONE || game_data.player_control[g] == PLAYER_CONTROL_CLOSED) && player_color_map[g] > e && (f == -1 || player_color_map[g] < player_color_map[f]) )
                                        f = g;
                                if( f == -1 )
                                    for( uint16 g = 0; g<10 ; g++ )
                                        if( (game_data.player_control[g] == PLAYER_CONTROL_NONE || game_data.player_control[g] == PLAYER_CONTROL_CLOSED) && (f == -1 || player_color_map[g] < player_color_map[f]) )
                                            f = g;
                                if( f != -1 ) {
                                    sint16 g = player_color_map[f];
                                    player_color_map[i] = g;								// update game data
                                    player_color_map[f] = e;

                                    guiobj =  setupgame_area.get_object( format("gamesetup.color%d", i) );
                                    if( guiobj )
                                        guiobj->Data = gfx->makeintcol(player_color[player_color_map[i]*3],player_color[player_color_map[i]*3+1],player_color[player_color_map[i]*3+2]);			// Update gui
                                    guiobj =  setupgame_area.get_object( format("gamesetup.color%d", f) );
                                    if( guiobj )
                                        guiobj->Data = gfx->makeintcol(player_color[player_color_map[f]*3],player_color[player_color_map[f]*3+1],player_color[player_color_map[f]*3+2]);			// Update gui
                                }
                                network_manager.sendSpecial("NOTIFY UPDATE");
                            }
                        }
                    }
                    else if( params[0] == "SET" ) {
                        if( params[1] == "FOW" ) {
                            int value = atoi( params[2].c_str() );
                            GUIOBJ *obj = setupgame_area.get_object( "gamesetup.FOW" );
                            if( obj && value >= 0 && value < 4 ) {
                                obj->Value = value;
                                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                                game_data.fog_of_war = obj->Value;
                            }
                        }
                        else if( params[1] == "MAP" ) {
                            set_map = ReplaceChar( params[2], 1, ' ' );
                            if( set_map == game_data.map_filename )	set_map = "";		// Don't reload map !!
                        }
                        else if( params[1] == "SCRIPT" ) {
                            String script_name = ReplaceChar( params[2], 1, ' ' );
                            if( script_name != game_data.game_script ) {
                                setupgame_area.set_caption( "gamesetup.script_name", script_name );
                                free( game_data.game_script );
                                game_data.game_script = strdup( script_name.c_str() );

                                if( client && !HPIManager->Exists( script_name.c_str() ) ) {
                                    if( !previous_lua_port.empty() )
                                        network_manager.stopFileTransfer( previous_lua_port );
                                    previous_lua_port = network_manager.getFile( 1, ReplaceChar( script_name, '\\', '/') );
                                    network_manager.sendSpecial( format( "REQUEST FILE %s %s", ReplaceChar(script_name, ' ', 1 ).c_str(), previous_lua_port.c_str() ) );
                                }
                            }
                        }
                    }
                }
                else if( params.size() == 4 ) {
                    if( params[0] == "REQUEST" ) {					// REQUEST FILE filename port
                        if( params[1] == "FILE" ) {
                            String file_name = ReplaceChar( params[2], 1, ' ' );
                            network_manager.stopFileTransfer( params[3], from );
                            network_manager.sendFile( from, file_name, params[3] );
                        }
                    }
                }
                else if( params.size() == 9 ) {
                    if( params[0] == "PLAYER_INFO" ) {							// We've received player information, let's update :)
                        int i = atoi( params[1].c_str() );
                        int n_id = atoi( params[2].c_str() );
                        if( i >= 0 && i < 10 && (client || from == n_id ) ) {		// Server doesn't accept someone telling him what to do
                            int side_id = atoi( params[3].c_str() );
                            int ai_level = atoi( params[4].c_str() );
                            int metal_q = atoi( params[5].c_str() );
                            int energy_q = atoi( params[6].c_str() );
                            bool ready = atoi( params[8].c_str() );
                            game_data.player_network_id[i] = n_id;
                            game_data.player_sides[i] = side_str[ side_id ];
                            game_data.ai_level[i] = ai_level >= 0 ? ai_level : 0;
                            game_data.metal[i] = metal_q;
                            game_data.energy[i] = energy_q;
                            game_data.player_names[i] = ReplaceChar( params[7], 1, ' ' );
                            game_data.ready[i] = ready;
                            if( n_id < 0 && ai_level >= 0 )
                                game_data.player_control[i] = PLAYER_CONTROL_REMOTE_AI;		// AIs are on the server, no need to replicate them
                            else if( n_id < 0 && ai_level < 0 )
                                game_data.player_control[i] = PLAYER_CONTROL_NONE;
                            else
                                game_data.player_control[i] = (n_id == my_player_id) ? PLAYER_CONTROL_LOCAL_HUMAN : PLAYER_CONTROL_REMOTE_HUMAN;

                            setupgame_area.set_caption( format( "gamesetup.name%d", i ),game_data.player_names[i]);									// Update gui
                            setupgame_area.set_caption( format( "gamesetup.ai%d", i ), (game_data.player_control[i] & PLAYER_CONTROL_FLAG_AI) ? ai_level_str[game_data.ai_level[i]] : String("") );
                            setupgame_area.set_caption( format("gamesetup.side%d", i) , side_str[side_id] );							// Update gui
                            setupgame_area.set_caption( format("gamesetup.energy%d", i), format("%d",game_data.energy[i]) );			// Update gui
                            setupgame_area.set_caption( format("gamesetup.metal%d", i), format("%d",game_data.metal[i]) );				// Update gui
                            setupgame_area.set_state( format("gamesetup.ready%d", i), ready );											// Update gui

                            GUIOBJ *guiobj =  setupgame_area.get_object( format("gamesetup.color%d", i) );
                            if( guiobj ) {
                                guiobj->Data = gfx->makeintcol(player_color[player_color_map[i]*3],player_color[player_color_map[i]*3+1],player_color[player_color_map[i]*3+2]);			// Update gui
                                if( game_data.player_control[i] == PLAYER_CONTROL_NONE || game_data.player_control[i] == PLAYER_CONTROL_CLOSED )
                                    guiobj->Flag |= FLAG_HIDDEN;
                                else
                                    guiobj->Flag &= ~FLAG_HIDDEN;
                            }
                            if( !client )
                                network_manager.sendSpecial("NOTIFY UPDATE", from);
                        }
                        else
                            Console->AddEntry("packet error : %s", received_special_msg.message);
                    }
                }
                else if( params.size() == 11 ) {
                    if( params[0] == "PLAYERCOLORMAP" ) {
                        for( int i = 0 ; i < 10 ; i++ ) {
                            player_color_map[i] = atoi( params[i+1].c_str() );
                            GUIOBJ *guiobj =  setupgame_area.get_object( format("gamesetup.color%d", i) );
                            if( guiobj )
                                guiobj->Data = gfx->makeintcol(player_color[player_color_map[i]*3],player_color[player_color_map[i]*3+1],player_color[player_color_map[i]*3+2]);			// Update gui
                        }
                    }
                }

            if( network_manager.getNextSpecial( &received_special_msg ) == 0 )
                special_msg = received_special_msg.message;
            else
                special_msg = "";
        }

        if( set_map.empty() && network_manager.isTransferFinished( previous_ota_port ) && network_manager.isTransferFinished( previous_tnt_port )
            && (!previous_tnt_port.empty() || !previous_ota_port.empty()) ) {
            set_map = game_data.map_filename;
            previous_ota_port = "";
            previous_tnt_port = "";
        }

        //-------------------------------------------------------------- Network Code : chat system --------------------------------------------------------------

        while( !chat_msg.empty() ) {													// Chat receiver
            GUIOBJ *chat_list = setupgame_area.get_object("gamesetup.chat_list");

            if( chat_list ) {
                chat_list->Text.push_back( chat_msg );
                if( chat_list->Text.size() > 5 )
                    chat_list->Data++;
                chat_list->Pos = chat_list->Text.size() - 1;
            }

            if( network_manager.getNextChat( &received_chat_msg ) == 0 )
                chat_msg = received_chat_msg.message;
            else
                chat_msg = "";
        }

        //-------------------------------------------------------------- Network Code : advert system --------------------------------------------------------------

        while( !broadcast_msg.empty() )	// Broadcast message receiver
        {
            Vector<String> params;
            ReadVectorString(params, broadcast_msg, " ");
            if( params.size() == 3 && params[0] == "PING" && params[1] == "SERVER" )
            {
                if( params[2] == "LIST" && host ) // Sending information about this server
                {
                    uint16 nb_open = 0;
                    for( uint16 f = 0 ; f < 10 ; f++ )
                        if( setupgame_area.get_caption(format("gamesetup.name%d", f)) == player_str[2] ) 
                            nb_open++;
                    if( TA3D_CURRENT_MOD.empty() )
                        network_manager.broadcastMessage( format( "PONG SERVER %s . %s %d", ReplaceChar( host, ' ', 1).c_str(), ReplaceChar( TA3D_ENGINE_VERSION,' ', 1 ).c_str(), nb_open ).c_str() );
                    else
                        network_manager.broadcastMessage( format( "PONG SERVER %s %s %s %d", ReplaceChar( host, ' ', 1).c_str(), ReplaceChar( TA3D_CURRENT_MOD, ' ', 1 ).c_str(), ReplaceChar( TA3D_ENGINE_VERSION,' ',1 ).c_str(), nb_open ).c_str() );
                }
            }
            broadcast_msg = network_manager.getNextBroadcastedMessage();
        }

        //-------------------------------------------------------------- End of Network Code --------------------------------------------------------------

        amx = mouse_x;
        amy = mouse_y;
        amz = mouse_z;
        amb = mouse_b;

        if( key[KEY_ENTER] && !setupgame_area.get_caption("gamesetup.t_chat").empty() ) {
            String message = "<" + lp_CONFIG->player_name + "> " + setupgame_area.get_caption("gamesetup.t_chat");
            if( host ) {
                struct chat msg;
                network_manager.sendChat( strtochat( &msg, message ) );
            }
            GUIOBJ *chat_list = setupgame_area.get_object("gamesetup.chat_list");

            if( chat_list ) {
                chat_list->Text.push_back( message );
                if( chat_list->Text.size() > 5 )
                    chat_list->Data++;
                chat_list->Pos = chat_list->Text.size() - 1;
            }

            setupgame_area.set_caption("gamesetup.t_chat", "");
        }

        if( setupgame_area.get_value( "gamesetup.FOW" ) >= 0 && !client ) {
            GUIOBJ *obj = setupgame_area.get_object( "gamesetup.FOW" );
            if( obj && obj->Value != -1 ) {
                obj->Text[0] = obj->Text[ 1 + obj->Value ];
                game_data.fog_of_war = obj->Value;
                if( host )	network_manager.sendSpecial( format( "SET FOW %d", obj->Value ) );
            }
        }

        if( client )
            setupgame_area.msg("scripts.hide");	// Hide the scripts window in client mode

        if( setupgame_area.get_state( "scripts.b_ok" ) && !client )
        {
            guiobj = setupgame_area.get_object( "scripts.script_list" );
            if( guiobj && guiobj->Pos >= 0 && guiobj->Pos < guiobj->num_entries() ) {
                setupgame_area.set_caption( "gamesetup.script_name", guiobj->Text[ guiobj->Pos ] );
                free( game_data.game_script );
                game_data.game_script = strdup( guiobj->Text[ guiobj->Pos ].c_str() );
                if( host )	network_manager.sendSpecial( "SET SCRIPT " + ReplaceChar( guiobj->Text[ guiobj->Pos ], ' ', 1 ) );
            }
        }

        if( setupgame_area.get_state( "gamesetup.b_ok" ) && !client )
        {
            bool ready = true;
            if( host )
                for( int i = 0 ; i < 10 ; i++ )
                    if( game_data.player_control[i] == PLAYER_CONTROL_LOCAL_HUMAN || game_data.player_control[i] == PLAYER_CONTROL_REMOTE_HUMAN )
                        ready &= game_data.ready[i];

            if (ready)
            {
                while( key[KEY_ENTER] )	{	rest( 20 );	poll_keyboard();	}
                clear_keybuf();
                done=true;		// If user click "OK" or hit enter then leave the window
                start_game = true;
                network_manager.sendSpecial( "NOTIFY START" );
            }
        }
        if( setupgame_area.get_state("gamesetup.b_cancel"))
            done=true;		// En cas de click sur "retour", on quitte la fenêtre

        for(uint16 i = 0 ; i < 10 ; ++i)
        {
            if( setupgame_area.get_state( format("gamesetup.ready%d", i ) ) != game_data.ready[i] ) {
                if( game_data.player_control[i] == PLAYER_CONTROL_LOCAL_HUMAN ) {
                    network_manager.sendSpecial( "NOTIFY UPDATE" );
                    game_data.ready[i] = !game_data.ready[i];
                }
                else
                    setupgame_area.set_state( format("gamesetup.ready%d", i ), game_data.ready[i] );
            }
            if(client && game_data.player_network_id[i] != my_player_id )
                continue;							// You cannot change other player's settings
            if( setupgame_area.get_state( format("gamesetup.b_name%d", i) ) && !client ) // Change player type
            {
                if( game_data.player_network_id[i] >= 0 && game_data.player_network_id[i] != my_player_id ) // Kick player !!
                {
                    network_manager.dropPlayer( game_data.player_network_id[i] );
                    network_manager.sendSpecial( "NOTIFY UPDATE" );
                }
                uint16 e = 0;
                for( uint16 f = 0 ; f<player_str_n ; f++ )
                    if( setupgame_area.get_caption(format("gamesetup.name%d", i)) == player_str[f].c_str() ) {	e = f;	break;	}
                e = (e+1) % player_str_n;

                if (player_control[e] == PLAYER_CONTROL_LOCAL_HUMAN)// We can only have one local human player ( or it crashes )
                {
                    for( uint16 f = 0 ; f<10 ; f++ )
                    {
                        if( f!= i && game_data.player_control[f] == PLAYER_CONTROL_LOCAL_HUMAN ) // If we already have a local human player pass this player type value
                        {
                            e = (e+1) % player_str_n;
                            break;
                        }
                    }
                }

                game_data.player_names[i] = player_str[e];								// Update game data
                game_data.player_control[i] = player_control[e];
                if( player_control[e] == PLAYER_CONTROL_LOCAL_HUMAN )
                    game_data.player_network_id[i] = my_player_id;
                else
                    game_data.player_network_id[i] = -1;

                setupgame_area.set_caption( format( "gamesetup.name%d", i ),player_str[e]);			// Update gui
                setupgame_area.set_caption( format( "gamesetup.ai%d", i ), (game_data.player_control[i] & PLAYER_CONTROL_FLAG_AI) ? ai_level_str[game_data.ai_level[i]] : String("") );
                guiobj = setupgame_area.get_object( format( "gamesetup.color%d", i ) );
                if( guiobj ) {
                    if( player_control[e] == PLAYER_CONTROL_NONE || player_control[e] == PLAYER_CONTROL_CLOSED )
                        guiobj->Flag |= FLAG_HIDDEN;
                    else
                        guiobj->Flag &= ~FLAG_HIDDEN;
                }
                if( host )	network_manager.sendSpecial( "NOTIFY UPDATE" );
            }
            if( setupgame_area.get_state( format("gamesetup.b_side%d", i) ) ) {	// Change player side
                uint16 e = 0;
                for( uint16 f = 0 ; f<side_str_n ; f++ )
                    if( setupgame_area.get_caption( format("gamesetup.side%d", i) ) == side_str[f].c_str() ) {	e = f;	break;	}
                e = (e+1) % side_str_n;
                setupgame_area.set_caption( format("gamesetup.side%d", i) , side_str[e] );			// Update gui

                game_data.player_sides[i] = side_str[e];								// update game data
                if( host )	network_manager.sendSpecial( "NOTIFY UPDATE" );
            }
            if (setupgame_area.get_state( format("gamesetup.b_ai%d", i) ) ) // Change player level (for AI)
            {
                uint16 e = 0;
                for( uint16 f = 0 ; f<ai_level_str_n ; f++ )
                    if( setupgame_area.get_caption( format("gamesetup.ai%d", i) ) == ai_level_str[f].c_str() ) {	e = f;	break;	}
                e = (e+1) % ai_level_str_n;
                setupgame_area.set_caption( format("gamesetup.ai%d", i), game_data.player_control[i] & PLAYER_CONTROL_FLAG_AI ? ai_level_str[e] : String("") );			// Update gui

                game_data.ai_level[i] = e;								// update game data
                if (host)
                    network_manager.sendSpecial("NOTIFY UPDATE");
            }
            if(setupgame_area.get_state( format("gamesetup.b_color%d", i)))	// Change player color
            {
                if (client)
                    network_manager.sendSpecial(format("NOTIFY COLORCHANGE %d", i));
                sint16 e = player_color_map[i];
                sint16 f = -1;
                for( sint16 g = 0; g<10 ; g++ )						// Look for the next color
                {
                    if( (game_data.player_control[g] == PLAYER_CONTROL_NONE || game_data.player_control[g] == PLAYER_CONTROL_CLOSED)
                        && player_color_map[g] > e && (f == -1 || player_color_map[g] < player_color_map[f]) )
                    {
                        f = g;
                    }
                }
                if( f == -1 )
                    for( uint16 g = 0; g<10 ; g++ )
                        if( (game_data.player_control[g] == PLAYER_CONTROL_NONE || game_data.player_control[g] == PLAYER_CONTROL_CLOSED) && (f == -1 || player_color_map[g] < player_color_map[f]) )
                            f = g;
                if( f != -1 )
                {
                    sint16 g = player_color_map[f];
                    player_color_map[i] = g;								// update game data
                    player_color_map[f] = e;

                    guiobj =  setupgame_area.get_object( format("gamesetup.color%d", i) );
                    if( guiobj )
                        guiobj->Data = gfx->makeintcol(player_color[player_color_map[i]*3],player_color[player_color_map[i]*3+1],player_color[player_color_map[i]*3+2]);			// Update gui
                    guiobj =  setupgame_area.get_object( format("gamesetup.color%d", f) );
                    if( guiobj )
                        guiobj->Data = gfx->makeintcol(player_color[player_color_map[f]*3],player_color[player_color_map[f]*3+1],player_color[player_color_map[f]*3+2]);			// Update gui
                }
                if(host && !client)
                    network_manager.sendSpecial( "NOTIFY UPDATE" );
            }
            if( setupgame_area.get_state( format("gamesetup.b_energy%d", i) ) ) // Change player energy stock
            {
                game_data.energy[i] = (game_data.energy[i] + 500) % 10500;
                if( game_data.energy[i] == 0 ) game_data.energy[i] = 500;

                setupgame_area.set_caption( format("gamesetup.energy%d", i), format("%d",game_data.energy[i]) );			// Update gui
                if( host )	network_manager.sendSpecial( "NOTIFY UPDATE" );
            }
            if( setupgame_area.get_state( format("gamesetup.b_metal%d", i) ) ) // Change player metal stock
            {
                game_data.metal[i] = (game_data.metal[i] + 500) % 10500;
                if( game_data.metal[i] == 0 ) game_data.metal[i] = 500;

                setupgame_area.set_caption( format("gamesetup.metal%d", i), format("%d",game_data.metal[i]) );			// Update gui
                if( host )	network_manager.sendSpecial( "NOTIFY UPDATE" );
            }
        }

        if( minimap_obj != NULL &&
            ( ( setupgame_area.get_state( "gamesetup.minimap" ) || setupgame_area.get_state("gamesetup.change_map")) && !client)
            || ( client && !set_map.empty() ) )	// Clic on the mini-map or received map set command
        {
            String map_filename;
            char *new_map;
            if( !client )
            {
                map_filename = game_data.map_filename;
                new_map = select_map( &map_filename );
                for( int i = 0 ; i < 10 ; i++ )
                    player_timer[ i ] = msec_timer;
            }
            else
                new_map = strdup( set_map.c_str() );

            gfx->SCREEN_W_TO_640 = 1.0f;				// To have mouse sensibility undependent from the resolution
            gfx->SCREEN_H_TO_480 = 1.0f;
            cursor_type=CURSOR_DEFAULT;
            gfx->set_2D_mode();

            if(new_map)
            {
                if( host && !client)
                    network_manager.sendSpecial(format("SET MAP %s", ReplaceChar( new_map, ' ', 1 ).c_str()));

                String new_map_name = new_map;

                gfx->destroy_texture( glimg );

                if(game_data.map_filename)
                    free(game_data.map_filename);
                game_data.map_filename = new_map;
                glimg = load_tnt_minimap_fast(game_data.map_filename,&dx,&dy);
                ldx = dx * ( mini_map_x2 - mini_map_x1 ) / 504.0f;
                    ldy = dy * ( mini_map_y2 - mini_map_y1 ) / 504.0f;
                    minimap_obj->x1 = mini_map_x-ldx;
                minimap_obj->y1 = mini_map_y-ldy;
                minimap_obj->x2 = mini_map_x+ldx;
                minimap_obj->y2 = mini_map_y+ldy;
                minimap_obj->u2 = dx/252.0f;
                minimap_obj->v2 = dy/252.0f;

                map_data.destroy();
                map_data.load( replace_extension( (char*)tmp_char, game_data.map_filename, "ota", 1024 ) );
                String map_info = "";
                if(map_data.missionname)
                    map_info += String( map_data.missionname ) + "\n";
                if(map_data.numplayers)
                    map_info += String( map_data.numplayers ) + "\n";
                if(map_data.missiondescription)
                    map_info += map_data.missiondescription;
                setupgame_area.set_caption("gamesetup.map_info", map_info );

                if( client && !HPIManager->Exists( new_map_name.c_str() ) ) {
                    if( !previous_tnt_port.empty() )
                        network_manager.stopFileTransfer( previous_tnt_port );
                    previous_tnt_port = network_manager.getFile( 1, ReplaceChar( new_map_name, '\\', '/') );
                    network_manager.sendSpecial( format( "REQUEST FILE %s %s", ReplaceChar(new_map_name, ' ', 1 ).c_str(), previous_tnt_port.c_str() ) );
                }

                new_map_name = new_map_name.substr( 0, new_map_name.size() - 3 ) + "ota";

                if( client && !HPIManager->Exists( new_map_name.c_str()))
                {
                    if( !previous_ota_port.empty())
                        network_manager.stopFileTransfer(previous_ota_port);
                    previous_ota_port = network_manager.getFile( 1, ReplaceChar( new_map_name, '\\', '/') );
                    network_manager.sendSpecial( format( "REQUEST FILE %s %s", ReplaceChar(new_map_name, ' ', 1 ).c_str(), previous_ota_port.c_str()));
                }
            }

            minimap_obj->Data = glimg;		// Synchronize the picture on GUI
        }
        set_map = "";

        if(key[KEY_ESC]) done=true;			// Quitte si on appuie sur echap

        setupgame_area.draw();

        glEnable(GL_TEXTURE_2D);
        gfx->set_color(0xFFFFFFFF);
        draw_cursor();

        // Affiche
        gfx->flip();

    } while(!done);

    if( setupgame_area.background == gfx->glfond )	setupgame_area.background = 0;
    setupgame_area.destroy();

    map_data.destroy();

    gfx->destroy_texture(glimg);

    reset_mouse();
    while (key[KEY_ESC])
    {
        rest(1);
        poll_keyboard();
    }

    if( network_manager.isServer() )
        network_manager.registerToNetServer( host, 0 );				// Tell the world we're gone

    if(start_game)
    {
        if(game_data.map_filename && game_data.game_script)
        {
            lp_CONFIG->last_script = game_data.game_script;		// Remember the last script we played
            lp_CONFIG->last_map = game_data.map_filename;		// Remember the last map we played
            lp_CONFIG->last_FOW = game_data.fog_of_war;

            game_data.nb_players = 0;
            for(uint16 i = 0 ; i<10 ; i++)		// Move players to the top of the vector, so it's easier to access data
            {
                if( game_data.player_control[i] != PLAYER_CONTROL_NONE && game_data.player_control[i] != PLAYER_CONTROL_CLOSED )
                {
                    game_data.player_control[game_data.nb_players] = game_data.player_control[i];
                    game_data.player_names[game_data.nb_players] = game_data.player_names[i];
                    game_data.player_sides[game_data.nb_players] = game_data.player_sides[i];
                    game_data.ai_level[game_data.nb_players] = game_data.ai_level[i];
                    game_data.energy[game_data.nb_players] = game_data.energy[i];
                    game_data.metal[game_data.nb_players] = game_data.metal[i];
                    uint16 e = player_color_map[game_data.nb_players];
                    player_color_map[game_data.nb_players] = player_color_map[i];
                    player_color_map[i] = e;
                    game_data.nb_players++;
                }
            }

            gfx->unset_2D_mode();
            GuardStart( play );
            play(&game_data);
            GuardCatch();
            if( IsExceptionInProgress() ) // if guard threw an error this will be true.
            {
                GuardDisplayAndLogError();   // record and display the error.
                exit(1);                      // we outa here.
            }
            gfx->set_2D_mode();
            gfx->ReInitTexSys();
        }

        while (key[KEY_ESC])
        {
            rest(1);
            poll_keyboard();
        }
    }
    else
    {
        if( client )
            network_manager.sendSpecial( "NOTIFY PLAYER_LEFT" );
    }
    network_manager.Disconnect();
}



/*-----------------------------------------------------------------------------\
  |                            void network_room(void)                           |
  |                                                                              |
  |    Displays the list of available servers and allow to join/host a game      |
  \-----------------------------------------------------------------------------*/

#define SERVER_LIST_REFRESH_DELAY	5000

void network_room(void)				// Let players create/join a game
{
    set_uformat(U_UTF8);

    network_manager.InitBroadcast("1234");		// broadcast mode

    int server_list_timer = msec_timer - SERVER_LIST_REFRESH_DELAY;
    int internet_server_list_timer = msec_timer - INTERNET_AD_COUNTDOWN;

    List< SERVER_DATA >	servers;					// the server list

    AREA networkgame_area("network game area");
    networkgame_area.load_tdf("gui/networkgame.area");
    if( !networkgame_area.background )	networkgame_area.background = gfx->glfond;

    String sel_index = "";
    String o_sel = "";

    bool done=false;

    int amx = -1;
    int amy = -1;
    int amz = -1;
    int amb = -1;

    String join_host = "";

    do
    {
        bool key_is_pressed = false;
        do {
            key_is_pressed = keypressed() || msec_timer - server_list_timer >= SERVER_LIST_REFRESH_DELAY || msec_timer - internet_server_list_timer >= INTERNET_AD_COUNTDOWN;
            networkgame_area.check();
            rest( 1 );
        } while( amx == mouse_x && amy == mouse_y && amz == mouse_z && amb == mouse_b && mouse_b == 0 && !key[ KEY_ENTER ]
                 && !key[ KEY_ESC ] && !done && !key_is_pressed && !networkgame_area.scrolling && !network_manager.BroadcastedMessages() );

        amx = mouse_x;
        amy = mouse_y;
        amz = mouse_z;
        amb = mouse_b;

        if( network_manager.BroadcastedMessages() ) {
            String msg = network_manager.getNextBroadcastedMessage();
            while( !msg.empty() ) {
                Vector<String> params;
                ReadVectorString(params, msg, " ");
                if( params.size() == 6 && params[0] == "PONG" && params[1] == "SERVER" ) // It looks like "PONG SERVER <name> <mod> <version> <nb open player slots>
                {
                    String name = ReplaceChar( params[2], 1, ' ' );
                    String mod = ReplaceChar( params[3], 1, ' ' );
                    if( mod == "." )	mod = "";
                    String version = ReplaceChar( params[4], 1, ' ' );
                    String host_address = network_manager.getLastMessageAddress();
                    int nb_open = atoi( params[5].c_str() );

                    if( version == TA3D_ENGINE_VERSION && mod == TA3D_CURRENT_MOD && nb_open != 0 )
                    {
                        bool updated = false;
                        for( List< SERVER_DATA >::iterator server_i = servers.begin() ; server_i != servers.end() ; server_i++ )		// Update the list
                        {
                            if( server_i->name == name )
                            {
                                updated = true;
                                server_i->timer = msec_timer;
                                server_i->nb_open = nb_open;
                                server_i->host = host_address;
                                if( name == o_sel )	o_sel += "_";
                                break;
                            }
                        }
                        if( !updated )
                        {
                            SERVER_DATA new_server;
                            new_server.internet = false;
                            new_server.name = name;
                            new_server.timer = msec_timer;
                            new_server.nb_open = nb_open;
                            new_server.host = host_address;
                            servers.push_back( new_server );
                        }
                    }
                }

                msg = network_manager.getNextBroadcastedMessage();
            }

            for( List< SERVER_DATA >::iterator server_i = servers.begin() ; server_i != servers.end() ; )		// Remove those who timeout
                if( !server_i->internet && msec_timer - server_i->timer >= 30000 )
                    servers.erase( server_i++ );
                else
                    server_i++;

            GUIOBJ *obj = networkgame_area.get_object("networkgame.server_list");
            if( obj )
            {
                obj->Text.resize( servers.size() );
                List<String> server_names;
                for( List< SERVER_DATA >::iterator server_i = servers.begin() ; server_i != servers.end() ; server_i++ )		// Remove those who timeout
                    server_names.push_back( server_i->name );
                server_names.sort();
                int i = 0;
                for( List< String >::iterator server_i = server_names.begin() ; server_i != server_names.end() ; server_i++, i++ )		// Remove those who timeout
                    obj->Text[i] = *server_i;
                if( obj->Text.size() == 0 )
                    obj->Text.push_back(TRANSLATE("No server found"));
            }
        }

        bool refresh_all = false;
        if( networkgame_area.get_state("networkgame.b_refresh") ) {
            refresh_all = true;
            servers.clear();
        }

        if( msec_timer - internet_server_list_timer >= INTERNET_AD_COUNTDOWN || refresh_all ) // Refresh server list
        {
            internet_server_list_timer = msec_timer;
            network_manager.listNetGames( servers );

            GUIOBJ *obj = networkgame_area.get_object("networkgame.server_list");
            if( obj ) {
                obj->Text.resize( servers.size() );
                List<String> server_names;
                for( List< SERVER_DATA >::iterator server_i = servers.begin() ; server_i != servers.end() ; server_i++ )		// Remove those who timeout
                    server_names.push_back( server_i->name );
                server_names.sort();
                int i = 0;
                for( List< String >::iterator server_i = server_names.begin() ; server_i != server_names.end() ; server_i++, i++ )		// Remove those who timeout
                    obj->Text[i] = *server_i;
                if( obj->Text.size() == 0 )
                    obj->Text.push_back(TRANSLATE("No server found"));
            }
        }

        if( msec_timer - server_list_timer >= SERVER_LIST_REFRESH_DELAY || refresh_all ) {		// Refresh server list
            for( List< SERVER_DATA >::iterator server_i = servers.begin() ; server_i != servers.end() ; )		// Remove those who timeout
                if( !server_i->internet && msec_timer - server_i->timer >= 30000 )
                    servers.erase( server_i++ );
                else
                    server_i++;

            GUIOBJ *obj = networkgame_area.get_object("networkgame.server_list");
            if( obj ) {
                obj->Text.resize( servers.size() );
                List<String> server_names;
                for( List< SERVER_DATA >::iterator server_i = servers.begin() ; server_i != servers.end() ; server_i++ )		// Remove those who timeout
                    server_names.push_back( server_i->name );
                server_names.sort();
                int i = 0;
                for( List< String >::iterator server_i = server_names.begin() ; server_i != server_names.end() ; server_i++, i++ )		// Remove those who timeout
                    obj->Text[i] = *server_i;
                if( obj->Text.size() == 0 )
                    obj->Text.push_back(TRANSLATE("No server found"));
            }

            server_list_timer = msec_timer;
            if( network_manager.broadcastMessage( "PING SERVER LIST" ) )
                printf("error : could not broadcast packet to refresh server list!!\n");
        }

        if( networkgame_area.get_state( "hosting.b_ok" ) || ( key[KEY_ENTER] && networkgame_area.get_state( "hosting" ) ) ) {
            while( key[KEY_ENTER] )	{	rest( 20 );	poll_keyboard();	}
            clear_keybuf();
            network_manager.Disconnect();
            String host = networkgame_area.get_caption( "hosting.t_hostname" );

            setup_game( false, host.c_str() );	// Host a game

            done = true;
        }

        if( networkgame_area.get_state( "joinip.b_ok" ) ) {
            join_host = networkgame_area.get_caption( "joinip.t_hostname" );
            done = true;
        }

        if( networkgame_area.get_state( "networkgame.b_join" ) ) {
            while( key[KEY_ENTER] )	{	rest( 20 );	poll_keyboard();	}
            clear_keybuf();
            done=true;		// If user click "OK" or hit enter then leave the window
            List< SERVER_DATA >::iterator i_server = servers.begin();
            while( i_server != servers.end() && i_server->name != sel_index )	i_server++;

            if( i_server != servers.end() )		// Server not found !!
                join_host = i_server->host;
            else
                join_host.clear();
        }

        if( networkgame_area.get_state( "networkgame.b_cancel" ) || key[KEY_ESC] ) {
            while( key[KEY_ESC] )	{	rest( 20 );	poll_keyboard();	}
            clear_keybuf();
            done=true;		// If user click "Cancel" or hit ESC then leave the screen returning NULL
            sel_index = "";
        }

        if( networkgame_area.get_object("networkgame.server_list") && !done ) {
            GUIOBJ *obj = networkgame_area.get_object("networkgame.server_list");
            sel_index = obj->Pos >= 0 && obj->Pos < obj->Text.size() ? obj->Text[ obj->Pos ]: "";

            if( sel_index != o_sel ) {			// Update displayed server info
                o_sel = sel_index;
                List< SERVER_DATA >::iterator i_server = servers.begin();
                while( i_server != servers.end() && i_server->name != sel_index )	i_server++;

                if( i_server != servers.end() ) {
                    networkgame_area.set_caption("networkgame.server_name", i_server->name );
                    networkgame_area.set_caption("networkgame.host", i_server->host );
                    networkgame_area.set_caption("networkgame.open_slots", format( "%d", i_server->nb_open ) );
                }
            }
        }

        networkgame_area.draw();

        glEnable(GL_TEXTURE_2D);
        gfx->set_color(0xFFFFFFFF);
        draw_cursor();

        // Affiche
        gfx->flip();
    }while(!done);

    if( networkgame_area.background == gfx->glfond )	networkgame_area.background = 0;
    networkgame_area.destroy();

    reset_mouse();
    while(key[KEY_ESC]) {	rest(1);	poll_keyboard();	}

    network_manager.Disconnect();

    if( !join_host.empty() ) {			// Join a game
        setup_game( true, join_host.c_str() );
        gfx->set_2D_mode();
        gfx->ReInitTexSys();
    }
}

void campaign_main_menu(void)
{
    cursor_type=CURSOR_DEFAULT;

    float resize_w = SCREEN_W / 640.0f;
    float resize_h = SCREEN_H / 480.0f;

    AREA campaign_area("campaign");
    campaign_area.load_tdf("gui/campaign.area");
    if( !campaign_area.background )	campaign_area.background = gfx->glfond;

    std::list< String > campaign_list;
    HPIManager->GetFilelist("camps\\*.tdf",&campaign_list);
    for(std::list< String >::iterator i = campaign_list.begin() ; i != campaign_list.end() ; )		// Removes sub directories entries
        if( SearchString( i->substr( 6, i->size() - 6 ), "/", true ) != -1 || SearchString( i->substr( 6, i->size() - 6 ), "\\", true ) != -1 )
            campaign_list.erase( i++ );
        else
            i++;

    if( campaign_area.get_object("campaign.campaign_list") && campaign_list.size() > 0 ) {
        GUIOBJ *guiobj = campaign_area.get_object("campaign.campaign_list");
        guiobj->Text.clear();
        guiobj->Text.resize( campaign_list.size() );
        int n = 0;
        for(std::list< String >::iterator i = campaign_list.begin() ; i != campaign_list.end() ; i++, n++ )
            guiobj->Text[n] = i->substr(6, i->size() - 10 );
    }

    ANIMS side_logos;
    {
        byte *data = HPIManager->PullFromHPI( "anims\\newgame.gaf" );
        side_logos.load_gaf( data, true, NULL );
        if( data )	free( data );
    }

    cTAFileParser	*campaign_parser = NULL;

    bool done=false;

    bool start_game = false;

    int amx = -1;
    int amy = -1;
    int amz = -1;
    int amb = -1;
    int last_campaign_id = -1;
    String	campaign_name = "";
    int mission_id = -1;
    int nb_mission = 0;

    do
    {
        bool key_is_pressed = false;
        do {
            key_is_pressed = keypressed();
            campaign_area.check();
            rest( 1 );
        } while( amx == mouse_x && amy == mouse_y && amz == mouse_z && amb == mouse_b && mouse_b == 0 && !key[ KEY_ENTER ] && !key[ KEY_ESC ] && !done && !key_is_pressed && !campaign_area.scrolling );

        amx = mouse_x;
        amy = mouse_y;
        amz = mouse_z;
        amb = mouse_b;

        if( campaign_area.get_object("campaign.campaign_list") && campaign_list.size() > 0 ) {				// If we don't have campaign data, then load them
            GUIOBJ *guiobj = campaign_area.get_object("campaign.campaign_list");
            if( guiobj->Pos >= 0 && guiobj->Pos < guiobj->Text.size() && last_campaign_id != guiobj->Pos ) {
                if( !campaign_parser )	delete campaign_parser;
                last_campaign_id = guiobj->Pos;
                mission_id = -1;
                campaign_name = "camps\\" + guiobj->Text[ guiobj->Pos ] + ".tdf";
                campaign_parser = new cTAFileParser( campaign_name );

                guiobj = campaign_area.get_object("campaign.mission_list");
                nb_mission = 0;
                if( guiobj ) {
                    guiobj->Text.clear();
                    int i = 0;
                    String current_name = "";
                    while( !(current_name = campaign_parser->PullAsString( format( "MISSION%d.missionname", i ) ) ).empty() ) {
                        guiobj->Text.push_back( current_name );
                        nb_mission++;
                        i++;
                    }
                }

                guiobj = campaign_area.get_object("campaign.logo");
                if( guiobj ) {
                    guiobj->Data = 0;
                    for( int i = 0 ; i < ta3d_sidedata.nb_side ; i++ )
                        if( Lowercase( ta3d_sidedata.side_name[ i ] ) == Lowercase( campaign_parser->PullAsString( "HEADER.campaignside" ) ) ) {
                            if( side_logos.nb_anim > i )
                                guiobj->Data = side_logos.anm[i].glbmp[0];
                            break;
                        }
                }
            }
        }

        if( campaign_area.get_object("campaign.mission_list") ) {
            GUIOBJ *guiobj = campaign_area.get_object("campaign.mission_list");
            if( guiobj->Pos >= 0 && guiobj->Pos < guiobj->Text.size() )
                mission_id = guiobj->Pos;
        }

        if( campaign_area.get_state( "campaign.b_ok" ) || key[KEY_ENTER] ) {
            while( key[KEY_ENTER] )	{	rest( 20 );	poll_keyboard();	}
            clear_keybuf();
            done=true;		// If user click "OK" or hit enter then leave the window
            start_game = true;
        }
        if( campaign_area.get_state( "campaign.b_cancel" ) ) done=true;		// En cas de click sur "retour", on quitte la fenêtre

        if(key[KEY_ESC]) done=true;			// Quitte si on appuie sur echap

        campaign_area.draw();

        glEnable(GL_TEXTURE_2D);
        gfx->set_color(0xFFFFFFFF);
        draw_cursor();

        // Affiche
        gfx->flip();
    }while(!done);

    if( campaign_area.get_object("campaign.logo") )	campaign_area.get_object("campaign.logo")->Data = 0;

    if( campaign_area.background == gfx->glfond )	campaign_area.background = 0;
    campaign_area.destroy();

    if( campaign_parser )	delete	campaign_parser;

    reset_mouse();
    while(key[KEY_ESC]) {	rest(1);	poll_keyboard();	}

    if(start_game) {					// Open the briefing screen and start playing the campaign

        int exit_mode = 0;
        while( mission_id < nb_mission && (exit_mode = brief_screen( campaign_name, mission_id )) >= 0 )
            if( exit_mode == EXIT_VICTORY )	mission_id++;

        while(key[KEY_ESC]) {	rest(1);	poll_keyboard();	}
    }
}

int brief_screen( String campaign_name, int mission_id )
{
    cursor_type=CURSOR_DEFAULT;

    AREA brief_area("brief");
    brief_area.load_tdf("gui/brief.area");
    if( !brief_area.background )	brief_area.background = gfx->glfond;

    cTAFileParser	brief_parser( campaign_name );			// Loads the campaign file

    String map_filename = "maps\\" + brief_parser.PullAsString( format( "MISSION%d.missionfile", mission_id ) );
    cTAFileParser	ota_parser( map_filename );

    String narration_file = "camps\\briefs\\" + ota_parser.PullAsString( "GlobalHeader.narration" ) + ".wav";		// The narration file
    String language_suffix = "";
    switch( LANG )
    {
        case TA3D_LANG_ENGLISH:	language_suffix = "";			break;
        case TA3D_LANG_FRENCH:	language_suffix = "-french";	break;
        case TA3D_LANG_GERMAN:	language_suffix = "-german";	break;
        case TA3D_LANG_SPANISH:	language_suffix = "-spanish";	break;
        case TA3D_LANG_ITALIAN:	language_suffix = "-italian";	break;
    };
    String brief_file = "camps\\briefs" + language_suffix + "\\" + ota_parser.PullAsString( "GlobalHeader.brief" ) + ".txt";				// The brief file

    {
        if( !HPIManager->Exists( brief_file ) )			// try without the .txt
            brief_file = "camps\\briefs" + language_suffix + "\\" + ota_parser.PullAsString( "GlobalHeader.brief" );
        if( !HPIManager->Exists( brief_file ) )			// try without the suffix if we cannot find it
            brief_file = "camps\\briefs\\" + ota_parser.PullAsString( "GlobalHeader.brief" ) + ".txt";
        if( !HPIManager->Exists( brief_file ) )			// try without the suffix if we cannot find it
            brief_file = "camps\\briefs\\" + ota_parser.PullAsString( "GlobalHeader.brief" );
        byte *data = HPIManager->PullFromHPI( brief_file );
        if( data ) {
            String brief_info = (const char*)data;
            brief_area.set_caption( "brief.info", brief_info );
            free(data);
        }
    }

    ANIMS planet_animation;
    {
        String planet_file = Lowercase( ota_parser.PullAsString("GlobalHeader.planet") );

        if( planet_file == "green planet" )				planet_file = "anims\\greenbrief.gaf";
        else if( planet_file == "archipelago" )			planet_file = "anims\\archibrief.gaf";
        else if( planet_file == "desert" )				planet_file = "anims\\desertbrief.gaf";
        else if( planet_file == "lava" )				planet_file = "anims\\lavabrief.gaf";
        else if( planet_file == "wet desert" )			planet_file = "anims\\wdesertbrief.gaf";
        else if( planet_file == "metal" )				planet_file = "anims\\metalbrief.gaf";
        else if( planet_file == "red planet" )			planet_file = "anims\\marsbrief.gaf";
        else if( planet_file == "lunar" )				planet_file = "anims\\lunarbrief.gaf";
        else if( planet_file == "lush" )				planet_file = "anims\\lushbrief.gaf";
        else if( planet_file == "ice" )					planet_file = "anims\\icebrief.gaf";
        else if( planet_file == "slate" )				planet_file = "anims\\slatebrief.gaf";
        else if( planet_file == "water world" )			planet_file = "anims\\waterbrief.gaf";

        byte *data = HPIManager->PullFromHPI( planet_file );
        if( data ) {
            planet_animation.load_gaf( data, true, NULL );
            free( data );
        }
    }

    int schema = 0;

    if( brief_area.get_object( "brief.schema" ) ) {
        GUIOBJ *obj = brief_area.get_object( "brief.schema" );
        obj->Text.clear();
        obj->Text.resize( ota_parser.PullAsInt( "GlobalHeader.SCHEMACOUNT" ) + 1 );
        for( int i = 0 ; i < obj->Text.size() - 1 ; i++ )
            obj->Text[ i + 1 ] = TRANSLATE( ota_parser.PullAsString( format( "GlobalHeader.Schema %d.Type", i ) ) );
        if( obj->Text.size() > 1 )
            obj->Text[ 0 ] = obj->Text[ 1 ];
    }

    sound_manager->PlaySoundFileNow( narration_file );

    bool done=false;

    bool start_game = false;

    int amx = -1;
    int amy = -1;
    int amz = -1;
    int amb = -1;
    float planet_frame = 0.0f;

    int pan_id = 0;
    int rotate_id = 0;
    for( int i = 0 ; i < planet_animation.nb_anim ; i++ )
        if( Lowercase( String( planet_animation.anm[ i ].name ).substr( String( planet_animation.anm[ i ].name ).size() - 3, 3 ) ) == "pan" )
            pan_id = i;
        else if( Lowercase( String( planet_animation.anm[ i ].name ).substr( String( planet_animation.anm[ i ].name ).size() - 6, 6 ) ) == "rotate" )
            rotate_id = i;

    float pan_x1, pan_x2;

    if( brief_area.get_object( "brief.panning0" ) ) {
        pan_x1 = brief_area.get_object( "brief.panning0" )->x1;
        pan_x2 = brief_area.get_object( "brief.panning0" )->x2;
    }

    int time_ref = msec_timer;

    do
    {
        bool key_is_pressed = false;
        do {
            key_is_pressed = keypressed();
            brief_area.check();
            rest( 1 );
        } while( amx == mouse_x && amy == mouse_y && amz == mouse_z && amb == mouse_b && mouse_b == 0 && !key[ KEY_ENTER ]
                 && !key[ KEY_ESC ] && !done && !key_is_pressed && !brief_area.scrolling && (int)planet_frame == (int)((msec_timer - time_ref) * 0.01f) );

        amx = mouse_x;
        amy = mouse_y;
        amz = mouse_z;
        amb = mouse_b;

        planet_frame = (msec_timer - time_ref) * 0.01f;

        if( brief_area.get_value( "brief.schema" ) >= 0 ) {
            GUIOBJ *obj = brief_area.get_object( "brief.schema" );
            if( obj->Value != -1 ) {
                obj->Text[ 0 ] = obj->Text[ obj->Value + 1 ];
                schema = obj->Value;
            }
        }

        if( brief_area.get_state( "brief.info" ) )
            brief_area.get_object( "brief.info" )->Pos++;

        if( brief_area.get_object( "brief.planet" ) ) {
            GUIOBJ *guiobj = brief_area.get_object( "brief.planet" );
            guiobj->Data = planet_animation.anm[rotate_id].glbmp[ ((int)planet_frame) % planet_animation.anm[rotate_id].nb_bmp ];
        }

        float pan_frame = planet_frame / (pan_x2 - pan_x1);

        if( brief_area.get_object( "brief.panning0" ) ) {
            GUIOBJ *guiobj = brief_area.get_object( "brief.panning0" );
            guiobj->Data = planet_animation.anm[pan_id].glbmp[ ((int)pan_frame) % planet_animation.anm[pan_id].nb_bmp ];
            guiobj->x2 = pan_x2 + (pan_x1 - pan_x2) * (pan_frame - ((int)pan_frame));
            guiobj->u1 = (pan_frame - ((int)pan_frame));
        }

        if( brief_area.get_object( "brief.panning1" ) ) {
            GUIOBJ *guiobj = brief_area.get_object( "brief.panning1" );
            guiobj->Data = planet_animation.anm[pan_id].glbmp[ ((int)pan_frame + 1) % planet_animation.anm[pan_id].nb_bmp ];
            guiobj->x1 = pan_x2 + (pan_x1 - pan_x2) * (pan_frame - ((int)pan_frame));
            guiobj->u2 = (pan_frame - ((int)pan_frame));
        }

        if( brief_area.get_state( "brief.b_ok" ) || key[KEY_ENTER] ) {
            while( key[KEY_ENTER] )	{	rest( 20 );	poll_keyboard();	}
            clear_keybuf();
            done=true;		// If user click "OK" or hit enter then leave the window
            start_game = true;
        }
        if( brief_area.get_state( "brief.b_cancel" ) ) done=true;		// En cas de click sur "retour", on quitte la fenêtre

        if(key[KEY_ESC]) done=true;			// Quitte si on appuie sur echap

        brief_area.draw();

        glEnable(GL_TEXTURE_2D);
        gfx->set_color(0xFFFFFFFF);
        draw_cursor();

        // Affiche
        gfx->flip();
    }while(!done);

    if( brief_area.get_object( "brief.planet" ) )	brief_area.get_object( "brief.planet" )->Data = 0;
    if( brief_area.get_object( "brief.panning0" ) )	brief_area.get_object( "brief.panning0" )->Data = 0;
    if( brief_area.get_object( "brief.panning1" ) )	brief_area.get_object( "brief.panning1" )->Data = 0;

    if( brief_area.background == gfx->glfond )	brief_area.background = 0;
    brief_area.destroy();

    sound_manager->StopSoundFileNow();

    reset_mouse();
    while(key[KEY_ESC]) {	rest(1);	poll_keyboard();	}

    if(start_game) {					// Open the briefing screen and start playing the campaign
        GAME_DATA game_data;

        generate_script_from_mission( "scripts/__campaign_script.lua", &ota_parser, schema );	// Generate the script which will be removed later

        game_data.game_script = strdup( "scripts/__campaign_script.lua" );
        game_data.map_filename = strdup( ( map_filename.substr( 0, map_filename.size() - 3 ) + "tnt" ).c_str() );		// Remember the last map we played
        game_data.fog_of_war = FOW_ALL;

        game_data.nb_players = ota_parser.PullAsInt( "GlobalHeader.numplayers", 2 );
        if( game_data.nb_players == 0 )				// Yes it can happen !!
            game_data.nb_players = 2;

        game_data.player_control[ 0 ] = PLAYER_CONTROL_LOCAL_HUMAN;
        game_data.player_names[ 0 ] = brief_parser.PullAsString( "HEADER.campaignside" );
        game_data.player_sides[ 0 ] = brief_parser.PullAsString( "HEADER.campaignside" );
        game_data.energy[ 0 ] = ota_parser.PullAsInt( format( "GlobalHeader.Schema %d.humanenergy", schema ) );
        game_data.metal[ 0 ] = ota_parser.PullAsInt( format( "GlobalHeader.Schema %d.humanmetal", schema ) );

        String schema_type = Lowercase( ota_parser.PullAsString( format( "GlobalHeader.Schema %d.Type", schema ) ) );
        if( schema_type == "easy" )
            game_data.ai_level[ 0 ] = 0;
        else if( schema_type == "medium" )
            game_data.ai_level[ 0 ] = 1;
        else if( schema_type == "hard" )
            game_data.ai_level[ 0 ] = 2;

        player_color_map[ 0 ] = 0;

        for( int i = 1 ; i < game_data.nb_players ; i++ ) {
            game_data.player_control[ i ] = PLAYER_CONTROL_LOCAL_AI;
            game_data.player_names[ i ] = brief_parser.PullAsString( "HEADER.campaignside" );
            game_data.player_sides[ i ] = brief_parser.PullAsString( "HEADER.campaignside" );			// Has no meaning here since we are in campaign mode units are spawned by a script
            game_data.ai_level[ i ] = game_data.ai_level[ 0 ];
            game_data.energy[ i ] = ota_parser.PullAsInt( format( "GlobalHeader.Schema %d.computerenergy", schema ) );
            game_data.metal[ i ] = ota_parser.PullAsInt( format( "GlobalHeader.Schema %d.computermetal", schema ) );

            player_color_map[ i ] = i;
        }

        game_data.campaign = true;
        game_data.use_only = (char*)ota_parser.PullAsString( "GlobalHeader.useonlyunits" ).c_str();
        if( strlen( game_data.use_only ) == 0 )
            game_data.use_only = NULL;
        else
            game_data.use_only = strdup( ("camps\\useonly\\" + String( game_data.use_only )).c_str() );

        gfx->unset_2D_mode();
        int exit_mode = 0;
        GuardStart( play );
        exit_mode = play(&game_data);
        GuardCatch();
        if( IsExceptionInProgress() ) // if guard threw an error this will be true.
        {
            GuardDisplayAndLogError();   // record and display the error.
            exit(1);                      // we outa here.
        }
        gfx->set_2D_mode();
        gfx->ReInitTexSys();

        while(key[KEY_ESC]) {	rest(1);	poll_keyboard();	}

        return exit_mode;
    }
    return -1;
}

void wait_room(void *p_game_data)
{
    GAME_DATA *game_data = (GAME_DATA*) p_game_data;
    if( !network_manager.isConnected() )	return;

    gfx->set_2D_mode();

    gfx->ReInitTexSys();

    cursor_type=CURSOR_DEFAULT;

    AREA wait_area("wait");
    wait_area.load_tdf("gui/wait.area");
    if( !wait_area.background )	wait_area.background = gfx->glfond;

    bool dead_player[10];
    uint32 player_timer[10];

    for( int i = game_data->nb_players ; i < 10 ; i++ ) {
        dead_player[i] = true;
        wait_area.msg(format("wait.name%d.hide",i));
        wait_area.msg(format("wait.progress%d.hide",i));
        wait_area.msg(format("wait.ready%d.hide",i));
    }

    for( int i = 0 ; i < game_data->nb_players ; i++ ) {
        player_timer[i] = msec_timer;
        dead_player[i] = false;
        if( (game_data->player_control[i] & PLAYER_CONTROL_FLAG_AI) || game_data->player_control[i] == PLAYER_CONTROL_LOCAL_HUMAN ) {
            wait_area.set_data( format("wait.progress%d", i), 100 );
            wait_area.set_state( format("wait.ready%d", i), true );
        }
        wait_area.set_caption( format("wait.name%d", i), game_data->player_names[i] );
    }

    bool done=false;

    int amx = -1;
    int amy = -1;
    int amz = -1;
    int amb = -1;

    {									// We can only use units available on all clients
        for( int i = 0 ; i < unit_manager.nb_unit ; i++ )
            if( !unit_manager.unit_type[ i ].not_used )
                network_manager.sendAll( format( "USING %s", unit_manager.unit_type[ i ].Unitname ) );
        network_manager.sendAll("END USING");
    }

    network_manager.sendAll("READY");

    int ping_timer = msec_timer;					// Used to send simple PING requests in order to detect when a connection fails

    do
    {
        bool key_is_pressed = false;
        String special_msg = "";
        struct chat received_special_msg;
        bool playerDropped = false;
        do {
            playerDropped = network_manager.getPlayerDropped();
            if( network_manager.getNextSpecial( &received_special_msg ) == 0 )
                special_msg = received_special_msg.message;
            if( !network_manager.isConnected() ) {					// We're disconnected !!
                done = true;
                break;
            }
            key_is_pressed = keypressed();
            wait_area.check();
            rest( 1 );
        } while( amx == mouse_x && amy == mouse_y && amz == mouse_z && amb == mouse_b && mouse_b == 0 && !key[ KEY_ENTER ] && !key[ KEY_ESC ] && !done
                 && !key_is_pressed && !wait_area.scrolling && special_msg.empty() && !playerDropped
                 && ( msec_timer - ping_timer < 2000 || !network_manager.isServer() ) );

        //-------------------------------------------------------------- Network Code : syncing information --------------------------------------------------------------

        bool check_ready = false;

        if( network_manager.isServer() && msec_timer - ping_timer >= 2000 ) {		// Send a ping request
            network_manager.sendPing();
            ping_timer = msec_timer;
            check_ready = true;

            if( network_manager.isServer() )
                for( int i = 0 ; i < game_data->nb_players ; i++ )			// Ping time out
                    if( game_data->player_network_id[i] > 0 && msec_timer - player_timer[i] > 10000 )
                        network_manager.dropPlayer( game_data->player_network_id[i] );
        }

        if( playerDropped ) {
            for( int i = 0 ; i < game_data->nb_players ; i++ )
                if( game_data->player_network_id[i] > 0 && !network_manager.pollPlayer(game_data->player_network_id[i]) ) {		// A player is disconnected
                    dead_player[i] = true;
                    wait_area.msg(format("wait.name%d.hide",i));
                    wait_area.msg(format("wait.progress%d.hide",i));
                    wait_area.msg(format("wait.ready%d.hide",i));
                }
        }

        while( !special_msg.empty() ) {													// Special receiver (sync config data)
            int from = received_special_msg.from;
            int player_id = game_data->net2id( from );
            Vector<String> params;
            ReadVectorString(params, received_special_msg.message, " ");
            if( params.size() == 1 ) {
                if( params[0] == "PONG" ) {
                    if( player_id >= 0 )
                        player_timer[player_id] = msec_timer;
                }
                else if( params[0] == "NOT_READY" )
                    wait_area.set_state( format( "wait.ready%d", player_id ), false );
                else if( params[0] == "READY" ) {
                    wait_area.set_data( format( "wait.progress%d", player_id ), 100 );
                    wait_area.set_state( format( "wait.ready%d", player_id ), true );
                    check_ready = true;
                }
                else if( params[0] == "START" )
                    done = true;
            }
            else if( params.size() == 2 ) {
                if( params[0] == "LOADING" ) {
                    int percent = min( 100, max( 0, atoi( params[1].c_str() ) ) );
                    wait_area.set_data( format( "wait.progress%d", player_id ), percent );
                }
                else if( params[0] == "USING" ) {									// We can only use units available on all clients, so check the list
                    network_manager.sendAll("NOT_READY");

                    if( unit_manager.get_unit_index( params[1].c_str() ) == -1 )			// Tell it's missing
                        network_manager.sendAll( "MISSING " + params[1] );
                }
                else if( params[0] == "END" && params[1] == "USING" )
                    network_manager.sendAll("READY");
                else if( params[0] == "MISSING" ) {
                    int idx = unit_manager.get_unit_index( params[1].c_str() );
                    if( idx >= 0 )
                        unit_manager.unit_type[ idx ].not_used = true;
                }
            }

            if( network_manager.getNextSpecial( &received_special_msg ) == 0 )
                special_msg = received_special_msg.message;
            else
                special_msg = "";
        }

        if( network_manager.isServer() && check_ready ) {			// If server is late the game should begin once he is there
            bool ready = true;
            for( int i = 0 ; i < game_data->nb_players && ready ; i++ )
                if( !wait_area.get_state( format( "wait.ready%d", i ) ) && !dead_player[i] && game_data->player_network_id[i] > 0 )
                    ready = false;

            if( ready ) {
                network_manager.sendAll( "START" );			// Tell everyone to start the game!!
                rest(1);
                done = true;
            }
        }

        //-------------------------------------------------------------- End of Network Code --------------------------------------------------------------

        amx = mouse_x;
        amy = mouse_y;
        amz = mouse_z;
        amb = mouse_b;

        //		if(key[KEY_ESC]) done=true;			// Quitte si on appuie sur echap

        wait_area.draw();

        glEnable(GL_TEXTURE_2D);
        gfx->set_color(0xFFFFFFFF);
        draw_cursor();

        // Affiche
        gfx->flip();
    }while(!done);

    lp_CONFIG->timefactor = 1.0f;		// We can't afford running a network game at more than 1x !! (at least at the beginning, and not knowing if there are internet players)

    if( wait_area.background == gfx->glfond )	wait_area.background = 0;
    wait_area.destroy();

    gfx->unset_2D_mode();

    reset_mouse();
    while(key[KEY_ESC]) {	rest(1);	poll_keyboard();	}
}

