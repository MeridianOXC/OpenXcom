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

namespace OpenXcom
{

class RuleInventory;
class Game;
class BattleUnit;
class Timer;

/**
 * View of an alien inventory.
 * Lets the player view alien's equipment (in hand only).
 */
class AlienInventory : public InteractiveSurface
{
private:
	Game *_game;
	Surface *_grid, *_items;
	BattleUnit *_selUnit;
	int _dynamicOffset;
	int _animFrame;
	Timer* _animTimer;
	/// Gets the slot in the specified position.
	RuleInventory *getSlotInPosition(int *x, int *y) const;
public:
	/// Creates a new inventory view at the specified position and size.
	AlienInventory(Game *game, int width, int height, int x = 0, int y = 0);
	/// Cleans up the inventory.
	~AlienInventory();
	/// Sets the inventory's palette.
	void setPalette(const SDL_Color *colors, int firstcolor = 0, int ncolors = 256) override;
	/// Gets the inventory's selected unit.
	BattleUnit *getSelectedUnit() const;
	/// Sets the inventory's selected unit.
	void setSelectedUnit(BattleUnit *unit);
	/// Draws the inventory.
	void draw() override;
	/// Draws the inventory grid.
	void drawGrid();
	/// Draws the inventory items.
	void drawItems() const;
	/// Blits the inventory onto another surface.
	void blit(SDL_Surface *surface) override;
	/// Special handling for mouse clicks.
	void mouseClick(Action *action, State *state) override;
	/// Handles timers.
	void think() override;
	/// Animate surface.
	void animate();
};

}
