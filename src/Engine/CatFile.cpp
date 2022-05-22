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

#include "CatFile.h"
#include "Logger.h"
#include "FileMap.h"
#include "SDL2Helpers.h"

namespace OpenXcom
{

/**
 * Creates a CAT file stream. A CAT file starts with an index of the
 * offset and size of every file contained within. Each file consists
 * of a filename followed by its contents.
 * @param rw SDL_RWops of the CAT file.
 */
CatFile::CatFile(const std::string& filename) : _data(0), _items()
{
	// Get amount of files

	/* File format is: {offset_le32, size_le32}[N] ;  bytes[]
	 * the first offset effectively determines the offset/size table size
	 * which is N = offset/8 units.
	 *
	 * Size may or may not include the name "chunk" size, or there might
	 * not be such a chunk. True item size has to be calculated based
	 * on offsets only.
	 *
	 * The "name" chunk format is { int8_t size, char[size] name }
	 *
	 * 		SOUND/INTRO.CAT  : records do not contain names : offset+size = next offset or EOF
	 *		SOUND/RINTRO.CAT
	 * 		SOUND/ROLAND.CAT
	 * 		SOUND/AINTRO.CAT
	 *		SOUND/ADLIB.CAT
	 *
	 * 		SOUND/SOUND.CAT  : size does include name : offset+size = next offset or EOF
	 * 		SOUND/SOUND1.CAT
	 * 		SOUND/SOUND2.CAT
	 *
	 * 		SOUND/GM.CAT     : size does not include name : offset+size+namelen+1 = next offset or EOF
	 * 		SOUND/SAMPLE.CAT
	 * 		SOUND/SAMPLE2.CAT
	 * 		SOUND/SAMPLE2.CAT
	 *		TFTD/SOUND/SAMPLE.CAT
	 *
	 * Given an rwops object, this reads it and sets up rwops instances
	 * for each chunk.
	 *
	 * The original rwops object is consumed here.
	 */
	_filename = filename;
	auto rwops = FileMap::getRWops(filename);
	auto offset0 = SDL_ReadLE32(rwops); // read the first offset; 8 is the sizeof(item) of the header
	SDL_RWseek(rwops, 0, RW_SEEK_SET);  // reset the rwops pointer back
	size_t filesize;
	_data = (Uint8 *)SDL_LoadFile_RW(rwops, &filesize, SDL_FALSE); // read all of the file
	SDL_RWseek(rwops, 0, RW_SEEK_SET);  // and again reset the rwops pointer back

	if (offset0 >= filesize) {
		XComLog(LOG_WARNING) << "Catfile(" << filename << "): first offset " << offset0 << ">= file size " << filesize << ", not parsing.";
		SDL_RWclose(rwops);
		return;
	}
	for (Uint32 i = 0; i < offset0 / 8; ++i) {
		auto offset = SDL_ReadLE32(rwops);
		SDL_ReadLE32(rwops); // ignore size;
		// reject bad data
		if (offset >= filesize) {
			XComLog(LOG_WARNING) << "Catfile("<<filename<<"): item "<<i<<" outside of the file: offset="<<offset<<" "<<" filesize="<<filesize;
			continue;
		}
		_items.push_back(std::make_tuple(_data + offset, offset));
	}
	Uint32 last_offset = filesize;
	for ( auto it = _items.rbegin(); it != _items.rend(); ++it) {
		auto this_offset = std::get<1>(*it);
		std::get<1>(*it) = last_offset - this_offset;
		last_offset = this_offset;
	}
	SDL_RWclose(rwops);
}

/**
 * Frees associated memory.
 */
CatFile::~CatFile()
{
	if (_data) { SDL_free(_data); }
}

/**
 * Creates and returns an rwops for an item.
 * @param i Object number to load.
 * @param keepname Don't strip internal file name (needed for adlib).
 * @return SDL_RWops for the object data.
 */
SDL_RWops *CatFile::getRWops(Uint32 i) {
	if (i >= _items.size()) {
		XComLog(LOG_ERROR) << "Catfile<" << _filename << ">::getRWops("<<i<<"): >= size " << _items.size();
		return NULL;
	}
	return SDL_RWFromConstMem(std::get<0>(_items[i]), std::get<1>(_items[i]));
}

}
