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
#include "SoldierRankState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleSoldier.h"
#include "../Savegame/Base.h"
#include "../Savegame/RankCount.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Rank window.
 * @param base Pointer to the base to get info from.
 * @param soldierId ID of the selected soldier.
 */
SoldierRankState::SoldierRankState(Base* base, size_t soldierId) : _base(base), _soldierId(soldierId)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 192, 160, 64, 20, POPUP_BOTH);
	_btnCancel = new TextButton(140, 16, 90, 156);
	_txtTitle = new Text(182, 17, 69, 28);
	_txtRank = new Text(90, 9, 80, 52);
	_txtOpening = new Text(70, 9, 190, 52);
	_lstRanks = new TextList(160, 80, 73, 68);

	// Set palette
	setInterface("soldierRank");

	add(_window, "window", "soldierRank");
	add(_btnCancel, "button", "soldierRank");
	add(_txtTitle, "text", "soldierRank");
	add(_txtRank, "text", "soldierRank");
	add(_txtOpening, "text", "soldierRank");
	add(_lstRanks, "list", "soldierRank");

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

	_lstRanks->setColumns(2, 132, 21);
	_lstRanks->setSelectable(true);
	_lstRanks->setBackground(_window);
	_lstRanks->setMargin(8);

	PromotionOpenings openings = PromotionOpenings(_game->getSavedGame()->getAllActiveSoldiers(), _game->getMod());

	// a copy is unavoidable here because me may want to modify this set.
	std::vector<std::string> rankStringsCopy = soldier->getRules()->getRankStrings();

	// if the rank strings is empty and the soldier is allowed promotion, build a default set of rank strings.
	// ideally this would be encoded in the data someplace instead of relying on this behavior for empty rank strings.
	if (rankStringsCopy.empty() && soldier->getRules()->getAllowPromotion())
	{
		rankStringsCopy = { "STR_ROOKIE", "STR_SQUADDIE", "STR_SERGEANT", "STR_CAPTAIN", "STR_COLONEL", "STR_COMMANDER" };
	}

	// build the promotion list.
	for (const auto& rank : { RANK_ROOKIE, RANK_SQUADDIE, RANK_SERGEANT, RANK_CAPTAIN, RANK_COLONEL, RANK_COMMANDER })
	{
		if ((size_t)rank < rankStringsCopy.size())
		{
			_ranks.push_back(RankItem(rank, rankStringsCopy[rank], openings[rank], openings.isManualPromotionPossible(soldier, rank)));
		}
	}

	for (const auto& rankItem : _ranks)
	{
		const std::string quantityText = rankItem.openings >= 0 ? std::to_string(rankItem.openings) : "-";

		_lstRanks->addRow(2, tr(rankItem.name).c_str(), quantityText.c_str());

		if (!rankItem.promotionAllowed)
		{
			// disabled
			_lstRanks->setCellColor(_lstRanks->getLastRowIndex(), 0, _lstRanks->getSecondaryColor());
		}
	}

	_lstRanks->onMouseClick((ActionHandler)&SoldierRankState::lstRankClick);
	_lstRanks->onMouseClick((ActionHandler)&SoldierRankState::lstRankClickMiddle, SDL_BUTTON_MIDDLE);
}

/**
 *
 */
SoldierRankState::~SoldierRankState()
{
}


/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierRankState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Promotes/demotes the soldier if possible.
 * @param action Pointer to an action.
 */
void SoldierRankState::lstRankClick(Action*)
{
	const RankItem& selectedRank = _ranks[_lstRanks->getSelectedRow()];
	if (selectedRank.promotionAllowed)
	{
		Soldier* soldier = _base->getSoldiers()->at(_soldierId);
		soldier->setRank(selectedRank.rank);

		_game->popState();
	}
}

/**
 * Opens the corresponding Ufopaedia article.
 * @param action Pointer to an action.
 */
void SoldierRankState::lstRankClickMiddle(Action* action)
{
	std::string articleId = _ranks[_lstRanks->getSelectedRow()].name;
	Ufopaedia::openArticle(_game, articleId);
}

}
