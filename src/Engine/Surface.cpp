/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "Surface.h"
#include "ShaderDraw.h"
#include "ShaderMove.h"
#include <vector>
#include <algorithm>
#include <SDL_gfxPrimitives.h>
#include <SDL_image.h>
#include "../lodepng.h"
#include "Palette.h"
#include "Exception.h"
#include "Logger.h"
#include "SDL2Helpers.h"
#include "FileMap.h"
#ifdef _WIN32
#include <malloc.h>
#endif
#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#define _aligned_malloc __mingw_aligned_malloc
#define _aligned_free   __mingw_aligned_free
#endif //MINGW
#ifdef __MORPHOS__
#include <ppcinline/exec.h>
#endif

namespace OpenXcom
{


namespace
{

/**
 * Helper function counting pitch in bytes with 16byte padding
 * @param bpp bits per pixel
 * @param width number of pixel in row
 * @return pitch in bytes
 */
inline int GetPitch(int bpp, int width)
{
	return ((bpp/8) * width + 15) & ~0xF;
}


/**
 * Raw copy without any change of pixel index value between two SDL surface, palette is ignored
 * @param dest Destination surface
 * @param src Source surface
 */
inline void RawCopySurf(const Surface::UniqueSurfacePtr& dest, const Surface::UniqueSurfacePtr& src)
{
	ShaderDrawFunc(
		[](Uint8& destStuff, Uint8& srcStuff)
		{
			destStuff = srcStuff;
		},
		ShaderMove<Uint8>(dest.get()),
		ShaderMove<Uint8>(src.get())
	);
}

/**
 * TODO: function for purge, we should accept only "standard" surfaces
 * Helper function correcting graphic that should have index 0 as transparent,
 * but some do not have, we swap correct with incorrect
 * for maintain 0 as correct transparent index.
 * @param dest Surface to fix
 * @param currentTransColor current transparent color index
 */
inline void FixTransparent(const Surface::UniqueSurfacePtr& dest, int currentTransColor)
{
	if (currentTransColor != 0)
	{
		ShaderDrawFunc(
			[&](Uint8& destStuff)
			{
				if (destStuff == currentTransColor)
				{
					destStuff = 0;
				}
			},
			ShaderMove<Uint8>(dest.get())
		);
	}
}

} //namespace

/**
 * Helper function creating aligned buffer
 * @param bpp bits per pixel
 * @param width number of pixel in row
 * @param height number of rows
 * @return pointer to memory
 */
Surface::UniqueBufferPtr Surface::NewAlignedBuffer(int bpp, int width, int height)
{
	const int pitch = GetPitch(bpp, width);
	const int total = pitch * height;
	void* buffer = 0;

#ifndef _WIN32

	#ifdef __MORPHOS__

	buffer = calloc( total, 1 );
	if (!buffer)
	{
		throw Exception("Failed to allocate surface");
	}

	#else
	int rc;
	if ((rc = posix_memalign(&buffer, 16, total)))
	{
		throw Exception(strerror(rc));
	}
	#endif

#else

	// of course Windows has to be difficult about this!
	buffer = _aligned_malloc(total, 16);
	if (!buffer)
	{
		throw Exception("Failed to allocate surface");
	}

#endif

	memset(buffer, 0, total);
	return Surface::UniqueBufferPtr((Uint8*)buffer);
}

/**
 * Helper function creating new unique pointer
 * @param surface
 * @return Unique pointer
 */
Surface::UniqueSurfacePtr Surface::NewSdlSurface(SDL_Surface* surface)
{
	return Surface::UniqueSurfacePtr(surface);
}

/**
 * Helper function creating new SDL surface in unique pointer
 * @param buffer memory buffer
 * @param bpp bit depth
 * @param width width of surface
 * @param height height of surface
 * @return Unique pointer
 */
Surface::UniqueSurfacePtr Surface::NewSdlSurface(const Surface::UniqueBufferPtr& buffer, int bpp, int width, int height)
{
	auto surface = SDL_CreateRGBSurfaceFrom(buffer.get(), width, height, bpp, GetPitch(bpp, width), 0, 0, 0, 0);
	if (!surface)
	{
		throw Exception(SDL_GetError());
	}

	return NewSdlSurface(surface);
}

/**
 * Zero whole surface.
 */
void Surface::CleanSdlSurface(SDL_Surface* surface)
{
	if (surface->flags & SDL_SWSURFACE)
	{
		memset(surface->pixels, 0, surface->h * surface->pitch);
	}
	else
	{
		SDL_Rect c;
		c.x = 0;
		c.y = 0;
		c.w = surface->w;
		c.h = surface->h;
		SDL_FillRect(surface, &c, 0);
	}
}
/**
 * Default deleter for alignment buffer
 * @param buffer
 */
void Surface::UniqueBufferDeleter::operator ()(Uint8* buffer)
{
	if (buffer)
	{
#ifdef _WIN32
		_aligned_free(buffer);
#else
		free(buffer);
#endif
	}
}

/**
 * Default deleter for SDL surface
 * @param surf
 */
void Surface::UniqueSurfaceDeleter::operator ()(SDL_Surface* surf)
{
	SDL_FreeSurface(surf);
}



/**
 * Default empty surface.
 */
Surface::Surface() : _x{ }, _y{ }, _width{ }, _height{ }, _pitch{ }, _visible(true), _hidden(false), _redraw(false)
{

}

/**
 * Sets up a blank 8bpp surface with the specified size and position,
 * with pure black as the transparent color.
 * @note Surfaces don't have to fill the whole size since their
 * background is transparent, specially subclasses with their own
 * drawing logic, so it just covers the maximum drawing area.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 * @param bpp Bits-per-pixel depth.
 */
Surface::Surface(int width, int height, int x, int y) : _x(x), _y(y), _visible(true), _hidden(false), _redraw(false)
{
	std::tie(_alignedBuffer, _surface) = Surface::NewPair8Bit(width, height);
	_width = _surface->w;
	_height = _surface->h;
	_pitch = _surface->pitch;
	SDL_SetColorKey(_surface.get(), SDL_SRCCOLORKEY, 0);
}

/**
 * Performs a deep copy of an existing surface.
 * @param other Surface to copy from.
 */
Surface::Surface(const Surface& other) : Surface{ }
{
	if (!other)
	{
		return;
	}
	int width = other.getWidth();
	int height = other.getHeight();
	//move copy
	*this = Surface(width, height, other._x, other._y);
	//cant call `setPalette` because its virtual function and it doesn't work correctly in constructor
	SDL_SetColors(_surface.get(), other.getPalette(), 0, 255);
	RawCopySurf(_surface, other._surface);

	_x = other._x;
	_y = other._y;
	_visible = other._visible;
	_hidden = other._hidden;
	_redraw = other._redraw;
}

/**
 * Deletes the surface from memory.
 */
Surface::~Surface()
{

}

/**
 * Performs a fast copy of a pixel array, accounting for pitch.
 * @param src Source array.
 */
template <typename T>
void Surface::rawCopy(const std::vector<T> &src)
{
	// Copy whole thing
	if (_surface->pitch == _surface->w)
	{
		size_t end = std::min(size_t(_surface->w * _surface->h * _surface->format->BytesPerPixel), src.size());
		std::copy(src.begin(), src.begin() + end, (T*)_surface->pixels);
	}
	// Copy row by row
	else
	{
		for (int y = 0; y < _surface->h; ++y)
		{
			size_t begin = y * _surface->w;
			size_t end = std::min(begin + _surface->w, src.size());
			if (begin >= src.size())
				break;
			std::copy(src.begin() + begin, src.begin() + end, (T*)getRaw(0, y));
		}
	}
}

/**
 * Loads a raw array of pixels into the surface. The pixels must be
 * in the same BPP as the surface.
 * @param bytes Pixel array.
 */
void Surface::loadRaw(const std::vector<unsigned char> &bytes)
{
	lock();
	rawCopy(bytes);
	unlock();
}

/**
 * Loads a raw array of pixels into the surface. The pixels must be
 * in the same BPP as the surface.
 * @param bytes Pixel array.
 */
void Surface::loadRaw(const std::vector<char> &bytes)
{
	lock();
	rawCopy(bytes);
	unlock();
}

/**
 * Loads the contents of an X-Com SCR image file into
 * the surface. SCR files are simply uncompressed images
 * containing the palette offset of each pixel.
 * @param filename Filename of the SCR image.
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#SCR_.26_DAT
 */
void Surface::loadScr(const std::string& filename)
{
	// Load file and put pixels in surface
	auto istream = FileMap::getIStream(filename);
	std::vector<char> buffer((std::istreambuf_iterator<char>(*(istream))), (std::istreambuf_iterator<char>()));
	loadRaw(buffer);
}
/**
 * Loads the contents of an image file of a
 * known format into the surface.
 * @param filename Filename of the image.
 */
void Surface::loadImage(const std::string &filename)
{
	// Destroy current surface (will be replaced)
	_alignedBuffer = nullptr;
	_surface = nullptr;

	Log(LOG_VERBOSE) << "Loading image: " << filename;
	auto rw = FileMap::getRWops(filename);
	if (!rw) { return; } // relevant message gets logged in FileMap.

	// Try loading with LodePNG first
	if (CrossPlatform::compareExt(filename, "png"))
	{
		size_t size;
		void *data = SDL_LoadFile_RW(rw, &size, SDL_FALSE);
		if ((data != NULL) && (size > 8 + 12 + 12)) // minimal PNG file size: header and two empty chunks
		{
			std::vector<unsigned char> png;
			png.resize(size);
			memcpy(&png[0], data, size);

			std::vector<unsigned char> image;
			unsigned width, height;
			lodepng::State state;
			state.decoder.color_convert = 0;
			unsigned error = lodepng::decode(image, width, height, state, png);
			if (!error)
			{
				LodePNGColorMode *color = &state.info_png.color;
				unsigned bpp = lodepng_get_bpp(color);
				if (bpp == 8)
				{
					*this = Surface(width, height, 0, 0);
					setPalette((SDL_Color*)color->palette, 0, color->palettesize);

					ShaderDrawFunc(
						[](Uint8& dest, unsigned char& src)
						{
							dest = src;
						},
						ShaderSurface(this),
						ShaderSurface(SurfaceRaw<unsigned char>(image, width, height))
					);
					int transparent = 0;
					for (int c = 0; c < _surface->format->palette->ncolors; ++c)
					{
						SDL_Color *palColor = _surface->format->palette->colors + c;
						if (palColor->unused == 0)
						{
							transparent = c;
							break;
						}
					}
					FixTransparent(_surface, transparent);
					if (transparent != 0)
					{
						Log(LOG_WARNING) << "Image " << filename << " (from lodepng) has incorrect transparent color index " << transparent << " (instead of 0).";
					}
				}
			} else {
				Log(LOG_ERROR) << "Image " << filename << " lodepng failed:" << lodepng_error_text(error);
			}
		}
		if (data) { SDL_free(data); }
	}
	if (_surface)
	{
		SDL_RWclose(rw);
	}
	else // Otherwise default to SDL_Image
	{
		SDL_RWseek(rw, RW_SEEK_SET, 0); // rewind in case .png was no PNG at all
		auto surface = NewSdlSurface(IMG_Load_RW(rw, SDL_TRUE));
		if (!surface)
		{
			std::string err = filename + ":" + IMG_GetError();
			throw Exception(err);
		}
		if (surface->format->BitsPerPixel != 8)
		{
			std::string err = filename + ": OpenXcom supports only 8bit images.";
			throw Exception(err);
		}

		*this = Surface(surface->w, surface->h, 0, 0);
		setPalette(surface->format->palette->colors, 0, surface->format->palette->ncolors);
		RawCopySurf(_surface, surface);
		FixTransparent(_surface, surface->format->colorkey);
		if (surface->format->colorkey != 0)
		{
			Log(LOG_WARNING) << "Image " << filename << " (from SDL) has incorrect transparent color index " << surface->format->colorkey << " (instead of 0).";
		}
	}
}

/**
 * Loads the contents of an X-Com SPK image file into
 * the surface. SPK files are compressed with a custom
 * algorithm since they're usually full-screen images.
 * @param filename Filename of the SPK image.
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#SPK
 */
void Surface::loadSpk(const std::string& filename)
{
	Uint16 flag;
	int x = 0, y = 0;
	auto rw = FileMap::getRWopsReadAll(filename);
	auto rwsize = SDL_RWsize(rw);
	// Lock the surface
	lock();
	while(SDL_RWtell(rw) < rwsize - 1) {
		flag = SDL_ReadLE16(rw);
		if (flag == 65535) {
			flag = SDL_ReadLE16(rw);
			for (int i = 0; i < flag * 2; ++i) { setPixelIterative(&x, &y, 0); }
		} else if (flag == 65534) {
			flag = SDL_ReadLE16(rw);
			for (int i = 0; i < flag * 2; ++i) { setPixelIterative(&x, &y, SDL_ReadU8(rw)); }
		}
	}
	// Unlock the surface
	unlock();
	SDL_RWclose(rw);
}

/**
 * Loads the contents of a TFTD BDY image file into
 * the surface. BDY files are compressed with a custom
 * algorithm.
 * @param filename Filename of the BDY image.
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#BDY
 */
void Surface::loadBdy(const std::string &filename)
{
	Uint8 dataByte;
	int pixelCnt;
	int x = 0, y = 0;
	int currentRow = 0;
	auto rw = FileMap::getRWopsReadAll(filename);
	auto rwsize = SDL_RWsize(rw);
	// Lock the surface
	lock();
	while (SDL_RWtell(rw) < rwsize) {
		dataByte = SDL_ReadU8(rw);
		if (dataByte >= 129)
		{
			pixelCnt = 257 - (int)dataByte;
			dataByte = SDL_ReadU8(rw);
			currentRow = y;
			for (int i = 0; i < pixelCnt; ++i)
			{
				setPixelIterative(&x, &y, dataByte);
				if (currentRow != y) // avoid overscan into next row
					break;
			}
		}
		else
		{
			pixelCnt = 1 + (int)dataByte;
			currentRow = y;
			for (int i = 0; i < pixelCnt; ++i)
			{
				dataByte = SDL_ReadU8(rw);
				if (currentRow == y) // avoid overscan into next row
					setPixelIterative(&x, &y, dataByte);
			}
		}
	}
	// Unlock the surface
	unlock();
	SDL_RWclose(rw);
}

/**
 * Clears the entire contents of the surface, resulting
 * in a blank image of the specified color. (0 for transparent)
 * @param color the colour for the background of the surface.
 */
void Surface::clear()
{
	CleanSdlSurface(_surface.get());
}

/**
 * Shifts all the colors in the surface by a set amount.
 * This is a common method in 8bpp games to simulate color
 * effects for cheap.
 * @param off Amount to shift.
 * @param min Minimum color to shift to.
 * @param max Maximum color to shift to.
 * @param mul Shift multiplier.
 */
void Surface::offset(int off, int min, int max, int mul)
{
	if (off == 0)
		return;

	// Lock the surface
	lock();

	for (int x = 0, y = 0; x < getWidth() && y < getHeight();)
	{
		Uint8 pixel = getPixel(x, y);
		int p;
		if (off > 0)
		{
			p = pixel * mul + off;
		}
		else
		{
			p = (pixel + off) / mul;
		}
		if (min != -1 && p < min)
		{
			p = min;
		}
		else if (max != -1 && p > max)
		{
			p = max;
		}

		if (pixel > 0)
		{
			setPixelIterative(&x, &y, p);
		}
		else
		{
			setPixelIterative(&x, &y, 0);
		}
	}

	// Unlock the surface
	unlock();
}

/**
 * Shifts all the colors in the surface by a set amount, but
 * keeping them inside a fixed-size color block chunk.
 * @param off Amount to shift.
 * @param blk Color block size.
 * @param mul Shift multiplier.
 */
void Surface::offsetBlock(int off, int blk, int mul)
{
	if (off == 0)
		return;

	// Lock the surface
	lock();

	for (int x = 0, y = 0; x < getWidth() && y < getHeight();)
	{
		Uint8 pixel = getPixel(x, y);
		int min = pixel / blk * blk;
		int max = min + blk;
		int p;
		if (off > 0)
		{
			p = pixel * mul + off;
		}
		else
		{
			p = (pixel + off) / mul;
		}
		if (min != -1 && p < min)
		{
			p = min;
		}
		else if (max != -1 && p > max)
		{
			p = max;
		}

		if (pixel > 0)
		{
			setPixelIterative(&x, &y, p);
		}
		else
		{
			setPixelIterative(&x, &y, 0);
		}
	}

	// Unlock the surface
	unlock();
}

/**
 * Inverts all the colors in the surface according to a middle point.
 * Used for effects like shifting a button between pressed and unpressed.
 * @param mid Middle point.
 */
void Surface::invert(Uint8 mid)
{
	// Lock the surface
	lock();

	for (int x = 0, y = 0; x < getWidth() && y < getHeight();)
	{
		Uint8 pixel = getPixel(x, y);
		if (pixel > 0)
		{
			setPixelIterative(&x, &y, pixel + 2 * ((int)mid - (int)pixel));
		}
		else
		{
			setPixelIterative(&x, &y, 0);
		}
	}

	// Unlock the surface
	unlock();
}

/**
 * Runs any code the surface needs to keep updating every
 * game cycle, like animations and other real-time elements.
 */
void Surface::think()
{

}

/**
 * Draws the graphic that the surface contains before it
 * gets blitted onto other surfaces. The surface is only
 * redrawn if the flag is set by a property change, to
 * avoid unnecessary drawing.
 */
void Surface::draw()
{
	_redraw = false;
	clear();
}

/**
 * Blits this surface onto another one, with its position
 * relative to the top-left corner of the target surface.
 * The cropping rectangle controls the portion of the surface
 * that is blitted.
 * @param surface Pointer to surface to blit onto.
 */
void Surface::blit(SDL_Surface *surface)
{
	if (_visible && !_hidden)
	{
		if (_redraw)
			draw();

		SDL_Rect target {};
		target.x = getX();
		target.y = getY();
		SDL_BlitSurface(_surface.get(), nullptr, surface, &target);
	}
}

/**
 * Copies the exact contents of another surface onto this one.
 * Only the content that would overlap both surfaces is copied, in
 * accordance with their positions. This is handy for applying
 * effects over another surface without modifying the original.
 * @param surface Pointer to surface to copy from.
 */
void Surface::copy(Surface *surface)
{
	/*
	SDL_BlitSurface uses colour matching,
	and is therefor unreliable as a means
	to copy the contents of one surface to another
	instead we have to do this manually

	SDL_Rect from;
	from.x = getX() - surface->getX();
	from.y = getY() - surface->getY();
	from.w = getWidth();
	from.h = getHeight();
	SDL_BlitSurface(surface->getSurface(), &from, _surface, 0);
	*/
	const int from_x = getX() - surface->getX();
	const int from_y = getY() - surface->getY();

	lock();

	ShaderDrawFunc(
		[](Uint8& dest, const Uint8& src)
		{
			dest = src;
		},
		ShaderMove<Uint8>(_surface.get(), from_x, from_y),
		ShaderMove<Uint8>(surface, 0, 0)
	);

	unlock();
}

/**
 * Draws a filled rectangle on the surface.
 * @param rect Pointer to Rect.
 * @param color Color of the rectangle.
 */
void Surface::drawRect(SDL_Rect *rect, Uint8 color)
{
	SDL_FillRect(_surface.get(), rect, color);
}

/**
 * Draws a filled rectangle on the surface.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 * @param w Width in pixels.
 * @param h Height in pixels.
 * @param color Color of the rectangle.
 */
void Surface::drawRect(Sint16 x, Sint16 y, Sint16 w, Sint16 h, Uint8 color)
{
	SDL_Rect rect;
	rect.w = w;
	rect.h = h;
	rect.x = x;
	rect.y = y;
	SDL_FillRect(_surface.get(), &rect, color);
}

/**
 * Draws a line on the surface.
 * @param x1 Start x coordinate in pixels.
 * @param y1 Start y coordinate in pixels.
 * @param x2 End x coordinate in pixels.
 * @param y2 End y coordinate in pixels.
 * @param color Color of the line.
 */
void Surface::drawLine(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 color)
{
	lineColor(_surface.get(), x1, y1, x2, y2, Palette::getRGBA(getPalette(), color));
}

/**
 * Draws a filled circle on the surface.
 * @param x X coordinate in pixels.
 * @param y Y coordinate in pixels.
 * @param r Radius in pixels.
 * @param color Color of the circle.
 */
void Surface::drawCircle(Sint16 x, Sint16 y, Sint16 r, Uint8 color)
{
	filledCircleColor(_surface.get(), x, y, r, Palette::getRGBA(getPalette(), color));
}

/**
 * Draws a filled polygon on the surface.
 * @param x Array of x coordinates.
 * @param y Array of y coordinates.
 * @param n Number of points.
 * @param color Color of the polygon.
 */
void Surface::drawPolygon(Sint16 *x, Sint16 *y, int n, Uint8 color)
{
	filledPolygonColor(_surface.get(), x, y, n, Palette::getRGBA(getPalette(), color));
}

/**
 * Draws a textured polygon on the surface.
 * @param x Array of x coordinates.
 * @param y Array of y coordinates.
 * @param n Number of points.
 * @param texture Texture for polygon.
 * @param dx X offset of texture relative to the screen.
 * @param dy Y offset of texture relative to the screen.
 */
void Surface::drawTexturedPolygon(Sint16 *x, Sint16 *y, int n, Surface *texture, int dx, int dy)
{
	texturedPolygon(_surface.get(), x, y, n, texture->getSurface(), dx, dy);
}

/**
 * Draws a text string on the surface.
 * @param x X coordinate in pixels.
 * @param y Y coordinate in pixels.
 * @param s Character string to draw.
 * @param color Color of string.
 */
void Surface::drawString(Sint16 x, Sint16 y, const char *s, Uint8 color)
{
	stringColor(_surface.get(), x, y, s, Palette::getRGBA(getPalette(), color));
}

/**
 * Changes the position of the surface in the X axis.
 * @param x X position in pixels.
 */
void Surface::setX(int x)
{
	_x = x;
}

/**
 * Changes the position of the surface in the Y axis.
 * @param y Y position in pixels.
 */
void Surface::setY(int y)
{
	_y = y;
}

/**
 * Changes the visibility of the surface. A hidden surface
 * isn't blitted nor receives events.
 * @param visible New visibility.
 */
void Surface::setVisible(bool visible)
{
	_visible = visible;
}

/**
 * Returns the visible state of the surface.
 * @return Current visibility.
 */
bool Surface::getVisible() const
{
	return _visible;
}

/**
 * Returns the cropping rectangle for this surface.
 * @return Pointer to the cropping rectangle.
 */
SurfaceCrop Surface::getCrop() const
{
	return SurfaceCrop{ this };
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void Surface::setPalette(const SDL_Color *colors, int firstcolor, int ncolors)
{
	if (_surface->format->BitsPerPixel == 8)
		SDL_SetColors(_surface.get(), const_cast<SDL_Color *>(colors), firstcolor, ncolors);
}

/**
 * This is a separate visibility setting intended
 * for temporary effects like window popups,
 * so as to not override the default visibility setting.
 * @note Do not confuse with setVisible!
 * @param hidden Shown or hidden.
 */
void Surface::setHidden(bool hidden)
{
	_hidden = hidden;
}

/**
 * Locks the surface from outside access
 * for pixel-level access. Must be unlocked
 * afterwards.
 * @sa unlock()
 */
void Surface::lock()
{
	SDL_LockSurface(_surface.get());
}

/**
 * Unlocks the surface after it's been locked
 * to resume blitting operations.
 * @sa lock()
 */
void Surface::unlock()
{
	SDL_UnlockSurface(_surface.get());
}

/**
 * Specific blit function to blit battlescape terrain data in different shades in a fast way.
 */
void Surface::blitRaw(SurfaceRaw<Uint8> destSurf, SurfaceRaw<const Uint8> srcSurf, int x, int y, int shade, bool half, int newBaseColor)
{
	ShaderMove<const Uint8> src(srcSurf, x, y);
	if (half)
	{
		GraphSubset g = src.getDomain();
		g.beg_x = g.end_x/2;
		src.setDomain(g);
	}
	if (newBaseColor)
	{
		--newBaseColor;
		newBaseColor <<= 4;
		ShaderDraw<helper::ColorReplace>(ShaderSurface(destSurf), src, ShaderScalar(shade), ShaderScalar(newBaseColor));
	}
	else
	{
		ShaderDraw<helper::StandardShade>(ShaderSurface(destSurf), src, ShaderScalar(shade));
	}
}

/**
 * Specific blit function to blit battlescape terrain data in different shades in a fast way.
 * Notice there is no surface locking here - you have to make sure you lock the surface yourself
 * at the start of blitting and unlock it when done.
 * @param surface to blit to
 * @param x
 * @param y
 * @param off
 * @param half some tiles are blitted only the right half
 * @param newBaseColor Attention: the actual color + 1, because 0 is no new base color.
 */
void Surface::blitNShade(SurfaceRaw<Uint8> surface, int x, int y, int shade, bool half, int newBaseColor) const
{
	blitRaw(surface, SurfaceRaw<const Uint8>(this), x, y, shade, half, newBaseColor);
}

/**
 * Specific blit function to blit battlescape terrain data in different shades in a fast way.
 * @param surface destination blit to
 * @param x
 * @param y
 * @param shade shade offset
 * @param range area that limit draw surface
 */
void Surface::blitNShade(SurfaceRaw<Uint8> surface, int x, int y, int shade, GraphSubset range) const
{
	ShaderMove<const Uint8> src(this, x, y);
	ShaderMove<Uint8> dest(surface);

	dest.setDomain(range);

	ShaderDraw<helper::StandardShade>(dest, src, ShaderScalar(shade));
}

/**
 * Set the surface to be redrawn.
 * @param valid true means redraw.
 */
void Surface::invalidate(bool valid)
{
	_redraw = valid;
}

/**
 * Recreates the surface with a new size.
 * Old contents will not be altered, and may be
 * cropped to fit the new size.
 * @param width Width in pixels.
 * @param height Height in pixels.
 */
void Surface::resize(int width, int height)
{
	// Set up new surface
	Uint8 bpp = _surface->format->BitsPerPixel;
	auto alignedBuffer = NewAlignedBuffer(bpp, width, height);
	auto surface = NewSdlSurface(alignedBuffer, bpp, width, height);

	// Copy old contents
	SDL_SetColorKey(surface.get(), SDL_SRCCOLORKEY, 0);
	SDL_SetColors(surface.get(), getPalette(), 0, 256);

	RawCopySurf(surface, _surface);

	// Delete old surface
	_surface = std::move(surface);
	_alignedBuffer = std::move(alignedBuffer);
	_width = _surface->w;
	_height = _surface->h;
	_pitch = _surface->pitch;
}

/**
 * Changes the width of the surface.
 * @warning This is not a trivial setter!
 * It will force the surface to be recreated for the new size.
 * @param width New width in pixels.
 */
void Surface::setWidth(int width)
{
	resize(width, getHeight());
	_redraw = true;
}

/**
 * Changes the height of the surface.
 * @warning This is not a trivial setter!
 * It will force the surface to be recreated for the new size.
 * @param height New height in pixels.
 */
void Surface::setHeight(int height)
{
	resize(getWidth(), height);
	_redraw = true;
}

/**
 * Blit surface with crop
 * @param dest
 */
void SurfaceCrop::blit(Surface* dest)
{
	if (_surface)
	{
		auto srcShader = ShaderCrop(*this, _x, _y);
		auto destShader = ShaderMove<Uint8>(dest, 0, 0);

		ShaderDrawFunc(
			[](Uint8& d, Uint8 s)
			{
				if (s) d = s;
			},
			destShader,
			srcShader
		);
	}
}

}
