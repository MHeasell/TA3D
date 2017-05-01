#include <catch.hpp>
#include "Camera.h"
#include "vector.h"

namespace TA3D
{
	TEST_CASE("Camera2.screenToWorldRay")
	{
		SECTION("emits a ray into the world")
		{
			Vector2D pos(0.0f, 0.0f);
			Camera cam(400.0f, 300.0f);

			auto ray = cam.screenToWorldRay(pos);

			REQUIRE(ray.origin.x == Approx(0.0f));
			REQUIRE(ray.origin.y == Approx(1000.0f));
			REQUIRE(ray.origin.z == Approx(500.0f));

			REQUIRE(ray.direction.x == Approx(0.0f));
			REQUIRE(ray.direction.y == Approx(-2000.0f));
			REQUIRE(ray.direction.z == Approx(-1000.0f));
		}

		SECTION("copes with offset positions")
		{
			Vector2D pos(-0.5f, 0.5f);
			Camera cam(400.0f, 300.0f);

			auto ray = cam.screenToWorldRay(pos);

			REQUIRE(ray.origin.x == Approx(-100.0f));
			REQUIRE(ray.origin.y == Approx(1000.0f));
			REQUIRE(ray.origin.z == Approx(425.0f));

			REQUIRE(ray.direction.x == Approx(0.0f));
			REQUIRE(ray.direction.y == Approx(-2000.0f));
			REQUIRE(ray.direction.z == Approx(-1000.0f));
		}
	}
}
