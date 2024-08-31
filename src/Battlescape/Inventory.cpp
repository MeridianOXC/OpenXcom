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
#include "../Engine/Logger.h"
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
#include "../Mod/RuleEnviroEffects.h"
#include "../Engine/SurfaceSet.h"
#include "../Savegame/BattleItem.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleItemCategory.h"
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
Inventory::Inventory(Game *game, int width, int height, int x, int y, bool base) : InteractiveSurface(width, height, x, y), _game(game), _selUnit(0), _selItem(0), _tu(true), _base(base), _groundOffset(0), _animFrame(0), _xBeforeDrag(0), _yBeforeDrag(0), _dragging(false), _clicked(false)
{
	_twoHandedRed = _game->getMod()->getInterface("battlescape")->getElement("twoHandedRed")->color;
	_twoHandedGreen = _game->getMod()->getInterface("battlescape")->getElement("twoHandedGreen")->color;

	_depth = _game->getSavedGame()->getSavedBattle()->getDepth();
	_grid = new Surface(width, height, 0, 0);
	_items = new Surface(width, height, 0, 0);
	_gridLabels = new Surface(width, height, 0, 0);
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
		auto* enviro = battleSave->getEnviroEffects();
		if (enviro)
		{
			if (!enviro->getInventoryShockIndicator().empty())
			{
				_shockIndicator = _game->getMod()->getSurface(enviro->getInventoryShockIndicator(), false);
			}
		}
	}

	_inventorySlotRightHand = _game->getMod()->getInventoryRightHand();
	_inventorySlotLeftHand = _game->getMod()->getInventoryLeftHand();
	_inventorySlotBackPack = _game->getMod()->getInventoryBackpack();
	_inventorySlotBelt = _game->getMod()->getInventoryBelt();
	_inventorySlotGround = _game->getMod()->getInventoryGround();

	_groundSlotsX = (Screen::ORIGINAL_WIDTH - _inventorySlotGround->getX()) / RuleInventory::SLOT_W;
	_groundSlotsY = (Screen::ORIGINAL_HEIGHT - _inventorySlotGround->getY()) / RuleInventory::SLOT_H;
	_xMax = 0;
	_occupiedSlotsCache.resize(_groundSlotsY, std::vector<char>(_groundSlotsX * 2, false));
#ifdef __MOBILE__
	_longPressTimer = new Timer(Options::longPressDuration, true);
	_longPressTimer->onTimer((SurfaceHandler)&Inventory::longPressAction);
	_longPressTimer->stop();

	_longPressAction = NULL;
#endif
}

/**
 * Deletes inventory surfaces.
 */
Inventory::~Inventory()
{
	delete _grid;
	delete _items;
	delete _gridLabels;
	delete _selection;
	delete _warning;
	delete _stackNumber;
	delete _animTimer;
#ifdef __MOBILE__
	delete _longPressTimer;
	if (_longPressAction)
	{
		delete _longPressAction;
	}
#endif
}

/**
 * Replaces a certain amount of colors in the inventory's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void Inventory::setPalette(const SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_grid->setPalette(colors, firstcolor, ncolors);
	_items->setPalette(colors, firstcolor, ncolors);
	_gridLabels->setPalette(colors, firstcolor, ncolors);
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
void Inventory::setSelectedUnit(BattleUnit *unit, bool resetGroundOffset)
{
	_selUnit = unit;
	if (resetGroundOffset)
	{
		_groundOffset = 9999;
		arrangeGround(1);
	}
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

	for (const auto& invPair : *_game->getMod()->getInventories())
	{
		RuleInventory* ruleInv = invPair.second;

		// Draw grid
		if (ruleInv->getType() == INV_SLOT)
		{
			for (const auto& slotInfo : *ruleInv->getSlots())
			{
				SDL_Rect r;
				r.x = ruleInv->getX() + RuleInventory::SLOT_W * slotInfo.x;
				r.y = ruleInv->getY() + RuleInventory::SLOT_H * slotInfo.y;
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
		else if (ruleInv->getType() == INV_HAND)
		{
			SDL_Rect r;
			r.x = ruleInv->getX();
			r.y = ruleInv->getY();
			r.w = RuleInventory::HAND_W * RuleInventory::SLOT_W;
			r.h = RuleInventory::HAND_H * RuleInventory::SLOT_H;
			_grid->drawRect(&r, color);
			r.x++;
			r.y++;
			r.w -= 2;
			r.h -= 2;
			_grid->drawRect(&r, 0);
		}
		else if (ruleInv->getType() == INV_GROUND)
		{
			for (int x = ruleInv->getX(); x <= 320; x += RuleInventory::SLOT_W)
			{
				for (int y = ruleInv->getY(); y <= 200; y += RuleInventory::SLOT_H)
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
	_gridLabels->clear();

	Text text = Text(90, 9, 0, 0);
	text.setPalette(_gridLabels->getPalette());
	text.initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());

	RuleInterface *rule = _game->getMod()->getInterface("inventory");

	text.setColor(rule->getElement("textSlots")->color);
	text.setHighContrast(true);

	// Note: iterating over the (sorted) invs vector instead of invs map, because we want to consider listOrder here
	for (auto& invName : _game->getMod()->getInvsList())
	{
		auto* i = _game->getMod()->getInventory(invName, true);
		// Draw label
		text.setX(i->getX());
		text.setY(i->getY() - text.getFont()->getHeight() - text.getFont()->getSpacing());
		if (showTuCost && _selItem != 0 && _selItem->getSlot() != i)
		{
			std::ostringstream ss;
			ss << _game->getLanguage()->getString(i->getId()).arg(1 + _groundOffset / _groundSlotsX).arg(1 + _xMax / _groundSlotsX);
			ss << ":";
			ss << _selItem->getMoveToCost(i);
			text.setText(ss.str().c_str());
		}
		else
		{
			text.setText(_game->getLanguage()->getString(i->getId()).arg(1 + _groundOffset / _groundSlotsX).arg(1 + _xMax / _groundSlotsX));
		}
		text.blit(_gridLabels->getSurface());
	}
}

/**
 * Draws the items contained in the soldier's inventory.
 */
void Inventory::drawItems()
{
	const int Pulsate[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };
	const SavedBattleGame* save = _game->getSavedGame()->getSavedBattle();
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
		for (auto* invItem : *_selUnit->getInventory())
		{
			const Surface *frame = invItem->getBigSprite(texture, save, _animFrame);

			if (invItem == _selItem || !frame)
				continue;

			int x, y;
			if (invItem->getSlot()->getType() == INV_SLOT)
			{
				x = (invItem->getSlot()->getX() + invItem->getSlotX() * RuleInventory::SLOT_W);
				y = (invItem->getSlot()->getY() + invItem->getSlotY() * RuleInventory::SLOT_H);
			}
			else if (invItem->getSlot()->getType() == INV_HAND)
			{
				x = (invItem->getSlot()->getX() + invItem->getRules()->getHandSpriteOffX());
				y = (invItem->getSlot()->getY() + invItem->getRules()->getHandSpriteOffY());
			}
			else
			{
				continue;
			}
			BattleItem::ScriptFill(&work, invItem, save, BODYPART_ITEM_INVENTORY, _animFrame, 0);
			work.executeBlit(frame, _items, x, y, 0);

			// two-handed indicator
			if (invItem->getSlot()->getType() == INV_HAND)
			{
				if (invItem->getRules()->isTwoHanded() || invItem->getRules()->isBlockingBothHands())
				{
					NumberText text = NumberText(10, 5, 0, 0);
					text.setPalette(getPalette());
					text.setColor(invItem->getRules()->isBlockingBothHands() ? _twoHandedRed : _twoHandedGreen);
					text.setBordered(false);
					text.setX(invItem->getSlot()->getX() + RuleInventory::HAND_W * RuleInventory::SLOT_W - 5);
					text.setY(invItem->getSlot()->getY() + RuleInventory::HAND_H * RuleInventory::SLOT_H - 7);
					text.setValue(2);
					text.blit(_items->getSurface());
				}
			}

			// grenade primer indicators
			if (invItem->getFuseTimer() >= 0 && invItem->getRules()->getInventoryWidth() > 0)
			{
				primers(x, y, invItem->isFuseEnabled());
			}
		}

		Surface stackLayer(getWidth(), getHeight(), 0, 0);
		stackLayer.setPalette(getPalette());

		// Ground items
		int fatalWounds = 0;
		auto& occupiedSlots = *clearOccupiedSlotsCache();
		for (auto* groundItem : *_selUnit->getTile()->getInventory())
		{
			const Surface *frame = groundItem->getBigSprite(texture, save, _animFrame);
			// note that you can make items invisible by setting their width or height to 0 (for example used with tank corpse items)
			if (groundItem == _selItem || groundItem->getRules()->getInventoryHeight() == 0 || groundItem->getRules()->getInventoryWidth() == 0 || !frame)
				continue;

			// check if item is in visible range
			if (groundItem->getSlotX() < _groundOffset || groundItem->getSlotX() >= _groundOffset + _groundSlotsX)
				continue;

			// check if something was draw here before
			auto& pos = occupiedSlots[groundItem->getSlotY()][groundItem->getSlotX() - _groundOffset];
			if (pos)
			{
				continue;
			}
			else
			{
				pos = true;
			}

			int x, y;
			x = (groundItem->getSlot()->getX() + (groundItem->getSlotX() - _groundOffset) * RuleInventory::SLOT_W);
			y = (groundItem->getSlot()->getY() + groundItem->getSlotY() * RuleInventory::SLOT_H);
			BattleItem::ScriptFill(&work, groundItem, save, BODYPART_ITEM_INVENTORY, _animFrame, 0);
			work.executeBlit(frame, _items, x, y, 0);

			// grenade primer indicators
			if (groundItem->getFuseTimer() >= 0 && groundItem->getRules()->getInventoryWidth() > 0)
			{
				primers(x, y, groundItem->isFuseEnabled());
			}

			// fatal wounds
			fatalWounds = 0;
			if (groundItem->getUnit())
			{
				// don't show on dead units
				if (groundItem->getUnit()->getStatus() == STATUS_UNCONSCIOUS && groundItem->getUnit()->indicatorsAreEnabled())
				{
					fatalWounds = groundItem->getUnit()->getFatalWounds();
					if (_burnIndicator && groundItem->getUnit()->getFire() > 0)
					{
						indicators(_burnIndicator, x, y);
					}
					else if (_woundIndicator && fatalWounds > 0)
					{
						indicators(_woundIndicator, x, y);
					}
					else if (_shockIndicator && groundItem->getUnit()->hasNegativeHealthRegen())
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
				_stackNumber->setX((groundItem->getSlot()->getX() + ((groundItem->getSlotX() + groundItem->getRules()->getInventoryWidth()) - _groundOffset) * RuleInventory::SLOT_W)-4);
				if (fatalWounds > 9)
				{
					_stackNumber->setX(_stackNumber->getX()-4);
				}
				_stackNumber->setY((groundItem->getSlot()->getY() + (groundItem->getSlotY() + groundItem->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H)-6);
				_stackNumber->setValue(fatalWounds);
				_stackNumber->draw();
				_stackNumber->setColor(color2);
				_stackNumber->blit(stackLayer.getSurface());
			}

			// item stacking
			if (_stackLevel[groundItem->getSlotX()][groundItem->getSlotY()] > 1)
			{
				_stackNumber->setX((groundItem->getSlot()->getX() + ((groundItem->getSlotX() + groundItem->getRules()->getInventoryWidth()) - _groundOffset) * RuleInventory::SLOT_W)-4);
				if (_stackLevel[groundItem->getSlotX()][groundItem->getSlotY()] > 9)
				{
					_stackNumber->setX(_stackNumber->getX()-4);
				}
				_stackNumber->setY((groundItem->getSlot()->getY() + (groundItem->getSlotY() + groundItem->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H)-6);
				_stackNumber->setValue(_stackLevel[groundItem->getSlotX()][groundItem->getSlotY()]);
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
		_selItem->getRules()->drawHandSprite(_game->getMod()->getSurfaceSet("BIGOBS.PCK"), _selection, _selItem, _game->getSavedGame()->getSavedBattle(), _animFrame);
	}
}

/**
 * Clear all occupied slots markers.
 */
std::vector<std::vector<char>>* Inventory::clearOccupiedSlotsCache()
{
	for (auto& v : _occupiedSlotsCache)
	{
		for (auto& b : v)
		{
			b = false;
		}
	}
	return &_occupiedSlotsCache;
}

/**
 * Moves an item to a specified slot in the
 * selected player's inventory.
 * @param item Pointer to battle item.
 * @param slot Inventory slot, or NULL if none.
 * @param x X position in slot.
 * @param y Y position in slot.
 */
void Inventory::moveItem(BattleItem *item, const RuleInventory *slot, int x, int y)
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
bool Inventory::overlapItems(BattleUnit *unit, BattleItem *item, const RuleInventory *slot, int x, int y)
{
	if (slot->getType() != INV_GROUND)
	{
		for (const auto* bi : *unit->getInventory())
		{
			if (bi->getSlot() == slot && bi->occupiesSlot(x, y, item))
			{
				return true;
			}
		}
	}
	else if (unit->getTile() != 0)
	{
		for (const auto* bi : *unit->getTile()->getInventory())
		{
			if (bi->occupiesSlot(x, y, item))
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
	for (const auto& pair : *_game->getMod()->getInventories())
	{
		if (pair.second->checkSlotInPosition(x, y))
		{
			return pair.second;
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
	drawItems();
	if (_tu)
	{
		drawGridLabels(!Options::oxceDisableInventoryTuCost);
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
#ifdef __MOBILE__
	_longPressTimer->think(NULL, this);
#endif
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
	_gridLabels->blitNShade(this, 0, 0);
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

	if (CrossPlatform::getPointerState(0, 0) && _clicked)
	{
		int mx = action->getDetails()->motion.x;
		int my = action->getDetails()->motion.y;
		if ((std::abs(mx - _xBeforeDrag) + std::abs(my - _yBeforeDrag)) > Options::dragScrollPixelTolerance)
		{
			if (!_dragging)
			{
				action->setMouseAction(_xBeforeDrag + action->getLeftBlackBand(),
					_yBeforeDrag + action->getTopBlackBand(),
					action->getSender()->getX(),
					action->getSender()->getY());
				mouseClick(action, state);
				_dragging = true;
				//_longPressTimer->stop();
				_clicked = false;
			}
		}
	}

	InteractiveSurface::mouseOver(action, state);
}

/**
 * Picks up / drops an item.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Inventory::mouseClick(Action *action, State *state)
{
	if (_game->isLeftClick(action))
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
				if (item != 0)
				{
					if (_game->isShiftPressed())
					{
						bool quickUnload = false;
						bool allowed = true;
						// Quick-unload check
						if (!_tu)
						{
							// Outside of the battlescape, quick-unload:
							// - the weapon is never moved from its original slot
							// - the ammo always drops on the ground
							quickUnload = true;
						}
						else
						{
							if (item->getSlot()->getType() != INV_HAND)
							{
								// During the battle, only weapons held in hand can be shift-unloaded
								allowed = false;
							}
						}
						if (allowed)
						{
							_selItem = item; // don't worry, we'll unselect it later!
							if (unload(quickUnload))
							{
								_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_DROP)->play();
							}
							_selItem = 0; // see, I told you!
						}
					}
					else if (item->getRules()->isFixed())
					{
						// do nothing!
					}
					else if (_game->isCtrlPressed())
					{
						const RuleInventory* newSlot = _inventorySlotGround;
						std::string warning = "STR_NOT_ENOUGH_SPACE";
						bool placed = false;

						if (slot->getType() == INV_GROUND)
						{
							// B1 - default slot by item
							if (!placed)
							{
								_stackLevel[item->getSlotX()][item->getSlotY()] -= 1;

								if (item->getRules()->getDefaultInventorySlot() && item->getRules()->getDefaultInventorySlot()->getType() != INV_GROUND)
								{
									newSlot = item->getRules()->getDefaultInventorySlot();

									placed = fitItem(newSlot, item, warning);
								}
							}

							// B2 - slot order by item category
							if (!placed)
							{
								auto* cat = item->getRules()->getFirstCategoryWithInvOrder(_game->getMod());
								if (cat)
								{
									for (const auto& s : cat->getInvOrder())
									{
										if (placed)
										{
											break; // loop finished
										}
										newSlot = _game->getMod()->getInventory(s);
										if (newSlot->getType() == INV_GROUND)
										{
											continue;
										}
										placed = fitItem(newSlot, item, warning);
									}
								}
							}

							// A1 - vanilla default attempt
							if (!placed)
							{
								// reset
								_stackLevel[item->getSlotX()][item->getSlotY()] += 1;
								newSlot = _inventorySlotGround;

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
						}

						if (newSlot->getType() != INV_GROUND)
						{
							// A1 - vanilla default attempt
							if (!placed)
							{
								_stackLevel[item->getSlotX()][item->getSlotY()] -= 1;

								placed = fitItem(newSlot, item, warning);
							}

							if (!placed)
							{
								if (Mod::EXTENDED_INVENTORY_SLOT_SORTING)
								{
									// B3 - fallback: slot order by listOrder
									for (const auto& s : _game->getMod()->getInvsList())
									{
										if (placed)
										{
											break; // loop finished
										}
										newSlot = _game->getMod()->getInventory(s);
										if (newSlot->getType() == INV_GROUND)
										{
											continue;
										}
										placed = fitItem(newSlot, item, warning);
									}
								}
								else
								{
									// A2 - fallback: vanilla alphabetical slot order
									for (const auto& wildCard : *_game->getMod()->getInventories())
									{
										if (placed)
										{
											break; // loop finished
										}
										newSlot = wildCard.second;
										if (newSlot->getType() == INV_GROUND)
										{
											continue;
										}
										placed = fitItem(newSlot, item, warning);
									}
								}
							}
							if (!placed)
							{
								_stackLevel[item->getSlotX()][item->getSlotY()] += 1;
							}
						}
						else
						{
							if (!_tu || _selUnit->spendTimeUnits(item->getMoveToCost(newSlot)))
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
				if (!_selItem->getRules()->canBePlacedIntoInventorySection(slot))
				{
					_warning->showMessage(_game->getLanguage()->getString("STR_CANNOT_PLACE_ITEM_INTO_THIS_SECTION"));
				}
				// Put item in empty slot, or stack it, if possible.
				else if (item == 0 || item == _selItem || canStack)
				{
					if (!overlapItems(_selUnit, _selItem, slot, x, y) && slot->fitItemInSlot(_selItem->getRules(), x, y))
					{
						if (!_tu || _selUnit->spendTimeUnits(_selItem->getMoveToCost(slot)))
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
						if (!_tu || _selUnit->spendTimeUnits(_selItem->getMoveToCost(slot)))
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
					int slotAmmo = item->getRules()->getSlotForAmmo(_selItem->getRules());
					if (slotAmmo == -1)
					{
						_warning->showMessage(_game->getLanguage()->getString("STR_WRONG_AMMUNITION_FOR_THIS_WEAPON"));
					}
					else
					{
						// 4. the cost of loading the weapon with the new ammo (from the offhand)
						int tuCost = item->getRules()->getTULoad(slotAmmo);

						if (Mod::EXTENDED_ITEM_RELOAD_COST && _selItem->getSlot()->getType() != INV_HAND)
						{
							// 3. the cost of moving the new ammo from the current slot to the offhand
							// Note: the cost for left/right hand might *NOT* be the same, but using the right hand "by definition"
							tuCost += _selItem->getMoveToCost(_inventorySlotRightHand);
						}

						BattleItem *weaponRightHand = _selUnit->getRightHandWeapon();
						BattleItem *weaponLeftHand = _selUnit->getLeftHandWeapon();

						auto* oldAmmoGoesTo = _inventorySlotGround;
						if (!weaponRightHand || _selItem == weaponRightHand)
						{
							oldAmmoGoesTo = _inventorySlotRightHand;
						}
						else if (!weaponLeftHand || _selItem == weaponLeftHand)
						{
							oldAmmoGoesTo = _inventorySlotLeftHand;
						}

						bool canLoad = true;
						if (item->getAmmoForSlot(slotAmmo) != 0)
						{
							int tuUnload = item->getRules()->getTUUnload(slotAmmo);
							if (_game->isShiftPressed() && (!_tu || tuUnload))
							{
								// Quick-swap check
								if (!_tu)
								{
									// Outside of the battlescape, the old ammo always drops on the ground
									oldAmmoGoesTo = _inventorySlotGround;
								}
								else
								{
									// During the battle, only weapons held in hand can use ammo quick-swap
									if (item->getSlot()->getType() != INV_HAND)
									{
										canLoad = false;
									}
								}

								// 1. the cost of unloading the old ammo (to the offhand)
								tuCost += tuUnload;
								if (oldAmmoGoesTo == _inventorySlotGround)
								{
									// 2. the cost of dropping the old ammo on the ground (from the offhand)
									// Note: the cost for left/right hand is (should be) the same, so just using the right hand
									tuCost += _inventorySlotRightHand->getCost(_inventorySlotGround);
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
								bool arrangeFloor = false;
								auto* oldAmmo = item->setAmmoForSlot(slotAmmo, _selItem);
								if (oldAmmo)
								{
									moveItem(oldAmmo, oldAmmoGoesTo, 0, 0);
									if (oldAmmoGoesTo == _inventorySlotGround)
									{
										arrangeFloor = true;
									}
								}

								int sound = _selItem->getRules()->getReloadSound();
								if (sound == Mod::NO_SOUND)
								{
									sound = item->getRules()->getReloadSound();
								}
								if (sound == Mod::NO_SOUND)
								{
									sound = Mod::ITEM_RELOAD;
								}

								setSelectedItem(0);
								_game->getMod()->getSoundByDepth(_depth, sound)->play();
								if (arrangeFloor || item->getSlot()->getType() == INV_GROUND)
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
						if (!_tu || _selUnit->spendTimeUnits(_selItem->getMoveToCost(slot)))
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
	else if (_game->isRightClick(action))
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
										playSound(item->getRules()->getPrimeSound()); // prime sound
									}
								}
								else
								{
									_warning->showMessage(_game->getLanguage()->getString(item->getRules()->getUnprimeActionMessage()));
									item->setFuseTimer(-1);  // Unprime the grenade
									arrangeGround();
									playSound(item->getRules()->getUnprimeSound()); // unprime sound
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
	else if (_game->isMiddleClick(action))
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
				std::string articleId = item->getRules()->getUfopediaType();
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
 * @param quickUnload Quick unload using specific rules (the rules are different in and outside of the battlescape)
 * @return The success of the weapon being unloaded.
 */
bool Inventory::unload(bool quickUnload)
{
	// Must be holding an item
	if (_selItem == 0)
	{
		return false;
	}

	const BattleType type = _selItem->getRules()->getBattleType();
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
		if (_selItem->getRules()->getFuseTimerType() == BFT_NONE)
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

			int tu = _selItem->getRules()->getTUUnload(slot);
			if (tu == 0 && !_tu)
			{
				return false;
			}

			auto* ammo = _selItem->getAmmoForSlot(slot);
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
		// History lesson:
		// The old shift-unload logic by Yankes (i.e. unload ammo slots in reverse order) was removed from here,
		// because the new shift-unload logic by Fonzo/Ohartenstein now has a completely different meaning (i.e. quick-unload)
		{
			for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
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

	// Simplified logic for quick-unload outside of the battlescape
	if (quickUnload && !_tu)
	{
		// noop(); // 1. do not move the weapon at all!
		if (grenade)
		{
			_selItem->setFuseTimer(-1);
			_warning->showMessage(_game->getLanguage()->getString(_selItem->getRules()->getUnprimeActionMessage()));
			playSound(_selItem->getRules()->getUnprimeSound()); // unprime sound
		}
		else
		{
			auto* oldAmmo = _selItem->setAmmoForSlot(slotForAmmoUnload, nullptr);
			moveItem(oldAmmo, _inventorySlotGround, 0, 0); // 2. + 3. always drop the ammo on the ground
			arrangeGround();
		}
		setSelectedItem(0);
		return true;
	}

	// Check which hands are free.
	RuleInventory *FirstFreeHand = _inventorySlotRightHand;
	RuleInventory *SecondFreeHand = _inventorySlotLeftHand;

	for (auto* bi : *_selUnit->getInventory())
	{
		if (bi->getSlot()->getType() == INV_HAND && bi != _selItem)
		{
			if (bi->getSlot() == SecondFreeHand)
			{
				SecondFreeHand = nullptr;
			}
			if (bi->getSlot() == FirstFreeHand)
			{
				FirstFreeHand = nullptr;
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
		// 2. unload (= move the ammo to the second free hand)
		cost.Time += toForAmmoUnload;

		if (SecondFreeHand == nullptr)
		{
			// 3. drop the ammo on the ground (if the second hand is not free)
			cost.Time += FirstFreeHand->getCost(_inventorySlotGround);
		}
	}

	if (cost.haveTU() && _selItem->getSlot()->getType() != INV_HAND)
	{
		// 1. move the weapon to the first free hand
		cost.Time += _selItem->getMoveToCost(FirstFreeHand);
	}

	std::string err;
	if (!_tu || cost.spendTU(&err))
	{
		moveItem(_selItem, FirstFreeHand, 0, 0); // 1.
		if (grenade)
		{
			_selItem->setFuseTimer(-1);
			_warning->showMessage(_game->getLanguage()->getString(_selItem->getRules()->getUnprimeActionMessage()));
			playSound(_selItem->getRules()->getUnprimeSound()); // unprime sound
		}
		else
		{
			auto* oldAmmo = _selItem->setAmmoForSlot(slotForAmmoUnload, nullptr);
			if (SecondFreeHand != nullptr)
			{
				moveItem(oldAmmo, SecondFreeHand, 0, 0); // 2.
			}
			else
			{
				moveItem(oldAmmo, _inventorySlotGround, 0, 0); // 2. + 3.
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
		for (const auto& itemCategoryName : item->getRules()->getCategories())
		{
			std::string catLocalName = _game->getLanguage()->getString(itemCategoryName);
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
				for (const auto& itemAmmoCategoryName : item->getAmmoForSlot(slot)->getRules()->getCategories())
				{
					std::string catLocalName = _game->getLanguage()->getString(itemAmmoCategoryName);
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

	int slotsX = _groundSlotsX;
	int slotsY = _groundSlotsY;
	int x = 0;
	int y = 0;
	bool donePlacing = false;
	bool canPlace = false;
	_xMax = 0;
	_stackLevel.clear();

	if (_selUnit != 0)
	{
		std::unordered_map< std::string, std::vector< std::vector<BattleItem*> > > typeItemLists; // Lists of items of the same type
		std::vector<BattleItem*> itemListOrder; // Placement order of item type stacks.
		std::vector< std::vector<int> > startIndexCacheX; // Cache for skipping to last known available position of a given size.
		// Create chart of free slots for later rapid lookup.
		auto& occupiedSlots = *clearOccupiedSlotsCache();

		// Move items out of the way and find which stack they'll end up in within the inventory.
		for (auto& bi : *(_selUnit->getTile()->getInventory()))
		{
			// first move all items out of the way - a big number in X direction
			bi->setSlot(ground);
			bi->setSlotX(1000000);
			bi->setSlotY(0);

			// Only handle visible items from this point on.
			if (bi->getRules()->getInventoryWidth())
			{
				// Each item type has a list of stacks. Find / create a suitable one for this item.
				auto iterItemList = typeItemLists.find(bi->getRules()->getType());
				if (iterItemList == typeItemLists.end()) {
					// New item type, create a list of item stacks for it.
					iterItemList = typeItemLists.insert(std::pair<std::string, std::vector< std::vector<BattleItem*> > >(bi->getRules()->getType(), { { bi } })).first;
					itemListOrder.push_back(bi);
					// Figure out the largest item size present:
					x = std::max(x, bi->getRules()->getInventoryWidth());
					y = std::max(y, bi->getRules()->getInventoryHeight());
				}
				else {
					// With the item type found. Find which stack of this item type to add the item to.
					bool stacked = false;
					for (auto& itemStack : iterItemList->second)
					{
						if (canBeStacked(bi, itemStack.at(0)))
						{
							itemStack.push_back(bi);
							stacked = true;
							break;
						}
					}
					if (!stacked) {
						// Not able to be stacked with previously placed items of this type. Make a new stack containing only this item.
						iterItemList->second.push_back({ bi });
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
		for (auto& bi2 : itemListOrder)
		{
			// Fetch the list of item stacks for this item type. Then place each stack.
			auto iterItemList = typeItemLists.find(bi2->getRules()->getType());
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
				x = std::max(_xMax - slotsX/2, startIndexCacheX[itemTypeSample->getRules()->getInventoryHeight()][itemTypeSample->getRules()->getInventoryWidth()]);
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
						_xMax = std::max(_xMax, x + itemTypeSample->getRules()->getInventoryWidth());
						if ( (x + startIndexCacheX[0].size() ) >= occupiedSlots[0].size())
						{
							// Filled enough for the widest item to potentially request occupancy checks outside of current cache. Expand slot cache.
							size_t newCacheSize = occupiedSlots[0].size() * 2;
							for (auto j = occupiedSlots.begin(); j != occupiedSlots.end(); ++j) {
								j->resize(newCacheSize, 0);
							}
						}
						// Reserve the slots this item will occupy.
						for (int xd = 0; xd < itemTypeSample->getRules()->getInventoryWidth() && canPlace; xd++)
						{
							for (int yd = 0; yd < itemTypeSample->getRules()->getInventoryHeight() && canPlace; yd++)
							{
								occupiedSlots[y + yd][x + xd] = 1;
							}
						}

						// Place all items from this stack in the location we just reserved.
						for (auto& bi3 : itemStack)
						{
							bi3->setSlotX(x);
							bi3->setSlotY(y);
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
		if (_xMax >= _groundOffset + slotsX)
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
			while (_xMax >= _groundOffset + slotsX)
			{
				_groundOffset += slotsX;
			}
		}
	}
	drawItems();
	drawGridLabels(!Options::oxceDisableInventoryTuCost);
}

/**
 * Attempts to place the item in the inventory slot.
 * @param newSlot Where to place the item.
 * @param item Item to be placed.
 * @param warning Warning message if item could not be placed.
 * @return True, if the item was successfully placed in the inventory.
 */
bool Inventory::fitItem(const RuleInventory *newSlot, BattleItem *item, std::string &warning)
{
	// Check if this inventory section supports the item
	if (!item->getRules()->canBePlacedIntoInventorySection(newSlot))
	{
		warning = "STR_CANNOT_PLACE_ITEM_INTO_THIS_SECTION";
		return false;
	}

	bool placed = false;
	int maxSlotX = 0;
	int maxSlotY = 0;
	for (const auto& slotInfo : *newSlot->getSlots())
	{
		if (slotInfo.x > maxSlotX) maxSlotX = slotInfo.x;
		if (slotInfo.y > maxSlotY) maxSlotY = slotInfo.y;
	}
	for (int y2 = 0; y2 <= maxSlotY && !placed; ++y2)
	{
		for (int x2 = 0; x2 <= maxSlotX && !placed; ++x2)
		{
			if (!overlapItems(_selUnit, item, newSlot, x2, y2) && newSlot->fitItemInSlot(item->getRules(), x2, y2))
			{
				if (!_tu || _selUnit->spendTimeUnits(item->getMoveToCost(newSlot)))
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
		auto* ammoA = itemA->getAmmoForSlot(slot);
		auto* ammoB = itemB->getAmmoForSlot(slot);
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
		// and if it's a medikit, it has the same number of charges
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

/**
 * Play a sound.
 */
void Inventory::playSound(int sound)
{
	if (sound != Mod::NO_SOUND)
	{
		_game->getMod()->getSoundByDepth(_depth, sound)->play();
	}
}

#ifdef __MOBILE__
void Inventory::mousePress(Action *action, State *state)
{
	//Log(LOG_INFO) << "Long press timer started";
	// This should only be called when we're in pre-battle inventory.
	_xBeforeDrag = action->getDetails()->button.x;
	_yBeforeDrag = action->getDetails()->button.y;
	_dragging = false;
	_clicked = true;
	if (!_tu)
	{
		// Prepare our fake action
		if (_longPressAction != NULL)
		{
			delete _longPressAction;
		}
		_longPressEvent = *(action->getDetails());
		//Log(LOG_INFO) << "Event type: " << fakeEvent.type;
		//if (fakeEvent.type == SDL_MOUSEBUTTONDOWN)
		//{
		//	Log(LOG_INFO) << "(it's MouseButtonDown)";
		//}
		_longPressAction = new Action(&_longPressEvent, action->getXScale(), action->getYScale(), action->getTopBlackBand(), action->getLeftBlackBand());
		_longPressAction->setMouseAction(action->getXMouse() + action->getLeftBlackBand(),
					 action->getYMouse() + action->getTopBlackBand(),
					 action->getSender()->getX(),
					 action->getSender()->getY());
		_longPressAction->setSender(action->getSender());
		_longPressState = state;
		// And start the timer
		_longPressTimer->start();
	}
}

void Inventory::mouseRelease(Action *action, State *state)
{
	_clicked = false;
	//Log(LOG_INFO) << "Long press timer stopped";
	if (_dragging)
	{
		_dragging = false;
	}
	if (!_tu)
	{
		_longPressTimer->stop();
		if (_longPressAction != NULL)
		{
			//mouseClick(_longPressAction, _longPressState);
			delete _longPressAction;
			_longPressAction = NULL;
		}
	}
}

void Inventory::longPressAction()
{
//	Log(LOG_INFO) << "Long press action called";
//	_longPressAction->getDetails()->button.button = SDL_BUTTON_LEFT;
//	mouseClick(_longPressAction, _longPressState);
	_clicked = false;
	if (!_dragging)
	{
		_longPressAction->getDetails()->button.button = SDL_BUTTON_RIGHT;
		mouseClick(_longPressAction, _longPressState);
	}
	_longPressTimer->stop();
	delete _longPressAction;
	_longPressAction = NULL;
}

#endif
}
