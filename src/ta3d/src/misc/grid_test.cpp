#include <catch.hpp>
#include "grid.h"

namespace TA3D
{
	TEST_CASE("Grid")
	{
		SECTION("when constructed")
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

			SECTION("can be filled")
			{
				g.fill(5);
				for (int i = 0; i < 3; ++i)
				{
					for (int j = 0; j < 4; ++j)
					{
						REQUIRE(g(i, j) == 5);
					}
				}
			}
		}

		SECTION("zero-argument ctor")
		{
			SECTION("constructs an empty grid")
			{
				Grid<int> g;
				REQUIRE(g.getWidth() == 0);
				REQUIRE(g.getHeight() == 0);
			}
		}
	}

	TEST_CASE("Grid.circlefill")
	{
		SECTION("Fills a circular area in the grid")
		{
			std::vector<int> arr {
				0, 0, 0, 1, 0, 0, 0,
				0, 1, 1, 1, 1, 1, 0,
				0, 1, 1, 1, 1, 1, 0,
				1, 1, 1, 1, 1, 1, 1,
				0, 1, 1, 1, 1, 1, 0,
				0, 1, 1, 1, 1, 1, 0,
				0, 0, 0, 1, 0, 0, 0,
			};
			Grid<int> expected(7, 7, arr);

			Grid<int> g(7, 7, 0);
			g.circlefill(3, 3, 3, 1);

			REQUIRE(g == expected);
		}
	}
}
