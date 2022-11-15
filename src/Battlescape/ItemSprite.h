#pragma once
/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#include "../Engine/Surface.h"

namespace OpenXcom
{

class BattleUnit;
class BattleItem;
class SavedBattleGame;
class SurfaceSet;
class Mod;

/**
 * A class that renders a specific unit, given its render rules
 * combining the right frames from the surfaceset.
 */
class ItemSprite
{
private:
	const SurfaceSet *_itemSurface;
	int _animationFrame;
	Surface *_dest;
	const SavedBattleGame *_save;


public:
	/// Creates a new ItemSprite at the specified position and size.
	ItemSprite(Surface* dest, const Mod* mod, const SavedBattleGame *_save, int frame);
	/// Cleans up the ItemSprite.
	~ItemSprite();
	/// Draws the item.
	void draw(const BattleItem* item, int x, int y, int shade);
	/// Draws the item shadow.
	void drawShadow(const BattleItem* item, int x, int y);
};

} //namespace OpenXcom
