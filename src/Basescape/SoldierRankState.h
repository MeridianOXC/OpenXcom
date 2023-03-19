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
#include "../SaveGame/Soldier.h"
#include <vector>

namespace OpenXcom
{

struct RankItem
{
	RankItem(const SoldierRank _rank, const std::string& _name, int _openings, bool _promotionAllowed)
		: rank(_rank), name(_name), openings(_openings), promotionAllowed(_promotionAllowed)
	{
	}
	SoldierRank rank;
	int openings;
	std::string name;
	bool promotionAllowed;
};

class Base;
class TextButton;
class Window;
class Text;
class TextEdit;
class TextList;

class SoldierRankState : public State
{
private:
	Base *_base;
	size_t _soldierId;

	TextButton *_btnCancel;
	Window *_window;
	Text *_txtTitle, *_txtRank, *_txtOpening;
	TextEdit *_btnQuickSearch;
	TextList *_lstRank;
	std::vector<RankItem> _ranks;
	std::vector<int> _indices;

  public:
	/// Creates the Soldier Rank state.
	SoldierRankState(Base *base, size_t soldier);
	/// Updates the rank list.
	void updateList();
	/// togges the quick search visible and not
	void btnQuickSearchToggle(Action *action);
	/// applies the quick search
	void btnQuickSearchApply(Action *);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Handler for clicking the Rank list (promotes if possible).
	void lstRankClick(Action *action);
	/// Handler for middle clicking the Rank list (ufopedia article if possible).
	void lstRankClickMiddle(Action *action);
	/// Gets the currently selected Rank Item
	const RankItem &getSelectedRankItem() const;
};

}
