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
#include "../Engine/State.h"
#include "../Savegame/Soldier.h"

namespace OpenXcom
{

class Base;
class Soldiers;
class Window;
class Text;
class TextList;
class ArrowButton;
class TextButton;
class ToggleTextButton;
class RuleManufacture;
class Production;
class Timer;
class InteractiveSurface;

/**
 * Screen that allows changing of Production settings (assigned engineer, units to build).
 */
class ManufactureInfoStateFtA : public State
{
private:
	Base * _base;
	RuleManufacture * _item;
	Production * _production;
	bool _ftaUi, _newProject;
	std::vector<Soldier *> _engineers;
	Window * _window;
	ArrowButton * _btnUnitUp, * _btnUnitDown;
	TextButton * _btnStop, * _btnOk, *_btnAllocateEngineers;
	Text * _txtTitle, * _txtAvailableEngineer, * _txtAvailableSpace, * _txtAllocatedEngineers, *_txtGrade, *_txtAvgEfficiency, * _txtAvgDiligence;
	Text * _txtUnitToProduce, * _txtUnitUp, * _txtUnitDown, * _txtTodo;
	Timer *_timerMoreUnit, * _timerLessUnit;
	InteractiveSurface *_surfaceUnits;
	TextList * _lstEngineers;
	int _unitsToProduce;
	int _producedItems;
	bool _infiniteProduction;
	/// Handler for the Stop button.
	void btnStopClick (Action * action);
	/// Handler for the OK button.
	void btnOkClick (Action * action);
	/// Handler for the Allocate engineers button.
	void btnAllocateClick(Action* action);
	/// Adds given number of units to produce to the project if possible.
	void moreUnit(int change);
	/// Handler for pressing the more unit button.
	void moreUnitPress(Action * action);
	/// Handler for releasing the more unit button.
	void moreUnitRelease(Action * action);
	/// Handler for clicking the more unit button.
	void moreUnitClick(Action * action);
	/// Removes the given number of units to produce from the project if possible.
	void lessUnit(int change);
	/// Handler for pressing the less unit button.
	void lessUnitPress(Action * action);
	/// Handler for releasing the less unit button.
	void lessUnitRelease(Action * action);
	/// Handler for clicking the less unit button.
	void lessUnitClick(Action * action);
	/// Increases count of number of units to make.
	void onMoreUnit();
	/// Decreases count of number of units to make (if possible).
	void onLessUnit();
	/// Handler for using the mouse wheel on the Unit-part of the screen.
	void handleWheelUnit(Action *action);
	/// Updates display of assigned/available engineers and workshop space.
	void setAssignedEngineer();
	/// Runs state functionality every cycle.
	void think() override;
	/// Builds the User Interface.
	void buildUi();
	/// Helper to exit the State.
	void exitState();
	int calcAvgStat(bool check);
public:
	/// Creates the State (new production).
	ManufactureInfoStateFtA(Base * base, RuleManufacture * _item);
	/// Creates the State (modify production).
	ManufactureInfoStateFtA(Base * base, Production * production);
	/// Cleans up the state
	~ManufactureInfoStateFtA();
	/// Updates the state.
	void init() override;
	void fillEngineersList(size_t scrl);

	const RuleManufacture* getManufactureRules();
	std::vector<Soldier*> getEngineers() { return _engineers; };
	void addEngineer(Soldier* engineer) { _engineers.push_back(engineer); }
	void removeEngineer(Soldier* engineer);
	void setEngineers(std::vector<Soldier*> engineers) { _engineers = engineers; };
};

}
