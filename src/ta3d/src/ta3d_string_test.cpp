#include <catch.hpp>
#include "ta3d_string.h"

namespace TA3D {

	TEST_CASE("String.explode()")
	{
		SECTION("when the string is empty")
		{
			String s("");

			SECTION("emits empty list")
			{
				String::Vector out;
				s.explode(out, ",");
				REQUIRE(out.size() == 0);
			}
		}
	}
}
