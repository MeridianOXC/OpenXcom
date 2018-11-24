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
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/SurfaceSet.h"
#include "../Interface/Text.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/RuleInterface.h"
#include "../Savegame/BattleUnit.h"
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
AlienInventory::AlienInventory(Game *game, int width, int height, int x, int y) : InteractiveSurface(width, height, x, y), _game(game), _selUnit(0), _dynamicOffset(0)
{
	_grid = new Surface(width, height, 0, 0);
	_items = new Surface(width, height, 0, 0);
}

/**
 * Deletes inventory surfaces.
 */
AlienInventory::~AlienInventory()
{
	delete _grid;
	delete _items;
}

/**
 * Replaces a certain amount of colors in the inventory's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void AlienInventory::setPalette(SDL_Color *colors, int firstcolor, int ncolors)
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
	if (unit && unit->getArmor()->getSize() > 1)
	{
		_dynamicOffset = 32;
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

	if (_selUnit != 0) for (std::vector<std::string>::const_iterator i = _selUnit->getArmor()->getInventorySlots().cbegin(); i != _selUnit->getArmor()->getInventorySlots().cend(); i++)
	{
		RuleInventory *ruleI = (*_game->getMod()->getInventories())[*i];
		if (ruleI->getType() == INV_HAND)
		{
			SDL_Rect r;
			r.x = ruleI->getX();
			r.x += ALIEN_INVENTORY_STATIC_OFFSET;

			if (ruleI->getId() == _selUnit->getArmor()->getDefaultInventoryMap("STR_RIGHT_HAND"))
				r.x -= _dynamicOffset;
			else if (ruleI->getId() == _selUnit->getArmor()->getDefaultInventoryMap("STR_LEFT_HAND"))
				r.x += _dynamicOffset;

			r.y = ruleI->getY();
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
	_items->clear();
	if (_selUnit != 0)
	{
		SurfaceSet *texture = _game->getMod()->getSurfaceSet("BIGOBS.PCK");
		for (std::vector<BattleItem*>::iterator i = _selUnit->getInventory()->begin(); i != _selUnit->getInventory()->end(); ++i)
		{
			if ((*i)->getSlot()->getType() == INV_HAND)
			{
				Surface *frame = texture->getFrame((*i)->getRules()->getBigSprite());
				int x = (*i)->getSlot()->getX() + (*i)->getRules()->getHandSpriteOffX();
				x += ALIEN_INVENTORY_STATIC_OFFSET;

				if ((*i)->getSlot()->getId() == _selUnit->getArmor()->getDefaultInventoryMap("STR_RIGHT_HAND"))
					x -= _dynamicOffset;
				else if ((*i)->getSlot()->getId() == _selUnit->getArmor()->getDefaultInventoryMap("STR_LEFT_HAND"))
					x += _dynamicOffset;

				frame->blitNShade(_items, x, (*i)->getSlot()->getY() + (*i)->getRules()->getHandSpriteOffY());
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
	if (_selUnit != 0) for (std::vector<std::string>::const_iterator i = _selUnit->getArmor()->getInventorySlots().cbegin(); i != _selUnit->getArmor()->getInventorySlots().cend(); i++)
	{
		RuleInventory *ruleI = (*_game->getMod()->getInventories())[*i];
		if (ruleI->checkSlotInPosition(x, y))
		{
			return ruleI;
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
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_game->popState();
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_game->popState();
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
	{
		if (_selUnit == 0)
			return;

		int x = (int)floor(action->getAbsoluteXMouse()) - getX(),
			y = (int)floor(action->getAbsoluteYMouse()) - getY();

		if (x >= Screen::ORIGINAL_WIDTH / 2)
			x -= _dynamicOffset;
		else
			x += _dynamicOffset;

		x -= ALIEN_INVENTORY_STATIC_OFFSET;

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

}
