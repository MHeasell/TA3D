#include <catch.hpp>
#include "ta3d_math.h"

namespace TA3D
{
	TEST_CASE("Clamp")
	{
		SECTION("clamps a value in a range")
		{
			REQUIRE(Math::Clamp(4, 1, 10) == 4);
			REQUIRE(Math::Clamp(0, 1, 10) == 1);
			REQUIRE(Math::Clamp(11, 1, 10) == 10);
		}
	}
}
