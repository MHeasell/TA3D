#ifndef __TA3D_ENGINE_UNIT_H__
#define __TA3D_ENGINE_UNIT_H__

#include <stdafx.h>
#include <misc/string.h>
#include <threads/thread.h>
#include <misc/vector.h>
#include <ai/pathfinding.h>
#include <scripts/unit.script.interface.h>
#include "mission.h"
#include "weapondata.h"
#include "unit.defines.h"
#include <fbi.h>

namespace TA3D
{

	class Unit : public ObjectSync // Classe pour la gestion des unités	/ Class to store units's data
	{
	private:
		//! A structure to store all required information for rendering
		//! (copied from simulation data only once for all rendering steps)
		struct RenderData
		{
			Vector3D Pos;
			Vector3D Angle;
			AnimationData Anim;
			int px;
			int py;
			uint32 UID;
			int type_id;
		};

	public:
		//! \name Constructor & Destructor
		//@{
		//! Default constructor
		Unit();
		//! Destructor
		virtual ~Unit();
		//@}

		//! functions called from scripts (COB/BOS and Lua) (see unit.script.func module in scripts)
		void script_explode(int obj, int explosion_type);
		void script_turn_object(int obj, int axis, float angle, float speed);
		void script_move_object(int obj, int axis, float pos, float speed);
		int script_get_value_from_port(int portID, int* param = NULL);
		void script_spin_object(int obj, int axis, float target_speed, float accel);
		void script_show_object(int obj);
		void script_hide_object(int obj);
		void script_cache(int obj);
		void script_dont_cache(int obj);
		void script_shade(int obj);
		void script_dont_shade(int obj);
		void script_emit_sfx(int smoke_type, int from_piece);
		void script_stop_spin(int obj, int axis, float speed);
		void script_move_piece_now(int obj, int axis, float pos);
		void script_turn_piece_now(int obj, int axis, float angle);
		int script_get(int type, int v1, int v2);
		void script_set_value(int type, int v);
		void script_attach_unit(int unit_id, int piece_id);
		void script_drop_unit(int unit_id);
		bool script_is_turning(int obj, int axis);
		bool script_is_moving(int obj, int axis);

		int get_sweet_spot();

		float damage_modifier() const
		{
			return port[ARMORED] ? unit_manager.unit_type[typeId]->DamageModifier : 1.0f;
		}

		/**
		 * Returns true if this unit is owned by the given player,
		 * otherwise false.
		 */
		bool isOwnedBy(const PlayerId playerId) const;

		/**
		 * Returns true if this unit is not owned by the given player,
		 * otherwise false.
		 */
		bool isNotOwnedBy(const PlayerId playerId) const;

		/**
		 * Returns true if the unit is alive, otherwise false.
		 *
		 * I believe this should always be true in general
		 * (once a unit is dead it should be removed from play and destructed),
		 * however this requires further investigation.
		 */
		bool isAlive() const;

		/**
		 * Returns true if the unit with the given ID
		 * is an enemy of this unit.
		 * A unit is considered an enemy if it is not owned
		 * by either the player or one of their allies.
		 */
		bool isEnemy(const int t) const;

		/**
		 * Returns true if the unit is currently being built, otherwise false.
		 * A unit is being built if it is being nanolathed into existence
		 * by another unit.
		 */
		bool isBeingBuilt() const;

		/**
		 * Returns true if the unit can be selected by the given player, otherwise false.
		 * The unit may not be selectable at all (e.g. it is being built)
		 * or it may be not selectable by the specific player
		 * (e.g. because it is not owned by the player).
		 */
		bool isSelectableBy(PlayerId playerId) const;

		/**
		 * Sets the units position and updates any relevant cached data.
		 * Use this in preference to setting the position manually.
		 */
		void setPosition(const Vector3D& newPosition);

		void draw_on_map();
		void clear_from_map();

		void draw_on_FOW(bool jamming = false);

		bool is_on_radar(PlayerMask p_mask) const;

		void start_mission_script(int mission_type);

		void next_mission();

		void clear_mission();

		void add_mission(int mission_type, const Vector3D* target = NULL, bool step = false, int dat = 0,
			void* pointer = NULL, byte m_flags = 0,
			int move_data = 0, int patrol_node = -1);

		void set_mission(int mission_type, const Vector3D* target = NULL, bool step = false, int dat = 0,
			bool stopit = true, void* pointer = NULL, byte m_flags = 0,
			int move_data = 0);

		void compute_model_coord();

		void init_alloc_data();

		void toggle_self_destruct();

		void lock_command();

		void unlock_command();

		void init(int unit_type = -1, PlayerId owner = -1, bool full = false, bool basic = false);

		void clear_def_mission();

		void destroy(bool full = false);

		void draw(float t, bool height_line = true);

		void drawHealthBar() const;

		void draw_shadow(const Vector3D& Dir);

		void drawShadowBasic(const Vector3D& Dir);

		int launchScript(const int id, int nb_param = 0, int* param = NULL); // Start a script as a separate "thread" of the unit

		int runScriptFunction(const int id, int nb_param = 0, int* param = NULL); // Launch and run the script, returning it's values to param if not NULL

		void resetScript();

		bool playSound(const String& key);

		int move(const float dt, const int key_frame = 0);

		void computeHeadingBasedOnEnergy(Vector3D& dir, const bool moving);

		inline float getLocalMapEnergy(int x, int y);

		void followPath(const float dt, bool& b_TargetAngle, float& f_TargetAngle, Vector3D& NPos, int& n_px, int& n_py, bool& precomputed_position);

		void show_orders(bool only_build_commands = false, bool def_orders = false); // Dessine les ordres reçus

		void activate();

		void deactivate();

		int shoot(const int target, const Vector3D& startpos, const Vector3D& Dir, const int w_id, const Vector3D& target_pos);

		bool hit(const Vector3D& P, const Vector3D& Dir, Vector3D* hit_vec, const float length = 100.0f);

		bool hit_fast(const Vector3D& P, const Vector3D& Dir, Vector3D* hit_vec, const float length = 100.0f);

		void stopMoving();

		void explode();

		inline bool do_nothing() const
		{
			return missionQueue.doNothing() && !port[INBUILDSTANCE];
		}

		inline bool is_obstacle() const
		{
			return (!(missionQueue.doingNothing() && !port[INBUILDSTANCE]) && !(missionQueue->getFlags() & MISSION_FLAG_MOVE) && missionQueue->mission() != MISSION_MOVE && missionQueue->mission() != MISSION_BUILD && missionQueue->mission() != MISSION_REPAIR && missionQueue->mission() != MISSION_ATTACK && missionQueue->mission() != MISSION_CAPTURE && missionQueue->mission() != MISSION_RECLAIM && missionQueue->mission() != MISSION_REVIVE && missionQueue->mission() != MISSION_GUARD && missionQueue->mission() != MISSION_LOAD && missionQueue->mission() != MISSION_UNLOAD && missionQueue->mission() != MISSION_PATROL) || build_percent_left > 0.0f;
		}

		inline bool do_nothing_ai() const
		{
			return missionQueue.doNothingAI() && !port[INBUILDSTANCE];
		}

		void renderTick();

		void stopMovingAnimation();
		void startMovingAnimation();

		inline bool isVeteran() const { return false; }

	public:
		UnitScriptInterface::Ptr script; // Scripts concernant l'unité
		RenderData render;				 // Store render data in a sub object
		Model* model;					 // Modèle représentant l'objet

		//! ID of the player that owns this unit
		PlayerId ownerId;

		//! Type of the unit
		int typeId;

		//! The unit's remaining hit points
		float hp;

		Vector3D position;
		Vector3D velocity;
		Vector3D orientation;
		Vector3D angularVelocity;

		//! Indicates whether the unit is currently selected
		bool isSelected;
		AnimationData data;				 // Données pour l'animation de l'unité par le script
		volatile bool drawing;
		bool movingAnimation;
		bool requestedMovingAnimationState;
		sint16* port;			  // Ports
		MissionQueue missionQueue;	 // Orders given to the unit
		MissionQueue defaultMissionQueue; // Orders given to units built by this plant

		/**
		 * To indicate, among other things, to the unit manager if the unit exists.
		 *
		 * The following flags are valid:
		 *
		 * 0 -> the unit has been destroyed
		 * 1 -> the unit exists
		 * 2 -> unknown purpose ("unit detectable")
		 * 4 -> the unit is being killed
		 * 16 -> unknown purpose (seems to suppress explosion when killed, set when being reclaimed or captured)
		 * 64 -> the unit is landed (for planes)
		 */
		byte flags;

		//! The number of kills the unit has
		int kills;
		bool selfmove;			  // The unit has decided to move to a place with lower MAP::energy
		float lastEnergy;		  // Previous energy level at the unit position (if it changes then someone is getting closer, we can decide to move)
		float c_time;		   // Compteur de temps entre 2 émissions de particules par une unité de construction
		bool compute_coord;	// Indique s'il est nécessaire de recalculer les coordonnées du modèle 3d
		uint16 idx;			   // Indice dans le tableau d'unité
		uint32 ID;			   // the number of the unit (in total creation order) so we can identify it even if we move it :)
		float h;			   // Altitude (par rapport au sol)
		bool visible;		   // Indique si l'unité est visible / Tell if the unit is currently visible
		bool on_radar;		   // Radar drawing mode (icons)
		short groupe;		   // Indique si l'unité fait partie d'un groupe
		bool built;			   // Indique si l'unité est en cours de construction (par une autre unité)
		bool attacked;		   // Indique si l'unité est attaquée
		float planned_weapons; // Armes en construction / all is in the name
		uint32* memory;		   // Pour se rappeler sur quelles armes on a déjà tiré
		int mem_size;
		bool attached;
		short* attached_list;
		short* link_list;
		int nb_attached;
		bool just_created;
		bool first_move;
		int severity;

		/**
		 * The X index of the cell that contains this unit's position on the map's heightmap grid.
		 */
		int cur_px;

		/**
		 * The Y index of the cell that contains this unit's position on the map's heightmap grid.
		 */
		int cur_py;

		float metal_prod;
		float metal_cons;
		float energy_prod;
		float energy_cons;
		uint32 last_time_sound; // Remember last time it played a sound, so we don't get a unit SHOUTING for a simple move
		float cur_metal_prod;
		float cur_metal_cons;
		float cur_energy_prod;
		float cur_energy_cons;
		uint32 ripple_timer;
		WeaponData::Vector weapon;
		float death_delay;
		bool was_moving;
		float last_path_refresh;
		float shadow_scale_dir;
		bool hidden; // Used when unit is attached to another one but is hidden (i.e. transport ship)
		bool flying;
		bool cloaked;  // Is the unit cloaked
		bool cloaking; // Cloaking the unit if enough energy
		float paralyzed;
		float birthTime; // When was the unit created ?

		// Following variables are used to control the drawing of the unit on the presence maps
		bool drawn_open; // Used to store the last state the unit was drawn on the presence map (opened or closed)
		bool drawn_flying;
		bool drawn_obstacle; // Was the unit considered an obstacle
		sint32 drawn_x, drawn_y;
		bool drawn;

		// Following variables are used to control the drawing of FOW data
		uint32 sight;
		uint32 radar_range;
		uint32 sonar_range;
		uint32 radar_jam_range;
		uint32 sonar_jam_range;
		sint32 old_px;
		sint32 old_py;

		Vector3D move_target_computed;
		float was_locked;

		float self_destruct; // Count down for self-destruction
		float build_percent_left;
		float metal_extracted;

		bool requesting_pathfinder;
		uint16 pad1, pad2; // Repair pads currently used
		float pad_timer;   // Store last try to find a free landing pad

		bool command_locked; // Used for missions

		uint32 yardmap_timer; // To redraw the unit on yardmap periodically
		uint32 death_timer;   // To remove dead units

		// Following variables are used to control the synchronization of data between game clients
		uint32 sync_hash;
		uint32* last_synctick;
		bool local;
		bool exploding;
		struct sync previous_sync; // previous sync data
		sint32 nanolathe_target;
		bool nanolathe_reverse;
		bool nanolathe_feature;

		//! Indicates whether the unit should steer towards a target angle
		bool b_TargetAngle;
		//! The angle the unit should steer towards.
		float f_TargetAngle;

	private:
		void start_building(const Vector3D& dir);

	}; // class Unit

} // namespace TA3D

#endif // __TA3D_ENGINE_UNIT_H__
