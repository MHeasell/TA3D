#include <catch.hpp>
#include "geometry.h"

namespace TA3D
{
	TEST_CASE("Ray3D.pointAt")
	{
		SECTION("Gets the point after travelling for some time")
		{
			Ray3D r(Vector3D(3.0f, 10.0f, 4.0f), Vector3D(0.0f, -2.0f, 0.0f));
			auto i = r.pointAt(3.0f);
			REQUIRE(i.x == Approx(3.0f));
			REQUIRE(i.y == Approx(4.0f));
			REQUIRE(i.z == Approx(4.0f));
		}
	}

	TEST_CASE("Plane3D.intersect")
	{
		SECTION("Gets the time of intersection with a ray")
		{
			Ray3D r(Vector3D(3.0f, 10.0f, 4.0f), Vector3D(0.0f, -2.0f, 0.0f));
			Plane3D p(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));
			REQUIRE(p.intersect(r) == 5.0f);
		}

		SECTION("Returns infinity when the ray is parallel")
		{
			Ray3D r(Vector3D(1.0f, 1.0f, 1.0f), Vector3D(1.0f, 0.0f, 0.0f));
			Plane3D p(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));
			REQUIRE(std::abs(p.intersect(r)) == std::numeric_limits<float>::infinity());
		}
	}

	TEST_CASE("Plane3D.isInFront")
	{
		SECTION("returns true when point is on the side the normal points to")
		{
			{
				Plane3D p(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));
				Vector3D v(3.0f, 4.0f, 5.0f);
				REQUIRE(p.isInFront(v));
			}
			{
				Plane3D p(Vector3D(3.0f, 4.0f, 5.0f), Vector3D(0.0f, 1.0f, 0.0f));
				Vector3D v(-1.0f, 5.0f, 17.0f);
				REQUIRE(p.isInFront(v));
			}
		}

		SECTION("returns false when point is on the other side")
		{
			{
				Plane3D p(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));
				Vector3D v(3.0f, -4.0f, 5.0f);
				REQUIRE(!p.isInFront(v));
			}

			{
				Plane3D p(Vector3D(3.0f, 4.0f, 5.0f), Vector3D(0.0f, 1.0f, 0.0f));
				Vector3D v(2.0f, 3.0f, 11.0f);
				REQUIRE(!p.isInFront(v));
			}
		}

		SECTION("returns false when the point lies on the plane")
		{
			Plane3D p(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));
			Vector3D v(3.0f, 0.0f, 5.0f);
			REQUIRE(!p.isInFront(v));
		}
	}

	TEST_CASE("Plane3D::fromPoints")
	{
		SECTION("creates a plane from 3 points in anti-clockwise winding")
		{
			Vector3D a(0.0f, 0.0f, 0.0f);
			Vector3D b(1.0f, 0.0f, 0.0f);
			Vector3D c(1.0f, 1.0f, 0.0f);

			Plane3D p = Plane3D::fromPoints(a, b, c);

			// not strictly required by implementation
			// (could use b or c as the point)
			// but no better way to verify at the moment
			REQUIRE(p.point.x == Approx(0.0f));
			REQUIRE(p.point.y == Approx(0.0f));
			REQUIRE(p.point.z == Approx(0.0f));

			REQUIRE(p.normal.x == Approx(0.0f));
			REQUIRE(p.normal.y == Approx(0.0f));
			REQUIRE(p.normal.z == Approx(1.0f));
		}
	}

	TEST_CASE("BoundingBox.intersect")
	{
		SECTION("finds the point at which the ray enters the box")
		{
			BoundingBox3D box(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(1.0f, 1.0f, 1.0f));
			Ray3D ray(Vector3D(-5.0f, 0.0f, 0.0f), Vector3D(1.0f, 0.0f, 0.0f));
			auto intersect = box.intersect(ray);
			REQUIRE(intersect.hit);
			REQUIRE(intersect.enter == Approx(4.0f));
			REQUIRE(intersect.exit == Approx(6.0f));
		}

		SECTION("returns a miss when the ray misses")
		{
			BoundingBox3D box(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(1.0f, 1.0f, 1.0f));
			Ray3D ray(Vector3D(-5.0f, 0.0f, 0.0f), Vector3D(1.0f, 1.0f, 0.0f));
			auto intersect = box.intersect(ray);
			REQUIRE(!intersect.hit);
		}
	}
}
