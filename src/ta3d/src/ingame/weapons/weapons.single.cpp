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
#include "weapons.h"
#include <tdf.h>
#include <UnitEngine.h>
#include <gfx/fx.h>
#include <misc/camera.h>
#include <misc/ta3d_math.h>
#include <sounds/manager.h>
#include <ingame/players.h>
#include <EngineClass.h>
#include <misc/bvh.h>

namespace TA3D
{

	void Weapon::init()
	{
		ticks_to_compute = 0U;
		just_explode = false;
		damage = -1;
		visible = true;
		idx = 0;
		phase = 1;
		a_time = 0.0f;
		f_time = 0.0f;
		shooter_idx = -1;
		anim_sprite = 0;
		weaponId = -1; // Non défini
		position.reset();
		velocity.reset();
		targetPosition = position;
		target = -1; // Pas de cible
		stime = 0.0f;
		killtime = 0.0f;
		dying = false;
		smoke_time = 0.0f;
		owner = 0;
		local = true;
		bInit = true;
	}

	void Weapon::move(const float dt) // Anime les armes
	{
		if (weaponId < 0)
			return;

		if (bInit)
		{
			bInit = false;
			startPosition = position;
		}

		const WeaponDef* const weapon_def = &(weapon_manager.weapon[weaponId]);

		smoke_time += dt;
		f_time -= dt;
		a_time += dt;
		Vector3D A;

		if (weapon_def->twophase && phase == 1)
		{
			if (!dying && a_time >= weapon_def->weapontimer) // Entre dans la seconde phase
			{
				phase = 2;
				f_time = weapon_def->flighttime;
				stime = 0.0f;
			}
			if (weapon_def->vlaunch)
			{
				velocity.x = 0.0f;
				velocity.z = 0.0f;
				if (velocity.y < weapon_def->weaponvelocity)
					A.y = weapon_def->weaponacceleration;
				else
					velocity.y = weapon_def->weaponvelocity;
			}
		}
		if (!dying && weapon_def->selfprop && f_time <= 0.0f && ((weapon_def->twophase && phase == 2) || !weapon_def->twophase))
			dying = true;
		if (weapon_def->smoketrail && weapon_def->smokedelay < smoke_time) // Trainée de fumée des missiles
		{
			smoke_time = 0.0f;
			if (visible)
				particle_engine.make_smoke(position, 0, 1, 0.0f, -1.0f, -2.0f, 0.3f);
		}

		Vector3D hit_vec;
		const Vector3D OPos(position);

		const float h = the_map->get_unit_h(position.x, position.z);
		if (dying)
			killtime -= dt;
		else
		{
			if ((!weapon_def->lineofsight && (weapon_def->ballistic || weapon_def->dropped)) || (weapon_def->waterweapon && position.y > the_map->sealvl)) // Arme soumise à la gravité
				A.y -= the_map->ota_data.gravity;

			if (weapon_def->guidance && ((weapon_def->twophase && phase == 2) || !weapon_def->twophase) && ((weapon_def->waterweapon && position.y < the_map->sealvl) || !weapon_def->waterweapon)) // Traque sa cible
			{
				float speed = velocity.length();
				if (weapon_def->tracks && target >= 0)
				{
					Vector3D target_V;
					if (weapon_def->interceptor && (uint32)target <= weapons.nb_weapon && weapons.weapon[target].weaponId != -1)
					{
						targetPosition = weapons.weapon[target].position;
						target_V = weapons.weapon[target].velocity;
					}
					else if (!weapon_def->interceptor && (uint32)target < units.max_unit && units.unit[target].isAlive()) // Met à jour les coordonnées de la cible
					{
						targetPosition = units.unit[target].position;
						target_V = units.unit[target].velocity;
					}
					else
						target = -1;
					const float speed = velocity.lengthSquared();
					const float target_speed = target_V.lengthSquared();
					if (speed > 0.0f && target_speed > 0.0f) // Make it aim better
					{
						const float time_to_hit = (targetPosition - position).lengthSquared() / speed;
						targetPosition = targetPosition + sqrtf(time_to_hit / target_speed) * target_V;
					}
				}
				if (targetPosition.y < the_map->sealvl && !weapon_def->waterweapon)
					targetPosition.y = the_map->sealvl;
				Vector3D Dir = targetPosition - position;
				Dir.normalize();
				Vector3D I(velocity), J, K; // Crée un trièdre
				I.normalize();
				J = I * Dir;
				K = J * I;
				if (speed < weapon_def->weaponvelocity)
				{
					if (speed > 0.0f)
						A += weapon_def->weaponacceleration * I;
					else
						A += weapon_def->weaponacceleration * Dir;
				}
				else
				{
					if (speed > 0.5f * weapon_def->weaponvelocity && velocity.dot(Dir) < 0.0f) // Can slow down if needed
						A -= weapon_def->weaponacceleration * I;
					else
					{
						if (speed > weapon_def->weaponvelocity) // Reduce speed
							speed = weapon_def->weaponvelocity;
					}
				}

				const float rotate = dt * float(weapon_def->turnrate) * TA2RAD;
				velocity = speed * (cosf(rotate) * I + sinf(rotate) * K);
			}
			velocity = velocity + dt * A;
			position = position + dt * velocity;
			stime += dt;
		}

		if (weapon_def->waterweapon && position.y <= the_map->sealvl && OPos.y > the_map->sealvl) // A weapon that gets into water slows down
			velocity = 0.5f * velocity;

		const float length = ((Vector3D) (OPos - position)).length();
		if (!dying)
		{
			if (weapon_def->waterweapon && position.y > the_map->sealvl && OPos.y <= the_map->sealvl) // An aquatic weapon does not come out of the water
			{
				position.y = the_map->sealvl;
				velocity.y = 0.0f;
			}
			else
			{
				hit_vec = the_map->hit(position, velocity, !weapon_def->waterweapon, length);
				if (!weapon_def->waterweapon && position.y <= the_map->sealvl && h < the_map->sealvl)
				{
					hit_vec = the_map->hit(position, velocity, !weapon_def->waterweapon, length);
					if (!Math::AlmostZero(velocity.y))
						hit_vec = position - (the_map->sealvl - hit_vec.y) / velocity.y * velocity;
					else
						hit_vec = position;
					hit_vec.y = the_map->sealvl;
				}
			}
		}

		if (!dying && weapon_def->cruise && ((weapon_def->twophase && phase == 2) || phase == 1))
		{
			if (((Vector3D) (targetPosition - position)).length() > 2.0f * fabsf(position.y - h) && velocity.y < 0.0f)
				velocity.y = 0.0f;
		}

		bool hit = false;
		if (!dying)
			hit = (hit_vec - position).dot(velocity) <= 0.0f && (hit_vec - OPos).dot(velocity) >= 0.0f;
		bool u_hit = false;

		if (just_explode)
		{
			hit_vec = position;
			hit = true;
			just_explode = false;
		}

		if (weapon_def->interceptor && (position - targetPosition).lengthSquared() < 1024.0f)
		{
			hit = true;
			hit_vec = position;
			if (target >= 0 && (uint32)target <= weapons.nb_weapon && weapons.weapon[target].weaponId != -1)
			{
				weapons.weapon[target].dying = true;
				weapons.weapon[target].killtime = 0.0f;
			}
		}

		int hit_idx = -1;
		if (!dying && !hit)
		{
			auto heightmapIndex = the_map->worldToHeightmapIndex(OPos);
			const int py = heightmapIndex.y;
			const int px = heightmapIndex.x;
			Vector3D Dir(velocity);
			Dir.normalize();
			std::deque<BVH_UnitTKit::T> neighbors;
			weapons.bvhUnits->boxCollisionQuery(neighbors, 0.5f * (position + OPos), (OPos - position).length());
			for (std::deque<BVH_UnitTKit::T>::const_iterator it = neighbors.begin(); it != neighbors.end(); ++it)
			{
				const int t_idx = it->first->idx;
				if (t_idx == shooter_idx)
					continue;
				if ((it->first->isNotOwnedBy(owner) || target == t_idx) && it->first->isAlive()) // No Friendly Fire
				{
					Vector3D t_vec;
					u_hit = const_cast<Unit*>(it->first)->hit_fast(OPos, Dir, &t_vec, length);
					if (u_hit)
					{
						if ((t_vec - position).dot(velocity) <= 0.0f) // Touché
						{
							if (!hit)
							{
								hit_vec = t_vec;
								hit_idx = t_idx;
							}
							else if ((hit_vec - t_vec).dot(Dir) >= 0.0f)
							{
								hit_vec = t_vec;
								hit_idx = t_idx;
							}
							else
								u_hit = false;
						}
						else
							u_hit = false;
					}
					hit |= u_hit;
				}
			}
			if (px >= 0 && px < the_map->widthInHeightmapTiles && py >= 0 && py < the_map->heightInHeightmapTiles)
			{
				const int t_idx = the_map->map_data(px, py).unit_idx;
				if (t_idx <= -2 && !weapon_def->unitsonly)
				{
					if (!hit && -t_idx - 2 < features.max_features && features.feature[-t_idx - 2].type >= 0 && features.feature[-t_idx - 2].Pos.y + float(feature_manager.getFeaturePointer(features.feature[-t_idx - 2].type)->height) * 0.5f > OPos.y)
					{
						hit = true;
						hit_vec = OPos;
						hit_idx = t_idx;
					}
				}
			}
			if (hit_idx >= 0)
			{
				Unit* const pUnit = &(units.unit[hit_idx]);
				pUnit->lock();
				if (pUnit->isAlive() && pUnit->local)
				{
					const bool ok = pUnit->hp > 0.0f; // Juste pour identifier l'assassin...
					damage = float(weapon_def->get_damage_for_unit(unit_manager.unit_type[pUnit->typeId]->Unitname)) * pUnit->damage_modifier();
					if (weapon_def->paralyzer)
					{
						if (!unit_manager.unit_type[pUnit->typeId]->ImmuneToParalyzer)
						{
							pUnit->paralyzed = damage / 60.0f; // Get paralyzed (900 dmg <-> 15sec according to WeaponS.TDF)
							if (network_manager.isConnected()) // Send damage event
								g_ta3d_network->sendParalyzeEvent(hit_idx, damage);
						}
					}
					else
					{
						pUnit->hp -= damage;																																												  // L'unité touchée encaisse les dégats
						pUnit->flags &= 0xEF;																																												  // This unit must explode if it has been damaged by a weapon even if it is being reclaimed
						if (ok && shooter_idx >= 0 && (uint32)shooter_idx < units.max_unit && pUnit->hp <= 0.0f && units.unit[shooter_idx].ownerId < players.count() && pUnit->ownerId != units.unit[shooter_idx].ownerId) // Non,non les unités que l'on se détruit ne comptent pas dans le nombre de tués mais dans les pertes
						{
							players.kills[units.unit[shooter_idx].ownerId]++;
							units.unit[shooter_idx].kills++;
						}
						if (pUnit->hp <= 0.0f)
							pUnit->severity = Math::Max(pUnit->severity, (int)damage);

						if (network_manager.isConnected()) // Send damage event
							g_ta3d_network->sendDamageEvent(hit_idx, damage);
					}

					Vector3D D = velocity * RotateY(-pUnit->orientation.y * DEG2RAD);
					D.normalize();
					int param[] = {(int)(10.0f * DEG2TA * D.z), (int)(10.0f * DEG2TA * D.x)};
					pUnit->launchScript(SCRIPT_HitByWeapon, 2, param);

					pUnit->attacked = true;
				}
				pUnit->unlock();
			}
			else if (!weapon_def->paralyzer && hit_idx <= -2) // We can't paralyze features :P
			{
				features.lock();
				FeatureData* const pFeature = &(features.feature[-hit_idx - 2]);
				if (pFeature->type >= 0) // Only local weapons here, otherwise weapons would destroy features multiple times
				{
					damage = float(weapon_def->damage);
					const Feature* const feature = feature_manager.getFeaturePointer(pFeature->type);

					// Start a fire ?
					if (feature->flamable && !pFeature->burning && weapon_def->firestarter && local)
					{
						const int starter_score = Math::RandomTable() % 100;
						if (starter_score < weapon_def->firestarter)
						{
							features.burn_feature(-hit_idx - 2);
							if (network_manager.isConnected())
								g_ta3d_network->sendFeatureFireEvent(-hit_idx - 2);
						}
					}

					pFeature->hp -= damage; // The feature hit is taking damage
					if (pFeature->hp <= 0.0f && !pFeature->burning && local)
					{
						if (network_manager.isConnected())
							g_ta3d_network->sendFeatureDeathEvent(-hit_idx - 2);

						const int sx = pFeature->px; // Delete the feature
						const int sy = pFeature->py;
						const Vector3D feature_pos = pFeature->Pos;
						const int feature_type = pFeature->type;
						features.removeFeatureFromMap(-hit_idx - 2);
						features.delete_feature(-hit_idx - 2); // Supprime l'objet

						// Replace the feature if needed
						Feature* feat2 = feature_manager.getFeaturePointer(feature_type);
						if (feat2 && !feat2->feature_dead.empty())
						{
							const int type = feature_manager.get_feature_index(feat2->feature_dead);
							if (type >= 0)
							{
								the_map->map_data(sx, sy).stuff = features.add_feature(feature_pos, type);
								features.drawFeatureOnMap(the_map->map_data(sx, sy).stuff);
								if (network_manager.isConnected())
									g_ta3d_network->sendFeatureCreationEvent(the_map->map_data(sx, sy).stuff);
							}
						}
					}
				}
				features.unlock();
			}
		}

		if (hit && weapon_def->areaofeffect > 0) // Domages colatéraux
		{
			if (damage < 0.0f)
				damage = float(weapon_def->damage);
			auto heightTileIndex = the_map->worldToHeightmapIndex(OPos);
			const int py = heightTileIndex.y;
			const int px = heightTileIndex.x;
			const int s = (weapon_def->areaofeffect + 31) >> 5;
			const int d = (weapon_def->areaofeffect * weapon_def->areaofeffect + 15) >> 4;
			std::deque<BVH_UnitTKit::T> neighbors;
			weapons.bvhUnits->boxCollisionQuery(neighbors, position, std::sqrt(float(d)));
			for (std::deque<BVH_UnitTKit::T>::const_iterator it = neighbors.begin(); it != neighbors.end(); ++it)
			{
				Unit* const pUnit = const_cast<Unit*>(it->first);
				if (pUnit->idx == shooter_idx)
					continue;
				pUnit->lock();
				if (pUnit->isAlive() && pUnit->local && ((Vector3D) (pUnit->position - position)).lengthSquared() <= d)
				{
					const bool ok = pUnit->hp > 0.0f;
					damage = float(weapon_def->get_damage_for_unit(unit_manager.unit_type[pUnit->typeId]->Unitname));
					const float cur_damage = damage * weapon_def->edgeeffectiveness * pUnit->damage_modifier();
					if (weapon_def->paralyzer)
					{
						if (!unit_manager.unit_type[pUnit->typeId]->ImmuneToParalyzer)
						{
							pUnit->paralyzed = cur_damage / 60.0f; // Get paralyzed (900 <-> 15sec)
							if (network_manager.isConnected())	 // Send damage event
								g_ta3d_network->sendParalyzeEvent(pUnit->idx, cur_damage);
						}
					}
					else
					{
						pUnit->hp -= cur_damage;																																											  // L'unité touchée encaisse les dégats
						unsetFlag(pUnit->flags, 0x10);																																										  // This unit must explode if it has been damaged by a weapon even if it is being reclaimed
						if (ok && shooter_idx >= 0 && (uint32)shooter_idx < units.max_unit && pUnit->hp <= 0.0f && units.unit[shooter_idx].ownerId < players.count() && pUnit->ownerId != units.unit[shooter_idx].ownerId) // Non,non les unités que l'on se détruit ne comptent pas dans le nombre de tués mais dans les pertes
						{
							players.kills[units.unit[shooter_idx].ownerId]++;
							units.unit[shooter_idx].kills++;
						}
						if (pUnit->hp <= 0.0f)
							pUnit->severity = Math::Max(pUnit->severity, (int)cur_damage);

						if (network_manager.isConnected()) // Send damage event
							g_ta3d_network->sendDamageEvent(pUnit->idx, cur_damage);
					}

					Vector3D D = (pUnit->position - position) * RotateY(-pUnit->orientation.y * DEG2RAD);
					D.normalize();
					int param[] = {(int)(10.0f * DEG2TA * D.z), (int)(10.0f * DEG2TA * D.x)};
					pUnit->launchScript(SCRIPT_HitByWeapon, 2, param);

					pUnit->attacked = true;
				}
				pUnit->unlock();
			}

			std::deque<int> oidx;
			for (int y = -s; y <= s; ++y)
			{
				if (py + y < 0 || py + y >= the_map->heightInHeightmapTiles)
					continue;
				for (int x = -s; x <= s; ++x)
				{
					if (px + x < 0 || px + x >= the_map->widthInHeightmapTiles)
						continue;

					const int t_idx = the_map->map_data(px + x, py + y).unit_idx;
					if (t_idx >= -1)
						continue;
					if (std::find(oidx.begin(), oidx.end(), t_idx) != oidx.end())
						continue;
					features.lock();
					FeatureData* const pFeature = &(features.feature[-t_idx - 2]);
					if (!weapon_def->unitsonly && pFeature->type >= 0 &&
						((Vector3D) (pFeature->Pos - position)).lengthSquared() <= d)
					{
						Feature* feature = feature_manager.getFeaturePointer(pFeature->type);
						// Start a fire ?
						if (feature->flamable && !pFeature->burning && weapon_def->firestarter && local)
						{
							const int starter_score = Math::RandomTable() % 100;
							if (starter_score < weapon_def->firestarter)
							{
								features.burn_feature(-t_idx - 2);
								if (network_manager.isConnected())
									g_ta3d_network->sendFeatureFireEvent(-t_idx - 2);
							}
						}

						oidx.push_back(t_idx);
						if (!feature->indestructible && !pFeature->burning)
						{
							damage = float(weapon_def->damage);
							const float cur_damage = damage * weapon_def->edgeeffectiveness;
							pFeature->hp -= cur_damage; // L'objet touché encaisse les dégats
							if (pFeature->hp <= 0.0f && local)
							{
								if (network_manager.isConnected())
									g_ta3d_network->sendFeatureDeathEvent(-t_idx - 2);
								const int sx = pFeature->px; // Remove the object
								const int sy = pFeature->py;
								const Vector3D feature_pos = pFeature->Pos;
								const int feature_type = pFeature->type;
								features.removeFeatureFromMap(-t_idx - 2);
								features.delete_feature(-t_idx - 2); // Supprime l'objet

								// Replace the feature if needed
								Feature* feat2 = feature_manager.getFeaturePointer(feature_type);
								if (feat2 && !feat2->feature_dead.empty())
								{
									const int type = feature_manager.get_feature_index(feat2->feature_dead);
									if (type >= 0)
									{
										the_map->map_data(sx, sy).stuff = features.add_feature(feature_pos, type);
										features.drawFeatureOnMap(the_map->map_data(sx, sy).stuff);
										if (network_manager.isConnected())
											g_ta3d_network->sendFeatureCreationEvent(the_map->map_data(sx, sy).stuff);
									}
								}
							}
						}
					}
					features.unlock();
				}
			}
			oidx.clear();
		}

		if (hit && visible && weapon_def->areaofeffect >= 256) // Effet de souffle / Shock wave
		{
			fx_manager.addFlash(position, float(weapon_def->areaofeffect) * 0.5f);
			particle_engine.make_shockwave(position, 1, weapon_def->areaofeffect, float(weapon_def->areaofeffect) * 0.75f);
			particle_engine.make_shockwave(position, 0, weapon_def->areaofeffect, float(weapon_def->areaofeffect) * 0.5f);
			particle_engine.make_nuke(position, 1, weapon_def->areaofeffect >> 1, float(weapon_def->areaofeffect) * 0.25f);
		}

		if (hit && weapon_def->interceptor)
		{
			units.unit[shooter_idx].lock();
			if (units.unit[shooter_idx].isAlive())
			{
				int e = 0;
				for (int i = 0; i + e < units.unit[shooter_idx].mem_size; ++i)
				{
					if (units.unit[shooter_idx].memory[i + e] == (uint32)target)
					{
						++e;
						--i;
						continue;
					}
					units.unit[shooter_idx].memory[i] = units.unit[shooter_idx].memory[i + e];
				}
				units.unit[shooter_idx].mem_size -= e;
			}
			units.unit[shooter_idx].unlock();
		}

		const float travelled = (position - startPosition).lengthSquared();
		const bool rangeReached = travelled >= (float(weapon_def->range * weapon_def->range) * 0.25f);

		if ((((stime > 0.5f * weapon_def->time_to_range || rangeReached) && (!weapon_def->noautorange || weapon_def->burnblow)) || hit) && !dying)
		{
			if (hit)
			{
				position = hit_vec;
			}
			if (Math::AlmostEquals(position.y, the_map->sealvl))
			{
				if (!weapon_def->soundwater.empty())
					sound_manager->playSound(weapon_def->soundwater, &position);
			}
			else
			{
				if (!weapon_def->soundhit.empty())
					sound_manager->playSound(weapon_def->soundhit, &position);
			}

			if (hit && !weapon_def->explosiongaf.empty() && !weapon_def->explosionart.empty() && !Math::AlmostEquals(position.y, the_map->sealvl))
			{
				if (visible && weapon_def->areaofeffect < 256) // Nuclear type explosion don't draw sprites :)
					fx_manager.add(weapon_def->explosiongaf, weapon_def->explosionart, position, 1.0f);
			}
			else if (hit && Math::AlmostEquals(position.y, the_map->sealvl))
			{
				auto tileIndex = the_map->worldToGraphicalTileIndex(position);
				const int px = tileIndex.x;
				const int py = tileIndex.y;
				Vector3D P = position;
				P.y += 3.0f;
				if (px >= 0 && px < the_map->widthInGraphicalTiles && py >= 0 && py < the_map->heightInGraphicalTiles)
				{
					if (the_map->bloc[the_map->bmap(px, py)].lava && !weapon_def->lavaexplosiongaf.empty() && !weapon_def->lavaexplosionart.empty())
					{
						if (visible)
							fx_manager.add(weapon_def->lavaexplosiongaf, weapon_def->lavaexplosionart, position, 1.0f);
					}
					else if (!the_map->bloc[the_map->bmap(px, py)].lava && !weapon_def->waterexplosiongaf.empty() && !weapon_def->waterexplosionart.empty())
						if (visible)
							fx_manager.add(weapon_def->waterexplosiongaf, weapon_def->waterexplosionart, position, 1.0f);
				}
				else if (!weapon_def->explosiongaf.empty() && !weapon_def->explosionart.empty())
					if (visible)
						fx_manager.add(weapon_def->explosiongaf, weapon_def->explosionart, position, 1.0f);
			}
			if (weapon_def->endsmoke)
			{
				if (visible)
					particle_engine.make_smoke(position, 0, 1, 0.0f, -1.0f);
			}
			if (weapon_def->noexplode && hit && !rangeReached) // Special flag used by dguns
			{
				dying = false;
				position.y += fabsf(3.0f * dt * velocity.y);
			}
			else
			{
				if (weapon_def->rendertype == RENDER_TYPE_LASER)
				{
					dying = true;
					killtime = weapon_def->duration;
				}
				else
					weaponId = -1;
			}
		}
		else if (dying && killtime <= 0.0f)
			weaponId = -1;
		else if (position.x < -the_map->halfWidthInWorldUnits || position.x > the_map->halfWidthInWorldUnits || position.z < -the_map->halfHeightInWorldUnits || position.z > the_map->halfHeightInWorldUnits) // We're out of the map
			weaponId = -1;
	}

	void Weapon::draw() // Dessine les objets produits par les armes
	{
		if (weaponId < 0)
			return;

		visible = false;

		auto tileIndex = the_map->worldToGraphicalTileIndex(position);
		const int px = tileIndex.x;
		const int py = tileIndex.y;
		if (px < 0 || py < 0 || px >= the_map->widthInGraphicalTiles || py >= the_map->heightInGraphicalTiles)
			return;
		const PlayerMask player_mask = toPlayerMask(players.local_human_id);
		if (the_map->view(px, py) != 1 || !(the_map->sight_map(px, py) & player_mask))
			return;

		glPushMatrix();

		const WeaponDef* const weapon_def = &(weapon_manager.weapon[weaponId]);

		visible = true;
		switch (weapon_def->rendertype)
		{
			case RENDER_TYPE_LASER:			 // Draw the laser
				{
					Vector3D P(position);
					float length = weapon_def->duration;
					if (weapon_def->duration > stime)
						length = stime;
					if (dying && length > killtime)
						length = killtime;
					P = P - length * velocity;
					glDisable(GL_LIGHTING);
					int color0 = weapon_def->color[0];
					int color1 = weapon_def->color[1];
					float coef = (cosf(stime * 5.0f) + 1.0f) * 0.5f;
					GLubyte r = (GLubyte)(coef * float(getr(color0)) + (1.0f - coef) * float(getr(color1)));
					GLubyte g = (GLubyte)(coef * float(getg(color0)) + (1.0f - coef) * float(getg(color1)));
					GLubyte b = (GLubyte)(coef * float(getb(color0)) + (1.0f - coef) * float(getb(color1)));
					Vector3D D(position - Camera::inGame->position());
					Vector3D Up(D * velocity);
					Up.normalize();
					if (damage < 0.0f)
						damage = float(weapon_def->damage);
					Up = Math::Min(damage / 60.0f + float(weapon_def->firestarter) / 200.0f + float(weapon_def->areaofeffect) / 40.0f, 1.0f) * Up; // Variable width!!
					glDisable(GL_CULL_FACE);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

					if (weapon_def->laserTex1 == 0)
					{
						glDisable(GL_TEXTURE_2D);
						glBegin(GL_QUADS);
						glColor4ub(r, g, b, 0);
						glVertex3f(position.x + Up.x, position.y + Up.y, position.z + Up.z);
						glVertex3f(P.x + Up.x, P.y + Up.y, P.z + Up.z);
						glColor4ub(r, g, b, 0xFF);
						glVertex3f(P.x, P.y, P.z);
						glVertex3f(position.x, position.y, position.z);

						glVertex3f(position.x, position.y, position.z);
						glVertex3f(P.x, P.y, P.z);
						glColor4ub(r, g, b, 0);
						glVertex3f(P.x - Up.x, P.y - Up.y, P.z - Up.z);
						glVertex3f(position.x - Up.x, position.y - Up.y, position.z - Up.z);
						glEnd();
					}
					else
					{
						byte a = byte(weapon_def->laserTex2 ? int(0xFF * coef) : 0xFF);
						glEnable(GL_TEXTURE_2D);
						glBindTexture(GL_TEXTURE_2D, weapon_def->laserTex1);
						glColor4ub(r, g, b, a);
						glBegin(GL_QUADS);
						glTexCoord2f(0.0f, 0.0f);
						glVertex3f(position.x + Up.x, position.y + Up.y, position.z + Up.z);
						glTexCoord2f(1.0f, 0.0f);
						glVertex3f(P.x + Up.x, P.y + Up.y, P.z + Up.z);
						glTexCoord2f(1.0f, 1.0f);
						glVertex3f(P.x - Up.x, P.y - Up.y, P.z - Up.z);
						glTexCoord2f(0.0f, 1.0f);
						glVertex3f(position.x - Up.x, position.y - Up.y, position.z - Up.z);
						glEnd();

						if (a != 0xFF)
						{
							glDepthFunc(GL_LEQUAL);
							a = byte(0xFF - a);
							glBindTexture(GL_TEXTURE_2D, weapon_def->laserTex2);
							glColor4ub(r, g, b, a);
							glBegin(GL_QUADS);
							glTexCoord2f(0.0f, 0.0f);
							glVertex3f(position.x + Up.x, position.y + Up.y, position.z + Up.z);
							glTexCoord2f(1.0f, 0.0f);
							glVertex3f(P.x + Up.x, P.y + Up.y, P.z + Up.z);
							glTexCoord2f(1.0f, 1.0f);
							glVertex3f(P.x - Up.x, P.y - Up.y, P.z - Up.z);
							glTexCoord2f(0.0f, 1.0f);
							glVertex3f(position.x - Up.x, position.y - Up.y, position.z - Up.z);
							glEnd();
							glDepthFunc(GL_LESS);
						}
					}
					glDisable(GL_BLEND);
					glEnable(GL_CULL_FACE);
				}
				break;
			case RENDER_TYPE_MISSILE: // Dessine le missile
				if (weapon_def->model)
				{
					glTranslatef(position.x, position.y, position.z);

					Vector3D I(0.0f, 0.0f, 1.0f), Dir(velocity);
					Dir.normalize();
					Vector3D J(velocity * I);
					J.normalize();
					float theta = -acosf(Dir.z) * RAD2DEG;
					glRotatef(theta, J.x, J.y, J.z);

					glEnable(GL_LIGHTING);
					glEnable(GL_TEXTURE_2D);
					glDisable(GL_CULL_FACE);
					weapon_def->model->draw(0.0f);
					glEnable(GL_CULL_FACE);
				}
				break;
			case RENDER_TYPE_BITMAP:
				glDisable(GL_LIGHTING);
				glDisable(GL_TEXTURE_2D);
				glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
				if (weapon_manager.cannonshell.nb_bmp > 0)
				{
					anim_sprite = short(((int)(stime * 15.0f)) % weapon_manager.cannonshell.nb_bmp);
					gfx->set_alpha_blending();
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, weapon_manager.cannonshell.glbmp[anim_sprite]);
					Vector3D A, B, C, D;
					A = position + ((-0.5f * float(weapon_manager.cannonshell.h[anim_sprite] - weapon_manager.cannonshell.ofs_y[anim_sprite])) * Camera::inGame->up() + (-0.5f * float(weapon_manager.cannonshell.w[anim_sprite] - weapon_manager.cannonshell.ofs_x[anim_sprite])) * Camera::inGame->side());
					B = position + ((-0.5f * float(weapon_manager.cannonshell.h[anim_sprite] - weapon_manager.cannonshell.ofs_y[anim_sprite])) * Camera::inGame->up() + (0.5f * float(weapon_manager.cannonshell.w[anim_sprite] - weapon_manager.cannonshell.ofs_x[anim_sprite])) * Camera::inGame->side());
					C = position + ((0.5f * float(weapon_manager.cannonshell.h[anim_sprite] - weapon_manager.cannonshell.ofs_y[anim_sprite])) * Camera::inGame->up() + (-0.5f * float(weapon_manager.cannonshell.w[anim_sprite] - weapon_manager.cannonshell.ofs_x[anim_sprite])) * Camera::inGame->side());
					D = position + ((0.5f * float(weapon_manager.cannonshell.h[anim_sprite] - weapon_manager.cannonshell.ofs_y[anim_sprite])) * Camera::inGame->up() + (0.5f * float(weapon_manager.cannonshell.w[anim_sprite] - weapon_manager.cannonshell.ofs_x[anim_sprite])) * Camera::inGame->side());
					glBegin(GL_QUADS);
					glTexCoord2f(0.0f, 0.0f);
					glVertex3f(A.x, A.y, A.z);
					glTexCoord2f(1.0f, 0.0f);
					glVertex3f(B.x, B.y, B.z);
					glTexCoord2f(1.0f, 1.0f);
					glVertex3f(D.x, D.y, D.z);
					glTexCoord2f(0.0f, 1.0f);
					glVertex3f(C.x, C.y, C.z);
					glEnd();
					gfx->unset_alpha_blending();
				}
				else
				{
					glBegin(GL_QUADS);
					glVertex3f(position.x - 2.5f, position.y - 2.5f, position.z);
					glVertex3f(position.x + 2.5f, position.y - 2.5f, position.z);
					glVertex3f(position.x + 2.5f, position.y + 2.5f, position.z);
					glVertex3f(position.x - 2.5f, position.y + 2.5f, position.z);
					glEnd();
				}
				glEnable(GL_LIGHTING);
				break;
			case RENDER_TYPE_BOMB:
				if (weapon_def->model)
				{
					glTranslatef(position.x, position.y, position.z);

					Vector3D I(0.0f, 0.0f, 1.0f), Dir(velocity);
					Dir.normalize();
					Vector3D J(velocity * I);
					J.normalize();
					float theta = -acosf(Dir.z) * RAD2DEG;
					glRotatef(theta, J.x, J.y, J.z);

					glEnable(GL_LIGHTING);
					glEnable(GL_TEXTURE_2D);
					glDisable(GL_CULL_FACE);
					weapon_def->model->draw(0.0f);
					glEnable(GL_CULL_FACE);
				}
				break;
			case RENDER_TYPE_LIGHTNING:
			{
				Vector3D P = position;
				float length = weapon_def->duration;
				if (weapon_def->duration > stime)
					length = stime;
				if (dying && length > killtime)
					length = killtime;
				P = P - length * velocity;
				glDisable(GL_LIGHTING);
				glDisable(GL_TEXTURE_2D);
				int color0 = weapon_def->color[0];
				int color1 = weapon_def->color[1];
				float coef = (cosf(stime) + 1.0f) * 0.5f;

				GLubyte r = (GLubyte)(coef * float((color0 >> 16) & 0xFF) + coef * float((color1 >> 16) & 0xFF));
				GLubyte g = (GLubyte)(coef * float((color0 >> 8) & 0xFF) + coef * float((color1 >> 8) & 0xFF));
				GLubyte b = (GLubyte)(coef * float(color0 & 0xFF) + coef * float(color1 & 0xFF));
				glColor4ub(r, g, b, 0xFF);
				glBegin(GL_LINE_STRIP);

				float x = 0.f;
				float y = 0.f;
				float z = 0.f;
				for (unsigned int i = 0; i < TA3D_PLAYERS_HARD_LIMIT; ++i)
				{
					if (i > 0 && i < 9)
					{
						x = float(((sint32)(Math::RandomTable() % 2001)) - 1000) * 0.005f;
						y = float(((sint32)(Math::RandomTable() % 2001)) - 1000) * 0.005f;
						z = float(((sint32)(Math::RandomTable() % 2001)) - 1000) * 0.005f;
					}
					glVertex3f(position.x + (P.x - position.x) * float(i) / 9.0f + x, position.y + (P.y - position.y) * float(i) / 9.0f + y, position.z + (P.z - position.z) * float(i) / 9.0f + z);
				}
				glEnd();
			}
			break;
			case RENDER_TYPE_DGUN: // Dessine le dgun
				if (weapon_def->model)
				{
					glTranslatef(position.x, position.y, position.z);

					Vector3D I(0.0f, 0.0f, 1.0f), Dir(velocity);
					Dir.normalize();
					Vector3D J(velocity * I);
					J.normalize();
					glRotatef((float)(-acosf(Dir.z) * RAD2DEG), J.x, J.y, J.z);

					glEnable(GL_LIGHTING);
					glEnable(GL_TEXTURE_2D);
					glDisable(GL_CULL_FACE);
					weapon_def->model->draw(0.0f);
					glEnable(GL_CULL_FACE);
				}
				break;
			case RENDER_TYPE_GUN: // Dessine une "balle"
				glDisable(GL_LIGHTING);
				glDisable(GL_TEXTURE_2D);
				glBegin(GL_POINTS);
				glColor3ub(0xBF, 0xBF, 0xBF);
				glVertex3f(position.x, position.y, position.z);
				glEnd();
				break;
			case RENDER_TYPE_PARTICLES: // Dessine des particules
				glDisable(GL_LIGHTING);
				glDisable(GL_TEXTURE_2D);
				glBegin(GL_POINTS);
				glColor3ub(0xBF, 0xBF, 0xBF);
				for (int i = 0; i < 10; ++i)
				{
					glVertex3f(
						position.x + float(Math::RandomTable() % 201) * 0.01f - 1.0f,
						position.y + float(Math::RandomTable() % 201) * 0.01f - 1.0f,
						position.z + float(Math::RandomTable() % 201) * 0.01f - 1.0f);
				}
				glEnd();
				break;
		}
		glPopMatrix();
	}

} // namespace TA3D
