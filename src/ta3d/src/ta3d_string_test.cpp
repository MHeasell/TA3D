#include <catch.hpp>
#include "ta3d_string.h"

namespace TA3D {

	TEST_CASE("String.last")
	{
		SECTION("gets the last character")
		{
			REQUIRE(String("foo").last() == 'o');
			REQUIRE(String("asdf").last() == 'f');
		}

		SECTION("returns null if the string is empty")
		{
			REQUIRE(String("").last() == '\0');
			REQUIRE(String().last() == '\0');
		}
	}

	TEST_CASE("String.to_sint32()")
	{
		SECTION("converts strings to signed integers")
		{
			REQUIRE(String("2").to_sint32() == 2);
			REQUIRE(String("13").to_sint32() == 13);
			REQUIRE(String("-8").to_sint32() == -8);
		}

		SECTION("handles base 16 numbers")
		{
			REQUIRE(String("0xFF").to_sint32() == 255);
			REQUIRE(String("#FF").to_sint32() == 255);

			// we expect to wrap around
			REQUIRE(String("0xFFFFFFFF").to_sint32() == -1);
		}
	}

	TEST_CASE("String.to_int()")
	{
		SECTION("converts strings to signed integers")
		{
			REQUIRE(String("2").to_int() == 2);
			REQUIRE(String("13").to_int() == 13);
			REQUIRE(String("-8").to_int() == -8);
		}

		SECTION("handles base 16 numbers")
		{
			REQUIRE(String("0xFF").to_int() == 255);
			REQUIRE(String("#FF").to_int() == 255);

			// we expect to wrap around
			REQUIRE(String("0xFFFFFFFF").to_int() == -1);
		}
	}

	TEST_CASE("String.to_uint32()")
	{
		SECTION("converts strings to unsigned integers")
		{
			REQUIRE(String("2").to_uint32() == 2);
			REQUIRE(String("13").to_uint32() == 13);
			REQUIRE(String("-8").to_uint32() == 4294967288);
		}

		SECTION("handles base 16 numbers")
		{
			REQUIRE(String("0xFF").to_uint32() == 255);
			REQUIRE(String("#FF").to_uint32() == 255);

			// we expect to not wrap around
			REQUIRE(String("0xFFFFFFFF").to_uint32() == 4294967295);
		}
	}

	TEST_CASE("String.to_uint()")
	{
		SECTION("converts strings to unsigned integers")
		{
			REQUIRE(String("2").to_uint() == 2);
			REQUIRE(String("13").to_uint() == 13);
			REQUIRE(String("-8").to_uint() == 4294967288);
		}

		SECTION("handles base 16 numbers")
		{
			REQUIRE(String("0xFF").to_uint() == 255);
			REQUIRE(String("#FF").to_uint() == 255);

			// we expect to not wrap around
			REQUIRE(String("0xFFFFFFFF").to_uint() == 4294967295);
		}
	}

	TEST_CASE("String.detectBase()")
	{
		SECTION("assumes base 10 in invalid cases")
		{
			REQUIRE(String("").detectBase() == String::BaseDetectionResult(0, 10));
			REQUIRE(String("fred").detectBase() == String::BaseDetectionResult(0, 10));
			REQUIRE(String(" 1").detectBase() == String::BaseDetectionResult(0, 10));
		}

		SECTION("emits base 10 for ordinary numbers")
		{
			REQUIRE(String("2").detectBase() == String::BaseDetectionResult(0, 10));
			REQUIRE(String("0").detectBase() == String::BaseDetectionResult(0, 10));
			REQUIRE(String("17").detectBase() == String::BaseDetectionResult(0, 10));
			REQUIRE(String("-4").detectBase() == String::BaseDetectionResult(0, 10));
			REQUIRE(String("22").detectBase() == String::BaseDetectionResult(0, 10));
		}

		SECTION("emits base 16 for 0x prefix")
		{
			REQUIRE(String("0x40").detectBase() == String::BaseDetectionResult(2, 16));
			REQUIRE(String("0X2").detectBase() == String::BaseDetectionResult(2, 16));
			REQUIRE(String("0XFFFF").detectBase() == String::BaseDetectionResult(2, 16));
			REQUIRE(String("0xdeadbeef").detectBase() == String::BaseDetectionResult(2, 16));
		}

		SECTION("emits base 16 for # prefix")
		{
			REQUIRE(String("#ffffff").detectBase() == String::BaseDetectionResult(1, 16));
			REQUIRE(String("#02ab5f").detectBase() == String::BaseDetectionResult(1, 16));
		}

		// Some systems consider numbers like "0755" to be octal. We don't.
		SECTION("emits base 10 for 0 prefixed numbers")
		{
			REQUIRE(String("0755").detectBase() == String::BaseDetectionResult(0, 10));
		}
	}

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
