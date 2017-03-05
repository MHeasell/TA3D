#ifndef __TA3D_TA3D_STRING_H__
#define __TA3D_TA3D_STRING_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include <ostream>
#include <vector>
#include <list>
#include <algorithm>
#include <sstream>

#include <utf8.h>
#include <fmt/format.h>

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
		typedef std::string::iterator iterator;
		typedef std::string::const_iterator const_iterator;

		static const size_type npos = std::string::npos;

		String() {}

		String(const nullptr_t) {}

		String(char ch) : str(1, ch) {}

		String(const char* chars, size_type count) : str(chars, count) {}

		String(const char* chars) : str(chars) {}

		String(std::string s) : str(s) {}

		String(const String& other, size_type pos, size_type count = npos): str(other.str, pos, count) {}

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
			append(s.str);
		}

		void append(const std::string& s)
		{
			str.append(s);
		}

		void append(const char* chars)
		{
			str.append(chars);
		}

		template <typename T>
		void append(const T& t)
		{
			std::ostringstream out;
			out << t;
			append(out.str());
		}

		void append(const char* s, size_type count)
		{
			str.append(s, count);
		}

		template <typename... Args>
		void appendFormat(const char* fmt, const Args& ... args)
		{
			append(format(fmt, args...));
		}

		bool empty() const
		{
			return str.empty();
		}

		bool notEmpty() const
		{
			return !str.empty();
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

		size_type find(const char* s) const
		{
			return str.find(s);
		}

		size_type find(const String& substr) const
		{
			return str.find(substr.str);
		}

		int32_t to_sint32() const
		{
			return std::stoi(str);
		}

		int to_int() const
		{
			return std::stoi(str);
		}

		uint32_t to_uint32() const
		{
			return (uint32_t)std::stoul(str);
		}

		unsigned int to_uint() const
		{
			return (unsigned int)std::stoul(str);
		}

		float to_float() const
		{
			return std::stof(str);
		}

		bool to_float(float& f) const
		{
			try
			{
				f = std::stof(str);
			}
			catch (const std::invalid_argument&)
			{
				return false;
			}
			catch (const std::out_of_range&)
			{
				return false;
			}

			return true;
		}

		bool to_bool() const
		{
			switch (size())
			{
				case 0:
					return false;
				case 1:
					{
						char c = first();
						return c == '1' || c == 'Y' || c == 'y' || c == 'O' || c == 'o' || c == 't' || c == 'T';
					}
				default:
					{
						if (size() > 4)
						{
							return false;
						}
						String buf(*this, 0, std::min(size(), (size_type)4));
						buf.toLower();
						return buf == "true" || buf == "on" || buf == "yes";
					}
			}

			return false;
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
			return str.find(substr.str) != std::string::npos;
		}

		bool contains(char ch) const
		{
			return str.find(ch) != std::string::npos;
		}

		void reserve(size_type count)
		{
			str.reserve(count);
		}

		bool endsWith(const String& substr) const
		{
			size_type our_size = size();
			size_type sub_size = substr.size();

			if (sub_size > our_size)
			{
				return false;
			}

			size_type start_index = our_size - sub_size;
			return str.compare(start_index, sub_size, substr.str) == 0;
		}

		bool endsWith(char ch) const
		{
			return empty() ? false : last() == ch;
		}

		bool startsWith(const String& substr) const
		{
			size_type our_size = size();
			size_type sub_size = substr.size();

			if (sub_size > our_size)
			{
				return false;
			}

			return str.compare(0, sub_size, substr.str) == 0;
		}

		bool startsWith(char ch) const
		{
			return empty() ? false : first() == ch;
		}

		// Removes the last n octets from the string.
		// Note that this is unsafe on UTF-8 strings
		// as it is possible to chop a portion of a multi-byte code point.
		void chop(size_type n)
		{
			auto index_from_end = str.size() - n;
			str.erase(index_from_end, n);
		}

		// Truncates the string to at most len code points
		void truncate(size_type len)
		{
			if (len >= size())
			{
				return;
			}

			str.erase(len);
		}

		iterator begin()
		{
			return str.begin();
		}

		const_iterator begin() const
		{
			return str.begin();
		}

		iterator end()
		{
			return str.end();
		}

		const_iterator end() const
		{
			return str.end();
		}

		// Returns the last octet.
		// If the string is empty, returns NULL.
		char last() const
		{
			return str.empty() ? '\0' : str.back();
		}

		// Returns the first octet.
		// If the string is empty, returns NULL.
		char first() const
		{
			return str.empty() ? '\0' : str.front();
		}

		size_type find_first_of(const String& chars, size_type pos) const
		{
			return str.find_first_of(chars.str, pos);
		}

		size_type find_first_of(const String& chars) const
		{
			return str.find_first_of(chars.str);
		}

		size_type find_first_not_of(const String& chars, size_type pos) const
		{
			return str.find_first_not_of(chars.str, pos);
		}

		size_type find_first_not_of(const String& chars) const
		{
			return str.find_first_not_of(chars.str);
		}

		size_type find_last_of(const String& chars) const
		{
			return str.find_last_of(chars.str);
		}

		size_type find_last_of(const char ch) const
		{
			return str.find_last_of(ch);
		}

		template <typename... Args>
		String format(const char* fmt, const Args& ... args)
		{
			return String(fmt::sprintf(fmt, args...));
		}

		void erase(size_type index, size_type count)
		{
			str.erase(index, count);
		}

		void assign(const char* s, size_type count)
		{
			str.assign(s, count);
		}

		void assign(const char* s)
		{
			str.assign(s);
		}

		void assign(const String& s, size_type count)
		{
			str.assign(s.str, 0, count);
		}

		// Removes the last octet of the string.
		// Note that this is not safe for UTF-8 strings.
		void removeLast()
		{
			str.pop_back();
		}

		// This is not safe on UTF-8 strings.
		//
		// The following behaviours are quirks from Yuni:
		//  - All elements are trimmed
		//  - Empty strings are not kept
		template<class T>
		void explode(T& out, const String& separators, bool emptyBefore = true, bool keepEmptyElements = false, bool trimElements = true) const
		{
			if (emptyBefore)
			{
				out.clear();
			}

			if (empty())
			{
				return;
			}

			size_type start_pos = 0;
			while (start_pos < str.size())
			{
				size_type separator_pos = str.find_first_of(separators.str, start_pos);
				if (separator_pos == std::string::npos)
				{
					String tmp = String(*this, start_pos);
					if (trimElements)
					{
						tmp.trim();
					}
					if (keepEmptyElements || tmp.notEmpty())
					{
						out.push_back(tmp);
					}
					return;
				}

				String tmp = String(str, start_pos, separator_pos - start_pos);
				if (trimElements)
				{
					tmp.trim();
				}
				if (keepEmptyElements || tmp.notEmpty())
				{
					out.push_back(tmp);
				}
				start_pos = separator_pos + 1;
			}
		}

		template<class T>
		void explode(T& out, char separator, bool emptyBefore = true, bool keepEmptyElements = false, bool trimElements = true) const
		{
			String s;
			s.append(separator);
			explode(out, s, emptyBefore, keepEmptyElements, trimElements);
		}

		// From yuni, a different interface for explode
		template<class T>
		void split(T& out, const String& separators, bool keepEmptyElements = false, bool trimElements = true, bool emptyBefore = true) const
		{
			explode(out, separators, emptyBefore, keepEmptyElements, trimElements);
		}

		void trimLeft();

		void trimLeft(char ch);

		void trimRight();

		// trims whitespace characters from the start and end of the string
		void trim();

		// Replaces all occurrences of original with replacement
		void replace(const String& original, const String& replacement)
		{
			std::string::size_type offset = 0;
			std::string::size_type pos;
			while (offset < size())
			{
				pos = str.find(original.str, offset);
				if (pos == std::string::npos)
				{
					return;
				}
				str.replace(pos, original.size(), replacement.str);
				offset = pos + replacement.size();
			}
		}

		void replace(char original, char replacement)
		{
			replace(String(original), String(replacement));
		}

		void convertSlashesIntoBackslashes()
		{
			replace('/', '\\');
		}

		void convertBackslashesIntoSlashes()
		{
			replace('\\', '/');
		}

		void removeTrailingSlash()
		{
			if (empty())
			{
				return;
			}

			if (last() == '/' || last() == '\\')
			{
				chop(1);
			}
		}

		bool glob(const String& pattern) const
		{
			if (pattern.empty())
			{
				// if the pattern is empty,
				// we only match if we're also empty
				return empty();
			}

			size_type pattern_index = 0;

			// place to jump back to for backtracking
			size_type prev = npos;

			for (size_type i = 0; i < size(); ++i)
			{
				if (pattern[pattern_index] == '*')
				{
					// if the wildcard is the last character,
					// we're guaranteed to match
					if (pattern_index + 1 == pattern.size())
					{
						return true;
					}

					// consume any additional wildcards
					while (pattern[pattern_index + 1] == '*')
					{
						++pattern_index;
					}

					// if the wildcard is the last character,
					// we're guaranteed to match
					if (pattern_index + 1 == pattern.size())
					{
						return true;
					}

					// remember the position for backtracking
					prev = pattern_index;

					// check the first non-wildcard char
					if (pattern[pattern_index + 1] == str[i])
					{
						pattern_index += 2;
					}
				}
				else
				{
					if (pattern_index < pattern.size() && pattern[pattern_index] == str[i])
					{
						// we matched, advance to next char in pattern
						++pattern_index;
					}
					else
					{
						// we didn't match, try to backtrack
						if (prev == npos)
						{
							// no place to backtrack to, abort.
							return false;
						}

						pattern_index = prev;
					}
				}
			}

			// We consumed the whole string.
			// We matched if we also consumed the whole pattern.
			return pattern_index == pattern.size();
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

		bool utf8valid() const
		{
			return utf8::is_valid(begin(), end());
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

		bool operator<(const String& rhs) const
		{
			return str < rhs.str;
		}

		bool operator<=(const String& rhs) const
		{
			return str <= rhs.str;
		}

		bool operator>(const String& rhs) const
		{
			return str > rhs.str;
		}

		bool operator>=(const String& rhs) const
		{
			return str >= rhs.str;
		}

		bool operator==(const char* rhs) const
		{
			return str == rhs;
		}

		bool operator!=(const char* rhs) const
		{
			return str != rhs;
		}

		bool operator<(const char* rhs) const
		{
			return str < rhs;
		}

		bool operator<=(const char* rhs) const
		{
			return str <= rhs;
		}

		bool operator>(const char* rhs) const
		{
			return str > rhs;
		}

		bool operator>=(const char* rhs) const
		{
			return str >= rhs;
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

	std::ostream& operator<<(std::ostream& os, const String& str);

	String::const_utf8iterator operator+(const String::const_utf8iterator& it, String::size_type count);

	String operator+(char lhs, const String& rhs);

	String operator+(const String& lhs, const char* rhs);

	bool operator==(const char* lhs, const String& rhs);
}

#endif
