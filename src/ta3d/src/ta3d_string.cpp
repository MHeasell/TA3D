#include "string.h"

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
		auto first_trailing_whitespace = std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<char, bool>(is_space))).base();
		str.erase(first_trailing_whitespace, str.end());
	}

	// trims whitespace characters from the start and end of the string
	void String::trim()
	{
		trimLeft();
		trimRight();
	}
}
