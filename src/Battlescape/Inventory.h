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
#include "../Engine/InteractiveSurface.h"
#include <locale>
#include <map>
#include <string>

namespace OpenXcom
{

class RuleInventory;
class Game;
class WarningMessage;
class BattleItem;
class BattleUnit;
class NumberText;
class SurfaceSet;
class Timer;

/**
 * Interactive view of an inventory.
 * Lets the player view and manage a soldier's equipment.
 */
class Inventory : public InteractiveSurface
{
private:
	Game *_game;
	Surface *_grid, *_items, *_gridLabels;
	/// surface for the currently select item, if any. Equal in size to a hand-slot.
	Surface *_selection;
	/// Surface set containing big objects (inventory sprite pictures).
	SurfaceSet* _bigObs = nullptr;

	WarningMessage *_warning;
	BattleUnit *_selUnit;
	BattleItem *_selItem;
	bool _tu, _base;
	BattleItem *_mouseOverItem;
	int _groundOffset, _animFrame;
	std::map<int, std::map<int, int> > _stackLevel;
	std::vector<std::vector<char>> _occupiedSlotsCache;
	NumberText *_stackNumber;
	std::string _searchString;
	Timer *_animTimer;
	int _depth, _groundSlotsX, _groundSlotsY;
	RuleInventory *_inventorySlotRightHand = nullptr;
	RuleInventory *_inventorySlotLeftHand = nullptr;
	RuleInventory *_inventorySlotBackPack = nullptr;
	RuleInventory *_inventorySlotBelt = nullptr;
	RuleInventory *_inventorySlotGround = nullptr;

	/// Clear all occupied slots markers.
	std::vector<std::vector<char>>* clearOccupiedSlotsCache();
	/// Moves an item to a specified slot.
	void moveItem(BattleItem *item, RuleInventory *slot, int x, int y);
	/// Gets the slot in the specified position.
	RuleInventory *getSlotInPosition(int *x, int *y) const;
	/// Play a sound.
	void playSound(int sound);
public:
	/// Creates a new inventory view at the specified position and size.
	Inventory(Game *game, int width, int height, int x = 0, int y = 0, bool base = false);
	/// Cleans up the inventory.
	~Inventory();
	/// Sets the inventory's palette.
	void setPalette(const SDL_Color *colors, int firstcolor = 0, int ncolors = 256) override;
	/// Sets the inventory's Time Unit mode.
	void setTuMode(bool tu);
	/// Gets the inventory's selected unit.
	BattleUnit *getSelectedUnit() const;
	/// Sets the inventory's selected unit.
	void setSelectedUnit(BattleUnit *unit, bool resetGroundOffset);
	/// Draws the inventory.
	void draw() override;
	/// Draws the inventory grid.
	void drawGrid();
	/// Draws the inventory grid labels.
	void drawGridLabels(bool showTuCost = false);
	/// Draws the inventory items.
	void drawItems();
	/// Draws the selected item.
	void drawSelectedItem();
	/// Gets the currently selected item.
	BattleItem *getSelectedItem() const;
	/// Sets the currently selected item.
	void setSelectedItem(BattleItem *item);
	/// Sets the search string.
	void setSearchString(const std::string &searchString);
	/// Gets the mouse over item.
	BattleItem *getMouseOverItem() const;
	/// Sets the mouse over item.
	void setMouseOverItem(BattleItem *item);
	/// Handles timers.
	void think() override;
	/// Blits the inventory onto another surface.
	void blit(SDL_Surface *surface) override;
	/// Special handling for mouse hovers.
	void mouseOver(Action *action, State *state) override;
	/// Special handling for mouse clicks.
	void mouseClick(Action *action, State *state) override;
	/// Unloads the selected weapon.
	bool unload(bool quickUnload = false);
	/// Checks whether the given item is visible with the current search string.
	bool isInSearchString(BattleItem *item);
	/// Arranges items on the ground.
	void arrangeGround(int alterOffset = 0);
	/// Attempts to place an item in an inventory slot.
	bool fitItem(RuleInventory *newSlot, BattleItem *item, std::string &warning);
	/// Checks if two items can be stacked on one another.
	bool canBeStacked(BattleItem *itemA, BattleItem *itemB);
	/// Checks for item overlap.
	static bool overlapItems(BattleUnit *unit, BattleItem *item, RuleInventory *slot, int x = 0, int y = 0);
	/// Shows a warning message.
	void showWarning(const std::string &msg);
	/// Animate surface.
	void animate();
	/// Get current animation frame for inventory.
	int getAnimFrame() const { return _animFrame; }
};

}
