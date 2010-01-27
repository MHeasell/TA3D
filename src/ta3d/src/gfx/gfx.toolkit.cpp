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
#include <stdafx.h>
#include "gfx.toolkit.h"
#include <misc/paths.h>
#include <TA3D_NameSpace.h>
#include "gfx.h"
#include <zlib.h>
#include <yuni/core/io/file/stream.h>

using namespace Yuni::Core::IO::File;

namespace TA3D
{
	static bool use_TA_palette = true;


	void disable_TA_palette()
	{
		use_TA_palette = false;
	}

	void enable_TA_palette()
	{
		use_TA_palette = true;
	}



	SDL_Surface *convert_format(SDL_Surface *bmp)
	{
		if (bmp->format->BitsPerPixel != 32
			|| bmp->format->Rmask != 0x000000FF
			|| bmp->format->Gmask != 0x0000FF00
			|| bmp->format->Bmask != 0x00FF0000
			|| bmp->format->Amask != 0xFF000000)
		{
			SDL_PixelFormat target_format;
			target_format.palette = NULL;
			target_format.BitsPerPixel = 32;
			target_format.BytesPerPixel = 4;
			target_format.Rloss = target_format.Gloss = target_format.Bloss = target_format.Aloss = 0;
			target_format.Rmask = 0x000000FF;
			target_format.Gmask = 0x0000FF00;
			target_format.Bmask = 0x00FF0000;
			target_format.Amask = 0xFF000000;
			target_format.colorkey = 0x00FF00FF;
			target_format.alpha = 0xFF;
			target_format.Rshift = 0;
			target_format.Gshift = 8;
			target_format.Bshift = 16;
			target_format.Ashift = 24;

			if (bmp->format->BitsPerPixel == 8 && use_TA_palette)
				SDL_SetPalette(bmp, SDL_LOGPAL|SDL_PHYSPAL, TA3D::VARS::pal, 0, 256);

			SDL_Surface *tmp = SDL_ConvertSurface(bmp, &target_format, SDL_SWSURFACE);
			SDL_FreeSurface(bmp);
			bmp = tmp;
		}
		return bmp;
	}

	SDL_Surface *convert_format_copy(SDL_Surface *bmp)
	{
		SDL_PixelFormat target_format;
		target_format.palette = NULL;
		target_format.BitsPerPixel = 32;
		target_format.BytesPerPixel = 4;
		target_format.Rloss = target_format.Gloss = target_format.Bloss = target_format.Aloss = 0;
		target_format.Rmask = 0x000000FF;
		target_format.Gmask = 0x0000FF00;
		target_format.Bmask = 0x00FF0000;
		target_format.Amask = 0xFF000000;
		target_format.colorkey = 0x00FF00FF;
		target_format.alpha = 0xFF;
		target_format.Rshift = 0;
		target_format.Gshift = 8;
		target_format.Bshift = 16;
		target_format.Ashift = 24;

		if (bmp->format->BitsPerPixel == 8 && use_TA_palette)
			SDL_SetPalette(bmp, SDL_LOGPAL|SDL_PHYSPAL, TA3D::VARS::pal, 0, 256);

		SDL_Surface *tmp = SDL_ConvertSurface(bmp, &target_format, SDL_SWSURFACE);

		return tmp;
	}

	SDL_Surface *convert_format_24(SDL_Surface *bmp)
	{
		if (bmp->format->BitsPerPixel != 24
			|| bmp->format->Rmask != 0x000000FF
			|| bmp->format->Gmask != 0x0000FF00
			|| bmp->format->Bmask != 0x00FF0000)
		{
			SDL_PixelFormat target_format;
			target_format.palette = NULL;
			target_format.BitsPerPixel = 24;
			target_format.BytesPerPixel = 3;
			target_format.Rloss = target_format.Gloss = target_format.Bloss = 0;
			target_format.Aloss = 8;
			target_format.Rmask = 0x000000FF;
			target_format.Gmask = 0x0000FF00;
			target_format.Bmask = 0x00FF0000;
			target_format.Amask = 0x00000000;
			target_format.colorkey = 0x00FF00FF;
			target_format.alpha = 0xFF;
			target_format.Rshift = 0;
			target_format.Gshift = 8;
			target_format.Bshift = 16;
			target_format.Ashift = 24;

			if (bmp->format->BitsPerPixel == 8 && use_TA_palette)
				SDL_SetPalette(bmp, SDL_LOGPAL|SDL_PHYSPAL, TA3D::VARS::pal, 0, 256);

			SDL_Surface *tmp = SDL_ConvertSurface(bmp, &target_format, SDL_SWSURFACE);
			SDL_FreeSurface(bmp);
			bmp = tmp;
		}
		return bmp;
	}

	SDL_Surface *convert_format_16(SDL_Surface *bmp)
	{
		if (bmp->format->BitsPerPixel != 16)
		{
			SDL_PixelFormat target_format;
			target_format.palette = NULL;
			target_format.BitsPerPixel = 16;
			target_format.BytesPerPixel = 2;
			target_format.Rloss = 3;
			target_format.Gloss = 2;
			target_format.Bloss = 3;
			target_format.Aloss = 8;
			target_format.Rmask = 0x0000001F;
			target_format.Gmask = 0x000007E0;
			target_format.Bmask = 0x0000F800;
			target_format.Amask = 0x00000000;
			target_format.colorkey = 0x0000F81F;
			target_format.alpha = 0xFF;
			target_format.Rshift = 0;
			target_format.Gshift = 5;
			target_format.Bshift = 11;
			target_format.Ashift = 16;

			if (bmp->format->BitsPerPixel == 8 && use_TA_palette)
				SDL_SetPalette(bmp, SDL_LOGPAL|SDL_PHYSPAL, TA3D::VARS::pal, 0, 256);

			SDL_Surface *tmp = SDL_ConvertSurface(bmp, &target_format, SDL_SWSURFACE);
			SDL_FreeSurface(bmp);
			bmp = tmp;
		}
		return bmp;
	}

	void blit(SDL_Surface *in, SDL_Surface *out, int x0, int y0, int x1, int y1, int w, int h)
	{
		SDL_Surface *tmp = in;
		if (in->format->BitsPerPixel != out->format->BitsPerPixel)
		{
			if (in->format->BitsPerPixel == 8 && use_TA_palette)
				SDL_SetPalette(in, SDL_LOGPAL|SDL_PHYSPAL, TA3D::VARS::pal, 0, 256);

			tmp = SDL_ConvertSurface(in, out->format, SDL_SWSURFACE);
		}

		SDL_LockSurface(tmp);
		SDL_LockSurface(out);

		int sx = x0;
		int dx = x1;
		int cw = w;
		if (sx < 0)
		{
			dx -= sx;
			cw += sx;
			sx = 0;
		}
		if (dx < 0)
		{
			sx -= dx;
			cw += dx;
			dx = 0;
		}

		if (sx < tmp->w && dx < out->w)
		{
			sx *= tmp->format->BytesPerPixel;
			dx *= tmp->format->BytesPerPixel;
			cw *= tmp->format->BytesPerPixel;
			for(int y = 0 ; y < h ; y++)
			{
				int dy = y1 + y;
				int sy = y + y0;
				if (dy < 0 || dy >= out->h || sy < 0 || sy >= tmp->h)    continue;
				dy *= out->pitch;
				sy *= tmp->pitch;

				memcpy( ((byte*)out->pixels) + dy + dx, ((byte*)tmp->pixels) + sy + sx, cw);
			}
		}

		SDL_UnlockSurface(tmp);
		SDL_UnlockSurface(out);

		if (in->format->BitsPerPixel != out->format->BitsPerPixel)
			SDL_FreeSurface(tmp);
	}

	void masked_blit(SDL_Surface *in, SDL_Surface *out, int x0, int y0, int x1, int y1, int w, int h)
	{
		SDL_LockSurface(in);
		SDL_LockSurface(out);
		for(int y = 0 ; y < h ; y++)
		{
			int dy = (y1 + y) * out->pitch;
			int sy = (y + y0) * in->pitch;
			for(int x = 0 ; x < w ; x++)
			{
				int sx = x + x0;
				int dx = x1 + x;
				byte b = ((byte*)in->pixels)[sy + sx];
				if (b)
					((byte*)out->pixels)[dy + dx] = b;
			}
		}
		SDL_UnlockSurface(in);
		SDL_UnlockSurface(out);
	}

	void stretch_blit( SDL_Surface *in, SDL_Surface *out, int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1 )
	{
		sint32 dw = (w0 << 16) / w1;
		sint32 dh = (h0 << 16) / h1;
		int dy = y1 * out->pitch;
		switch(in->format->BitsPerPixel)
		{
			case 8:
				for(int y = 0 ; y < h1 ; y++)
				{
					int sy = (y0 + (y * dh >> 16)) * in->pitch;
					byte *d = ((byte*)out->pixels) + x1 + dy;
					int sx = (sy + x0) << 16;
					for(int x = 0 ; x < w1 ; x++)
					{
						*d = ((byte*)in->pixels)[sx >> 16];
						d++;
						sx += dw;
					}
					dy += out->pitch;
				}
				break;
			case 16:
				dy >>= 1;
				x1 >>= 1;
				for(int y = 0 ; y < h1 ; y++)
				{
					int sy = (y0 + (y * dh >> 16)) * in->pitch >> 1;
					uint16 *d = ((uint16*)out->pixels) + dy + x1;
					int sx = (sy + x0) << 16;
					for(int x = 0 ; x < w1 ; x++)
					{
						*d = ((uint16*)in->pixels)[sx >> 16];
						d++;
						sx += dw;
					}
					dy += out->pitch >> 1;
				}
				break;
			case 24:
				x1 *= 3;
				for(int y = 0 ; y < h1 ; y++)
				{
					int sy = (y0 + (y * dh >> 16)) * in->pitch;
					byte *d = ((byte*)out->pixels) + dy + x1;
					int sx = x0 << 16;
					for(int x = 0 ; x < w1 ; x++)
					{
						byte *ref = ((byte*)in->pixels) + sy + (sx >> 16) * 3;
						*(d++) = *(ref++);
						*(d++) = *(ref++);
						*(d++) = *(ref++);
						sx += dw;
					}
					dy += out->pitch;
				}
				break;
			case 32:
				x1 <<= 2;
				for(int y = 0 ; y < h1 ; y++)
				{
					int sy = (y0 + (y * dh >> 16)) * in->pitch;
					uint32 *d = (uint32*) (((byte*)out->pixels) + dy + x1);
					int sx = x0 << 16;
					for(int x = 0 ; x < w1 ; x++)
					{
						*d++ = *(uint32*)(((byte*)in->pixels) + sy + ((sx >> 16) << 2));
						sx += dw;
					}
					dy += out->pitch;
				}
				break;
		};
	}

	void stretch_blit_smooth( SDL_Surface *in, SDL_Surface *out, int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1 )
	{
		w0--;
		h0--;
		sint32 dw = (w0 << 16) / w1;
		sint32 dh = (h0 << 16) / h1;
		int dy = y1 * out->pitch;
		switch(in->format->BitsPerPixel)
		{
			case 8:
				for(int y = 0 ; y < h1 ; y++)
				{
					int gy = y * dh;
					int sy = (y0 + (gy >> 16)) * in->pitch;
					byte *d = ((byte*)out->pixels) + x1 + dy;
					int sx = (sy + x0) << 16;
					for(int x = 0 ; x < w1 ; x++)
					{
						byte *ref = ((byte*)in->pixels) + (sx >> 16);
						int c0 = *ref++, c1, c2, c3;
						c1 = *ref;
						ref += in->pitch - 1;
						c2 = *ref++;
						c3 = *ref;

						c0 += (c1 - c0) * (sx & 0xFFFF) >> 16;
						c2 += (c3 - c2) * (sx & 0xFFFF) >> 16;
						*d++ = byte(c0 + ((c2 - c0) * (gy & 0xFFFF) >> 16));
						sx += dw;
					}
					dy += out->pitch;
				}
				break;
			case 16:
				LOG_DEBUG(LOG_PREFIX_GFX << "stretch_blit_smooth doesn't support 16 bits images");
				break;
			case 24:
				x1 *= 3;
				for(int y = 0 ; y < h1 ; y++)
				{
					int gy = y * dh;
					int sy = (y0 + (gy >> 16)) * in->pitch;
					byte *d = ((byte*)out->pixels) + dy + x1;
					int sx = x0 << 16;
					for(int x = 0 ; x < w1 ; x++)
					{
						byte *ref = ((byte*)in->pixels) + sy + (sx >> 16) * 3;
						int r0 = *ref++, r1, r2, r3;
						int g0 = *ref++, g1, g2, g3;
						int b0 = *ref++, b1, b2, b3;
						r1 = *ref++;
						g1 = *ref++;
						b1 = *ref;
						ref += in->pitch - 5;
						r2 = *ref++;
						g2 = *ref++;
						b2 = *ref++;
						r3 = *ref++;
						g3 = *ref++;
						b3 = *ref;

						r0 += (r1 - r0) * (sx & 0xFFFF) >> 16;
						r2 += (r3 - r2) * (sx & 0xFFFF) >> 16;
						*d++ = byte(r0 + ((r2 - r0) * (gy & 0xFFFF) >> 16));

						g0 += (g1 - g0) * (sx & 0xFFFF) >> 16;
						g2 += (g3 - g2) * (sx & 0xFFFF) >> 16;
						*d++ = byte(g0 + ((g2 - g0) * (gy & 0xFFFF) >> 16));

						b0 += (b1 - b0) * (sx & 0xFFFF) >> 16;
						b2 += (b3 - b2) * (sx & 0xFFFF) >> 16;
						*d++ = byte(b0 + ((b2 - b0) * (gy & 0xFFFF) >> 16));
						sx += dw;
					}
					dy += out->pitch;
				}
				break;
			case 32:
				x1 <<= 2;
				for(int y = 0 ; y < h1 ; y++)
				{
					int gy = y * dh;
					int sy = (y0 + (gy >> 16)) * in->pitch;
					byte *d = ((byte*)out->pixels) + dy + x1;
					int sx = x0 << 16;
					for(int x = 0 ; x < w1 ; x++)
					{
						byte *ref = ((byte*)in->pixels) + sy + ((sx >> 16) << 2);

						int r0 = *ref++, r1, r2, r3;
						int g0 = *ref++, g1, g2, g3;
						int b0 = *ref++, b1, b2, b3;
						int a0 = *ref++, a1, a2, a3;
						r1 = *ref++;
						g1 = *ref++;
						b1 = *ref++;
						a1 = *ref;
						ref += in->pitch - 7;
						r2 = *ref++;
						g2 = *ref++;
						b2 = *ref++;
						a2 = *ref++;
						r3 = *ref++;
						g3 = *ref++;
						b3 = *ref++;
						a3 = *ref;

						r0 += (r1 - r0) * (sx & 0xFFFF) >> 16;
						r2 += (r3 - r2) * (sx & 0xFFFF) >> 16;
						*d++ = byte(r0 + ((r2 - r0) * (gy & 0xFFFF) >> 16));

						g0 += (g1 - g0) * (sx & 0xFFFF) >> 16;
						g2 += (g3 - g2) * (sx & 0xFFFF) >> 16;
						*d++ = byte(byte(g0 + ((g2 - g0) * (gy & 0xFFFF) >> 16)));

						b0 += (b1 - b0) * (sx & 0xFFFF) >> 16;
						b2 += (b3 - b2) * (sx & 0xFFFF) >> 16;
						*d++ = byte(b0 + ((b2 - b0) * (gy & 0xFFFF) >> 16));

						a0 += (a1 - a0) * (sx & 0xFFFF) >> 16;
						a2 += (a3 - a2) * (sx & 0xFFFF) >> 16;
						*d++ = byte(a0 + ((a2 - a0) * (gy & 0xFFFF) >> 16));

						sx += dw;
					}
					dy += out->pitch;
				}
				break;
		};
	}

	void putpixel(SDL_Surface *bmp, int x, int y, uint32 col)
	{
		if (x < 0 || y < 0 || x >= bmp->w || y >= bmp->h)   return;
		switch(bmp->format->BitsPerPixel)
		{
			case 8:
				SurfaceByte(bmp, x, y) = byte(col);
				break;
			case 16:
				(((uint16*)((bmp)->pixels))[(y) * ((bmp)->pitch >> 1) + (x)]) = uint16(col);
				break;
			case 24:
				SurfaceByte(bmp, x * 3, y) = getb32(col);
				SurfaceByte(bmp, x * 3 + 1, y) = getg32(col);
				SurfaceByte(bmp, x * 3 + 2, y) = getr32(col);
				break;
			case 32:
				SurfaceInt(bmp, x, y) = col;
				break;
		};
	}

	uint32 getpixel(SDL_Surface *bmp, int x, int y)
	{
		if (x < 0 || y < 0 || x >= bmp->w || y >= bmp->h)   return 0;
		switch(bmp->format->BitsPerPixel)
		{
			case 8:
				return SurfaceByte(bmp, x, y);
			case 16:
				return (((uint16*)((bmp)->pixels))[(y) * ((bmp)->pitch >> 1) + (x)]);
			case 24:
				{
					int b = SurfaceByte(bmp, x * 3, y);
					int g = SurfaceByte(bmp, x * 3 + 1, y);
					int r = SurfaceByte(bmp, x * 3 + 2, y);
					return makecol24(r,g,b);
				}
			case 32:
				return SurfaceInt(bmp, x, y);
		};
		return 0;
	}

	void line(SDL_Surface *bmp, int x0, int y0, int x1, int y1, uint32 col)
	{
		if (abs(x0 - x1) > abs(y0 - y1))
		{
			if (x0 > x1)
			{
				x0 ^= x1;   x1 ^= x0;   x0 ^= x1;
				y0 ^= y1;   y1 ^= y0;   y0 ^= y1;
			}
			for(int x = x0 ; x <= x1 ; x++)
				putpixel(bmp, x, y0 + (y1 - y0) * (x - x0) / (x1 - x0), col);
		}
		else
		{
			if (y0 > y1)
			{
				x0 ^= x1;   x1 ^= x0;   x0 ^= x1;
				y0 ^= y1;   y1 ^= y0;   y0 ^= y1;
			}
			for(int y = y0 ; y <= y1 ; y++)
				putpixel(bmp, x0 + (x1 - x0) * (y - y0) / (y1 - y0), y, col);
		}
	}

	void triangle(SDL_Surface *bmp, int x0, int y0, int x1, int y1, int x2, int y2, uint32 col)
	{
		if (y0 > y1)
		{
			x0 ^= x1;   x1 ^= x0;   x0 ^= x1;
			y0 ^= y1;   y1 ^= y0;   y0 ^= y1;
		}
		if (y1 > y2)
		{
			x2 ^= x1;   x1 ^= x2;   x2 ^= x1;
			y2 ^= y1;   y1 ^= y2;   y2 ^= y1;
		}
		if (y0 > y1)
		{
			x0 ^= x1;   x1 ^= x0;   x0 ^= x1;
			y0 ^= y1;   y1 ^= y0;   y0 ^= y1;
		}

		if (y0 < y1)
			for(int y = y0 ; y <= y1 ; y++)
			{
				int ax = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
				int bx = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
				if (ax > bx)
				{
					ax ^= bx;   bx ^= ax;   ax ^= bx;
				}
				for(int x = ax ; x <= bx ; x++)
					putpixel(bmp, x, y, col);
			}
		if (y1 < y2)
			for(int y = y1 ; y <= y2 ; y++)
			{
				int ax = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
				int bx = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
				if (ax > bx)
				{
					ax ^= bx;   bx ^= ax;   ax ^= bx;
				}
				for(int x = ax ; x <= bx ; x++)
					putpixel(bmp, x, y, col);
			}
	}

	void circlefill(SDL_Surface *bmp, int x, int y, int r, uint32 col)
	{
		int r2 = r * r;
		int my = Math::Max(-r, -y);
		int My = Math::Min(r, bmp->h - 1 - y);
		switch(bmp->format->BitsPerPixel)
		{
			case 8:
				for (int sy = my ; sy <= My ; ++sy)
				{
					int dx = int(sqrtf(float(r2 - sy * sy)));
					int ax = Math::Max(x - dx, 0);
					int bx = Math::Min(x + dx, bmp->w - 1);
					memset((byte*)bmp->pixels + ax + (y + sy) * bmp->pitch, col, bx - ax + 1);
				}
				break;
			case 16:
				{
					uint16 col16 = uint16(col);
					for (int sy = my ; sy <= My ; ++sy)
					{
						int dx = int(sqrtf(float(r - sy * sy)));
						int ax = Math::Max(x - dx, 0);
						int bx = Math::Min(x + dx, bmp->w - 1);
						uint16 *p = (uint16*)bmp->pixels + ax + (y + sy) * (bmp->pitch >> 1);
						for (uint16 *end = p + bx - ax + 1; p != end ; ++p)
							*p = col16;
					}
				}
				break;
			case 24:
				{
					byte colb = getb32(col);
					byte colg = getg32(col);
					byte colr = getr32(col);
					for (int sy = my ; sy <= My ; ++sy)
					{
						int dx = int(sqrtf(float(r - sy * sy)));
						int ax = Math::Max(x - dx, 0);
						int bx = Math::Min(x + dx, bmp->w - 1);
						byte *p = (byte*)bmp->pixels + ax * 3 + (y + sy) * bmp->pitch;
						for (byte *end = p + (bx - ax + 1) * 3 ; p != end ; ++p)
						{
							*p++ = colb;
							*p++ = colg;
							*p = colr;
						}
					}
				}
				break;
			case 32:
				for (int sy = my ; sy <= My ; ++sy)
				{
					int dx = int(sqrtf(float(r - sy * sy)));
					int ax = Math::Max(x - dx, 0);
					int bx = Math::Min(x + dx, bmp->w - 1);
					uint32 *p = (uint32*)bmp->pixels + ax + (y + sy) * (bmp->pitch >> 2);
					for (uint32 *end = p + bx - ax + 1; p != end ; ++p)
						*p = col;
				}
				break;
		};
	}

	void rectfill(SDL_Surface *bmp, int x0, int y0, int x1, int y1, uint32 col)
	{
		SDL_Rect rect = {x0, y0, x1 - x0, y1 - y0};
		SDL_FillRect(bmp, &rect, col);
	}

    void vflip_bitmap(SDL_Surface* bmp)
    {
        for(int y = 0 ; y < (bmp->h + 1 >> 1) ; ++y)
            for(int x = 0 ; x < bmp->w ; ++x)
            {
                uint32 c = getpixel(bmp, x, y);
                putpixel(bmp, x, y, getpixel(bmp, x, bmp->h - 1 - y));
                putpixel(bmp, x, bmp->h - 1 - y, c);
            }
    }

    void hflip_bitmap(SDL_Surface* bmp)
    {
        for(int y = 0 ; y < bmp->h ; ++y)
            for(int x = 0 ; x < (bmp->w + 1 >> 1) ; ++x)
            {
                uint32 c = getpixel(bmp, x, y);
                putpixel(bmp, x, y, getpixel(bmp, bmp->w - 1 - x, y));
                putpixel(bmp, bmp->w - 1 - x, y, c);
            }
    }

	void SaveTex(SDL_Surface *bmp, const String &filename)
	{
        gzFile file = gzopen(filename.c_str(), "wb6");
		if (file)
		{
			SDL_LockSurface(bmp);
			int w = bmp->w;
			int h = bmp->h;
			int bpp = bmp->format->BitsPerPixel;
			gzwrite( file, &w, sizeof(w));
			gzwrite( file, &h, sizeof(h));
			gzwrite( file, &bpp, sizeof(bpp));
			for(int y = 0 ; y < bmp->h ; y++)
				gzwrite( file, ((char*)(bmp->pixels)) + y * bmp->pitch, bmp->w * bmp->format->BytesPerPixel);
			SDL_UnlockSurface(bmp);

			gzclose( file );
		}
		else
			LOG_ERROR("could not save file : " << filename);
	}

	SDL_Surface *LoadTex(const String &filename)
	{
		gzFile file = gzopen(filename.c_str(), "rb");
		if (file)
		{
			int w, h, bpp;
			gzread( file, &w, sizeof(w));
			gzread( file, &h, sizeof(h));
			gzread( file, &bpp, sizeof(bpp));
			SDL_Surface *bmp = gfx->create_surface_ex(bpp, w, h);
			SDL_LockSurface(bmp);
			for(int y = 0 ; y < bmp->h ; y++)
				gzread( file, ((char*)(bmp->pixels)) + y * bmp->pitch, bmp->w * bmp->format->BytesPerPixel);
			SDL_UnlockSurface(bmp);

			gzclose( file );

			return bmp;
		}
		else
			LOG_ERROR("could not load file : " << filename);
		return NULL;
	}

	void save_bitmap(const String &filename, SDL_Surface* bmp)
	{
		String ext = String::ToLower( Paths::ExtractFileExt(filename) );
		if (ext == ".bmp")
			SDL_SaveBMP(bmp, filename.c_str());
		else if (ext == ".tex")                      // This is for cached texture data
			SaveTex(bmp, filename);
		else if (ext == ".tga")
			save_TGA(filename, bmp);
		else
			LOG_WARNING("save_bitmap : file format not supported : " << ext << " (" << filename << ")");
	}

	struct TGAHeader
	{
		// imagetype 2==truecolour uncompressed,
		// 3==b+w uncompressed (theres no implementational difference between the two)

		byte id;        // image ID size (between header and image data), here 0, we don't need it
		byte colormap;
		byte type;
		byte colormapSpec[5];

		uint16 x;
		uint16 y;
		uint16 w;
		uint16 h;
		uint8  bpp;

		byte description;
	};

	void save_TGA(const String &filename, SDL_Surface* bmp, bool compress)
	{
		TGAHeader header;

		header.id = 0;
		header.colormap = 0;
		header.type = compress ? 10 : 2; // 24/32 bits uncompressed image
		memset( header.colormapSpec, 0, 5 );

		header.x = 0;
		header.y = 0;
		header.w = bmp->w;
		header.h = bmp->h;
		header.bpp = bmp->format->BitsPerPixel;
		header.description = (header.bpp == 32) ? 0x28 : 0x20;

		Stream file( filename, OpenMode::write );

		if (file.opened())
		{
			file.write( (const char*)&header, sizeof(header) );
			if (!compress)			// Uncompressed
			{
				for(int y = 0 ; y < bmp->h ; ++y)
				{
					for(int x = 0 ; x < bmp->w ; ++x)
					{
						switch(bmp->format->BitsPerPixel)
						{
						case 8:
							file.put( getpixel(bmp, x, y) );
							break;
						case 16:
							file.write( (const char*)bmp->pixels + (bmp->w * y + x << 1), 2 );
							break;
						case 24:
							{
								uint32 c = getpixel(bmp, x, y);
								file.put( (bmp->format->Bmask & c) >> bmp->format->Bshift);
								file.put( (bmp->format->Gmask & c) >> bmp->format->Gshift);
								file.put( (bmp->format->Rmask & c) >> bmp->format->Rshift);
							}
							break;
						case 32:
							{
								uint32 c = getpixel(bmp, x, y);
								file.put( (bmp->format->Bmask & c) >> bmp->format->Bshift);
								file.put( (bmp->format->Gmask & c) >> bmp->format->Gshift);
								file.put( (bmp->format->Rmask & c) >> bmp->format->Rshift);
								file.put( (bmp->format->Amask & c) >> bmp->format->Ashift);
							}
							break;
						};
					}
				}
			}
			else					// Compressed
			{
				int type = 0;
				uint32 c = 0;
				int len = 0;
				for(int i = 0 ; i < bmp->w * bmp->h ; ++i)
				{
					int x = i % bmp->w;
					int y = i / bmp->w;
					if (len == 0)
					{
						c = getpixel(bmp, x, y);
						++len;
						continue;
					}
					if (len == 1)
					{
						++len;
						if (c == getpixel(bmp, x, y))
							type = 1;
						else
							type = 0;
						continue;
					}
					if (len == 128
						|| (type == 1 && c != getpixel(bmp, x, y))
						|| (type == 0 && c == getpixel(bmp, x, y))
						|| i + 1 == bmp->w * bmp->h)
					{
						file.put( (type << 7) | (len - 1) );
						int s = (type == 1) ? i - 1 : i - len;

						for(int j = s ; j < i ; ++j)
						{
							x = j % bmp->w;
							y = j / bmp->w;
							switch(bmp->format->BitsPerPixel)
							{
							case 8:
								file.put( getpixel(bmp, x, y) );
								break;
							case 16:
								file.write( (const char*)bmp->pixels + (bmp->w * y + x << 1), 2 );
								break;
							case 24:
								{
									uint32 c = getpixel(bmp, x, y);
									file.put( (bmp->format->Bmask & c) >> bmp->format->Bshift);
									file.put( (bmp->format->Gmask & c) >> bmp->format->Gshift);
									file.put( (bmp->format->Rmask & c) >> bmp->format->Rshift);
								}
								break;
							case 32:
								{
									uint32 c = getpixel(bmp, x, y);
									file.put( (bmp->format->Bmask & c) >> bmp->format->Bshift);
									file.put( (bmp->format->Gmask & c) >> bmp->format->Gshift);
									file.put( (bmp->format->Rmask & c) >> bmp->format->Rshift);
									file.put( (bmp->format->Amask & c) >> bmp->format->Ashift);
								}
								break;
							};
						}

						if (i + 1 == bmp->w * bmp->h)
							break;
						len = 0;
						--i;
						continue;
					}
					if (type == 0)
						c = getpixel(bmp, x, y);
					++len;
				}
			}
			file.close();
		}
	}
}
