#pragma once
/*
 * Copyright 2010-2023 OpenXcom Developers.
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

struct RankItem
{
	RankItem(SoldierRank _rank, const std::string& _name, int _openings, bool _promotionAllowed)
		: rank(_rank), name(_name), openings(_openings), promotionAllowed(_promotionAllowed)
	{
	}
	SoldierRank rank;
	std::string name;
	int openings;
	bool promotionAllowed;
};

class Base;
class Text;
class TextButton;
class TextList;
class Window;

class SoldierRankState : public State
{
private:
	Base *_base;
	size_t _soldierId;

	TextButton *_btnCancel;
	Window *_window;
	Text *_txtTitle, *_txtRank, *_txtOpening;
	TextList *_lstRanks;
	std::vector<RankItem> _ranks;

  public:
	/// Creates the Soldier Rank state.
	SoldierRankState(Base* base, size_t soldier);
	/// Cleans up the Soldier Rank state.
	~SoldierRankState();
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action* action);
	/// Handler for left clicking the ranks list (promotes/demotes if possible).
	void lstRankClick(Action* action);
	/// Handler for middle clicking the ranks list (opens ufopedia article if possible).
	void lstRankClickMiddle(Action* action);
};

}
