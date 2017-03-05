#include "ta3d_string.h"

#include <algorithm>
#include <functional>

namespace TA3D
{
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
		auto first_trailing_whitespace = std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<char, bool>(is_space))).base();
		str.erase(first_trailing_whitespace, str.end());
	}

	// trims whitespace characters from the start and end of the string
	void String::trim()
	{
		trimLeft();
		trimRight();
	}

	std::ostream& operator<<(std::ostream& os, const String& str)
	{
		return os << str.c_str();
	}

	String::const_utf8iterator operator+(const String::const_utf8iterator& it, String::size_type count)
	{
		auto new_it = String::const_utf8iterator(it);
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
