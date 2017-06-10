#include <catch.hpp>
#include "vector.h"

namespace TA3D
{
	TEST_CASE("Vector.cross()")
	{
		SECTION("returns the cross product of two simple vectors")
		{
			Vector3D a(1.0f, 0.0f, 0.0f);
			Vector3D b(0.0f, 1.0f, 0.0f);
			Vector3D c = a.cross(b);

			REQUIRE(c.x == 0.0f);
			REQUIRE(c.y == 0.0f);
			REQUIRE(c.z == 1.0f);
		}
	}

	TEST_CASE("Vector.dot")
	{
		SECTION("returns 0 for perpendicular vectors")
		{
			Vector3D a(1.0f, 0.0f, 0.0f);
			Vector3D b(0.0f, 1.0f, 0.0f);
			REQUIRE(a.dot(b) == 0.0f);
		}

		SECTION("computes dot product")
		{
			Vector3D a(1.0f, 2.0f, 3.0f);
			Vector3D b(4.0f, 5.0f, 6.0f);
			REQUIRE(a.dot(b) == 32.0f);
		}
	}
	TEST_CASE("Vector operator<<")
	{
		SECTION("prints the vector to the stream")
		{
			Vector3D v(0.5f, 1.0f, 1.5f);
			std::ostringstream os;
			os << v;
			REQUIRE(os.str() == "(0.5, 1, 1.5)");
		}
	}

	TEST_CASE("Vector.normalized")
	{
		SECTION("returns a new normalized vector")
		{
			Vector3D v(3.0, 4.0, 0.0);
			Vector3D n = v.normalized();
			REQUIRE(n.x == Approx(0.6f));
			REQUIRE(n.y == Approx(0.8f));
			REQUIRE(n.z == Approx(0.0f));
		}
	}
}
