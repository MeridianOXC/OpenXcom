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
#include "AlienInventory.h"
#include <cmath>
#include "../Engine/Action.h"
#include "../Engine/Font.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Screen.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"
#include "../Interface/Text.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/RuleInterface.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedGame.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

/**
 * Sets up an inventory with the specified size and position.
 * @param game Pointer to core game.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
AlienInventory::AlienInventory(Game *game, int width, int height, int x, int y) : InteractiveSurface(width, height, x, y), _game(game), _selUnit(0), _dynamicOffset(0), _animFrame(0)
{
	_grid = new Surface(width, height, 0, 0);
	_items = new Surface(width, height, 0, 0);

	_animTimer = new Timer(100);
	_animTimer->onTimer((SurfaceHandler)&AlienInventory::animate);
	_animTimer->start();
}

/**
 * Deletes inventory surfaces.
 */
AlienInventory::~AlienInventory()
{
	delete _grid;
	delete _items;
	delete _animTimer;
}

/**
 * Replaces a certain amount of colors in the inventory's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void AlienInventory::setPalette(const SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_grid->setPalette(colors, firstcolor, ncolors);
	_items->setPalette(colors, firstcolor, ncolors);
}

/**
 * Returns the currently selected (i.e. displayed) unit.
 * @return Pointer to selected unit, or 0 if none.
 */
BattleUnit *AlienInventory::getSelectedUnit() const
{
	return _selUnit;
}

/**
 * Changes the unit to display the inventory of.
 * @param unit Pointer to battle unit.
 */
void AlienInventory::setSelectedUnit(BattleUnit *unit)
{
	_selUnit = unit;
	_dynamicOffset = 0;
	if (unit && unit->isBigUnit())
	{
		_dynamicOffset = _game->getMod()->getAlienInventoryOffsetBigUnit();
	}
}

/**
 * Draws the inventory elements.
 */
void AlienInventory::draw()
{
	drawGrid();
	drawItems();
}

/**
 * Draws the inventory grid.
 */
void AlienInventory::drawGrid()
{
	_grid->clear();
	RuleInterface *rule = _game->getMod()->getInterface("inventory");
	Uint8 color = rule->getElement("grid")->color;

	for (std::map<std::string, RuleInventory*>::iterator i = _game->getMod()->getInventories()->begin(); i != _game->getMod()->getInventories()->end(); ++i)
	{
		if (i->second->getType() == INV_HAND)
		{
			SDL_Rect r;
			r.x = i->second->getX();
			r.x += _game->getMod()->getAlienInventoryOffsetX();

			if (i->second->isRightHand())
				r.x -= _dynamicOffset;
			else if (i->second->isLeftHand())
				r.x += _dynamicOffset;

			r.y = i->second->getY();
			r.w = RuleInventory::HAND_W * RuleInventory::SLOT_W;
			r.h = RuleInventory::HAND_H * RuleInventory::SLOT_H;
			_grid->drawRect(&r, color);
			r.x++;
			r.y++;
			r.w -= 2;
			r.h -= 2;
			_grid->drawRect(&r, 0);
		}
		else
		{
			continue;
		}
	}
}

/**
 * Draws the items contained in the alien's hands.
 */
void AlienInventory::drawItems()
{
	const SavedBattleGame* save = _game->getSavedGame()->getSavedBattle();
	ScriptWorkerBlit work;
	_items->clear();
	if (_selUnit != 0)
	{
		SurfaceSet *texture = _game->getMod()->getSurfaceSet("BIGOBS.PCK");
		for (std::vector<BattleItem*>::iterator i = _selUnit->getInventory()->begin(); i != _selUnit->getInventory()->end(); ++i)
		{
			if ((*i)->getSlot()->getType() == INV_HAND)
			{
				const Surface* frame = (*i)->getBigSprite(texture, save, _animFrame);

				if (!frame)
					continue;

				int x = (*i)->getSlot()->getX() + (*i)->getRules()->getHandSpriteOffX();
				x += _game->getMod()->getAlienInventoryOffsetX();

				if ((*i)->getSlot()->isRightHand())
					x -= _dynamicOffset;
				else if ((*i)->getSlot()->isLeftHand())
					x += _dynamicOffset;

				int y = (*i)->getSlot()->getY() + (*i)->getRules()->getHandSpriteOffY();

				BattleItem::ScriptFill(&work, *i, save, BODYPART_ITEM_INVENTORY, _animFrame, 0);
				work.executeBlit(frame, _items, x, y, 0);
			}
			else
			{
				continue;
			}
		}
	}
}

/**
 * Gets the inventory slot located in the specified mouse position.
 * @param x Mouse X position. Returns the slot's X position.
 * @param y Mouse Y position. Returns the slot's Y position.
 * @return Slot rules, or NULL if none.
 */
RuleInventory *AlienInventory::getSlotInPosition(int *x, int *y) const
{
	for (std::map<std::string, RuleInventory*>::iterator i = _game->getMod()->getInventories()->begin(); i != _game->getMod()->getInventories()->end(); ++i)
	{
		if (i->second->checkSlotInPosition(x, y))
		{
			return i->second;
		}
	}
	return 0;
}

/**
 * Blits the inventory elements.
 * @param surface Pointer to surface to blit onto.
 */
void AlienInventory::blit(SDL_Surface *surface)
{
	clear();
	_grid->blitNShade(this, 0, 0);
	_items->blitNShade(this, 0, 0);
	Surface::blit(surface);
}

/**
 * Item info.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void AlienInventory::mouseClick(Action *action, State *state)
{
	if (_game->isLeftClick(action))
	{
		_game->popState();
	}
	else if (_game->isRightClick(action))
	{
		_game->popState();
	}
	else if (_game->isMiddleClick(action))
	{
		if (_selUnit == 0)
			return;

		int x = (int)floor(action->getAbsoluteXMouse()) - getX(),
			y = (int)floor(action->getAbsoluteYMouse()) - getY();

		if (x >= Screen::ORIGINAL_WIDTH / 2)
			x -= _dynamicOffset;
		else
			x += _dynamicOffset;

		x -= _game->getMod()->getAlienInventoryOffsetX();

		RuleInventory *slot = getSlotInPosition(&x, &y);
		if (slot != 0)
		{
			if (slot->getType() == INV_HAND)
			{
				BattleItem *item = _selUnit->getItem(slot, x, y);
				if (item != 0)
				{
					std::string articleId = item->getRules()->getType();
					Ufopaedia::openArticle(_game, articleId);
				}
			}
		}
	}
	InteractiveSurface::mouseClick(action, state);
}

/**
 * Handles timers.
 */
void AlienInventory::think()
{
	_animTimer->think(0, this);
}

/**
 * Animate surface.
 */
void AlienInventory::animate()
{
	_animFrame++;

	drawItems();
}

}
