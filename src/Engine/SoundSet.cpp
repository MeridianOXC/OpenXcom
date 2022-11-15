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
#include "SoundSet.h"
#include "CatFile.h"
#include "Sound.h"
#include "Logger.h"
#include "SDL2Helpers.h"
#include <climits>
#include <cassert>

namespace OpenXcom
{

/**
 * Sets up a new empty sound set.
 */
SoundSet::SoundSet() : _sharedSounds(INT_MAX)
{

}

/**
 * Converts a 8Khz sample to 11Khz.
 * @param oldsound Pointer to original sample buffer.
 * @param oldsize Original buffer size.
 * @param newsound Pointer to converted sample buffer.
 * @return Converted buffer size.
 */
int SoundSet::convertSampleRate(Uint8 *oldsound, size_t oldsize, Uint8 *newsound) const
{
	const Uint32 step16 = (8000 << 16) / 11025;
	int newsize = 0;
	for (Uint32 offset16 = 0; (offset16 >> 16) < oldsize; offset16 += step16, ++newsound, ++newsize)
	{
		*newsound = oldsound[offset16 >> 16];
	}
	return newsize;
}

static const Uint8 header[] = {  'R',  'I',  'F',  'F', 0x00, 0x00, 0x00, 0x00,  'W',  'A',  'V',  'E',
								 'f',  'm',  't',  ' ', 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
								0x11, 0x2b, 0x00, 0x00, 0x11, 0x2b, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00,
								 'd',  'a',  't',  'a', 0x00, 0x00, 0x00, 0x00                           };
/**
 * Write out a WAV. Resample if needed.
 * @param dest where to write
 * @param sound sound data
 * @param size  size of sound data
 * @param resample if resampling is needed.
 */
void SoundSet::writeWAV(SDL_RWops *dest, Uint8 *sound, size_t size, bool resample) const {
	SDL_RWwrite(dest, header, sizeof(header), 1);
	int newsize = size;

	if (resample) {
		auto newsound = SDL_malloc(2*size);
		newsize = convertSampleRate(sound, size, (Uint8 *)newsound);
		SDL_RWwrite(dest, newsound, newsize, 1);
		SDL_free(newsound);
	} else {
		SDL_RWwrite(dest, sound, size, 1);
	}

	// update the header
	SDL_RWseek(dest, 4, RW_SEEK_SET);	// write WAVE chunk size
	SDL_WriteLE32(dest, newsize + 36);
	SDL_RWseek(dest, 40, RW_SEEK_SET); 	// write data subchunk size
	SDL_WriteLE32(dest, newsize);
}

/**
 * Loads the contents of an X-Com CAT file which usually contains
 * a set of sound files. The CAT starts with an index of the offset
 * and size of every file contained within. Each file consists of a
 * filename followed by its contents.
 * @param rw RWops of the CAT set.
 * @param wav Are the sounds in WAV format?
 * @sa http://www.ufopaedia.org/index.php?title=SOUND
 */
void SoundSet::loadCat(CatFile &catFile)
{
	for (size_t i = 0; i < catFile.size(); ++i) { loadCatByIndex(catFile, i); }
}

/**
 * Returns a particular wave from the sound set.
 * @param i Sound number in the set.
 * @return Pointer to the respective sound.
 */
Sound *SoundSet::getSound(int i)
{
	if (_sounds.find(i) != _sounds.end())
	{
		return &_sounds[i];
	}
	return 0;
}

/**
 * Creates and returns a particular wave in the sound set.
 * @param i Sound number in the set.
 * @return Pointer to the respective sound.
 */
Sound *SoundSet::addSound(int i)
{
	assert(i >= 0 && "Negative indexes are not supported in SoundSet");
	_sounds[i] = Sound();
	return &_sounds[i];
}

/**
 * Set number of shared sound indexes that are accessible for all mods.
 */
void SoundSet::setMaxSharedSounds(int i)
{
	if (i >= 0)
	{
		_sharedSounds = i;
	}
	else
	{
		_sharedSounds = 0;
	}
}

/**
 * Gets number of shared sound indexes that are accessible for all mods.
 */
int SoundSet::getMaxSharedSounds() const
{
	return _sharedSounds;
}

/**
 * Returns the total amount of sounds currently
 * stored in the set.
 * @return Number of sounds.
 */
size_t SoundSet::getTotalSounds() const
{
	return _sounds.size();
}

/**
 * Loads individual contents of a sound CAT file by index.
 * a set of sound files. The CAT starts with an index of the offset
 * and size of every file contained within. Each file consists of a
 * filename followed by its contents.
 * @param filename Filename of the CAT set.
 * @param index which index in the cat file do we load?
 * @param tftd if to expect signed 8bit 11Khz instead of unsigned 6bit 8KHz in the data.
 *             and also under which ID to put the sound
 * @sa http://www.ufopaedia.org/index.php?title=SOUND
 */
void SoundSet::loadCatByIndex(CatFile &catFile, int index, bool tftd)
{
	int set_index = tftd ? getTotalSounds() : index;
	_sounds[set_index] = Sound(); // in case everything else fails, an empty Sound.
	auto rwops = catFile.getRWops(index);
	if (!rwops) {
		Log(LOG_VERBOSE) << "SoundSet::loadCatByIndex(" << catFile.fileName() << ", " << index << "): got NULL.";
		return;
	}
	int namesize = SDL_ReadU8(rwops);
	SDL_RWseek(rwops, namesize, RW_SEEK_CUR); // skip "name".
	// NB: the original code after ce548c29d5742e26a442a44ef2a5fcce3f80dace
	// did not adjust the size for the namesize byte when skipping the name
	// and thus submitted one trailing byte of garbage.
	// The original code before that commit did not adjust the size at all
	// when skipping the name and thus submitted at least one trailing byte of garbage.
	// v1.4 sounds do miss namesize+1 bytes as they are in the catfile -
	// comparing what's in the WAV header to cat item size without name.

	size_t size;
	Uint8 *sound = (Uint8 *)SDL_LoadFile_RW(rwops, &size, SDL_TRUE);

	// Skip short data
	if (size < 12) {
		Log(LOG_VERBOSE) << "SoundSet::loadCatByIndex(" << catFile.fileName() << ", " << index << ") size=" << size <<" , skipping.";
		SDL_free(sound);
		return;
	}

	// See if we've got RIFF header here.
	bool wav = ((sound[0] == 'R') && (sound[1] == 'I') && (sound[2]  == 'F') && (sound[3]  == 'F')
			 && (sound[8] == 'W') && (sound[9] == 'A') && (sound[10] == 'V') && (sound[11] == 'E'));

	Uint8 *samples;
	size_t samplecount;
	bool do_resample = true;
	if (wav) { // skip WAV header
		int expected_size = *(Sint32 *)(sound +0x04) + 8;
		int delta = ((int)size) - expected_size;
		// fix the header if we miss some data.
		if (delta < 0) {
			*(Sint32 *)(sound +0x04) += delta; // WAVE chunk size
			*(Sint32 *)(sound +0x28) += delta; // data chunk size
		}
		int samplerate = *(Sint32 *)(sound + 0x18);
		do_resample  = (samplerate < 11025);
		samples = sound + 44;
		samplecount = size - 44;
	} else { // skip DOS header
		// UFO2000 style
		samples = sound + 6;
		samplecount = size - 6;

		// OpenXcom style
		samples = sound + 5;
		samplecount = size - 6;

		// scale to 8 bits (UFO) or get rid of signedness (TFTD)
		for (size_t n = 0; n < samplecount; ++n) {
			int sample = samples[n];
			samples[n] = (Uint8) (tftd ? sample + 128 : sample * 4);
		}
	}
	size_t dest_size = 44 + 2 * size; // worst-case estimation
	auto dest_mem = SDL_malloc(dest_size);
	auto dest_rwops = SDL_RWFromMem(dest_mem, dest_size);

	if (do_resample) {
		writeWAV(dest_rwops, samples, samplecount, !tftd);
	} else { // nothing to do.
		SDL_RWwrite(dest_rwops, sound, size, 1);
	}
	SDL_RWseek(dest_rwops, 0, RW_SEEK_SET);
	_sounds[set_index].load(dest_rwops);  // this frees the dest_rwops
	SDL_free(dest_mem);
	SDL_free(sound);
}

}
