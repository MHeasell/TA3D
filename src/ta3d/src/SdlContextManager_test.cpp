#include <catch.hpp>
#include <SDL_version.h>
#include "SdlContextManager.h"

namespace TA3D
{
	TEST_CASE("formatSdlVersion")
	{
		SECTION("creates a printable string from SDL_version struct")
		{
			SDL_version v;
			v.major = 2;
			v.minor = 0;
			v.patch = 5;
			String s = formatSdlVersion(v);
			REQUIRE(s == "2.0.5");
		}
	}
}
