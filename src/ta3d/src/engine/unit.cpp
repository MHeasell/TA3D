
#include "unit.h"
#include <UnitEngine.h>
#include <ingame/players.h>
#include <gfx/fx.manager.h>
#include <sounds/manager.h>
#include <input/mouse.h>

template <class T>
inline T SQUARE(T X) { return ((X) * (X)); }

namespace TA3D
{

	Unit::Unit()
		: script((UnitScriptInterface*)NULL),
		  render(),
		  model(NULL),
		  ownerId(0),
		  typeId(0),
		  hp(0.0f),
		  position(),
		  velocity(),
		  orientation(),
		  angularVelocity(),
		  isSelected(false),
		  data(),
		  drawing(false),
		  port(NULL),
		  missionQueue(),
		  defaultMissionQueue(),
		  flags(0),
		  kills(0),
		  selfmove(false),
		  lastEnergy(0.0f),
		  c_time(0),
		  compute_coord(false),
		  idx(0),
		  ID(0),
		  h(0.0f),
		  visible(false),
		  on_radar(false),
		  groupe(0),
		  built(0),
		  attacked(false),
		  planned_weapons(0.0f),
		  memory(NULL),
		  mem_size(0),
		  attached(false),
		  attached_list(NULL),
		  link_list(NULL),
		  nb_attached(0),
		  just_created(false),
		  first_move(false),
		  severity(0),
		  cur_px(0),
		  cur_py(0),
		  metal_prod(0.0f),
		  metal_cons(0.0f),
		  energy_prod(0.0f),
		  energy_cons(0.0f),
		  last_time_sound(0),
		  cur_metal_prod(0.0f),
		  cur_metal_cons(0.0f),
		  cur_energy_prod(0.0f),
		  cur_energy_cons(0.0f),
		  ripple_timer(0),
		  weapon(),
		  death_delay(0.0f),
		  was_moving(false),
		  last_path_refresh(0.0f),
		  shadow_scale_dir(0.0f),
		  hidden(false),
		  flying(false),
		  cloaked(false),
		  cloaking(false),
		  paralyzed(0.0f),
		  //
		  drawn_open(false),
		  drawn_flying(false),
		  drawn_x(0),
		  drawn_y(0),
		  drawn(false),
		  //
		  sight(0),
		  radar_range(0),
		  sonar_range(0),
		  radar_jam_range(0),
		  sonar_jam_range(0),
		  old_px(0),
		  old_py(0),
		  move_target_computed(),
		  was_locked(0.0f),
		  self_destruct(0.0f),
		  build_percent_left(0.0f),
		  metal_extracted(0.0f),
		  requesting_pathfinder(false),
		  pad1(0),
		  pad2(0),
		  pad_timer(0.0f),
		  command_locked(false),
		  yardmap_timer(0),
		  death_timer(0),
		  //
		  sync_hash(0),
		  last_synctick(NULL),
		  local(false),
		  exploding(false),
		  previous_sync(),
		  nanolathe_target(0),
		  nanolathe_reverse(false),
		  nanolathe_feature(false),
		  hasTargetAngle(false),
		  targetAngle(0.0f)
	{
	}

	Unit::~Unit()
	{
		DELETE_ARRAY(port);
		DELETE_ARRAY(memory);
		DELETE_ARRAY(attached_list);
		DELETE_ARRAY(link_list);
		DELETE_ARRAY(last_synctick);
	}

	void Unit::destroy(bool full)
	{
		while (drawing)
			rest(0);
		pMutex.lock();
		ID = 0;
		script = NULL;
		while (!missionQueue.empty())
			clear_mission();
		clear_def_mission();
		data.destroy();
		init();
		flags = 0;
		groupe = 0;
		pMutex.unlock();
		if (full)
		{
			DELETE_ARRAY(port);
			DELETE_ARRAY(memory);
			DELETE_ARRAY(attached_list);
			DELETE_ARRAY(link_list);
			DELETE_ARRAY(last_synctick);
		}
	}

	void Unit::start_building(const Vector3D& dir)
	{
		activate();
		// Work in model coordinates
		Vector3D Dir(dir * RotateXZY(-orientation.x * DEG2RAD, -orientation.z * DEG2RAD, -orientation.y * DEG2RAD));
		Vector3D P(Dir);
		P.y = 0.0f;
		float angle = acosf(P.z / P.length()) * RAD2DEG;
		if (P.x < 0.0f)
			angle = -angle;
		if (angle > 180)
			angle -= 360;
		else
		{
			if (angle < -180)
				angle += 360;
		}

		float angleX = asinf(Dir.y / Dir.length()) * RAD2DEG;
		if (angleX > 180)
			angleX -= 360;
		else if (angleX < -180)
			angleX += 360;
		int param[] = {(int)(angle * DEG2TA), (int)(angleX * DEG2TA)};
		launchScript(SCRIPT_startbuilding, 2, param);
		if (!(missionQueue->getFlags() & MISSION_FLAG_COMMAND_FIRED))
		{
			if (playSound("build"))
				missionQueue->Flags() |= MISSION_FLAG_COMMAND_FIRED;
		}
	}

	void Unit::start_mission_script(int mission_type)
	{
		if (script)
		{
			switch (mission_type)
			{
				case MISSION_STOP:
				case MISSION_STANDBY:
					if (port[INBUILDSTANCE])
					{
						launchScript(SCRIPT_stopbuilding);
						deactivate();
					}
					break;
				case MISSION_ATTACK:
					stopMoving();
					break;
				case MISSION_PATROL:
				case MISSION_MOVE:
					break;
				case MISSION_BUILD_2:
					break;
				case MISSION_RECLAIM:
					break;
			}
			if (mission_type != MISSION_STOP)
				// clear the "landed" flag (64)
				// (191 == 1011 1111)
				flags &= 191;
		}
	}

	void Unit::clear_mission()
	{
		if (!missionQueue)
			return;

		// Don't forget to detach the planes from air repair pads!
		if (missionQueue->mission() == MISSION_GET_REPAIRED && missionQueue->getTarget().isUnit())
		{
			Unit* target_unit = missionQueue->getUnit();
			target_unit->lock();
			if (target_unit->isAlive())
			{
				int piece_id = missionQueue->getData() >= 0 ? missionQueue->getData() : (-missionQueue->getData() - 1);
				if (target_unit->pad1 == piece_id) // tell others we've left
					target_unit->pad1 = 0xFFFF;
				else
					target_unit->pad2 = 0xFFFF;
			}
			target_unit->unlock();
		}
		pMutex.lock();
		missionQueue.next();
		pMutex.unlock();
	}

	void Unit::compute_model_coord()
	{
		if (!compute_coord || !model)
			return;
		pMutex.lock();
		compute_coord = false;
		if (typeId < 0 || !isAlive()) // The unit is dead
		{
			pMutex.unlock();
			return;
		}
		const UnitType* pType = unit_manager.unit_type[typeId];
		const float scale = pType->Scale;

		// Matrice pour le calcul des positions des éléments du modèle de l'unité
		//    M = RotateZ(Angle.z*DEG2RAD) * RotateY(Angle.y * DEG2RAD) * RotateX(Angle.x * DEG2RAD) * Scale(scale);
		Matrix M = RotateZYX(orientation.z * DEG2RAD, orientation.y * DEG2RAD, orientation.x * DEG2RAD) * Scale(scale);
		model->compute_coord(&data, &M);
		pMutex.unlock();
	}

	void Unit::init_alloc_data()
	{
		port = new sint16[21];						  // Ports
		memory = new uint32[TA3D_PLAYERS_HARD_LIMIT]; // Pour se rappeler sur quelles armes on a déjà tiré
		attached_list = new short[20];
		link_list = new short[20];
		last_synctick = new uint32[TA3D_PLAYERS_HARD_LIMIT];
	}

	void Unit::toggle_self_destruct()
	{
		const UnitType* pType = unit_manager.unit_type[typeId];
		if (self_destruct < 0.0f)
			self_destruct = pType->selfdestructcountdown;
		else
			self_destruct = -1.0f;
	}

	bool Unit::isEnemy(const int t) const
	{
		return t >= 0 && t < (int)units.max_unit && !(players.team[units.unit[t].ownerId] & players.team[ownerId]);
	}

	int Unit::runScriptFunction(const int id, int nb_param, int* param) // Launch and run the script, returning it's values to param if not NULL
	{
		const int type = typeId;
		if (!script || type == -1 || !unit_manager.unit_type[type]->script || !unit_manager.unit_type[type]->script->isCached(id))
			return -1;
		const String f_name(UnitScriptInterface::get_script_name(id));
		MutexLocker mLocker(pMutex);
		if (script)
		{
			const int result = script->execute(f_name, param, nb_param);
			if (result == -2)
			{
				unit_manager.unit_type[type]->script->Uncache(id);
				return -1;
			}
			return result;
		}
		return -1;
	}

	void Unit::resetScript()
	{
		pMutex.lock();
		pMutex.unlock();
	}

	void Unit::stopMoving()
	{
		if (missionQueue->getFlags() & MISSION_FLAG_MOVE)
		{
			unsetFlag(missionQueue->Flags(), MISSION_FLAG_MOVE);
			if (!missionQueue->Path().empty())
			{
				missionQueue->Path().clear();
				velocity.reset();
			}
			if (!(unit_manager.unit_type[typeId]->canfly && nb_attached > 0)) // Once charged with units the Atlas cannot land
				stopMovingAnimation();
			was_moving = false;
			if (!(missionQueue->Flags() & MISSION_FLAG_DONT_STOP_MOVE))
				velocity.reset(); // Stop unit's movement
		}
		else if (selfmove && !(unit_manager.unit_type[typeId]->canfly && nb_attached > 0))
			stopMovingAnimation();
		selfmove = false;
	}

	void Unit::stopMovingAnimation()
	{
		requestedMovingAnimationState = false;
	}

	void Unit::startMovingAnimation()
	{
		requestedMovingAnimationState = true;
	}

	void Unit::lock_command()
	{
		pMutex.lock();
		command_locked = true;
		pMutex.unlock();
	}

	void Unit::unlock_command()
	{
		pMutex.lock();
		command_locked = false;
		pMutex.unlock();
	}

	void Unit::activate()
	{
		pMutex.lock();
		if (port[ACTIVATION] == 0)
		{
			playSound("activate");
			launchScript(SCRIPT_Activate);
			port[ACTIVATION] = 1;
		}
		pMutex.unlock();
	}

	void Unit::deactivate()
	{
		pMutex.lock();
		if (port[ACTIVATION] != 0)
		{
			playSound("deactivate");
			launchScript(SCRIPT_Deactivate);
			port[ACTIVATION] = 0;
		}
		pMutex.unlock();
	}

	void Unit::init(int unit_type, PlayerId owner, bool full, bool basic)
	{
		pMutex.lock();

		birthTime = 0.0f;

		movingAnimation = false;

		kills = 0;
		selfmove = false;
		lastEnergy = 99999999.9f;

		ID = 0;
		paralyzed = 0.0f;

		yardmap_timer = 1;
		death_timer = 0;

		drawing = false;

		local = true; // Is local by default, set to remote by create_unit when needed

		nanolathe_target = -1; // Used for remote units only
		nanolathe_reverse = false;
		nanolathe_feature = false;

		exploding = false;

		command_locked = false;

		pad1 = 0xFFFF;
		pad2 = 0xFFFF;
		pad_timer = 0.0f;

		requesting_pathfinder = false;

		was_locked = 0.0f;

		metal_extracted = 0.0f;

		move_target_computed.x = move_target_computed.y = move_target_computed.z = 0.0f;

		self_destruct = -1; // Don't auto destruct!!

		drawn_open = drawn_flying = false;
		drawn_x = drawn_y = 0;
		drawn = false;
		drawn_obstacle = false;

		old_px = old_py = -10000;

		flying = false;

		cloaked = false;
		cloaking = false;

		hidden = false;
		shadow_scale_dir = -1.0f;
		last_path_refresh = 0.0f;
		metal_prod = metal_cons = energy_prod = energy_cons = cur_metal_prod = cur_metal_cons = cur_energy_prod = cur_energy_cons = 0.0f;
		last_time_sound = MILLISECONDS_SINCE_INIT;
		ripple_timer = MILLISECONDS_SINCE_INIT;
		was_moving = false;
		cur_px = 0;
		cur_py = 0;
		sight = 0;
		radar_range = 0;
		sonar_range = 0;
		radar_jam_range = 0;
		sonar_jam_range = 0;
		severity = 0;
		if (full)
			init_alloc_data();
		just_created = true;
		first_move = true;
		attached = false;
		nb_attached = 0;
		mem_size = 0;
		planned_weapons = 0.0f;
		attacked = false;
		groupe = 0;
		weapon.clear();
		h = 0.0f;
		compute_coord = true;
		c_time = 0.0f;
		flags = 1;
		isSelected = false;
		script = NULL;
		model = NULL;
		ownerId = owner;
		typeId = -1;
		hp = 0.0f;
		velocity.reset();
		position.reset();
		data.init();
		orientation.reset();
		angularVelocity = orientation;
		int i;
		for (i = 0; i < 21; ++i)
			port[i] = 0;
		if (unit_type < 0 || unit_type >= unit_manager.nb_unit)
			unit_type = -1;
		port[ACTIVATION] = 0;
		missionQueue.clear();
		defaultMissionQueue.clear();
		build_percent_left = 0.0f;
		memset(last_synctick, 0, 40);
		if (unit_type != -1)
		{
			typeId = unit_type;
			if (!basic)
			{
				pMutex.unlock();
				set_mission(MISSION_STANDBY);
				pMutex.lock();
			}
			const UnitType* const pType = unit_manager.unit_type[typeId];
			model = pType->model;
			weapon.resize(pType->weapon.size());
			hp = (float)pType->MaxDamage;
			script = UnitScriptInterface::Ptr(UnitScriptInterface::instanciate(pType->script));
			port[STANDINGMOVEORDERS] = pType->StandingMoveOrder;
			port[STANDINGFIREORDERS] = pType->StandingFireOrder;
			if (!basic)
			{
				pMutex.unlock();
				set_mission(pType->DefaultMissionType);
				pMutex.lock();
			}
			if (script)
			{
				script->setUnitID(idx);
				data.load(script->getNbPieces());
				launchScript(SCRIPT_create);
			}
		}
		pMutex.unlock();
	}

	void Unit::clear_def_mission()
	{
		pMutex.lock();
		defaultMissionQueue.clear();
		pMutex.unlock();
	}

	bool Unit::is_on_radar(PlayerMask p_mask) const
	{
		if (typeId == -1)
			return false;
		const UnitType* const pType = unit_manager.unit_type[typeId];
		const int px = cur_px >> 1;
		const int py = cur_py >> 1;
		gfx->lock();
		if (px >= 0 && py >= 0 && px < the_map->radar_map.getWidth() && py < the_map->radar_map.getHeight())
		{
			const bool r = ((the_map->radar_map(px, py) & p_mask) && !pType->Stealth && (pType->fastCategory & CATEGORY_NOTSUB)) || ((the_map->sonar_map(px, py) & p_mask) && !(pType->fastCategory & CATEGORY_NOTSUB));
			gfx->unlock();
			return r;
		}
		gfx->unlock();
		return false;
	}

	void Unit::add_mission(int mission_type, const Vector3D* target, bool step, int dat, void* pointer,
		byte m_flags, int move_data, int patrol_node)
	{
		MutexLocker locker(pMutex);

		if (command_locked && !(mission_type & MISSION_FLAG_AUTO))
			return;

		const UnitType* const pType = unit_manager.unit_type[typeId];
		mission_type &= ~MISSION_FLAG_AUTO;

		uint32 target_ID = 0;
		int targetIdx = -1;
		Mission::Target::Type targetType = Mission::Target::TargetNone;

		if (pointer != NULL)
		{
			switch (mission_type)
			{
				case MISSION_GET_REPAIRED:
				case MISSION_CAPTURE:
				case MISSION_LOAD:
				case MISSION_BUILD_2:
				case MISSION_RECLAIM:
				case MISSION_REPAIR:
				case MISSION_GUARD:
					target_ID = ((Unit*)pointer)->ID;
					targetType = Mission::Target::TargetUnit;
					targetIdx = ((Unit*)pointer)->idx;
					break;
				case MISSION_ATTACK:
					if (!(m_flags & MISSION_FLAG_TARGET_WEAPON))
					{
						target_ID = ((Unit*)pointer)->ID;
						targetIdx = ((Unit*)pointer)->idx;
						targetType = Mission::Target::TargetUnit;
					}
					else
					{
						targetIdx = ((Weapon*)pointer)->idx;
						targetType = Mission::Target::TargetWeapon;
					}
					break;
			}
		}
		else if (target)
			targetType = Mission::Target::TargetStatic;

		bool def_mode = false;
		if (typeId != -1 && !pType->BMcode)
		{
			switch (mission_type)
			{
				case MISSION_MOVE:
				case MISSION_PATROL:
				case MISSION_GUARD:
					def_mode = true;
					break;
			}
		}

		if (pointer == this && !def_mode) // A unit cannot target itself
			return;

		if (mission_type == MISSION_MOVE || mission_type == MISSION_PATROL)
			m_flags |= MISSION_FLAG_MOVE;

		if (typeId != -1 && mission_type == MISSION_BUILD && pType->BMcode && pType->Builder && target != NULL)
		{
			bool removed = false;
			MissionQueue::iterator cur = missionQueue.begin();
			if (!missionQueue.empty())
				++cur; // Don't read the first one ( which is being executed )

			while (cur != missionQueue.end()) // Reads the mission list
			{
				const float x_space = fabsf(cur->lastStep().getTarget().getPos().x - target->x);
				const float z_space = fabsf(cur->lastStep().getTarget().getPos().z - target->z);
				const int cur_data = cur->lastStep().getData();
				if (cur->lastMission() == MISSION_BUILD && cur_data >= 0 && cur_data < unit_manager.nb_unit && x_space < ((unit_manager.unit_type[dat]->FootprintX + unit_manager.unit_type[cur_data]->FootprintX) << 2) && z_space < ((unit_manager.unit_type[dat]->FootprintZ + unit_manager.unit_type[cur_data]->FootprintZ) << 2)) // Remove it
				{
					cur = missionQueue.erase(cur);
					removed = true;
				}
				else
					++cur;
			}
			if (removed)
				return;
		}

		Mission tmp;
		Mission& new_mission = step ? (def_mode ? defaultMissionQueue.front() : missionQueue.front()) : tmp;

		new_mission.addStep();
		new_mission.setMissionType((uint8)mission_type);
		new_mission.getTarget().set(targetType, targetIdx, target_ID);
		if (target)
			new_mission.getTarget().setPos(*target);
		new_mission.setTime(0.0f);
		new_mission.setData(dat);
		new_mission.Path().clear();
		new_mission.setLastD(9999999.0f);
		new_mission.setFlags(m_flags);
		new_mission.setMoveData(move_data);
		new_mission.setNode((uint16)patrol_node);

		if (!step && patrol_node == -1 && mission_type == MISSION_PATROL)
		{
			MissionQueue& mission_base = def_mode ? defaultMissionQueue : missionQueue;
			if (!mission_base.empty()) // Ajoute l'ordre aux autres
			{
				MissionQueue::iterator cur = mission_base.begin();
				MissionQueue::iterator last = mission_base.end();
				patrol_node = 0;
				while (cur != mission_base.end())
				{
					if (cur->mission() == MISSION_PATROL && patrol_node <= cur->getNode())
					{
						patrol_node = cur->getNode();
						last = cur;
					}
					++cur;
				}
				new_mission.setNode(uint16(patrol_node + 1));

				if (last != mission_base.end())
					++last;

				mission_base.insert(last, new_mission);
			}
			else
			{
				new_mission.setNode(1);
				mission_base.add(new_mission);
			}
		}
		else
		{
			bool stop = true;
			if (step)
			{
				switch (new_mission.lastMission())
				{
					case MISSION_MOVE:
					case MISSION_PATROL:
					case MISSION_STANDBY:
					case MISSION_VTOL_STANDBY:
					case MISSION_STOP: // Don't stop if it's not necessary
						stop = !(mission_type == MISSION_MOVE || mission_type == MISSION_PATROL);
						break;
					case MISSION_BUILD:
					case MISSION_BUILD_2:
						// Prevent factories from closing when already building a unit
						stop = mission_type == MISSION_BUILD && typeId != -1 && !pType->BMcode;
				};
			}
			else
				stop = pType->BMcode;
			if (stop)
			{
				new_mission.addStep();
				new_mission.setMissionType(MISSION_STOP);
				new_mission.setFlags(byte(m_flags & ~MISSION_FLAG_MOVE));
			}
			if (!step)
			{
				if (def_mode)
					defaultMissionQueue.add(new_mission);
				else
					missionQueue.add(new_mission);
			}
			else if (!def_mode)
				start_mission_script(missionQueue->mission());
		}
	}

	void Unit::set_mission(int mission_type, const Vector3D* target, bool /*step*/, int dat, bool stopit,
		void* pointer, byte m_flags, int move_data)
	{
		MutexLocker locker(pMutex);

		if (command_locked && !(mission_type & MISSION_FLAG_AUTO))
			return;
		const UnitType* pType = unit_manager.unit_type[typeId];
		mission_type &= ~MISSION_FLAG_AUTO;

		uint32 target_ID = 0;
		int targetIdx = -1;
		Mission::Target::Type targetType = Mission::Target::TargetNone;

		if (pointer != NULL)
		{
			switch (mission_type)
			{
				case MISSION_GET_REPAIRED:
				case MISSION_CAPTURE:
				case MISSION_LOAD:
				case MISSION_BUILD_2:
				case MISSION_RECLAIM:
				case MISSION_REPAIR:
				case MISSION_GUARD:
					target_ID = ((Unit*)pointer)->ID;
					targetIdx = ((Unit*)pointer)->idx;
					targetType = Mission::Target::TargetUnit;
					break;
				case MISSION_ATTACK:
					if (!(m_flags & MISSION_FLAG_TARGET_WEAPON))
					{
						target_ID = ((Unit*)pointer)->ID;
						targetIdx = ((Unit*)pointer)->idx;
						targetType = Mission::Target::TargetUnit;
					}
					else
					{
						targetIdx = ((Weapon*)pointer)->idx;
						targetType = Mission::Target::TargetWeapon;
					}
					break;
			}
		}
		else if (target)
			targetType = Mission::Target::TargetStatic;

		if (nanolathe_target >= 0 && network_manager.isConnected())
		{
			nanolathe_target = -1;
			g_ta3d_network->sendUnitNanolatheEvent(idx, -1, false, false);
		}

		bool def_mode = false;
		if (typeId != -1 && !pType->BMcode)
		{
			switch (mission_type)
			{
				case MISSION_MOVE:
				case MISSION_PATROL:
				case MISSION_GUARD:
					def_mode = true;
					break;
			}
		}

		if (pointer == this && !def_mode) // A unit cannot target itself
			return;

		int old_mission = -1;
		if (!def_mode)
		{
			if (!missionQueue.empty())
				old_mission = missionQueue->mission();
		}
		else
			clear_def_mission();

		bool already_running = false;

		if (mission_type == MISSION_MOVE || mission_type == MISSION_PATROL)
			m_flags |= MISSION_FLAG_MOVE;

		switch (old_mission) // Commandes de fin de mission
		{
			case MISSION_REPAIR:
			case MISSION_REVIVE:
			case MISSION_RECLAIM:
			case MISSION_CAPTURE:
			case MISSION_BUILD_2:
				deactivate();
				launchScript(SCRIPT_stopbuilding);
				if (typeId != -1 && !pType->BMcode) // Delete the unit we were building
				{
					sint32 prev = -1;
					for (int i = units.nb_unit - 1; i >= 0; --i)
					{
						if (units.idx_list[i] == missionQueue->getUnit()->idx)
						{
							prev = i;
							break;
						}
					}
					if (prev >= 0)
						units.kill(missionQueue->getUnit()->idx, prev);
				}
				break;
			case MISSION_ATTACK:
				if (mission_type != MISSION_ATTACK && typeId != -1 && (!pType->canfly || (pType->canfly && mission_type != MISSION_MOVE && mission_type != MISSION_PATROL)))
					deactivate();
				else
				{
					stopit = false;
					already_running = true;
				}
				break;
			case MISSION_MOVE:
			case MISSION_PATROL:
				if (mission_type == MISSION_MOVE || mission_type == MISSION_PATROL || (typeId != -1 && pType->canfly && mission_type == MISSION_ATTACK))
				{
					stopit = false;
					already_running = true;
				}
				break;
		};

		if (!def_mode)
		{
			while (!missionQueue.empty())
				clear_mission(); // Efface les ordres précédents
			last_path_refresh = 10.0f;
			requesting_pathfinder = false;
			if (nanolathe_target >= 0 && network_manager.isConnected()) // Stop nanolathing
			{
				nanolathe_target = -1;
				g_ta3d_network->sendUnitNanolatheEvent(idx, -1, false, false);
			}
		}

		if (def_mode)
		{
			defaultMissionQueue.clear();
			defaultMissionQueue.add();

			defaultMissionQueue->setMissionType((uint8)mission_type);
			defaultMissionQueue->getTarget().set(targetType, targetIdx, target_ID);
			defaultMissionQueue->setData(dat);
			defaultMissionQueue->Path().clear();
			defaultMissionQueue->setLastD(9999999.0f);
			defaultMissionQueue->setFlags(m_flags);
			defaultMissionQueue->setMoveData(move_data);
			defaultMissionQueue->setNode(1);
			if (target)
				defaultMissionQueue->getTarget().setPos(*target);

			if (stopit)
			{
				defaultMissionQueue->addStep();
				defaultMissionQueue->setMissionType(MISSION_STOP);
				defaultMissionQueue->setData(0);
				defaultMissionQueue->Path().clear();
				defaultMissionQueue->setFlags(uint8(m_flags & ~MISSION_FLAG_MOVE));
			}
		}
		else
		{
			missionQueue.clear();
			missionQueue.add();

			missionQueue->setMissionType(uint8(mission_type));
			missionQueue->getTarget().set(targetType, targetIdx, target_ID);
			missionQueue->setData(dat);
			missionQueue->Path().clear();
			missionQueue->setLastD(9999999.0f);
			missionQueue->setFlags(m_flags);
			missionQueue->setMoveData(move_data);
			missionQueue->setNode(1);
			if (target)
				missionQueue->getTarget().setPos(*target);

			if (stopit)
			{
				missionQueue->addStep();
				missionQueue->setMissionType(MISSION_STOP);
				missionQueue->setData(0);
				missionQueue->Path().clear();
				missionQueue->setLastD(9999999.0f);
				missionQueue->setFlags(m_flags | MISSION_FLAG_MOVE);
				missionQueue->setMoveData(move_data);
			}
			else
			{
				if (!already_running)
					start_mission_script(missionQueue->mission());
			}
			c_time = 0.0f;
		}
	}

	void Unit::next_mission()
	{
		UnitType* pType = typeId != -1 ? unit_manager.unit_type[typeId] : NULL;
		last_path_refresh = 10.0f; // By default allow to compute a new path
		requesting_pathfinder = false;
		if (nanolathe_target >= 0 && network_manager.isConnected())
		{
			nanolathe_target = -1;
			g_ta3d_network->sendUnitNanolatheEvent(idx, -1, false, false);
		}

		if (!missionQueue)
		{
			command_locked = false;
			if (typeId != -1)
				set_mission(pType->DefaultMissionType, NULL, false, 0, false);
			return;
		}
		switch (missionQueue->mission()) // Commandes de fin de mission
		{
			case MISSION_REPAIR:
			case MISSION_RECLAIM:
			case MISSION_BUILD_2:
			case MISSION_REVIVE:
			case MISSION_CAPTURE:
				if (!missionQueue.hasNext() || (pType && pType->BMcode) || missionQueue[1].lastMission() != MISSION_BUILD)
				{
					launchScript(SCRIPT_stopbuilding);
					deactivate();
				}
				break;
			case MISSION_ATTACK:
				deactivate();
				break;
		}
		if (missionQueue->mission() == MISSION_STOP && !missionQueue.hasNext())
		{
			command_locked = false;
			missionQueue->setData(0);
			return;
		}
		bool old_step = missionQueue->isStep();
		missionQueue.next();
		if (!missionQueue)
		{
			command_locked = false;
			if (typeId != -1)
				set_mission(pType->DefaultMissionType);
		}

		// Skip a stop order before a normal order if the unit can fly (prevent planes from looking for a place to land when they don't need to land !!)
		if (pType != NULL && pType->canfly && missionQueue->mission() == MISSION_STOP && missionQueue.hasNext() && missionQueue(1) != MISSION_STOP)
		{
			missionQueue.next();
		}

		if (old_step && !missionQueue.empty() && missionQueue.hasNext() && (missionQueue->mission() == MISSION_STOP || missionQueue->mission() == MISSION_VTOL_STANDBY || missionQueue->mission() == MISSION_STANDBY))
			next_mission();

		start_mission_script(missionQueue->mission());
		c_time = 0.0f;
	}

	void Unit::draw(float t, bool height_line)
	{
		MutexLocker locker(pMutex);

		if (!isAlive() || typeId == -1 || ID != render.UID || !visible)
			return;

		UnitType* const pType = unit_manager.unit_type[typeId];
		visible = false;
		on_radar = false;

		if (!model || hidden)
			return; // If there is no associated model, skip drawing

		const int px = render.px >> 1;
		const int py = render.py >> 1;
		if (px < 0 || py < 0 || px >= the_map->widthInGraphicalTiles || py >= the_map->heightInGraphicalTiles)
			return; // Unit is outside the map
		const PlayerMask player_mask = toPlayerMask(players.local_human_id);

		on_radar = is_on_radar(player_mask);
		if (the_map->view(px, py) == 0 || (the_map->view(px, py) > 1 && !on_radar))
			return; // Unit is not visible

		if (!on_radar)
		{
			gfx->lock();
			if (!(the_map->sight_map(px, py) & player_mask))
			{
				gfx->unlock();
				return;
			}
			gfx->unlock();
		}

		const bool radar_detected = on_radar;

		on_radar &= the_map->view(px, py) > 1;

		// don't draw the object if it is not in the camera view
		if (!Camera::inGame->viewportContains(render.Pos))
		{
			return;
		}

		Vector3D zeroVector(0.0f, 0.0f, 0.0f);

		if (!cloaked || isOwnedBy(players.local_human_id)) // Don't show cloaked units
		{
			visible = true;
		}
		else
		{
			on_radar |= radar_detected;
			visible = on_radar;
		}

		Matrix M;
		glPushMatrix();
		if (!on_radar && visible && model)
		{
			// Set time origin to our birth date
			t -= birthTime;
			glTranslatef(render.Pos.x, render.Pos.y, render.Pos.z);

			glRotatef(render.Angle.x, 1.0f, 0.0f, 0.0f);
			glRotatef(render.Angle.z, 0.0f, 0.0f, 1.0f);
			glRotatef(render.Angle.y, 0.0f, 1.0f, 0.0f);
			const float scale = pType->Scale;
			glScalef(scale, scale, scale);

			//            M=RotateY(Angle.y*DEG2RAD)*RotateZ(Angle.z*DEG2RAD)*RotateX(Angle.x*DEG2RAD)*Scale(scale);			// Matrice pour le calcul des positions des éléments du modèle de l'unité
			M = RotateYZX(render.Angle.y * DEG2RAD,
					render.Angle.z * DEG2RAD,
					render.Angle.x * DEG2RAD)
				* Scale(scale); // Matrice pour le calcul des positions des éléments du modèle de l'unité

			const Vector3D *target = NULL, *center = NULL;
			Vector3D upos;
			bool c_part = false;
			bool reverse = false;
			float size = 0.0f;
			Mesh* src = NULL;
			AnimationData* src_data = NULL;
			Vector3D v_target; // Needed in network mode
			Unit* unit_target = NULL;
			Model* const the_model = model;
			drawing = true;

			if (!pType->emitting_points_computed) // Compute model emitting points if not already done, do it here in Unit::Locked code ...
			{
				pType->emitting_points_computed = true;
				int first = runScriptFunction(SCRIPT_QueryNanoPiece);
				int current;
				int i = 0;
				do
				{
					current = runScriptFunction(SCRIPT_QueryNanoPiece);
					if (model)
						model->mesh->compute_emitter_point(current);
					++i;
				} while (first != current && i < 1000);
			}

			if (!isBeingBuilt() && !missionQueue.empty() && port[INBUILDSTANCE] != 0 && local)
			{
				if (c_time >= 0.125f)
				{
					reverse = (missionQueue->mission() == MISSION_RECLAIM);
					c_time = 0.0f;
					c_part = true;
					upos.x = upos.y = upos.z = 0.0f;
					upos = upos + render.Pos;
					if (missionQueue->getTarget().isUnit() && (missionQueue->mission() == MISSION_REPAIR || missionQueue->mission() == MISSION_BUILD || missionQueue->mission() == MISSION_BUILD_2 || missionQueue->mission() == MISSION_CAPTURE || missionQueue->mission() == MISSION_RECLAIM))
					{
						unit_target = missionQueue->getUnit();
						if (unit_target)
						{
							pMutex.unlock();
							unit_target->lock();
							if (unit_target->isAlive() && unit_target->model != NULL)
							{
								size = unit_target->model->size2;
								center = &(unit_target->model->center);
								src = unit_target->model->mesh;
								src_data = &(unit_target->data);
								unit_target->compute_model_coord();
							}
							else
							{
								unit_target->unlock();
								pMutex.lock();
								unit_target = NULL;
								c_part = false;
							}
						}
						else
						{
							unit_target = NULL;
							c_part = false;
						}
					}
					else
					{
						if (missionQueue->mission() == MISSION_RECLAIM || missionQueue->mission() == MISSION_REVIVE) // Reclaiming features
						{
							const int feature_type = features.feature[missionQueue->getData()].type;
							const Feature* const feature = feature_manager.getFeaturePointer(feature_type);
							if (missionQueue->getData() >= 0 && feature && feature->model)
							{
								size = feature->model->size2;
								center = &(feature->model->center);
								src = feature->model->mesh;
								src_data = NULL;
							}
							else
							{
								center = &zeroVector;
								size = 32.0f;
							}
						}
						else
							c_part = false;
					}
					target = &(missionQueue->getTarget().getPos());
				}
			}
			else
			{
				if (!local && nanolathe_target >= 0 && port[INBUILDSTANCE] != 0)
				{
					if (c_time >= 0.125f)
					{
						reverse = nanolathe_reverse;
						c_time = 0.0f;
						c_part = true;
						upos.reset();
						upos = upos + render.Pos;
						if (!nanolathe_feature)
						{
							unit_target = &(units.unit[nanolathe_target]);
							pMutex.unlock();
							unit_target->lock();
							if (unit_target->isAlive() && unit_target->model)
							{
								size = unit_target->model->size2;
								center = &(unit_target->model->center);
								src = unit_target->model->mesh;
								src_data = &(unit_target->data);
								unit_target->compute_model_coord();
								v_target = unit_target->position;
							}
							else
							{
								unit_target->unlock();
								pMutex.lock();
								unit_target = NULL;
								c_part = false;
							}
						}
						else // Reclaiming features
						{
							const int feature_type = features.feature[nanolathe_target].type;
							v_target = features.feature[nanolathe_target].Pos;
							const Feature* const feature = feature_manager.getFeaturePointer(feature_type);
							if (feature && feature->model)
							{
								size = feature->model->size2;
								center = &(feature->model->center);
								src = feature->model->mesh;
								src_data = NULL;
							}
							else
							{
								center = &zeroVector;
								size = 32.0f;
							}
						}
						target = &v_target;
					}
				}
			}

			glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);

			if (!isBeingBuilt())
			{
				if (pType->onoffable && !port[ACTIVATION])
					t = 0.0f;
				if (cloaked || (cloaking && isNotOwnedBy(players.local_human_id)))
					glColor4ub(0xFF, 0xFF, 0xFF, 0x7F);
				glDisable(GL_CULL_FACE);
				the_model->draw(t, &render.Anim, isOwnedBy(players.local_human_id) && isSelected, false, c_part, build_part, target, &upos, &M, size, center, reverse, ownerId, cloaked, src, src_data);
				glEnable(GL_CULL_FACE);
				if (cloaked || (cloaking && isNotOwnedBy(players.local_human_id)))
					gfx->set_color(0xFFFFFFFF);
				if (height_line && h > 1.0f && pType->canfly) // For flying units, draw a line that shows how high is the unit
				{
					glPopMatrix();
					glPushMatrix();
					glDisable(GL_TEXTURE_2D);
					glDisable(GL_LIGHTING);
					glColor3ub(0xFF, 0xFF, 0);
					glBegin(GL_LINES);
					for (float y = render.Pos.y; y > render.Pos.y - h; y -= 10.0f)
					{
						glVertex3f(render.Pos.x, y, render.Pos.z);
						glVertex3f(render.Pos.x, y - 5.0f, render.Pos.z);
					}
					glEnd();
				}
			}
			else
			{
				if (build_percent_left <= 33.0f)
				{
					float h = model->top - model->bottom;
					double eqn[4] = {0.0f, 1.0f, 0.0f, -model->bottom - h * (33.0f - build_percent_left) * 0.033333f};

					glClipPlane(GL_CLIP_PLANE0, eqn);
					glEnable(GL_CLIP_PLANE0);
					the_model->draw(t, &render.Anim, isOwnedBy(players.local_human_id) && isSelected, true, c_part, build_part, target, &upos, &M, size, center, reverse, ownerId, true, src, src_data);

					eqn[1] = -eqn[1];
					eqn[3] = -eqn[3];
					glClipPlane(GL_CLIP_PLANE0, eqn);
					the_model->draw(t, &render.Anim, isOwnedBy(players.local_human_id) && isSelected, false, false, build_part, target, &upos, &M, size, center, reverse, ownerId);
					glDisable(GL_CLIP_PLANE0);

					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					the_model->draw(t, &render.Anim, isOwnedBy(players.local_human_id) && isSelected, true, false, build_part, target, &upos, &M, size, center, reverse, ownerId);
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				}
				else
				{
					if (build_percent_left <= 66.0f)
					{
						float h = model->top - model->bottom;
						double eqn[4] = {0.0f, 1.0f, 0.0f, -model->bottom - h * (66.0f - build_percent_left) * 0.033333f};

						glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
						glClipPlane(GL_CLIP_PLANE0, eqn);
						glEnable(GL_CLIP_PLANE0);
						glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
						the_model->draw(t, &render.Anim, isOwnedBy(players.local_human_id) && isSelected, true, c_part, build_part, target, &upos, &M, size, center, reverse, ownerId, true, src, src_data);
						glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

						eqn[1] = -eqn[1];
						eqn[3] = -eqn[3];
						glClipPlane(GL_CLIP_PLANE0, eqn);
						the_model->draw(t, &render.Anim, isOwnedBy(players.local_human_id) && isSelected, true, false, build_part, target, &upos, &M, size, center, reverse, ownerId);
						glDisable(GL_CLIP_PLANE0);
					}
					else
					{
						glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
						the_model->draw(t, &render.Anim, isOwnedBy(players.local_human_id) && isSelected, true, false, build_part, target, &upos, &M, size, center, reverse, ownerId);
						glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					}
				}
			}

			if (unit_target)
			{
				unit_target->unlock();
				pMutex.lock();
			}
		}
		drawing = false;
		glPopMatrix();
	}

	void Unit::draw_shadow(const Vector3D& Dir)
	{
		pMutex.lock();
		if (!isAlive() || ID != render.UID)
		{
			pMutex.unlock();
			return;
		}

		if (on_radar || hidden)
		{
			pMutex.unlock();
			return;
		}

		if (!model)
		{
			LOG_WARNING("Model is NULL ! (" << __FILE__ << ":" << __LINE__ << ")");
			pMutex.unlock();
			return;
		}

		if (cloaked && isNotOwnedBy(players.local_human_id)) // Unit is cloaked
		{
			pMutex.unlock();
			return;
		}

		if (!visible)
		{
			const Vector3D S_Pos = render.Pos - (h / Dir.y) * Dir; //the_map->hit(position,Dir);
			auto tileIndex = the_map->worldToGraphicalTileIndex(S_Pos);
			const int px = tileIndex.x;
			const int py = tileIndex.y;
			if (px < 0 || py < 0 || px >= the_map->widthInGraphicalTiles || py >= the_map->heightInGraphicalTiles)
			{
				pMutex.unlock();
				return; // Shadow out of the map
			}
			if (the_map->view(px, py) != 1)
			{
				pMutex.unlock();
				return; // Unvisible shadow
			}
		}

		const UnitType* const pType = unit_manager.unit_type[typeId];
		drawing = true; // Prevent the model to be set to NULL and the data structure from being reset
		pMutex.unlock();

		glPushMatrix();
		glTranslatef(render.Pos.x, render.Pos.y, render.Pos.z);
		glRotatef(render.Angle.x, 1.0f, 0.0f, 0.0f);
		glRotatef(render.Angle.z, 0.0f, 0.0f, 1.0f);
		glRotatef(render.Angle.y, 0.0f, 1.0f, 0.0f);
		const float scale = pType->Scale;
		glScalef(scale, scale, scale);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if ((typeId != -1 && pType->canmove) || shadow_scale_dir < 0.0f)
		{
			Vector3D H = render.Pos;
			H.y += 2.0f * model->size2 + 1.0f;
			const Vector3D D = the_map->hit(H, Dir, true, 2000.0f);
			shadow_scale_dir = (D - H).length();
		}
		model->draw_shadow(shadow_scale_dir * Dir * RotateXZY(-render.Angle.x * DEG2RAD, -render.Angle.z * DEG2RAD, -render.Angle.y * DEG2RAD), 0.0f, &render.Anim);

		glPopMatrix();

		drawing = false;
	}

	void Unit::drawShadowBasic(const Vector3D& Dir)
	{
		pMutex.lock();
		if (!isAlive() || ID != render.UID)
		{
			pMutex.unlock();
			return;
		}
		if (on_radar || hidden)
		{
			pMutex.unlock();
			return;
		}

		if (!model)
		{
			LOG_WARNING("Model is NULL ! (" << __FILE__ << ":" << __LINE__ << ")");
			pMutex.unlock();
			return;
		}

		if (cloaked && isNotOwnedBy(players.local_human_id)) // Unit is cloaked
		{
			pMutex.unlock();
			return;
		}

		if (!visible)
		{
			const Vector3D S_Pos(render.Pos - (h / Dir.y) * Dir); //the_map->hit(position,Dir);
			auto tileIndex = the_map->worldToGraphicalTileIndex(S_Pos);
			const int px = tileIndex.x;
			const int py = tileIndex.y;
			if (px < 0 || py < 0 || px >= the_map->widthInGraphicalTiles || py >= the_map->heightInGraphicalTiles)
			{
				pMutex.unlock();
				return; // Shadow out of the map
			}
			if (the_map->view(px, py) != 1)
			{
				pMutex.unlock();
				return; // Unvisible shadow
			}
		}
		const UnitType* const pType = unit_manager.unit_type[typeId];
		drawing = true; // Prevent the model to be set to NULL and the data structure from being reset
		pMutex.unlock();

		glPushMatrix();
		glTranslatef(render.Pos.x, render.Pos.y, render.Pos.z);
		glRotatef(render.Angle.x, 1.0f, 0.0f, 0.0f);
		glRotatef(render.Angle.z, 0.0f, 0.0f, 1.0f);
		glRotatef(render.Angle.y, 0.0f, 1.0f, 0.0f);
		const float scale = pType->Scale;
		glScalef(scale, scale, scale);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		if (pType->canmove || shadow_scale_dir < 0.0f)
		{
			Vector3D H = render.Pos;
			H.y += 2.0f * model->size2 + 1.0f;
			const Vector3D D = the_map->hit(H, Dir, true, 2000.0f);
			shadow_scale_dir = (D - H).length();
		}
		model->draw_shadow_basic(shadow_scale_dir * Dir * RotateXZY(-render.Angle.x * DEG2RAD, -render.Angle.z * DEG2RAD, -render.Angle.y * DEG2RAD), 0.0f, &render.Anim);

		glPopMatrix();

		drawing = false;
	}

	void Unit::explode()
	{
		exploding = true;
		const UnitType* const pType = unit_manager.unit_type[typeId];
		if (local && network_manager.isConnected()) // Sync unit destruction (and corpse creation ;) )
		{
			struct event explode_event;
			explode_event.type = EVENT_UNIT_EXPLODE;
			explode_event.opt1 = idx;
			explode_event.opt2 = (uint16)severity;
			explode_event.x = position.x;
			explode_event.y = position.y;
			explode_event.z = position.z;
			network_manager.sendEvent(&explode_event);
		}

		const int power = Math::Max(pType->FootprintX, pType->FootprintZ);
		fx_manager.addFlash(position, float(power * 32));

		int param[] = {severity * 100 / pType->MaxDamage, 0};
		int corpse_type = runScriptFunction(SCRIPT_killed, 2, param);
		if (attached)
			corpse_type = 3; // When we were flying we just disappear
		const bool sinking = the_map->get_unit_h(position.x, position.z) <= the_map->sealvl;

		switch (corpse_type)
		{
			case 1: // Some good looking corpse
			{
				pMutex.unlock();
				flags = 1; // Set it to 1 otherwise it won't remove it from map
				clear_from_map();
				flags = 4;
				pMutex.lock();
				if (cur_px > 0 && cur_py > 0 && cur_px < the_map->widthInHeightmapTiles && cur_py < the_map->heightInHeightmapTiles)
					if (the_map->map_data(cur_px, cur_py).stuff == -1)
					{
						int type = feature_manager.get_feature_index(pType->Corpse);
						if (type >= 0)
						{
							the_map->map_data(cur_px, cur_py).stuff = features.add_feature(position, type);
							if (the_map->map_data(cur_px, cur_py).stuff >= 0) // Keep unit orientation
							{
								features.feature[the_map->map_data(cur_px, cur_py).stuff].angle = orientation.y;
								if (sinking)
									features.sink_feature(the_map->map_data(cur_px, cur_py).stuff);
								features.drawFeatureOnMap(the_map->map_data(cur_px, cur_py).stuff);
							}
						}
					}
			}
			break;
			case 2: // Some exploded corpse
			{
				pMutex.unlock();
				flags = 1; // Set it to 1 otherwise it won't remove it from map
				clear_from_map();
				flags = 4;
				pMutex.lock();
				if (cur_px > 0 && cur_py > 0 && cur_px < the_map->widthInHeightmapTiles && cur_py < the_map->heightInHeightmapTiles)
					if (the_map->map_data(cur_px, cur_py).stuff == -1)
					{
						int type = feature_manager.get_feature_index(String(pType->name) << "_heap");
						if (type >= 0)
						{
							the_map->map_data(cur_px, cur_py).stuff = features.add_feature(position, type);
							if (the_map->map_data(cur_px, cur_py).stuff >= 0) // Keep unit orientation
							{
								features.feature[the_map->map_data(cur_px, cur_py).stuff].angle = orientation.y;
								if (sinking)
									features.sink_feature(the_map->map_data(cur_px, cur_py).stuff);
								features.drawFeatureOnMap(the_map->map_data(cur_px, cur_py).stuff);
							}
						}
					}
			}
			break;
			default:
				flags = 1; // Nothing replaced just remove the unit from position map
				pMutex.unlock();
				clear_from_map();
				pMutex.lock();
		}
		pMutex.unlock();
		const int w_id = weapons.add_weapon(weapon_manager.get_weapon_index(Math::AlmostZero(self_destruct) ? pType->SelfDestructAs : pType->ExplodeAs), idx);
		pMutex.lock();
		if (w_id >= 0)
		{
			weapons.weapon[w_id].Pos = position;
			weapons.weapon[w_id].target_pos = position;
			weapons.weapon[w_id].target = -1;
			weapons.weapon[w_id].just_explode = true;
		}
		for (int i = 0; i < data.nb_piece; ++i)
		{
			if (!(data.data[i].flag & FLAG_EXPLODE)) // || (data.flag[i]&FLAG_EXPLODE && (data.explosion_flag[i]&EXPLODE_BITMAPONLY)))
				data.data[i].flag |= FLAG_HIDE;
		}
	}

	//! Ballistic calculations take place here
	float ballistic_angle(float v, float g, float d, float y_s, float y_e) // Calculs de ballistique pour l'angle de tir
	{
		const float v2 = v * v;
		const float gd = g * d;
		const float v2gd = v2 / gd;
		const float a = v2gd * (4.0f * v2gd - 8.0f * (y_e - y_s) / d) - 4.0f;
		if (a < 0.0f) // Pas de solution
			return 360.0f;
		return RAD2DEG * atanf(v2gd - 0.5f * sqrtf(a));
	}

	//! Compute the local map energy WITHOUT current unit contribution (it assumes (x,y) is on pType->gRepulsion)
	float Unit::getLocalMapEnergy(int x, int y)
	{
		if (x < 0 || y < 0 || x >= the_map->widthInHeightmapTiles || y >= the_map->heightInHeightmapTiles)
			return 999999999.0f;
		float e = the_map->energy(x, y);
		const UnitType* pType = unit_manager.unit_type[typeId];
		e -= pType->gRepulsion(x - cur_px + (pType->gRepulsion.getWidth() >> 1),
			y - cur_py + (pType->gRepulsion.getHeight() >> 1));
		return e;
	}

	//! Compute the dir vector based on MAP::energy and targeting
	void Unit::computeHeadingBasedOnEnergy(Vector3D& dir, const bool moving)
	{
		static const int order_dx[] = {-1, 0, 1, 1, 1, 0, -1, -1};
		static const int order_dz[] = {-1, -1, -1, 0, 1, 1, 1, 0};
		int b = -1;
		auto heightmapIndex = the_map->worldToHeightmapIndex(dir);
		const int x = heightmapIndex.x;
		const int z = heightmapIndex.y;
		float E = getLocalMapEnergy(cur_px, cur_py);
		const UnitType* pType = unit_manager.unit_type[typeId];
		if (moving)
		{
			const float dist = sqrtf(float(SQUARE(cur_px - x) + SQUARE(cur_py - z)));
			if (dist < 64.0f)
				E *= dist * 0.015625f;
			E += 80.0f * float(pType->MaxSlope) * dist;
		}
		for (int i = 0; i < 8; ++i)
		{
			float e = getLocalMapEnergy(cur_px + order_dx[i], cur_py + order_dz[i]);
			if (moving)
			{
				const float dist = sqrtf(float(SQUARE(cur_px + order_dx[i] - x) + SQUARE(cur_py + order_dz[i] - z)));
				if (dist < 64.0f)
					e *= dist * 0.015625f;
				e += 80.0f * float(pType->MaxSlope) * dist;
			}
			if (e < E)
			{
				E = e;
				b = i;
			}
		}
		if (b == -1)
			dir.reset();
		else
		{
			dir = Vector3D((float)order_dx[b], 0.0f, (float)order_dz[b]);
			dir.normalize();
		}
	}

	//! Send a request to the pathfinder when we need a complex path, then follow computed paths
	void Unit::followPath(const float dt, bool& b_TargetAngle, float& f_TargetAngle, Vector3D& NPos, int& n_px, int& n_py, bool& precomputed_position)
	{
		// Don't control remote units
		if (!local)
			return;

		const UnitType* pType = unit_manager.unit_type[typeId];
		//----------------------------------- Beginning of moving code ------------------------------------

		if (pType->canmove && pType->BMcode && (!pType->canfly || (missionQueue->getFlags() & MISSION_FLAG_MOVE)))
		{
			Vector3D J, I, K(0.0f, 1.0f, 0.0f);
			Vector3D Target(missionQueue->getTarget().getPos());
			if (missionQueue->getFlags() & MISSION_FLAG_MOVE)
			{
				if (selfmove)
				{
					selfmove = false;
					if (was_moving)
					{
						velocity.reset();
						if (!(pType->canfly && nb_attached > 0)) // Once charged with units the Atlas cannot land
							stopMovingAnimation();
					}
					was_moving = false;
					requesting_pathfinder = false;
				}
				if (missionQueue.mission() == MISSION_STOP) // Special mission type : MISSION_STOP
				{
					stopMoving();
					return;
				}
				else
				{
					if (!missionQueue->Path().empty() && (!(missionQueue->getFlags() & MISSION_FLAG_REFRESH_PATH) || (last_path_refresh < 5.0f && !pType->canfly && (missionQueue->getFlags() & MISSION_FLAG_REFRESH_PATH))))
						Target = missionQueue->Path().Pos();
					else
					{								  // Look for a path to the target
						if (!missionQueue->Path().empty()) // If we want to refresh the path
						{
							Target = missionQueue->getTarget().getPos();
							missionQueue->Path().clear();
						}
						const float dist = (Target - position).lengthSquared();
						if ((missionQueue->getMoveData() <= 0 && dist > 100.0f) || ((SQUARE(missionQueue->getMoveData()) << 6) < dist))
						{
							if ((last_path_refresh >= 5.0f && !requesting_pathfinder) || pType->canfly)
							{
								unsetFlag(missionQueue->Flags(), MISSION_FLAG_REFRESH_PATH);

								move_target_computed = missionQueue->getTarget().getPos();
								last_path_refresh = 0.0f;
								if (pType->canfly)
								{
									requesting_pathfinder = false;
									if (missionQueue->getMoveData() <= 0)
										missionQueue->Path() = Pathfinder::directPath(missionQueue->getTarget().getPos());
									else
									{
										Vector3D Dir = missionQueue->getTarget().getPos() - position;
										Dir.normalize();
										missionQueue->Path() = Pathfinder::directPath(missionQueue->getTarget().getPos() - float(missionQueue->getMoveData() << 3) * Dir);
									}
								}
								else
								{
									requesting_pathfinder = true;
									Pathfinder::instance()->addTask(idx, missionQueue->getMoveData(), position, missionQueue->getTarget().getPos());

									if (!(unit_manager.unit_type[typeId]->canfly && nb_attached > 0)) // Once loaded with units the Atlas cannot land
										stopMovingAnimation();
									was_moving = false;
									velocity.reset();
									angularVelocity.reset();
								}
								if (!missionQueue->Path().empty()) // Update required data
									Target = missionQueue->Path().Pos();
							}
						}
						else
						{
							stopMoving();
							return;
						}
					}
				}
				if (!missionQueue->Path().empty()) // If we have a path, follow it
				{
					if ((missionQueue->getTarget().getPos() - move_target_computed).lengthSquared() >= 10000.0f) // Follow the target above all...
						missionQueue->Flags() |= MISSION_FLAG_REFRESH_PATH;
					J = Target - position;
					J.y = 0.0f;
					const float dist = J.lengthSquared();
					if (dist > missionQueue->getLastD() && (dist < 256.0f || (dist < 225.0f && missionQueue->Path().length() <= 1)))
					{
						missionQueue->Path().next();
						missionQueue->setLastD(99999999.0f);
						if (missionQueue->Path().empty()) // End of path reached
						{
							requesting_pathfinder = false;
							J = move_target_computed - position;
							J.y = 0.0f;
							if (J.lengthSquared() <= 256.0f || flying)
							{
								if (!(missionQueue->getFlags() & MISSION_FLAG_DONT_STOP_MOVE) && (!missionQueue.empty() || missionQueue->mission() != MISSION_PATROL))
									playSound("arrived1");
								unsetFlag(missionQueue->Flags(), MISSION_FLAG_MOVE);
							}
							else // We are not where we are supposed to be !!
								setFlag(missionQueue->Flags(), MISSION_FLAG_REFRESH_PATH);
							if (!(pType->canfly && nb_attached > 0)) // Once charged with units the Atlas cannot land
							{
								stopMovingAnimation();
								was_moving = false;
							}
							if (!(missionQueue->getFlags() & MISSION_FLAG_DONT_STOP_MOVE))
								velocity.reset();		   // Stop unit's movement
							if (missionQueue->isStep()) // It's meaningless to try to finish this mission like an ordinary order
							{
								next_mission();
								return;
							}
							if (!(missionQueue->getFlags() & MISSION_FLAG_DONT_STOP_MOVE))
								return;
						}
						else // If we managed to get there you can forget path refresh order
							unsetFlag(missionQueue->Flags(), MISSION_FLAG_REFRESH_PATH);
					}
					else
						missionQueue->setLastD(dist);
				}
			}

			if (pType->canfly)
			{
				if (missionQueue->getFlags() & MISSION_FLAG_MOVE)
				{
					J = Target - position;
				}
				else
					J.reset();
			}
			else
			{
				const float energy = getLocalMapEnergy(cur_px, cur_py);
				if (selfmove || ((missionQueue->getFlags() & MISSION_FLAG_MOVE) && !missionQueue->Path().empty()))
				{
					J = Target;
					computeHeadingBasedOnEnergy(J, missionQueue->getFlags() & MISSION_FLAG_MOVE);
				}
				else if (!(missionQueue->getFlags() & MISSION_FLAG_MOVE) && lastEnergy < energy && !requesting_pathfinder && local)
				{
					switch (missionQueue->mission())
					{
						case MISSION_ATTACK:
						case MISSION_GUARD:
						case MISSION_STANDBY:
						case MISSION_VTOL_STANDBY:
						case MISSION_STOP:
							J = Target;
							computeHeadingBasedOnEnergy(J, false);
							selfmove = J.lengthSquared() > 0.1f;
							break;
						default:
							J.reset();
					};
				}
				else
					J.reset();
				lastEnergy = energy;
			}

			if (((missionQueue->getFlags() & MISSION_FLAG_MOVE) && !missionQueue->Path().empty()) || (!(missionQueue->getFlags() & MISSION_FLAG_MOVE) &&
				J.lengthSquared() > 0.1f)) // Are we still moving ??
			{
				if (!was_moving)
				{
					startMovingAnimation();
					was_moving = true;
				}
				const float dist = (missionQueue->getFlags() & MISSION_FLAG_MOVE) ? (Target - position).length() : 999999.9f;
				if (dist > 0.0f && pType->canfly)
					J = 1.0f / dist * J;

				b_TargetAngle = true;
				f_TargetAngle = acosf(J.z) * RAD2DEG;
				if (J.x < 0.0f)
					f_TargetAngle = -f_TargetAngle;

				if (orientation.y - f_TargetAngle >= 360.0f)
					f_TargetAngle += 360.0f;
				else if (orientation.y - f_TargetAngle <= -360.0f)
					f_TargetAngle -= 360.0f;

				J.z = cosf(orientation.y * DEG2RAD);
				J.x = sinf(orientation.y * DEG2RAD);
				J.y = 0.0f;
				I.z = -J.x;
				I.x = J.z;
				I.y = 0.0f;
				velocity = (velocity % K) * K + (velocity % J) * J;

				Vector3D D(Target.z - position.z, 0.0f, position.x - Target.x);
				D.normalize();
				float speed = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
				const float vsin = fabsf(D % velocity);
				const float deltaX = 8.0f * vsin / (pType->TurnRate * DEG2RAD);
				const float time_to_stop = speed / pType->BrakeRate;
				const float min_dist = time_to_stop * (speed - pType->BrakeRate * 0.5f * time_to_stop);
				if ((deltaX > dist && vsin * dist > speed * 16.0f) || (min_dist >= dist
																		  //					&& mission->Path().length() == 1
																		  && !(missionQueue->getFlags() & MISSION_FLAG_DONT_STOP_MOVE) && (!missionQueue.hasNext() || (missionQueue(1) != MISSION_MOVE && missionQueue(1) != MISSION_PATROL)))) // Brake if needed
				{
					velocity = velocity - pType->BrakeRate * dt * J;
					// Don't go backward
					if (J % velocity <= 0.0f)
						velocity.reset();
				}
				else if (speed < pType->MaxVelocity)
				{
					velocity = velocity + pType->Acceleration * dt * J;
					speed = velocity.length();
					if (speed > pType->MaxVelocity)
						velocity = pType->MaxVelocity / speed * velocity;
				}
			}
			else if (selfmove)
			{
				selfmove = false;
				if (was_moving)
				{
					velocity.reset();
					if (!(pType->canfly && nb_attached > 0)) // Once charged with units the Atlas cannot land
						stopMovingAnimation();
				}
				was_moving = false;
				if (!(missionQueue->getFlags() & MISSION_FLAG_MOVE))
					requesting_pathfinder = false;
			}

			NPos = position + dt * velocity;			   // Check if the unit can go where V brings it
			if (!Math::AlmostZero(was_locked)) // Random move to solve the unit lock problem
			{
				if (velocity.x > 0.0f)
					NPos.x += float(Math::RandomTable() % 101) * 0.01f;
				else
					NPos.x -= float(Math::RandomTable() % 101) * 0.01f;
				if (velocity.z > 0.0f)
					NPos.z += float(Math::RandomTable() % 101) * 0.01f;
				else
					NPos.z -= float(Math::RandomTable() % 101) * 0.01f;

				if (was_locked >= 5.0f)
				{
					was_locked = 5.0f;
					setFlag(missionQueue->Flags(), MISSION_FLAG_REFRESH_PATH); // Refresh path because this shouldn't happen unless
																		  // obstacles have moved
				}
			}
			auto heightmapIndex = the_map->worldToHeightmapIndex(NPos);
			n_px = heightmapIndex.x;
			n_py = heightmapIndex.y;
			precomputed_position = true;
			bool locked = false;
			if (!flying)
			{
				if (n_px != cur_px || n_py != cur_py) // has something changed ??
				{
					bool place_is_empty = can_be_there(n_px, n_py, typeId, ownerId, idx);
					if (!(flags & 64) && !place_is_empty)
					{
						if (!pType->canfly)
						{
							locked = true;
							// Check some basic solutions first
							if (cur_px != n_px && can_be_there(cur_px, n_py, typeId, ownerId, idx))
							{
								velocity.z = !Math::AlmostZero(velocity.z)
									? (velocity.z < 0.0f
											  ? -sqrtf(SQUARE(velocity.z) + SQUARE(velocity.x))
											  : sqrtf(SQUARE(velocity.z) + SQUARE(velocity.x)))
									: 0.0f;
								velocity.x = 0.0f;
								NPos.x = position.x;
								n_px = cur_px;
							}
							else if (cur_py != n_py && can_be_there(n_px, cur_py, typeId, ownerId, idx))
							{
								velocity.x = !Math::AlmostZero(velocity.x)
									? ((velocity.x < 0.0f)
											  ? -sqrtf(SQUARE(velocity.z) + SQUARE(velocity.x))
											  : sqrtf(SQUARE(velocity.z) + SQUARE(velocity.x)))
									: 0.0f;
								velocity.z = 0.0f;
								NPos.z = position.z;
								n_py = cur_py;
							}
							else if (can_be_there(cur_px, cur_py, typeId, ownerId, idx))
							{
								velocity.x = velocity.y = velocity.z = 0.0f; // Don't move since we can't
								NPos = position;
								n_px = cur_px;
								n_py = cur_py;
								missionQueue->Flags() |= MISSION_FLAG_MOVE;
								if (fabsf(orientation.y - f_TargetAngle) <= 0.1f || !b_TargetAngle) // Don't prevent unit from rotating!!
									missionQueue->Path().clear();
							}
							else
								LOG_WARNING("A Unit is blocked !" << __FILE__ << ":" << __LINE__);
						}
						else if (!flying && local)
						{
							if (position.x < -the_map->halfWidthInWorldUnits || position.x > the_map->halfWidthInWorldUnits || position.z < -the_map->halfHeightInWorldUnits || position.z > the_map->halfHeightInWorldUnits)
							{
								Vector3D target = position;
								if (target.x < -the_map->halfWidthInWorldUnits + 256)
									target.x = float(-the_map->halfWidthInWorldUnits + 256);
								else if (target.x > the_map->halfWidthInWorldUnits - 256)
									target.x = float(the_map->halfWidthInWorldUnits - 256);
								if (target.z < -the_map->halfHeightInWorldUnits + 256)
									target.z = float(-the_map->halfHeightInWorldUnits + 256);
								else if (target.z > the_map->halfHeightInWorldUnits - 256)
									target.z = float(the_map->halfHeightInWorldUnits - 256);
								next_mission();
								add_mission(MISSION_MOVE | MISSION_FLAG_AUTO, &target, true, 0, NULL, 0, 1); // Stay on map
							}
							else
							{
								if (!can_be_there(cur_px, cur_py, typeId, ownerId, idx) && !flying)
								{
									NPos = position;
									n_px = cur_px;
									n_py = cur_py;
									Vector3D target = position;
									target.x += float(((sint32)(Math::RandomTable() & 0x1F)) - 16); // Look for a place to land
									target.z += float(((sint32)(Math::RandomTable() & 0x1F)) - 16);
									setFlag(missionQueue->Flags(), MISSION_FLAG_MOVE);
									missionQueue->Path() = Pathfinder::directPath(target);
								}
							}
						}
					}
					else if (!(flags & 64) && pType->canfly && (!missionQueue || (missionQueue->mission() != MISSION_MOVE && missionQueue->mission() != MISSION_GUARD && missionQueue->mission() != MISSION_ATTACK)))
						flags |= 64;
				}
				else
				{
					const bool place_is_empty = the_map->check_rect(n_px - (pType->FootprintX >> 1), n_py - (pType->FootprintZ >> 1), pType->FootprintX, pType->FootprintZ, idx);
					if (!place_is_empty)
					{
						pMutex.unlock();
						clear_from_map();
						pMutex.lock();
						LOG_WARNING("A Unit is blocked ! (probably spawned on something)" << __FILE__ << ":" << __LINE__);
					}
				}
			}
			if (locked)
				was_locked += dt;
			else
				was_locked = 0.0f;
		}
		else
		{
			if (was_moving)
				stopMoving();
			was_moving = false;
			requesting_pathfinder = false;
		}

		if (flying && local) // Force planes to stay on map
		{
			if (position.x < -the_map->halfWidthInWorldUnits || position.x > the_map->halfWidthInWorldUnits || position.z < -the_map->halfHeightInWorldUnits || position.z > the_map->halfHeightInWorldUnits)
			{
				if (position.x < -the_map->halfWidthInWorldUnits)
					velocity.x += dt * (-(float)the_map->halfWidthInWorldUnits - position.x) * 0.1f;
				else if (position.x > the_map->halfWidthInWorldUnits)
					velocity.x -= dt * (position.x - (float)the_map->halfWidthInWorldUnits) * 0.1f;
				if (position.z < -the_map->halfHeightInWorldUnits)
					velocity.z += dt * (-(float)the_map->halfHeightInWorldUnits - position.z) * 0.1f;
				else if (position.z > the_map->halfHeightInWorldUnits)
					velocity.z -= dt * (position.z - (float)the_map->halfHeightInWorldUnits) * 0.1f;
				float speed = velocity.length();
				if (speed > pType->MaxVelocity && speed > 0.0f)
				{
					velocity = pType->MaxVelocity / speed * velocity;
					speed = pType->MaxVelocity;
				}
				if (speed > 0.0f)
				{
					orientation.y = acosf(velocity.z / speed) * RAD2DEG;
					if (velocity.x < 0.0f)
						orientation.y = -orientation.y;
				}
			}
		}

		//----------------------------------- End of moving code ------------------------------------
	}

	int Unit::move(const float dt)
	{
		pMutex.lock();

		requestedMovingAnimationState = movingAnimation;

		const bool was_open = port[YARD_OPEN] != 0;
		const bool was_flying = flying;
		const sint32 o_px = cur_px;
		const sint32 o_py = cur_py;
		compute_coord = true;
		const Vector3D old_V = velocity;   // Store the speed, so we can do some calculations
		hasTargetAngle = false;
		targetAngle = 0.0f;

		Vector3D NPos = position;
		int n_px = cur_px;
		int n_py = cur_py;
		bool precomputed_position = false;

		if (typeId < 0 || typeId >= unit_manager.nb_unit || flags == 0) // A unit which cannot exist
		{
			pMutex.unlock();
			LOG_ERROR("Unit::move : A unit which doesn't exist was found");
			return -1; // Should NEVER happen
		}

		const UnitType* const pType = unit_manager.unit_type[typeId];

		const float resource_min_factor = TA3D::Math::Min(TA3D::players.energy_factor[ownerId], TA3D::players.metal_factor[ownerId]);

		if (!isBeingBuilt() && pType->isfeature) // Turn this unit into a feature
		{
			if (cur_px > 0 && cur_py > 0 && cur_px < the_map->widthInHeightmapTiles && cur_py < the_map->heightInHeightmapTiles)
			{
				if (the_map->map_data(cur_px, cur_py).stuff == -1)
				{
					const int type = feature_manager.get_feature_index(pType->Corpse);
					if (type >= 0)
					{
						features.lock();
						the_map->map_data(cur_px, cur_py).stuff = features.add_feature(position, type);
						if (the_map->map_data(cur_px, cur_py).stuff == -1)
							LOG_ERROR("Could not turn `" << pType->Unitname << "` into a feature ! Cannot create the feature");
						else
							features.feature[the_map->map_data(cur_px, cur_py).stuff].angle = orientation.y;
						pMutex.unlock();
						clear_from_map();
						pMutex.lock();
						features.drawFeatureOnMap(the_map->map_data(cur_px, cur_py).stuff);
						features.unlock();
						flags = 4;
					}
					else
						LOG_ERROR("Could not turn `" << pType->Unitname << "` into a feature ! Feature not found");
				}
			}
			pMutex.unlock();
			return -1;
		}

		if (the_map->ota_data.waterdoesdamage && position.y < the_map->sealvl) // The unit is damaged by the "acid" water
			hp -= dt * float(the_map->ota_data.waterdamage);

		if (!isBeingBuilt() && self_destruct >= 0.0f) // Self-destruction code
		{
			const int old = (int)self_destruct;
			self_destruct -= dt;
			if (old != (int)self_destruct) // Play a sound :-)
				playSound(String("count") << old);
			if (self_destruct <= 0.0f)
			{
				self_destruct = 0.0f;
				hp = 0.0f;
				severity = pType->MaxDamage;
			}
		}

		if (hp <= 0.0f && (local || exploding)) // L'unité est détruite
		{
			if (!missionQueue.empty() && !pType->BMcode && (missionQueue->mission() == MISSION_BUILD_2 || missionQueue->mission() == MISSION_BUILD)) // It was building something that we must destroy too
			{
				Unit* p = missionQueue->getUnit();
				if (p)
				{
					p->lock();
					p->hp = 0.0f;
					p->built = false;
					p->unlock();
				}
			}
			++death_timer;
			if (death_timer == 255) // Ok we've been dead for a long time now ...
			{
				pMutex.unlock();
				return -1;
			}
			switch (flags & 0x17)
			{
				case 1:		   // Début de la mort de l'unité	(Lance le script)
					flags = 4; // Don't remove the data on the position map because they will be replaced
					if (!isBeingBuilt() && local)
						explode();
					else
						flags = 1;
					death_delay = 1.0f;
					if (flags == 1)
					{
						pMutex.unlock();
						return -1;
					}
					break;
				case 4: // Vérifie si le script est terminé
					if (death_delay <= 0.0f || !data.explode)
					{
						flags = 1;
						pMutex.unlock();
						clear_from_map();
						return -1;
					}
					death_delay -= dt;
					for (AnimationData::DataVector::iterator i = data.data.begin(); i != data.data.end(); ++i)
						if (!(i->flag & FLAG_EXPLODE)) // || (data.flag[i]&FLAG_EXPLODE && (data.explosion_flag[i]&EXPLODE_BITMAPONLY)))
							i->flag |= FLAG_HIDE;
					break;
				case 0x14: // Unit has been captured, this is a FAKE unit, just here to be removed
					flags = 4;
					pMutex.unlock();
					return -1;
				default: // It doesn't explode (it has been reclaimed for example)
					flags = 1;
					pMutex.unlock();
					clear_from_map();
					return -1;
			}
			if (data.nb_piece > 0 && !isBeingBuilt())
			{
				data.move(dt, the_map->ota_data.gravity);
				if (c_time >= 0.1f)
				{
					c_time = 0.0f;
					for (AnimationData::DataVector::iterator i = data.data.begin(); i != data.data.end(); ++i)
						if ((i->flag & FLAG_EXPLODE) && (i->explosion_flag & EXPLODE_BITMAPONLY) != EXPLODE_BITMAPONLY)
						{
							if (i->explosion_flag & EXPLODE_FIRE)
							{
								compute_model_coord();
								particle_engine.make_smoke(position + i->pos, fire, 1, 0.0f, 0.0f);
							}
							if (i->explosion_flag & EXPLODE_SMOKE)
							{
								compute_model_coord();
								particle_engine.make_smoke(position + i->pos, 0, 1, 0.0f, 0.0f);
							}
						}
				}
			}
			goto script_exec;
		}
		else if (do_nothing() && local)
			if (position.x < -the_map->halfWidthInWorldUnits || position.x > the_map->halfWidthInWorldUnits || position.z < -the_map->halfHeightInWorldUnits || position.z > the_map->halfHeightInWorldUnits)
			{
				Vector3D target = position;
				if (target.x < -the_map->halfWidthInWorldUnits + 256)
					target.x = float(-the_map->halfWidthInWorldUnits + 256);
				else if (target.x > the_map->halfWidthInWorldUnits - 256)
					target.x = float(the_map->halfWidthInWorldUnits - 256);
				if (target.z < -the_map->halfHeightInWorldUnits + 256)
					target.z = float(-the_map->halfHeightInWorldUnits + 256);
				else if (target.z > the_map->halfHeightInWorldUnits - 256)
					target.z = float(the_map->halfHeightInWorldUnits - 256);
				add_mission(MISSION_MOVE | MISSION_FLAG_AUTO, &target, true, 0, NULL, 0, 1); // Stay on map
			}

		// clear flag 16 (0xFF == 1110 1111)
		flags &= 0xEF; // To fix a bug

		if (build_percent_left > 0.0f) // Unit isn't finished
		{
			if (!built && local)
			{
				float frac = 1000.0f / float(6 * pType->BuildTime);
				metal_prod = frac * (float)pType->BuildCostMetal;
				frac *= dt;
				hp -= frac * (float)pType->MaxDamage;
				build_percent_left += frac * 100.0f;
			}
			else
				metal_prod = 0.0f;
			goto script_exec;
		}
		else
		{
			if (hp < pType->MaxDamage && pType->HealTime > 0)
			{
				hp += (float)pType->MaxDamage * dt / (float)pType->HealTime;
				if (hp > (float)pType->MaxDamage)
					hp = (float)pType->MaxDamage;
			}
		}

		if (data.nb_piece > 0)
			data.move(dt, units.g_dt);

		if (cloaking && paralyzed <= 0.0f)
		{
			const int conso_energy = (!missionQueue || !(missionQueue->getFlags() & MISSION_FLAG_MOVE)) ? pType->CloakCost : pType->CloakCostMoving;
			TA3D::players.requested_energy[ownerId] += (float)conso_energy;
			if (players.energy[ownerId] >= (energy_cons + (float)conso_energy) * dt)
			{
				energy_cons += (float)conso_energy;
				const int dx = pType->mincloakdistance >> 3;
				const int distance = SQUARE(pType->mincloakdistance);
				// byte mask = 1 << ownerId;
				bool found = false;
				for (int y = cur_py - dx; y <= cur_py + dx && !found; y++)
					if (y >= 0 && y < the_map->heightInHeightmapTiles - 1)
						for (int x = cur_px - dx; x <= cur_px + dx; x++)
							if (x >= 0 && x < the_map->widthInHeightmapTiles - 1)
							{
								const int cur_idx = the_map->map_data(x, y).unit_idx;

								if (cur_idx >= 0 && cur_idx < (int)units.max_unit && units.unit[cur_idx].isAlive() && units.unit[cur_idx].isNotOwnedBy(ownerId) && distance >=
																																										(position -
																																										 units.unit[cur_idx].position).lengthSquared())
								{
									found = true;
									break;
								}
							}
				cloaked = !found;
			}
			else
				cloaked = false;
		}
		else
			cloaked = false;

		if (paralyzed > 0.0f) // This unit is paralyzed
		{
			paralyzed -= dt;
			if (pType->model)
			{
				Vector3D randVec;
				bool random_vector = false;
				int n = 0;
				for (int base_n = Math::RandomTable(); !random_vector && n < (int)pType->model->nb_obj; ++n)
					random_vector = pType->model->mesh->random_pos(&data, (base_n + n) % (int)pType->model->nb_obj, &randVec);
				if (random_vector)
					fx_manager.addElectric(position + randVec);
			}
			if (!isBeingBuilt())
				metal_prod = 0.0f;
		}

		if (attached || paralyzed > 0.0f)
			goto script_exec;

		if (pType->canload && nb_attached > 0)
		{
			int e = 0;
			compute_model_coord();
			for (int i = 0; i + e < nb_attached; ++i)
			{
				if (units.unit[attached_list[i]].flags)
				{
					units.unit[attached_list[i]].setPosition(position + data.data[link_list[i]].pos);
					units.unit[attached_list[i]].orientation = orientation;
				}
				else
				{
					++e;
					--i;
					continue;
				}
				attached_list[i] = attached_list[i + e];
			}
			nb_attached -= e;
		}

		if (planned_weapons > 0.0f) // Construit des armes / build weapons
		{
			const float old = planned_weapons - float(int(planned_weapons));
			int idx = -1;
			for (unsigned int i = 0; i < pType->weapon.size(); ++i)
			{
				if (pType->weapon[i] && pType->weapon[i]->stockpile)
				{
					idx = i;
					break;
				}
			}
			if (idx != -1 && !Math::AlmostZero(pType->weapon[idx]->reloadtime))
			{
				const float dn = dt / pType->weapon[idx]->reloadtime;
				const float conso_metal = ((float)pType->weapon[idx]->metalpershot) / pType->weapon[idx]->reloadtime;
				const float conso_energy = ((float)pType->weapon[idx]->energypershot) / pType->weapon[idx]->reloadtime;

				TA3D::players.requested_energy[ownerId] += conso_energy;
				TA3D::players.requested_metal[ownerId] += conso_metal;

				if (players.metal[ownerId] >= (metal_cons + conso_metal * resource_min_factor) * dt && players.energy[ownerId] >= (energy_cons + conso_energy * resource_min_factor) * dt)
				{
					metal_cons += conso_metal * resource_min_factor;
					energy_cons += conso_energy * resource_min_factor;
					planned_weapons -= dn * resource_min_factor;
					const float last = planned_weapons - float(int(planned_weapons));
					if ((Math::AlmostZero(last) && !Math::AlmostEquals(last, old)) || (last > old && old > 0.0f) || planned_weapons <= 0.0f) // On en a fini une / one is finished
						weapon[idx].stock++;
					if (planned_weapons < 0.0f)
						planned_weapons = 0.0f;
				}
			}
		}

		angularVelocity.reset();
		c_time += dt;

		//------------------------------ Beginning of weapon related code ---------------------------------------
		for (unsigned int i = 0; i < weapon.size(); ++i)
		{
			if (pType->weapon[i] == NULL)
				continue; // Skip that weapon if not present on the unit
			weapon[i].delay += dt;
			weapon[i].time += dt;

			int Query_script;
			int Aim_script;
			int AimFrom_script;
			int Fire_script;
			switch (i)
			{
				case 0:
					Query_script = SCRIPT_QueryPrimary;
					Aim_script = SCRIPT_AimPrimary;
					AimFrom_script = SCRIPT_AimFromPrimary;
					Fire_script = SCRIPT_FirePrimary;
					break;
				case 1:
					Query_script = SCRIPT_QuerySecondary;
					Aim_script = SCRIPT_AimSecondary;
					AimFrom_script = SCRIPT_AimFromSecondary;
					Fire_script = SCRIPT_FireSecondary;
					break;
				case 2:
					Query_script = SCRIPT_QueryTertiary;
					Aim_script = SCRIPT_AimTertiary;
					AimFrom_script = SCRIPT_AimFromTertiary;
					Fire_script = SCRIPT_FireTertiary;
					break;
				default:
					Query_script = SCRIPT_QueryWeapon + (i - 3) * 4;
					Aim_script = SCRIPT_AimWeapon + (i - 3) * 4;
					AimFrom_script = SCRIPT_AimFromWeapon + (i - 3) * 4;
					Fire_script = SCRIPT_FireWeapon + (i - 3) * 4;
			}

			switch (weapon[i].state & 3)
			{
				case WEAPON_FLAG_IDLE: // Doing nothing, waiting for orders
					if (pType->weapon[i]->turret)
						script->setReturnValue(UnitScriptInterface::get_script_name(Aim_script), 0);
					weapon[i].data = -1;
					break;
				case WEAPON_FLAG_AIM: // Vise une unité / aiming code
					if (!(missionQueue->getFlags() & MISSION_FLAG_CAN_ATTACK) || (weapon[i].target == NULL && pType->weapon[i]->toairweapon))
					{
						weapon[i].data = -1;
						weapon[i].state = WEAPON_FLAG_IDLE;
						break;
					}

					if (weapon[i].target == NULL || ((weapon[i].state & WEAPON_FLAG_WEAPON) == WEAPON_FLAG_WEAPON && ((Weapon*)(weapon[i].target))->weapon_id != -1) || ((weapon[i].state & WEAPON_FLAG_WEAPON) != WEAPON_FLAG_WEAPON && ((Unit*)(weapon[i].target))->isAlive()))
					{
						if ((weapon[i].state & WEAPON_FLAG_WEAPON) != WEAPON_FLAG_WEAPON && weapon[i].target != NULL && ((Unit*)(weapon[i].target))->cloaked && ((const Unit*)(weapon[i].target))->isNotOwnedBy(ownerId) && !((const Unit*)(weapon[i].target))->is_on_radar(toPlayerMask(ownerId)))
						{
							weapon[i].data = -1;
							weapon[i].state = WEAPON_FLAG_IDLE;
							break;
						}

						if (!(weapon[i].state & WEAPON_FLAG_COMMAND_FIRE) && pType->weapon[i]->commandfire) // Not allowed to fire
						{
							weapon[i].data = -1;
							weapon[i].state = WEAPON_FLAG_IDLE;
							break;
						}

						if (weapon[i].delay >= pType->weapon[i]->reloadtime || pType->weapon[i]->stockpile)
						{
							bool readyToFire = false;

							Unit* const target_unit = (weapon[i].state & WEAPON_FLAG_WEAPON) == WEAPON_FLAG_WEAPON ? NULL : (Unit*)weapon[i].target;
							const Weapon* const target_weapon = (weapon[i].state & WEAPON_FLAG_WEAPON) == WEAPON_FLAG_WEAPON ? (Weapon*)weapon[i].target : NULL;

							Vector3D target = target_unit == NULL ? (target_weapon == NULL ? weapon[i].target_pos - position : target_weapon->Pos - position) : target_unit->position - position;
							float dist = target.lengthSquared();
							int maxdist = 0;
							int mindist = 0;

							if (pType->attackrunlength > 0)
							{
								if (target % velocity < 0.0f)
								{
									weapon[i].state = WEAPON_FLAG_IDLE;
									weapon[i].data = -1;
									break; // We're not shooting at the target
								}
								const float t = 2.0f / the_map->ota_data.gravity * fabsf(target.y);
								mindist = (int)sqrtf(t * velocity.lengthSquared()) - ((pType->attackrunlength + 1) >> 1);
								maxdist = mindist + (pType->attackrunlength);
							}
							else
							{
								if (pType->weapon[i]->waterweapon && position.y > the_map->sealvl)
								{
									if (target % velocity < 0.0f)
									{
										weapon[i].state = WEAPON_FLAG_IDLE;
										weapon[i].data = -1;
										break; // We're not shooting at the target
									}
									const float t = 2.0f / the_map->ota_data.gravity * fabsf(target.y);
									mindist = (int)sqrtf(t * velocity.lengthSquared());
									maxdist = mindist + (pType->weapon[i]->range >> 1);
								}
								else
									maxdist = pType->weapon[i]->range >> 1;
							}

							if (dist > maxdist * maxdist || dist < mindist * mindist)
							{
								weapon[i].state = WEAPON_FLAG_IDLE;
								weapon[i].data = -1;
								break; // We're too far from the target
							}

							Vector3D target_translation;
							if (target_unit != NULL)
								for (int k = 0; k < 3; ++k) // Iterate to get a better approximation
									target_translation = ((target + target_translation).length() / pType->weapon[i]->weaponvelocity) * (target_unit->velocity - velocity);

							if (pType->weapon[i]->turret) // Si l'unité doit viser, on la fait viser / if it must aim, we make it aim
							{
								readyToFire = script->getReturnValue(UnitScriptInterface::get_script_name(Aim_script));

								int start_piece = weapon[i].aim_piece;
								if (weapon[i].aim_piece < 0)
									weapon[i].aim_piece = start_piece = runScriptFunction(AimFrom_script);
								if (start_piece < 0 || start_piece >= data.nb_piece)
									start_piece = 0;
								compute_model_coord();

								Vector3D target_pos_on_unit; // Read the target piece on the target unit so we better know where to aim
								target_pos_on_unit.reset();
								const Model* pModel = NULL;
								Vector3D pos_of_target_unit;
								if (target_unit != NULL)
								{
									target_unit->lock();
									if (target_unit->isAlive())
									{
										pos_of_target_unit = target_unit->position;
										pModel = target_unit->model;
										if (weapon[i].data == -1 && pModel && pModel->nb_obj > 0)
											weapon[i].data = sint16(Math::RandomTable() % pModel->nb_obj);
										if (weapon[i].data >= 0)
										{
											target_unit->compute_model_coord();
											if (target_unit->isAlive())
											{
												if (pModel && (int)target_unit->data.data.size() < weapon[i].data && pModel->mesh->random_pos(&(target_unit->data), weapon[i].data, &target_pos_on_unit))
													target_pos_on_unit = target_unit->data.data[weapon[i].data].tpos;
											}
										}
										else if (pModel)
											target_pos_on_unit = pModel->center;
									}
									target_unit->unlock();
								}

								target += target_translation - data.data[start_piece].tpos;
								if (target_unit != NULL)
									target += target_pos_on_unit;

								if (pType->aim_data[i].check) // Check angle limitations (not in OTA)
								{
									// Go back in model coordinates so we can compare to the weapon main direction
									Vector3D dir = target * RotateXZY(-orientation.x * DEG2RAD, -orientation.z * DEG2RAD, -orientation.y * DEG2RAD);
									// Check weapon
									if (VAngle(dir, pType->aim_data[i].dir) > pType->aim_data[i].Maxangledif)
									{
										weapon[i].state = WEAPON_FLAG_IDLE;
										weapon[i].data = -1;
										break;
									}
								}

								dist = target.length();
								target = (1.0f / dist) * target;
								const Vector3D I(0.0f, 0.0f, 1.0f),
									J(1.0f, 0.0f, 0.0f),
									IJ(0.0f, 1.0f, 0.0f);
								Vector3D RT = target;
								RT.y = 0.0f;
								RT.normalize();
								float angle = acosf(RT.z) * RAD2DEG;
								if (RT.x < 0.0f)
									angle = -angle;
								angle -= orientation.y;
								if (angle < -180.0f)
									angle += 360.0f;
								else if (angle > 180.0f)
									angle -= 360.0f;

								int aiming[] = {(int)(angle * DEG2TA), -4096};
								if (pType->weapon[i]->ballistic) // Calculs de ballistique / ballistic calculations
								{
									Vector3D D = target_unit == NULL
										? (target_weapon == NULL
												  ? position + data.data[start_piece].tpos - weapon[i].target_pos
												  : (position + data.data[start_piece].tpos - target_weapon->Pos))
										: (position + data.data[start_piece].tpos - pos_of_target_unit - target_pos_on_unit);
									D.y = 0.0f;
									float v;
									if (Math::AlmostZero(pType->weapon[i]->startvelocity))
										v = pType->weapon[i]->weaponvelocity;
									else
										v = pType->weapon[i]->startvelocity;
									if (target_unit == NULL)
									{
										if (target_weapon == NULL)
											aiming[1] = (int)(ballistic_angle(v, the_map->ota_data.gravity, D.length(), (position + data.data[start_piece].tpos).y, weapon[i].target_pos.y) * DEG2TA);
										else
											aiming[1] = (int)(ballistic_angle(v, the_map->ota_data.gravity, D.length(), (position + data.data[start_piece].tpos).y, target_weapon->Pos.y) * DEG2TA);
									}
									else if (pModel)
										aiming[1] = (int)(ballistic_angle(v, the_map->ota_data.gravity, D.length(),
															  (position + data.data[start_piece].tpos).y,
															  pos_of_target_unit.y + pModel->center.y * 0.5f)
											* DEG2TA);
								}
								else
								{
									angle = acosf(RT % target) * RAD2DEG;
									if (target.y < 0.0f)
										angle = -angle;
									angle -= orientation.x;
									if (angle > 180.0f)
										angle -= 360.0f;
									if (angle < -180.0f)
										angle += 360.0f;
									if (fabsf(angle) > 180.0f)
									{
										weapon[i].state = WEAPON_FLAG_IDLE;
										weapon[i].data = -1;
										break;
									}
									aiming[1] = (int)(angle * DEG2TA);
								}
								if (readyToFire)
								{
									if (pType->weapon[i]->lineofsight)
									{
										if (!target_unit)
										{
											if (target_weapon == NULL)
												weapon[i].aim_dir = weapon[i].target_pos - (position + data.data[start_piece].tpos);
											else
												weapon[i].aim_dir = ((Weapon*)(weapon[i].target))->Pos - (position + data.data[start_piece].tpos);
										}
										else
											weapon[i].aim_dir = ((Unit*)(weapon[i].target))->position + target_pos_on_unit - (position + data.data[start_piece].tpos);
										weapon[i].aim_dir += target_translation;
										weapon[i].aim_dir.normalize();
									}
									else
										weapon[i].aim_dir = cosf((float)aiming[1] * TA2RAD) * (cosf((float)aiming[0] * TA2RAD + orientation.y * DEG2RAD) * I + sinf((float)aiming[0] * TA2RAD + orientation.y * DEG2RAD) * J) + sinf((float)aiming[1] * TA2RAD) * IJ;
								}
								else
									launchScript(Aim_script, 2, aiming);
							}
							else
							{
								readyToFire = launchScript(Aim_script, 0, NULL) != 0;
								if (!readyToFire)
									readyToFire = script->getReturnValue(UnitScriptInterface::get_script_name(Aim_script));
								if (pType->weapon[i]->lineofsight)
								{
									int start_piece = weapon[i].aim_piece;
									if (weapon[i].aim_piece < 0)
										weapon[i].aim_piece = start_piece = runScriptFunction(AimFrom_script);
									if (start_piece < 0 || start_piece >= data.nb_piece)
										start_piece = 0;
									compute_model_coord();

									if (!target_unit)
									{
										if (target_weapon == NULL)
											weapon[i].aim_dir = weapon[i].target_pos - (position + data.data[start_piece].tpos);
										else
											weapon[i].aim_dir = ((Weapon*)(weapon[i].target))->Pos - (position + data.data[start_piece].tpos);
									}
									else
									{
										Vector3D target_pos_on_unit; // Read the target piece on the target unit so we better know where to aim
										if (weapon[i].data == -1)
											weapon[i].data = (sint16)target_unit->get_sweet_spot();
										if (weapon[i].data >= 0)
										{
											if (target_unit->model && target_unit->model->mesh->random_pos(&(target_unit->data), weapon[i].data, &target_pos_on_unit))
												target_pos_on_unit = target_unit->data.data[weapon[i].data].tpos;
										}
										else if (target_unit->model)
											target_pos_on_unit = target_unit->model->center;
										weapon[i].aim_dir = ((Unit*)(weapon[i].target))->position + target_pos_on_unit - (position + data.data[start_piece].tpos);
									}
									weapon[i].aim_dir += target_translation;
									weapon[i].aim_dir.normalize();
								}
								weapon[i].data = -1;
							}
							if (readyToFire)
							{
								weapon[i].time = 0.0f;
								weapon[i].state = WEAPON_FLAG_SHOOT; // (puis) on lui demande de tirer / tell it to fire
								weapon[i].burst = 0;
							}
						}
					}
					else
					{
						launchScript(SCRIPT_TargetCleared);
						weapon[i].state = WEAPON_FLAG_IDLE;
						weapon[i].data = -1;
					}
					break;
				case WEAPON_FLAG_SHOOT: // Tire sur une unité / fire!
					if (weapon[i].target == NULL || ((weapon[i].state & WEAPON_FLAG_WEAPON) == WEAPON_FLAG_WEAPON && ((Weapon*)(weapon[i].target))->weapon_id != -1) || ((weapon[i].state & WEAPON_FLAG_WEAPON) != WEAPON_FLAG_WEAPON && ((Unit*)(weapon[i].target))->isAlive()))
					{
						if (weapon[i].burst > 0 && weapon[i].delay < pType->weapon[i]->burstrate)
							break;
						if ((players.metal[ownerId] < pType->weapon[i]->metalpershot || players.energy[ownerId] < pType->weapon[i]->energypershot) && !pType->weapon[i]->stockpile)
						{
							weapon[i].state = WEAPON_FLAG_AIM; // Pas assez d'énergie pour tirer / not enough energy to fire
							weapon[i].data = -1;
							script->setReturnValue(UnitScriptInterface::get_script_name(Aim_script), 0);
							break;
						}
						if (pType->weapon[i]->stockpile && weapon[i].stock <= 0)
						{
							weapon[i].state = WEAPON_FLAG_AIM; // Plus rien pour tirer / nothing to fire
							weapon[i].data = -1;
							script->setReturnValue(UnitScriptInterface::get_script_name(Aim_script), 0);
							break;
						}
						int start_piece = runScriptFunction(Query_script);
						if (start_piece >= 0 && start_piece < data.nb_piece)
						{
							compute_model_coord();
							if (!pType->weapon[i]->waterweapon && position.y + data.data[start_piece].tpos.y <= the_map->sealvl) // Can't shoot from water !!
								break;
							Vector3D Dir = data.data[start_piece].dir;
							if (pType->weapon[i]->vlaunch)
							{
								Dir.x = 0.0f;
								Dir.y = 1.0f;
								Dir.z = 0.0f;
							}
							else if (!pType->weapon[i]->turret || Dir.isNull())
								Dir = weapon[i].aim_dir;
							if (i == 3)
							{
								LOG_DEBUG("firing from " << (position + data.data[start_piece].tpos).y << " (" << the_map->get_unit_h((position + data.data[start_piece].tpos).x, (position + data.data[start_piece].tpos).z) << ")");
								LOG_DEBUG("from piece " << start_piece << " (" << Query_script << "," << Aim_script << "," << Fire_script << ")");
							}

							// SHOOT NOW !!
							if (pType->weapon[i]->stockpile)
								weapon[i].stock--;
							else
							{ // We use energy and metal only for weapons with no prebuilt ammo
								players.c_metal[ownerId] -= (float)pType->weapon[i]->metalpershot;
								players.c_energy[ownerId] -= (float)pType->weapon[i]->energypershot;
							}
							launchScript(Fire_script); // Run the fire animation script
							if (!pType->weapon[i]->soundstart.empty())
								sound_manager->playSound(pType->weapon[i]->soundstart, &position);

							if (weapon[i].target == NULL)
								shoot(-1, position + data.data[start_piece].tpos, Dir, i, weapon[i].target_pos);
							else
							{
								if (weapon[i].state & WEAPON_FLAG_WEAPON)
									shoot(((Weapon*)(weapon[i].target))->idx, position + data.data[start_piece].tpos, Dir, i, weapon[i].target_pos);
								else
									shoot(((Unit*)(weapon[i].target))->idx, position + data.data[start_piece].tpos, Dir, i, weapon[i].target_pos);
							}
							weapon[i].burst++;
							if (weapon[i].burst >= pType->weapon[i]->burst)
								weapon[i].burst = 0;
							weapon[i].delay = 0.0f;
							weapon[i].aim_piece = -1;
						}
						if (weapon[i].burst == 0 && pType->weapon[i]->commandfire && !pType->weapon[i]->dropped) // Shoot only once
						{
							weapon[i].state = WEAPON_FLAG_IDLE;
							weapon[i].data = -1;
							script->setReturnValue(UnitScriptInterface::get_script_name(Aim_script), 0);
							if (!missionQueue.empty())
								missionQueue->Flags() |= MISSION_FLAG_COMMAND_FIRED;
							break;
						}
						if (weapon[i].target != NULL && (weapon[i].state & WEAPON_FLAG_WEAPON) != WEAPON_FLAG_WEAPON && ((Unit*)(weapon[i].target))->hp > 0) // La cible est-elle détruite ?? / is target destroyed ??
						{
							if (weapon[i].burst == 0)
							{
								weapon[i].state = WEAPON_FLAG_AIM;
								weapon[i].data = -1;
								weapon[i].time = 0.0f;
								script->setReturnValue(UnitScriptInterface::get_script_name(Aim_script), 0);
							}
						}
						else if (weapon[i].target != NULL || weapon[i].burst == 0)
						{
							launchScript(SCRIPT_TargetCleared);
							weapon[i].state = WEAPON_FLAG_IDLE;
							weapon[i].data = -1;
							script->setReturnValue(UnitScriptInterface::get_script_name(Aim_script), 0);
						}
					}
					else
					{
						launchScript(SCRIPT_TargetCleared);
						weapon[i].state = WEAPON_FLAG_IDLE;
						weapon[i].data = -1;
					}
					break;
			}
		}

		//---------------------------- Beginning of mission execution code --------------------------------------

		if (!missionQueue)
			was_moving = false;

		if (!missionQueue.empty())
		{
			auto currentMission = missionQueue.front();

			currentMission.setTime(currentMission.getTime() + dt);
			last_path_refresh += dt;

			followPath(dt, hasTargetAngle, targetAngle, NPos, n_px, n_py, precomputed_position);

			switch (currentMission.mission()) // General orders
			{
				case MISSION_WAIT:												  // Wait for a specified time (campaign)
					doWaitMission(currentMission);
					break;
				case MISSION_WAIT_ATTACKED: // Wait until a specified unit is attacked (campaign)
					doWaitAttackMission(currentMission);
					break;
				case MISSION_GET_REPAIRED:
					doGetRepairedMission(currentMission, dt);
					break;
				case MISSION_STANDBY_MINE: // Don't even try to do something else, the unit must die !!
					doStandbyMineMission(currentMission);
					break;
				case MISSION_UNLOAD:
					doUnloadMission(currentMission);
					break;
				case MISSION_LOAD:
					doLoadMission(currentMission);
					break;
				case MISSION_CAPTURE:
					doCaptureMission(currentMission, dt);
					break;
				case MISSION_REVIVE:
					doReviveMission(currentMission, dt);
					break;
				case MISSION_RECLAIM:
					doReclaimMission(currentMission, dt);
					break;
				case MISSION_GUARD:
					doGuardMission(currentMission);
					break;
				case MISSION_PATROL: // Mode patrouille
				{
					pad_timer += dt;

					if (!missionQueue.hasNext())
						add_mission(MISSION_PATROL | MISSION_FLAG_AUTO, &position, false, 0, NULL, MISSION_FLAG_CAN_ATTACK, 0, 0); // Retour à la case départ après l'éxécution de tous les ordres / back to beginning

					if (pType->CanReclamate // Auto reclaim things on the battle field when needed
						&& (players.r_energy[ownerId] >= players.energy_t[ownerId] || players.r_metal[ownerId] >= players.metal_t[ownerId]))
					{
						const bool energyLack = players.r_energy[ownerId] >= players.energy_t[ownerId];
						const bool metalLack = players.r_metal[ownerId] >= players.metal_t[ownerId];
						const int dx = pType->SightDistance >> 3;
						const int dx2 = SQUARE(dx);
						int feature_idx = -1;
						const int sx = Math::RandomTable() & 0xF;
						const int sy = Math::RandomTable() & 0xF;
						for (int y = cur_py - dx + sy; y <= cur_py + dx && feature_idx == -1; y += 0x8)
						{
							if (y >= 0 && y < the_map->heightInHeightmapTiles - 1)
							{
								for (int x = cur_px - dx + sx; x <= cur_px + dx && feature_idx == -1; x += 0x8)
								{
									if (SQUARE(cur_px - x) + SQUARE(cur_py - y) > dx2)
										continue;
									if (x >= 0 && x < the_map->widthInHeightmapTiles - 1)
									{
										const int cur_idx = the_map->map_data(x, y).stuff;
										if (cur_idx >= 0) // There is a feature
										{
											Feature* pFeature = feature_manager.getFeaturePointer(features.feature[cur_idx].type);
											if (pFeature && pFeature->autoreclaimable && ((pFeature->metal > 0 && metalLack) || (pFeature->energy > 0 && energyLack)))
												feature_idx = cur_idx;
										}
									}
								}
							}
						}
						if (feature_idx >= 0) // We've something to recycle :P
						{
							add_mission(MISSION_RECLAIM, &(features.feature[feature_idx].Pos), true, feature_idx, NULL);
							break;
						}
					}
					if (pType->Builder) // Repair units if we can
					{
						const int dx = pType->SightDistance;
						std::deque<UnitTKit::T> friends;
						units.kdTreeFriends[ownerId]->maxDistanceQuery(friends, position, (float)dx);
						bool done = false;

						for (std::deque<UnitTKit::T>::const_iterator i = friends.begin(); i != friends.end(); ++i)
						{
							const Unit* const pUnit = i->first;
							if (pUnit == this) // No self-healing
								continue;
							const int friend_type_id = pUnit->typeId;
							if (friend_type_id == -1)
								continue;
							const UnitType* const pFriendType = unit_manager.unit_type[friend_type_id];
							if (pFriendType->BMcode && pUnit->build_percent_left > 0.0f) // Don't help factories
								continue;
							if (pUnit->isAlive() && pUnit->hp < pFriendType->MaxDamage)
							{
								add_mission(MISSION_REPAIR, &(pUnit->position), true, 0, (void*)pUnit);
								done = true;
								break;
							}
						}
						if (done)
							break;
					}

					missionQueue->Flags() |= MISSION_FLAG_CAN_ATTACK;
					if (pType->canfly) // Don't stop moving and check if it can be repaired
					{
						missionQueue->Flags() |= MISSION_FLAG_DONT_STOP_MOVE;

						if (hp < (float)pType->MaxDamage * 0.75f && !attacked && pad_timer >= 5.0f) // Check if a repair pad is free
						{
							bool attacking = false;
							for (uint32 i = 0; i < weapon.size(); ++i)
							{
								if (weapon[i].state != WEAPON_FLAG_IDLE)
								{
									attacking = true;
									break;
								}
							}
							if (!attacking)
							{
								pad_timer = 0.0f;
								bool going_to_repair_pad = false;
								std::deque<UnitTKit::T> repair_pads;
								units.kdTreeRepairPads[ownerId]->maxDistanceQuery(repair_pads, position, pType->ManeuverLeashLength);
								for (std::deque<UnitTKit::T>::const_iterator i = repair_pads.begin(); i != repair_pads.end() && !going_to_repair_pad; ++i)
								{
									const Unit* const pUnit = i->first;
									if ((pUnit->pad1 == 0xFFFF || pUnit->pad2 == 0xFFFF) && !pUnit->isBeingBuilt()) // He can repair us :)
									{
										add_mission(MISSION_GET_REPAIRED | MISSION_FLAG_AUTO, &(pUnit->position), true, 0, (void*)pUnit);
										going_to_repair_pad = true;
									}
								}
								if (going_to_repair_pad)
									break;
							}
						}
					}

					if ((missionQueue->getFlags() & MISSION_FLAG_MOVE) == 0) // Monitor the moving process
					{
						if (!pType->canfly || (!missionQueue.hasNext() || missionQueue->mission() != MISSION_PATROL))
						{
							velocity.reset(); // Stop the unit
							if (precomputed_position)
							{
								NPos = position;
								n_px = cur_px;
								n_py = cur_py;
							}
						}

						missionQueue.add(missionQueue.front());
						missionQueue.back().Flags() |= MISSION_FLAG_MOVE;
						missionQueue.back().Path().clear();

						MissionQueue::iterator cur = missionQueue.begin();
						for (++cur; cur != missionQueue.end() && cur->mission() != MISSION_PATROL; ++cur)
						{
							missionQueue.add(*cur);
							missionQueue.back().Path().clear();
						}

						next_mission();
					}
				}
				break;
				case MISSION_STANDBY:
				case MISSION_VTOL_STANDBY:
					doStandbyMission(currentMission);
					break;
				case MISSION_ATTACK: // Attaque une unité / attack a unit
					doAttackMission(currentMission);
					break;
				case MISSION_GUARD_NOMOVE:
					setFlag(missionQueue->Flags(), MISSION_FLAG_CAN_ATTACK);
					unsetFlag(missionQueue->Flags(), MISSION_FLAG_MOVE);
					if (missionQueue.hasNext())
						next_mission();
					break;
				case MISSION_STOP:																																																													// Arrête tout ce qui était en cours / stop everything running
					doStopMission(currentMission);
					break;
				case MISSION_REPAIR:
					doRepairMission(currentMission, dt);
					break;
				case MISSION_BUILD_2:
					if (!missionQueue->getTarget().isValid())
					{
						next_mission();
						break;
					}
					{
						Unit* target_unit = missionQueue->getUnit();
						if (target_unit && target_unit->flags)
						{
							target_unit->lock();
							if (target_unit->build_percent_left <= 0.0f)
							{
								target_unit->build_percent_left = 0.0f;
								if (unit_manager.unit_type[target_unit->typeId]->ActivateWhenBuilt) // Start activated
								{
									target_unit->port[ACTIVATION] = 0;
									target_unit->activate();
								}
								if (unit_manager.unit_type[target_unit->typeId]->init_cloaked) // Start cloaked
									target_unit->cloaking = true;
								if (!pType->BMcode) // Ordre de se déplacer
								{
									Vector3D target = position;
									target.z += 128.0f;
									if (!defaultMissionQueue)
										target_unit->set_mission(MISSION_MOVE | MISSION_FLAG_AUTO, &target, false, 5, true, NULL, 0, 5); // Fait sortir l'unité du bâtiment
									else
										target_unit->missionQueue = defaultMissionQueue;
								}
								missionQueue->getTarget().set(Mission::Target::TargetNone, -1, 0);
								next_mission();
							}
							else if (port[INBUILDSTANCE] != 0)
							{
								if (local && network_manager.isConnected() && nanolathe_target < 0) // Synchronize nanolathe emission
								{
									nanolathe_target = target_unit->idx;
									g_ta3d_network->sendUnitNanolatheEvent(idx, target_unit->idx, false, false);
								}

								unsetFlag(missionQueue->Flags(), MISSION_FLAG_CAN_ATTACK); // Don't attack when building

								const float conso_metal = ((float)(pType->WorkerTime * unit_manager.unit_type[target_unit->typeId]->BuildCostMetal)) / (float)unit_manager.unit_type[target_unit->typeId]->BuildTime;
								const float conso_energy = ((float)(pType->WorkerTime * unit_manager.unit_type[target_unit->typeId]->BuildCostEnergy)) / (float)unit_manager.unit_type[target_unit->typeId]->BuildTime;

								TA3D::players.requested_energy[ownerId] += conso_energy;
								TA3D::players.requested_metal[ownerId] += conso_metal;

								if (players.metal[ownerId] >= (metal_cons + conso_metal * resource_min_factor) * dt && players.energy[ownerId] >= (energy_cons + conso_energy * resource_min_factor) * dt)
								{
									metal_cons += conso_metal * resource_min_factor;
									energy_cons += conso_energy * resource_min_factor;
									const UnitType* pTargetType = unit_manager.unit_type[target_unit->typeId];
									const float base = dt * resource_min_factor * (float)pType->WorkerTime;
									const float maxdmg = float(pTargetType->MaxDamage);
									target_unit->build_percent_left = std::max(0.0f, target_unit->build_percent_left - base * 100.0f / (float)pTargetType->BuildTime);
									target_unit->hp = std::min(maxdmg, target_unit->hp + base * maxdmg / (float)pTargetType->BuildTime);
								}
								if (!pType->BMcode)
								{
									const int buildinfo = runScriptFunction(SCRIPT_QueryBuildInfo);
									if (buildinfo >= 0)
									{
										compute_model_coord();
										Vector3D old_pos = target_unit->position;
										target_unit->setPosition(position + data.data[buildinfo].pos);
										if (unit_manager.unit_type[target_unit->typeId]->Floater || (unit_manager.unit_type[target_unit->typeId]->canhover && old_pos.y <= the_map->sealvl))
											target_unit->position.y = old_pos.y;
										if (((Vector3D) (old_pos - target_unit->position)).lengthSquared() > 1000000.0f) // It must be continuous
										{
											target_unit->setPosition(old_pos);
										}

										target_unit->orientation = orientation;
										target_unit->orientation.y += data.data[buildinfo].axe[1].angle;
										pMutex.unlock();
										target_unit->draw_on_map();
										pMutex.lock();
									}
								}
								target_unit->built = true;
							}
							else
							{
								activate();
								target_unit->built = true;
							}
							target_unit->unlock();
						}
						else
							next_mission();
					}
					break;
				case MISSION_BUILD:
					if (missionQueue->getUnit())
					{
						missionQueue->setMissionType(MISSION_BUILD_2); // Change mission type
						missionQueue->getUnit()->built = true;
					}
					else
					{
						Vector3D Dir = missionQueue->getTarget().getPos() - position;
						Dir.y = 0.0f;
						const float dist = Dir.lengthSquared();
						const auto unitType = unit_manager.unit_type[missionQueue->getData()];
						const int maxdist = (int)pType->BuildDistance + ((unitType->FootprintX + unitType->FootprintZ) << 1);
						if (dist > maxdist * maxdist && pType->BMcode) // If the unit is too far from the worksite
						{
							setFlag(missionQueue->Flags(), MISSION_FLAG_MOVE);
							missionQueue->setMoveData(maxdist * 7 / 80);
						}
						else
						{
							if (missionQueue->getFlags() & MISSION_FLAG_MOVE) // Stop moving if needed
							{
								stopMoving();
								break;
							}
							if (!pType->BMcode)
							{
								const int buildinfo = runScriptFunction(SCRIPT_QueryBuildInfo);
								if (buildinfo >= 0)
								{
									compute_model_coord();
									missionQueue->getTarget().setPos(position + data.data[buildinfo].pos);
								}
							}
							if (port[INBUILDSTANCE])
							{
								velocity.x = 0.0f;
								velocity.y = 0.0f;
								velocity.z = 0.0f;

								const Vector3D target = missionQueue->getTarget().getPos();
								const Vector2D topLeftPos(
									target.x - ((unitType->FootprintX * MAP::HeightmapTileWidthInWorldUnits) / 2.0f),
									target.z - ((unitType->FootprintX * MAP::HeightmapTileWidthInWorldUnits) / 2.0f)
								);
								auto topLeftHeightmapIndex = the_map->worldToNearestHeightmapCorner(topLeftPos.x, topLeftPos.y);

								// Check if we have an empty place to build our unit
								if (the_map->check_rect(topLeftHeightmapIndex.x, topLeftHeightmapIndex.y, unitType->FootprintX, unitType->FootprintZ, -1))
								{
									pMutex.unlock();
									Unit* p = create_unit(missionQueue->getData(), ownerId, missionQueue->getTarget().getPos());
									if (p)
										missionQueue->getTarget().set(Mission::Target::TargetUnit, p->idx, p->ID);
									pMutex.lock();
									if (p)
									{
										p->hp = 0.000001f;
										p->built = true;
									}
								}
								else if (pType->BMcode)
									next_mission();
							}
							else
								start_building(missionQueue->getTarget().getPos() - position);
						}
					}
					break;
			};

			switch (pType->TEDclass) // Special orders
			{
				case CLASS_PLANT:
					switch (missionQueue->mission())
					{
						case MISSION_STANDBY:
						case MISSION_BUILD:
						case MISSION_BUILD_2:
						case MISSION_REPAIR:
							break;
						default:
							next_mission();
					};
					break;
				case CLASS_WATER:
				case CLASS_VTOL:
				case CLASS_KBOT:
				case CLASS_COMMANDER:
				case CLASS_TANK:
				case CLASS_CNSTR:
				case CLASS_SHIP:
				{
					if (!(missionQueue->getFlags() & MISSION_FLAG_MOVE) && !(missionQueue->getFlags() & MISSION_FLAG_DONT_STOP_MOVE) && ((missionQueue->mission() != MISSION_ATTACK && pType->canfly) || !pType->canfly) && !selfmove)
					{
						if (!flying)
							velocity.x = velocity.z = 0.0f;
						if (precomputed_position)
						{
							NPos = position;
							n_px = cur_px;
							n_py = cur_py;
						}
					}
					switch (missionQueue->mission())
					{
						case MISSION_ATTACK:
						case MISSION_PATROL:
						case MISSION_REPAIR:
						case MISSION_BUILD:
						case MISSION_BUILD_2:
						case MISSION_GET_REPAIRED:
							if (pType->canfly)
								activate();
							break;
						case MISSION_STANDBY:
							if (missionQueue.hasNext())
								next_mission();
							if (!selfmove)
								velocity.reset(); // Frottements
							break;
						case MISSION_MOVE:
							missionQueue->Flags() |= MISSION_FLAG_CAN_ATTACK;
							if (!(missionQueue->getFlags() & MISSION_FLAG_MOVE)) // Monitor the moving process
							{
								if (missionQueue.hasNext() && (missionQueue(1) == MISSION_MOVE || (missionQueue(1) == MISSION_STOP && missionQueue(2) == MISSION_MOVE)))
									missionQueue->Flags() |= MISSION_FLAG_DONT_STOP_MOVE;

								if (!(missionQueue->getFlags() & MISSION_FLAG_DONT_STOP_MOVE)) // If needed
									velocity.reset();											  // Stop the unit
								if (precomputed_position)
								{
									NPos = position;
									n_px = cur_px;
									n_py = cur_py;
								}
								if ((missionQueue->getFlags() & MISSION_FLAG_DONT_STOP_MOVE) && missionQueue.hasNext() && missionQueue(1) == MISSION_STOP) // If needed
									next_mission();
								next_mission();
							}
							break;
						default:
							if (pType->canfly)
								deactivate();
					};
				}
				break;
				case CLASS_UNDEF:
				case CLASS_METAL:
				case CLASS_ENERGY:
				case CLASS_SPECIAL:
				case CLASS_FORT:
					break;
				default:
					LOG_WARNING("Unknown type :" << pType->TEDclass);
			};

			switch (missionQueue->mission()) // Quelques animations spéciales / Some special animation code
			{
				case MISSION_ATTACK:
					if (pType->canfly && !pType->hoverattack) // Un avion?? / A plane ?
					{
						activate();
						unsetFlag(missionQueue->Flags(), MISSION_FLAG_MOVE); // We're doing it here, so no need to do it twice
						Vector3D J, I, K(0.0f, 1.0f, 0.0f);
						Vector3D Target = missionQueue->getTarget().getPos();
						J = Target - position;
						J.y = 0.0f;
						const float dist = J.length();
						missionQueue->setLastD(dist);
						if (dist > 0.0f)
							J = 1.0f / dist * J;
						if (dist > (float)pType->ManeuverLeashLength * 0.5f)
						{
							hasTargetAngle = true;
							targetAngle = acosf(J.z) * RAD2DEG;
							if (J.x < 0.0f)
								targetAngle = -targetAngle;
						}

						J.z = cosf(orientation.y * DEG2RAD);
						J.x = sinf(orientation.y * DEG2RAD);
						J.y = 0.0f;
						I.z = -J.x;
						I.x = J.z;
						I.y = 0.0f;
						velocity = (velocity % K) * K + (velocity % J) * J + units.exp_dt_4 * (velocity % I) * I;
						const float speed = velocity.lengthSquared();
						if (speed < pType->MaxVelocity * pType->MaxVelocity)
							velocity = velocity + pType->Acceleration * dt * J;
					}
					if (!pType->hoverattack)
						break;
				case MISSION_CAPTURE:
				case MISSION_RECLAIM:
				case MISSION_REPAIR:
				case MISSION_BUILD_2:
				case MISSION_REVIVE:
					if (flying) // Brawler and construction aircrafts animation
					{
						activate();
						unsetFlag(missionQueue->Flags(), MISSION_FLAG_MOVE); // We're doing it here, so no need to do it twice
						Vector3D K(0.0f, 1.0f, 0.0f);
						Vector3D J = missionQueue->getTarget().getPos() - position;
						J.y = 0.0f;
						const float dist = J.length();
						if (dist > 0.0f)
							J = 1.0f / dist * J;
						hasTargetAngle = true;
						targetAngle = acosf(J.z) * RAD2DEG;
						if (J.x < 0.0f)
							targetAngle = -targetAngle;

						float ideal_dist = (float)pType->SightDistance * 0.25f;
						switch (missionQueue->mission())
						{
							case MISSION_BUILD_2:
							case MISSION_CAPTURE:
							case MISSION_REPAIR:
							case MISSION_RECLAIM:
							case MISSION_REVIVE:
								ideal_dist = pType->BuildDistance * 0.5f;
						};

						velocity += (Math::Clamp(10.0f * (dist - ideal_dist), -pType->Acceleration, pType->Acceleration) * dt) * J;

						if (dist < 2.0f * ideal_dist)
						{
							J.z = sinf(orientation.y * DEG2RAD);
							J.x = -cosf(orientation.y * DEG2RAD);
							J.y = 0.0f;
							velocity = units.exp_dt_4 * velocity + (dt * dist * Math::Max(8.0f * (cosf(PI * ((float)units.current_tick) / (float)TICKS_PER_SEC) - 0.5f), 0.0f)) * J;
						}
						else
						{
							J.z = sinf(orientation.y * DEG2RAD);
							J.x = -cosf(orientation.y * DEG2RAD);
							J.y = 0.0f;
							J = (J % velocity) * J;
							velocity = (velocity - J) + units.exp_dt_4 * J;
						}
						const float speed = velocity.lengthSquared();
						if (speed > pType->MaxVelocity * pType->MaxVelocity)
							velocity = pType->MaxVelocity / velocity.length() * velocity;
					}
					break;
				case MISSION_GUARD:
					if (pType->canfly) // Aircrafts fly around guarded units
					{
						activate();
						unsetFlag(missionQueue->Flags(), MISSION_FLAG_MOVE); // We're doing it here, so no need to do it twice
						Vector3D J = missionQueue->getTarget().getPos() - position;
						J.y = 0.0f;
						const float dist = J.length();
						if (dist > 0.0f)
							J = 1.0f / dist * J;
						hasTargetAngle = true;
						targetAngle = acosf(J.z) * RAD2DEG;
						if (J.x < 0.0f)
							targetAngle = -targetAngle;
						const float ideal_dist = (float)pType->SightDistance;

						Vector3D acc;
						if (dist > 2.0f * ideal_dist)
							acc = pType->Acceleration * J;
						else
						{
							targetAngle += 90.0f;
							acc = pType->Acceleration * (10.0f * (dist - ideal_dist) * J + Vector3D(J.z, 0.0f, -J.x));
							if (acc.lengthSquared() >= pType->Acceleration * pType->Acceleration)
							{
								acc.normalize();
								acc *= pType->Acceleration;
							}
						}
						velocity += dt * acc;

						J.z = sinf(orientation.y * DEG2RAD);
						J.x = -cosf(orientation.y * DEG2RAD);
						J.y = 0.0f;
						J = (J % velocity) * J;
						velocity = (velocity - J) + units.exp_dt_4 * J;

						const float speed = velocity.lengthSquared();
						if (speed > pType->MaxVelocity * pType->MaxVelocity)
							velocity = pType->MaxVelocity / sqrtf(speed) * velocity;
					}
					break;
			}

			if ((missionQueue->getFlags() & MISSION_FLAG_MOVE) || !local) // Set unit orientation if it's on the ground
			{
				if (!pType->canfly && !pType->Upright && !pType->floatting() && !(pType->canhover && position.y <= the_map->sealvl))
				{
					Vector3D I, J, K, A, B, C;
					Matrix M = RotateY((orientation.y + 90.0f) * DEG2RAD);
					I.x = 4.0f;
					J.z = 4.0f;
					K.y = 1.0f;
					A = position - pType->FootprintZ * I * M;
					B = position + (pType->FootprintX * I - pType->FootprintZ * J) * M;
					C = position + (pType->FootprintX * I + pType->FootprintZ * J) * M;
					A.y = the_map->get_unit_h(A.x, A.z); // Projete le triangle
					B.y = the_map->get_unit_h(B.x, B.z);
					C.y = the_map->get_unit_h(C.x, C.z);
					Vector3D D = (B - A) * (B - C);
					if (D.y >= 0.0f) // On ne met pas une unité à l'envers!!
					{
						D.normalize();
						const float dist_sq = sqrtf(D.y * D.y + D.z * D.z);
						float angle_1 = !Math::AlmostZero(dist_sq) ? acosf(D.y / dist_sq) * RAD2DEG : 0.0f;
						if (D.z < 0.0f)
							angle_1 = -angle_1;
						D = D * RotateX(-angle_1 * DEG2RAD);
						float angle_2 = VAngle(D, K) * RAD2DEG;
						if (D.x > 0.0f)
							angle_2 = -angle_2;
						if (fabsf(angle_1 - orientation.x) <= 180.0f && fabsf(angle_2 - orientation.z) <= 180.0f)
						{
							orientation.x = angle_1;
							orientation.z = angle_2;
						}
					}
				}
				else if (!pType->canfly)
					orientation.x = orientation.z = 0.0f;
			}

			bool returning_fire = (port[STANDINGFIREORDERS] == SFORDER_RETURN_FIRE && attacked);
			if ((((missionQueue->getFlags() & MISSION_FLAG_CAN_ATTACK) == MISSION_FLAG_CAN_ATTACK) || do_nothing()) && (port[STANDINGFIREORDERS] == SFORDER_FIRE_AT_WILL || returning_fire) && local)
			{
				// Si l'unité peut attaquer d'elle même les unités enemies proches, elle le fait / Attack nearby enemies

				bool can_fire = pType->AutoFire && pType->canattack;
				bool canTargetGround = false;

				if (!can_fire)
				{
					for (uint32 i = 0; i < weapon.size() && !can_fire; ++i)
						can_fire = pType->weapon[i] != NULL && !pType->weapon[i]->commandfire && !pType->weapon[i]->interceptor && weapon[i].state == WEAPON_FLAG_IDLE;
				}
				else
				{
					can_fire = false;
					for (uint32 i = 0; i < weapon.size() && !can_fire; ++i)
						can_fire = pType->weapon[i] != NULL && weapon[i].state == WEAPON_FLAG_IDLE;
				}
				for (uint32 i = 0; i < weapon.size() && !canTargetGround; ++i)
					if (pType->weapon[i] && weapon[i].state == WEAPON_FLAG_IDLE)
						canTargetGround |= !pType->weapon[i]->toairweapon;

				if (can_fire)
				{
					int dx = pType->SightDistance + (int)(h + 0.5f);
					int enemy_idx = -1;
					for (uint32 i = 0; i < weapon.size(); ++i)
						if (pType->weapon[i] != NULL && (pType->weapon[i]->range >> 1) > dx && !pType->weapon[i]->interceptor && !pType->weapon[i]->commandfire)
							dx = pType->weapon[i]->range >> 1;
					if (pType->kamikaze && pType->kamikazedistance > dx)
						dx = pType->kamikazedistance;
					const PlayerMask mask = toPlayerMask(ownerId);

					std::deque<UnitTKit::T> possibleTargets;
					for (int i = 0; i < NB_PLAYERS; ++i)
						if (i != ownerId && !(players.team[ownerId] & players.team[i]))
							units.kdTree[i]->maxDistanceQuery(possibleTargets, position, float(dx));

					for (std::deque<UnitTKit::T>::iterator i = possibleTargets.begin(); enemy_idx == -1 && i != possibleTargets.end(); ++i)
					{
						const int cur_idx = i->first->idx;
						const int x = i->first->cur_px;
						const int y = i->first->cur_py;
						const int cur_type_id = units.unit[cur_idx].typeId;
						if (x < 0 || x >= the_map->widthInHeightmapTiles - 1 || y < 0 || y >= the_map->heightInHeightmapTiles - 1 || cur_type_id == -1)
							continue;
						if (units.unit[cur_idx].flags && (units.unit[cur_idx].is_on_radar(mask) || ((the_map->sight_map(x >> 1, y >> 1) & mask) && !units.unit[cur_idx].cloaked)) && (canTargetGround || units.unit[cur_idx].flying) && !unit_manager.unit_type[cur_type_id]->checkCategory(pType->NoChaseCategory))
						{
							if (returning_fire)
							{
								for (uint32 i = 0; i < units.unit[cur_idx].weapon.size(); ++i)
								{
									if (units.unit[cur_idx].weapon[i].state != WEAPON_FLAG_IDLE && units.unit[cur_idx].weapon[i].target == this)
									{
										enemy_idx = cur_idx;
										break;
									}
								}
							}
							else
								enemy_idx = cur_idx;
						}
					}
					if (enemy_idx >= 0) // Si on a trouvé une unité, on l'attaque
					{
						if (do_nothing())
							set_mission(MISSION_ATTACK | MISSION_FLAG_AUTO, &(units.unit[enemy_idx].position), false, 0, true, &(units.unit[enemy_idx]));
						else if (!missionQueue.empty() && missionQueue->mission() == MISSION_PATROL)
						{
							add_mission(MISSION_MOVE | MISSION_FLAG_AUTO, &(position), true, 0);
							add_mission(MISSION_ATTACK | MISSION_FLAG_AUTO, &(units.unit[enemy_idx].position), true, 0, &(units.unit[enemy_idx]));
						}
						else
							for (uint32 i = 0; i < weapon.size(); ++i)
								if (weapon[i].state == WEAPON_FLAG_IDLE && pType->weapon[i] != NULL && !pType->weapon[i]->commandfire && !pType->weapon[i]->interceptor && (!pType->weapon[i]->toairweapon || (pType->weapon[i]->toairweapon && units.unit[enemy_idx].flying)) && !unit_manager.unit_type[units.unit[enemy_idx].typeId]->checkCategory(pType->NoChaseCategory))
								{
									weapon[i].state = WEAPON_FLAG_AIM;
									weapon[i].target = &(units.unit[enemy_idx]);
									weapon[i].data = -1;
								}
					}
				}
				if (weapon.size() > 0 && pType->antiweapons && pType->weapon[0])
				{
					const float coverage = pType->weapon[0]->coverage * pType->weapon[0]->coverage;
					const float range = float(pType->weapon[0]->range * pType->weapon[0]->range >> 2);
					int enemy_idx = -1;
					int e = 0;
					for (int i = 0; i + e < mem_size; ++i)
					{
						if (memory[i + e] >= weapons.nb_weapon || weapons.weapon[memory[i + e]].weapon_id == -1)
						{
							++e;
							--i;
							continue;
						}
						memory[i] = memory[i + e];
					}
					mem_size -= e;
					unlock();
					weapons.lock();
					for (std::vector<uint32>::iterator f = weapons.idx_list.begin(); f != weapons.idx_list.end(); ++f)
					{
						const uint32 i = *f;
						// Yes we don't defend against allies :D, can lead to funny situations :P
						if (weapons.weapon[i].weapon_id != -1 && !(players.team[units.unit[weapons.weapon[i].shooter_idx].ownerId] & players.team[ownerId]) && weapon_manager.weapon[weapons.weapon[i].weapon_id].targetable)
						{
							if (((Vector3D) (weapons.weapon[i].target_pos - position)).lengthSquared() <= coverage &&
								((Vector3D) (weapons.weapon[i].Pos - position)).lengthSquared() <= range)
							{
								int idx = -1;
								for (e = 0; e < mem_size; ++e)
								{
									if (memory[e] == i)
									{
										idx = i;
										break;
									}
								}
								if (idx == -1)
								{
									enemy_idx = i;
									if (mem_size < TA3D_PLAYERS_HARD_LIMIT)
									{
										memory[mem_size] = i;
										mem_size++;
									}
									break;
								}
							}
						}
					}
					weapons.unlock();
					lock();
					if (enemy_idx >= 0) // If we found a target, then attack it, here  we use attack because we need the mission list to act properly
						add_mission(MISSION_ATTACK | MISSION_FLAG_AUTO,
							&(weapons.weapon[enemy_idx].Pos),
							false, 0, &(weapons.weapon[enemy_idx]), 12); // 12 = 4 | 8, targets a weapon and automatic fire
				}
			}
		}

		if (pType->canfly) // Set plane orientation
		{
			Vector3D J, K(0.0f, 1.0f, 0.0f);
			J = velocity * K;

			Vector3D virtual_G;				  // Compute the apparent gravity force ( seen from the plane )
			virtual_G.x = virtual_G.z = 0.0f; // Standard gravity vector
			virtual_G.y = -4.0f * units.g_dt;
			float d = J.lengthSquared();
			if (!Math::AlmostZero(d))
				virtual_G = virtual_G + (((old_V - velocity) % J) / d) * J; // Add the opposite of the speed derivative projected on the side of the unit

			d = virtual_G.length();
			if (!Math::AlmostZero(d))
			{
				virtual_G = -1.0f / d * virtual_G;

				d = sqrtf(virtual_G.y * virtual_G.y + virtual_G.z * virtual_G.z);
				float angle_1 = !Math::AlmostZero(d) ? acosf(virtual_G.y / d) * RAD2DEG : 0.0f;
				if (virtual_G.z < 0.0f)
					angle_1 = -angle_1;
				virtual_G = virtual_G * RotateX(-angle_1 * DEG2RAD);
				float angle_2 = acosf(virtual_G % K) * RAD2DEG;
				if (virtual_G.x > 0.0f)
					angle_2 = -angle_2;

				if (fabsf(angle_1 - orientation.x) < 360.0f)
					orientation.x += 5.0f * dt * (angle_1 - orientation.x); // We need something continuous
				if (fabsf(angle_2 - orientation.z) < 360.0f)
					orientation.z += 5.0f * dt * (angle_2 - orientation.z);

				if (orientation.x < -360.0f || orientation.x > 360.0f)
					orientation.x = 0.0f;
				if (orientation.z < -360.0f || orientation.z > 360.0f)
					orientation.z = 0.0f;
			}
		}

		if (!isBeingBuilt())
		{

			// Change the unit's angle the way we need it to be changed

			if (hasTargetAngle && !isNaN(targetAngle) && pType->BMcode) // Don't remove the class check otherwise factories can spin
			{
				while (!isNaN(targetAngle) && fabsf(targetAngle - orientation.y) > 180.0f)
				{
					if (targetAngle < orientation.y)
						orientation.y -= 360.0f;
					else
						orientation.y += 360.0f;
				}
				if (!isNaN(targetAngle) && fabsf(targetAngle - orientation.y) >= 1.0f)
				{
					float aspeed = pType->TurnRate;
					if (targetAngle < orientation.y)
						aspeed = -aspeed;
					float a = targetAngle - orientation.y;
					angularVelocity.y = aspeed;
					float b = targetAngle - (orientation.y + dt * angularVelocity.y);
					if (((a < 0.0f && b > 0.0f) || (a > 0.0f && b < 0.0f)) && !isNaN(targetAngle))
					{
						angularVelocity.y = 0.0f;
						orientation.y = targetAngle;
					}
				}
			}

			orientation = orientation + dt * angularVelocity;
			Vector3D OPos = position;
			if (precomputed_position)
			{
				if (pType->canmove && pType->BMcode && !flying)
					velocity.y -= units.g_dt; // L'unité subit la force de gravitation
				position = NPos;
				position.y = OPos.y + velocity.y * dt;
				cur_px = n_px;
				cur_py = n_py;
			}
			else
			{
				if (pType->canmove && pType->BMcode)
					velocity.y -= units.g_dt; // L'unité subit la force de gravitation
				setPosition(position + (dt * velocity)); // move the unit
			}
		}
	script_exec:
		if (!attached && (pType->canmove || first_move))
		{
			bool hover_on_water = false;
			float min_h = the_map->get_unit_h(position.x, position.z);
			h = position.y - min_h;
			if (!pType->Floater && !pType->canfly && !pType->canhover && h > 0.0f && Math::AlmostZero(pType->WaterLine))
				position.y = min_h;
			else if (pType->canhover && position.y <= the_map->sealvl)
			{
				hover_on_water = true;
				position.y = the_map->sealvl;
				if (velocity.y < 0.0f)
					velocity.y = 0.0f;
			}
			else if (pType->Floater)
			{
				position.y = the_map->sealvl + (float)pType->AltFromSeaLevel * H_DIV;
				velocity.y = 0.0f;
			}
			else if (!Math::AlmostZero(pType->WaterLine))
			{
				position.y = the_map->sealvl - pType->WaterLine * H_DIV;
				velocity.y = 0.0f;
			}
			else if (!pType->canfly && position.y > Math::Max(min_h, the_map->sealvl) && pType->BMcode) // Prevent non flying units from "jumping"
			{
				position.y = Math::Max(min_h, the_map->sealvl);
				if (velocity.y < 0.0f)
					velocity.y = 0.0f;
			}
			if (pType->canhover)
			{
				int param[1] = {hover_on_water ? (the_map->sealvl - min_h >= 8.0f ? 2 : 1) : 4};
				runScriptFunction(SCRIPT_setSFXoccupy, 1, param);
			}
			if (min_h > position.y)
			{
				position.y = min_h;
				if (velocity.y < 0.0f)
					velocity.y = 0.0f;
			}
			if (pType->canfly && !isBeingBuilt() && local)
			{
				if (!missionQueue.empty() && ((missionQueue->getFlags() & MISSION_FLAG_MOVE) || missionQueue->mission() == MISSION_BUILD || missionQueue->mission() == MISSION_BUILD_2 || missionQueue->mission() == MISSION_REPAIR || missionQueue->mission() == MISSION_ATTACK || missionQueue->mission() == MISSION_MOVE || missionQueue->mission() == MISSION_GUARD || missionQueue->mission() == MISSION_GET_REPAIRED || missionQueue->mission() == MISSION_PATROL || missionQueue->mission() == MISSION_RECLAIM || nb_attached > 0 || position.x < -the_map->halfWidthInWorldUnits || position.x > the_map->halfWidthInWorldUnits || position.z < -the_map->halfHeightInWorldUnits || position.z > the_map->halfHeightInWorldUnits))
				{
					if (!(missionQueue->mission() == MISSION_GET_REPAIRED && (missionQueue->getFlags() & MISSION_FLAG_BEING_REPAIRED)))
					{
						const float ideal_h = Math::Max(min_h, the_map->sealvl) + (float)pType->CruiseAlt * H_DIV;
						velocity.y = (ideal_h - position.y) * 2.0f;
					}
					flying = true;
				}
				else
				{
					if (can_be_there(cur_px, cur_py, typeId, ownerId, idx)) // Check it can be there
					{
						float ideal_h = min_h;
						velocity.y = (ideal_h - position.y) * 1.5f;
						flying = false;
					}
					else // There is someone there, find an other place to land
					{
						flying = true;
						if (do_nothing())				// Wait for MISSION_STOP to check if we have some work to do
						{								// This prevents planes from keeping looking for a place to land
							Vector3D next_target = position; // instead of going back to work :/
							const float find_angle = float(Math::RandomTable() % 360) * DEG2RAD;
							next_target.x += cosf(find_angle) * float(32 + pType->FootprintX * 8);
							next_target.z += sinf(find_angle) * float(32 + pType->FootprintZ * 8);
							add_mission(MISSION_MOVE | MISSION_FLAG_AUTO, &next_target, true);
						}
					}
				}
			}
			port[GROUND_HEIGHT] = (sint16)(position.y - min_h + 0.5f);
		}
		port[HEALTH] = (sint16)((int)hp * 100 / pType->MaxDamage);

		// Update moving animation state
		if (requestedMovingAnimationState ^ movingAnimation)
		{
			if (!requestedMovingAnimationState)
				launchScript(SCRIPT_StopMoving);
			else
			{
				if (pType->canfly)
					activate();
				launchScript(SCRIPT_startmoving);
				if (nb_attached == 0)
					launchScript(SCRIPT_MoveRate1); // For the armatlas
				else
					launchScript(SCRIPT_MoveRate2);
			}
			movingAnimation = requestedMovingAnimationState;
		}

		if (script)
			script->run(dt);
		yardmap_timer--;
		if (hp > 0.0f && (((o_px != cur_px || o_py != cur_py || first_move || (was_flying ^ flying) || ((port[YARD_OPEN] != 0) ^ was_open) || yardmap_timer == 0) && build_percent_left <= 0.0f) || !drawn || (drawn && drawn_obstacle != is_obstacle())))
		{
			first_move = build_percent_left > 0.0f;
			pMutex.unlock();
			draw_on_map();
			pMutex.lock();
			yardmap_timer = TICKS_PER_SEC + (Math::RandomTable() & 15);
		}

		built = false;
		attacked = false;
		pMutex.unlock();
		return 0;
	}

	bool Unit::hit(const Vector3D& P, const Vector3D& Dir, Vector3D* hit_vec, const float length)
	{
		MutexLocker mLock(pMutex);
		if (!isAlive())
			return false;
		if (model)
		{
			const Vector3D c_dir = model->center + position - P;
			if (c_dir.length() - length <= model->size2)
			{
				const UnitType* pType = unit_manager.unit_type[typeId];
				const float scale = pType->Scale;
				const Matrix M = RotateXZY(-orientation.x * DEG2RAD, -orientation.z * DEG2RAD, -orientation.y * DEG2RAD) * Scale(1.0f / scale);
				const Vector3D RP = (P - position) * M;
				const bool is_hit = model->hit(RP, Dir, &data, hit_vec, M) >= -1;
				if (is_hit)
				{
					*hit_vec = ((*hit_vec) * RotateYZX(orientation.y * DEG2RAD, orientation.z * DEG2RAD, orientation.x * DEG2RAD)) * Scale(scale) + position;
					*hit_vec = ((*hit_vec - P) % Dir) * Dir + P;
				}

				return is_hit;
			}
		}
		return false;
	}

	bool Unit::hit_fast(const Vector3D& P, const Vector3D& Dir, Vector3D* hit_vec, const float length)
	{
		MutexLocker mLock(pMutex);
		if (!isAlive())
			return false;
		if (model)
		{
			const Vector3D c_dir = model->center + position - P;
			if (c_dir.lengthSquared() <= (model->size2 + length) * (model->size2 + length))
			{
				const UnitType* pType = unit_manager.unit_type[typeId];
				const float scale = pType->Scale;
				const Matrix M = RotateXZY(-orientation.x * DEG2RAD, -orientation.z * DEG2RAD, -orientation.y * DEG2RAD) * Scale(1.0f / scale);
				const Vector3D RP = (P - position) * M;
				const bool is_hit = model->hit_fast(RP, Dir, &data, hit_vec, M);
				if (is_hit)
				{
					*hit_vec = ((*hit_vec) * RotateYZX(orientation.y * DEG2RAD, orientation.z * DEG2RAD, orientation.x * DEG2RAD)) * Scale(scale) + position;
					*hit_vec = ((*hit_vec - P) % Dir) * Dir + P;
				}

				return is_hit;
			}
		}
		return false;
	}

	void Unit::show_orders(bool only_build_commands, bool def_orders) // Dessine les ordres reçus
	{
		if (!def_orders)
			show_orders(only_build_commands, true);

		pMutex.lock();

		if (!isAlive())
		{
			pMutex.unlock();
			return;
		}

		MissionQueue::iterator cur = def_orders ? defaultMissionQueue.begin() : missionQueue.begin();
		MissionQueue::iterator end = def_orders ? defaultMissionQueue.end() : missionQueue.end();

		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);

		Vector3D p_target = position;
		Vector3D n_target = position;
		const float rab = float(MILLISECONDS_SINCE_INIT % 1000) * 0.001f;
		const UnitType* pType = unit_manager.unit_type[typeId];
		uint32 remaining_build_commands = !(pType->BMcode) ? 0 : 0xFFFFFFF;

		std::vector<Vector3D> points;

		while (cur != end)
		{
			Unit* p = cur->lastStep().getTarget().getUnit();
			if (!only_build_commands)
			{
				const int curseur = anim_cursor(CURSOR_CROSS_LINK);
				const float dx = 0.5f * (float)cursor[CURSOR_CROSS_LINK].ofs_x[curseur];
				const float dz = 0.5f * (float)cursor[CURSOR_CROSS_LINK].ofs_y[curseur];
				float x, y, z;
				const float dist = ((Vector3D) (cur->lastStep().getTarget().getPos() - p_target)).length();
				const int rec = (int)(dist / 30.0f);
				switch (cur->lastMission())
				{
					case MISSION_LOAD:
					case MISSION_UNLOAD:
					case MISSION_GUARD:
					case MISSION_PATROL:
					case MISSION_MOVE:
					case MISSION_BUILD:
					case MISSION_BUILD_2:
					case MISSION_REPAIR:
					case MISSION_ATTACK:
					case MISSION_RECLAIM:
					case MISSION_REVIVE:
					case MISSION_CAPTURE:
						if (cur->lastStep().getFlags() & MISSION_FLAG_TARGET_WEAPON)
						{
							++cur;
							continue; // Don't show this, it'll be removed
						}
						n_target = cur->lastStep().getTarget().getPos();
						n_target.y = Math::Max(the_map->get_unit_h(n_target.x, n_target.z), the_map->sealvl);
						if (rec > 0)
						{
							for (int i = 0; i < rec; ++i)
							{
								x = p_target.x + (n_target.x - p_target.x) * ((float)i + rab) / (float)rec;
								z = p_target.z + (n_target.z - p_target.z) * ((float)i + rab) / (float)rec;
								y = Math::Max(the_map->get_unit_h(x, z), the_map->sealvl);
								y += 0.75f;
								x -= dx;
								z -= dz;
								points.push_back(Vector3D(x, y, z));
							}
						}
						p_target = n_target;
				}
			}
			glDisable(GL_DEPTH_TEST);
			Vector3D target = cur->lastStep().getTarget().getPos();
			switch (cur->lastMission())
			{
				case MISSION_BUILD:
					if (p != NULL)
						target = p->position;
					if (cur->lastStep().getData() >= 0 && cur->lastStep().getData() < unit_manager.nb_unit && remaining_build_commands > 0)
					{
						--remaining_build_commands;
						const float DX = float(unit_manager.unit_type[cur->lastStep().getData()]->FootprintX << 2);
						const float DZ = float(unit_manager.unit_type[cur->lastStep().getData()]->FootprintZ << 2);
						const byte blue = only_build_commands ? 0xFF : 0x00, green = only_build_commands ? 0x00 : 0xFF;
						glPushMatrix();
						glTranslatef(target.x, Math::Max(target.y, the_map->sealvl), target.z);
						glDisable(GL_CULL_FACE);
						glDisable(GL_TEXTURE_2D);
						glEnable(GL_BLEND);
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
						glBegin(GL_QUADS);
						glColor4ub(0x00, green, blue, 0xFF);
						glVertex3f(-DX, 0.0f, -DZ); // First quad
						glVertex3f(DX, 0.0f, -DZ);
						glColor4ub(0x00, green, blue, 0x00);
						glVertex3f(DX + 2.0f, 5.0f, -DZ - 2.0f);
						glVertex3f(-DX - 2.0f, 5.0f, -DZ - 2.0f);

						glColor4ub(0x00, green, blue, 0xFF);
						glVertex3f(-DX, 0.0f, -DZ); // Second quad
						glVertex3f(-DX, 0.0f, DZ);
						glColor4ub(0x00, green, blue, 0x00);
						glVertex3f(-DX - 2.0f, 5.0f, DZ + 2.0f);
						glVertex3f(-DX - 2.0f, 5.0f, -DZ - 2.0f);

						glColor4ub(0x00, green, blue, 0xFF);
						glVertex3f(DX, 0.0f, -DZ); // Third quad
						glVertex3f(DX, 0.0f, DZ);
						glColor4ub(0x00, green, blue, 0x00);
						glVertex3f(DX + 2.0f, 5.0f, DZ + 2.0f);
						glVertex3f(DX + 2.0f, 5.0f, -DZ - 2.0f);

						glEnd();
						glDisable(GL_BLEND);
						glEnable(GL_TEXTURE_2D);
						glEnable(GL_CULL_FACE);
						glPopMatrix();
						if (unit_manager.unit_type[cur->lastStep().getData()]->model != NULL)
						{
							glEnable(GL_LIGHTING);
							glEnable(GL_CULL_FACE);
							glEnable(GL_DEPTH_TEST);
							glPushMatrix();
							glTranslatef(target.x, target.y, target.z);
							glColor4ub(0x00, green, blue, 0x7F);
							glDepthFunc(GL_GREATER);
							unit_manager.unit_type[cur->lastStep().getData()]->model->mesh->draw(0.0f, NULL, false, false, false);
							glDepthFunc(GL_LESS);
							unit_manager.unit_type[cur->lastStep().getData()]->model->mesh->draw(0.0f, NULL, false, false, false);
							glPopMatrix();
							glEnable(GL_BLEND);
							glEnable(GL_TEXTURE_2D);
							glDisable(GL_LIGHTING);
							glDisable(GL_CULL_FACE);
							glDisable(GL_DEPTH_TEST);
							glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
						}
						glPushMatrix();
						glTranslatef(target.x, Math::Max(target.y, the_map->sealvl), target.z);
						glDisable(GL_CULL_FACE);
						glDisable(GL_TEXTURE_2D);
						glEnable(GL_BLEND);
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
						glBegin(GL_QUADS);
						glColor4ub(0x00, green, blue, 0xFF);
						glVertex3f(-DX, 0.0f, DZ); // Fourth quad
						glVertex3f(DX, 0.0f, DZ);
						glColor4ub(0x00, green, blue, 0x00);
						glVertex3f(DX + 2.0f, 5.0f, DZ + 2.0f);
						glVertex3f(-DX - 2.0f, 5.0f, DZ + 2.0f);
						glEnd();
						glPopMatrix();
						glEnable(GL_BLEND);
						glEnable(GL_TEXTURE_2D);
						glDisable(GL_CULL_FACE);
						glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
					}
					break;
				case MISSION_UNLOAD:
				case MISSION_LOAD:
				case MISSION_MOVE:
				case MISSION_BUILD_2:
				case MISSION_REPAIR:
				case MISSION_RECLAIM:
				case MISSION_REVIVE:
				case MISSION_PATROL:
				case MISSION_GUARD:
				case MISSION_ATTACK:
				case MISSION_CAPTURE:
					if (!only_build_commands)
					{
						if (p != NULL)
							target = p->position;
						int cursor_type = CURSOR_ATTACK;
						switch (cur->lastMission())
						{
							case MISSION_GUARD:
								cursor_type = CURSOR_GUARD;
								break;
							case MISSION_ATTACK:
								cursor_type = CURSOR_ATTACK;
								break;
							case MISSION_PATROL:
								cursor_type = CURSOR_PATROL;
								break;
							case MISSION_RECLAIM:
								cursor_type = CURSOR_RECLAIM;
								break;
							case MISSION_BUILD_2:
							case MISSION_REPAIR:
								cursor_type = CURSOR_REPAIR;
								break;
							case MISSION_MOVE:
								cursor_type = CURSOR_MOVE;
								break;
							case MISSION_LOAD:
								cursor_type = CURSOR_LOAD;
								break;
							case MISSION_UNLOAD:
								cursor_type = CURSOR_UNLOAD;
								break;
							case MISSION_REVIVE:
								cursor_type = CURSOR_REVIVE;
								break;
							case MISSION_CAPTURE:
								cursor_type = CURSOR_CAPTURE;
								break;
						}
						const int curseur = anim_cursor(cursor_type);
						const float x = target.x - 0.5f * (float)cursor[cursor_type].ofs_x[curseur];
						const float y = target.y + 1.0f;
						const float z = target.z - 0.5f * (float)cursor[cursor_type].ofs_y[curseur];
						const float sx = 0.5f * float(cursor[cursor_type].bmp[curseur]->w - 1);
						const float sy = 0.5f * float(cursor[cursor_type].bmp[curseur]->h - 1);

						glBindTexture(GL_TEXTURE_2D, cursor[cursor_type].glbmp[curseur]);
						glBegin(GL_QUADS);
						glTexCoord2f(0.0f, 0.0f);
						glVertex3f(x, y, z);
						glTexCoord2f(1.0f, 0.0f);
						glVertex3f(x + sx, y, z);
						glTexCoord2f(1.0f, 1.0f);
						glVertex3f(x + sx, y, z + sy);
						glTexCoord2f(0.0f, 1.0f);
						glVertex3f(x, y, z + sy);
						glEnd();
					}
					break;
			}
			glEnable(GL_DEPTH_TEST);
			++cur;
		}

		if (!points.empty())
		{
			const int curseur = anim_cursor(CURSOR_CROSS_LINK);
			const float sx = 0.5f * float(cursor[CURSOR_CROSS_LINK].bmp[curseur]->w - 1);
			const float sy = 0.5f * float(cursor[CURSOR_CROSS_LINK].bmp[curseur]->h - 1);

			Vector3D* P = new Vector3D[points.size() << 2];
			float* T = new float[points.size() << 3];

			int n = 0;
			for (std::vector<Vector3D>::const_iterator i = points.begin(); i != points.end(); ++i)
			{
				P[n] = *i;
				T[n << 1] = 0.0f;
				T[(n << 1) | 1] = 0.0f;
				++n;

				P[n] = *i;
				P[n].x += sx;
				T[n << 1] = 1.0f;
				T[(n << 1) | 1] = 0.0f;
				++n;

				P[n] = *i;
				P[n].x += sx;
				P[n].z += sy;
				T[n << 1] = 1.0f;
				T[(n << 1) | 1] = 1.0f;
				++n;

				P[n] = *i;
				P[n].z += sy;
				T[n << 1] = 0.0f;
				T[(n << 1) | 1] = 1.0f;
				++n;
			}

			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

			glVertexPointer(3, GL_FLOAT, 0, P);
			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			glTexCoordPointer(2, GL_FLOAT, 0, T);
			glBindTexture(GL_TEXTURE_2D, cursor[CURSOR_CROSS_LINK].glbmp[curseur]);

			glDrawArrays(GL_QUADS, 0, n);

			DELETE_ARRAY(P);
			DELETE_ARRAY(T);
		}
		glDisable(GL_BLEND);
		pMutex.unlock();
	}

	int Unit::shoot(const int target, const Vector3D& startpos, const Vector3D& Dir, const int w_id, const Vector3D& target_pos)
	{
		const UnitType* pType = unit_manager.unit_type[typeId];
		const WeaponDef* pW = pType->weapon[w_id]; // Critical information, we can't lose it so we save it before unlocking this unit
		const int owner = ownerId;
		const Vector3D D = Dir * RotateY(-orientation.y * DEG2RAD);
		int param[] = {(int)(-10.0f * DEG2TA * D.z), (int)(-10.0f * DEG2TA * D.x)};
		launchScript(SCRIPT_RockUnit, 2, param);

		if (pW->startsmoke && visible)
			particle_engine.make_smoke(startpos, 0, 1, 0.0f, -1.0f, 0.0f, 0.3f);

		pMutex.unlock();

		weapons.lock();

		const int w_idx = weapons.add_weapon(pW->nb_id, idx);

		if (network_manager.isConnected() && local) // Send synchronization packet
		{
			struct event event;
			event.type = EVENT_WEAPON_CREATION;
			event.opt1 = idx;
			event.opt2 = (uint16)target;
			event.opt3 = units.current_tick; // Will be used to extrapolate those data on client side
			event.opt4 = pW->damage;
			event.opt5 = ownerId;
			event.x = target_pos.x;
			event.y = target_pos.y;
			event.z = target_pos.z;
			event.vx = startpos.x;
			event.vy = startpos.y;
			event.vz = startpos.z;
			event.dx = (sint16)(Dir.x * 16384.0f);
			event.dy = (sint16)(Dir.y * 16384.0f);
			event.dz = (sint16)(Dir.z * 16384.0f);
			memcpy(event.str, pW->internal_name.c_str(), pW->internal_name.size() + 1);

			network_manager.sendEvent(&event);
		}

		weapons.weapon[w_idx].damage = (float)pW->damage;
		weapons.weapon[w_idx].Pos = startpos;
		weapons.weapon[w_idx].local = local;
		if (Math::AlmostZero(pW->startvelocity) && !pW->selfprop)
			weapons.weapon[w_idx].V = pW->weaponvelocity * Dir;
		else
			weapons.weapon[w_idx].V = pW->startvelocity * Dir;
		weapons.weapon[w_idx].V = weapons.weapon[w_idx].V + velocity;
		weapons.weapon[w_idx].owner = (byte)owner;
		weapons.weapon[w_idx].target = target;
		if (target >= 0)
		{
			if (pW->interceptor)
				weapons.weapon[w_idx].target_pos = weapons.weapon[target].Pos;
			else
				weapons.weapon[w_idx].target_pos = target_pos;
		}
		else
			weapons.weapon[w_idx].target_pos = target_pos;

		weapons.weapon[w_idx].stime = 0.0f;
		weapons.weapon[w_idx].visible = visible; // Not critical so we don't duplicate this
		weapons.unlock();
		pMutex.lock();
		return w_idx;
	}

	void Unit::draw_on_map()
	{
		const int type = typeId;
		if (type == -1 || !isAlive())
			return;

		if (drawn)
			clear_from_map();
		if (attached)
			return;

		pMutex.lock();
		drawn_obstacle = is_obstacle();
		pMutex.unlock();
		drawn_flying = flying;
		const UnitType* const pType = unit_manager.unit_type[type];
		if (!flying)
		{
			// First check we're on a "legal" place if it can move
			pMutex.lock();
			if (pType->canmove && pType->BMcode && !can_be_there(cur_px, cur_py, typeId, ownerId))
			{
				// Try to find a suitable place

				bool found = false;
				for (int r = 1; r < 20 && !found; r++) // Circular check
				{
					const int r2 = r * r;
					for (int y = 0; y <= r; ++y)
					{
						const int x = (int)(sqrtf(float(r2 - y * y)) + 0.5f);
						if (can_be_there(cur_px + x, cur_py + y, typeId, ownerId))
						{
							cur_px += x;
							cur_py += y;
							found = true;
							break;
						}
						if (can_be_there(cur_px - x, cur_py + y, typeId, ownerId))
						{
							cur_px -= x;
							cur_py += y;
							found = true;
							break;
						}
						if (can_be_there(cur_px + x, cur_py - y, typeId, ownerId))
						{
							cur_px += x;
							cur_py -= y;
							found = true;
							break;
						}
						if (can_be_there(cur_px - x, cur_py - y, typeId, ownerId))
						{
							cur_px -= x;
							cur_py -= y;
							found = true;
							break;
						}
					}
				}
				if (found)
				{
					auto worldPosition = the_map->heightmapIndexToWorld(Point<int>(cur_px, cur_py));
					position.x = worldPosition.x;
					position.z = worldPosition.y;
					if (!missionQueue.empty() && (missionQueue->getFlags() & MISSION_FLAG_MOVE))
						missionQueue->Flags() |= MISSION_FLAG_REFRESH_PATH;
				}
				else
				{
					LOG_ERROR(LOG_PREFIX_BATTLE << "units overlaps on yardmap !!");
				}
			}
			pMutex.unlock();

			if (!(pType->canmove && pType->BMcode) || drawn_obstacle)
				the_map->obstaclesRect(cur_px - (pType->FootprintX >> 1),
					cur_py - (pType->FootprintZ >> 1),
					pType->FootprintX, pType->FootprintZ, true,
					pType->yardmap, port[YARD_OPEN] != 0);
			the_map->rect(cur_px - (pType->FootprintX >> 1),
				cur_py - (pType->FootprintZ >> 1),
				pType->FootprintX, pType->FootprintZ,
				idx, pType->yardmap, port[YARD_OPEN] != 0);
			the_map->energy.add(pType->gRepulsion,
				cur_px - (pType->gRepulsion.getWidth() >> 1),
				cur_py - (pType->gRepulsion.getHeight() >> 1));
			drawn_open = port[YARD_OPEN] != 0;
		}
		drawn_x = cur_px;
		drawn_y = cur_py;
		drawn = true;
	}

	void Unit::clear_from_map()
	{
		if (!drawn)
			return;

		const int type = typeId;

		if (type == -1 || !isAlive())
			return;

		const UnitType* const pType = unit_manager.unit_type[type];
		drawn = false;
		if (!drawn_flying)
		{
			if (!(pType->canmove && pType->BMcode) || drawn_obstacle)
				the_map->obstaclesRect(cur_px - (pType->FootprintX >> 1),
					cur_py - (pType->FootprintZ >> 1),
					pType->FootprintX, pType->FootprintZ, false,
					pType->yardmap, drawn_open);
			the_map->rect(drawn_x - (pType->FootprintX >> 1),
				drawn_y - (pType->FootprintZ >> 1),
				pType->FootprintX, pType->FootprintZ,
				-1, pType->yardmap, drawn_open);
			the_map->energy.sub(pType->gRepulsion,
				drawn_x - (pType->gRepulsion.getWidth() >> 1),
				drawn_y - (pType->gRepulsion.getHeight() >> 1));
		}
	}

	void Unit::draw_on_FOW(bool jamming)
	{
		if (hidden || isBeingBuilt())
			return;

		const int unit_type = typeId;

		if (flags == 0 || unit_type == -1)
			return;

		const bool system_activated = (port[ACTIVATION] && unit_manager.unit_type[unit_type]->onoffable) || !unit_manager.unit_type[unit_type]->onoffable;

		if (jamming)
		{
			radar_jam_range = system_activated ? (unit_manager.unit_type[unit_type]->RadarDistanceJam / MAP::GraphicalTileWidthInWorldUnits) : 0;
			sonar_jam_range = system_activated ? (unit_manager.unit_type[unit_type]->SonarDistanceJam / MAP::GraphicalTileWidthInWorldUnits) : 0;

			the_map->update_player_visibility(ownerId, cur_px, cur_py, 0, 0, 0, radar_jam_range, sonar_jam_range, true);
		}
		else
		{
			uint32 cur_sight = ((int)h + unit_manager.unit_type[unit_type]->SightDistance) / MAP::GraphicalTileWidthInWorldUnits;
			radar_range = system_activated ? (unit_manager.unit_type[unit_type]->RadarDistance / MAP::GraphicalTileWidthInWorldUnits) : 0;
			sonar_range = system_activated ? (unit_manager.unit_type[unit_type]->SonarDistance / MAP::GraphicalTileWidthInWorldUnits) : 0;

			the_map->update_player_visibility(ownerId, cur_px, cur_py, cur_sight, radar_range, sonar_range, 0, 0, false, old_px != cur_px || old_py != cur_py || cur_sight != sight);

			sight = cur_sight;
			old_px = cur_px;
			old_py = cur_py;
		}
	}

	bool Unit::playSound(const String& key)
	{
		bool bPlayed = false;
		pMutex.lock();
		if (isOwnedBy(players.local_human_id) && int(MILLISECONDS_SINCE_INIT - last_time_sound) >= units.sound_min_ticks)
		{
			last_time_sound = MILLISECONDS_SINCE_INIT;
			const UnitType* pType = unit_manager.unit_type[typeId];
			sound_manager->playTDFSound(pType->soundcategory, key, &position);
			bPlayed = true;
		}
		pMutex.unlock();
		return bPlayed;
	}

	int Unit::launchScript(const int id, int nb_param, int* param) // Start a script as a separate "thread" of the unit
	{
		const int type = typeId;
		if (!script || type == -1 || !unit_manager.unit_type[type]->script || !unit_manager.unit_type[type]->script->isCached(id))
			return -2;
		const String& f_name = UnitScriptInterface::get_script_name(id);
		if (f_name.empty())
			return -2;

		MutexLocker locker(pMutex);

		if (local && network_manager.isConnected()) // Send synchronization event
		{
			struct event event;
			event.type = EVENT_UNIT_SCRIPT;
			event.opt1 = idx;
			event.opt2 = (uint16)id;
			event.opt3 = nb_param;
			memcpy(event.str, param, sizeof(int) * nb_param);
			network_manager.sendEvent(&event);
		}

		ScriptInterface* newThread = script->fork(f_name, param, nb_param);

		if (newThread == NULL || !newThread->is_self_running())
		{
			unit_manager.unit_type[type]->script->Uncache(id);
			return -2;
		}

		return 0;
	}

	int Unit::get_sweet_spot()
	{
		if (typeId < 0)
			return -1;
		UnitType* pType = unit_manager.unit_type[typeId];
		if (pType->sweetspot_cached == -1)
		{
			lock();
			pType->sweetspot_cached = runScriptFunction(SCRIPT_SweetSpot);
			unlock();
		}
		return pType->sweetspot_cached;
	}

	void Unit::drawHealthBar() const
	{
		if (render.type_id < 0 || isNotOwnedBy(players.local_human_id) || render.UID != ID)
			return;

		Vector3D up = Camera::inGame->up();
		Vector3D side = Camera::inGame->side();

		const int maxdmg = unit_manager.unit_type[render.type_id]->MaxDamage;
		Vector3D vPos = render.Pos;
		const float size = unit_manager.unit_type[render.type_id]->model->size2 * 0.5f;

		const float scale = 200.0f;
		float w = 0.04f * scale;
		float h = 0.006f * scale;
		vPos -= size * up;
		units.hbars_bkg.push_back(vPos - w * side + h * up);
		units.hbars_bkg.push_back(vPos + w * side + h * up);
		units.hbars_bkg.push_back(vPos + w * side - h * up);
		units.hbars_bkg.push_back(vPos - w * side - h * up);

		w -= scale * gfx->SCREEN_W_INV;
		h -= scale * gfx->SCREEN_H_INV;

		if (hp <= 0.0f)
			return;

		uint32 color = makeacol(0x50, 0xD0, 0x50, 0xFF);
		if (hp <= (maxdmg >> 2))
			color = makeacol(0xFF, 0x46, 0x00, 0xFF);
		else if (hp <= (maxdmg >> 1))
			color = makeacol(0xFF, 0xD0, 0x00, 0xFF);
		units.hbars_color.push_back(color);
		units.hbars_color.push_back(color);
		units.hbars_color.push_back(color);
		units.hbars_color.push_back(color);
		const float pw = w * (2.0f * (hp / (float)maxdmg) - 1.0f);
		units.hbars.push_back(vPos - w * side + h * up);
		units.hbars.push_back(vPos + pw * side + h * up);
		units.hbars.push_back(vPos + pw * side - h * up);
		units.hbars.push_back(vPos - w * side - h * up);
	}

	void Unit::renderTick()
	{
		render.UID = ID;
		render.Pos = position;
		render.Angle = orientation;
		lock();
		render.Anim = data;
		unlock();
		render.px = cur_px;
		render.py = cur_py;
		render.type_id = typeId;
	}

	bool Unit::isOwnedBy(const PlayerId playerId) const
	{
		return playerId == ownerId;
	}

	bool Unit::isNotOwnedBy(const PlayerId playerId) const
	{
		return playerId != ownerId;
	}

	bool Unit::isAlive() const
	{
		return (flags & 1) != 0;
	}

	bool Unit::isBeingBuilt() const
	{
		return !Math::AlmostZero(build_percent_left);
	}

	bool Unit::isSelectableBy(PlayerId playerId) const
	{
		return isAlive() && isOwnedBy(playerId) && !isBeingBuilt();
	}

	void Unit::setPosition(const Vector3D& newPosition)
	{
		position = newPosition;
		auto heightmapPosition = the_map->worldToHeightmapIndex(newPosition);
		cur_px = heightmapPosition.x;
		cur_py = heightmapPosition.y;
	}

	void Unit::doWaitMission(Mission& mission)
	{
		mission.setFlags(0); // Don't move, do not shoot !! just wait
		if (mission.getTime() >= (float)mission.getData() * 0.001f) // Done :)
			next_mission();
	}

	void Unit::doWaitAttackMission(Mission& mission)
	{
		if (mission.getData() < 0 || mission.getData() >= (int)units.max_unit || !units.unit[mission.getData()].isAlive())
			next_mission();
		else if (units.unit[mission.getData()].attacked)
			next_mission();
	}

	void Unit::doGetRepairedMission(Mission& mission, float dt)
	{
		const auto pType = unit_manager.unit_type[typeId];

		if (!mission.getTarget().isValid())
		{
			next_mission();
			return;
		}
		if (mission.getTarget().isUnit() && mission.getUnit() && mission.getUnit()->isAlive())
		{
			Unit* target_unit = mission.getUnit();

			if (!(mission.getFlags() & MISSION_FLAG_PAD_CHECKED))
			{
				mission.Flags() |= MISSION_FLAG_PAD_CHECKED;
				int param[] = {0, 1};
				target_unit->runScriptFunction(SCRIPT_QueryLandingPad, 2, param);
				mission.setData(param[0]);
			}

			target_unit->compute_model_coord();
			const int piece_id = mission.getData() >= 0 ? mission.getData() : (-mission.getData() - 1);
			const Vector3D target = target_unit->position + target_unit->data.data[piece_id].pos;

			Vector3D Dir = target - position;
			Dir.y = 0.0f;
			const float dist = Dir.lengthSquared();
			const int maxdist = 6;
			if (dist > maxdist * maxdist && pType->BMcode) // Si l'unité est trop loin du chantier
			{
				unsetFlag(mission.Flags(), MISSION_FLAG_BEING_REPAIRED);
				c_time = 0.0f;
				setFlag(mission.Flags(), MISSION_FLAG_MOVE);
				mission.setMoveData(maxdist * 8 / 80);
				if (!mission.Path().empty())
					mission.Path().setPos(target); // Update path in real time!
			}
			else if (!(mission.getFlags() & MISSION_FLAG_MOVE))
			{
				hasTargetAngle = true;
				targetAngle = target_unit->orientation.y;
				if (mission.getData() >= 0)
				{
					setFlag(mission.Flags(), MISSION_FLAG_BEING_REPAIRED);
					Dir = target - position;
					position = position + 3.0f * dt * Dir;
					position.x = target.x;
					position.z = target.z;
					if (Dir.lengthSquared() < 3.0f)
					{
						target_unit->lock();
						if (target_unit->pad1 != 0xFFFF && target_unit->pad2 != 0xFFFF) // We can't land here
						{
							target_unit->unlock();
							next_mission();
							if (!missionQueue.empty() && mission.mission() == MISSION_STOP) // Don't stop we were patroling
								next_mission();
							return;
						}
						if (target_unit->pad1 == 0xFFFF) // tell others we're here
							target_unit->pad1 = (uint16)piece_id;
						else
							target_unit->pad2 = (uint16)piece_id;
						target_unit->unlock();
						mission.setData(-mission.getData() - 1);
					}
				}
				else
				{ // being repaired
					position = target;
					velocity.reset();

					if (target_unit->port[ACTIVATION])
					{
						const float conso_energy = float(unit_manager.unit_type[target_unit->typeId]->WorkerTime * pType->BuildCostEnergy) / float(pType->BuildTime);
						TA3D::players.requested_energy[ownerId] += conso_energy;
						if (players.energy[ownerId] >= (energy_cons + conso_energy * TA3D::players.energy_factor[ownerId]) * dt)
						{
							target_unit->lock();
							target_unit->energy_cons += conso_energy * TA3D::players.energy_factor[ownerId];
							target_unit->unlock();
							hp += dt * TA3D::players.energy_factor[ownerId] * float(unit_manager.unit_type[target_unit->typeId]->WorkerTime * pType->MaxDamage) / (float)pType->BuildTime;
						}
						if (hp >= pType->MaxDamage) // Unit has been repaired
						{
							hp = (float)pType->MaxDamage;
							target_unit->lock();
							if (target_unit->pad1 == piece_id) // tell others we've left
								target_unit->pad1 = 0xFFFF;
							else
								target_unit->pad2 = 0xFFFF;
							target_unit->unlock();
							next_mission();
							if (!missionQueue.empty() && mission.mission() == MISSION_STOP) // Don't stop we were patroling
								next_mission();
							return;
						}
						built = true;
					}
				}
			}
			else
				stopMoving();
		}
		else
			next_mission();
	}

	void Unit::doStandbyMineMission(Mission& mission)
	{
		const auto pType = unit_manager.unit_type[typeId];

		if (self_destruct < 0.0f)
		{
			int dx = ((pType->SightDistance + (int)(h + 0.5f)) >> 3) + 1;
			int enemy_idx = -1;
			int sx = Math::RandomTable() & 1;
			int sy = Math::RandomTable() & 1;
			for (int y = cur_py - dx + sy; y <= cur_py + dx; y += 2)
			{
				if (y >= 0 && y < the_map->heightInHeightmapTiles - 1)
					for (int x = cur_px - dx + sx; x <= cur_px + dx; x += 2)
						if (x >= 0 && x < the_map->widthInHeightmapTiles - 1)
						{
							const int cur_idx = the_map->map_data(x, y).unit_idx;
							if (cur_idx >= 0 && cur_idx < (int)units.max_unit && units.unit[cur_idx].isAlive() && units.unit[cur_idx].isNotOwnedBy(ownerId) && unit_manager.unit_type[units.unit[cur_idx].typeId]->ShootMe) // This unit is on the sight_map since dx = sightdistance !!
							{
								enemy_idx = cur_idx;
								break;
							}
						}
				if (enemy_idx >= 0)
					break;
				sx ^= 1;
			}
			if (enemy_idx >= 0) // Annihilate it !!!
				toggle_self_destruct();
		}
	}

	void Unit::doUnloadMission(Mission& mission)
	{
		const auto pType = unit_manager.unit_type[typeId];

		if (nb_attached > 0)
		{
			Vector3D Dir = mission.getTarget().getPos() - position;
			Dir.y = 0.0f;
			float dist = Dir.lengthSquared();
			int maxdist = 0;
			if (pType->TransportMaxUnits == 1) // Code for units like the arm atlas
				maxdist = 3;
			else
				maxdist = pType->SightDistance;
			if (dist > maxdist * maxdist && pType->BMcode) // Si l'unité est trop loin du chantier
			{
				c_time = 0.0f;
				mission.Flags() |= MISSION_FLAG_MOVE;
				mission.setMoveData(maxdist * 8 / 80);
			}
			else if (!(mission.getFlags() & MISSION_FLAG_MOVE))
			{
				if (mission.getLastD() >= 0.0f)
				{
					if (pType->TransportMaxUnits == 1) // Code for units like the arm atlas
					{
						if (attached_list[0] >= 0 && attached_list[0] < (int)units.max_unit // Check we can do that
							&& units.unit[attached_list[0]].flags && can_be_built(position, units.unit[attached_list[0]].typeId, ownerId))
						{
							launchScript(SCRIPT_EndTransport);

							Unit* target_unit = &(units.unit[attached_list[0]]);
							target_unit->attached = false;
							target_unit->hidden = false;
							nb_attached = 0;
							pMutex.unlock();
							target_unit->draw_on_map();
							pMutex.lock();
						}
						else if (attached_list[0] < 0 || attached_list[0] >= (int)units.max_unit || units.unit[attached_list[0]].flags == 0)
							nb_attached = 0;

						next_mission();
					}
					else
					{
						if (attached_list[nb_attached - 1] >= 0 && attached_list[nb_attached - 1] < (int)units.max_unit // Check we can do that
							&& units.unit[attached_list[nb_attached - 1]].flags && can_be_built(mission.getTarget().getPos(), units.unit[attached_list[nb_attached - 1]].typeId, ownerId))
						{
							const int idx = attached_list[nb_attached - 1];
							int param[] = {idx, PACKXZ(mission.getTarget().getPos().x * 2.0f + (float)the_map->widthInWorldUnits, mission.getTarget().getPos().z * 2.0f + (float)the_map->heightInWorldUnits)};
							launchScript(SCRIPT_TransportDrop, 2, param);
						}
						else if (attached_list[nb_attached - 1] < 0 || attached_list[nb_attached - 1] >= (int)units.max_unit || units.unit[attached_list[nb_attached - 1]].flags == 0)
							nb_attached--;
					}
					mission.setLastD(-1.0f);
				}
				else
				{
					if (port[BUSY] == 0)
						next_mission();
				}
			}
			else
				stopMoving();
		}
		else
			next_mission();
	}

	void Unit::doLoadMission(Mission& mission)
	{
		const auto pType = unit_manager.unit_type[typeId];

		if (!mission.getTarget().isValid())
		{
			next_mission();
			return;
		}
		if (mission.getUnit())
		{
			Unit* target_unit = mission.getUnit();
			if (!target_unit->isAlive())
			{
				next_mission();
				return;
			}
			Vector3D Dir = target_unit->position - position;
			Dir.y = 0.0f;
			float dist = Dir.lengthSquared();
			int maxdist = 0;
			if (pType->TransportMaxUnits == 1) // Code for units like the arm atlas
				maxdist = 3;
			else
				maxdist = pType->SightDistance;
			if (dist > maxdist * maxdist && pType->BMcode) // Si l'unité est trop loin du chantier
			{
				c_time = 0.0f;
				mission.Flags() |= MISSION_FLAG_MOVE;
				mission.setMoveData(maxdist * 8 / 80);
			}
			else if (!(mission.getFlags() & MISSION_FLAG_MOVE))
			{
				if (mission.getLastD() >= 0.0f)
				{
					if (pType->TransportMaxUnits == 1) // Code for units like the arm atlas
					{
						if (nb_attached == 0)
						{
							int param[] = {(int)((position.y - target_unit->position.y) * 2.0f) << 16};
							launchScript(SCRIPT_BeginTransport, 1, param);
							runScriptFunction(SCRIPT_QueryTransport, 1, param);
							target_unit->attached = true;
							link_list[nb_attached] = (short)param[0];
							target_unit->hidden = param[0] < 0;
							attached_list[nb_attached++] = target_unit->idx;
							target_unit->clear_from_map();
						}
						next_mission();
					}
					else
					{
						if (nb_attached >= pType->TransportMaxUnits)
						{
							next_mission();
							return;
						}
						int param[] = {target_unit->idx};
						launchScript(SCRIPT_TransportPickup, 1, param);
					}
					mission.setLastD(-1.0f);
				}
				else
				{
					if (port[BUSY] == 0)
						next_mission();
				}
			}
			else
				stopMoving();
		}
		else
			next_mission();
	}

	void Unit::doCaptureMission(Mission& mission, float dt)
	{
		const auto pType = unit_manager.unit_type[typeId];

		selfmove = false;
		if (!mission.getTarget().isValid())
		{
			next_mission();
			return;
		}
		if (mission.getUnit()) // Récupère une unité / It's a unit
		{
			Unit* target_unit = mission.getUnit();
			if (target_unit->isAlive())
			{
				if (unit_manager.unit_type[target_unit->typeId]->commander || target_unit->isOwnedBy(ownerId))
				{
					playSound("cant1");
					next_mission();
					return;
				}
				if (!(mission.getFlags() & MISSION_FLAG_TARGET_CHECKED))
				{
					mission.Flags() |= MISSION_FLAG_TARGET_CHECKED;
					mission.setData(Math::Min(unit_manager.unit_type[target_unit->typeId]->BuildCostMetal * 100, 10000));
				}

				Vector3D Dir = target_unit->position - position;
				Dir.y = 0.0f;
				float dist = Dir.lengthSquared();
				UnitType* tType = target_unit->typeId == -1 ? NULL : unit_manager.unit_type[target_unit->typeId];
				int tsize = (tType == NULL) ? 0 : ((tType->FootprintX + tType->FootprintZ) << 2);
				int maxdist = pType->SightDistance + tsize;
				if (dist > maxdist * maxdist && pType->BMcode) // Si l'unité est trop loin du chantier
				{
					c_time = 0.0f;
					if (!(mission.Flags() & MISSION_FLAG_MOVE))
						mission.Flags() |= MISSION_FLAG_REFRESH_PATH | MISSION_FLAG_MOVE;
					mission.setMoveData(Math::Max(maxdist * 7 / 80, (tsize + 7) >> 3));
					mission.setLastD(0.0f);
				}
				else if (!(mission.getFlags() & MISSION_FLAG_MOVE))
				{
					if (mission.getLastD() >= 0.0f)
					{
						start_building(target_unit->position - position);
						mission.setLastD(-1.0f);
					}

					if (pType->BMcode && port[INBUILDSTANCE] != 0)
					{
						if (local && network_manager.isConnected() && nanolathe_target < 0)
						{ // Synchronize nanolathe emission
							nanolathe_target = target_unit->idx;
							g_ta3d_network->sendUnitNanolatheEvent(idx, target_unit->idx, false, false);
						}

						playSound("working");

						mission.setData(mission.getData() - (int)(dt * 1000.0f + 0.5f));
						if (mission.getData() <= 0) // Unit has been captured
						{
							pMutex.unlock();

							target_unit->clear_from_map();
							target_unit->lock();

							Unit* new_unit = create_unit(target_unit->typeId, ownerId, target_unit->position);
							if (new_unit)
							{
								new_unit->lock();

								new_unit->orientation = target_unit->orientation;
								new_unit->hp = target_unit->hp;
								new_unit->build_percent_left = target_unit->build_percent_left;

								new_unit->unlock();
							}

							target_unit->flags = 0x14;
							target_unit->hp = 0.0f;
							target_unit->local = true; // Force synchronization in networking mode

							target_unit->unlock();

							pMutex.lock();
							next_mission();
						}

					}
				}
				else
					stopMoving();
			}
			else
				next_mission();
		}
		else
			next_mission();
	}

	void Unit::doReviveMission(Mission& mission, float dt)
	{
		const auto pType = unit_manager.unit_type[typeId];

		selfmove = false;
		if (!mission.getTarget().isValid())
		{
			next_mission();
			return;
		}

		if (mission.getData() >= 0 && mission.getData() < features.max_features) // Reclaim a feature/wreckage
		{
			features.lock();
			if (features.feature[mission.getData()].type <= 0)
			{
				features.unlock();
				next_mission();
				return;
			}
			bool feature_locked = true;

			Vector3D Dir = features.feature[mission.getData()].Pos - position;
			Dir.y = 0.0f;
			mission.getTarget().setPos(features.feature[mission.getData()].Pos);
			float dist = Dir.lengthSquared();
			Feature* pFeature = feature_manager.getFeaturePointer(features.feature[mission.getData()].type);
			int tsize = pFeature == NULL ? 0 : ((pFeature->footprintx + pFeature->footprintz) << 2);
			int maxdist = pType->SightDistance + tsize;
			if (dist > maxdist * maxdist && pType->BMcode) // If the unit is too far from its target
			{
				c_time = 0.0f;
				if (!(mission.Flags() & MISSION_FLAG_MOVE))
					mission.Flags() |= MISSION_FLAG_REFRESH_PATH | MISSION_FLAG_MOVE;
				mission.setMoveData(Math::Max(maxdist * 7 / 80, (tsize + 7) >> 3));
				mission.setLastD(0.0f);
			}
			else if (!(mission.getFlags() & MISSION_FLAG_MOVE))
			{
				if (mission.getLastD() >= 0.0f)
				{
					start_building(features.feature[mission.getData()].Pos - position);
					mission.setLastD(-1.0f);
				}
				if (pType->BMcode && port[INBUILDSTANCE] != 0)
				{
					if (local && network_manager.isConnected() && nanolathe_target < 0) // Synchronize nanolathe emission
					{
						nanolathe_target = mission.getData();
						g_ta3d_network->sendUnitNanolatheEvent(idx, mission.getData(), true, true);
					}

					playSound("working");
					// Reclaim the object
					const Feature* feature = feature_manager.getFeaturePointer(features.feature[mission.getData()].type);
					const float recup = std::min(dt * float(pType->WorkerTime * feature->damage) / (5.5f * (float)feature->metal), features.feature[mission.getData()].hp);
					features.feature[mission.getData()].hp -= recup;
					if (features.feature[mission.getData()].hp <= 0.0f) // Job done
					{
						features.removeFeatureFromMap(mission.getData()); // Remove the object from map

						if (!feature->name.empty()) // Creates the corresponding unit
						{
							bool success = false;
							String wreckage_name = feature->name;
							wreckage_name = Substr(wreckage_name, 0, wreckage_name.length() - 5); // Remove the _dead/_heap suffix

							int wreckage_type_id = unit_manager.get_unit_index(wreckage_name);
							Vector3D obj_pos = features.feature[mission.getData()].Pos;
							float obj_angle = features.feature[mission.getData()].angle;
							features.unlock();
							feature_locked = false;
							if (network_manager.isConnected())
								g_ta3d_network->sendFeatureDeathEvent(mission.getData());
							features.delete_feature(mission.getData()); // Delete the object

							if (wreckage_type_id >= 0)
							{
								pMutex.unlock();
								Unit* unit_p = create_unit(wreckage_type_id, ownerId, obj_pos);

								if (unit_p)
								{
									unit_p->lock();

									unit_p->orientation.y = obj_angle;
									unit_p->hp = 0.01f;				   // Need to be repaired :P
									unit_p->build_percent_left = 0.0f; // It's finished ...
									unit_p->unlock();
									unit_p->draw_on_map();
								}
								pMutex.lock();

								if (unit_p)
								{
									mission.setMissionType(MISSION_REPAIR); // Now let's repair what we've resurrected
									mission.getTarget().set(Mission::Target::TargetUnit, unit_p->idx, unit_p->ID);
									mission.setData(1);
									success = true;
								}
							}
							if (!success)
							{
								playSound("cant1");
								next_mission();
							}
						}
						else
						{
							features.unlock();
							feature_locked = false;
							if (network_manager.isConnected())
								g_ta3d_network->sendFeatureDeathEvent(mission.getData());
							features.delete_feature(mission.getData()); // Delete the object
							next_mission();
						}
					}
				}
			}
			else
				stopMoving();
			if (feature_locked)
				features.unlock();
		}
		else
			next_mission();
	}

	void Unit::doReclaimMission(Mission& mission, float dt)
	{
		const auto pType = unit_manager.unit_type[typeId];

		selfmove = false;
		if (!mission.getTarget().isValid())
		{
			next_mission();
			return;
		}
		if (mission.getUnit()) // Récupère une unité / It's a unit
		{
			Unit* target_unit = mission.getUnit();
			if (target_unit->isAlive())
			{
				Vector3D Dir = target_unit->position - position;
				Dir.y = 0.0f;
				float dist = Dir.lengthSquared();
				UnitType* tType = target_unit->typeId == -1 ? NULL : unit_manager.unit_type[target_unit->typeId];
				int tsize = (tType == NULL) ? 0 : ((tType->FootprintX + tType->FootprintZ) << 2);
				int maxdist = ((int)(pType->BuildDistance)) + tsize;
				if (dist > maxdist * maxdist && pType->BMcode) // Si l'unité est trop loin du chantier
				{
					c_time = 0.0f;
					if (!(mission.Flags() & MISSION_FLAG_MOVE))
						mission.Flags() |= MISSION_FLAG_REFRESH_PATH | MISSION_FLAG_MOVE;
					mission.setMoveData(Math::Max(maxdist * 7 / 80, (tsize + 7) >> 3));
					mission.setLastD(0.0f);
				}
				else if (!(mission.getFlags() & MISSION_FLAG_MOVE))
				{
					if (mission.getLastD() >= 0.0f)
					{
						start_building(target_unit->position - position);
						mission.setLastD(-1.0f);
					}

					if (pType->BMcode && port[INBUILDSTANCE] != 0)
					{
						if (local && network_manager.isConnected() && nanolathe_target < 0)
						{ // Synchronize nanolathe emission
							nanolathe_target = target_unit->idx;
							g_ta3d_network->sendUnitNanolatheEvent(idx, target_unit->idx, false, true);
						}

						playSound("working");


						// Récupère l'unité
						const UnitType* const pTargetType = unit_manager.unit_type[target_unit->typeId];
						const float recup = std::min(dt * float(pType->WorkerTime * pTargetType->MaxDamage) * target_unit->damage_modifier() / ((isVeteran() ? 5.5f : 11.0f) * (float)pTargetType->BuildCostMetal),
													 target_unit->hp);

						target_unit->hp -= recup;
						if (dt > 0.0f)
							metal_prod += recup * (float)pTargetType->BuildCostMetal / (dt * (float)pTargetType->MaxDamage);
						if (target_unit->hp <= 0.0f) // Work done
						{
							target_unit->flags |= 0x10; // This unit is being reclaimed it doesn't explode!
							next_mission();
						}
					}
				}
				else
					stopMoving();
			}
			else
				next_mission();
		}
		else if (mission.getData() >= 0 && mission.getData() < features.max_features) // Reclaim a feature/wreckage
		{
			features.lock();
			if (features.feature[mission.getData()].type <= 0)
			{
				features.unlock();
				next_mission();
				return;
			}
			bool feature_locked = true;

			Vector3D Dir = features.feature[mission.getData()].Pos - position;
			Dir.y = 0.0f;
			mission.getTarget().setPos(features.feature[mission.getData()].Pos);
			float dist = Dir.lengthSquared();
			Feature* pFeature = feature_manager.getFeaturePointer(features.feature[mission.getData()].type);
			int tsize = pFeature == NULL ? 0 : ((pFeature->footprintx + pFeature->footprintz) << 2);
			int maxdist = ((int)(pType->BuildDistance)) + tsize;
			if (dist > maxdist * maxdist && pType->BMcode) // If the unit is too far from its target
			{
				c_time = 0.0f;
				if (!(mission.Flags() & MISSION_FLAG_MOVE))
					mission.Flags() |= MISSION_FLAG_REFRESH_PATH | MISSION_FLAG_MOVE;
				mission.setMoveData(Math::Max(maxdist * 7 / 80, (tsize + 7) >> 3));
				mission.setLastD(0.0f);
			}
			else if (!(mission.getFlags() & MISSION_FLAG_MOVE))
			{
				if (mission.getLastD() >= 0.0f)
				{
					start_building(features.feature[mission.getData()].Pos - position);
					mission.setLastD(-1.0f);
				}
				if (pType->BMcode && port[INBUILDSTANCE] != 0)
				{
					if (local && network_manager.isConnected() && nanolathe_target < 0) // Synchronize nanolathe emission
					{
						nanolathe_target = mission.getData();
						g_ta3d_network->sendUnitNanolatheEvent(idx, mission.getData(), true, true);
					}

					playSound("working");
					// Reclaim the object
					const Feature* feature = feature_manager.getFeaturePointer(features.feature[mission.getData()].type);
					const float recup = std::min(dt * float(pType->WorkerTime * feature->damage) / (5.5f * (float)feature->metal), features.feature[mission.getData()].hp);
					features.feature[mission.getData()].hp -= recup;
					if (dt > 0.0f)
					{
						metal_prod += recup * (float)feature->metal / (dt * (float)feature->damage);
						energy_prod += recup * (float)feature->energy / (dt * (float)feature->damage);
					}
					if (features.feature[mission.getData()].hp <= 0.0f) // Job done
					{
						features.removeFeatureFromMap(mission.getData()); // Remove the object from map

						features.unlock();
						feature_locked = false;
						if (network_manager.isConnected())
							g_ta3d_network->sendFeatureDeathEvent(mission.getData());
						features.delete_feature(mission.getData()); // Delete the object
						next_mission();
					}
				}
			}
			else
				stopMoving();
			if (feature_locked)
				features.unlock();
		}
		else
			next_mission();
	}

	void Unit::doGuardMission(Mission& mission)
	{
		const auto pType = unit_manager.unit_type[typeId];

		auto targetUnit = mission.getUnit();

		if (targetUnit && targetUnit->isAlive() && targetUnit->isOwnedBy(ownerId))
		{ // On ne défend pas n'importe quoi
			if (pType->Builder)
			{
				if (targetUnit->isBeingBuilt() || targetUnit->hp < unit_manager.unit_type[targetUnit->typeId]->MaxDamage) // Répare l'unité
				{
					add_mission(MISSION_REPAIR | MISSION_FLAG_AUTO, &targetUnit->position, true, 0, targetUnit);
					return;
				}
				else if (!targetUnit->missionQueue.empty() && (targetUnit->missionQueue->mission() == MISSION_BUILD_2 || targetUnit->missionQueue->mission() == MISSION_REPAIR)) // L'aide à construire
				{
					add_mission(MISSION_REPAIR | MISSION_FLAG_AUTO, &targetUnit->missionQueue->getTarget().getPos(), true, 0, targetUnit->missionQueue->getUnit());
					return;
				}
			}
			if (pType->canattack)
			{
				if (!targetUnit->missionQueue.empty() && targetUnit->missionQueue->mission() == MISSION_ATTACK) // L'aide à attaquer
				{
					add_mission(MISSION_ATTACK | MISSION_FLAG_AUTO, &targetUnit->missionQueue->getTarget().getPos(), true, 0, targetUnit->missionQueue->getUnit());
					return;
				}
			}
			if (!pType->canfly)
			{
				if (((Vector3D) (position - targetUnit->position)).lengthSquared() >= 25600.0f) // On reste assez près
				{
					mission.Flags() |= MISSION_FLAG_MOVE; // | MISSION_FLAG_REFRESH_PATH;
					mission.setMoveData(10);
					c_time = 0.0f;
					return;
				}
				else if (mission.getFlags() & MISSION_FLAG_MOVE)
					stopMoving();
			}
		}
		else
			next_mission();
	}

	void Unit::doStandbyMission(Mission& mission)
	{
		if (mission.getData() > 5)
		{
			if (missionQueue.hasNext()) // If there is a mission after this one
			{
				next_mission();
				if (!missionQueue.empty() && (mission.mission() == MISSION_STANDBY || mission.mission() == MISSION_VTOL_STANDBY))
					mission.setData(0);
			}
		}
		else
			mission.setData(mission.getData() + 1);
	}

	void Unit::doAttackMission(Mission& mission)
	{
		const auto pType = unit_manager.unit_type[typeId];

		if (!missionQueue->getTarget().isValid())
		{
			next_mission();
			return;
		}

		Unit* target_unit = missionQueue->getUnit();
		Weapon* target_weapon = missionQueue->getWeapon();
		if ((target_unit != NULL && target_unit->isAlive()) || (target_weapon != NULL && target_weapon->weapon_id != -1) || missionQueue->getTarget().isStatic())
		{
			if (target_unit) // Check if we can target the unit
			{
				const PlayerMask mask = toPlayerMask(ownerId);
				if (target_unit->cloaked && !target_unit->is_on_radar(mask))
				{
					for (uint32 i = 0; i < weapon.size(); ++i)
						if (weapon[i].target == target_unit) // Stop shooting
							weapon[i].state = WEAPON_FLAG_IDLE;
					next_mission();
					return;
				}
			}

			if (weapon.size() == 0 && !pType->kamikaze) // Check if this units has weapons
			{
				next_mission();
				return;
			}

			Vector3D Dir = missionQueue->getTarget().getPos() - position;
			Dir.y = 0.0f;
			float dist = Dir.lengthSquared();
			int maxdist = 0;
			int mindist = 0xFFFFF;

			if (target_unit != NULL && unit_manager.unit_type[target_unit->typeId]->checkCategory(pType->NoChaseCategory))
			{
				next_mission();
				return;
			}

			for (uint32 i = 0; i < weapon.size(); ++i)
			{
				if (pType->weapon[i] == NULL || pType->weapon[i]->interceptor)
					continue;
				int cur_mindist;
				int cur_maxdist;
				bool allowed_to_fire = true;
				if (pType->attackrunlength > 0)
				{
					if (Dir % velocity < 0.0f)
						allowed_to_fire = false;
					const float t = 2.0f / the_map->ota_data.gravity * fabsf(position.y - missionQueue->getTarget().getPos().y);
					cur_mindist = (int)sqrtf(t * velocity.lengthSquared()) - ((pType->attackrunlength + 1) / 2);
					cur_maxdist = cur_mindist + pType->attackrunlength;
				}
				else if (pType->weapon[i]->waterweapon && position.y > the_map->sealvl)
				{
					if (Dir % velocity < 0.0f)
						allowed_to_fire = false;
					const float t = 2.0f / the_map->ota_data.gravity * fabsf(position.y - missionQueue->getTarget().getPos().y);
					cur_maxdist = (int)sqrtf(t * velocity.lengthSquared()) + (pType->weapon[i]->range / 2);
					cur_mindist = 0;
				}
				else
				{
					cur_maxdist = pType->weapon[i]->range / 2;
					cur_mindist = 0;
				}
				if (maxdist < cur_maxdist)
					maxdist = cur_maxdist;
				if (mindist > cur_mindist)
					mindist = cur_mindist;
				if (allowed_to_fire && dist >= cur_mindist * cur_mindist && dist <= cur_maxdist * cur_maxdist && !pType->weapon[i]->interceptor)
				{
					if (((weapon[i].state & 3) == WEAPON_FLAG_IDLE || ((weapon[i].state & 3) != WEAPON_FLAG_IDLE && weapon[i].target != target_unit)) && (target_unit == NULL || ((!pType->weapon[i]->toairweapon || (pType->weapon[i]->toairweapon && target_unit->flying)) && !unit_manager.unit_type[target_unit->typeId]->checkCategory(pType->NoChaseCategory))) && (((missionQueue->getFlags() & MISSION_FLAG_COMMAND_FIRE) && (pType->weapon[i]->commandfire || !pType->candgun)) || (!(missionQueue->getFlags() & MISSION_FLAG_COMMAND_FIRE) && !pType->weapon[i]->commandfire) || pType->weapon[i]->dropped))
					{
						weapon[i].state = WEAPON_FLAG_AIM;
						weapon[i].target = target_unit;
						weapon[i].target_pos = missionQueue->getTarget().getPos();
						weapon[i].data = -1;
						if (missionQueue->getFlags() & MISSION_FLAG_TARGET_WEAPON)
							weapon[i].state |= WEAPON_FLAG_WEAPON;
						if (pType->weapon[i]->commandfire)
							weapon[i].state |= WEAPON_FLAG_COMMAND_FIRE;
					}
				}
			}

			if (pType->kamikaze && pType->kamikazedistance > maxdist)
				maxdist = pType->kamikazedistance;

			if (mindist > maxdist)
				mindist = maxdist;

			missionQueue->Flags() |= MISSION_FLAG_CAN_ATTACK;

			if (pType->kamikaze // Kamikaze attack !!
				&& dist <= pType->kamikazedistance * pType->kamikazedistance && self_destruct < 0.0f)
				self_destruct = 0.01f;

			if (dist > maxdist * maxdist || dist < mindist * mindist) // Si l'unité est trop loin de sa cible / if unit isn't where it should be
			{
				if (!pType->canmove) // Bah là si on peut pas bouger faut changer de cible!! / need to change target
				{
					next_mission();
					return;
				}
				else if (!pType->canfly || pType->hoverattack)
				{
					c_time = 0.0f;
					missionQueue->Flags() |= MISSION_FLAG_MOVE;
					missionQueue->setMoveData(maxdist * 7 / 80);
				}
			}
			else if (missionQueue->getData() == 0)
			{
				missionQueue->setData(2);
				int param[] = {0};
				for (uint32 i = 0; i < weapon.size(); ++i)
					if (pType->weapon[i])
						param[0] = Math::Max(param[0], (int)(pType->weapon[i]->reloadtime * 1000.0f) * Math::Max(1, (int)pType->weapon[i]->burst));
				launchScript(SCRIPT_SetMaxReloadTime, 1, param);
			}

			if (missionQueue->getFlags() & MISSION_FLAG_COMMAND_FIRED)
				next_mission();
		}
		else
			next_mission();
	}

	void Unit::doStopMission(Mission& mission)
	{
		while (missionQueue.hasNext() && (missionQueue->mission() == MISSION_STOP || missionQueue->mission() == MISSION_STANDBY || missionQueue->mission() == MISSION_VTOL_STANDBY) && (missionQueue(1) == MISSION_STOP || missionQueue(1) == MISSION_STANDBY || missionQueue(1) == MISSION_VTOL_STANDBY)) // Don't make a big stop stack :P
			next_mission();
		if (missionQueue->mission() != MISSION_STOP && missionQueue->mission() != MISSION_STANDBY && missionQueue->mission() != MISSION_VTOL_STANDBY)
			return;
		missionQueue->setMissionType(MISSION_STOP);
		if (missionQueue->getData() > 5)
		{
			if (missionQueue.hasNext())
			{
				next_mission();
				if (!missionQueue.empty() && missionQueue->mission() == MISSION_STOP) // Mode attente / wait mode
					missionQueue->setData(1);
			}
		}
		else
		{
			if (missionQueue->getData() == 0)
			{
				stopMovingAnimation();
				was_moving = false;
				selfmove = false;
				if (port[INBUILDSTANCE])
				{
					launchScript(SCRIPT_stopbuilding);
					deactivate();
				}
				for (uint32 i = 0; i < weapon.size(); ++i)
					if (weapon[i].state)
					{
						launchScript(SCRIPT_TargetCleared);
						break;
					}
				for (uint32 i = 0; i < weapon.size(); ++i) // Stop weapons
				{
					weapon[i].state = WEAPON_FLAG_IDLE;
					weapon[i].data = -1;
				}
			}
			missionQueue->setData(missionQueue->getData() + 1);
		}
	}

	void Unit::doRepairMission(Mission& mission, float dt)
	{
		const auto pType = unit_manager.unit_type[typeId];

		if (!mission.getTarget().isValid())
		{
			next_mission();
			return;
		}

		Unit* target_unit = mission.getUnit();
		if (target_unit != NULL && target_unit->isAlive() && !target_unit->isBeingBuilt())
		{
			if (target_unit->hp >= unit_manager.unit_type[target_unit->typeId]->MaxDamage || !pType->BMcode)
			{
				if (pType->BMcode)
					target_unit->hp = (float)unit_manager.unit_type[target_unit->typeId]->MaxDamage;
				next_mission();
			}
			else
			{
				Vector3D Dir = target_unit->position - position;
				Dir.y = 0.0f;
				const float dist = Dir.lengthSquared();
				const int maxdist = (int)pType->BuildDistance + ((unit_manager.unit_type[target_unit->typeId]->FootprintX + unit_manager.unit_type[target_unit->typeId]->FootprintZ) << 1);
				if (dist > maxdist * maxdist && pType->BMcode) // Si l'unité est trop loin du chantier
				{
					mission.Flags() |= MISSION_FLAG_MOVE;
					mission.setMoveData(maxdist * 7 / 80);
					mission.setData(0);
					c_time = 0.0f;
				}
				else
				{
					if (mission.getFlags() & MISSION_FLAG_MOVE) // Stop moving if needed
					{
						stopMoving();
						return;
					}
					if (mission.getData() == 0)
					{
						mission.setData(1);
						start_building(target_unit->position - position);
					}

					if (port[INBUILDSTANCE] != 0)
					{
						if (local && network_manager.isConnected() && nanolathe_target < 0) // Synchronize nanolathe emission
						{
							nanolathe_target = target_unit->idx;
							g_ta3d_network->sendUnitNanolatheEvent(idx, target_unit->idx, false, false);
						}

						const float conso_energy = ((float)(pType->WorkerTime * unit_manager.unit_type[target_unit->typeId]->BuildCostEnergy)) / (float)unit_manager.unit_type[target_unit->typeId]->BuildTime;
						TA3D::players.requested_energy[ownerId] += conso_energy;
						if (players.energy[ownerId] >= (energy_cons + conso_energy * TA3D::players.energy_factor[ownerId]) * dt)
						{
							energy_cons += conso_energy * TA3D::players.energy_factor[ownerId];
							const UnitType* pTargetType = unit_manager.unit_type[target_unit->typeId];
							const float maxdmg = float(pTargetType->MaxDamage);
							target_unit->hp = std::min(maxdmg,
													   target_unit->hp + dt * TA3D::players.energy_factor[ownerId] * (float)pType->WorkerTime * maxdmg / (float)pTargetType->BuildTime);
						}
						target_unit->built = true;
					}
				}
			}
		}
		else if (target_unit != NULL && target_unit->flags)
		{
			Vector3D Dir = target_unit->position - position;
			Dir.y = 0.0f;
			const float dist = Dir.lengthSquared();
			const int maxdist = (int)pType->BuildDistance + ((unit_manager.unit_type[target_unit->typeId]->FootprintX + unit_manager.unit_type[target_unit->typeId]->FootprintZ) << 1);
			if (dist > maxdist * maxdist && pType->BMcode) // Si l'unité est trop loin du chantier
			{
				c_time = 0.0f;
				mission.Flags() |= MISSION_FLAG_MOVE;
				mission.setMoveData(maxdist * 7 / 80);
			}
			else
			{
				if (mission.getFlags() & MISSION_FLAG_MOVE) // Stop moving if needed
					stopMoving();
				if (pType->BMcode)
				{
					start_building(target_unit->position - position);
					mission.setMissionType(MISSION_BUILD_2); // Change de type de mission
				}
			}
		}
	}
} // namespace TA3D
