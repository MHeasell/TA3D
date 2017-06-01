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

/*-----------------------------------------------------------------\
|                           pathfinding.cpp                        |
|   contient toutes les fonctions et classes nécessaires à la mise |
| en place du pathfinding de TA3D                                  |
\-----------------------------------------------------------------*/

#include <stdafx.h>
#include <misc/matrix.h>
#include <TA3D_NameSpace.h>
#include <ta3dbase.h>
#include <scripts/cob.h> // For unit scripts / Pour la lecture et l'éxecution des scripts
#include <tdf.h>		 // For map features / Pour la gestion des éléments du jeu
#include <EngineClass.h> // The engine, also includes pathfinding.h / Inclus le moteur(dont le fichier pathfinding.h)
#include <misc/ta3d_math.h>
#include <UnitEngine.h>
#include <misc/usectimer.h>
#include <misc/bitmap.h>
#include <algorithm>
#include <thread>

#define PATHFINDER_MAX_LENGTH 500000

namespace TA3D
{
	inline int sq(int a)
	{
		return a * a;
	}

	Mutex Pathfinder::sMutex;

	Pathfinder* Pathfinder::instance()
	{
		static Pathfinder sInstance;
		return &sInstance;
	}

	namespace AI
	{
		void Path::next()
		{
			if (nodes.empty())
				return;

			nodes.pop_front();
			computeCoord();
		}

		void Path::clear()
		{
			nodes.clear();
			pos.reset();
			_ready = false;
		}

		void Path::computeCoord()
		{
			if (nodes.empty())
				return;

			pos.x = float((nodes.front().x() << 3) - the_map->halfWidthInWorldUnits);
			pos.z = float((nodes.front().z() << 3) - the_map->halfHeightInWorldUnits);
			pos.y = 0.0f;
		}

#define SAVE(i) gzwrite(file, (void*)&(i), sizeof(i))
#define LOAD(i) gzread(file, (void*)&(i), sizeof(i))

		void Path::save(gzFile file)
		{
			int n = int(nodes.size());
			SAVE(n);
			for (iterator i = begin(); i != end(); ++i)
			{
				SAVE(i->x());
				SAVE(i->z());
			}
		}

		void Path::load(gzFile file)
		{
			clear();
			int n(0);
			LOAD(n);
			for (int i = 0; i < n; ++i)
			{
				Node node(0, 0);
				LOAD(node.x());
				LOAD(node.z());
				nodes.push_back(node);
			}
		}
	}

	Pathfinder::Pathfinder() : tasks(), stasks(), taskOffset(0), nbCores(), pSync(2), bRunning(false)
	{
		nbCores = std::max(std::thread::hardware_concurrency(), 1u);
	}

	int Pathfinder::taskCount()
	{
		MutexLocker mLock(pMutex);
		return int(tasks.size());
	}

	void Pathfinder::clear()
	{
		MutexLocker mLock(pMutex);
		tasks.clear();
		stasks.clear();
		taskOffset = 0;
	}

	void Pathfinder::addTask(int idx, int dist, const Vector3D& start, const Vector3D& end)
	{
		Task t = {dist, idx, units.unit[idx].ID, start, end};

		lock();

		TaskSet::iterator pos = stasks.find(t.UID);
		const bool found = (pos != stasks.end());

		if (found) // If it's in the queue, then change the request
			tasks[*pos - taskOffset] = t;
		// Otherwise add a new request to the task list
		else
		{
			stasks[t.UID] = int(tasks.size()) + taskOffset;
			tasks.push_back(t);
		}

		if (!isRunning())
			this->start();
		unlock();

		pSync.release();
	}

	bool Pathfinder::hasTaskQueueForUnit(uint32 UID)
	{
		MutexLocker mLock(pMutex);
		return stasks.count(UID) != 0;
	}

	AI::Path Pathfinder::directPath(const Vector3D& end)
	{
		auto heightmapIndex = the_map->worldToHeightmapIndex(end);
		const int x = heightmapIndex.x;
		const int z = heightmapIndex.y;
		AI::Path path;
		path.push_back(AI::Path::Node(x, z));
		path.setPos(end);
		path._ready = true;

		return path;
	}

	Pathfinder::~Pathfinder()
	{
		destroyThread();

		for (HashMap<BitMap*>::Dense::iterator it = hBitMap.begin(); it != hBitMap.end(); ++it)
			if (*it)
				delete *it;
	}

	void Pathfinder::proc(void*)
	{
		bRunning = true;
		while (!pDead)
		{
			lock();
			if (tasks.empty())
			{
				unlock();
				pSync.sync();
				continue;
			}
			Task cur = tasks.front();
			stasks.erase(cur.UID);
			tasks.pop_front();
			++taskOffset;
			unlock();

			// Here we are free to compute this path
			const uint32 start_timer = MILLISECONDS_SINCE_INIT;
			if (cur.idx >= 0 && units.unit[cur.idx].ID == cur.UID && units.unit[cur.idx].isAlive())
			{
				AI::Path path;
				findPath(path, cur);
				path._ready = true;

				Unit* pUnit = &(units.unit[cur.idx]);
				pUnit->lock();
				if (pUnit->ID == cur.UID && pUnit->isAlive() && !pUnit->missionQueue.empty() && pUnit->requesting_pathfinder && (pUnit->missionQueue->getFlags() & MISSION_FLAG_MOVE))
				{
					pUnit->missionQueue->Path().replaceWith(path);
					pUnit->requesting_pathfinder = false;
					if (pUnit->missionQueue->Path().empty()) // Can't find a path to get where it has been ordered to go
						pUnit->playSound("cant1");
				}
				pUnit->unlock();
			}

			// We don't want to use more than 25% of the CPU here
			if (suspend((nbCores == 1) ? ((MILLISECONDS_SINCE_INIT - start_timer) << 2) : 0))
			{
				// The thread should stop as soon as possible
				bRunning = false;
				return;
			}
		}
		bRunning = false;
	}

	void Pathfinder::findPath(AI::Path& path, const Task& task)
	{
		if (task.idx >= 0)
		{
			units.unit[task.idx].lock();
			if (units.unit[task.idx].ID != task.UID || units.unit[task.idx].type_id < 0)
			{
				units.unit[task.idx].unlock();
				return;
			}
		}
		const UnitType* pType = task.idx >= 0
			? unit_manager.unit_type[units.unit[task.idx].type_id]
			: unit_manager.unit_type[-task.idx];
		if (task.idx >= 0)
			units.unit[task.idx].unlock();

		// Get the precomputed walkable area quadmap
		const BitMap* bmap = Pathfinder::instance()->hBitMap[pType->getMoveStringID()];
		if (!bmap)
		{
			LOG_ERROR(LOG_PREFIX_PATHS << "path request for a unit without precomputed walkable area quadmap");
			return;
		}

		MutexLocker mLock(sMutex);

		auto startHeightmapIndex = the_map->worldToHeightmapIndex(task.start);
		const int start_x = startHeightmapIndex.x;
		const int start_z = startHeightmapIndex.y;

		auto endHeightmapIndex = the_map->worldToHeightmapIndex(task.end);
		const int end_x = endHeightmapIndex.x;
		const int end_z = endHeightmapIndex.y;

		static std::vector<AI::Path::Node> nodes;
		nodes.clear();
		nodes.push_back(AI::Path::Node(start_x, start_z));

		static const int order_p1[] = {1, 2, 3, 4, 5, 6, 7, 0};
		static const int order_p2[] = {2, 3, 4, 5, 6, 7, 0, 1};
		static const int order_m1[] = {7, 0, 1, 2, 3, 4, 5, 6};
		static const int order_m2[] = {6, 7, 0, 1, 2, 3, 4, 5};
		static const int order_dx[] = {-1, 0, 1, 1, 1, 0, -1, -1};
		static const int order_dz[] = {-1, -1, -1, 0, 1, 1, 1, 0};
		static const int order_d[] = {10, 7, 10, 7, 10, 7, 10, 7};

		const int m_dist = task.dist * task.dist;

		const int smh = pType->FootprintZ;
		const int smw = pType->FootprintX;

		const int mw_h = smw >> 1;
		const int mh_h = smh >> 1;

		if (nodes.back().x() < 0 || nodes.back().z() < 0 || nodes.back().x() >= the_map->widthInHeightmapTiles || nodes.back().z() >= the_map->heightInHeightmapTiles) // Hum we are out !!
		{
			path.clear();
			return; // So we can't find a path
		}

		Grid<int>& zone = the_map->path;
		const Grid<float>& energy = the_map->energy;
		std::deque<AI::Path::Node> qNode;
		std::deque<int> qDistFromStart;

		bool pathFound = true;
		if ((m_dist == 0 && (nodes.back().x() != end_x || nodes.back().z() != end_z)) || (m_dist > 0 && sq(nodes.back().x() - end_x) + sq(nodes.back().z() - end_z) > m_dist))
		{
			nodes.reserve(PATHFINDER_MAX_LENGTH);
			pathFound = false;
			uint32 curDistFromStart = 0;
			uint32 minPathLength = uint32(-1);
			uint32 depthLimit = PATHFINDER_MAX_LENGTH;
			const float distanceCoef = 2.0f * float(pType->MaxSlope);
			uint32 n = 0;
			while (n < depthLimit)
			{
				++zone(nodes.back().x(), nodes.back().z());

				int m = -1;

				int nx = nodes.back().x() + Math::Sgn(end_x - nodes.back().x());
				int nz = nodes.back().z() + Math::Sgn(end_z - nodes.back().z());

				if (nx < 0 || nz < 0 || nx >= the_map->widthInHeightmapTiles || nz >= the_map->heightInHeightmapTiles)
					break; // If we have to go out there is a problem ...

				if (zone(nx, nz) >= 2 || curDistFromStart > minPathLength)
				{
					if (qNode.empty()) // We're done
						break;
					nodes.push_back(qNode.back());
					qNode.pop_back();
					curDistFromStart = qDistFromStart.back();
					qDistFromStart.pop_back();
					++n;
					continue; // Instead of looping we restart from a Node in the qNode
				}

				if (zone(nx, nz) || !(*bmap)(nx, nz) || !checkRectFast(nx - mw_h, nz - mh_h, pType))
				{
					float dist[8];
					bool zoned[8];
					for (int e = 0; e < 8; ++e) // Gather required data
					{
						dist[e] = -1.0f;
						nx = nodes.back().x() + order_dx[e];
						nz = nodes.back().z() + order_dz[e];
						zoned[e] = false;
						if (nx < 0 || nz < 0 || nx >= the_map->widthInHeightmapTiles || nz >= the_map->heightInHeightmapTiles)
							continue;
						zoned[e] = zone(nx, nz);
						if (!zoned[e] && !(*bmap)(nx, nz))
							continue;
						if (!zoned[e] && !checkRectFast(nx - mw_h, nz - mh_h, pType))
							continue;
						dist[e] = distanceCoef * sqrtf(float(sq(end_x - nx) + sq(end_z - nz))) + energy(nx, nz);

						if (zoned[e])
							dist[e] = -1.0f;
					}
					for (int e = 0; e < 8; ++e) // Look for a way to go
					{
						if (((dist[order_m1[e]] < 0.0f && !zoned[order_m1[e]]) || (dist[order_p1[e]] < 0.0f && !zoned[order_p1[e]]) || (dist[order_m2[e]] < 0.0f && !zoned[order_m2[e]]) || (dist[order_p2[e]] < 0.0f && !zoned[order_p2[e]])) && dist[e] >= 0.0f)
						{
							if (m == -1)
								m = e;
							else if (dist[e] < dist[m])
								m = e;
						}
					}
					if (m == -1) // Second try
					{
						for (int e = 0; e < 8; ++e)
						{
							if (dist[e] >= 0.0f && ((!zoned[order_m1[e]] && !zoned[order_p1[e]]) || pathFound))
							{
								if (m == -1)
									m = e;
								else if (dist[e] < dist[m])
									m = e;
							}
						}
					}
					if (m >= 0) // We found something
					{
						nx = nodes.back().x() + order_dx[m];
						nz = nodes.back().z() + order_dz[m];
						// add unexplored nodes to the queue
						for (int i = 0; i < 8; ++i)
						{
							if (i != m && dist[i] >= 0.0f)
							{
								if ((dist[order_m1[i]] < 0.0f && !zoned[order_m1[i]]) || (dist[order_p1[i]] < 0.0f && !zoned[order_p1[i]]) || (dist[order_m2[i]] < 0.0f && !zoned[order_m2[i]]) || (dist[order_p2[i]] < 0.0f && !zoned[order_p2[i]]))
								{
									qNode.push_back(AI::Path::Node(nodes.back().x() + order_dx[i], nodes.back().z() + order_dz[i]));
									qDistFromStart.push_back(curDistFromStart + order_d[i]);
								}
							}
						}
						curDistFromStart += order_d[m];
					}
				}
				else
					m = -2;

				if (m == -1)
				{
					if (qNode.empty()) // We're done
						break;
					nodes.push_back(qNode.back());
					qNode.pop_back();
					curDistFromStart = qDistFromStart.back();
					qDistFromStart.pop_back();
					++n;
					continue; // Instead of looping we restart from a Node in the qNode
				}

				nodes.push_back(AI::Path::Node(nx, nz));

				++n;
				if ((m_dist == 0 && nodes.back().x() == end_x && nodes.back().z() == end_z) || (m_dist > 0 && sq(nodes.back().x() - end_x) + sq(nodes.back().z() - end_z) <= m_dist))
				{
					minPathLength = Math::Min(minPathLength, curDistFromStart);
					if (!pathFound)
						depthLimit = n << 1; // Limit search complexity
					pathFound = true;
					if (qNode.empty()) // We're done
						break;
					nodes.push_back(qNode.back());
					qNode.pop_back();
					curDistFromStart = qDistFromStart.back();
					qDistFromStart.pop_back();
					++n;
					break;
				}
			}
		}

		if (!nodes.empty() && pathFound)
		{
			static std::vector<AI::Path::Node> cleanList;
			static std::vector<AI::Path::Node> nextPass;
			cleanList.clear();
			nextPass.clear();
#define SEARCH_AREA_WIDTH 16
			cleanList.reserve(nodes.size() * (2 * SEARCH_AREA_WIDTH + 1));
			nextPass.reserve(nodes.size() * 4);
			for (std::vector<AI::Path::Node>::const_iterator cur = nodes.begin(), end = nodes.end(); cur != end; ++cur) // Mark the path with a special pattern
			{
				const int x = cur->x();
				const int z = cur->z();
				zone(x, z) = 1;
				if (x > 0)
					nextPass.push_back(AI::Path::Node(x - 1, z));
				if (z > 0)
					nextPass.push_back(AI::Path::Node(x, z - 1));
				if (x + 1 < zone.getWidth())
					nextPass.push_back(AI::Path::Node(x + 1, z));
				if (z + 1 < zone.getHeight())
					nextPass.push_back(AI::Path::Node(x, z + 1));
			}
			static std::vector<AI::Path::Node> curPass;
			curPass.clear();
			for (int i = 0; i < SEARCH_AREA_WIDTH; ++i)
			{
				const bool last = (i + 1) == SEARCH_AREA_WIDTH;
				curPass.swap(nextPass);
				nextPass.reserve(curPass.size() * 4);
				nextPass.clear();
				for (std::vector<AI::Path::Node>::const_iterator it = curPass.begin(), end = curPass.end(); it != end; ++it)
				{
					const int x = it->x();
					const int z = it->z();
					if (zone(x, z))
						continue;
					if (!(*bmap)(x, z) || !checkRectFast(x - mw_h, z - mh_h, pType))
						continue;
					zone(x, z) = 1;
					cleanList.push_back(*it);

					if (last)
						continue;

					if (x > 0)
						nextPass.push_back(AI::Path::Node(x - 1, z));
					if (z > 0)
						nextPass.push_back(AI::Path::Node(x, z - 1));
					if (x + 1 < zone.getWidth())
						nextPass.push_back(AI::Path::Node(x + 1, z));
					if (z + 1 < zone.getHeight())
						nextPass.push_back(AI::Path::Node(x, z + 1));
				}
			}
#undef SEARCH_AREA_WIDTH

			qNode.clear();
			if (m_dist == 0)
			{
				if (zone(end_x, end_z) == 1)
				{
					qNode.push_back(AI::Path::Node(end_x, end_z));
					zone(end_x, end_z) = 2;
				}
			}
			else
			{
				for (int z = -task.dist; z <= task.dist; ++z)
				{
					if (end_z + z < 0 || end_z + z >= the_map->heightInHeightmapTiles)
						continue;
					const int dx = int(sqrtf(float(m_dist - z * z)) + 0.5f);
					for (int x = -dx; x <= dx && end_x + x < the_map->widthInHeightmapTiles; ++x)
						if (end_x + x >= 0 && zone(end_x + x, end_z + z) == 1)
						{
							qNode.push_back(AI::Path::Node(end_x + x, end_z + z));
							zone(end_x + x, end_z + z) = 2;
						}
				}
			}

			while (!qNode.empty()) // Fill the discovered region with the distance to end
			{
				AI::Path::Node cur = qNode.front();
				qNode.pop_front();

				const int ref = zone(cur.x(), cur.z());
				for (int i = 0; i < 8; ++i)
				{
					if (cur.x() + order_dx[i] < 0 || cur.x() + order_dx[i] >= the_map->widthInHeightmapTiles || cur.z() + order_dz[i] < 0 || cur.z() + order_dz[i] >= the_map->heightInHeightmapTiles)
						continue;
					const int t = zone(cur.x() + order_dx[i], cur.z() + order_dz[i]);
					const int r = ref + order_d[i];
					if (t == 1 || t > r)
					{
						zone(cur.x() + order_dx[i], cur.z() + order_dz[i]) = r;
						qNode.push_back(AI::Path::Node(cur.x() + order_dx[i], cur.z() + order_dz[i]));
					}
				}
			}

			static std::vector<AI::Path::Node> tmp;
			tmp.clear();
			tmp.swap(nodes);
			nodes.reserve(tmp.size());
			nodes.push_back(AI::Path::Node(start_x, start_z));
			const float coef = 0.05f / float(pType->MaxSlope);
			while ((m_dist == 0 && (nodes.back().x() != end_x || nodes.back().z() != end_z)) || (m_dist > 0 && sq(nodes.back().x() - end_x) + sq(nodes.back().z() - end_z) > m_dist)) // Reconstruct the path
			{
				zone(nodes.back().x(), nodes.back().z()) = 0;
				AI::Path::Node next = nodes.back();

				int b = -1;
				float m(0.0f);
				for (int i = 0; i < 8; ++i)
				{
					const int nx = next.x() + order_dx[i];
					const int nz = next.z() + order_dz[i];
					if (nx < 0 || nx >= the_map->widthInHeightmapTiles || nz < 0 || nz >= the_map->heightInHeightmapTiles)
						continue;
					const int t = zone(nx, nz);
					const float f = (float)t + coef * energy(nx, nz);
					if (t > 0 && (f < m || b == -1))
					{
						b = i;
						m = f;
					}
				}

				if (b == -1) // This should not be possible unless there is no path from start to end
				{
					nodes.clear();
					break;
				}
				next.x() += order_dx[b];
				next.z() += order_dz[b];

				nodes.push_back(next);
			}

			for (std::vector<AI::Path::Node>::const_iterator cur = tmp.begin(), end = tmp.end(); cur != end; ++cur) // Do some cleaning
				zone(cur->x(), cur->z()) = 0;
			for (std::vector<AI::Path::Node>::const_iterator cur = cleanList.begin(), end = cleanList.end(); cur != end; ++cur) // Do some cleaning
				zone(cur->x(), cur->z()) = 0;

			path.clear();
			for (std::vector<AI::Path::Node>::const_iterator cur = nodes.begin(), end = nodes.end(); cur != end; ++cur)
			{
				if (path.empty())
				{
					path.push_back(*cur);
					continue;
				}
				std::vector<AI::Path::Node>::const_iterator next = cur;
				++next;
				if (next == nodes.end() || (next->x() - path.back().x()) * (cur->z() - path.back().z()) != (cur->x() - path.back().x()) * (next->z() - path.back().z())) // Remove useless points
					path.push_back(*cur);
			}

			path.next(); // The unit is already at Start!! So remove it
		}
		else
		{
			for (std::vector<AI::Path::Node>::const_iterator cur = nodes.begin(), end = nodes.end(); cur != end; ++cur) // Clean the map data
				zone(cur->x(), cur->z()) = 0;
			path.clear();
		}
	}

	inline bool Pathfinder::checkRectFast(const int x1, const int y1, const UnitType* pType)
	{
		const int fy = Math::Min(y1 + pType->FootprintZ, the_map->heightInHeightmapTiles);
		const int fx = Math::Min(x1 + pType->FootprintX, the_map->widthInHeightmapTiles);
		const Grid<bool>& obstacles = the_map->obstacles;
		const int x0 = Math::Max(x1, 0);
		bool result = true;
		for (int y = Math::Max(y1, 0); y < fy && result; ++y)
			for (int x = x0; x < fx && result; ++x)
				result = !obstacles(x, y);
		return result;
	}

	bool Pathfinder::checkRectFull(int x1, int y1, const UnitType* pType)
	{
		const float dh_max = float(pType->MaxSlope) * H_DIV;
		const float h_min = pType->canhover ? -100.0f : the_map->sealvl - float(pType->MaxWaterDepth) * H_DIV;
		const float h_max = the_map->sealvl - float(pType->MinWaterDepth) * H_DIV;
		const float hover_h = pType->canhover ? the_map->sealvl : -100.0f;
		const int fy = Math::Min(y1 + pType->FootprintZ, the_map->heightInHeightmapTiles);
		const int fx = Math::Min(x1 + pType->FootprintX, the_map->widthInHeightmapTiles);
		const bool bfog = the_map->ota_data.whitefog;
		y1 = Math::Max(y1, 0);
		x1 = Math::Max(x1, 0);
		bool result = true;
		for (int y = y1; y < fy && result; ++y)
		{
			for (int x = x1; x < fx && result; ++x)
			{
				const float h = the_map->h_map(x, y);
				result = !((the_map->slope(x, y) > dh_max && h > hover_h) || the_map->map_data(x, y).isLava() || (h < the_map->sealvl && bfog) || h < h_min || h > h_max);
			}
		}
		return result;
	}

	void Pathfinder::signalExitThread()
	{
		pDead = 1;
		while (bRunning)
		{
			pSync.release();
			rest(1);
		}
	}

	void Pathfinder::computeWalkableAreas()
	{
		for (HashMap<BitMap*>::Dense::iterator it = hBitMap.begin(); it != hBitMap.end(); ++it)
			if (*it)
				delete *it;
		hBitMap.clear();
		size_t memoryUsed = 0U;
		const size_t end = unit_manager.unit_type.size();
		size_t i = 0;

		while (i < end)
		{
			const size_t e = i++;
			const UnitType* const pType = unit_manager.unit_type[e];
			if (!pType || pType->canfly || !pType->BMcode || !pType->canmove)
			{
				continue;
			}
			const String key = pType->getMoveStringID();
			if (hBitMap.count(key)) // Already done ?
				continue;
			BitMap* bmap = new BitMap(the_map->widthInHeightmapTiles, the_map->heightInHeightmapTiles);
			hBitMap[key] = bmap;


			const int mwh = pType->FootprintX >> 1;
			const int mhh = pType->FootprintZ >> 1;

			for (int y = 0; y < the_map->heightInHeightmapTiles; ++y)
			{
				for (int x = 0; x < the_map->widthInHeightmapTiles; ++x)
				{
					bmap->set(x, y, checkRectFull(x - mwh, y - mhh, pType));
				}
			}

			memoryUsed += bmap->getMemoryUse();
		}

		LOG_INFO(LOG_PREFIX_PATHS << "walkable areas : " << memoryUsed / 1024U << "kb");
	}
} // namespace TA3D
