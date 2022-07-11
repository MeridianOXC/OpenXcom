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
#include "ManufactureInfoStateFtA.h"
#include <algorithm>
#include "../Interface/Window.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Interface/ArrowButton.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Unicode.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleManufacture.h"
#include "../Savegame/Base.h"
#include "../Savegame/Production.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Soldier.h"
#include "../Engine/Timer.h"
#include "../Menu/ErrorMessageState.h"
#include "../Mod/RuleInterface.h"
#include "../Basescape/ManufactureAllocateEngineers.h"
#include <climits>

namespace OpenXcom
{

/**
 * Initializes all elements in the Production settings screen (new Production).
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param item The RuleManufacture to produce.
 */
ManufactureInfoStateFtA::ManufactureInfoStateFtA(Base *base, RuleManufacture *item) : _base(base), _item(item), _production(0)
{
	_newProject = true;
	_unitsToProduce = 0;
	_producedItems = 0;
	_infiniteProduction = false;
	buildUi();
}

/**
 * Initializes all elements in the Production settings screen (modifying Production).
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param production The Production to modify.
 */
ManufactureInfoStateFtA::ManufactureInfoStateFtA(Base *base, Production *production) : _base(base), _item(0), _production(production)
{
	_newProject = false;
	_unitsToProduce = _production->getAmountTotal();
	_producedItems = -_production->getAmountProduced();
	_infiniteProduction = _production->getInfiniteAmount();
	buildUi();
}

/**
 * Builds screen User Interface.
 */
void ManufactureInfoStateFtA::buildUi()
{
	_screen = false;
	_ftaUi = _game->getMod()->getIsFTAGame();

	_window = new Window(this, 320, 160, 0, 20, POPUP_BOTH);
	_txtTitle = new Text(302, 17, 9, 30);
	_btnOk = new TextButton(136, 16, 168, 155);
	_btnStop = new TextButton(136, 16, 16, 155);
	_txtAvailableEngineer = new Text(160, 9, 16, 50);
	_txtAvailableSpace = new Text(160, 9, 16, 60);
	_txtAllocatedEngineers = new Text(150, 9, 16, 70);
	_txtUnitToProduce = new Text(112, 48, 168, 64);
	_txtUnitUp = new Text(90, 9, 192, 118);
	_txtUnitDown = new Text(90, 9, 192, 138);
	_btnUnitUp = new ArrowButton(ARROW_BIG_UP, 13, 14, 284, 114);
	_btnUnitDown = new ArrowButton(ARROW_BIG_DOWN, 13, 14, 284, 136);
	_txtTodo = new Text(40, 16, 280, 88);

	_txtAvgEfficiency = new Text(143, 9, 168, 50);
	_txtAvgDiligence = new Text(143, 9, 168, 59);
	_btnAllocateEngineers = new TextButton(110, 16, 16, 80);
	_txtGrade = new Text(32, 9, 132, 88);
	_lstEngineers = new TextList(148, 57, 16, 97);

	_surfaceUnits = new InteractiveSurface(160, 150, 160, 25);
	_surfaceUnits->onMouseClick((ActionHandler)&ManufactureInfoStateFtA::handleWheelUnit, 0);

	// Set palette
	setInterface("manufactureInfo");

	add(_surfaceUnits);
	add(_window, "window", "manufactureInfo");
	add(_txtTitle, "text", "manufactureInfo");
	add(_txtAvailableEngineer, "text", "manufactureInfo");
	add(_txtAvailableSpace, "text", "manufactureInfo");
	add(_txtAllocatedEngineers, "text", "manufactureInfo");
	add(_txtUnitToProduce, "text", "manufactureInfo");
	add(_txtTodo, "text", "manufactureInfo");
	add(_txtUnitUp, "text", "manufactureInfo");
	add(_txtUnitDown, "text", "manufactureInfo");
	add(_btnUnitUp, "button1", "manufactureInfo");
	add(_btnUnitDown, "button1", "manufactureInfo");
	add(_btnOk, "button2", "manufactureInfo");
	add(_btnStop, "button2", "manufactureInfo");
	add(_txtAvgEfficiency, "text", "manufactureInfo");
	add(_txtAvgDiligence, "text", "manufactureInfo");
	add(_btnAllocateEngineers, "button2", "manufactureInfo");
	add(_txtGrade, "text", "manufactureInfo");
	add(_lstEngineers, "list", "manufactureInfo");

	centerAllSurfaces();

	setWindowBackground(_window, "manufactureInfo");

	_txtTitle->setText(tr(getManufactureRules()->getName()));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtTodo->setBig();

	_txtUnitToProduce->setText(tr("STR_UNITS_TO_PRODUCE"));
	_txtUnitToProduce->setBig();
	_txtUnitToProduce->setWordWrap(true);
	_txtUnitToProduce->setVerticalAlign(ALIGN_BOTTOM);

	_btnUnitUp->onMousePress((ActionHandler)&ManufactureInfoStateFtA::moreUnitPress);
	_btnUnitUp->onMouseRelease((ActionHandler)&ManufactureInfoStateFtA::moreUnitRelease);
	_btnUnitUp->onMouseClick((ActionHandler)&ManufactureInfoStateFtA::moreUnitClick, 0);

	_btnUnitDown->onMousePress((ActionHandler)&ManufactureInfoStateFtA::lessUnitPress);
	_btnUnitDown->onMouseRelease((ActionHandler)&ManufactureInfoStateFtA::lessUnitRelease);
	_btnUnitDown->onMouseClick((ActionHandler)&ManufactureInfoStateFtA::lessUnitClick, 0);

	_txtUnitUp->setText(tr("STR_INCREASE_UC"));

	_txtUnitDown->setText(tr("STR_DECREASE_UC"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ManufactureInfoStateFtA::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ManufactureInfoStateFtA::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&ManufactureInfoStateFtA::btnOkClick, Options::keyCancel);

	if (!_newProject)
	{
		if (getManufactureRules()->getRefund())
		{
			_btnStop->setText(tr("STR_REFUND_PRODUCTION"));
		}
		else
		{
			_btnStop->setText(tr("STR_STOP_PRODUCTION"));
		}
	}
	else
	{
		_btnStop->setText(tr("STR_CANCEL_UC"));
	}
	_btnStop->onMouseClick((ActionHandler)&ManufactureInfoStateFtA::btnStopClick);

	setAssignedEngineer();

	_btnAllocateEngineers->setText(tr("STR_ALLOCATE_ENGINEERS"));
	_btnAllocateEngineers->onMouseClick((ActionHandler)&ManufactureInfoStateFtA::btnAllocateClick, 0);
	_txtGrade->setText(tr("STR_GRADE_UC"));

	_lstEngineers->setColumns(2, 116, 32);
	_lstEngineers->setAlign(ALIGN_LEFT, 0);
	_lstEngineers->setBackground(_window);
	//_lstEngineers->setMargin(2);
	_lstEngineers->setWordWrap(true);

	_timerMoreUnit = new Timer(250);
	_timerLessUnit = new Timer(250);

	_timerMoreUnit->onTimer((StateHandler)&ManufactureInfoStateFtA::onMoreUnit);
	_timerLessUnit->onTimer((StateHandler)&ManufactureInfoStateFtA::onLessUnit);
}

/**
 * Frees up memory that's not automatically cleaned on exit
 */
ManufactureInfoStateFtA::~ManufactureInfoStateFtA()
{
	delete _timerMoreUnit;
	delete _timerLessUnit;
}

void ManufactureInfoStateFtA::init()
{
	State::init();
	setAssignedEngineer();
	fillEngineersList(0);
}

void ManufactureInfoStateFtA::fillEngineersList(size_t scrl)
{
	_lstEngineers->clearList();
	for (auto e : _engineers)
	{
		std::ostringstream ss;
		ss << e->getRoleRank(ROLE_ENGINEER);
		_lstEngineers->addRow(2, e->getName().c_str(), ss.str().c_str());
		//_lstEngineers->addRow(1, e->getName().c_str());
	}
}

const RuleManufacture* ManufactureInfoStateFtA::getManufactureRules()
{
	if (_item != nullptr)
	{
		return _item;
	}
	else
	{
		return _production->getRules();
	}
}

void ManufactureInfoStateFtA::removeEngineer(Soldier* engineer)
{
	auto iter = std::find(std::begin(_engineers), std::end(_engineers), engineer);
	for (int k = 0; k < _engineers.size(); k++)
	{
		if (_engineers[k] == engineer)
		{
			_engineers.erase(_engineers.begin() + k);
		}
	}
}

/**
 * Stops this Production. Returns to the previous screen.
 * @param action A pointer to an Action.
 */
void ManufactureInfoStateFtA::btnStopClick(Action *)
{
	if (!_newProject && getManufactureRules()->getRefund())
	{
		_production->refundItem(_base, _game->getSavedGame(), _game->getMod());
	}

	for (auto s : _engineers)
	{
		s->setProductionProject(0);
	}

	_base->removeProduction(_production);
	exitState();
}

/**
 * Starts this Production (if new). Returns to the previous screen.
 * @param action A pointer to an Action.
 */
void ManufactureInfoStateFtA::btnOkClick(Action *)
{
	int efficiency = 0;
	for (auto s : _engineers)
	{
		efficiency += s->getCurrentStats()->efficiency;
		s->setProductionProject(_production);
	}

	if (_newProject && !_engineers.empty())
	{
		_production = new Production(_item, 1);
		_base->addProduction(_production);
		_production->startItem(_base, _game->getSavedGame(), _game->getMod());
	}

	if (_engineers.size() > 0)
	{
		_production->setEfficiency(efficiency / _engineers.size());
	}
	else
	{
		_production->setEfficiency(100);
	}

	_production->setSellItems(false);
	_production->setAmountTotal(_unitsToProduce);
	_production->setInfiniteAmount(_infiniteProduction);
	_production->setAssignedEngineers(0); //this is FtA, baby! we use soldiers, assigned to the project instead.
	exitState();
}

void ManufactureInfoStateFtA::btnAllocateClick(Action* action)
{
	_game->pushState(new ManufactureAllocateEngineers(_base, this));
}

/**
 * Returns to the previous screen.
 */
void ManufactureInfoStateFtA::exitState()
{
	_game->popState();
	if (_item)
	{
		_game->popState();
	}
}

int ManufactureInfoStateFtA::calcAvgStat(bool check)
{
	double result = 0;
	if (_engineers.size() > 0)
	{
		
		for (auto s : _engineers)
		{
			if (check) //efficiency; im so sorry if you can see this =)
			{
				result += s->getStatsWithAllBonuses()->efficiency;
			}
			else
			{
				result += s->getStatsWithAllBonuses()->diligence;
			}
		}

		result /= _engineers.size();
	}
	return static_cast<int>(result);
}

/**
 * Updates display of assigned/available engineer/workshop space.
 */
void ManufactureInfoStateFtA::setAssignedEngineer()
{
	size_t baseEngineers = 0;
	for (auto s : *_base->getSoldiers())
	{
		if (s->getRoleRank(ROLE_ENGINEER) > 0)
		{
			if (s->getProductionProject() != 0 && s->getProductionProject()->getRules() == this->getManufactureRules())
			{
				_engineers.push_back(s);
			}
			else
			{
				baseEngineers++;
			}
		}
	}
	_txtAvailableEngineer->setText(tr("STR_ENGINEERS_AVAILABLE_UC").arg(baseEngineers));
	_txtAvailableSpace->setText(tr("STR_WORKSHOP_SPACE_AVAILABLE_UC").arg(_base->getFreeWorkshops(true)));

	std::ostringstream s4;
	s4 << ">" << Unicode::TOK_COLOR_FLIP;
	if (_infiniteProduction)
	{
		s4 << "∞";
	}
	else
	{
		s4 << _unitsToProduce;
	}
	_txtTodo->setText(s4.str());

	_txtAllocatedEngineers->setText(tr("STR_ENGINEERS_ALLOCATED_UC").arg(_engineers.size()));
	_txtAvgDiligence->setText(tr("STR_AVERAGE_DILIGENCE_UC").arg(calcAvgStat(false)));
	_txtAvgEfficiency->setText(tr("STR_AVERAGE_EFFICIENCY_UC").arg(calcAvgStat(true)));

	if (_engineers.empty())
	{
		_txtAvgDiligence->setVisible(false);
		_txtAvgEfficiency->setVisible(false);
	}
}


/**
 * Adds given number of units to produce to the project if possible.
 * @param change How much we want to add.
 */
void ManufactureInfoStateFtA::moreUnit(int change)
{
	if (change <= 0)
	{
		return;
	}

	if (this->getManufactureRules()->getProducedCraft() && _base->getAvailableHangars() - _base->getUsedHangars() <= 0)
	{
		_timerMoreUnit->stop();
		_game->pushState(new ErrorMessageState(tr("STR_NO_FREE_HANGARS_FOR_CRAFT_PRODUCTION"), _palette, _game->getMod()->getInterface("basescape")->getElement("errorMessage")->color, "BACK17.SCR", _game->getMod()->getInterface("basescape")->getElement("errorPalette")->color));
	}
	else
	{
		int units = _unitsToProduce;
		if (units == 1 && change > 1)
		{
			--change; // e.g. jump from 1 to 10, not to 11
		}
		change = std::min(INT_MAX - units, change);
		if (this->getManufactureRules()->getProducedCraft())
			change = std::min(_base->getAvailableHangars() - _base->getUsedHangars(), change);
		_unitsToProduce = units + change;
		setAssignedEngineer();
	}
}

/**
 * Starts the timerMoreUnit.
 * @param action A pointer to an Action.
 */
void ManufactureInfoStateFtA::moreUnitPress(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && _unitsToProduce < INT_MAX)
		_timerMoreUnit->start();
}

/**
 * Stops the timerMoreUnit.
 * @param action A pointer to an Action.
 */
void ManufactureInfoStateFtA::moreUnitRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerMoreUnit->setInterval(250);
		_timerMoreUnit->stop();
	}
}

/**
 * Increases the "units to produce", in the case of a right-click, to infinite, and 1 on left-click.
 * @param action A pointer to an Action.
 */
void ManufactureInfoStateFtA::moreUnitClick(Action *action)
{
	if (_infiniteProduction)
	{
		return; // We can't increase over infinite :)
	}

	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (this->getManufactureRules()->getProducedCraft())
		{
			moreUnit(INT_MAX);
		}
		else
		{
			_infiniteProduction = true;
			setAssignedEngineer();
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moreUnit(1);
	}
}

/**
 * Removes the given number of units to produce from the project if possible.
 * @param change How much we want to subtract.
 */
void ManufactureInfoStateFtA::lessUnit(int change)
{
	if (change <= 0)
	{
		return;
	}

	int units = _unitsToProduce;
	change = std::min(units - (_producedItems + 1), change);
	_unitsToProduce = units - change;
	setAssignedEngineer();
}

/**
 * Starts the timerLessUnit.
 * @param action A pointer to an Action.
 */
void ManufactureInfoStateFtA::lessUnitPress(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT) _timerLessUnit->start();
}

/**
 * Stops the timerLessUnit.
 * @param action A pointer to an Action.
 */
void ManufactureInfoStateFtA::lessUnitRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerLessUnit->setInterval(250);
		_timerLessUnit->stop();
	}
}

/**
 * Decreases the units to produce.
 * @param action A pointer to an Action.
 */
void ManufactureInfoStateFtA::lessUnitClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT
	||  action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		bool wasInfinite = _infiniteProduction;
		_infiniteProduction = false;
		if (action->getDetails()->button.button == SDL_BUTTON_RIGHT
		|| _unitsToProduce <= _producedItems)
		{ // So the produced item number is increased over the planned, OR it was simply a right-click
			_unitsToProduce += 1;
			setAssignedEngineer();
		}
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			if (wasInfinite)
			{
				// when infinite amount is decreased by 1, set the amount to maximum possible considering current funds and store supplies
				int productionPossible = INT_MAX;
				if (getManufactureRules()->getManufactureCost() > 0)
				{
					int byFunds = _game->getSavedGame()->getFunds() / getManufactureRules()->getManufactureCost();
					productionPossible = std::min(productionPossible, byFunds);
				}
				for (auto &item : getManufactureRules()->getRequiredItems())
				{
					productionPossible = std::min(productionPossible, _base->getStorageItems()->getItem(item.first) / item.second);
				}
				productionPossible = std::max(0, productionPossible);

				int newTotal = _producedItems + productionPossible;
				if (!_item)
				{
					newTotal += 1; // +1 for the item being produced currently
				}
				_unitsToProduce = newTotal + 1; // +1 because of lessUnit(1) call below
			}
			lessUnit(1);
		}
	}
}

/**
 * Builds one more unit.
 */
void ManufactureInfoStateFtA::onMoreUnit()
{
	_timerMoreUnit->setInterval(50);
	moreUnit(1);
}

/**
 * Builds one less unit( if possible).
 */
void ManufactureInfoStateFtA::onLessUnit()
{
	_timerLessUnit->setInterval(50);
	lessUnit(1);
}

/**
 * Increases or decreases the Units to produce according the mouse-wheel used.
 * @param action A pointer to an Action.
 */
void ManufactureInfoStateFtA::handleWheelUnit(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		moreUnit(Options::changeValueByMouseWheel);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		lessUnit(Options::changeValueByMouseWheel);
}

/**
 * Runs state functionality every cycle (used to update the timer).
 */
void ManufactureInfoStateFtA::think()
{
	State::think();
	_timerMoreUnit->think(this, 0);
	_timerLessUnit->think(this, 0);
}

}
