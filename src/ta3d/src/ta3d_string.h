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
	/**
	 * A numeric type used to hold a Unicode code point.
	 */
	typedef uint32_t CodePoint;

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

		struct BaseDetectionResult
		{
			size_type offset;
			int base;
			BaseDetectionResult(size_type offset, int base): offset(offset), base(base) {}
			bool operator==(const BaseDetectionResult& rhs) const
			{
				return base == rhs.base && offset == rhs.offset;
			}
		};

		/**
		 * Constructs an empty string.
		 */
		String();

		/**
		 * Constructs an empty string.
		 */
		String(const NullPtr&);

		/**
		 * Constructs a string containing only ch.
		 */
		String(char ch);

		/**
		 * Constructs a string from the first count octets of chars.
		 */
		String(const char* chars, size_type count);

		/**
		 * Constructs a string from the null-terminated string chars.
		 */
		String(const char* chars);

		/**
		 * Constructs a string from the std::string s.
		 */
		String(std::string s);

		/**
		 * Constructs a string from the first count octets of other, starting from pos.
		 * @param other The string to copy.
		 * @param pos The index at which to start copying.
		 * @param count The number of octets to copy.
		 */
		String(const String& other, size_type pos, size_type count = npos);

		/**
		 * Constructs a string from the given iterators.
		 * Iterates from first until reaching last.
		 */
		template <class InputIt>
		String(InputIt first, InputIt last) : str(first, last) {}

		/**
		 * Returns the c-string representation of the string.
		 */
		const char* c_str() const;

		/**
		 * Returns the underlying string data.
		 */
		const char* data() const;

		/**
		 * Appends s to the end of the string.
		 */
		void append(const String& s);

		/**
		 * Appends s to the end of the string.
		 */
		void append(const std::string& s);

		/**
		 * Appends the null-terminated string chars to the end of the string.
		 */
		void append(const char* chars);

		/**
		 * Appends the arbitrary item t to the end of the string.
		 * t is first converted to a string by insertion into a stringstream,
		 * and that value is then appended to the string.
		 */
		template <typename T>
		void append(const T& t)
		{
			std::ostringstream out;
			out << t;
			append(out.str());
		}

		/**
		 * Appends the first count octets of s to the string.
		 */
		void append(const char* s, size_type count);

		/**
		 * Appends the string fmt to the string
		 * after substituting format parameters.
		 * fmt is expected to be a printf-style format string.
		 */
		template <typename... Args>
		void appendFormat(const char* fmt, const Args&... args)
		{
			append(format(fmt, args...));
		}

		/**
		 * Returns true if the string is empty, otherwise false.
		 */
		bool empty() const;

		/**
		 * Returns true if the string is not empty, otherwise false.
		 */
		bool notEmpty() const;

		/**
		 * Clears all content from the string, making it empty.
		 * @return A reference to the current string, for chaining.
		 */
		String& clear();

		/**
		 * Finds the first occurrence of octet ch within the string.
		 * @param ch The octet to search for.
		 * @return The index of the first occurrence of octet ch, or npos if not found.
		 */
		size_type find(char ch) const;

		/**
		 * Finds the first occurrence of octet sequence s within the string.
		 * @param s The sequence to search for.
		 * @return The index of the start of the octet sequence in the string, or npos if not found.
		 */
		size_type find(const char* s) const;

		/**
		 * Finds the first occurrence of octet sequence substr within the string.
		 * @param substr The sequence to search for.
		 * @return The index of the start of the octet sequence in the string, or npos if not found.
		 */
		size_type find(const String& substr) const;

		/**
		 * Returns the string as a signed 32-bit integer.
		 * If the string cannot be converted, behaviour is not specified.
		 */
		int32_t to_sint32() const;

		/**
		 * Returns the string as a signed integer.
		 * If the string cannot be converted, behaviour is not specified.
		 */
		int to_int() const;

		/**
		 * Returns the string as an unsigned 32-bit integer.
		 * If the string cannot be converted, behaviour is not specified.
		 */
		uint32_t to_uint32() const;

		/**
		 * Returns the string as an unsigned integer.
		 * If the string cannot be converted, behaviour is not specified.
		 */
		unsigned int to_uint() const;

		/**
		 * Returns the string as a float.
		 * If the string cannot be converted, behaviour is not specified.
		 */
		float to_float() const;

		/**
		 * Converts the string to a float.
		 * @param[out] f The float where output is written to.
		 * @return true if the conversion succeeded, otherwise false.
		 */
		bool to_float(float& f) const;

		/**
		 * Returns the string as a boolean.
		 *
		 * The following values are considered true:
		 *
		 *  - true
		 *  - yes
		 *  - on
		 *
		 *  These are case-insensitive.
		 *  Just the first letter is also accepted.
		 */
		bool to_bool() const;

		/**
		 * Counts the number of octets in the string.
		 * If the string is UTF8, this may be larger
		 * than the number of code points,
		 * because a code point can be encoded as multiple octets.
		 */
		size_type size() const;

		/**
		 * Same as size().
		 */
		size_type length() const;

		/**
		 * Returns true if the string contains substr, otherwise false.
		 */
		bool contains(const String& substr) const;

		/**
		 * Returns true if the string contains ch, otherwise false.
		 */
		bool contains(char ch) const;

		/**
		 * Reserves the given amount of capacity in the string.
		 * Just wraps the std::string version.
		 */
		void reserve(size_type count);

		/**
		 * Returns true if the string ends with substr, otherwise false.
		 */
		bool endsWith(const String& substr) const;

		/**
		 * Returns true if the string ends with ch, otherwise false.
		 */
		bool endsWith(char ch) const;

		/**
		 * Returns true if the string starts with substr, otherwise false.
		 */
		bool startsWith(const String& substr) const;

		/**
		 * Returns true if the string starts with ch, otherwise false.
		 */
		bool startsWith(char ch) const;

		/**
		 * Removes the last n octets from the string.
		 * Note that this is unsafe on UTF-8 strings
		 * as it is possible to chop a portion of a multi-byte code point.
		 */
		void chop(size_type n);

		/**
		 * Truncates the string to at most len octets.
		 * Note that this is unsafe on UTF-8 strings
		 * as it is possible to chop a portion of a multi-byte code point.
		 */
		void truncate(size_type len);

		/**
		 * Returns an iterator to the beginning of the string.
		 */
		iterator begin();

		/**
		 * Returns an iterator to the beginning of the string.
		 */
		const_iterator begin() const;

		/**
		 * Returns an iterator to the end of the string.
		 */
		iterator end();

		/**
		 * Returns an iterator to the end of the string.
		 */
		const_iterator end() const;

		/**
		 * Returns the last octet, or NULL if the string is empty.
		 */
		char last() const;

		/**
		 * Returns the first octet, or NULL if the string is empty.
		 */
		char first() const;

		/**
		 * Finds the first occurrence of an octet in chars in the string.
		 * @param chars The set of octets to search for.
		 * @param pos The position in the string to start searching from.
		 * @return The index of the octet, or npos if not found.
		 */
		size_type find_first_of(const String& chars, size_type pos) const;

		/**
		 * Finds the first occurrence of an octet in chars in the string.
		 * @param chars The set of octets to search for.
		 * @return The index of the octet, or npos if not found.
		 */
		size_type find_first_of(const String& chars) const;

		/**
		 * Finds the first occurrence of an octet NOT in chars in the string.
		 * @param chars The set of octets to exclude.
		 * @param pos The position in the string to start searching from.
		 * @return The index of the octet, or npos if not found.
		 */
		size_type find_first_not_of(const String& chars, size_type pos) const;

		/**
		 * Finds the first occurrence of an octet NOT in chars in the string.
		 * @param chars The set of octets to exclude.
		 * @return The index of the octet, or npos if not found.
		 */
		size_type find_first_not_of(const String& chars) const;

		/**
		 * Finds the last occurrence of an octet in chars in the string.
		 * @param chars The set of octets to search for.
		 * @return The index of the octet, or npos if not found.
		 */
		size_type find_last_of(const String& chars) const;

		/**
		 * Finds the last occurrence of ch in the string.
		 * @param ch The octet to search for.
		 * @return The index of the octet, or npos if not found.
		 */
		size_type find_last_of(const char ch) const;

		/**
		 * Returns the string formed after substituting the given args
		 * into the format string fmt.
		 * fmt is expected to be a printf-style format string.
		 *
		 * Note: this method does not operate on the string instance at all
		 * and is effectively a static method, however it remains this way
		 * for compatibility with existing callers.
		 */
		template <typename... Args>
		String format(const char* fmt, const Args&... args)
		{
			return String(fmt::sprintf(fmt, args...));
		}

		/**
		 * Erases count octets from the string starting at the given index.
		 * The erased portion of the string is removed
		 * (not filled in with blanks), so the string gets shorter.
		 * @param index The index to start erasing from.
		 * @param count The number of octets to erase.
		 */
		void erase(size_type index, size_type count);

		/**
		 * Replaces the contents of this string with the first count octets of s.
		 * @param s The octet sequence to replace this string with.
		 * @param count The number of octets to read from s.
		 */
		void assign(const char* s, size_type count);

		/**
		 * Replaces the contents of this string with the null-terminated string s.
		 */
		void assign(const char* s);

		/**
		 * Replaces the contents of this string with the first count octets of s.
		 * @param s The octet sequence to replace this string with.
		 * @param count The number of octets to read from s.
		 */
		void assign(const String& s, size_type count);

		/**
		 * Removes the last octet of the string.
		 * Note that this is not safe for UTF-8 strings.
		 */
		void removeLast();

		/**
		 * Splits the string by occurrences of the given separators.
		 * The split strings are appended to out via push_back().
		 * For example, "foo:bar:baz" => {"foo", "bar", "baz"}
		 * @tparam T The container type.
		 * @param out The container to append strings to.
		 * @param separators The set of octets to separate by.
		 * @param emptyBefore Set to true to empty the container before filling.
		 * @param keepEmptyElements Set to true to retain empty strings between separators.
		 * @param trimElements Set to true to trim whitespace on split elements.
		 */
		template <class T>
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

		/**
		 * Splits the string by occurrences of the given separator.
		 * The split strings are appended to out via push_back().
		 * For example, "foo:bar:baz" => {"foo", "bar", "baz"}
		 * @tparam T The container type.
		 * @param out The container to append strings to.
		 * @param separator The octet to separate by.
		 * @param emptyBefore Set to true to empty the container before filling.
		 * @param keepEmptyElements Set to true to retain empty strings between separators.
		 * @param trimElements Set to true to trim whitespace on split elements.
		 */
		template <class T>
		void explode(T& out, char separator, bool emptyBefore = true, bool keepEmptyElements = false, bool trimElements = true) const
		{
			String s;
			s.append(separator);
			explode(out, s, emptyBefore, keepEmptyElements, trimElements);
		}

		/**
		 * Same as explode() but with different parameter order,
		 * see explode() for details.
		 */
		template <class T>
		void split(T& out, const String& separators, bool keepEmptyElements = false, bool trimElements = true, bool emptyBefore = true) const
		{
			explode(out, separators, emptyBefore, keepEmptyElements, trimElements);
		}

		/**
		 * Trims whitespace from the left side of the string.
		 * ASCII spaces, tabs, and newlines (CR and LF) are considered whitespace.
		 */
		void trimLeft();

		/**
		 * Trims leading runs of ch from the left side of the string.
		 */
		void trimLeft(char ch);

		/**
		 * Trims whitespace from the right side of the string.
		 * ASCII spaces, tabs, and newlines (CR and LF) are considered whitespace.
		 */
		void trimRight();

		/**
		 * Trims whitespace characters from both sides of the string.
		 * ASCII spaces, tabs, and newlines (CR and LF) are considered whitespace.
		 */
		void trim();

		/**
		 * Replaces all occurrences of original with replacement.
		 */
		void replace(const String& original, const String& replacement);

		/**
		 * Replaces all occurrences of original with replacement.
		 * This is not safe on UTF-8 strings.
		 */
		void replace(char original, char replacement);

		/**
		 * Replaces all forward-slashes with backslashes.
		 * This is not safe on UTF-8 strings.
		 */
		void convertSlashesIntoBackslashes();

		/**
		 * Replaces all backslashes with forward-slashes.
		 * This is not safe on UTF-8 strings.
		 */
		void convertBackslashesIntoSlashes();

		/**
		 * If the string ends with a backslash or forward-slash, removes it.
		 */
		void removeTrailingSlash();

		/**
		 * Returns true if the string matches the given glob pattern.
		 * The glob pattern is a string containing at most one wildcard (*).
		 * e.g. "file-*.sav"
		 * If the string contains multiple wildcards, the behaviour is unspecified.
		 */
		bool glob(const String& pattern) const;

		/**
		 * Returns the number of UTF-8 code points in the string.
		 * This operation takes O(n) time.
		 */
		size_type utf8size() const;

		/**
		 * Returns a UTF-8 code point iterator to the beginning of the string.
		 * The iterator treats the string as UTF-8 encoded text
		 * and yields one value per code point.
		 */
		const_utf8iterator utf8begin() const;

		/**
		 * Returns a UTF-8 code point iterator to the end of the string.
		 * The iterator treats the string as UTF-8 encoded text
		 * and yields one value per code point.
		 */
		const_utf8iterator utf8end() const;

		/**
		 * Returns true if the string is valid UTF-8.
		 * This operation takes O(n) time.
		 */
		bool utf8valid() const;

		/**
		 * Converts the string to uppercase in-place.
		 * This function only considers the standard latin alphabet (a-z),
		 * where "uppercase" is well defined.
		 * Other code points are left as-is.
		 * This is not safe on UTF-8 strings.
		 * @return A reference to the string, for chaining.
		 */
		String& toUpper();

		/**
		 * Converts the string to lowercase in-place.
		 * This function only considers the standard latin alphabet (a-z),
		 * where "lowercase" is well defined.
		 * Other code points are left as-is.
		 * This is not safe on UTF-8 strings.
		 * @return A reference to the string, for chaining.
		 */
		String& toLower();

		/**
		 * Attempts to detect the base of the numeric value
		 * contained in the string.
		 *
		 * If the base could be detected, returns the base
		 * and the offset where the numeric value starts.
		 * If the base is not detected, base is assumed 10 and offset is 0.
		 */
		BaseDetectionResult detectBase() const;

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

		/**
		 * Equivalent to append() with one argument.
		 * @return A reference to the string, for chaining.
		 */
		template <typename T>
		String& operator<<(const T& t)
		{
			append(t);
			return *this;
		}

		/**
		 * Does nothing.
		 * @return A reference to the string, for chaining.
		 */
		String& operator<<(const NullPtr&);

		/**
		 * Equivalent to append() with one argument.
		 * @return A reference to the string, for chaining.
		 */
		template <typename T>
		String& operator+=(const T& t)
		{
			append(t);
			return *this;
		}

		/**
		 * Does nothing.
		 * @return A reference to the string, for chaining.
		 */
		String& operator+=(const NullPtr&);

		/**
		 * Returns a reference to the octet at the given position.
		 */
		char& operator[](size_type pos);

		/**
		 * Returns a reference to the octet at the given position.
		 */
		const char& operator[](size_type pos) const;

	private:
		/**
		 * If c is an ASCII character in the range a-z,
		 * returns the uppercase version.
		 * Otherwise, returns the octet as-is.
		 */
		static char to_upper(char c);

		/**
		 * If c is an ASCII character in the range A-Z,
		 * returns the lowercase version.
		 * Otherwise, returns the octet as-is.
		 */
		static char to_lower(char c);

		/**
		 * Returns true if c is a whitespace character.
		 * Space, tab, CR and LF are considered whitespace.
		 */
		static bool is_space(char c);

		/**
		 * The underlying string.
		 */
		std::string str;
	};

	std::ostream& operator<<(std::ostream& os, const String& str);

	String::const_utf8iterator operator+(const String::const_utf8iterator& it, String::size_type count);

	String operator+(char lhs, const String& rhs);

	String operator+(const String& lhs, const char* rhs);

	bool operator==(const char* lhs, const String& rhs);
}

#endif
