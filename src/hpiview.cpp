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

#include "stdafx.h"
#include "matrix.h"
#define TA3D_NO_SOUND
#include "TA3D_NameSpace.h"
#include "ta3dbase.h"
#include "jpeg/ta3d_jpg.h"
#include "EngineClass.h"
#include "tnt.h"

using namespace TA3D::UTILS::HPI;

void install_TA_files( String def_path = "" );

int hpiview(int argc,char *argv[])
{
int done = false;

allegro_init();

set_uformat(U_ASCII);		// Juste histoire d'avoir un affichage correct des textes

if(argc>=2) {
	if(strcasecmp(argv[1],"help")==0) {
		printf("available commands:\n");
		printf(" create_gaf		->	create a 24/32bits gaf from sprites\n");
		printf(" extract		->	extract a file\n");
		printf(" extract_gaf		->	extract a gaf into sprites\n");
		printf(" help			->	this screen\n");
		printf(" install		->	install TA files\n");
		printf(" listmods		->	list all available mods\n");
		printf(" mapdescription		->	extract a map description\n");
		printf(" minimap		->	extract a minimap\n");
		printf(" print			->	show the content of a file\n");
		printf(" show			->	show files matching a pattern\n");
		printf("\nfor more information on a command type :\n# %s command_name\n", argv[0]);
		done = true;
		}
	else if(strcasecmp(argv[1],"show")==0) {
		if( argc >= 3 ) {
			HPIManager=new cHPIHandler("");

			List<String> file_list;
			String ext = argc > 2 ? argv[2] : "";
			HPIManager->GetFilelist(ext,&file_list);
			file_list.sort();
			for(List<String>::iterator cur_file=file_list.begin();cur_file!=file_list.end();cur_file++)
				printf("%s\n",cur_file->c_str());

			delete HPIManager;
			}
		else
			printf("SYNTAX: %s show pattern\n", argv[0] );
		done = true;
		}
	else if(strcasecmp(argv[1],"minimap")==0) {		// Extract minimap from a .tnt file
		if( argc >= 4 ) {
			HPIManager=new cHPIHandler("");
			TA3D::VARS::pal = new RGB[256];      // Allocate a new palette

			byte *palette = HPIManager->PullFromHPI( "palettes\\palette.pal" );
			if(palette) {
				for(int i=0;i<256;i++) {
					pal[i].r=palette[i<<2]>>2;
					pal[i].g=palette[(i<<2)+1]>>2;
					pal[i].b=palette[(i<<2)+2]>>2;
					}
				free(palette);
				set_palette(pal);      // Activate the palette
				}

			jpgalleg_init();

			set_color_depth( 32 );
			BITMAP *minimap = load_tnt_minimap_fast_bmp( argv[2] );
			if( minimap )
				save_bitmap( argv[3], minimap, NULL );

			delete[] TA3D::VARS::pal;
			delete HPIManager;
			}
		else
			printf("SYNTAX: %s minimap \"maps\\map.tnt\" minimap_file.jpg\n", argv[0] );
		done = true;
		}
	else if(strcasecmp(argv[1],"mapdescription")==0) {		// Extract map description from a .ota file
		if( argc >= 4 ) {
			HPIManager=new cHPIHandler("");

			MAP_OTA map_data;
			map_data.load( argv[2] );
			
			std::ofstream   m_File;

			m_File.open( argv[3], std::ios::out | std::ios::trunc );

			if( m_File.is_open() ) {
				m_File << "[MAP]\n{\n";
				m_File << " missionname=" << map_data.missionname << ";\n";
				m_File << " missiondescription=" << ReplaceString( map_data.missiondescription, "\n", "\\n", false ) << ";\n";
				m_File << " planet=" << map_data.planet << ";\n";
				m_File << " tidalstrength=" << map_data.tidalstrength << ";\n";
				m_File << " solarstrength=" << map_data.solarstrength << ";\n";
				m_File << " gravity=" << map_data.gravity << ";\n";
				m_File << " numplayers=" << ReplaceString( map_data.numplayers, "\n", "\\n", false ) << ";\n";
				m_File << " size=" << ReplaceString( map_data.map_size, "\n", "\\n", false ) << ";\n";
				m_File << " minwindspeed=" << map_data.minwindspeed << ";\n";
				m_File << " maxwindspeed=" << map_data.maxwindspeed << ";\n";
				m_File << "}\n";
				m_File.close();
				}

			delete HPIManager;
			}
		else
			printf("SYNTAX: %s mapdescription \"maps\\map.ota\" description.tdf\n", argv[0] );
		done = true;
		}
	else if(strcasecmp(argv[1],"listmods")==0) {		// List available mods in
		if( argc >= 3 ) {
			HPIManager=new cHPIHandler("");

			std::ofstream   m_File;

			m_File.open( argv[2], std::ios::out | std::ios::trunc );

			if( m_File.is_open() ) {
				List< String > modlist = GetFileList( "mods/*" );

				modlist.sort();

				foreach( modlist, i )
					if( (*i)[0] != '.' )
						m_File << *i << "\n";
				m_File.close();
				}

			delete HPIManager;
			}
		else
			printf("SYNTAX: %s listmods modlist.txt\n", argv[0] );
		done = true;
		}
	else if(strcasecmp(argv[1],"extract")==0) {
		if( argc >= 3 ) {
			HPIManager=new cHPIHandler("");

			uint32 file_size32 = 0;
			byte *data = HPIManager->PullFromHPI(argv[2],&file_size32);

			if(data) {
				char *name=argv[2];
				char *f=argv[2];
				while(f[0]) {
					if(f[0]=='\\' || f[0]=='/')
						name=f+1;
					f++;
					}
				FILE *dst = TA3D_OpenFile(name,"wb");
				fwrite(data,file_size32,1,dst);
				fclose(dst);
				free(data);
				}

			delete HPIManager;
			}
		else
			printf("SYNTAX: %s extract filename\n", argv[0] );
		done = true;
		}
	else if(strcasecmp(argv[1],"print")==0) {			// like extract but write on the standard output
		if( argc >= 3 ) {
			HPIManager=new cHPIHandler("");

			List<String> file_list;
			String ext = argc > 2 ? argv[2] : "";
			HPIManager->GetFilelist(ext,&file_list);
			file_list.sort();

			for(List<String>::iterator cur_file=file_list.begin();cur_file!=file_list.end();cur_file++) {
				uint32 file_size32 = 0;
				byte *data = HPIManager->PullFromHPI( *cur_file, &file_size32 );

				if(data) {
					printf("%s\n",(char*)data);
					free(data);
					}
				}

			delete HPIManager;
			}
		else
			printf("SYNTAX: %s print pattern\n", argv[0] );
		done = true;
		}
	else if(strcasecmp(argv[1],"install")==0) {

		install_TA_files( argc >= 3 ? argv[2] : "" );

		done = true;
		}
	else if(strcasecmp(argv[1],"extract_gaf")==0 && argc >= 3) {	// Extract images from a GAF file
		if( argc >= 3 ) {
			HPIManager=new cHPIHandler("");

			uint32 file_size32 = 0;
			byte *data = HPIManager->PullFromHPI(argv[2],&file_size32);

			if(data) {
				set_color_depth( 32 );
				set_gfx_mode( GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0 );
				TA3D::VARS::pal=new RGB[256];      // Allocate a new palette

				byte *palette = HPIManager->PullFromHPI( "palettes\\palette.pal" );
				if(palette)
				{
					for(int i=0;i<256;i++)
					{
						pal[i].r=palette[i<<2]>>2;
						pal[i].g=palette[(i<<2)+1]>>2;
						pal[i].b=palette[(i<<2)+2]>>2;
					}
					free(palette);
					set_palette(pal);      // Activate the palette
				}

				ANIMS anims;
				anims.load_gaf( data );

				std::ofstream   m_File;

				m_File.open( format("%s.txt", get_filename( argv[2] )).c_str(), std::ios::out | std::ios::trunc );

				m_File << "[gadget0]\n{\n";
				m_File << "    filename=" << argv[2] << ";\n";
				m_File << "    entries=" << anims.nb_anim << ";\n";
				m_File << "}\n";

				for( int i = 0 ; i < anims.nb_anim ; i++ ) {
					m_File << "[gadget" << (i + 1) << "]\n{\n";
					m_File << "    frames=" << anims.anm[i].nb_bmp << ";\n";
					m_File << "    name=" << anims.anm[i].name << ";\n";
					for( int e = 0 ; e < anims.anm[i].nb_bmp ; e++ ) {
						String filename = format("%s%d.tga", anims.anm[i].name, e );
						m_File << "    [frame" << e << "]\n    {\n";
						m_File << "        XPos=" << anims.anm[i].ofs_x[ e ] << ";\n";
						m_File << "        YPos=" << anims.anm[i].ofs_y[ e ] << ";\n";
						m_File << "        filename=" << filename << ";\n";
						m_File << "    }\n";
						save_bitmap( filename.c_str(), anims.anm[i].bmp[e], NULL );
						}
					m_File << "}\n";
					}
			
				m_File.flush();
				m_File.close();

				anims.destroy();

				free(data);
				delete[] TA3D::VARS::pal;
				}

			delete HPIManager;
			}
		else
			printf("SYNTAX: %s extract_gaf filename.gaf\n", argv[0] );
		done = true;
		}
	else if(strcasecmp(argv[1],"create_gaf")==0 && argc >= 3) {		// Create a truecolor gaf from images
		if( argc >= 3 ) {
			jpgalleg_init();		// We'll need JPG
			cTAFileParser parser( argv[2], false, false, false );

			String filename = parser.PullAsString( "gadget0.filename" );

			FILE *gaf_file = TA3D_OpenFile( get_filename( filename.c_str() ), "wb" );

			if( gaf_file ) {
				set_color_depth( 32 );
				set_gfx_mode( GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0 );
				GAFHEADER	header;
				header.IDVersion = GAF_TRUECOLOR;
				header.Entries = parser.PullAsInt( "gadget0.entries" );
				header.Unknown1 = 0;

				fwrite( &header, 12, 1, gaf_file );

				for( int i = 0 ; i < header.Entries * 4 ; i++ )			// List of pointers
					putc( 0, gaf_file );

				GAFENTRY Entry;

				for( int i = 0 ; i < header.Entries ; i++ ) {
					int pos = ftell( gaf_file );				// Write the pointer to this entry
					fseek( gaf_file, 12 + i * 4, SEEK_SET );
					fwrite( &pos, 4, 1, gaf_file );
					fseek( gaf_file, pos, SEEK_SET );

					Entry.Frames = parser.PullAsInt( format( "gadget%d.frames", i + 1 ) );
					Entry.Unknown1 = 1;
					Entry.Unknown2 = 0;
					String Entry_name = parser.PullAsString( format( "gadget%d.name", i + 1 ) );
					memset( Entry.Name, 0, 32 );
					memcpy( Entry.Name, Entry_name.c_str(), Entry_name.size() + 1 );
				
					fwrite( &Entry, 40, 1, gaf_file );		// Write the entry

					GAFFRAMEENTRY	FrameEntry;
					int FrameEntryPos = ftell( gaf_file );
					FrameEntry.PtrFrameTable = 0;
					for( int e = 0 ; e < Entry.Frames ; e++ )
						fwrite( &(FrameEntry), 8, 1, gaf_file );		// Write the frame entry

					for( int e = 0 ; e < Entry.Frames ; e++ ) {
						pos = ftell( gaf_file );				// Write the pointer to this entry
						fseek( gaf_file, FrameEntryPos + e * 8, SEEK_SET );
						FrameEntry.PtrFrameTable = pos;
						fwrite( &(FrameEntry), 8, 1, gaf_file );		// Write the frame entry
						fseek( gaf_file, pos, SEEK_SET );

						GAFFRAMEDATA FrameData;
						FrameData.XPos = parser.PullAsInt( format( "gadget%d.frame%d.XPos", i + 1, e ) );
						FrameData.YPos = parser.PullAsInt( format( "gadget%d.frame%d.YPos", i + 1, e ) );
						FrameData.FramePointers = 0;
						FrameData.Unknown2 = 0;
						FrameData.Compressed = 1;
					
						BITMAP *frame_img = load_bitmap( parser.PullAsString( format( "gadget%d.frame%d.filename", i + 1, e ) ).c_str(), NULL );
						if( frame_img ) {
							FrameData.Width = frame_img->w;
							FrameData.Height = frame_img->h;
							bool alpha = false;
							for( int y = 0 ; y < frame_img->h && !alpha ; y++ )
								for( int x = 0 ; x < frame_img->w && !alpha ; x++ )
									alpha |= (frame_img->line[y][(x<<2)+3] != 255);
							FrameData.Transparency = alpha ? 1 : 0;
							FrameData.PtrFrameData = ftell( gaf_file ) + 24;
						
							fwrite( &FrameData, 24, 1, gaf_file );

							int buf_size = frame_img->w * frame_img->h * 5 + 10240;
							byte *buffer = new byte[ buf_size ];

							int img_size = buf_size;
							if( save_memory_jpg_ex( buffer, &img_size, frame_img, NULL, 85, JPG_SAMPLING_444, NULL ) ) {		// RGB channels
								img_size = buf_size;
								if( save_memory_jpg_ex( buffer, &img_size, frame_img, NULL, 85, JPG_SAMPLING_444 | JPG_OPTIMIZE, NULL ) )		// RGB channels
									printf("error saving '%s'\n", parser.PullAsString( format( "gadget%d.frame%d.filename", i + 1, e ) ).c_str() );
								}

							fwrite( &img_size, sizeof( img_size ), 1, gaf_file );		// Save the result
							fwrite( buffer, img_size, 1, gaf_file );

							if( alpha ) {																								// Alpha channel
								for( int y = 0 ; y < frame_img->h ; y++ )
									for( int x = 0 ; x < frame_img->w ; x++ ) {
										uint32 c = frame_img->line[y][(x<<2)+3];
										((uint32*)(frame_img->line[y]))[x] = c * 0x010101;
										}

								img_size = buf_size;
								if( save_memory_jpg_ex( buffer, &img_size, frame_img, NULL, 100, JPG_GREYSCALE | JPG_OPTIMIZE, NULL ) ) {
									img_size = buf_size;
									if( save_memory_jpg_ex( buffer, &img_size, frame_img, NULL, 100, JPG_GREYSCALE, NULL ) )
										printf("error saving alpha channel for '%s'\n", parser.PullAsString( format( "gadget%d.frame%d.filename", i + 1, e ) ).c_str() );
									}

								fwrite( &img_size, sizeof( img_size ), 1, gaf_file );		// Save the result
								fwrite( buffer, img_size, 1, gaf_file );
								}

							delete[] buffer;
						
							destroy_bitmap( frame_img );
							}
						else {
							printf("error: in frame %d, could not load %s\n", e, parser.PullAsString( format( "gadget%d.frame%d.filename", i + 1, e ) ).c_str() );
							i = header.Entries;
							break;
							}
						}
					}
				}
			else
				printf("error : could not create file!\n");
			}
		else
			printf("SYNTAX: %s create_gaf gafdescription.txt\n", argv[0] );
		done = true;
		}
	}

allegro_exit();

return done;
}
