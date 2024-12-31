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
#include "Font.h"
#include "DosFont.h"
#include "Surface.h"
#include "FileMap.h"
#include "Unicode.h"

namespace OpenXcom
{

const SDL_Color Font::TerminalColors[2] = {{0, 0, 0, 0}, {185, 185, 185, 255}};

/**
 * Initializes the font with a blank surface.
 */
Font::Font() : _monospace(false)
{
}

/**
 * Deletes the font's surface.
 */
Font::~Font()
{
	for (auto& fontImage : _images)
	{
		delete fontImage.surface;
	}
}

/**
 * Loads the font from a YAML file.
 * @param node YAML node.
 */
void Font::load(const YAML::YamlNodeReader& reader)
{
	int width = reader["width"].readVal(0);
	int height = reader["height"].readVal(0);
	int spacing = reader["spacing"].readVal(0);
	_monospace = reader["monospace"].readVal(_monospace);
	for (const auto& imageReader : reader["images"].children())
	{
		FontImage image;
		image.width = imageReader["width"].readVal(width);
		image.height = imageReader["height"].readVal(height);
		image.spacing = imageReader["spacing"].readVal(spacing);
		std::string file = "Language/" + imageReader["file"].readVal<std::string>();
		UString chars = Unicode::convUtf8ToUtf32(imageReader["chars"].readVal<std::string>());
		image.surface = new Surface(image.width, image.height);
		image.surface->loadImage(file);
		_images.push_back(image);
		init(_images.size() - 1, chars);
	}
}

/**
 * Generates a pre-defined Codepage 437 (MS-DOS terminal) font.
 */
void Font::loadTerminal()
{
	FontImage image;
	image.width = 9;
	image.height = 16;
	image.spacing = 0;
	_monospace = true;

	SDL_RWops *rw = SDL_RWFromConstMem(dosFont, DOSFONT_SIZE);
	SDL_Surface *s = SDL_LoadBMP_RW(rw, SDL_TRUE);
	image.surface = new Surface(s->w, s->h);
	image.surface->setPalette(TerminalColors, 0, std::size(TerminalColors));
	SDL_BlitSurface(s, 0, image.surface->getSurface(), 0);
	SDL_FreeSurface(s);
	_images.push_back(image);

	UString chars = Unicode::convUtf8ToUtf32(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");
	init(_images.size() - 1, chars);
}


/**
 * Calculates the real size and position of each character in
 * the surface and stores them in SDL_Rect's for future use
 * by other classes.
 * @param index The index of the surface to use.
 * @param str A string of characters to map to the surface.
 */
void Font::init(size_t index, const UString &str)
{
	FontImage *image = &_images[index];
	Surface *surface = image->surface;
	surface->lock();
	int length = (surface->getWidth() / image->width);

	_chars.reserve(_chars.size() + str.size());

	if (_monospace)
	{
		for (size_t i = 0; i < str.length(); ++i)
		{
			SDL_Rect rect;
			int startX = i % length * image->width;
			int startY = i / length * image->height;
			rect.x = startX;
			rect.y = startY;
			rect.w = image->width;
			rect.h = image->height;
			_chars[str[i]] = std::make_pair(index, rect);
		}
	}
	else
	{
		for (size_t i = 0; i < str.length(); ++i)
		{
			SDL_Rect rect;
			int left = -1, right = -1;
			int startX = i % length * image->width;
			int startY = i / length * image->height;
			for (int x = startX; x < startX + image->width; ++x)
			{
				for (int y = startY; y < startY + image->height && left == -1; ++y)
				{
					Uint8 pixel = surface->getPixel(x, y);
					if (pixel != 0)
					{
						left = x;
					}
				}
			}
			for (int x = startX + image->width - 1; x >= startX; --x)
			{
				for (int y = startY + image->height; y-- != startY && right == -1;)
				{
					Uint8 pixel = surface->getPixel(x, y);
					if (pixel != 0)
					{
						right = x;
					}
				}
			}
			rect.x = left;
			rect.y = startY;
			rect.w = right - left + 1;
			rect.h = image->height;

			_chars[str[i]] = std::make_pair(index, rect);
		}
	}
	surface->unlock();
}

/**
 * Returns a particular character from the set stored in the font.
 * @param c Character to use for size/position.
 * @return Pointer to the font's surface with the respective
 * cropping rectangle set up.
 */
SurfaceCrop Font::getChar(UCode c) const
{
	auto f = _chars.find(c);
	if (f == _chars.end())
		f = _chars.find('?');
	auto surfaceCrop = _images[f->second.first].surface->getCrop();
	*surfaceCrop.getCrop() = f->second.second;
	return surfaceCrop;
}

/**
 * Returns the maximum width for any character in the font.
 * @return Width in pixels.
 */
int Font::getWidth() const
{
	return _images[0].width;
}

/**
 * Returns the maximum height for any character in the font.
 * @return Height in pixels.
 */
int Font::getHeight() const
{
	return _images[0].height;
}

/**
 * Returns the spacing between any character in the font.
 * @return Spacing in pixels.
 * @note This does not refer to character spacing within the surface,
 * but to the spacing used between successive characters in a line.
 */
int Font::getSpacing() const
{
	return _images[0].spacing;
}

/**
 * Returns the dimensions of a particular character in the font.
 * @param c Font character.
 * @return Width and Height dimensions (X and Y are ignored).
 */
SDL_Rect Font::getCharSize(UCode c) const
{
	SDL_Rect size = { 0, 0, 0, 0 };
	if (Unicode::isPrintable(c))
	{
		auto f = _chars.find(c);
		if (f == _chars.end())
			f = _chars.find('?');

		const FontImage *image = &_images[f->second.first];
		size.w = f->second.second.w + image->spacing;
		size.h = f->second.second.h + image->spacing;
	}
	else
	{
		if (_monospace)
			size.w = getWidth() + getSpacing();
		else if (c == Unicode::TOK_NBSP)
			size.w = getWidth() / 4;
		else if (c == '\t')
			size.w = getWidth() * 3 / 4;
		else
			size.w = getWidth() / 2;
		size.h = getHeight() + getSpacing();
	}
	// In case anyone mixes them up
	size.x = size.w;
	size.y = size.h;
	return size;
}

}
