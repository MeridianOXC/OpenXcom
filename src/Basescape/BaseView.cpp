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
#include "BaseView.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include "../Engine/SurfaceSet.h"
#include "../Engine/Action.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Savegame/Craft.h"
#include "../Interface/Text.h"
#include "../Engine/Timer.h"
#include "../Engine/Options.h"
#include <climits>

namespace OpenXcom
{

/**
 * Sets up a base view with the specified size and position.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
BaseView::BaseView(int width, int height, int x, int y) : InteractiveSurface(width, height, x, y),
	_base(0), _texture(0), _selFacility(0), _big(0), _small(0), _lang(0),
	_gridX(0), _gridY(0), _selSizeX(0), _selSizeY(0),
	_selector(0), _blink(true),
	_redColor(0), _yellowColor(0), _greenColor(0), _highContrast(true),
	_cellColor(0), _selectorColor(0)
{
	// Clear grid
	for (int i = 0; i < BASE_SIZE; ++i)
	{
		for (int j = 0; j < BASE_SIZE; ++j)
		{
			_facilities[i][j] = 0;
		}
	}

	_timer = new Timer(100);
	_timer->onTimer((SurfaceHandler)&BaseView::blink);
	_timer->start();
}

/**
 * Deletes contents.
 */
BaseView::~BaseView()
{
	delete _selector;
	delete _timer;
}

/**
 * Changes the various resources needed for text rendering.
 * The different fonts need to be passed in advance since the
 * text size can change mid-text, and the language affects
 * how the text is rendered.
 * @param big Pointer to large-size font.
 * @param small Pointer to small-size font.
 * @param lang Pointer to current language.
 */
void BaseView::initText(Font *big, Font *small, Language *lang)
{
	_big = big;
	_small = small;
	_lang = lang;
}

/**
 * Changes the current base to display and
 * initializes the internal base grid.
 * @param base Pointer to base to display.
 */
void BaseView::setBase(Base *base)
{
	_base = base;
	_selFacility = 0;

	// Clear grid
	for (int x = 0; x < BASE_SIZE; ++x)
	{
		for (int y = 0; y < BASE_SIZE; ++y)
		{
			_facilities[x][y] = 0;
		}
	}

	// Fill grid with base facilities
	for (auto* fac : *_base->getFacilities())
	{
		for (int y = fac->getY(); y < fac->getY() + fac->getRules()->getSizeY(); ++y)
		{
			for (int x = fac->getX(); x < fac->getX() + fac->getRules()->getSizeX(); ++x)
			{
				_facilities[x][y] = fac;
			}
		}
	}

	_redraw = true;
}

/**
 * Changes the texture to use for drawing
 * the various base elements.
 * @param texture Pointer to SurfaceSet to use.
 */
void BaseView::setTexture(SurfaceSet *texture)
{
	_texture = texture;
}

/**
 * Returns the facility the mouse is currently over.
 * @return Pointer to base facility (0 if none).
 */
BaseFacility *BaseView::getSelectedFacility() const
{
	return _selFacility;
}

/**
 * Prevents any mouseover bugs on dismantling base facilities before setBase has had time to update the base.
 */
void BaseView::resetSelectedFacility()
{
	_facilities[_selFacility->getX()][_selFacility->getY()] = 0;
	_selFacility = 0;
}


/**
 * Returns the X position of the grid square
 * the mouse is currently over.
 * @return X position on the grid.
 */
int BaseView::getGridX() const
{
	return _gridX;
}

/**
 * Returns the Y position of the grid square
 * the mouse is currently over.
 * @return Y position on the grid.
 */
int BaseView::getGridY() const
{
	return _gridY;
}

/**
 * If enabled, the base view will respond to player input,
 * highlighting the selected facility.
 * @param size Facility length (0 disables it).
 */
void BaseView::setSelectable(int sizeX, int sizeY)
{
	_selSizeX = sizeX;
	_selSizeY = sizeY;
	if (_selSizeX > 0 && _selSizeY > 0)
	{
		_selector = new Surface(sizeX * GRID_SIZE, sizeY * GRID_SIZE, _x, _y);
		_selector->setPalette(getPalette());
		SDL_Rect r;
		r.w = _selector->getWidth();
		r.h = _selector->getHeight();
		r.x = 0;
		r.y = 0;
		_selector->drawRect(&r, _selectorColor);
		r.w -= 2;
		r.h -= 2;
		r.x++;
		r.y++;
		_selector->drawRect(&r, 0);
		_selector->setVisible(false);
	}
	else
	{
		delete _selector;
	}
}

/**
 * Returns if a certain facility can be successfully
 * placed on the currently selected square.
 * @param rule Facility type.
 * @param facilityBeingMoved Selected facility.
 * @param isStartFacility Is this a start facility?
 * @return 0 if placeable, otherwise error code for why we couldn't place it
 * 1: not connected to lift or on top of another facility (standard OXC behavior)
 * 2: trying to upgrade over existing facility, but it's in use
 * 3: trying to upgrade over existing facility, but it's already being upgraded
 * 4: trying to upgrade over existing facility, but size/placement mismatch
 * 5: trying to upgrade over existing facility, but ruleset of new facility requires a specific existing facility
 * 6: trying to upgrade over existing facility, but ruleset disallows it
 * 7: trying to upgrade over existing facility, but all buildings next to it are under construction and build queue is off
 */
BasePlacementErrors BaseView::getPlacementError(const RuleBaseFacility *rule, BaseFacility *facilityBeingMoved, bool isStartFacility) const
{
	// We'll need to know for the final check if we're upgrading an existing facility
	bool buildingOverExisting = false;

	// Area where we want to place a new building
	const BaseAreaSubset placementArea = BaseAreaSubset(rule->getSizeX(), rule->getSizeY()).offset(_gridX, _gridY);
	// Whole base
	const BaseAreaSubset baseArea = BaseAreaSubset(BASE_SIZE, BASE_SIZE);

	// Check if the facility fits inside the base boundaries
	if (BaseAreaSubset::intersection(placementArea, baseArea) != placementArea)
	{
		return BPE_NotConnected;
	}

	// Check usage of facilites in the area that will be replaced by a new building
	if (facilityBeingMoved == nullptr)
	{
		BasePlacementErrors areaUseError = _base->isAreaInUse(placementArea, rule);
		if (areaUseError != BPE_None)
		{
			return areaUseError;
		}
	}

	// Check if square isn't occupied
	for (int y = placementArea.beg_y; y < placementArea.end_y; ++y)
	{
		for (int x = placementArea.beg_x; x < placementArea.end_x; ++x)
		{
			BaseFacility* facility = _facilities[x][y];
			if (facility != 0)
			{
				if (isStartFacility)
				{
					return BPE_NotConnected;
				}
				// when moving an existing facility, it should not block itself
				if (facilityBeingMoved == nullptr)
				{
					// Further check to see if the facility already there can be built over and we're not removing an important base function
					BasePlacementErrors canBuildOverError = rule->getCanBuildOverOtherFacility(facility->getRules());
					if (canBuildOverError != BPE_None)
					{
						return canBuildOverError;
					}

					// Make sure the facility we're building over is entirely within the size of the one we're checking
					const BaseAreaSubset removedArea = facility->getPlacement();
					if (BaseAreaSubset::intersection(placementArea, removedArea) != removedArea)
					{
						return BPE_UpgradeSizeMismatch;
					}

					// Make sure this facility is not already being upgraded
					if (facility->getIfHadPreviousFacility() && facility->getBuildTime() != 0)
					{
						return BPE_Upgrading;
					}

					buildingOverExisting = true;
				}
				else if (facility != facilityBeingMoved)
				{
					return BPE_NotConnected;
				}
			}
		}
	}

	bool bq=Options::allowBuildingQueue;
	bool hasConnectingFacility = false;

	// Check for another facility to connect to
	for (int i = 0; i < rule->getSizeX(); ++i)
	{
		// top
		if (_gridY > 0 && _facilities[_gridX + i][_gridY - 1] != 0)
		{
			hasConnectingFacility = true;
			if ((!buildingOverExisting && bq) || _facilities[_gridX + i][_gridY - 1]->isBuiltOrHadPreviousFacility())
				return BPE_None;
		}
		// bottom
		if (_gridY + rule->getSizeY() < BASE_SIZE && _facilities[_gridX + i][_gridY + rule->getSizeY()] != 0)
		{
			hasConnectingFacility = true;
			if ((!buildingOverExisting && bq) || _facilities[_gridX + i][_gridY + rule->getSizeY()]->isBuiltOrHadPreviousFacility())
				return BPE_None;
		}
	}
	for (int i = 0; i < rule->getSizeY(); ++i)
	{
		// left
		if (_gridX > 0 && _facilities[_gridX - 1][_gridY + i] != 0)
		{
			hasConnectingFacility = true;
			if ((!buildingOverExisting && bq) || _facilities[_gridX - 1][_gridY + i]->isBuiltOrHadPreviousFacility())
				return BPE_None;
		}
		// right
		if (_gridX + rule->getSizeX() < BASE_SIZE && _facilities[_gridX + rule->getSizeX()][_gridY + i] != 0)
		{
			hasConnectingFacility = true;
			if ((!buildingOverExisting && bq) || _facilities[_gridX + rule->getSizeX()][_gridY + i]->isBuiltOrHadPreviousFacility())
				return BPE_None;
		}
	}

	// We can assume if we've reached this point that none of the connecting facilities are finished!
	if (hasConnectingFacility && (!bq || buildingOverExisting))
		return BPE_Queue;

	return BPE_NotConnected;
}

/**
 * Returns if the placed facility is placed in queue or not.
 * @param rule Facility type.
 * @return True if queued, False otherwise.
 */
bool BaseView::isQueuedBuilding(const RuleBaseFacility *rule) const
{
	for (int i = 0; i < rule->getSizeX(); ++i)
	{
		if ((_gridY > 0 && _facilities[_gridX + i][_gridY - 1] != 0 && _facilities[_gridX + i][_gridY - 1]->isBuiltOrHadPreviousFacility()) ||
			(_gridY + rule->getSizeY() < BASE_SIZE && _facilities[_gridX + i][_gridY + rule->getSizeY()] != 0 && _facilities[_gridX + i][_gridY + rule->getSizeY()]->isBuiltOrHadPreviousFacility()))
		{
			return false;
		}
	}
	for (int i = 0; i < rule->getSizeY(); ++i)
	{
		if ((_gridX > 0 && _facilities[_gridX - 1][_gridY + i] != 0 && _facilities[_gridX - 1][_gridY + i]->isBuiltOrHadPreviousFacility()) ||
			(_gridX + rule->getSizeX() < BASE_SIZE && _facilities[_gridX + rule->getSizeX()][_gridY + i] != 0 && _facilities[_gridX + rule->getSizeX()][_gridY + i]->isBuiltOrHadPreviousFacility()))
		{
			return false;
		}
	}
	return true;
}

/**
 * ReCalculates the remaining build-time of all queued buildings.
 */
void BaseView::reCalcQueuedBuildings()
{
	setBase(_base);
	std::vector<BaseFacility*> facilities;
	for (auto* fac : *_base->getFacilities())
	{
		if (fac->getAdjustedBuildTime() > 0)
		{
			// Set all queued buildings to infinite.
			if (fac->getAdjustedBuildTime() > fac->getRules()->getBuildTime())
			{
				fac->setBuildTime(INT_MAX);
			}
			facilities.push_back(fac);
		}
	}

	// Applying a simple Dijkstra Algorithm
	while (!facilities.empty())
	{
		auto min = facilities.begin();
		for (auto it = facilities.begin(); it != facilities.end(); ++it)
		{
			if ((*it)->getAdjustedBuildTime() < (*min)->getAdjustedBuildTime())
			{
				min = it;
			}
		}
		BaseFacility* facility=(*min);
		facilities.erase(min);
		const RuleBaseFacility *rule=facility->getRules();
		int x=facility->getX(), y=facility->getY();
		for (int i = 0; i < rule->getSizeX(); ++i)
		{
			if (y > 0) updateNeighborFacilityBuildTime(facility,_facilities[x + i][y - 1]);
			if (y + rule->getSizeY() < BASE_SIZE) updateNeighborFacilityBuildTime(facility,_facilities[x + i][y + rule->getSizeY()]);
		}
		for (int i = 0; i < rule->getSizeY(); ++i)
		{
			if (x > 0) updateNeighborFacilityBuildTime(facility, _facilities[x - 1][y + i]);
			if (x + rule->getSizeX() < BASE_SIZE) updateNeighborFacilityBuildTime(facility, _facilities[x + rule->getSizeX()][y + i]);
		}
	}
}

/**
 * Updates the neighborFacility's build time. This is for internal use only (reCalcQueuedBuildings()).
 * @param facility Pointer to a base facility.
 * @param neighbor Pointer to a neighboring base facility.
 */
void BaseView::updateNeighborFacilityBuildTime(BaseFacility* facility, BaseFacility* neighbor)
{
	if (facility != 0 && neighbor != 0
	&& neighbor->getAdjustedBuildTime() > neighbor->getRules()->getBuildTime()
	&& facility->getAdjustedBuildTime() + neighbor->getRules()->getBuildTime() < neighbor->getAdjustedBuildTime())
		neighbor->setBuildTime(facility->getAdjustedBuildTime() + neighbor->getRules()->getBuildTime());
}

/**
 * Keeps the animation timers running.
 */
void BaseView::think()
{
	_timer->think(0, this);
}

/**
 * Makes the facility selector blink.
 */
void BaseView::blink()
{
	_blink = !_blink;

	if (_selSizeX > 0 && _selSizeY > 0)
	{
		SDL_Rect r;
		if (_blink)
		{
			r.w = _selector->getWidth();
			r.h = _selector->getHeight();
			r.x = 0;
			r.y = 0;
			_selector->drawRect(&r, _selectorColor);
			r.w -= 2;
			r.h -= 2;
			r.x++;
			r.y++;
			_selector->drawRect(&r, 0);
		}
		else
		{
			r.w = _selector->getWidth();
			r.h = _selector->getHeight();
			r.x = 0;
			r.y = 0;
			_selector->drawRect(&r, 0);
		}
	}
}

/**
 * Draws the view of all the facilities in the base, connectors
 * between them and crafts landed in hangars.
 */
void BaseView::draw()
{
	Surface::draw();

	// Draw grid squares
	for (int x = 0; x < BASE_SIZE; ++x)
	{
		for (int y = 0; y < BASE_SIZE; ++y)
		{
			Surface *frame = _texture->getFrame(0);
			int fx = (x * GRID_SIZE);
			int fy = (y * GRID_SIZE);
			frame->blitNShade(this, fx, fy);
		}
	}

	auto craftIt = _base->getCrafts()->begin();

	for (const auto* fac : *_base->getFacilities())
	{
		// Draw facility shape
		int num = 0;
		for (int y = fac->getY(); y < fac->getY() + fac->getRules()->getSizeY(); ++y)
		{
			for (int x = fac->getX(); x < fac->getX() + fac->getRules()->getSizeX(); ++x)
			{
				Surface *frame;

				int outline = fac->getRules()->isSmall() ? 3 : fac->getRules()->getSizeX() * fac->getRules()->getSizeY();
				if (fac->getBuildTime() == 0)
					frame = _texture->getFrame(fac->getRules()->getSpriteShape() + num);
				else
					frame = _texture->getFrame(fac->getRules()->getSpriteShape() + num + outline);

				int fx = (x * GRID_SIZE);
				int fy = (y * GRID_SIZE);
				frame->blitNShade(this, fx, fy);

				num++;
			}
		}
	}

	for (const auto* fac : *_base->getFacilities())
	{
		// Draw connectors
		if (fac->isBuiltOrHadPreviousFacility() && !fac->getRules()->connectorsDisabled())
		{
			// Facilities to the right
			int x = fac->getX() + fac->getRules()->getSizeX();
			if (x < BASE_SIZE)
			{
				for (int y = fac->getY(); y < fac->getY() + fac->getRules()->getSizeY(); ++y)
				{
					if (_facilities[x][y] != 0 && _facilities[x][y]->isBuiltOrHadPreviousFacility() && !_facilities[x][y]->getRules()->connectorsDisabled())
					{
						Surface *frame = _texture->getFrame(7);
						int fx = (x * GRID_SIZE - GRID_SIZE / 2);
						int fy = (y * GRID_SIZE);
						frame->blitNShade(this, fx, fy);
					}
				}
			}

			// Facilities to the bottom
			int y = fac->getY() + fac->getRules()->getSizeY();
			if (y < BASE_SIZE)
			{
				for (int subX = fac->getX(); subX < fac->getX() + fac->getRules()->getSizeX(); ++subX)
				{
					if (_facilities[subX][y] != 0 && _facilities[subX][y]->isBuiltOrHadPreviousFacility() && !_facilities[subX][y]->getRules()->connectorsDisabled())
					{
						Surface *frame = _texture->getFrame(8);
						int fx = (subX * GRID_SIZE);
						int fy = (y * GRID_SIZE - GRID_SIZE / 2);
						frame->blitNShade(this, fx, fy);
					}
				}
			}
		}
	}

	// TODO: make const in the future
	for (auto* fac : *_base->getFacilities())
	{
		// Draw facility graphic
		int num = 0;
		for (int y = fac->getY(); y < fac->getY() + fac->getRules()->getSizeY(); ++y)
		{
			for (int x = fac->getX(); x < fac->getX() + fac->getRules()->getSizeX(); ++x)
			{
				if (fac->getRules()->getSpriteEnabled())
				{
					Surface *frame = _texture->getFrame(fac->getRules()->getSpriteFacility() + num);
					int fx = (x * GRID_SIZE);
					int fy = (y * GRID_SIZE);
					frame->blitNShade(this, fx, fy);
				}

				num++;
			}
		}

		// Draw crafts
		fac->setCraftForDrawing(0);
		if (fac->getBuildTime() == 0 && fac->getRules()->getCrafts() > 0)
		{
			if (craftIt != _base->getCrafts()->end())
			{
				if ((*craftIt)->getStatus() != "STR_OUT")
				{
					Surface *frame = _texture->getFrame((*craftIt)->getSkinSprite() + 33);
					int fx = (fac->getX() * GRID_SIZE + (fac->getRules()->getSizeX() - 1) * GRID_SIZE / 2 + 2);
					int fy = (fac->getY() * GRID_SIZE + (fac->getRules()->getSizeY() - 1) * GRID_SIZE / 2 - 4);
					frame->blitNShade(this, fx, fy);
					fac->setCraftForDrawing(*craftIt);
				}
				++craftIt;
			}
		}

		// Draw time remaining
		if (fac->getBuildTime() > 0 || fac->getDisabled())
		{
			Text *text = new Text(GRID_SIZE * fac->getRules()->getSizeX(), 16, 0, 0);
			text->setPalette(getPalette());
			text->initText(_big, _small, _lang);
			text->setX(fac->getX() * GRID_SIZE);
			text->setY(fac->getY() * GRID_SIZE + (GRID_SIZE * fac->getRules()->getSizeY() - 16) / 2);
			text->setBig();
			std::ostringstream ss;
			if (fac->getDisabled())
				ss << "X";
			else
				ss << fac->getBuildTime();
			if (fac->getIfHadPreviousFacility()) // Indicate that this facility still counts for connectivity
				ss << "*";
			text->setAlign(ALIGN_CENTER);
			text->setColor(_cellColor);
			text->setText(ss.str());
			text->blit(this->getSurface());
			delete text;
		}

		// Draw ammo indicator
		if (fac->getBuildTime() == 0 && fac->getRules()->getAmmoMax() > 0)
		{
			Text* text = new Text(GRID_SIZE * fac->getRules()->getSizeX(), 9, 0, 0);
			text->setPalette(getPalette());
			text->initText(_big, _small, _lang);
			text->setX(fac->getX() * GRID_SIZE);
			text->setY(fac->getY() * GRID_SIZE);
			text->setHighContrast(_highContrast);
			if (fac->getAmmo() >= fac->getRules()->getAmmoMax())
				text->setColor(_greenColor); // 100%
			else if (fac->getAmmo() <= fac->getRules()->getAmmoMax() / 2)
				text->setColor(_redColor); // 0-50%
			else
				text->setColor(_yellowColor); // 51-99%
			std::ostringstream ss;
			ss << fac->getAmmo() << "/" << fac->getRules()->getAmmoMax();
			text->setText(ss.str());
			text->blit(this->getSurface());
			delete text;
		}
	}
}

/**
 * Blits the base view and selector.
 * @param surface Pointer to surface to blit onto.
 */
void BaseView::blit(SDL_Surface *surface)
{
	Surface::blit(surface);
	if (_selector != 0)
	{
		_selector->blit(surface);
	}
}

/**
 * Selects the facility the mouse is over.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void BaseView::mouseOver(Action *action, State *state)
{
	_gridX = (int)floor(action->getRelativeXMouse() / (GRID_SIZE * action->getXScale()));
	_gridY = (int)floor(action->getRelativeYMouse() / (GRID_SIZE * action->getYScale()));
	if (_gridX >= 0 && _gridX < BASE_SIZE && _gridY >= 0 && _gridY < BASE_SIZE)
	{
		_selFacility = _facilities[_gridX][_gridY];
		if (_selSizeX > 0 && _selSizeY > 0)
		{
			if (_gridX + _selSizeX - 1 < BASE_SIZE && _gridY + _selSizeY - 1 < BASE_SIZE)
			{
				_selector->setX(_x + _gridX * GRID_SIZE);
				_selector->setY(_y + _gridY * GRID_SIZE);
				_selector->setVisible(true);
			}
			else
			{
				_selector->setVisible(false);
			}
		}
	}
	else
	{
		_selFacility = 0;
		if (_selSizeX > 0 && _selSizeY > 0)
		{
			_selector->setVisible(false);
		}
	}

	InteractiveSurface::mouseOver(action, state);
}

/**
 * Deselects the facility.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void BaseView::mouseOut(Action *action, State *state)
{
	_selFacility = 0;
	if (_selSizeX > 0 && _selSizeY > 0)
	{
		_selector->setVisible(false);
	}

	InteractiveSurface::mouseOut(action, state);
}

void BaseView::setColor(Uint8 color)
{
	_cellColor = color;
}
void BaseView::setSecondaryColor(Uint8 color)
{
	_selectorColor = color;
}
void BaseView::setOtherColors(Uint8 red, Uint8 yellow, Uint8 green, bool highContrast)
{
	_redColor = red;
	_yellowColor = yellow;
	_greenColor = green;
	_highContrast = highContrast;
}

}
