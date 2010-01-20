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

/*-----------------------------------------------------------------------------------\
  |                                         fbi.cpp                                    |
  |  Ce fichier contient les structures, classes et fonctions nécessaires à la lecture |
  | des fichiers fbi du jeu totalannihilation qui sont les fichiers de données sur les |
  | unités du jeu. Cela inclus les classes pour gérer les différents types d'unités et |
  | le système de gestion de liens entre unités.                                       |
  |                                                                                    |
  \-----------------------------------------------------------------------------------*/

#include "stdafx.h"
#include <vector>
#include <list>
#include <algorithm>
#include "misc/matrix.h"
#include "TA3D_NameSpace.h"
#include "ta3dbase.h"
#include "mesh/mesh.h"
#include "EngineClass.h"
#include "UnitEngine.h"
#include "languages/i18n.h"
#include "misc/math.h"
#include "logs/logs.h"
#include "ingame/players.h"
#include "misc/tdf.h"
#include "misc/paths.h"
#include "engine/mission.h"
#include "input/mouse.h"
#include "gfx/gui/area.h"



namespace TA3D
{



	UnitManager unit_manager;

	DlData::~DlData()
	{
		DELETE_ARRAY(dl_x);
		DELETE_ARRAY(dl_y);
		DELETE_ARRAY(dl_w);
		DELETE_ARRAY(dl_h);
	}

	inline bool overlaps( int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2 )
	{
		int w = w1 + w2;
		int h = h1 + h2;
		int X1 = Math::Min(x1, x2);
		int Y1 = Math::Min(y1, y2);
		int X2 = Math::Max(x1 + w1, x2 + w2);
		int Y2 = Math::Max(y1 + h1, y2 + h2);
		return X2 - X1 < w && Y2 - Y1 < h;
	}

	void UnitType::AddUnitBuild(int index, int px, int py, int pw, int ph, int p, GLuint Pic)
	{
		if (index < -1)
			return;

		if (!BuildList.empty() && nb_unit > 0)		// Vérifie si l'unité n'est pas déjà répertoriée / check if not already there
		{
			for (int i = 0; i < nb_unit; ++i)
			{
				if (BuildList[i] == index) // We already have it so leave now
				{
					if (Pic)
						gfx->destroy_texture(Pic);
					return;
				}
			}
		}

		if (p == -1)
		{
			if (dl_data == NULL)		// We can't add a menu entry if we don't know where to add it
			{
				LOG_ERROR(LOG_PREFIX_RESOURCES << "I can't add this menu entry without a list of available menu buttons");
				if (Pic)
					gfx->destroy_texture(Pic);
				return;
			}
			for(int i = 0 ; i <= nb_pages && p == -1 ; ++i)
			{
				for(int k = 0 ; k < dl_data->dl_num && p == -1 ; ++k)
				{
					bool found = true;
					for(int j = 0 ; j < nb_unit ; ++j)
					{
						if (Pic_p[j] == i
							&& overlaps(Pic_x[j], Pic_y[j], Pic_w[j], Pic_h[j],
										dl_data->dl_x[k], dl_data->dl_y[k], dl_data->dl_w[k], dl_data->dl_h[k]))
						{
							found = false;
							break;
						}
					}
					if (found)
					{
						p = i;
						px = dl_data->dl_x[k];
						py = dl_data->dl_y[k];
						pw = dl_data->dl_w[k];
						ph = dl_data->dl_h[k];
					}
				}
			}
		}
		nb_pages = Math::Max(nb_pages, short(p + 1));
		++nb_unit;
		if (BuildList.empty())
			nb_unit = 1;
		BuildList.push_back(short(index));
		PicList.push_back(Pic);
		Pic_x.push_back(short(px));
		Pic_y.push_back(short(py));
		Pic_w.push_back(short(pw));
		Pic_h.push_back(short(ph));
		Pic_p.push_back(short(p));
	}

	GLuint loadBuildPic(const String &gafFileName, const String &name, int *w = NULL, int *h = NULL)
	{
		if (name.empty())
			return 0;
		
		if (unit_manager.name2gaf.empty())
		{
			const String::Vector &animsList = unit_manager.animsList;

			for(String::Vector::const_iterator it = animsList.begin() ; it != animsList.end() ; ++it)
			{
				const String &gafName = *it;
				byte* gaf_file = VFS::Instance()->readFile( gafName );
				if (gaf_file)
				{
					int nbEntries = Gaf::RawDataEntriesCount(gaf_file);
					for(int i = 0 ; i < nbEntries ; ++i)
					{
						String key = Gaf::RawDataGetEntryName(gaf_file, i).toUpper();
						if (key.empty())
							continue;
						if (unit_manager.name2gaf.find(key) == unit_manager.name2gaf.end())
							unit_manager.name2gaf[key] = gafName;
					}
					DELETE_ARRAY(gaf_file);
				}
			}
		}

		GLuint tex = 0;
		gfx->set_texture_format(GL_RGB8);

		String key = String::ToUpper(name);
		HashMap< String >::Dense::iterator item = unit_manager.name2gaf.find(key);
		if (item != unit_manager.name2gaf.end())
		{
			std::vector<String> test;
			if (String::ToUpper(item->second) != String::ToUpper(gafFileName))
				test.push_back(gafFileName);
			test.push_back(item->second);
			for(std::vector<String>::iterator it = test.begin() ; it != test.end() && tex == 0 ; ++it)
			{
				byte* gaf_file = VFS::Instance()->readFile( *it );
				if (gaf_file)
				{
					SDL_Surface *img = Gaf::RawDataToBitmap(gaf_file, Gaf::RawDataGetEntryIndex(gaf_file, name), 0);
					if (img)
					{
						if (w)
							*w = img->w;
						if (h)
							*h = img->h;
						tex = gfx->make_texture(img, FILTER_LINEAR);
						SDL_FreeSurface(img);
					}

					DELETE_ARRAY(gaf_file);
				}
			}
		}
		return tex;
	}

	void UnitManager::analyse(String filename,int unit_index)
	{
		TDFParser gui_parser(filename, false, false, true);

		String number = Paths::ExtractFileNameWithoutExtension(filename);
		int first = int(number.size() - 1);
		while (first >= 0 && number[first] >= '0' && number[first] <= '9')
			--first;
		++first;
		number = number.substr(first, number.size() - first);

		int page = number.to<int>() - 1;		// Extract the page number

		int NbObj = gui_parser.pullAsInt("gadget0.totalgadgets");

		int x_offset = gui_parser.pullAsInt("gadget0.common.xpos");
		int y_offset = gui_parser.pullAsInt("gadget0.common.ypos");

		gfx->set_texture_format(GL_RGB8);
		String name;
		for (int i = 1; i <= NbObj; ++i)
		{
			int attribs = gui_parser.pullAsInt( String("gadget") << i << ".common.commonattribs" );
			if (!(attribs & 4) && !(attribs & 8))	// Neither a unit nor a weapon
				continue;
			int x = gui_parser.pullAsInt( String("gadget") << i << ".common.xpos" ) + x_offset;
			int y = gui_parser.pullAsInt( String("gadget") << i << ".common.ypos" ) + y_offset;
			int w = gui_parser.pullAsInt( String("gadget") << i << ".common.width" );
			int h = gui_parser.pullAsInt( String("gadget") << i << ".common.height" );
			name = gui_parser.pullAsString( String("gadget") << i << ".common.name" );
			int idx = (attribs & 4) ? get_unit_index(name) : -1;		// attribs & 4 ==> unit, attribs & 8 ==> weapon
			if ((attribs & 4) && idx == -1)
			{
				if (name != "IGPATCH")
					LOG_ERROR(LOG_PREFIX_RESOURCES << "Can't add unit to build menu : unit not found : '" << name << "'");
				continue;
			}

			GLuint tex = loadBuildPic( String("anims\\") << unit_type[unit_index]->Unitname << page + 1 << ".gaf", name, &w, &h);
			unit_type[unit_index]->AddUnitBuild(idx, x, y, w, h, page, tex);
		}
	}



	void UnitManager::analyse2(char *data,int size)
	{
		TDFParser parser;
		parser.loadFromMemory("analyse2", data, size, false, false, true);

		for(int g = 0 ; parser.exists(String("gadget") << g) ; g++)
		{
			String unitmenu = parser.pullAsString(String("gadget") << g << ".unitmenu");
			String unitname = parser.pullAsString(String("gadget") << g << ".unitname");

			if (unitmenu.empty() || unitname.empty()) continue;

			int unit_index = get_unit_index(unitmenu);
			if (unit_index == -1)
			{
				LOG_DEBUG("unit '" << unitmenu << "' not found");
				continue;		// Au cas où l'unité n'existerait pas
			}
			int idx = get_unit_index(unitname);
			if (idx >= 0 && idx < nb_unit)
			{
				GLuint tex = loadBuildPic( String("anims\\") << unitname << "_gadget.gaf", unitname);
				if (tex || unit_type[idx]->glpic)
					unit_type[unit_index]->AddUnitBuild(idx, -1, -1, 64, 64, -1, tex);
				else
				{	LOG_DEBUG("no build picture found for unit '" << unitname << "', cannot add it to " << unitmenu << " build menu");	}
			}
			else
			{	LOG_DEBUG("unit '" << unitname << "' not found, cannot add it to " << unitmenu << " build menu");	}
		}
	}



	int UnitManager::load_unit(const String &filename)			// Ajoute une nouvelle unité
	{
		unit_type.push_back(new UnitType());
		int result =  unit_type[nb_unit]->load(filename);
		if (!unit_type[nb_unit]->Unitname.empty())
			unit_hashtable[String::ToLower(unit_type[nb_unit]->Unitname)] = nb_unit + 1;
		if (!unit_type[nb_unit]->name.empty())
			unit_hashtable[String::ToLower(unit_type[nb_unit]->name)] = nb_unit + 1;
		if (!unit_type[nb_unit]->ObjectName.empty())
			unit_hashtable[String::ToLower(unit_type[nb_unit]->ObjectName)] = nb_unit + 1;
		if (!unit_type[nb_unit]->Description.empty())
			unit_hashtable[String::ToLower(unit_type[nb_unit]->Description)] = nb_unit + 1;
		if (!unit_type[nb_unit]->Designation_Name.empty())
			unit_hashtable[String::ToLower(unit_type[nb_unit]->Designation_Name)] = nb_unit + 1;
		nb_unit++;
		return result;
	}



	void UnitManager::gather_build_data()
	{
		uint32 file_size=0;
		String::List file_list;
		VFS::Instance()->getFilelist( ta3dSideData.download_dir + "*.tdf", file_list);

		for (String::List::const_iterator file = file_list.begin(); file != file_list.end(); ++file) // Cherche un fichier pouvant contenir des informations sur l'unité unit_name
		{
			byte* data = VFS::Instance()->readFile(*file, &file_size);		// Lit le fichier
			if (data)
			{
				analyse2((char*)data,file_size);
				DELETE_ARRAY(data);
			}
		}
	}


	void UnitManager::gather_all_build_data()
	{
		animsList.clear();
		VFS::Instance()->getFilelist("anims\\*.gaf", animsList);
		name2gaf.clear();

		String::List file_list;
		VFS::Instance()->getFilelist( ta3dSideData.guis_dir + "*.gui", file_list);

		String fileUp;
		String unitnameUp;
		const String::List::const_iterator end = file_list.end();
		// Cherche un fichier pouvant contenir des informations sur l'unité unit_name
		for (String::List::const_iterator file = file_list.begin(); file != end; ++file) 
		{
			for (int i = 0; i < nb_unit; ++i)
			{
				fileUp = *file;
				fileUp.toUpper();
				unitnameUp = unit_type[i]->Unitname;
				unitnameUp.toUpper();
				if (fileUp.find(unitnameUp) != String::npos && fileUp.find(unitnameUp + '.') == String::npos)
					analyse(*file,i);
			}
		}

		TDFParser sidedata_parser(ta3dSideData.gamedata_dir + "sidedata.tdf", false, true);
		for (int i = 0 ; i < nb_unit; ++i)
		{
			int n = 1;
			while(!sidedata_parser.pullAsString(String::ToLower(String("canbuild.") << unit_type[i]->Unitname << ".canbuild" << n ) ).empty())  n++;

			n--;
			String canbuild = sidedata_parser.pullAsString(String::ToLower(String("canbuild.") << unit_type[i]->Unitname << ".canbuild" << n ) );
			while (n > 0)
			{
				int idx = get_unit_index( canbuild );
				if (idx >= 0 && idx < nb_unit)
				{
					GLuint tex = loadBuildPic( String("anims\\") << canbuild << "_gadget.gaf", canbuild);
					if (unit_type[idx]->glpic || tex)
						unit_type[i]->AddUnitBuild(idx, -1, -1, 64, 64, -1, tex);
					else
					{	LOG_DEBUG("unit '" << canbuild << "' not found");	}
				}
				else
				{	LOG_DEBUG("unit '" << canbuild << "' not found");	}
				--n;
				canbuild = sidedata_parser.pullAsString( String("canbuild.") << unit_type[i]->Unitname << ".canbuild" << n );
			}
		}

		gather_build_data();			// Read additionnal build data
		animsList.clear();
		name2gaf.clear();
	}


	void UnitManager::load_script_file(const String &unit_name)
	{
		String uprname = String(unit_name).toUpper();
		int unit_index = get_unit_index(uprname);
		if (unit_index == -1)
			return;

		// Everything is done in the SCRIPT_DATA interface, it tries script types in priority order
		unit_type[unit_index]->script = ScriptData::loadScriptFile("scripts\\" + uprname);
	}


	void UnitManager::Identify()			// Identifie les pièces aux quelles les scripts font référence
	{
		for (int i = 0; i < nb_unit; ++i)
		{
			if (unit_type[i]->script && unit_type[i]->model)
				unit_type[i]->model->Identify(ScriptData::Ptr::WeakPointer(unit_type[i]->script));
		}
	}

	bool UnitType::floatting()
	{
		return Floater || canhover || !Yuni::Math::Zero(WaterLine);
	}

	void UnitType::destroy()
	{
		MovementClass.clear();
		soundcategory.clear();
		ExplodeAs.clear();
		SelfDestructAs.clear();
		script = NULL;

		w_badTargetCategory.clear();
		BadTargetCategory.clear();
		NoChaseCategory.clear();
		aim_data.clear();

		BuildList.clear();
		Pic_x.clear();
		Pic_y.clear();
		Pic_w.clear();
		Pic_h.clear();
		Pic_p.clear();

		if (!PicList.empty())
		{
			for (unsigned int i = 0; i < PicList.size(); ++i)
				gfx->destroy_texture(PicList[i]);
			PicList.clear();
		}

		yardmap.clear();
		model = NULL;
		if (unitpic)
			SDL_FreeSurface(unitpic);
		gfx->destroy_texture(glpic);
		Corpse.clear();
		Unitname.clear();
		name.clear();
		side.clear();
		ObjectName.clear();
		Designation_Name.clear();
		Description.clear();
		Category.clear();
		categories.clear();

		init();
	}

	void UnitType::init()
	{
        sweetspot_cached = -1;

        not_used = false;
		commander = false;
		selfdestructcountdown = 5;

		last_click = -1;
		click_time = 0.0f;

		emitting_points_computed = false;
		soundcategory.clear();

		isfeature=false;
		antiweapons=false;

		weapon.clear();         // No weapons
		aim_data.clear();

		script=NULL;		// Aucun script

		page=0;

		nb_pages = 0;
		nb_unit=0;
		BuildList.clear();
		PicList.clear();
		Pic_x.clear();				// Coordinates
		Pic_y.clear();
		Pic_w.clear();				// Coordinates
		Pic_h.clear();
		Pic_p.clear();				// Page where the pic has to be shown

		dl_data = NULL;

		init_cloaked = false;
		mincloakdistance = 10;
		DefaultMissionType=MISSION_STANDBY;
		attackrunlength=0;
		yardmap.clear();
		model=NULL;
		unitpic=NULL;
		glpic=0;
		hoverattack=false;
		SortBias=0;
		IsAirBase=false;
		AltFromSeaLevel=0;
		TransportSize=0;
		TransportCapacity=0;
		ManeuverLeashLength=640;
		CruiseAlt=0;
		TEDclass=CLASS_UNDEF;
		WaterLine=0.0f;
		StandingMoveOrder=1;
		MobileStandOrders=1;
		StandingFireOrder=1;
		FireStandOrders=1;
		ShootMe=false;
		ThreeD=true;
		Builder=false;
		Unitname.clear();
		name.clear();
		version=0;
		side.clear();
		ObjectName.clear();
		FootprintX=0;
		FootprintZ=0;
		Category.clear();
		categories.clear();
		fastCategory=0;
		MaxSlope=255;
		BMcode=0;
		norestrict=false;
		BuildAngle=10;
		canresurrect=false;
		Designation_Name.clear();	// Nom visible de l'unité
		Description.clear();		// Description
		BuildCostEnergy=0;		// Energie nécessaire pour la construire
		BuildCostMetal=0;		// Metal nécessaire pour la construire
		MaxDamage=10;			// Points de dégats maximum que l'unité peut encaisser
		EnergyUse=0;			// Energie nécessaire pour faire quelque chose
		BuildTime=0;			// Temps de construction
		WorkerTime=1;			// Vitesse de construction
		AutoFire=false;			// Tire automatique
		SightDistance=50;		// Distance maximale de vue de l'unité
		RadarDistance=0;		// Distance maximale de detection radar
		RadarDistanceJam=0;		// For Radar jammers
		EnergyStorage=0;		// Quantité d'énergie stockable par l'unité
		MetalStorage=0;			// Quantité de metal stockable par l'unité
		ExplodeAs.clear();			// Type d'explosion lorsque l'unité est détruite
		SelfDestructAs.clear();	// Type d'explosion lors de l'autodestruction
		Corpse.clear();			// Restes de l'unité
		UnitNumber=0;			// ID de l'unité
		canmove=false;			// Indique si l'unité peut bouger
		canpatrol=false;		// si elle peut patrouiller
		canstop=false;			// si elle peut s'arrêter
		canguard=false;			// si elle peut garder une autre unité
		MaxVelocity=1;			// Vitesse maximale
		BrakeRate=1;			// Vitesse de freinage
		Acceleration=1;			// Accélération
		TurnRate=1;				// Vitesse de tournage
		SteeringMode=0;
		canfly=false;			// si l'unité peut voler
		Scale=1.0f;				// Echelle
		BankScale=0;
		BuildDistance=0.0f;		// Distance maximale de construction
		CanReclamate=false;		// si elle peut récupérer
		EnergyMake=0;			// Production d'énergie de l'unité
		MetalMake=0.0f;			// Production de métal de l'unité
		MovementClass.clear();		// Type de mouvement
		Upright=false;			// Si l'unité est debout
		w_badTargetCategory.clear();	// Type d'unité non ciblable par les armes
		BadTargetCategory.clear();	// Type d'unité non attacable
		DamageModifier=1.0f;	// How much of the weapon damage it takes
		canattack=false;			// Si l'unité peut attaquer
		ActivateWhenBuilt=false;// L'unité s'active lorsqu'elle est achevée
		onoffable=false;		// (Dés)activable
		MaxWaterDepth=0;		// Profondeur maximale où l'unité peut aller
		MinWaterDepth=-0xFFF;	// Profondeur minimale où l'unité peut aller
		NoShadow=false;			// Si l'unité n'a pas d'ombre
		TransMaxUnits=0;		// Maximum d'unités portables
		canload=false;			// Si elle peut charger d'autres unités
		WeaponID.clear();		// Arme 2
		Floater=false;			// Si l'unité flotte
		canhover=false;			// For hovercrafts
		NoChaseCategory.clear();		// Type d'unité non chassable
		SonarDistance=0;		// Portée du sonar
		SonarDistanceJam=0;		// For Sonar jammers
		candgun=false;			// si l'unité peut utiliser l'arme ravage
		CloakCost = 0;			// Coût en energie pour rendre l'unité invisible
		CloakCostMoving = 0;	// Idem mais quand l'unité bouge
		HealTime=0;				// Temps nécessaire à l'unité pour se réparer
		CanCapture=false;		// Si elle peut capturer d'autres unités
		HideDamage=false;		// Cache la vie de l'unité aux autres joueurs
		ImmuneToParalyzer=false;	// Immunisation
		Stealth=false;
		MakesMetal=0;			// production de métal de l'unité
		ExtractsMetal=0.0f;		// métal extrait par l'unité
		TidalGenerator=false;	// Si l'unité est une centrale marée-motrice
		TransportMaxUnits=0;	// Maximum d'unités transportables
		kamikaze=false;			// Unité kamikaze
		kamikazedistance=0;
		WindGenerator=0;		// Centrale de type Eolienne
	}


	void UnitType::show_info()
	{
		Gui::AREA::current()->caption("unit_info.tName", I18N::Translate("Name") + ": " + name);
		Gui::AREA::current()->caption("unit_info.tInternalName", I18N::Translate("Internal name") + ": " + Unitname);
		Gui::AREA::current()->caption("unit_info.tDescription", Description);
		Gui::AREA::current()->caption("unit_info.tHP", I18N::Translate("HP") + ": " + String(MaxDamage));
		Gui::AREA::current()->caption("unit_info.tCost", I18N::Translate("Cost") + ": E " + String(BuildCostEnergy) + " M " + String(BuildCostMetal));
		Gui::AREA::current()->caption("unit_info.tBuildTime", I18N::Translate("Build time") + ": " + String(BuildTime));

		String tWeapons;
		for( std::vector<WeaponDef*>::iterator i = weapon.begin() ; i != weapon.end() ; ++i )
			if (*i)
				tWeapons << (*i)->name + ": " + String( (*i)->damage) << "\n";
		Gui::AREA::current()->caption("unit_info.tWeaponList", tWeapons);
		Gui::GUIOBJ::Ptr image = Gui::AREA::current()->get_object("unit_info.unitpic");
		if (image)
		{
			image->x1 = 10;
			image->y1 = 40;
			image->x2 = 106;
			image->y2 = 136;
			image->Data = (uint32)glpic;
		}

		Gui::AREA::current()->msg("unit_info.show");
	}

#define parseStringDef(x,y)  (unitParser.pullAsString(x, unitParser_ci.pullAsString(x, y)))
#define parseIntDef(x, y)    (unitParser.pullAsInt(x, unitParser_ci.pullAsInt(x, y)))
#define parseBoolDef(x, y)   (unitParser.pullAsBool(x, unitParser_ci.pullAsBool(x, y)))
#define parseFloatDef(x, y)  (unitParser.pullAsFloat(x, unitParser_ci.pullAsFloat(x, y)))

#define parseString(x)  (unitParser.pullAsString(x, unitParser_ci.pullAsString(x)))
#define parseInt(x)     (unitParser.pullAsInt(x, unitParser_ci.pullAsInt(x)))
#define parseBool(x)    (unitParser.pullAsBool(x, unitParser_ci.pullAsBool(x)))
#define parseFloat(x)   (unitParser.pullAsFloat(x, unitParser_ci.pullAsFloat(x)))

	int UnitType::load(const String &filename)
	{
		destroy();
		int nb_inconnu = 0;
		String lang_name = I18N::Translate("UNITTYPE_NAME", "UNITINFO.Name");
		String lang_desc = I18N::Translate("UNITTYPE_DESCRIPTION", "UNITINFO.Description");
		String lang_name_alt = I18N::Translate("UNITTYPE_NAME_ALT", "UNITINFO.Name");
		String lang_desc_alt = I18N::Translate("UNITTYPE_DESCRIPTION_ALT", "UNITINFO.Description");

		TDFParser unitParser( filename, true, true );         // FBI files are case sensitive (something related to variable priority)
		TDFParser unitParser_ci( filename, false, true );     // Case insensitive parser

		Unitname = parseString("UNITINFO.UnitName");
		version = byte(parseInt("UNITINFO.Version"));
		side = parseString("UNITINFO.Side");
		ObjectName = parseString("UNITINFO.Objectname");
		Designation_Name = parseString("UNITINFO.Designation");
		Description = parseStringDef( lang_desc, parseStringDef( lang_desc_alt, parseString("UNITINFO.Description") ) );
		name = parseStringDef( lang_name, parseStringDef(lang_name_alt, parseString("UNITINFO.Name") ) );

		FootprintX = byte(parseInt("UNITINFO.FootprintX"));
		FootprintZ = byte(parseInt("UNITINFO.FootprintZ"));
		BuildCostEnergy = parseInt("UNITINFO.BuildCostEnergy");
		BuildCostMetal = parseInt("UNITINFO.BuildCostMetal");
		MaxDamage = parseInt("UNITINFO.MaxDamage");
		MaxWaterDepth = short(parseIntDef("UNITINFO.MaxWaterDepth", 0));
		MinWaterDepth = short(parseIntDef("UNITINFO.MinWaterDepth", -0xFFF));
		if (MinWaterDepth != -0xFFF && MaxWaterDepth == 0) MaxWaterDepth = 255;
		EnergyUse = parseInt("UNITINFO.EnergyUse");
		BuildTime = parseInt("UNITINFO.BuildTime");
		WorkerTime = parseIntDef("UNITINFO.WorkerTime",1);
        Builder = parseBool("UNITINFO.Builder") | parseBool("UNITINFO.canbuild");
		ThreeD = parseBoolDef("UNITINFO.ThreeD",true);
		SightDistance = parseIntDef("UNITINFO.SightDistance",100) >> 1;
		RadarDistance = parseInt("UNITINFO.RadarDistance") >> 1;
		RadarDistanceJam = parseInt("UNITINFO.RadarDistanceJam") >> 1;
		soundcategory = parseString("UNITINFO.SoundCategory");
		if (!parseString("UNITINFO.wthi_badTargetCategory").empty())
		{
			if (w_badTargetCategory.size() < 3)
				w_badTargetCategory.resize(3);
			w_badTargetCategory[2] = parseString("UNITINFO.wthi_badTargetCategory");
		}
		if (!parseString("UNITINFO.wsec_badTargetCategory").empty())
		{
			if (w_badTargetCategory.size() < 2)
				w_badTargetCategory.resize(2);
			w_badTargetCategory[1] = parseString("UNITINFO.wsec_badTargetCategory");
		}
		if (!parseString("UNITINFO.wpri_badTargetCategory").empty())
		{
			if (w_badTargetCategory.size() < 1)
				w_badTargetCategory.resize(1);
			w_badTargetCategory[0] = parseString("UNITINFO.wpri_badTargetCategory");
		}
		for (unsigned int i = 4 ; !parseString( String("UNITINFO.w") << i << "_badTargetCategory" ).empty() ; ++i)
		{
			if (w_badTargetCategory.size() < i)
				w_badTargetCategory.resize(i);
			w_badTargetCategory[i-1] = parseString(String("UNITINFO.w") << i << "_badTargetCategory");
		}
		NoChaseCategory = parseString("UNITINFO.NoChaseCategory");
		BadTargetCategory = parseString("UNITINFO.BadTargetCategory");

		String category = String::ToLower( parseString("UNITINFO.Category") );
		Category.clear();
		categories.clear();
		category.explode(categories, ' ');
		for (String::Vector::const_iterator i = categories.begin(); i != categories.end(); ++i)
			if (!i->empty())
				Category[*i] = 1;
		fastCategory = 0;
		if (checkCategory( "kamikaze" ) )	fastCategory |= CATEGORY_KAMIKAZE;
		if (checkCategory( "notair" ) )		fastCategory |= CATEGORY_NOTAIR;
		if (checkCategory( "notsub" ) )		fastCategory |= CATEGORY_NOTSUB;
		if (checkCategory( "jam" ) )		fastCategory |= CATEGORY_JAM;
		if (checkCategory( "commander" ) )	fastCategory |= CATEGORY_COMMANDER;
		if (checkCategory( "weapon" ) )		fastCategory |= CATEGORY_WEAPON;
		if (checkCategory( "level3" ) )		fastCategory |= CATEGORY_LEVEL3;

		UnitNumber = short(parseInt("UNITINFO.UnitNumber"));
		canmove = parseBool("UNITINFO.canmove");
		canpatrol = parseBool("UNITINFO.canpatrol");
		canstop = parseBool("UNITINFO.canstop");
		canguard = parseBool("UNITINFO.canguard");
		MaxVelocity = parseFloatDef("UNITINFO.MaxVelocity", 1.0f / 16.0f) * 16.0f;
		BrakeRate = parseFloatDef("UNITINFO.BrakeRate", 1.0f / 160.0f) * 160.0f;
		Acceleration = parseFloatDef("UNITINFO.Acceleration", 1.0f / 160.0f) * 160.0f;
		TurnRate = parseFloatDef("UNITINFO.TurnRate", 1.0f / (TA2DEG * 20.0f)) * TA2DEG * 20.0f;
		candgun = parseBool("UNITINFO.candgun");
		canattack = parseBool("UNITINFO.canattack");
		CanReclamate = parseBool("UNITINFO.CanReclamate");
		EnergyMake = short(parseInt("UNITINFO.EnergyMake"));
		MetalMake = parseFloat("UNITINFO.MetalMake");
		CanCapture = parseBool("UNITINFO.CanCapture");
		HideDamage = parseBool("UNITINFO.HideDamage");
		HealTime = parseInt("UNITINFO.HealTime") * 30;
		CloakCost = parseInt("UNITINFO.CloakCost");
		CloakCostMoving = parseInt("UNITINFO.CloakCostMoving");
		init_cloaked = parseBool("UNITINFO.init_cloaked");
		mincloakdistance = parseIntDef("UNITINFO.mincloakdistance", 20) >> 1;
		BuildDistance = parseFloat("UNITINFO.Builddistance");
		ActivateWhenBuilt = parseBool("UNITINFO.ActivateWhenBuilt");
		ImmuneToParalyzer = parseBool("UNITINFO.ImmuneToParalyzer");
		SonarDistance = parseInt("UNITINFO.SonarDistance")>>1;
		SonarDistanceJam = parseInt("UNITINFO.SonarDistanceJam")>>1;
		// copyright = ... not needed here :P
		MaxSlope = short(parseIntDef("UNITINFO.MaxSlope", 255));
		SteeringMode = byte(parseInt("UNITINFO.SteeringMode"));
		BMcode = byte(parseInt("UNITINFO.BMcode"));
		ShootMe = parseBool("UNITINFO.ShootMe");
		Upright = parseBool("UNITINFO.Upright");
		norestrict = parseBool("UNITINFO.norestrict");
		AutoFire = !parseBoolDef("UNITINFO.NoAutoFire", true);
		EnergyStorage = parseInt("UNITINFO.EnergyStorage");
		MetalStorage = parseInt("UNITINFO.MetalStorage");
		StandingMoveOrder = byte(parseIntDef("UNITINFO.StandingMoveOrder", 1));
		MobileStandOrders = byte(parseIntDef("UNITINFO.mobilestandorders", 1));
		StandingFireOrder = byte(parseIntDef("UNITINFO.StandingFireOrder", 1));
		FireStandOrders = byte(parseIntDef("UNITINFO.firestandorders", 1));
		WaterLine = parseFloat("UNITINFO.WaterLine");

		String TEDclassString = String::ToLower( parseString("UNITINFO.TEDClass") );
		if (TEDclassString.find("water") != String::npos)           TEDclass = CLASS_WATER;
		else if (TEDclassString.find("ship") != String::npos)       TEDclass = CLASS_SHIP;
		else if (TEDclassString.find("energy") != String::npos)     TEDclass = CLASS_ENERGY;
		else if (TEDclassString.find("vtol") != String::npos)       TEDclass = CLASS_VTOL;
		else if (TEDclassString.find("kbot") != String::npos)       TEDclass = CLASS_KBOT;
		else if (TEDclassString.find("plant") != String::npos)      TEDclass = CLASS_PLANT;
		else if (TEDclassString.find("tank") != String::npos)       TEDclass = CLASS_TANK;
		else if (TEDclassString.find("special") != String::npos)    TEDclass = CLASS_SPECIAL;
		else if (TEDclassString.find("fort") != String::npos)       TEDclass = CLASS_FORT;
		else if (TEDclassString.find("metal") != String::npos)      TEDclass = CLASS_METAL;
		else if (TEDclassString.find("cnstr") != String::npos)      TEDclass = CLASS_CNSTR;
		else if (TEDclassString.find("commander") != String::npos)  TEDclass = CLASS_COMMANDER;
		else if (!TEDclassString.empty())
		{
			LOG_DEBUG("unknown tedclass ID : " << TEDclassString);
			nb_inconnu++;
		}

		NoShadow = parseBool("UNITINFO.NoShadow");
		antiweapons = parseBool("UNITINFO.antiweapons");
		BuildAngle = parseIntDef("UNITINFO.buildangle",10);
		canfly = parseBool("UNITINFO.Canfly");
		canload = parseBool("UNITINFO.canload");
		Floater = parseBool("UNITINFO.Floater");
		canhover = parseBool("UNITINFO.canhover");
		if (canhover)           // Can go over water so let's say MinWaterDepth is negative and MaxWaterDepth is null, that way it's a standard hovercraft :)
		{
			MinWaterDepth = -0xFFF;
			MaxWaterDepth = 0;
		}
		BankScale = byte(parseInt("UNITINFO.BankScale"));
		TidalGenerator = parseBool("UNITINFO.TidalGenerator");
		Scale = 1.0f;//parseFloat("UNITINFO.Scale",1.0f);
		Corpse = parseString("UNITINFO.Corpse");
		WindGenerator = short(parseInt("UNITINFO.WindGenerator"));
		onoffable = parseBool("UNITINFO.onoffable");
		kamikaze = parseBool("UNITINFO.kamikaze");
		kamikazedistance = uint16(parseInt("UNITINFO.kamikazedistance") >> 1);

		unsigned int i = 1;
		while (i <= 3 || !parseString( String("UNITINFO.Weapon") << i ).empty())
		{
			if (WeaponID.size() < i)
				WeaponID.resize(i,-1);
			WeaponID[i-1] = weapon_manager.get_weapon_index( parseString( String("UNITINFO.Weapon") << i ) );
			++i;
		}
		yardmap = parseString("UNITINFO.YardMap");
		if (!yardmap.empty())
		{
			i = 0;
			for (unsigned int e = 0 ; e < yardmap.size() ; e++)
				if (yardmap[e] == ' ')
					i++;
				else
					yardmap[e-i] = yardmap[e];
			yardmap.truncate(yardmap.size() - i);
			while (yardmap.size() < FootprintX * FootprintZ)     // Complete the yardmap if needed
				yardmap += yardmap.at(yardmap.size() - 1);
		}

		CruiseAlt = short(parseInt("UNITINFO.cruisealt"));
		ExplodeAs = parseString("UNITINFO.ExplodeAs");
		SelfDestructAs = parseString("UNITINFO.SelfDestructAs");
		ManeuverLeashLength = short(parseIntDef("UNITINFO.maneuverleashlength", 640));

		String DefaultMissionTypeString = String::ToLower( parseString("UNITINFO.DefaultMissionType") );
		if (DefaultMissionTypeString == "standby")				DefaultMissionType=MISSION_STANDBY;
		else if (DefaultMissionTypeString == "vtol_standby")		DefaultMissionType=MISSION_VTOL_STANDBY;
		else if (DefaultMissionTypeString == "guard_nomove")		DefaultMissionType=MISSION_GUARD_NOMOVE;
		else if (DefaultMissionTypeString == "standby_mine")		DefaultMissionType=MISSION_STANDBY_MINE;
		else if (!DefaultMissionTypeString.empty())
		{
			LOG_ERROR("Unknown constant: `" << DefaultMissionTypeString << "`");
			++nb_inconnu;
		}

		TransMaxUnits = TransportMaxUnits = byte(parseInt("UNITINFO.TransMaxUnits"));
		TransMaxUnits = TransportMaxUnits = byte(parseIntDef("UNITINFO.transportmaxunits",TransMaxUnits));
		TransportCapacity = byte(parseInt("UNITINFO.transportcapacity"));
		TransportSize = byte(parseInt("UNITINFO.transportsize"));
		AltFromSeaLevel = short(parseInt("UNITINFO.altfromsealevel"));
		MovementClass = parseString("UNITINFO.MovementClass");

		IsAirBase = parseBool("UNITINFO.IsAirBase");
		commander = parseBool("UNITINFO.Commander");
		DamageModifier = parseFloatDef("UNITINFO.DamageModifier",1.0f);
		MakesMetal = parseFloat("UNITINFO.MakesMetal");
		SortBias = byte(parseInt("UNITINFO.sortbias"));
		ExtractsMetal = parseFloat("UNITINFO.ExtractsMetal");
		hoverattack = parseBool("UNITINFO.HoverAttack");
		isfeature = parseBool("UNITINFO.IsFeature");
		Stealth = parseInt("UNITINFO.Stealth");
		attackrunlength = parseInt("UNITINFO.attackrunlength");
		selfdestructcountdown = uint8(parseIntDef("UNITINFO.selfdestructcountdown", 5));
		canresurrect = parseBool("UNITINFO.canresurrect") || parseBool("UNITINFO.resurrect");

		aim_data.resize(WeaponID.size());
		for (unsigned int i = 0 ; i < WeaponID.size(); i++)
		{
			aim_data[i].check = false;
			if (WeaponID[i] > -1)
			{
				String aimdir = parseString( String("UNITINFO.WeaponMainDir") << i );
				if (!aimdir.empty())
				{
					String::Vector vec;
					aimdir.explode(vec, ' ');
					if (vec.size() == 3)
					{
						aim_data[i].check = true;
						aim_data[i].dir.x = vec[0].to<float>();
						aim_data[i].dir.y = vec[1].to<float>();
						aim_data[i].dir.z = vec[2].to<float>();
						// Should read almost every possible case
						aim_data[i].Maxangledif = parseFloat( String("UNITINFO.Maxangledif") << i );
					}
					else
					{	LOG_DEBUG("FBI parser error: '" << aimdir << "' could not be parsed correctly");	}
				}
			}
		}

		if (canresurrect && Yuni::Math::Zero(BuildDistance))
			BuildDistance = float(SightDistance);
		weapon.resize( WeaponID.size() );
		w_badTargetCategory.resize( WeaponID.size() );
		for (unsigned int i = 0; i < WeaponID.size(); ++i)
		{
			if (WeaponID[i] > -1)
				weapon[i] = &(weapon_manager.weapon[WeaponID[i]]);
		}
		if (!Unitname.empty())
		{
			model = model_manager.get_model(ObjectName);
			if (model == NULL)
				LOG_ERROR("`" << Unitname << "` without a 3D model");
		}
		else
			LOG_WARNING("The unit does not have a name");
		if (canfly == 1)
			TurnRate = TurnRate * 3; // A hack thanks to Doors
		// Build the repulsion grid
		gRepulsion.resize(FootprintX * 4, FootprintZ * 4);
		float sigx = FootprintX * 0.75f;
		float sigz = FootprintZ * 0.75f;
		float sigx2 = -0.5f / (sigx * sigx);
		float sigz2 = -0.5f / (sigz * sigz);
		for(int z = 0 ; z < gRepulsion.getHeight() ; ++z)
		{
			float dz = z - gRepulsion.getHeight() * 0.5f;
			// Distance to the unit, not its center of gravity if it's a building
			if (!BMcode)
				dz = Math::Sgn(dz) * Math::Max(fabsf(dz) - FootprintZ * 0.5f, 0.0f);
			dz *= dz;
			for(int x = 0 ; x < gRepulsion.getWidth() ; ++x)
			{
				float dx = x - gRepulsion.getWidth() * 0.5f;
				// Distance to the unit, not its center of gravity if it's a building
				if (!BMcode)
					dx = Math::Sgn(dx) * Math::Max(fabsf(dx) - FootprintX * 0.5f, 0.0f);
				dx *= dx;
				gRepulsion(x,z) = 2550.0f * expf(sigx2 * dx + sigz2 * dz);
			}
		}
		load_dl();
		return nb_inconnu;
	}

	void UnitType::load_dl()
	{
		if (side.empty())
			return;
		dl_data = NULL;
		if (unit_manager.h_dl_data.count(String::ToLower(side)) != 0)
			dl_data = unit_manager.h_dl_data[String::ToLower(side)];

		if (dl_data)
			return;			// Ok it's already loaded

		int side_id = -1;
		for( int i = 0 ; i < ta3dSideData.nb_side && side_id == -1 ; i++ )
			if (String::ToLower( ta3dSideData.side_name[ i ] ) == String::ToLower( side ) )
				side_id = i;
		if (side_id == -1)
			return;


		TDFParser dl_parser;
		if (dl_parser.loadFromFile(ta3dSideData.guis_dir + ta3dSideData.side_pref[side_id] + "dl.gui", false, false, true))
		{
			dl_data = new DlData;
			int NbObj = dl_parser.pullAsInt( "gadget0.totalgadgets" );

			int x_offset = dl_parser.pullAsInt( "gadget0.common.xpos" );
			int y_offset = dl_parser.pullAsInt( "gadget0.common.ypos" );

			dl_data->dl_num = 0;

			for (int i = 1; i <= NbObj; ++i)
			{
				if (dl_parser.pullAsInt(String("gadget") << i << ".common.attribs") == 32)
					dl_data->dl_num++;
			}

			dl_data->dl_x = new short[dl_data->dl_num];
			dl_data->dl_y = new short[dl_data->dl_num];
			dl_data->dl_w = new short[dl_data->dl_num];
			dl_data->dl_h = new short[dl_data->dl_num];

			int e = 0;
			for (int i = 1; i <= NbObj; ++i)
			{
				if (dl_parser.pullAsInt( String("gadget") << i << ".common.attribs" ) == 32 )
				{
					dl_data->dl_x[e] = short(dl_parser.pullAsInt(String("gadget") << i << ".common.xpos") + x_offset);
					dl_data->dl_y[e] = short(dl_parser.pullAsInt(String("gadget") << i << ".common.ypos") + y_offset);
					dl_data->dl_w[e] = short(dl_parser.pullAsInt(String("gadget") << i << ".common.width"));
					dl_data->dl_h[e] = short(dl_parser.pullAsInt(String("gadget") << i << ".common.height"));
					++e;
				}
			}

			unit_manager.h_dl_data[String::ToLower(side)] = dl_data;
		}
		else
		{
			LOG_WARNING("`dl.gui` file is missing");
			dl_data = NULL;
		}
	}

	void UnitManager::destroy()
	{
		unit_hashtable.clear();

		for (HashMap< DlData* >::Dense::iterator i = h_dl_data.begin() ; i != h_dl_data.end() ; ++i)
			delete i->second;
		h_dl_data.clear();

		for (UnitList::iterator i = unit_type.begin(); i != unit_type.end(); ++i)
			delete *i;
		unit_type.clear();
		panel.destroy();
		paneltop.destroy();
		panelbottom.destroy();
		init();
	}

	void UnitManager::load_panel_texture( const String &player_side, const String &intgaf )
	{
		panel.destroy();
		String gaf_img;

		TDFParser parser;
		if (parser.loadFromFile(ta3dSideData.guis_dir + player_side + "MAIN.GUI"))
			gaf_img = parser.pullAsString( "gadget0.panel" );
		else
			LOG_ERROR("Unable to load `"<< (ta3dSideData.guis_dir + player_side + "MAIN.GUI") << "`");

		if (g_useTextureCompression && lp_CONFIG->use_texture_compression)
			gfx->set_texture_format(GL_COMPRESSED_RGB_ARB);
		else
			gfx->set_texture_format(GL_RGB8);
		int w,h;
		GLuint panel_tex = Gaf::ToTexture("anims\\" + player_side + "main.gaf", gaf_img, &w, &h, true);
		if (panel_tex == 0)
		{
			String::List file_list;
            VFS::Instance()->getFilelist( "anims\\*.gaf", file_list);
			for (String::List::const_iterator i = file_list.begin(); i != file_list.end() && panel_tex == 0; ++i)
				panel_tex = Gaf::ToTexture(*i, gaf_img, &w, &h, true);
		}
		panel.set(panel_tex);
		panel.width = w;
		panel.height = h;

		paneltop.set(Gaf::ToTexture("anims\\" + intgaf + ".gaf", "PANELTOP", &w, &h));
		paneltop.width = w;
		paneltop.height = h;
		panelbottom.set(Gaf::ToTexture("anims\\" + intgaf + ".gaf", "PANELBOT", &w, &h));
		panelbottom.width = w;
		panelbottom.height = h;
	}



	int UnitManager::unit_build_menu(int index,int omb,float &dt, int scrolling, bool GUI)				// Affiche et gère le menu des unités
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		bool nothing = index < -1 || index>=nb_unit;

		gfx->ReInitTexSys();
		glColor4ub(0xFF, 0xFF, 0xFF, 0xBF);
		if (GUI)
		{
			if (panel.tex && nothing)
				gfx->drawtexture( panel.tex, 0.0f, 128.0f, 128.0f, 128.0f + float(panel.height) );

			if (paneltop.tex )
			{
				gfx->drawtexture( paneltop.tex, 128.0f, 0.0f, 128.0f + float(paneltop.width), float(paneltop.height) );
				for (int k = 0 ; 128 + paneltop.width + panelbottom.width * k < uint32(SCREEN_W); ++k)
				{
					gfx->drawtexture(panelbottom.tex, 128.0f + float(paneltop.width + k * panelbottom.width), 0.0f,
							128.0f + float(paneltop.width + panelbottom.width * (k + 1)), float(panelbottom.height) );
				}
			}

			if (panelbottom.tex)
			{
				for (int k = 0 ; 128 + panelbottom.width * k < uint32(SCREEN_W) ; ++k)
				{
					gfx->drawtexture( panelbottom.tex, 128.0f + float(k * panelbottom.width),
							float(SCREEN_H - panelbottom.height), 128.0f + float(panelbottom.width * (k + 1)), float(SCREEN_H) );
				}
			}

			glDisable(GL_TEXTURE_2D);
			glColor4ub(0x0, 0x0, 0x0, 0xBF);
			glBegin(GL_QUADS);
			glVertex2i(0, 0);			// Barre latérale gauche
			glVertex2i(128, 0);
			glVertex2i(128, 128);
			glVertex2i(0, 128);

			glVertex2i(0, 128 + panel.height);			// Barre latérale gauche
			glVertex2i(128, 128 + panel.height);
			glVertex2i(128, SCREEN_H);
			glVertex2i(0, SCREEN_H);
			glEnd();
			glColor4ub(0xFF, 0xFF, 0xFF, 0xBF);
			return 0;
		}

		glEnable(GL_TEXTURE_2D);

		if (index<0 || index>=nb_unit) return -1;		// L'indice est incorrect

		int page=unit_type[index]->page;

		int sel=-1;

		gfx->set_2D_clip_rectangle(0, 128, 128, SCREEN_H - 128);

		glDisable(GL_BLEND);
		for( int i = 0 ; i < unit_type[index]->nb_unit ; ++i) // Affiche les différentes images d'unités constructibles
		{
			if (unit_type[index]->Pic_p[i] != page)
				continue;
			int px = unit_type[index]->Pic_x[ i ];
			int py = unit_type[index]->Pic_y[ i ] - scrolling;
			int pw = unit_type[index]->Pic_w[ i ];
			int ph = unit_type[index]->Pic_h[ i ];
			bool unused = unit_type[index]->BuildList[i] >= 0 && unit_type[unit_type[index]->BuildList[i]]->not_used;
			if (unused)
				glColor4ub(0x4C, 0x4C, 0x4C, 0xFF);		// Make it darker
			else
				glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);

			if (unit_type[index]->PicList[i])							// If a texture is given use it
				gfx->drawtexture(unit_type[index]->PicList[i], float(px), float(py), float(px + pw), float(py + ph));
			else if (unit_type[index]->BuildList[i] >= 0)
				gfx->drawtexture(unit_type[unit_type[index]->BuildList[i]]->glpic, float(px), float(py), float(px + pw), float(py + ph));

			if (mouse_x >= px && mouse_x < px + pw && mouse_y >= py && mouse_y < py + ph && !unused)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA,GL_ONE);
				glColor4ub(0xFF, 0xFF, 0xFF, 0xBF);
				if (unit_type[index]->PicList[i])							// If a texture is given use it
					gfx->drawtexture(unit_type[index]->PicList[i], float(px), float(py), float(px + pw), float(py + ph));
				else if (unit_type[index]->BuildList[i] >= 0)
					gfx->drawtexture(unit_type[unit_type[index]->BuildList[i]]->glpic, float(px), float(py), float(px + pw), float(py + ph));
				glDisable(GL_BLEND);
				sel = unit_type[index]->BuildList[i];
				if (sel == -1)
					sel = -2;
			}

			if (( unit_type[index]->BuildList[i] == unit_type[index]->last_click
				  || ( unit_type[index]->last_click == -2 && unit_type[index]->BuildList[i] == -1 ) )
				&& unit_type[index]->click_time > 0.0f )
			{
				glEnable(GL_BLEND);
				glDisable(GL_TEXTURE_2D);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glColor4ub(0xFF, 0xFF, 0xFF, byte(255.0f * unit_type[index]->click_time));
				float mx = float(px);
				float my = float(py);
				float mw = float(pw);
				float mh = float(ph);
				gfx->rectfill( mx, my, mx + mw, my + mh );
				glColor4ub(0xFF, 0xFF, 0x00, 0xBF);
				gfx->line( mx, my + mh * unit_type[index]->click_time, mx + mw, my + mh * unit_type[index]->click_time );
				gfx->line( mx, my + mh * (1.0f - unit_type[index]->click_time), mx + mw, my + mh * (1.0f - unit_type[index]->click_time) );
				gfx->line( mx + mw * unit_type[index]->click_time, my, mx + mw * unit_type[index]->click_time, my + mh );
				gfx->line( mx + mw * (1.0f - unit_type[index]->click_time), my, mx + mw * (1.0f - unit_type[index]->click_time), my + mh );
				glColor4ub(0xFF, 0xFF, 0xFF, 0xBF);
				glEnable(GL_TEXTURE_2D);
				glDisable(GL_BLEND);
			}
		}
		glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);

		if (unit_type[index]->last_click != -1 )
			unit_type[index]->click_time -= dt;

		if (sel>-1)
		{
			gfx->print(gfx->normal_font,ta3dSideData.side_int_data[ players.side_view ].Name.x1,ta3dSideData.side_int_data[ players.side_view ].Name.y1,0.0f,0xFFFFFFFF,
					   unit_type[sel]->name + " M:" + String(unit_type[sel]->BuildCostMetal)+" E:"+String(unit_type[sel]->BuildCostEnergy)+" HP:"+String(unit_type[sel]->MaxDamage) );

			if (!unit_type[sel]->Description.empty())
				gfx->print(gfx->normal_font,ta3dSideData.side_int_data[ players.side_view ].Description.x1,ta3dSideData.side_int_data[ players.side_view ].Description.y1,0.0f,0xFFFFFFFF,unit_type[sel]->Description );
			glDisable(GL_BLEND);
		}

		if (sel != -1 && mouse_b == 1 && omb != 1)		// Click !!
		{
			unit_type[index]->last_click = sint16(sel);
			unit_type[index]->click_time = 0.5f;		// One sec animation;
		}

		gfx->set_2D_clip_rectangle();

		return sel;
	}

	int load_all_units(void (*progress)(float percent,const String &msg))
	{
		unit_manager.init();
		int nb_inconnu=0;
		String::List file_list;
		VFS::Instance()->getFilelist( ta3dSideData.unit_dir + "*" + ta3dSideData.unit_ext, file_list);

		int n = 0;

		for (String::List::iterator i = file_list.begin(); i != file_list.end(); ++i)
		{
			if (progress != NULL && !(n & 0xF))
				progress((300.0f + float(n) * 50.0f / float(file_list.size() + 1)) / 7.0f, I18N::Translate("Loading units"));
			++n;

			String nom = String::ToUpper(Paths::ExtractFileNameWithoutExtension(*i));			// Vérifie si l'unité n'est pas déjà chargée

			if (unit_manager.get_unit_index(nom) == -1)
			{
				LOG_DEBUG("Loading the unit `" << nom << "`...");
				nb_inconnu += unit_manager.load_unit(*i);
				if (!unit_manager.unit_type[unit_manager.nb_unit - 1]->Unitname.empty())
				{
					String nom_pcx;
					nom_pcx << "unitpics\\" << unit_manager.unit_type[unit_manager.nb_unit - 1]->Unitname << ".pcx";
					unit_manager.unit_type[unit_manager.nb_unit - 1]->unitpic = gfx->load_image(nom_pcx);

					if (unit_manager.unit_type[unit_manager.nb_unit - 1]->unitpic)
					{
						gfx->set_texture_format(GL_RGB8);
						unit_manager.unit_type[unit_manager.nb_unit - 1]->glpic = gfx->make_texture(unit_manager.unit_type[unit_manager.nb_unit - 1]->unitpic, FILTER_LINEAR);
						SDL_FreeSurface(unit_manager.unit_type[unit_manager.nb_unit - 1]->unitpic);
						unit_manager.unit_type[unit_manager.nb_unit - 1]->unitpic = NULL;
					}
				}
			}
		}

		for (int i = 0;i < unit_manager.nb_unit; ++i)
			unit_manager.load_script_file(unit_manager.unit_type[i]->Unitname);

		unit_manager.Identify();

		unit_manager.gather_all_build_data();

		// Correct some data given in the FBI file using data from the moveinfo.tdf file
		TDFParser parser(ta3dSideData.gamedata_dir + "moveinfo.tdf");
		n = 0;
		while (!parser.pullAsString(String("CLASS") << n << ".name").empty())
			++n;

		for (int i = 0; i < unit_manager.nb_unit; ++i)
		{
			if (!unit_manager.unit_type[i]->MovementClass.empty())
			{
				for (int e = 0; e < n; ++e)
				{
					if (parser.pullAsString(String("CLASS") << e << ".name") == String::ToUpper(unit_manager.unit_type[i]->MovementClass))
					{
						unit_manager.unit_type[i]->FootprintX = byte(parser.pullAsInt(String("CLASS") << e << ".footprintx", unit_manager.unit_type[i]->FootprintX ));
						unit_manager.unit_type[i]->FootprintZ = byte(parser.pullAsInt(String("CLASS") << e << ".footprintz", unit_manager.unit_type[i]->FootprintZ ));
						unit_manager.unit_type[i]->MinWaterDepth = short(parser.pullAsInt(String("CLASS") << e << ".minwaterdepth", unit_manager.unit_type[i]->MinWaterDepth ));
						unit_manager.unit_type[i]->MaxWaterDepth = short(parser.pullAsInt(String("CLASS") << e << ".maxwaterdepth", unit_manager.unit_type[i]->MaxWaterDepth ));
						unit_manager.unit_type[i]->MaxSlope = short(parser.pullAsInt(String("CLASS") << e << ".maxslope", unit_manager.unit_type[i]->MaxSlope ));
						break;
					}
				}
			}
		}
		return nb_inconnu;
	}

	bool UnitType::canBuild(const int index) const
	{
		for (int i = 0; i < nb_unit; ++i)
		{
			if (BuildList[i] == index)
				return true;
		}
		return false;
	}


	UnitManager::UnitManager()
	{
		unit_hashtable.set_empty_key(String());
		h_dl_data.set_empty_key(String());
		name2gaf.set_empty_key(String());
		init();
	}

	void UnitManager::init()
	{
		nb_unit = 0;
		panel.init();
		paneltop.init();
		panelbottom.init();
	}

	UnitType::UnitType()
	{
		Category.set_empty_key(String());
		init();
	}
} // namespace TA3D

