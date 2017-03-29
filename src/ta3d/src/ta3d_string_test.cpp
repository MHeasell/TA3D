#include <catch.hpp>
#include "ta3d_string.h"

namespace TA3D {

	TEST_CASE("String.explode()")
	{
		SECTION("emits nothing when the string is empty")
		{
			String s("");
			String::Vector out;
			s.explode(out, ",");
			REQUIRE(out.size() == 0);
		}

		SECTION("splits a string")
		{
			String s("foo, bar, baz");
			String::Vector out;
			s.explode(out, ",");
			REQUIRE(out.size() == 3);
			REQUIRE(out[0] == "foo");
			REQUIRE(out[1] == "bar");
			REQUIRE(out[2] == "baz");
		}

		SECTION("empties the list first by default")
		{
			String s("foo, bar, baz");
			String::Vector out = { String("asdf"), String("qwer") };
			s.explode(out, ",");
			REQUIRE(out.size() == 3);
			REQUIRE(out[0] == "foo");
			REQUIRE(out[1] == "bar");
			REQUIRE(out[2] == "baz");
		}

		SECTION("doesn't empty the list first if specified")
		{
			String s("foo, bar, baz");
			String::Vector out = {"a", "b"};
			s.explode(out, ",", false);
			REQUIRE(out.size() == 5);
			REQUIRE(out[0] == "a");
			REQUIRE(out[1] == "b");
			REQUIRE(out[2] == "foo");
			REQUIRE(out[3] == "bar");
			REQUIRE(out[4] == "baz");
		}

		SECTION("skips empty elements by default")
		{
			String s("foo,,,bar");
			String::Vector out;
			s.explode(out, ",");
			REQUIRE(out.size() == 2);
			REQUIRE(out[0] == "foo");
			REQUIRE(out[1] == "bar");
		}

		SECTION("keeps empty elements when set")
		{
			String s("foo,,,bar");
			String::Vector out;
			s.explode(out, ",", true, true);
			REQUIRE(out.size() == 4);
			REQUIRE(out[0] == "foo");
			REQUIRE(out[1] == "");
			REQUIRE(out[2] == "");
			REQUIRE(out[3] == "bar");
		}

		SECTION("trims whitespace by default")
		{
			String s("    foo, bar    ,baz");
			String::Vector out;
			s.explode(out, ",");
			REQUIRE(out.size() == 3);
			REQUIRE(out[0] == "foo");
			REQUIRE(out[1] == "bar");
			REQUIRE(out[2] == "baz");
		}

		SECTION("doesn't trim whitespace if set to false")
		{
			String s("    foo, bar    ,baz");
			String::Vector out;
			s.explode(out, ",", true, false, false);
			REQUIRE(out.size() == 3);
			REQUIRE(out[0] == "    foo");
			REQUIRE(out[1] == " bar    ");
			REQUIRE(out[2] == "baz");
		}

		SECTION("doesn't trim whitespace if specified")
		{
			String s("foo, bar, baz");
			String::Vector out;
			s.explode(out, ",", true, true, false);
			REQUIRE(out.size() == 3);
			REQUIRE(out[0] == "foo");
			REQUIRE(out[1] == " bar");
			REQUIRE(out[2] == " baz");
		}

		SECTION("works with multiple separators")
		{
			String s("foo,bar|baz:blarg;f");
			String::Vector out;
			s.explode(out, "|;");

			REQUIRE(out.size() == 3);
			REQUIRE(out[0] == "foo,bar");
			REQUIRE(out[1] == "baz:blarg");
			REQUIRE(out[2] == "f");
		}
	}
}
