#include "suspend.h"
#include <thread>
#include <chrono>

namespace TA3D
{
	void SuspendMilliSeconds(unsigned int milliseconds)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	}
}
