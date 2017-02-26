#include "mutex.h"

namespace TA3D
{
	Synchronizer::Synchronizer(int nbThreadsToSync)
		: nbThreadsToSync(nbThreadsToSync),
		  nbThreadsWaiting(0),
		  pSignalled(0)
	{
	}

	Synchronizer::~Synchronizer()
	{
		nbThreadsToSync = 0;
		release();
	}

	void Synchronizer::sync()
	{
		std::unique_lock<std::mutex> lock(pMutex);
		++nbThreadsWaiting;

		if (nbThreadsToSync <= nbThreadsWaiting)
		{
			nbThreadsWaiting = 0;
			++pSignalled;
			lock.unlock();
			pCondition.notify_all();
		}
		else
		{
			// The pCondition.wait() will release the lock and wait for
			// signalling.

			unsigned int curSignal = pSignalled;
			do
			{
				// Spurious wakeups from this function can occur.
				// Therefore we must check out pSignalled variable to ensure we have
				// really been signalled.
				pCondition.wait(lock);
			} while (pSignalled == curSignal);

			lock.unlock();
		}
	}

	void Synchronizer::release()
	{
		pMutex.lock();

		nbThreadsWaiting = 0;
		++pSignalled;

		pCondition.notify_all();

		pMutex.unlock();
	}
}
