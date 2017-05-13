#ifndef __TA3D_GRID_H__
#define __TA3D_GRID_H__

#include <logs/logs.h>
#include <vector>
#include "misc/point.h"

namespace TA3D
{
	template <class T>
	struct GridDataType
	{
		typedef T type;
	};
	template <>
	struct GridDataType<bool>
	{
		typedef unsigned char type;
	};

	template <class T>
	class Grid
	{
	public:
		typedef typename GridDataType<T>::type Type;
		typedef typename std::vector<Type> Container;
		typedef Type& reference;
		typedef const Type& const_reference;

	public:
		Grid() : w(0U), h(0U), data()
		{
			resize(1U, 1U);
		}
		Grid(unsigned int w, unsigned int h) : w(0U), h(0U), data()
		{
			resize(w, h);
		}

		Grid(const Grid&) = delete;
		Grid& operator=(const Grid&) = delete;

		void resize(unsigned int w, unsigned int h)
		{
			this->w = w;
			this->h = h;
			data.resize(w * h);
		}

		inline const_reference operator()(unsigned int x, unsigned int y) const
		{
			LOG_ASSERT(x < w && y < h);
			return data[x + y * w];
		}
		inline reference operator()(unsigned int x, unsigned int y)
		{
			LOG_ASSERT(x < w && y < h);
			return data[x + y * w];
		}

		inline int getWidth() const { return w; }
		inline int getHeight() const { return h; }
		inline void add(const Grid<T>& grid, int x, int y)
		{
			const unsigned int j0 = std::max(0, -y);
			const unsigned int j1 = std::min<unsigned int>(grid.getHeight(), h - y);
			const unsigned int i0 = std::max(0, -x);
			const unsigned int i1 = std::min<unsigned int>(grid.getWidth(), w - x);
			for (unsigned int j = j0; j < j1; ++j)
				for (unsigned int i = i0; i < i1; ++i)
					(*this)(i + x, j + y) += grid(i, j);
		}
		inline void sub(const Grid<T>& grid, int x, int y)
		{
			const unsigned int j0 = std::max(0, -y);
			const unsigned int j1 = std::min<unsigned int>(grid.getHeight(), h - y);
			const unsigned int i0 = std::max(0, -x);
			const unsigned int i1 = std::min<unsigned int>(grid.getWidth(), w - x);
			for (unsigned int j = j0; j < j1; ++j)
				for (unsigned int i = i0; i < i1; ++i)
					(*this)(i + x, j + y) -= grid(i, j);
		}

		/**
		 * Overwrites every cell in the grid with the given value.
		 */
		inline void fill(const T& v)
		{
			std::fill(data.begin(), data.end(), v);
		}

		inline void* getData() { return &(data.front()); }
		inline unsigned int getSize() const { return (unsigned int)(data.size() * sizeof(T)); }

		inline void hline(const int x0, const int x1, const int y, const T& col)
		{
			std::fill(data.begin() + (y * w + x0), data.begin() + (y * w + x1 + 1), col);
		}
		inline void circlefill(const int x, const int y, const int r, const T& col)
		{
			const int r2 = r * r;
			const int my = std::max<int>(-r, -y);
			const int My = std::min<int>(r, h - 1 - y);
			for (int sy = my; sy <= My; ++sy)
			{
				const int dx = int(std::sqrt(float(r2 - sy * sy)));
				const int ax = std::max<int>(x - dx, 0);
				const int bx = std::min<int>(x + dx, w - 1);
				hline(ax, bx, y + sy, col);
			}
		}

	private:
		unsigned int w;
		unsigned int h;
		Container data;
	};

	void gaussianFilter(Grid<float>& grid, float sigma);
}

#endif
