#include "ta3d_string.h"

#include <algorithm>
#include <functional>

namespace TA3D
{
	String::String(const NullPtr&) {}

	String::String() {}

	String::String(char ch) : str(1, ch) {}

	String::String(const char* chars, String::size_type count) : str(chars, count) {}

	String::String(const char* chars) : str(chars) {}

	String::String(std::string s) : str(s) {}

	String::String(const String& other, String::size_type pos, String::size_type count) : str(other.str, pos, count) {}

	String& String::toUpper()
	{
		for (std::string::size_type i = 0; i < str.size(); ++i)
		{
			str[i] = to_upper(str[i]);
		}
		return *this;
	}

	String& String::toLower()
	{
		for (std::string::size_type i = 0; i < str.size(); ++i)
		{
			str[i] = to_lower(str[i]);
		}
		return *this;
	}

	void String::trimLeft()
	{
		auto first_non_whitespace = std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<char, bool>(is_space)));
		str.erase(str.begin(), first_non_whitespace);
	}

	void String::trimLeft(char ch)
	{
		auto first_non_whitespace = std::find(str.begin(), str.end(), ch);
		str.erase(str.begin(), first_non_whitespace);
	}

	void String::trimRight()
	{
		auto first_trailing_whitespace = std::find_if(
			str.rbegin(),
			str.rend(),
			std::not1(std::ptr_fun<char, bool>(is_space)))
											 .base();
		str.erase(first_trailing_whitespace, str.end());
	}

	// trims whitespace characters from the start and end of the string
	void String::trim()
	{
		trimLeft();
		trimRight();
	}

	const char* String::c_str() const
	{
		return str.c_str();
	}

	const char* String::data() const
	{
		return str.data();
	}

	void String::append(const String& s)
	{
		append(s.str);
	}

	void String::append(const std::string& s)
	{
		str.append(s);
	}

	void String::append(const char* chars)
	{
		str.append(chars);
	}

	void String::append(const char* s, String::size_type count)
	{
		str.append(s, count);
	}

	bool String::empty() const
	{
		return str.empty();
	}

	bool String::notEmpty() const
	{
		return !str.empty();
	}

	String::size_type String::find(char ch) const
	{
		return str.find(ch);
	}

	String::size_type String::find(const char* s) const
	{
		return str.find(s);
	}

	String::size_type String::find(const String& substr) const
	{
		return str.find(substr.str);
	}

	int32_t String::to_sint32() const
	{
		// callers expect that too high values will wrap around
		return (int32_t)to_uint32();
	}

	int String::to_int() const
	{
		// callers expect that too high values will wrap around
		return (int)to_uint();
	}

	uint32_t String::to_uint32() const
	{
		auto base = detectBase();
		return (uint32_t)std::strtoul(str.c_str() + base.offset, nullptr, base.base);
	}

	unsigned int String::to_uint() const
	{
		auto base = detectBase();
		return (unsigned int)std::strtoul(str.c_str() + base.offset, nullptr, base.base);
	}

	float String::to_float() const
	{
		return std::stof(str);
	}

	bool String::to_float(float& f) const
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

	bool String::to_bool() const
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
	}

	String::size_type String::size() const
	{
		return str.size();
	}

	String::size_type String::length() const
	{
		return str.length();
	}

	bool String::contains(const String& substr) const
	{
		return str.find(substr.str) != std::string::npos;
	}

	bool String::contains(char ch) const
	{
		return str.find(ch) != std::string::npos;
	}

	void String::reserve(String::size_type count)
	{
		str.reserve(count);
	}

	bool String::endsWith(const String& substr) const
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

	bool String::endsWith(char ch) const
	{
		return empty() ? false : last() == ch;
	}

	bool String::startsWith(const String& substr) const
	{
		size_type our_size = size();
		size_type sub_size = substr.size();

		if (sub_size > our_size)
		{
			return false;
		}

		return str.compare(0, sub_size, substr.str) == 0;
	}

	bool String::startsWith(char ch) const
	{
		return empty() ? false : first() == ch;
	}

	void String::chop(String::size_type n)
	{
		auto index_from_end = str.size() - n;
		str.erase(index_from_end, n);
	}

	void String::truncate(String::size_type len)
	{
		if (len >= size())
		{
			return;
		}

		str.erase(len);
	}

	String::iterator String::begin()
	{
		return str.begin();
	}

	String::const_iterator String::begin() const
	{
		return str.begin();
	}

	String::iterator String::end()
	{
		return str.end();
	}

	String::const_iterator String::end() const
	{
		return str.end();
	}

	char String::last() const
	{
		return str.empty() ? char(0) : str.back();
	}

	char String::first() const
	{
		return str.empty() ? char(0) : str.front();
	}

	String::size_type String::find_first_of(const String& chars, String::size_type pos) const
	{
		return str.find_first_of(chars.str, pos);
	}

	String::size_type String::find_first_of(const String& chars) const
	{
		return str.find_first_of(chars.str);
	}

	String::size_type String::find_first_not_of(const String& chars, String::size_type pos) const
	{
		return str.find_first_not_of(chars.str, pos);
	}

	String::size_type String::find_first_not_of(const String& chars) const
	{
		return str.find_first_not_of(chars.str);
	}

	String::size_type String::find_last_of(const String& chars) const
	{
		return str.find_last_of(chars.str);
	}

	String::size_type String::find_last_of(const char ch) const
	{
		return str.find_last_of(ch);
	}

	void String::erase(String::size_type index, String::size_type count)
	{
		str.erase(index, count);
	}

	void String::assign(const char* s, String::size_type count)
	{
		str.assign(s, count);
	}

	void String::assign(const char* s)
	{
		str.assign(s);
	}

	void String::assign(const String& s, String::size_type count)
	{
		str.assign(s.str, 0, count);
	}

	void String::removeLast()
	{
		str.pop_back();
	}

	void String::replace(const String& original, const String& replacement)
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

	void String::replace(char original, char replacement)
	{
		replace(String(original), String(replacement));
	}

	void String::convertSlashesIntoBackslashes()
	{
		replace('/', '\\');
	}

	void String::convertBackslashesIntoSlashes()
	{
		replace('\\', '/');
	}

	void String::removeTrailingSlash()
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

	bool String::glob(const String& pattern) const
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

	String::size_type String::utf8size() const
	{
		return utf8::distance(str.begin(), str.end());
	}

	String::const_utf8iterator String::utf8begin() const
	{
		return const_utf8iterator(str.begin(), str.begin(), str.end());
	}

	String::const_utf8iterator String::utf8end() const
	{
		return const_utf8iterator(str.begin(), str.end(), str.end());
	}

	bool String::utf8valid() const
	{
		return utf8::is_valid(begin(), end());
	}

	bool String::operator==(const String& rhs) const
	{
		return str == rhs.str;
	}

	bool String::operator!=(const String& rhs) const
	{
		return str != rhs.str;
	}

	bool String::operator<(const String& rhs) const
	{
		return str < rhs.str;
	}

	bool String::operator<=(const String& rhs) const
	{
		return str <= rhs.str;
	}

	bool String::operator>(const String& rhs) const
	{
		return str > rhs.str;
	}

	bool String::operator>=(const String& rhs) const
	{
		return str >= rhs.str;
	}

	bool String::operator==(const char* rhs) const
	{
		return str == rhs;
	}

	bool String::operator!=(const char* rhs) const
	{
		return str != rhs;
	}

	bool String::operator<(const char* rhs) const
	{
		return str < rhs;
	}

	bool String::operator<=(const char* rhs) const
	{
		return str <= rhs;
	}

	bool String::operator>(const char* rhs) const
	{
		return str > rhs;
	}

	bool String::operator>=(const char* rhs) const
	{
		return str >= rhs;
	}

	String& String::operator<<(String::NullPtr const&)
	{
		// don't do anything
		return *this;
	}

	String& String::operator+=(String::NullPtr const&)
	{
		// don't do anything
		return *this;
	}

	char& String::operator[](String::size_type pos)
	{
		return str[pos];
	}

	const char& String::operator[](String::size_type pos) const
	{
		return str[pos];
	}

	char String::to_upper(char c)
	{
		int delta = c - 'a';
		if (delta >= 0 && delta < 26)
		{
			return c & char(0x5f);
		}

		return c;
	}

	char String::to_lower(char c)
	{
		int delta = c - 'A';
		if (delta >= 0 && delta < 26)
		{
			return c | char(32);
		}

		return c;
	}

	bool String::is_space(char c)
	{
		switch (c)
		{
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				return true;
			default:
				return false;
		}
	}

	String& String::clear()
	{
		str.clear();
		return *this;
	}

	String::BaseDetectionResult String::detectBase() const
	{
		if (size() < 1)
		{
			return BaseDetectionResult(0, 10);
		}

		char first = (*this)[0];
		switch (first)
		{
			case '#':
				return BaseDetectionResult(1, 16);
			case '0':
			{
				if (size() >= 2)
				{
					// check for the full '0x' prefix for base 16
					char second = (*this)[1];
					if (second == 'x' || second == 'X')
					{
						return BaseDetectionResult(2, 16);
					}
				}

				return BaseDetectionResult(0, 10);
			}
			default:
				return BaseDetectionResult(0, 10);
		}
	}

	std::ostream& operator<<(std::ostream& os, const String& str)
	{
		return os << str.c_str();
	}

	String::const_utf8iterator operator+(const String::const_utf8iterator& it, String::size_type count)
	{
		String::const_utf8iterator new_it(it);
		for (String::size_type i = 0; i < count; ++i)
		{
			++new_it;
		}

		return new_it;
	}

	String operator+(char lhs, const String& rhs)
	{
		String n(lhs);
		n.append(rhs);
		return n;
	}

	String operator+(const String& lhs, const char* rhs)
	{
		String n(lhs);
		n.append(rhs);
		return n;
	}

	bool operator==(const char* lhs, const String& rhs)
	{
		return rhs == lhs;
	}
}
