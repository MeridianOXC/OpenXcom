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
#include "Inventory.h"
#include <algorithm>
#include <cmath>
#include "../Mod/Mod.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/RuleInterface.h"
#include "../Engine/Game.h"
#include "../Engine/Timer.h"
#include "../Interface/Text.h"
#include "../Interface/NumberText.h"
#include "../Engine/Font.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Mod/RuleStartingCondition.h"
#include "../Engine/SurfaceSet.h"
#include "../Savegame/BattleItem.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Engine/Action.h"
#include "../Engine/Sound.h"
#include "../Engine/Script.h"
#include "WarningMessage.h"
#include "../Savegame/Tile.h"
#include "PrimeGrenadeState.h"
#include "../Ufopaedia/Ufopaedia.h"
#include <unordered_map>
#include "../Engine/Screen.h"
#include "../Engine/CrossPlatform.h"
#include "TileEngine.h"

namespace OpenXcom
{

/**
 * Sets up an inventory with the specified size and position.
 * @param game Pointer to core game.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 * @param base Is the inventory being called from the basescape?
 */
Inventory::Inventory(Game *game, int width, int height, int x, int y, bool base) : InteractiveSurface(width, height, x, y), _game(game), _selUnit(0), _selItem(0), _tu(true), _base(base), _mouseOverItem(0), _groundOffset(0), _animFrame(0)
{
	_twoHandedRed = _game->getMod()->getInterface("battlescape")->getElement("twoHandedRed")->color;
	_twoHandedGreen = _game->getMod()->getInterface("battlescape")->getElement("twoHandedGreen")->color;

	_depth = _game->getSavedGame()->getSavedBattle()->getDepth();
	_grid = new Surface(width, height, 0, 0);
	_items = new Surface(width, height, 0, 0);
	_selection = new Surface(RuleInventory::HAND_W * RuleInventory::SLOT_W, RuleInventory::HAND_H * RuleInventory::SLOT_H, x, y);
	_warning = new WarningMessage(224, 24, 48, 176);
	_stackNumber = new NumberText(15, 15, 0, 0);
	_stackNumber->setBordered(true);

	_warning->initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());
	_warning->setColor(_game->getMod()->getInterface("battlescape")->getElement("warning")->color2);
	_warning->setTextColor(_game->getMod()->getInterface("battlescape")->getElement("warning")->color);

	_animTimer = new Timer(100);
	_animTimer->onTimer((SurfaceHandler)&Inventory::animate);
	_animTimer->start();

	_stunIndicator = _game->getMod()->getSurface("BigStunIndicator", false);
	_woundIndicator = _game->getMod()->getSurface("BigWoundIndicator", false);
	_burnIndicator = _game->getMod()->getSurface("BigBurnIndicator", false);
	_shockIndicator = _game->getMod()->getSurface("BigShockIndicator", false);

	const SavedBattleGame *battleSave = _game->getSavedGame()->getSavedBattle();
	if (battleSave)
	{
		const RuleStartingCondition *startingCondition = _game->getMod()->getStartingCondition(battleSave->getStartingConditionType());
		if (startingCondition)
		{
			if (!startingCondition->getInventoryShockIndicator().empty())
			{
				_shockIndicator = _game->getMod()->getSurface(startingCondition->getInventoryShockIndicator(), false);
			}
		}
	}

	_inventorySlotRightHand = _game->getMod()->getInventory("STR_RIGHT_HAND", true);
	_inventorySlotLeftHand = _game->getMod()->getInventory("STR_LEFT_HAND", true);
	_inventorySlotBackPack = _game->getMod()->getInventory("STR_BACK_PACK", true);
	_inventorySlotBelt = _game->getMod()->getInventory("STR_BELT", true);
	_inventorySlotGround = _game->getMod()->getInventory("STR_GROUND", true);
}

/**
 * Deletes inventory surfaces.
 */
Inventory::~Inventory()
{
	delete _grid;
	delete _items;
	delete _selection;
	delete _warning;
	delete _stackNumber;
	delete _animTimer;
}

/**
 * Replaces a certain amount of colors in the inventory's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void Inventory::setPalette(SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_grid->setPalette(colors, firstcolor, ncolors);
	_items->setPalette(colors, firstcolor, ncolors);
	_selection->setPalette(colors, firstcolor, ncolors);
	_warning->setPalette(colors, firstcolor, ncolors);
	_stackNumber->setPalette(getPalette());
}

/**
 * Changes the inventory's Time Units mode.
 * When True, inventory actions cost soldier time units (for battle).
 * When False, inventory actions don't cost anything (for pre-equip).
 * @param tu Time Units mode.
 */
void Inventory::setTuMode(bool tu)
{
	_tu = tu;
}

/**
 * Returns the currently selected (i.e. displayed) unit.
 * @return Pointer to selected unit, or 0 if none.
 */
BattleUnit *Inventory::getSelectedUnit() const
{
	return _selUnit;
}

/**
 * Changes the unit to display the inventory of.
 * @param unit Pointer to battle unit.
 */
void Inventory::setSelectedUnit(BattleUnit *unit)
{
	_selUnit = unit;
	_groundOffset = 999;
	arrangeGround(1);
}

/**
 * Draws the inventory elements.
 */
void Inventory::draw()
{
	drawGrid();
	drawItems();
}

/**
 * Draws the inventory grid for item placement.
 */
void Inventory::drawGrid()
{
	_grid->clear();
	RuleInterface *rule = _game->getMod()->getInterface("inventory");
	Uint8 color = rule->getElement("grid")->color;

	for (std::map<std::string, RuleInventory*>::iterator i = _game->getMod()->getInventories()->begin(); i != _game->getMod()->getInventories()->end(); ++i)
	{
		// Draw grid
		if (i->second->getType() == INV_SLOT)
		{
			for (std::vector<RuleSlot>::iterator j = i->second->getSlots()->begin(); j != i->second->getSlots()->end(); ++j)
			{
				SDL_Rect r;
				r.x = i->second->getX() + RuleInventory::SLOT_W * j->x;
				r.y = i->second->getY() + RuleInventory::SLOT_H * j->y;
				r.w = RuleInventory::SLOT_W + 1;
				r.h = RuleInventory::SLOT_H + 1;
				_grid->drawRect(&r, color);
				r.x++;
				r.y++;
				r.w -= 2;
				r.h -= 2;
				_grid->drawRect(&r, 0);
			}
		}
		else if (i->second->getType() == INV_HAND)
		{
			SDL_Rect r;
			r.x = i->second->getX();
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
		else if (i->second->getType() == INV_GROUND)
		{
			for (int x = i->second->getX(); x <= 320; x += RuleInventory::SLOT_W)
			{
				for (int y = i->second->getY(); y <= 200; y += RuleInventory::SLOT_H)
				{
					SDL_Rect r;
					r.x = x;
					r.y = y;
					r.w = RuleInventory::SLOT_W + 1;
					r.h = RuleInventory::SLOT_H + 1;
					_grid->drawRect(&r, color);
					r.x++;
					r.y++;
					r.w -= 2;
					r.h -= 2;
					_grid->drawRect(&r, 0);
				}
			}
		}
	}
	drawGridLabels();
}

/**
 * Draws the inventory grid labels.
 */
void Inventory::drawGridLabels(bool showTuCost)
{
	Text text = Text(90, 9, 0, 0);
	text.setPalette(_grid->getPalette());
	text.initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());

	RuleInterface *rule = _game->getMod()->getInterface("inventory");

	text.setColor(rule->getElement("textSlots")->color);
	text.setHighContrast(true);

	for (std::map<std::string, RuleInventory*>::iterator i = _game->getMod()->getInventories()->begin(); i != _game->getMod()->getInventories()->end(); ++i)
	{
		// Draw label
		text.setX(i->second->getX());
		text.setY(i->second->getY() - text.getFont()->getHeight() - text.getFont()->getSpacing());
		if (showTuCost && _selItem != 0)
		{
			std::ostringstream ss;
			ss << _game->getLanguage()->getString(i->second->getId());
			ss << ":";
			ss << _selItem->getSlot()->getCost(i->second);
			text.setText(ss.str().c_str());
		}
		else
		{
			text.setText(_game->getLanguage()->getString(i->second->getId()));
		}
		text.blit(_grid->getSurface());
	}
}

/**
 * Draws the items contained in the soldier's inventory.
 */
void Inventory::drawItems()
{
	const int Pulsate[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };
	Surface *tempSurface = _game->getMod()->getSurfaceSet("SCANG.DAT")->getFrame(6);
	auto primers = [&](int x, int y, bool a)
	{
		tempSurface->blitNShade(_items, x, y, Pulsate[_animFrame % 8], false, a ? 0 : 32);
	};
	auto indicators = [&](Surface *surf, int x, int y)
	{
		surf->blitNShade(_items, x, y, Pulsate[_animFrame % 8]);
	};

	ScriptWorkerBlit work;
	_items->clear();
	Uint8 color = _game->getMod()->getInterface("inventory")->getElement("numStack")->color;
	Uint8 color2 = _game->getMod()->getInterface("inventory")->getElement("numStack")->color2;
	if (_selUnit != 0)
	{
		SurfaceSet *texture = _game->getMod()->getSurfaceSet("BIGOBS.PCK");
		// Soldier items
		for (std::vector<BattleItem*>::iterator i = _selUnit->getInventory()->begin(); i != _selUnit->getInventory()->end(); ++i)
		{
			Surface *frame = (*i)->getBigSprite(texture);

			if ((*i) == _selItem || !frame)
				continue;

			int x, y;
			if ((*i)->getSlot()->getType() == INV_SLOT)
			{
				x = ((*i)->getSlot()->getX() + (*i)->getSlotX() * RuleInventory::SLOT_W);
				y = ((*i)->getSlot()->getY() + (*i)->getSlotY() * RuleInventory::SLOT_H);
			}
			else if ((*i)->getSlot()->getType() == INV_HAND)
			{
				x = ((*i)->getSlot()->getX() + (*i)->getRules()->getHandSpriteOffX());
				y = ((*i)->getSlot()->getY() + (*i)->getRules()->getHandSpriteOffY());
			}
			else
			{
				continue;
			}
			BattleItem::ScriptFill(&work, *i, BODYPART_ITEM_INVENTORY, _animFrame, 0);
			work.executeBlit(frame, _items, x, y, 0);

			// two-handed indicator
			if (Options::twoHandedIndicatorInventory && (*i)->getSlot()->getType() == INV_HAND)
			{
				if ((*i)->getRules()->isTwoHanded() || (*i)->getRules()->isBlockingBothHands())
				{
					NumberText text = NumberText(10, 5, 0, 0);
					text.setPalette(getPalette());
					text.setColor((*i)->getRules()->isBlockingBothHands() ? _twoHandedRed : _twoHandedGreen);
					text.setBordered(false);
					text.setX((*i)->getSlot()->getX() + RuleInventory::HAND_W * RuleInventory::SLOT_W - 5);
					text.setY((*i)->getSlot()->getY() + RuleInventory::HAND_H * RuleInventory::SLOT_H - 7);
					text.setValue(2);
					text.blit(_items->getSurface());
				}
			}

			// grenade primer indicators
			if ((*i)->getFuseTimer() >= 0)
			{
				primers(x, y, (*i)->isFuseEnabled());
			}
		}
		Surface stackLayer(getWidth(), getHeight(), 0, 0);
		stackLayer.setPalette(getPalette());
		// Ground items
		int fatalWounds = 0;
		for (std::vector<BattleItem*>::iterator i = _selUnit->getTile()->getInventory()->begin(); i != _selUnit->getTile()->getInventory()->end(); ++i)
		{
			Surface *frame = (*i)->getBigSprite(texture);
			// note that you can make items invisible by setting their width or height to 0 (for example used with tank corpse items)
			if ((*i) == _selItem || (*i)->getSlotX() < _groundOffset || (*i)->getRules()->getInventoryHeight() == 0 || (*i)->getRules()->getInventoryWidth() == 0 || !frame)
				continue;

			int x, y;
			x = ((*i)->getSlot()->getX() + ((*i)->getSlotX() - _groundOffset) * RuleInventory::SLOT_W);
			y = ((*i)->getSlot()->getY() + (*i)->getSlotY() * RuleInventory::SLOT_H);
			BattleItem::ScriptFill(&work, *i, BODYPART_ITEM_INVENTORY, _animFrame, 0);
			work.executeBlit(frame, _items, x, y, 0);

			// grenade primer indicators
			if ((*i)->getFuseTimer() >= 0)
			{
				primers(x, y, (*i)->isFuseEnabled());
			}

			// fatal wounds
			fatalWounds = 0;
			if ((*i)->getUnit())
			{
				// don't show on dead units
				if ((*i)->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
				{
					fatalWounds = (*i)->getUnit()->getFatalWounds();
					if (_burnIndicator && (*i)->getUnit()->getFire() > 0)
					{
						indicators(_burnIndicator, x, y);
					}
					else if (_woundIndicator && fatalWounds > 0)
					{
						indicators(_woundIndicator, x, y);
					}
					else if (_shockIndicator && (*i)->getUnit()->hasNegativeHealthRegen())
					{
						indicators(_shockIndicator, x, y);
					}
					else if (_stunIndicator)
					{
						indicators(_stunIndicator, x, y);
					}
				}
			}
			if (fatalWounds > 0)
			{
				_stackNumber->setX(((*i)->getSlot()->getX() + (((*i)->getSlotX() + (*i)->getRules()->getInventoryWidth()) - _groundOffset) * RuleInventory::SLOT_W)-4);
				if (fatalWounds > 9)
				{
					_stackNumber->setX(_stackNumber->getX()-4);
				}
				_stackNumber->setY(((*i)->getSlot()->getY() + ((*i)->getSlotY() + (*i)->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H)-6);
				_stackNumber->setValue(fatalWounds);
				_stackNumber->draw();
				_stackNumber->setColor(color2);
				_stackNumber->blit(stackLayer.getSurface());
			}

			// item stacking
			if (_stackLevel[(*i)->getSlotX()][(*i)->getSlotY()] > 1)
			{
				_stackNumber->setX(((*i)->getSlot()->getX() + (((*i)->getSlotX() + (*i)->getRules()->getInventoryWidth()) - _groundOffset) * RuleInventory::SLOT_W)-4);
				if (_stackLevel[(*i)->getSlotX()][(*i)->getSlotY()] > 9)
				{
					_stackNumber->setX(_stackNumber->getX()-4);
				}
				_stackNumber->setY(((*i)->getSlot()->getY() + ((*i)->getSlotY() + (*i)->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H)-6);
				_stackNumber->setValue(_stackLevel[(*i)->getSlotX()][(*i)->getSlotY()]);
				_stackNumber->draw();
				_stackNumber->setColor(color);
				_stackNumber->blit(stackLayer.getSurface());
			}
		}

		stackLayer.blitNShade(_items, 0, 0);
	}
}

/**
 * Draws the selected item.
 */
void Inventory::drawSelectedItem()
{
	if (_selItem)
	{
		_selection->clear();
		_selItem->getRules()->drawHandSprite(_game->getMod()->getSurfaceSet("BIGOBS.PCK"), _selection, _selItem, _animFrame);
	}
}

/**
 * Moves an item to a specified slot in the
 * selected player's inventory.
 * @param item Pointer to battle item.
 * @param slot Inventory slot, or NULL if none.
 * @param x X position in slot.
 * @param y Y position in slot.
 */
void Inventory::moveItem(BattleItem *item, RuleInventory *slot, int x, int y)
{
	_game->getSavedGame()->getSavedBattle()->getTileEngine()->itemMoveInventory(_selUnit->getTile(), _selUnit, item, slot, x, y);
}

/**
 * Checks if an item in a certain slot position would
 * overlap with any other inventory item.
 * @param unit Pointer to current unit.
 * @param item Pointer to battle item.
 * @param slot Inventory slot, or NULL if none.
 * @param x X position in slot.
 * @param y Y position in slot.
 * @return If there's overlap.
 */
bool Inventory::overlapItems(BattleUnit *unit, BattleItem *item, RuleInventory *slot, int x, int y)
{
	if (slot->getType() != INV_GROUND)
	{
		for (std::vector<BattleItem*>::const_iterator i = unit->getInventory()->begin(); i != unit->getInventory()->end(); ++i)
		{
			if ((*i)->getSlot() == slot && (*i)->occupiesSlot(x, y, item))
			{
				return true;
			}
		}
	}
	else if (unit->getTile() != 0)
	{
		for (std::vector<BattleItem*>::const_iterator i = unit->getTile()->getInventory()->begin(); i != unit->getTile()->getInventory()->end(); ++i)
		{
			if ((*i)->occupiesSlot(x, y, item))
			{
				return true;
			}
		}
	}
	return false;
}

/**
 * Gets the inventory slot located in the specified mouse position.
 * @param x Mouse X position. Returns the slot's X position.
 * @param y Mouse Y position. Returns the slot's Y position.
 * @return Slot rules, or NULL if none.
 */
RuleInventory *Inventory::getSlotInPosition(int *x, int *y) const
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
 * Returns the item currently grabbed by the player.
 * @return Pointer to selected item, or NULL if none.
 */
BattleItem *Inventory::getSelectedItem() const
{
	return _selItem;
}

/**
 * Changes the item currently grabbed by the player.
 * @param item Pointer to selected item, or NULL if none.
 */
void Inventory::setSelectedItem(BattleItem *item)
{
	_selItem = (item && !item->getRules()->isFixed()) ? item : 0;
	if (_selItem)
	{
		if (_selItem->getSlot()->getType() == INV_GROUND)
		{
			_stackLevel[_selItem->getSlotX()][_selItem->getSlotY()] -= 1;
		}
	}
	else
	{
		_selection->clear();
	}
	drawSelectedItem();

	// 1. first draw the grid
	if (_tu)
	{
		drawGrid();
	}

	// 2. then the items
	drawItems();

	// 3. lastly re-draw the grid labels (so that they are not obscured by the items)
	if (_tu)
	{
		drawGridLabels(true);
	}
}

/**
* Changes the item currently grabbed by the player.
* @param item Pointer to selected item, or NULL if none.
*/
void Inventory::setSearchString(const std::string &searchString)
{
	_searchString = searchString;
	Unicode::upperCase(_searchString);
	arrangeGround(1);
}

/**
 * Returns the item currently under mouse cursor.
 * @return Pointer to selected item, or 0 if none.
 */
BattleItem *Inventory::getMouseOverItem() const
{
	return _mouseOverItem;
}

/**
 * Changes the item currently under mouse cursor.
 * @param item Pointer to selected item, or NULL if none.
 */
void Inventory::setMouseOverItem(BattleItem *item)
{
	_mouseOverItem = (item && !(item->getRules()->isFixed() && item->getRules()->getBattleType() == BT_NONE)) ? item : 0;
}

/**
 * Handles timers.
 */
void Inventory::think()
{
	_warning->think();
	_animTimer->think(0,this);
}

/**
 * Blits the inventory elements.
 * @param surface Pointer to surface to blit onto.
 */
void Inventory::blit(SDL_Surface *surface)
{
	clear();
	_grid->blitNShade(this, 0, 0);
	_items->blitNShade(this, 0, 0);
	_selection->blitNShade(this, _selection->getX(), _selection->getY());
	_warning->blit(this->getSurface());
	Surface::blit(surface);
}

/**
 * Moves the selected item.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Inventory::mouseOver(Action *action, State *state)
{
	_selection->setX((int)floor(action->getAbsoluteXMouse()) - _selection->getWidth()/2 - getX());
	_selection->setY((int)floor(action->getAbsoluteYMouse()) - _selection->getHeight()/2 - getY());
	if (_selUnit == 0)
		return;

	int x = (int)floor(action->getAbsoluteXMouse()) - getX(),
		y = (int)floor(action->getAbsoluteYMouse()) - getY();
	RuleInventory *slot = getSlotInPosition(&x, &y);
	if (slot != 0)
	{
		if (slot->getType() == INV_GROUND)
		{
			x += _groundOffset;
		}
		BattleItem *item = _selUnit->getItem(slot, x, y);
		setMouseOverItem(item);
	}
	else
	{
		setMouseOverItem(0);
	}

	_selection->setX((int)floor(action->getAbsoluteXMouse()) - _selection->getWidth()/2 - getX());
	_selection->setY((int)floor(action->getAbsoluteYMouse()) - _selection->getHeight()/2 - getY());
	InteractiveSurface::mouseOver(action, state);
}

/**
 * Picks up / drops an item.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Inventory::mouseClick(Action *action, State *state)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_selUnit == 0)
			return;
		// Pickup item
		if (_selItem == 0)
		{
			int x = (int)floor(action->getAbsoluteXMouse()) - getX(),
				y = (int)floor(action->getAbsoluteYMouse()) - getY();
			RuleInventory *slot = getSlotInPosition(&x, &y);
			if (slot != 0)
			{
				if (slot->getType() == INV_GROUND)
				{
					x += _groundOffset;
				}
				BattleItem *item = _selUnit->getItem(slot, x, y);
				if (item != 0 && !item->getRules()->isFixed())
				{
					if ((SDL_GetModState() & KMOD_CTRL))
					{
						RuleInventory *newSlot = _inventorySlotGround;
						std::string warning = "STR_NOT_ENOUGH_SPACE";
						bool placed = false;

						if (slot->getType() == INV_GROUND)
						{
							switch (item->getRules()->getBattleType())
							{
							case BT_FIREARM:
								newSlot = _inventorySlotRightHand;
								break;
							case BT_MINDPROBE:
							case BT_PSIAMP:
							case BT_MELEE:
							case BT_CORPSE:
								newSlot = _inventorySlotLeftHand;
								break;
							default:
								if (item->getRules()->getInventoryHeight() > 2)
								{
									newSlot = _inventorySlotBackPack;
								}
								else
								{
									newSlot = _inventorySlotBelt;
								}
								break;
							}
						}

						if (newSlot->getType() != INV_GROUND)
						{
							_stackLevel[item->getSlotX()][item->getSlotY()] -= 1;

							placed = fitItem(newSlot, item, warning);

							if (!placed)
							{
								for (std::map<std::string, RuleInventory *>::const_iterator wildCard = _game->getMod()->getInventories()->begin(); wildCard != _game->getMod()->getInventories()->end() && !placed; ++wildCard)
								{
									newSlot = wildCard->second;
									if (newSlot->getType() == INV_GROUND)
									{
										continue;
									}
									placed = fitItem(newSlot, item, warning);
								}
							}
							if (!placed)
							{
								_stackLevel[item->getSlotX()][item->getSlotY()] += 1;
							}
						}
						else
						{
							if (!_tu || _selUnit->spendTimeUnits(item->getSlot()->getCost(newSlot)))
							{
								placed = true;
								moveItem(item, newSlot, 0, 0);
								_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_DROP)->play();
								arrangeGround();
							}
							else
							{
								warning = "STR_NOT_ENOUGH_TIME_UNITS";
							}
						}

						if (!placed)
						{
							_warning->showMessage(_game->getLanguage()->getString(warning));
						}
					}
					else
					{
						setSelectedItem(item);
						if (item->getFuseTimer() >= 0)
						{
							_warning->showMessage(_game->getLanguage()->getString(item->getRules()->getPrimeActionMessage()));
						}
					}
				}
			}
		}
		// Drop item
		else
		{
			int x = _selection->getX() + (RuleInventory::HAND_W - _selItem->getRules()->getInventoryWidth()) * RuleInventory::SLOT_W/2 + RuleInventory::SLOT_W/2,
				y = _selection->getY() + (RuleInventory::HAND_H - _selItem->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H/2 + RuleInventory::SLOT_H/2;
			RuleInventory *slot = getSlotInPosition(&x, &y);
			if (slot != 0)
			{
				if (slot->getType() == INV_GROUND)
				{
					x += _groundOffset;
				}
				BattleItem *item = _selUnit->getItem(slot, x, y);

				bool canStack = slot->getType() == INV_GROUND && canBeStacked(item, _selItem);

				// Check if this inventory section supports the item
				if (!_selItem->getRules()->canBePlacedIntoInventorySection(slot->getId()))
				{
					_warning->showMessage(_game->getLanguage()->getString("STR_CANNOT_PLACE_ITEM_INTO_THIS_SECTION"));
				}
				// Put item in empty slot, or stack it, if possible.
				else if (item == 0 || item == _selItem || canStack)
				{
					if (!overlapItems(_selUnit, _selItem, slot, x, y) && slot->fitItemInSlot(_selItem->getRules(), x, y))
					{
						if (!_tu || _selUnit->spendTimeUnits(_selItem->getSlot()->getCost(slot)))
						{
							moveItem(_selItem, slot, x, y);
							if (slot->getType() == INV_GROUND)
							{
								_stackLevel[x][y] += 1;
							}
							setSelectedItem(0);
							_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_DROP)->play();
						}
						else
						{
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
						}
					}
					else if (canStack)
					{
						if (!_tu || _selUnit->spendTimeUnits(_selItem->getSlot()->getCost(slot)))
						{
							moveItem(_selItem, slot, item->getSlotX(), item->getSlotY());
							_stackLevel[item->getSlotX()][item->getSlotY()] += 1;
							setSelectedItem(0);
							_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_DROP)->play();
						}
						else
						{
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
						}
					}
				}
				// Put item in weapon
				else if (item->isWeaponWithAmmo())
				{
					int slotAmmo = item->getRules()->getSlotForAmmo(_selItem->getRules()->getType());
					if (slotAmmo == -1)
					{
						_warning->showMessage(_game->getLanguage()->getString("STR_WRONG_AMMUNITION_FOR_THIS_WEAPON"));
					}
					else
					{
						int tuCost = item->getRules()->getTULoad(slotAmmo);

						if (_selItem->getSlot()->getType() != INV_HAND)
						{
							tuCost += _selItem->getSlot()->getCost(_inventorySlotRightHand);
						}

						BattleItem *weaponRightHand = _selUnit->getRightHandWeapon();
						BattleItem *weaponLeftHand = _selUnit->getLeftHandWeapon();

						auto canLoad = true;
						if (item->getAmmoForSlot(slotAmmo) != 0)
						{
							auto tuUnload = item->getRules()->getTUUnload(slotAmmo);
							if ((SDL_GetModState() & KMOD_SHIFT) && (!_tu || tuUnload) && (item == weaponRightHand || item == weaponLeftHand))
							{
								auto checkBoth = [&](BattleItem *i)
								{
									return nullptr == i || item == i || _selItem == i;
								};

								if (checkBoth(weaponRightHand) && checkBoth(weaponLeftHand))
								{
									tuCost += tuUnload;
								}
								else
								{
									canLoad = false;
									_warning->showMessage(_game->getLanguage()->getString("STR_BOTH_HANDS_MUST_BE_EMPTY"));
								}
							}
							else
							{
								canLoad = false;
								_warning->showMessage(_game->getLanguage()->getString("STR_WEAPON_IS_ALREADY_LOADED"));
							}
						}
						if (canLoad)
						{
							if (!_tu || _selUnit->spendTimeUnits(tuCost))
							{
								auto oldAmmo = item->setAmmoForSlot(slotAmmo, _selItem);
								if (oldAmmo)
								{
									moveItem(oldAmmo, (item == weaponRightHand ? _inventorySlotLeftHand : _inventorySlotRightHand), 0, 0);
								}
								setSelectedItem(0);
								_game->getMod()->getSoundByDepth(_depth, item->getRules()->getReloadSound())->play();
								if (item->getSlot()->getType() == INV_GROUND)
								{
									arrangeGround();
								}
							}
							else
							{
								_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
							}
						}
					}
				}
				// else swap the item positions?
			}
			else
			{
				// try again, using the position of the mouse cursor, not the item (slightly more intuitive for stacking)
				x = (int)floor(action->getAbsoluteXMouse()) - getX();
				y = (int)floor(action->getAbsoluteYMouse()) - getY();
				slot = getSlotInPosition(&x, &y);
				if (slot != 0 && slot->getType() == INV_GROUND)
				{
					x += _groundOffset;
					BattleItem *item = _selUnit->getItem(slot, x, y);
					if (canBeStacked(item, _selItem))
					{
						if (!_tu || _selUnit->spendTimeUnits(_selItem->getSlot()->getCost(slot)))
						{
							moveItem(_selItem, slot, item->getSlotX(), item->getSlotY());
							_stackLevel[item->getSlotX()][item->getSlotY()] += 1;
							setSelectedItem(0);
							_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_DROP)->play();
						}
						else
						{
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
						}
					}
				}
			}
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (_selItem == 0)
		{
			if (!_base || Options::includePrimeStateInSavedLayout)
			{
				if (!_tu)
				{
					int x = (int)floor(action->getAbsoluteXMouse()) - getX(),
						y = (int)floor(action->getAbsoluteYMouse()) - getY();
					RuleInventory *slot = getSlotInPosition(&x, &y);
					if (slot != 0)
					{
						if (slot->getType() == INV_GROUND)
						{
							x += _groundOffset;
						}
						BattleItem *item = _selUnit->getItem(slot, x, y);
						if (item != 0)
						{
							const BattleFuseType fuseType = item->getRules()->getFuseTimerType();
							if (fuseType != BFT_NONE)
							{
								if (item->getFuseTimer() == -1)
								{
									// Prime that grenade!
									if (fuseType == BFT_SET)
									{
										_game->pushState(new PrimeGrenadeState(0, true, item));
									}
									else
									{
										_warning->showMessage(_game->getLanguage()->getString(item->getRules()->getPrimeActionMessage()));
										item->setFuseTimer(item->getRules()->getFuseTimerDefault());
										arrangeGround();
									}
								}
								else
								{
									_warning->showMessage(_game->getLanguage()->getString(item->getRules()->getUnprimeActionMessage()));
									item->setFuseTimer(-1);  // Unprime the grenade
									arrangeGround();
								}
							}
						}
					}
				}
				else
				{
					_game->popState(); // Closes the inventory window on right-click (if not in preBattle equip screen!)
				}
			}
		}
		else
		{
			if (_selItem->getSlot()->getType() == INV_GROUND)
			{
				_stackLevel[_selItem->getSlotX()][_selItem->getSlotY()] += 1;
			}
			// Return item to original position
			setSelectedItem(0);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
	{
		if (_selUnit == 0)
			return;

		int x = (int)floor(action->getAbsoluteXMouse()) - getX(),
			y = (int)floor(action->getAbsoluteYMouse()) - getY();
		RuleInventory *slot = getSlotInPosition(&x, &y);
		if (slot != 0)
		{
			if (slot->getType() == INV_GROUND)
			{
				x += _groundOffset;
			}
			BattleItem *item = _selUnit->getItem(slot, x, y);
			if (item != 0)
			{
				std::string articleId = item->getRules()->getType();
				Ufopaedia::openArticle(_game, articleId);
			}
		}
	}
	InteractiveSurface::mouseClick(action, state);
}

/**
 * Unloads the selected weapon, placing the gun
 * on the right hand and the ammo on the left hand.
 * Or if only one hand is free, the gun is placed
 * in the hand and the ammo is placed on the ground.
 * @return The success of the weapon being unloaded.
 */
bool Inventory::unload()
{
	// Must be holding an item
	if (_selItem == 0)
	{
		return false;
	}

	const auto type = _selItem->getRules()->getBattleType();
	const bool grenade = type == BT_GRENADE || type == BT_PROXIMITYGRENADE;
	const bool weapon = type == BT_FIREARM || type == BT_MELEE;
	int slotForAmmoUnload = -1;
	int toForAmmoUnload = 0;

	// Item should be able to unload or unprimed.
	if (grenade)
	{
		// Item must be primed
		if (_selItem->getFuseTimer() == -1)
		{
			return false;
		}
	}
	else if (weapon)
	{
		// Item must be loaded
		bool showError = false;
		auto checkSlot = [&](int slot)
		{
			if (!_selItem->needsAmmoForSlot(slot))
			{
				return false;
			}

			auto tu = _selItem->getRules()->getTUUnload(slot);
			if (tu == 0 && !_tu)
			{
				return false;
			}

			auto ammo = _selItem->getAmmoForSlot(slot);
			if (ammo)
			{
				toForAmmoUnload = tu;
				slotForAmmoUnload = slot;
				return true;
			}
			else
			{
				showError = true;
				return false;
			}
		};
		if (!(SDL_GetModState() & KMOD_SHIFT))
		{
			for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
			{
				if (checkSlot(slot))
				{
					break;
				}
			}
		}
		else
		{
			for (int slot = RuleItem::AmmoSlotMax - 1; slot >= 0; --slot)
			{
				if (checkSlot(slot))
				{
					break;
				}
			}
		}
		if (slotForAmmoUnload == -1)
		{
			if (showError)
			{
				_warning->showMessage(_game->getLanguage()->getString("STR_NO_AMMUNITION_LOADED"));
			}
			return false;
		}
	}
	else
	{
		// not weapon or grenade, can't use unload button
		return false;
	}

	// Check which hands are free.
	RuleInventory *FirstFreeHand = _inventorySlotRightHand;
	RuleInventory *SecondFreeHand = _inventorySlotLeftHand;

	for (std::vector<BattleItem*>::iterator i = _selUnit->getInventory()->begin(); i != _selUnit->getInventory()->end(); ++i)
	{
		if ((*i)->getSlot()->getType() == INV_HAND && (*i) != _selItem)
		{
			if (!Options::oneHandedUnloading)
			{
				_warning->showMessage(_game->getLanguage()->getString("STR_BOTH_HANDS_MUST_BE_EMPTY"));
				return false;
			}
			else
			{
				if ((*i)->getSlot() == SecondFreeHand)
				{
					SecondFreeHand = nullptr;
				}
				if ((*i)->getSlot() == FirstFreeHand)
				{
					FirstFreeHand = nullptr;
				}
			}
		}
	}

	if (FirstFreeHand == nullptr)
	{
		FirstFreeHand = SecondFreeHand;
		SecondFreeHand = nullptr;
	}
	if (FirstFreeHand == nullptr)
	{
		_warning->showMessage(_game->getLanguage()->getString("STR_ONE_HAND_MUST_BE_EMPTY"));
		return false;
	}

	BattleActionCost cost { BA_NONE, _selUnit, _selItem };
	if (grenade)
	{
		cost.type = BA_UNPRIME;
		cost.updateTU();
	}
	else
	{
		cost.Time += toForAmmoUnload;
	}

	if (cost.haveTU() && _selItem->getSlot()->getType() != INV_HAND)
	{
		cost.Time += _selItem->getSlot()->getCost(FirstFreeHand);
	}

	std::string err;
	if (!_tu || cost.spendTU(&err))
	{
		moveItem(_selItem, FirstFreeHand, 0, 0);
		if (grenade)
		{
			_selItem->setFuseTimer(-1);
			_warning->showMessage(_game->getLanguage()->getString(_selItem->getRules()->getUnprimeActionMessage()));
		}
		else
		{
			auto oldAmmo = _selItem->setAmmoForSlot(slotForAmmoUnload, nullptr);
			if (SecondFreeHand != nullptr)
			{
				moveItem(oldAmmo, SecondFreeHand, 0, 0);
			}
			else
			{
				moveItem(oldAmmo, _inventorySlotGround, 0, 0);
				arrangeGround();
			}
		}
		setSelectedItem(0);
		return true;
	}
	else
	{
		if (!err.empty())
		{
			_warning->showMessage(_game->getLanguage()->getString(err));
		}
		return false;
	}
}

/**
* Checks whether the given item is visible with the current search string.
* @param item The item to check.
* @return True if item should be shown. False otherwise.
*/
bool Inventory::isInSearchString(BattleItem *item)
{
	if (!_searchString.length())
	{
		// No active search string.
		return true;
	}

	std::string itemLocalName;
	if (!_game->getSavedGame()->isResearched(item->getRules()->getRequirements()))
	{
		// Alien artifact, shouldn't match on the real name.
		itemLocalName = _game->getLanguage()->getString("STR_ALIEN_ARTIFACT");
	}
	else
	{
		itemLocalName = _game->getLanguage()->getString(item->getRules()->getName());
	}
	Unicode::upperCase(itemLocalName);
	if (itemLocalName.find(_searchString) != std::string::npos)
	{
		// Name match.
		return true;
	}

	// If present in the Ufopaedia, check categories for a match as well.
	ArticleDefinition *articleID = _game->getMod()->getUfopaediaArticle(item->getRules()->getType());
	if (articleID && Ufopaedia::isArticleAvailable(_game->getSavedGame(), articleID))
	{
		std::vector<std::string> itemCategories = item->getRules()->getCategories();
		for (std::vector<std::string>::iterator i = itemCategories.begin(); i != itemCategories.end(); ++i)
		{
			std::string catLocalName = _game->getLanguage()->getString((*i));
			Unicode::upperCase(catLocalName);
			if (catLocalName.find(_searchString) != std::string::npos)
			{
				// Category match
				return true;
			}
		}

		for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
		{
			if (item->getAmmoForSlot(slot))
			{
				std::vector<std::string> itemAmmoCategories = item->getAmmoForSlot(slot)->getRules()->getCategories();
				for (std::vector<std::string>::iterator i = itemAmmoCategories.begin(); i != itemAmmoCategories.end(); ++i)
				{
					std::string catLocalName = _game->getLanguage()->getString((*i));
					Unicode::upperCase(catLocalName);
					if (catLocalName.find(_searchString) != std::string::npos)
					{
						// Category match
						return true;
					}
				}
			}
		}
	}
	return false;
}

/**
 * Arranges items on the ground for the inventory display.
 * Since items on the ground aren't assigned to anyone,
 * they don't actually have permanent slot positions.
 * @param alterOffset Whether to alter the ground offset and in which direction.
 */
void Inventory::arrangeGround(int alterOffset)
{
	RuleInventory *ground = _inventorySlotGround;

	int slotsX = (Screen::ORIGINAL_WIDTH - ground->getX()) / RuleInventory::SLOT_W;
	int slotsY = (Screen::ORIGINAL_HEIGHT - ground->getY()) / RuleInventory::SLOT_H;
	int x = 0;
	int y = 0;
	bool donePlacing = false;
	bool canPlace = false;
	int xMax = 0;
	_stackLevel.clear();

	if (_selUnit != 0)
	{
		std::unordered_map< std::string, std::vector< std::vector<BattleItem*> > > typeItemLists; // Lists of items of the same type
		std::vector<BattleItem*> itemListOrder; // Placement order of item type stacks.
		std::vector< std::vector<int> > startIndexCacheX; // Cache for skipping to last known available position of a given size.
		// Create chart of free slots for later rapid lookup.
		std::vector< std::vector<bool> > occupiedSlots;
		occupiedSlots.resize(slotsY, std::vector<bool>(slotsX * 2, false));

		// Move items out of the way and find which stack they'll end up in within the inventory.
		for (auto& i : *(_selUnit->getTile()->getInventory()))
		{
			// first move all items out of the way - a big number in X direction
			i->setSlot(ground);
			i->setSlotX(1000000);
			i->setSlotY(0);

			// Only handle visible items from this point on.
			if (i->getRules()->getInventoryWidth())
			{
				// Each item type has a list of stacks. Find / create a suitable one for this item.
				auto iterItemList = typeItemLists.find(i->getRules()->getType());
				if (iterItemList == typeItemLists.end()) {
					// New item type, create a list of item stacks for it.
					iterItemList = typeItemLists.insert(std::pair<std::string, std::vector< std::vector<BattleItem*> > >(i->getRules()->getType(), { { i } })).first;
					itemListOrder.push_back(i);
					// Figure out the largest item size present:
					x = std::max(x, i->getRules()->getInventoryWidth());
					y = std::max(y, i->getRules()->getInventoryHeight());
				}
				else {
					// With the item type found. Find which stack of this item type to add the item to.
					bool stacked = false;
					for (auto& itemStack : iterItemList->second)
					{
						if (canBeStacked(i, itemStack.at(0)))
						{
							itemStack.push_back(i);
							stacked = true;
							break;
						}
					}
					if (!stacked) {
						// Not able to be stacked with previously placed items of this type. Make a new stack containing only this item.
						iterItemList->second.push_back({ i });
					}
				}
			}
		}
		// Create the cache of last placed index for a given item size.
		startIndexCacheX.resize(y + 1, std::vector<int>(x + 1, 0));

		// Before we place the items, we should sort the itemListOrder vector using the 'list order' of the items.
		std::sort(itemListOrder.begin(), itemListOrder.end(), [](BattleItem* const &a, BattleItem* const &b)
			{
				return a->getRules()->getListOrder() < b->getRules()->getListOrder();
			}
		);

		// Now for each item type, find the most topleft position that is not occupied and will fit.
		for (auto& i : itemListOrder)
		{
			// Fetch the list of item stacks for this item type. Then place each stack.
			auto iterItemList = typeItemLists.find(i->getRules()->getType());
			for (auto& itemStack : iterItemList->second)
			{
				BattleItem* itemTypeSample = itemStack.at(0); // Grab a sample of the stack of item type we're trying to place.
				if (!isInSearchString(itemTypeSample))
				{
					// quick search
					// Not a match with the active search string, skip this item stack. (Will remain outside the visible inventory)
					break;
				}

				// Start searching at the x value where we last placed an item of this size.
				// But also don't let more than half a screen behind the furtherst item (because we want to keep similar items together).
				x = std::max(xMax - slotsX/2, startIndexCacheX[itemTypeSample->getRules()->getInventoryHeight()][itemTypeSample->getRules()->getInventoryWidth()]);
				y = 0;
				donePlacing = false;
				while (!donePlacing)
				{
					canPlace = true; // Assume we can put the item type here, if one of the following checks fails, we can't.
					for (int xd = 0; xd < itemTypeSample->getRules()->getInventoryWidth() && canPlace; xd++)
					{
						if ((x + xd) % slotsX < x % slotsX)
						{
							canPlace = false;
						}
						else
						{
							for (int yd = 0; yd < itemTypeSample->getRules()->getInventoryHeight() && canPlace; yd++)
							{
								// If there's room and no item, we can place it here. (No need to check for stackability as nothing stackable has been placed)
								canPlace = !occupiedSlots[y + yd][x + xd];
							}
						}
					}
					if (canPlace) // Found a place for this item stack.
					{
						xMax = std::max(xMax, x + itemTypeSample->getRules()->getInventoryWidth());
						if ( (x + startIndexCacheX[0].size() ) >= occupiedSlots[0].size())
						{
							// Filled enough for the widest item to potentially request occupancy checks outside of current cache. Expand slot cache.
							size_t newCacheSize = occupiedSlots[0].size() * 2;
							for (std::vector< std::vector<bool> >::iterator j = occupiedSlots.begin(); j != occupiedSlots.end(); ++j) {
								j->resize(newCacheSize, false);
							}
						}
						// Reserve the slots this item will occupy.
						for (int xd = 0; xd < itemTypeSample->getRules()->getInventoryWidth() && canPlace; xd++)
						{
							for (int yd = 0; yd < itemTypeSample->getRules()->getInventoryHeight() && canPlace; yd++)
							{
								occupiedSlots[y + yd][x + xd] = true;
							}
						}

						// Place all items from this stack in the location we just reserved.
						for (auto& j : itemStack)
						{
							j->setSlotX(x);
							j->setSlotY(y);
							_stackLevel[x][y] += 1;
						}
						donePlacing = true;
					}
					if (!donePlacing)
					{
						y++;
						if (y > slotsY - itemTypeSample->getRules()->getInventoryHeight())
						{
							y = 0;
							x++;
						}
					}
				}
				// Now update the shortcut cache so that items to follow are able to skip a decent chunk of their search,
				// as there can be no spot before this x-address with available slots for items that are larger in one or more dimensions.
				int firstPossibleX = itemTypeSample->getRules()->getInventoryHeight() * 2 > slotsY ? x + itemTypeSample->getRules()->getInventoryWidth() : x;
				for (size_t offsetY = itemTypeSample->getRules()->getInventoryHeight(); offsetY < startIndexCacheX.size(); ++offsetY)
				{
					for (size_t offsetX = itemTypeSample->getRules()->getInventoryWidth(); offsetX < startIndexCacheX[offsetY].size(); ++offsetX)
					{
						startIndexCacheX[offsetY][offsetX] = std::max(firstPossibleX, startIndexCacheX[offsetY][offsetX]);
					}
				}
			} // end stacks for this item type
		} // end of item types
	}
	if (alterOffset > 0)
	{
		if (xMax >= _groundOffset + slotsX)
		{
			_groundOffset += slotsX;
		}
		else
		{
			_groundOffset = 0;
		}
	}
	else if (alterOffset < 0)
	{
		// one step back
		_groundOffset -= slotsX;

		// if too much, as many steps forward as possible
		if (_groundOffset < 0)
		{
			while (xMax >= _groundOffset + slotsX)
			{
				_groundOffset += slotsX;
			}
		}
	}
	drawItems();
}

/**
 * Attempts to place the item in the inventory slot.
 * @param newSlot Where to place the item.
 * @param item Item to be placed.
 * @param warning Warning message if item could not be placed.
 * @return True, if the item was successfully placed in the inventory.
 */
bool Inventory::fitItem(RuleInventory *newSlot, BattleItem *item, std::string &warning)
{
	// Check if this inventory section supports the item
	if (!item->getRules()->canBePlacedIntoInventorySection(newSlot->getId()))
	{
		warning = "STR_CANNOT_PLACE_ITEM_INTO_THIS_SECTION";
		return false;
	}

	bool placed = false;
	int maxSlotX = 0;
	int maxSlotY = 0;
	for (std::vector<RuleSlot>::iterator j = newSlot->getSlots()->begin(); j != newSlot->getSlots()->end(); ++j)
	{
		if (j->x > maxSlotX) maxSlotX = j->x;
		if (j->y > maxSlotY) maxSlotY = j->y;
	}
	for (int y2 = 0; y2 <= maxSlotY && !placed; ++y2)
	{
		for (int x2 = 0; x2 <= maxSlotX && !placed; ++x2)
		{
			if (!overlapItems(_selUnit, item, newSlot, x2, y2) && newSlot->fitItemInSlot(item->getRules(), x2, y2))
			{
				if (!_tu || _selUnit->spendTimeUnits(item->getSlot()->getCost(newSlot)))
				{
					placed = true;
					moveItem(item, newSlot, x2, y2);
					_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_DROP)->play();
					drawItems();
				}
				else
				{
					warning = "STR_NOT_ENOUGH_TIME_UNITS";
				}
			}
		}
	}
	return placed;
}

/**
 * Checks if two items can be stacked on one another.
 * @param itemA First item.
 * @param itemB Second item.
 * @return True, if the items can be stacked on one another.
 */
bool Inventory::canBeStacked(BattleItem *itemA, BattleItem *itemB)
{
	//both items actually exist
	if (!itemA || !itemB) return false;

	//both items have the same ruleset
	if (itemA->getRules() != itemB->getRules()) return false;

	for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
	{
		auto ammoA = itemA->getAmmoForSlot(slot);
		auto ammoB = itemB->getAmmoForSlot(slot);
		// or they both have ammo
		if (ammoA && ammoB)
		{
			// and the same ammo type
			if (ammoA->getRules() != ammoB->getRules()) return false;
			// and the same ammo quantity
			if (ammoA->getAmmoQuantity() != ammoB->getAmmoQuantity()) return false;
		}
		else if (ammoA || ammoB)
		{
			return false;
		}
	}

	return (
		// and neither is set to explode
		itemA->getFuseTimer() == -1 && itemB->getFuseTimer() == -1 &&
		// and neither is a corpse or unconscious unit
		itemA->getUnit() == 0 && itemB->getUnit() == 0 &&
		// and if it's a medkit, it has the same number of charges
		itemA->getPainKillerQuantity() == itemB->getPainKillerQuantity() &&
		itemA->getHealQuantity() == itemB->getHealQuantity() &&
		itemA->getStimulantQuantity() == itemB->getStimulantQuantity());
}

/**
 * Shows a warning message.
 * @param msg The message to show.
 */
void Inventory::showWarning(const std::string &msg)
{
	_warning->showMessage(msg);
}

/**
 * Animate surface.
 */
void Inventory::animate()
{
	if (_tu)
	{
		SavedBattleGame* save = _game->getSavedGame()->getSavedBattle();
		save->nextAnimFrame();
		_animFrame = save->getAnimFrame();
	}
	else
	{
		_animFrame++;
	}

	drawItems();
	drawSelectedItem();
}

}
