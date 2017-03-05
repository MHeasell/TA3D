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

		String();

		String(const nullptr_t);

		String(char ch);

		String(const char* chars, size_type count);

		String(const char* chars);

		String(std::string s);

		String(const String& other, size_type pos, size_type count = npos);

		template <class InputIt>
		String(InputIt first, InputIt last) : str(first, last) {}

		const char* c_str() const;

		const char* data() const;

		void append(const String& s);

		void append(const std::string& s);

		void append(const char* chars);

		template <typename T>
		void append(const T& t)
		{
			std::ostringstream out;
			out << t;
			append(out.str());
		}

		void append(const char* s, size_type count);

		template <typename... Args>
		void appendFormat(const char* fmt, const Args& ... args)
		{
			append(format(fmt, args...));
		}

		bool empty() const;

		bool notEmpty() const;

		String& clear()
		{
			str.clear();
			return *this;
		}

		size_type find(char ch) const;

		size_type find(const char* s) const;

		size_type find(const String& substr) const;

		int32_t to_sint32() const;

		int to_int() const;

		uint32_t to_uint32() const;

		unsigned int to_uint() const;

		float to_float() const;

		bool to_float(float& f) const;

		bool to_bool() const;

		// Counts the number of octets in the string.
		// If the string is UTF8, this may be larger
		// than the number of code points,
		// because a code point can be encoded as multiple octets.
		size_type size() const;

		// same as size()
		size_type length() const;
		
		bool contains(const String& substr) const;

		bool contains(char ch) const;

		void reserve(size_type count);

		bool endsWith(const String& substr) const;

		bool endsWith(char ch) const;

		bool startsWith(const String& substr) const;

		bool startsWith(char ch) const;

		// Removes the last n octets from the string.
		// Note that this is unsafe on UTF-8 strings
		// as it is possible to chop a portion of a multi-byte code point.
		void chop(size_type n);

		// Truncates the string to at most len code points
		void truncate(size_type len);

		iterator begin();

		const_iterator begin() const;

		iterator end();

		const_iterator end() const;

		// Returns the last octet.
		// If the string is empty, returns NULL.
		char last() const;

		// Returns the first octet.
		// If the string is empty, returns NULL.
		char first() const;

		size_type find_first_of(const String& chars, size_type pos) const;

		size_type find_first_of(const String& chars) const;

		size_type find_first_not_of(const String& chars, size_type pos) const;

		size_type find_first_not_of(const String& chars) const;

		size_type find_last_of(const String& chars) const;

		size_type find_last_of(const char ch) const;

		template <typename... Args>
		String format(const char* fmt, const Args& ... args)
		{
			return String(fmt::sprintf(fmt, args...));
		}

		void erase(size_type index, size_type count);

		void assign(const char* s, size_type count);

		void assign(const char* s);

		void assign(const String& s, size_type count);

		// Removes the last octet of the string.
		// Note that this is not safe for UTF-8 strings.
		void removeLast();

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
		void replace(const String& original, const String& replacement);

		void replace(char original, char replacement);

		void convertSlashesIntoBackslashes();

		void convertBackslashesIntoSlashes();

		void removeTrailingSlash();

		bool glob(const String& pattern) const;

		size_type utf8size() const;

		const_utf8iterator utf8begin() const;

		const_utf8iterator utf8end() const;

		bool utf8valid() const;

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

		bool operator==(const String& rhs) const;

		bool operator!=(const String& rhs) const;

		bool operator<(const String& rhs) const;

		bool operator<=(const String& rhs) const;

		bool operator>(const String& rhs) const;

		bool operator>=(const String& rhs) const;

		bool operator==(const char* rhs) const;

		bool operator!=(const char* rhs) const;

		bool operator<(const char* rhs) const;

		bool operator<=(const char* rhs) const;

		bool operator>(const char* rhs) const;

		bool operator>=(const char* rhs) const;

		template <typename T>
		String& operator<<(const T& t)
		{
			append(t);
			return *this;
		}

		String& operator<<(const NullPtr&);

		template <typename T>
		String& operator+=(const T& t)
		{
			append(t);
			return *this;
		}

		String& operator+=(const NullPtr&);

		char& operator[](size_type pos);

		const char& operator[](size_type pos) const;

	private:
		// If the octet is a UTF-8 character in the range a-z,
		// returns the uppercase version.
		// Otherwise, returns the octet as-is.
		static char to_upper(char c);

		// If the octet is a UTF-8 character in the range a-z,
		// returns the lowercase version.
		// Otherwise, returns the octet as-is.
		static char to_lower(char c);

		static bool is_space(char c);

		std::string str;
	};

	std::ostream& operator<<(std::ostream& os, const String& str);

	String::const_utf8iterator operator+(const String::const_utf8iterator& it, String::size_type count);

	String operator+(char lhs, const String& rhs);

	String operator+(const String& lhs, const char* rhs);

	bool operator==(const char* lhs, const String& rhs);
}

#endif
