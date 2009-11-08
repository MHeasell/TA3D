/*  TA3D, a remake of Total Annihilation
    Copyright (C) 2005  Roland BROCHARD

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA*/

#ifndef __STACK_H__
#define __STACK_H__

#include <deque>

namespace TA3D
{


	template< typename T >
	class Stack
	{
	private:
		std::deque<T> container;

	public:

		Stack() {}

		~Stack()    {   container.clear();  }

		void clear() {   container.clear();  }

		T& top()
		{
			return container.back();
		}

		const T& top() const
		{
			return container.back();
		}

		T& bottom()
		{
			return container.front();
		}

		const T& bottom() const
		{
			return container.front();
		}

		size_t size() const
		{
			return container.size();
		}

		T& operator[](int idx)
		{
			return container[idx];
		}

		const T& operator[](int idx) const
		{
			return container[idx];
		}

		T pop()
		{
			if (empty())
				return T();
			T t = container.back();
			container.pop_back();
			return t;
		}

		bool empty() const {   return container.empty();   }

		void push(const T &t)
		{
			container.push_back(t);
		}
	};





}
#endif
