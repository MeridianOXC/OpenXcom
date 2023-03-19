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
#include "SoldierRankState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Mod/Mod.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/RankCount.h"
#include "../Savegame/Soldier.h"
#include "../Mod/RuleSoldier.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Rank window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param soldier ID of the selected soldier.
 */
SoldierRankState::SoldierRankState(Base *base, size_t soldierId) : _base(base), _soldierId(soldierId)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 192, 160, 64, 20, POPUP_BOTH);
	_btnQuickSearch = new TextEdit(this, 48, 9, 80, 43);
	_btnCancel = new TextButton(140, 17, 90, 156);
	_txtTitle = new Text(182, 17, 69, 28);
	_txtRank = new Text(90, 9, 80, 52);
	_txtOpening = new Text(70, 9, 190, 52);
	_lstRank = new TextList(160, 80, 73, 68);

	setInterface("soldierRank");

	add(_window, "window", "soldierRank");
	add(_btnCancel, "button", "soldierRank");
	add(_btnQuickSearch, "button", "soldierRank");
	add(_txtTitle, "text", "soldierRank");
	add(_txtRank, "text", "soldierRank");
	add(_txtOpening, "text", "soldierRank");
	add(_lstRank, "list", "soldierRank");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "soldierRank");

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&SoldierRankState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SoldierRankState::btnCancelClick, Options::keyCancel);

	Soldier *soldier = _base->getSoldiers()->at(_soldierId);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PROMOTE_SOLDIER").arg(tr(soldier->getRankString())).arg(soldier->getName()));

	_txtRank->setText(tr("STR_RANK_HEADER"));
	_txtOpening->setText(tr("STR_OPENINGS_HEADER"));
	_txtOpening->setAlign(TextHAlign::ALIGN_CENTER);

	_lstRank->setColumns(2, 132, 21);
	_lstRank->setSelectable(true);
	_lstRank->setBackground(_window);
	_lstRank->setMargin(8);

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&SoldierRankState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	_btnCancel->onKeyboardRelease((ActionHandler)&SoldierRankState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	_lstRank->onMouseClick((ActionHandler)&SoldierRankState::lstRankClick);
	_lstRank->onMouseClick((ActionHandler)&SoldierRankState::lstRankClickMiddle, SDL_BUTTON_MIDDLE);

	// temporary objects for convience.
	const SavedGame *savedGame = _game->getSavedGame();
	const RuleSoldier *soldierRules = soldier->getRules();

	PromotionOpenings openings = PromotionOpenings(savedGame->getAllActiveSoldiers(), _game->getMod());

	// a copy is unavoidable here because me may want to modify this set.
	std::vector<std::string> rankStrings = soldierRules->getRankStrings();

	// if the rank strings is empty and the soldier is allowed promotion, build a default set of rank strings.
	// ideally this would be encoded in the data someplace instead of relying on this behavior for empty rank strings.
	if (rankStrings.empty() && soldierRules->getAllowPromotion())
	{
		rankStrings = {"STR_ROOKIE", "STR_SQUADDIE", "STR_SERGEANT", "STR_CAPTAIN", "STR_COLONEL", "STR_COMMANDER"};
	}

	// build the promotion list.
	for (const auto &rank : {RANK_ROOKIE, RANK_SQUADDIE, RANK_SERGEANT, RANK_CAPTAIN, RANK_COLONEL, RANK_COMMANDER})
	{
		if (rank < rankStrings.size())
		{
			_ranks.push_back(RankItem(rank, rankStrings[rank], openings[rank], openings.isManualPromotionPossible(soldier, rank)));
		}
	}

	updateList();
}

/**
 * Updates the rank list with the current list
 * of available ranks.
 */
void SoldierRankState::updateList()
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	_lstRank->clearList();
	_indices.clear();

	int index = -1;
	for (const auto &rankItem : _ranks)
	{
		++index;

		// quick search
		if (!searchString.empty())
		{
			std::string rankName = tr(rankItem.name);
			Unicode::upperCase(rankName);
			if (rankName.find(searchString) == std::string::npos)
			{
				continue;
			}
		}

		// if the promotion is not allowed, prepend a color flip to indicate this.
		std::string rankText;
		if (!rankItem.promotionAllowed)
		{
			rankText += Unicode::TOK_COLOR_FLIP;
		}
		rankText += tr(rankItem.name);

		const std::string quantityText = rankItem.openings >= 0 ? std::to_string(rankItem.openings)
																: tr("STR_UNLIMITED_QUANTITIES_SYMBOL").c_str();

		_lstRank->addRow(2, rankText.c_str(), quantityText.c_str());
		_indices.push_back(index);
	}
}

/**
 * Quick search toggle.
 * @param action Pointer to an action.
 */
void SoldierRankState::btnQuickSearchToggle(Action *action)
{
	if (_btnQuickSearch->getVisible())
	{
		_btnQuickSearch->setText("");
		_btnQuickSearch->setVisible(false);
		btnQuickSearchApply(action);
	}
	else
	{
		_btnQuickSearch->setVisible(true);
		_btnQuickSearch->setFocus(true);
	}
}

/**
 * Apply Quick search.
 * @param action Pointer to an action.
 */
void SoldierRankState::btnQuickSearchApply(Action *)
{
	updateList();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierRankState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Promotes the soldier if possible.
 * @param action Pointer to an action.
 */
void SoldierRankState::lstRankClick(Action *)
{
	const RankItem &selectedRank = getSelectedRankItem();
	if (selectedRank.promotionAllowed)
	{
		Soldier *soldier = _base->getSoldiers()->at(_soldierId);
		soldier->changeRank(selectedRank.rank);

		_game->popState();
	}
}

/**
 * Shows corresponding Ufopaedia article.
 * @param action Pointer to an action.
 */
void SoldierRankState::lstRankClickMiddle(Action *action)
{
	Ufopaedia::openArticle(_game, getSelectedRankItem().name);
}

/**
 * Gets the currently selected rank item.
 * @return the currently selected rank item.
*/
const RankItem& SoldierRankState::getSelectedRankItem() const
{
	return _ranks[_indices[_lstRank->getSelectedRow()]];
}

}
