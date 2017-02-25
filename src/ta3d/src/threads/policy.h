#ifndef __TA3D_THREADS_POLICY_H__
#define __TA3D_THREADS_POLICY_H__

#include <mutex>

namespace TA3D
{
	class ObjectLevelLockable
	{

	public:
		class MutexLocker
		{
		public:
			MutexLocker(const ObjectLevelLockable &obj) : lock(obj.mutex) {}
		private:
			std::unique_lock<std::recursive_mutex> lock;
		};

	private:
		mutable std::recursive_mutex mutex;
	};
}

#endif
