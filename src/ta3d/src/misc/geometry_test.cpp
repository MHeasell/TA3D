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

	TEST_CASE("Ray3D.isLessFar")
	{
		SECTION("returns true when point a is the least far along")
		{
			Ray3D r(Vector3D(3.0f, 10.0f, 4.0f), Vector3D(0.0f, -2.0f, 0.0f));
			REQUIRE(r.isLessFar(r.pointAt(1.0f), r.pointAt(2.0f)));
			REQUIRE(!r.isLessFar(r.pointAt(3.0f), r.pointAt(2.0f)));
			REQUIRE(r.isLessFar(r.pointAt(-5.0f), r.pointAt(-4.0f)));
			REQUIRE(!r.isLessFar(r.pointAt(-5.0f), r.pointAt(-6.0f)));
		}
	}

	TEST_CASE("Plane3D.intersect")
	{
		SECTION("Gets the time of intersection with a ray")
		{
			Ray3D r(Vector3D(3.0f, 10.0f, 4.0f), Vector3D(0.0f, -2.0f, 0.0f));
			Plane3D p(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));
			auto intersect = p.intersect(r);
			REQUIRE(intersect);
			REQUIRE(*intersect == 5.0f);
		}

		SECTION("Returns a miss when the ray is parallel in front of the plane")
		{
			Ray3D r(Vector3D(1.0f, 1.0f, 1.0f), Vector3D(1.0f, 0.0f, 0.0f));
			Plane3D p(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));
			auto intersect = p.intersect(r);
			REQUIRE(!intersect);
		}

		SECTION("Returns a miss when the ray is parallel behind the plane")
		{
			Ray3D r(Vector3D(1.0f, -1.0f, 1.0f), Vector3D(1.0f, 0.0f, 0.0f));
			Plane3D p(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));
			auto intersect = p.intersect(r);
			REQUIRE(!intersect);
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
			REQUIRE(intersect);
			REQUIRE(intersect->enter == Approx(4.0f));
			REQUIRE(intersect->exit == Approx(6.0f));
		}

		SECTION("hits at the corner of the box")
		{
			BoundingBox3D box(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(1.0f, 1.0f, 1.0f));
			Ray3D ray(Vector3D(-2.0f, -2.0f, 0.0f), Vector3D(1.0f, 1.0f, 1.0f));
			auto intersect = box.intersect(ray);
			REQUIRE(intersect);
			REQUIRE(intersect->enter == Approx(1.0f));
			REQUIRE(intersect->exit == Approx(1.0f));
		}

		SECTION("misses just above the corner of the box")
		{
			BoundingBox3D box(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(1.0f, 1.0f, 1.0f));
			Ray3D ray(Vector3D(-2.0f, -2.0f, 0.00001f), Vector3D(1.0f, 1.0f, 1.0f));
			auto intersect = box.intersect(ray);
			REQUIRE(!intersect);
		}

		SECTION("returns a miss when the ray misses")
		{
			BoundingBox3D box(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(1.0f, 1.0f, 1.0f));
			Ray3D ray(Vector3D(-5.0f, 0.0f, 0.0f), Vector3D(1.0f, 1.0f, 0.0f));
			auto intersect = box.intersect(ray);
			REQUIRE(!intersect);
		}
	}

	TEST_CASE("Triangle3D.toBarycentric")
	{
		SECTION("barycentric conversion test 1")
		{
			Vector3D p(1.0f, 0.0f, 0.0f);
			Triangle3D tri(
				Vector3D(0.0f, 0.0f, 0.0f),
				Vector3D(2.0f, 0.0f, 0.0f),
				Vector3D(0.0f, 2.0f, 0.0f));
			Vector3D bary = tri.toBarycentric(p);

			REQUIRE(bary.x == Approx(0.5f));
			REQUIRE(bary.y == Approx(0.5f));
			REQUIRE(bary.z == Approx(0.0f));
		}

		SECTION("barycentric conversion test 2")
		{
			Vector3D p(0.5f, 0.5f, 0.0f);
			Triangle3D tri(
				Vector3D(0.0f, 0.0f, 0.0f),
				Vector3D(2.0f, 0.0f, 0.0f),
				Vector3D(0.0f, 2.0f, 0.0f));
			Vector3D bary = tri.toBarycentric(p);

			REQUIRE(bary.x == Approx(0.5f));
			REQUIRE(bary.y == Approx(0.25f));
			REQUIRE(bary.z == Approx(0.25f));
		}

		SECTION("barycentric conversion test 3")
		{
			Vector3D p(-5.0f, 0.5f, 5.0f);
			Triangle3D tri(
				Vector3D(-0.5f, 0.5f, -0.5f),
				Vector3D(0.5f, 0.5f, -0.5f),
				Vector3D(0.5f, 0.5f, 0.5f));
			Vector3D bary = tri.toBarycentric(p);

			REQUIRE(bary.x == Approx(5.5f));
			REQUIRE(bary.y == Approx(-10.0f));
			REQUIRE(bary.z == Approx(5.5f));
		}
	}

	TEST_CASE("Triangle3D.toCartesian")
	{
		SECTION("test 1")
		{
			Vector3D p(0.5f, 0.5f, 0.0f);
			Triangle3D tri(
				Vector3D(0.0f, 0.0f, 0.0f),
				Vector3D(2.0f, 0.0f, 0.0f),
				Vector3D(0.0f, 2.0f, 0.0f));
			Vector3D cart = tri.toCartesian(p);

			REQUIRE(cart.x == Approx(1.0f));
			REQUIRE(cart.y == Approx(0.0f));
			REQUIRE(cart.z == Approx(0.0f));
		}

		SECTION("test 2")
		{
			Vector3D p(0.5f, 0.25f, 0.25f);
			Triangle3D tri(
				Vector3D(0.0f, 0.0f, 0.0f),
				Vector3D(2.0f, 0.0f, 0.0f),
				Vector3D(0.0f, 2.0f, 0.0f));
			Vector3D cart = tri.toCartesian(p);

			REQUIRE(cart.x == Approx(0.5f));
			REQUIRE(cart.y == Approx(0.5f));
			REQUIRE(cart.z == Approx(0.0f));
		}

		SECTION("test 3")
		{
			Vector3D p(5.5f, -10.0f, 5.5f);
			Triangle3D tri(
				Vector3D(-0.5f, 0.5f, -0.5f),
				Vector3D(0.5f, 0.5f, -0.5f),
				Vector3D(0.5f, 0.5f, 0.5f));
			Vector3D cart = tri.toCartesian(p);

			REQUIRE(cart.x == Approx(-5.0f));
			REQUIRE(cart.y == Approx(0.5f));
			REQUIRE(cart.z == Approx(5.0f));
		}
	}

	TEST_CASE("Triangle3D.intersect")
	{
		SECTION("returns the distance when the ray hits")
		{
			Triangle3D tri(
				Vector3D(-1.0f, -1.0f, 0.0f),
				Vector3D(1.0f, -1.0f, 0.0f),
				Vector3D(0.0f, 1.0f, 0.0f));
			Ray3D r(
				Vector3D(0.0f, 0.0f, 10.0f),
				Vector3D(0.0f, 0.0f, -1.0f));
			auto intersect = tri.intersect(r);
			REQUIRE(intersect);
			REQUIRE(*intersect == Approx(10.0f));
		}

		SECTION("hits at the corner of the triangle")
		{
			Triangle3D tri(
				Vector3D(-1.0f, -1.0f, 0.0f),
				Vector3D(1.0f, -1.0f, 0.0f),
				Vector3D(0.0f, 1.0f, 0.0f));
			Ray3D r(
				Vector3D(-1.0f, -1.0f, 10.0f),
				Vector3D(0.0f, 0.0f, -1.0f));
			auto intersect = tri.intersect(r);
			REQUIRE(intersect);
			REQUIRE(*intersect == Approx(10.0f));
		}

		SECTION("misses just below the corner of the triangle")
		{
			Triangle3D tri(
				Vector3D(-1.0f, -1.0f, 0.0f),
				Vector3D(1.0f, -1.0f, 0.0f),
				Vector3D(0.0f, 1.0f, 0.0f));
			Ray3D r(
				Vector3D(-1.0f, -1.000001f, 10.0f),
				Vector3D(0.0f, 0.0f, -1.0f));
			auto intersect = tri.intersect(r);
			REQUIRE(!intersect);
		}

		SECTION("returns a miss when the ray misses")
		{
			SECTION("case 1")
			{
				Triangle3D tri(
					Vector3D(-1.0f, -1.0f, 0.0f),
					Vector3D(1.0f, -1.0f, 0.0f),
					Vector3D(0.0f, 1.0f, 0.0f));
				Ray3D r(
					Vector3D(2.0f, 2.0f, 10.0f),
					Vector3D(0.0f, 0.0f, -1.0f));
				REQUIRE(!tri.intersect(r));
			}

			SECTION("case 2")
			{
				Triangle3D tri(
					Vector3D(1.0f, 1.0f, 0.0f),
					Vector3D(3.0f, 1.0f, 0.0f),
					Vector3D(0.0f, 3.0f, 0.0f));
				Ray3D r(
					Vector3D(0.0f, 0.0f, 10.0f),
					Vector3D(0.0f, 0.0f, -1.0f));
				REQUIRE(!tri.intersect(r));
			}

			SECTION("when ray is parallel above the triangle")
			{
				Triangle3D tri(
					Vector3D(1.0f, 1.0f, 0.0f),
					Vector3D(3.0f, 1.0f, 0.0f),
					Vector3D(0.0f, 3.0f, 0.0f));
				Ray3D r(
					Vector3D(0.0f, 0.0f, 10.0f),
					Vector3D(0.0f, 1.0f, 0.0f));
				REQUIRE(!tri.intersect(r));
			}

			SECTION("when ray is parallel below the triangle")
			{
				Triangle3D tri(
					Vector3D(1.0f, 1.0f, 0.0f),
					Vector3D(3.0f, 1.0f, 0.0f),
					Vector3D(0.0f, 3.0f, 0.0f));
				Ray3D r(
					Vector3D(0.0f, 0.0f, -10.0f),
					Vector3D(0.0f, 1.0f, 0.0f));
				REQUIRE(!tri.intersect(r));
			}
		}
	}

	TEST_CASE("Triangle3D.intersectLine")
	{
		SECTION("returns the point when the line hits")
		{
			Triangle3D tri(
				Vector3D(-1.0f, -1.0f, 0.0f),
				Vector3D(1.0f, -1.0f, 0.0f),
				Vector3D(0.0f, 1.0f, 0.0f));
			auto intersect = tri.intersectLine(Vector3D(0.0f, 0.0f, 10.0f), Vector3D(0.0f, 0.0f, -10.0f));
			REQUIRE(intersect);
			REQUIRE(intersect->x == Approx(0.0f));
			REQUIRE(intersect->y == Approx(0.0f));
			REQUIRE(intersect->z == Approx(0.0f));
		}

		SECTION("works for a line in the other direction")
		{
			Triangle3D tri(
				Vector3D(-1.0f, -1.0f, 0.0f),
				Vector3D(1.0f, -1.0f, 0.0f),
				Vector3D(0.0f, 1.0f, 0.0f));
			auto intersect = tri.intersectLine(Vector3D(0.0f, 0.0f, -10.0f), Vector3D(0.0f, 0.0f, 10.0f));
			REQUIRE(intersect);
			REQUIRE(intersect->x == Approx(0.0f));
			REQUIRE(intersect->y == Approx(0.0f));
			REQUIRE(intersect->z == Approx(0.0f));
		}

		SECTION("hits at the corner of the triangle")
		{
			Triangle3D tri(
				Vector3D(-1.0f, -1.0f, 0.0f),
				Vector3D(1.0f, -1.0f, 0.0f),
				Vector3D(0.0f, 1.0f, 0.0f));
			auto intersect = tri.intersectLine(Vector3D(-1.0f, -1.0f, 10.0f), Vector3D(-1.0f, -1.0f, -10.0f));
			REQUIRE(intersect);
			REQUIRE(intersect->x == Approx(-1.0f));
			REQUIRE(intersect->y == Approx(-1.0f));
			REQUIRE(intersect->z == Approx(0.0f));
		}

		SECTION("misses just below the corner of the triangle")
		{
			Triangle3D tri(
				Vector3D(-1.0f, -1.0f, 0.0f),
				Vector3D(1.0f, -1.0f, 0.0f),
				Vector3D(0.0f, 1.0f, 0.0f));
			auto intersect = tri.intersectLine(Vector3D(-1.0f, -1.000001f, 10.0f), Vector3D(-1.0f, -1.000001f, -10.0f));
			REQUIRE(!intersect);
		}
	}

	TEST_CASE("Triangle3D.toPlane")
	{
		SECTION("returns the plane the triangle lies on")
		{
			Triangle3D tri(
				Vector3D(-1, -1, 0),
				Vector3D(1, -1, 0),
				Vector3D(0, 1, 0));
			Plane3D p = tri.toPlane();

			REQUIRE(p.point.x == Approx(-1.0f));
			REQUIRE(p.point.y == Approx(-1.0f));
			REQUIRE(p.point.z == Approx(0.0f));

			REQUIRE(p.normal.x == Approx(0.0f));
			REQUIRE(p.normal.y == Approx(0.0f));
			REQUIRE(p.normal.z == Approx(4.0f));
		}
	}

	TEST_CASE("closestTo")
	{
		SECTION("returns a when b is not defined")
		{
			Vector3D origin(0.0f, 0.0f, 0.0f);
			boost::optional<Vector3D> a = Vector3D(1.0f, 2.0f, 3.0f);
			boost::optional<Vector3D> b;

			auto result = closestTo(origin, a, b);
			REQUIRE(result);
			REQUIRE(result->x == 1.0f);
			REQUIRE(result->y == 2.0f);
			REQUIRE(result->z == 3.0f);
		}

		SECTION("returns b when a is not defined")
		{
			Vector3D origin(0.0f, 0.0f, 0.0f);
			boost::optional<Vector3D> a;
			boost::optional<Vector3D> b = Vector3D(2.0f, 3.0f, 4.0f);

			auto result = closestTo(origin, a, b);
			REQUIRE(result);
			REQUIRE(result->x == 2.0f);
			REQUIRE(result->y == 3.0f);
			REQUIRE(result->z == 4.0f);
		}

		SECTION("when both are defined, returns the closer")
		{
			Vector3D origin(0.0f, 0.0f, 0.0f);
			boost::optional<Vector3D> a = Vector3D(1.0f, 2.0f, 3.0f);
			boost::optional<Vector3D> b = Vector3D(2.0f, 3.0f, 4.0f);

			auto result = closestTo(origin, a, b);
			REQUIRE(result);
			REQUIRE(result->x == 1.0f);
			REQUIRE(result->y == 2.0f);
			REQUIRE(result->z == 3.0f);
		}

		SECTION("when both are defined, returns the closer (test 2)")
		{
			Vector3D origin(3.0f, 3.0f, 3.0f);
			boost::optional<Vector3D> a = Vector3D(1.0f, 2.0f, 3.0f);
			boost::optional<Vector3D> b = Vector3D(2.0f, 3.0f, 4.0f);

			auto result = closestTo(origin, a, b);
			REQUIRE(result);
			REQUIRE(result->x == 2.0f);
			REQUIRE(result->y == 3.0f);
			REQUIRE(result->z == 4.0f);
		}
	}
}
