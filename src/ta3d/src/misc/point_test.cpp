#include <catch.hpp>
#include "point.h"

namespace TA3D
{
	TEST_CASE("Point")
	{
		SECTION("initially holds zeroes")
		{
			Point<int> p;
			REQUIRE(p.x == 0);
			REQUIRE(p.y == 0);
		}
		SECTION("holds info about 2D coordinates")
		{
			Point<int> p(4, 6);
			REQUIRE(p.x == 4);
			REQUIRE(p.y == 6);
		}
	}
}
