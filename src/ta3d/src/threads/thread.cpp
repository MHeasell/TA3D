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

#include "thread.h"
#include <logs/logs.h>
#include <typeinfo>
#include <chrono>

namespace TA3D
{
	Thread::Thread() : pDead(1), shouldStop(false)
	{
	}

	Thread::~Thread()
	{
		join();
	}

	void Thread::spawn(void* param)
	{
		pDead = 0;
		threadObj = std::thread([this, param] {
			proc(param);
			pDead = 1;
		});
	}

	void Thread::join()
	{
		if (threadObj.joinable())
		{
			if (pDead == 0)
			{
				signalExitThread();
				pDead = 1;
			}
			stop();
		}
	}

	void Thread::stop()
	{
		// notify the thread that it should stop
		{
			std::lock_guard<std::mutex> lock(shouldStopMutex);
			shouldStop = true;
		}
		shouldStopCondition.notify_one();

		// wait for the thread to complete
		threadObj.join();

		// we stopped, so we should no longer stop
		{
			std::lock_guard<std::mutex> lock(shouldStopMutex);
			shouldStop = false;
		}
	}

	// returns true if the thread should stop
	bool Thread::suspend(int ms)
	{
		std::unique_lock<std::mutex> lock(shouldStopMutex);

		// check if we should stop immediately
		if (shouldStop)
		{
			return true;
		}

		// go to sleep
		shouldStopCondition.wait_for(lock, std::chrono::milliseconds(ms));

		// we woke up either because the timer expired
		// or because we should stop now.
		return shouldStop;
	}
} // namespace TA3D
