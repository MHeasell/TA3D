#ifndef __TA3D_STRING_H__
#define __TA3D_STRING_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include <ostream>
#include <vector>
#include <list>

#include <utf8.h>

namespace TA3D
{
	class String
	{
	public:
		typedef utf8::iterator<std::string::const_iterator> const_utf8iterator;
		typedef std::string::size_type size_type;
		typedef size_type Size;
		typedef std::vector<String> Vector;
		typedef std::list<String> List;
		typedef std::nullptr_t NullPtr;

		static const size_type npos = std::string::npos;

		String() {}

		String(const char* chars, size_type count) : str(chars, count) {}

		String(const char* chars) : str(chars) {}

		template <class InputIt>
		String(InputIt first, InputIt last) : str(first, last) {}

		const char* c_str() const
		{
			return str.c_str();
		}

		const char* data() const
		{
			return str.data();
		}

		void append(const String& s)
		{
			str.append(s.c_str());
		}

		void append(const char* chars)
		{
			str.append(chars);
		}

		template <typename T>
		void append(T t)
		{
			str.append(std::to_string(t));
		}

		bool empty() const
		{
			return str.empty();
		}

		String& clear()
		{
			str.clear();
			return *this;
		}

		size_type find(char ch) const
		{
			return str.find(ch);
		}

		size_type find(String& substr) const
		{
			return str.find(substr.c_str());
		}

		int32_t to_sint32() const
		{
			return std::stoi(str);
		}

		// Counts the number of octets in the string.
		// If the string is UTF8, this may be larger
		// than the number of code points,
		// because a code point can be encoded as multiple octets.
		size_type size() const
		{
			return str.size();
		}

		// same as size()
		size_type length() const
		{
			return str.length();
		}
		
		bool contains(const String& substr) const
		{
			return str.find(substr.c_str()) != std::string::npos;
		}

		void trimLeft();

		void trimRight();

		// trims whitespace characters from the start and end of the string
		void trim();

		// Replaces all occurrences of original with replacement
		void replace(const String& original, const String& replacement)
		{
			std::string::size_type offset = 0;
			std::string::size_type pos;
			while (offset < size());
			{
				pos = str.find(original.c_str(), offset, original.length());
				if (pos == std::string::npos)
				{
					return;
				}
				str.replace(pos, original.length(), replacement.c_str(), replacement.length());
				offset = pos + replacement.length();
			}
		}

		size_type utf8size() const
		{
			return utf8::distance(str.begin(), str.end());
		}

		const_utf8iterator utf8begin() const
		{
			return const_utf8iterator(str.begin(), str.begin(), str.end());
		}

		const_utf8iterator utf8end() const
		{
			return const_utf8iterator(str.begin(), str.end(), str.end());
		}

		// Converts the string to uppercase in-place.
		// This function only considers the standard latin alphabet,
		// where "uppercase" is well defined.
		// Other code points are left as-is.
		String& toUpper();

		// Converts the string to lowerjjcase in-place.
		// This function only considers the standard latin alphabet,
		// where "lowercase" is well defined.
		// Other code points are left as-is.
		String& toLower();

		bool operator==(const String& rhs) const
		{
			return str == rhs.str;
		}

		bool operator!=(const String& rhs) const
		{
			return str != rhs.str;
		}

		template <typename T>
		String& operator<<(const T& t)
		{
			append(t);
			return *this;
		}

		String& operator<<(const NullPtr&)
		{
			// don't do anything
			return *this;
		}

		template <typename T>
		String& operator+=(const T& t)
		{
			append(t);
			return *this;
		}

		String& operator+=(const NullPtr&)
		{
			// don't do anything
			return *this;
		}

		char& operator[](size_type pos)
		{
			return str[pos];
		}

		const char& operator[](size_type pos) const
		{
			return str[pos];
		}

	private:
		// If the octet is a UTF-8 character in the range a-z,
		// returns the uppercase version.
		// Otherwise, returns the octet as-is.
		static char to_upper(char c)
		{
			int delta = c - 'a';
			if (delta >= 0 && delta < 26)
			{
				return c & 0x5f;
			}

			return c;
		}

		// If the octet is a UTF-8 character in the range a-z,
		// returns the lowercase version.
		// Otherwise, returns the octet as-is.
		static char to_lower(char c)
		{
			int delta = c - 'A';
			if (delta >= 0 && delta < 26)
			{
				return c | 32;
			}

			return c;
		}

		static bool is_space(char c)
		{
			switch (c) {
				case ' ':
				case '\t':
				case '\r':
				case '\n':
					return true;
				default:
					return false;
			}
		}

		std::string str;
	};

	std::ostream& operator<<(std::ostream& os, const String& str)
	{
		return os << str.c_str();
	}

	String::const_utf8iterator operator+(const String::const_utf8iterator& it, unsigned int count)
	{
		auto new_it = String::const_utf8iterator(it);
		for (unsigned int i = 0; i < count; ++i)
		{
			++new_it;
		}

		return new_it;
	}
}

#endif
