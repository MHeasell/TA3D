#include <catch.hpp>
#include "grid.h"

namespace TA3D
{
	TEST_CASE("Grid")
	{
		Grid<int> g(3, 4);

		SECTION("has the given width and height")
		{
			REQUIRE(g.getWidth() == 3);
			REQUIRE(g.getHeight() == 4);
		}

		SECTION("can be written to")
		{
			g(0, 0) = 2;
			g(2, 3) = 10;
			REQUIRE(g(0, 0) == 2);
			REQUIRE(g(2, 3) == 10);
		}
	}
}
