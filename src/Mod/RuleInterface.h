#pragma once
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
#include <string>
#include <map>
#include "../Engine/Yaml.h"

namespace OpenXcom
{

class Mod;
class SavedGame;

struct Element
{
	/// basic rect info.
	int x = INT_MAX;
	int y = INT_MAX;
	int w = INT_MAX;
	int h = INT_MAX;

	/// 3 colors
	int color = INT_MAX;
	int color2 = INT_MAX;
	int border = INT_MAX;

	/// Custom value
	int custom = 0;

	/// defines inversion behaviour
	bool TFTDMode = false;
};

class RuleInterface
{
private:
	std::string _type;
	std::string _palette;
	std::string _parent;
	std::string _backgroundImage;
	std::string _altBackgroundImage;
	std::string _music;
	int _sound;
	std::vector< std::pair<std::string, std::string> > _upgBackgroundImage;

	std::map <std::string, Element> _elements;
public:
	/// Constructor.
	RuleInterface(const std::string & type);
	/// Destructor.
	~RuleInterface();
	/// Load from YAML.
	void load(const YAML::YamlNodeReader& reader, Mod *mod);
	/// Get an element. Can be `nullptr`.
	const Element *getElementOptional(const std::string &id) const;
	/// Get an element.
	const Element *getElement(const std::string &id) const;
	/// Get palette.
	const std::string &getPalette() const;
	/// Get parent interface rule.
	const std::string &getParent() const;
	/// Get background image.
	const std::string &getBackgroundImage(const Mod* mod, const SavedGame* save) const;
	/// Get alternative background image (for battlescape theme).
	const std::string &getAltBackgroundImage() const;
	/// Get music.
	const std::string &getMusic() const;
	/// Get sound.
	int getSound() const;
};

}
