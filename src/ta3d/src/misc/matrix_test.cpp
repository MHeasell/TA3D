#include <catch.hpp>
#include "matrix.h"

namespace TA3D
{
	TEST_CASE("Matrix operator*")
	{
		SECTION("multiplies matrices")
		{
			// 2x scale matrix
			Matrix a;
			a.E[0][0] = 2.0f;
			a.E[1][0] = 0.0f;
			a.E[2][0] = 0.0f;
			a.E[3][0] = 0.0f;

			a.E[0][1] = 0.0f;
			a.E[1][1] = 2.0f;
			a.E[2][1] = 0.0f;
			a.E[3][1] = 0.0f;

			a.E[0][2] = 0.0f;
			a.E[1][2] = 0.0f;
			a.E[2][2] = 2.0f;
			a.E[3][2] = 0.0f;

			a.E[0][3] = 0.0f;
			a.E[1][3] = 0.0f;
			a.E[2][3] = 0.0f;
			a.E[3][3] = 1.0f;

			// translation matrix
			Matrix b;
			b.E[0][0] = 1.0f;
			b.E[1][0] = 0.0f;
			b.E[2][0] = 0.0f;
			b.E[3][0] = 4.0f;

			b.E[0][1] = 0.0f;
			b.E[1][1] = 1.0f;
			b.E[2][1] = 0.0f;
			b.E[3][1] = 5.0f;

			b.E[0][2] = 0.0f;
			b.E[1][2] = 0.0f;
			b.E[2][2] = 1.0f;
			b.E[3][2] = 6.0f;

			b.E[0][3] = 0.0f;
			b.E[1][3] = 0.0f;
			b.E[2][3] = 0.0f;
			b.E[3][3] = 1.0f;

			// Result should be translate then scale.
			// That is, the translation would be applied first,
			// then the result scaled.
			// This effectively scales the translation distance.
			Matrix c = a * b;

			REQUIRE(c.E[0][0] == 2.0f);
			REQUIRE(c.E[1][0] == 0.0f);
			REQUIRE(c.E[2][0] == 0.0f);
			REQUIRE(c.E[3][0] == 8.0f);

			REQUIRE(c.E[0][1] == 0.0f);
			REQUIRE(c.E[1][1] == 2.0f);
			REQUIRE(c.E[2][1] == 0.0f);
			REQUIRE(c.E[3][1] == 10.0f);

			REQUIRE(c.E[0][2] == 0.0f);
			REQUIRE(c.E[1][2] == 0.0f);
			REQUIRE(c.E[2][2] == 2.0f);
			REQUIRE(c.E[3][2] == 12.0f);

			REQUIRE(c.E[0][3] == 0.0f);
			REQUIRE(c.E[1][3] == 0.0f);
			REQUIRE(c.E[2][3] == 0.0f);
			REQUIRE(c.E[3][3] == 1.0f);
		}
	}

	TEST_CASE("Matrix.OrthographicProjection")
	{
		SECTION("combines with inverse to produce the identity")
		{
			Matrix a = OrthographicProjection(-40.0f, 60.0f, -30.0f, 10.0f, 2.0f, 200.0f);
			Matrix b = InverseOrthographicProjection(-40.0f, 60.0f, -30.0f, 10.0f, 2.0f, 200.0f);
			Matrix c = b * a;

			REQUIRE(c.E[0][0] == Approx(1.0f));
			REQUIRE(c.E[1][0] == Approx(0.0f));
			REQUIRE(c.E[2][0] == Approx(0.0f));
			REQUIRE(c.E[3][0] == Approx(0.0f));

			REQUIRE(c.E[0][1] == Approx(0.0f));
			REQUIRE(c.E[1][1] == Approx(1.0f));
			REQUIRE(c.E[2][1] == Approx(0.0f));
			REQUIRE(c.E[3][1] == Approx(0.0f));

			REQUIRE(c.E[0][2] == Approx(0.0f));
			REQUIRE(c.E[1][2] == Approx(0.0f));
			REQUIRE(c.E[2][2] == Approx(1.0f));
			REQUIRE(c.E[3][2] == Approx(0.0f));

			REQUIRE(c.E[0][3] == Approx(0.0f));
			REQUIRE(c.E[1][3] == Approx(0.0f));
			REQUIRE(c.E[2][3] == Approx(0.0f));
			REQUIRE(c.E[3][3] == Approx(1.0f));
		}
	}

	TEST_CASE("Matrix.CabinetProjection")
	{
		SECTION("Produces the identity when combined with opposite")
		{
			Matrix a = CabinetProjection(0.0f, 0.5f);
			Matrix b = CabinetProjection(0.0f, -0.5f);
			Matrix c = b * a;

			REQUIRE(c.E[0][0] == Approx(1.0f));
			REQUIRE(c.E[1][0] == Approx(0.0f));
			REQUIRE(c.E[2][0] == Approx(0.0f));
			REQUIRE(c.E[3][0] == Approx(0.0f));

			REQUIRE(c.E[0][1] == Approx(0.0f));
			REQUIRE(c.E[1][1] == Approx(1.0f));
			REQUIRE(c.E[2][1] == Approx(0.0f));
			REQUIRE(c.E[3][1] == Approx(0.0f));

			REQUIRE(c.E[0][2] == Approx(0.0f));
			REQUIRE(c.E[1][2] == Approx(0.0f));
			REQUIRE(c.E[2][2] == Approx(1.0f));
			REQUIRE(c.E[3][2] == Approx(0.0f));

			REQUIRE(c.E[0][3] == Approx(0.0f));
			REQUIRE(c.E[1][3] == Approx(0.0f));
			REQUIRE(c.E[2][3] == Approx(0.0f));
			REQUIRE(c.E[3][3] == Approx(1.0f));
		}
	}

	TEST_CASE("Matrix::Translate")
	{
		SECTION("Produces a translation matrix")
		{
			auto v = Translate(Vector3D(3.0f, 4.0f, 5.0f)) * Vector3D(11.0f, 15.0f, 19.0f);
			REQUIRE(v.x == Approx(14.0f));
			REQUIRE(v.y == Approx(19.0f));
			REQUIRE(v.z == Approx(24.0f));
		}
	}
}
