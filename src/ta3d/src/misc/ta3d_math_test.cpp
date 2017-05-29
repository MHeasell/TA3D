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

	TEST_CASE("snapToInterval")
	{
		SECTION("quantizes a value to the nearest multiple of the interval")
		{
			REQUIRE(Math::snapToInterval(11.3f, 2.0f) == Approx(12.0f));
			REQUIRE(Math::snapToInterval(11.0f, 2.0f) == Approx(12.0f));
			REQUIRE(Math::snapToInterval(-22.0f, 5.0f) == Approx(-20.0f));
			REQUIRE(Math::snapToInterval(-23.0f, 5.0f) == Approx(-25.0f));
			REQUIRE(Math::snapToInterval(-4.3f, 0.5f) == Approx(-4.5f));
			REQUIRE(Math::snapToInterval(-4.2f, 0.5f) == Approx(-4.0f));
			REQUIRE(Math::snapToInterval(0.24f, 0.5f) == Approx(0.0f));
			REQUIRE(Math::snapToInterval(0.25f, 0.5f) == Approx(0.5f));
			REQUIRE(Math::snapToInterval(0.26f, 0.5f) == Approx(0.5f));
		}
	}
}
