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
#define _USE_MATH_DEFINES
#include "ItemSprite.h"
#include "../Mod/Mod.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedBattleGame.h"

namespace OpenXcom
{

/**
 * Sets up a ItemSprite with the specified size and position.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
ItemSprite::ItemSprite(Surface* dest, const Mod* mod, const SavedBattleGame* save, int frame) :
	_itemSurface(const_cast<Mod*>(mod)->getSurfaceSet("FLOOROB.PCK")),
	_animationFrame(frame),
	_dest(dest),
	_save(save)
{

}

/**
 * Deletes the ItemSprite.
 */
ItemSprite::~ItemSprite()
{

}

/**
 * Draws a item, using the drawing rules of the item or unit if it's corpse.
 * This function is called by Map, for each item on the screen.
 */
void ItemSprite::draw(const BattleItem* item, int x, int y, int shade)
{
	const Surface* sprite = item->getFloorSprite(_itemSurface, _save, _animationFrame, shade);
	if (sprite)
	{
		ScriptWorkerBlit work;
		BattleItem::ScriptFill(&work, item, _save, BODYPART_ITEM_FLOOR, _animationFrame, shade);
		work.executeBlit(sprite, _dest, x, y, shade);
	}
}

/**
 * Draws shadow of item.
 */
void ItemSprite::drawShadow(const BattleItem* item, int x, int y)
{
	const Surface* sprite = item->getFloorSprite(_itemSurface, _save, _animationFrame, 16);
	if (sprite)
	{
		sprite->blitNShade(_dest, x, y, 16);
	}
}

} //namespace OpenXcom
