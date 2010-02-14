#include "usectimer.h"
#include <sys/time.h>

namespace TA3D
{
	uint64 usectimer()
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return uint64(tv.tv_sec) * 1000000 + tv.tv_usec;
	}
}
