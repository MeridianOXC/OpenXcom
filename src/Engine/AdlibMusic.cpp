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
#include "AdlibMusic.h"
#include <algorithm>
#include "Options.h"
#include "Logger.h"
#include "Game.h"
#include "SDL2Helpers.h"
#include "FileMap.h"
#include "Adlib/fmopl.h"
#include "Adlib/adlplayer.h"

extern FM_OPL* opl[2];

namespace OpenXcom
{

int AdlibMusic::delay = 0;
int AdlibMusic::rate = 0;
std::map<int, int> AdlibMusic::delayRates;

/**
 * Initializes a new music track.
 * @param volume Music volume modifier (1.0 = 100%).
 */
AdlibMusic::AdlibMusic(float volume) : Music(), _data(0), _size(0), _volume(volume)
{
	rate = Options::audioSampleRate;
	if (!opl[0])
	{
		opl[0] = OPLCreate(OPL_TYPE_YM3812, 3579545, rate);
	}
	if (!opl[1])
	{
		opl[1] = OPLCreate(OPL_TYPE_YM3812, 3579545, rate);
	}
	// magic value - length of 1 tick per sample rate
	if (delayRates.empty())
	{
		delayRates[8000] = 114 * 4;
		delayRates[11025] = 157 * 4;
		delayRates[16000] = 228 * 4;
		delayRates[22050] = 314 * 4;
		delayRates[32000] = 456 * 4;
		delayRates[44100] = 629 * 4;
		delayRates[48000] = 685 * 4;
	}
}

/**
 * Deletes the loaded music content.
 */
AdlibMusic::~AdlibMusic()
{
	if (opl[0])
	{
		stop();
		OPLDestroy(opl[0]);
		opl[0] = 0;
	}
	if (opl[1])
	{
		OPLDestroy(opl[1]);
		opl[1] = 0;
	}
	if (_data)
	{
		SDL_free(_data);
	}
}

/**
 * Loads a music file from a specified filename.
 * @param filename Filename of the music file.
 */
void AdlibMusic::load(const std::string &filename)
{
	load(FileMap::getRWops(filename));
}

/**
 * Loads a music file from a specified rwops.
 * @param rwops rwops of the music data.
 */
void AdlibMusic::load(SDL_RWops *rwops)
{
	_data = (char *)SDL_LoadFile_RW(rwops, &_size, SDL_TRUE);
}

/**
 * Plays the contained music track.
 * @param loop Amount of times to loop the track. -1 = infinite
 */
void AdlibMusic::play(int) const
{
#ifndef __NO_MUSIC
	if (!Options::mute)
	{
		stop();
		func_setup_music((unsigned char*)_data, _size);
		func_set_music_volume(127 * _volume);
		Mix_HookMusic(player, (void*)this);
	}
#endif
}

/**
 * Custom audio player.
 * @param udata User data to send to the player.
 * @param stream Raw audio to output.
 * @param len Length of audio to output.
 */
void AdlibMusic::player(void *udata, Uint8 *stream, int len)
{
#ifndef __NO_MUSIC
	// Check SDL volume for Background Mute functionality
	if (Options::musicVolume == 0 || Mix_VolumeMusic(-1) == 0)
		return;
	if (Options::musicAlwaysLoop && !func_is_music_playing())
	{
		AdlibMusic *music = (AdlibMusic*)udata;
		music->play();
		return;
	}
	while (len != 0)
	{
		if (!opl[0] || !opl[1])
			return;
		int i = std::min(delay, len);
		if (i)
		{
			float volume = Game::volumeExponent(Options::musicVolume);
			YM3812UpdateOne(opl[0], (INT16*)stream, i / 2, 2, volume);
			YM3812UpdateOne(opl[1], ((INT16*)stream) + 1, i / 2, 2, volume);
			stream += i;
			delay -= i;
			len -= i;
		}
		if (!len)
			return;
		func_play_tick();

		delay = delayRates[rate];
	}
#endif
}

bool AdlibMusic::isPlaying()
{
#ifndef __NO_MUSIC
	if (!Options::mute)
	{
		return func_is_music_playing();
	}
#endif
	return false;
}

}
