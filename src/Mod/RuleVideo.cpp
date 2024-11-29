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
#include "RuleVideo.h"
#include <climits>
#include "../Engine/Screen.h"

namespace OpenXcom
{

RuleVideo::RuleVideo(const std::string &id) : _id(id), _useUfoAudioSequence(false), _winGame(false), _loseGame(false)
{
	// backwards-compatibility failsafe
	if (_id == "winGame")
		_winGame = true;
	if (_id == "loseGame")
		_loseGame = true;
}

RuleVideo::~RuleVideo()
{
}

static void _loadSlide(SlideshowSlide &slide, const YAML::YamlNodeReader& reader)
{
	slide.imagePath = reader["imagePath"].readVal<std::string>("");
	slide.caption = reader["caption"].readVal<std::string>("");

	std::pair<int, int> size = reader["captionSize"].readVal<std::pair<int, int> >(
		std::pair<int, int>(Screen::ORIGINAL_WIDTH, Screen::ORIGINAL_HEIGHT));
	slide.w = size.first;
	slide.h = size.second;

	std::pair<int, int> pos = reader["captionPos"].readVal<std::pair<int, int> >(std::pair<int, int>(0, 0));
	slide.x = pos.first;
	slide.y = pos.second;

	slide.color = reader["captionColor"].readVal<int>(INT_MAX);
	slide.transitionSeconds = reader["transitionSeconds"].readVal<int>(0);
	slide.align = reader["captionAlign"].readVal(ALIGN_LEFT);
	slide.valign = reader["captionVerticalAlign"].readVal(ALIGN_TOP);
}

void RuleVideo::load(const YAML::YamlNodeReader& reader)
{
	_useUfoAudioSequence = reader["useUfoAudioSequence"].readVal(false);
	reader.tryRead("winGame", _winGame);
	reader.tryRead("loseGame", _loseGame);

	for (const auto& video : reader["videos"].children())
		_videos.push_back(video.readVal<std::string>());

	for (const auto& track : reader["audioTracks"].children())
		_audioTracks.push_back(track.readVal<std::string>());

	if (const auto& slideshow = reader["slideshow"])
	{
		_slideshowHeader.musicId = slideshow["musicId"].readVal<std::string>("");
		_slideshowHeader.transitionSeconds = slideshow["transitionSeconds"].readVal<int>(30);

		for (const auto& slideReader : slideshow["slides"].children())
		{
			SlideshowSlide slide;
			_loadSlide(slide, slideReader);
			_slides.push_back(slide);
		}
	}
}

bool RuleVideo::useUfoAudioSequence() const
{
	return _useUfoAudioSequence;
}

const std::vector<std::string> * RuleVideo::getVideos() const
{
	return &_videos;
}

const SlideshowHeader & RuleVideo::getSlideshowHeader() const
{
	return _slideshowHeader;
}

const std::vector<SlideshowSlide> * RuleVideo::getSlides() const
{
	return &_slides;
}

const std::vector<std::string> * RuleVideo::getAudioTracks() const
{
	return &_audioTracks;
}

}
