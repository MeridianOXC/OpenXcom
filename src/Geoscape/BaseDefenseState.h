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

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class Base;
class Ufo;
class TextList;
class GeoscapeState;
class Timer;
class RuleBaseFacility;

enum BaseDefenseActionType { BDA_NONE, BDA_FIRE, BDA_RESOLVE, BDA_DESTROY, BDA_END };

/**
 * Base Defense Screen for when ufos try to attack.
 */
class BaseDefenseState : public State
{
private:
	TextButton *_btnOk;
	TextButton *_btnStart, *_btnAbort;
	Window *_window;
	InteractiveSurface *_preview;
	Text *_txtTitle, *_txtInit;
	TextList *_lstDefenses;
	Base *_base;
	Ufo *_ufo;
	int _thinkcycles, _row, _passes, _gravShields, _defenses, _attacks, _explosionCount;
	BaseDefenseActionType _action;
	Timer *_timer;
	GeoscapeState *_state;
	bool applyDamage(const RuleBaseFacility* rule);
public:
	/// Creates the Base Defense state.
	BaseDefenseState(Base *base, Ufo *ufo, GeoscapeState *state, bool instaHyper);
	/// Cleans up the Base Defense state.
	~BaseDefenseState();
	/// Handle the Timer.
	void think() override;
	/// do the next step.
	void nextStep();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the [Start] button.
	void btnStartClick(Action *action);
};

}
