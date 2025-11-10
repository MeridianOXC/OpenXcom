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
#include "BattlescapeGame.h"

namespace OpenXcom
{

class Game;
class Font;
class Language;
class Text;
class Frame;
class RuleSkill;
class RuleItem;

/**
 * A class that represents a single box in the action popup menu on the battlescape.
 * It shows the possible actions of an item, their TU cost and accuracy.
 * Mouse over highlights the action, when clicked the action is sent to the parent state.
 */
class ActionMenuItem : public InteractiveSurface
{
private:
	bool _highlighted;
	BattleActionType _action;
	const RuleSkill* _skill;
	const RuleItem* _linkedItemRules;
	int _tu, _highlightModifier;
	Frame *_frame;
	Text *_txtDescription, *_txtAcc, *_txtTU;
public:
	/// Creates a new ActionMenuItem.
	ActionMenuItem(int id, Game *game, int x, int y);
	/// Cleans up the ActionMenuItem.
	~ActionMenuItem();
	/// Assigns an action to it.
	void setAction(BattleActionType action, const std::string &description, const std::string &accuracy, const std::string &timeunits, int tu);
	void setSkill(const RuleSkill* skill);
	void setLinkedItemRules(const RuleItem* linkedItemRules) { _linkedItemRules = linkedItemRules; }
	/// Gets the assigned action.
	BattleActionType getAction() const;
	/// Gets the assigned skill.
	const RuleSkill* getSkill() const;
	/// Gets the linked item rules.
	const RuleItem* getLinkedItemRules() const { return _linkedItemRules; }
	/// Gets the assigned action TUs.
	int getTUs() const;
	/// Sets the palettes.
	void setPalette(const SDL_Color *colors, int firstcolor, int ncolors) override;
	/// Redraws it.
	void draw() override;
	/// Processes a mouse hover in event.
	void mouseIn(Action *action, State *state) override;
	/// Processes a mouse hover out event.
	void mouseOut(Action *action, State *state) override;

};

}
