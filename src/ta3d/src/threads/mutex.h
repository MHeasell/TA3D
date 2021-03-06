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

#ifndef __TA3D_THREADS_MUTEX_H__
#define __TA3D_THREADS_MUTEX_H__

#include <mutex>
#include <condition_variable>

namespace TA3D
{

	typedef std::recursive_mutex Mutex;

	class MutexLocker
	{
	public:
		MutexLocker(Mutex& m) : lock(m) {}
	private:
		std::unique_lock<std::recursive_mutex> lock;
	}; // MutexLocker

	class Synchronizer
	{
	public:
		Synchronizer(int nbThreadsToSync);
		virtual ~Synchronizer();

		void sync();
		void release();
		void lock() { pMutex.lock(); }
		void unlock() { pMutex.lock(); }

		int getNbWaitingThreads() { return nbThreadsWaiting; }
		void setNbThreadsToSync(int n) { nbThreadsToSync = n; }

	protected:
		int nbThreadsToSync;
		volatile int nbThreadsWaiting;
		std::mutex pMutex;

		std::condition_variable pCondition;

		//! Have the condition been really signalled ?
		volatile unsigned int pSignalled;
	};

} // namespace TA3D

#endif // __TA3D_THREADS_MUTEX_H__
