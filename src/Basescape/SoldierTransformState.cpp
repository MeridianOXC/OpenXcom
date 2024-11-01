/*
 * Copyright 2010-2024 OpenXcom Developers.
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
#include "SoldierTransformState.h"
#include "SoldierTransformationState.h"
#include <algorithm>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ArrowButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleSoldierTransformation.h"
#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

struct compareTransformationName
{
	bool operator()(const TransformationItem& a, const TransformationItem& b) const
	{
		return Unicode::naturalCompare(a.name, b.name);
	}
};


/**
 * Initializes all the elements in the Soldier Transform window.
 * @param base Pointer to the base to get info from.
 * @param soldier ID of the selected soldier.
 */
SoldierTransformState::SoldierTransformState(Base* base, size_t soldier) : _base(base), _soldier(soldier)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 192, 160, 64, 20, POPUP_BOTH);
	_btnQuickSearch = new TextEdit(this, 48, 9, 80, 43);
	_btnCancel = new TextButton(140, 16, 90, 156);
	_txtTitle = new Text(182, 16, 69, 28);
	_txtType = new Text(90, 9, 80, 52);
	_lstTransformations = new TextList(160, 80, 73, 68);
	_sortName = new ArrowButton(ARROW_NONE, 11, 8, 80, 52);

	// Set palette
	setInterface("soldierTransform");

	add(_window, "window", "soldierTransform");
	add(_btnQuickSearch, "button", "soldierTransform");
	add(_btnCancel, "button", "soldierTransform");
	add(_txtTitle, "text", "soldierTransform");
	add(_txtType, "text", "soldierTransform");
	add(_lstTransformations, "list", "soldierTransform");
	add(_sortName, "text", "soldierTransform");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "soldierTransform");

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&SoldierTransformState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SoldierTransformState::btnCancelClick, Options::keyCancel);

	Soldier *s = _base->getSoldiers()->at(_soldier);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELECT_TRANSFORMATION_FOR").arg(s->getName()));

	_txtType->setText(tr("STR_TYPE"));

	_lstTransformations->setColumns(1, 153);
	_lstTransformations->setSelectable(true);
	_lstTransformations->setBackground(_window);
	_lstTransformations->setMargin(8);

	_sortName->setX(_sortName->getX() + _txtType->getTextWidth() + 4);
	_sortName->onMouseClick((ActionHandler)&SoldierTransformState::sortNameClick);

	std::vector<RuleSoldierTransformation*> availableTransformations;
	_game->getSavedGame()->getAvailableTransformations(availableTransformations, _game->getMod(), _base);

	for (auto* trRule : availableTransformations)
	{
		if (!s->isEligibleForTransformation(trRule))
			continue;

		_transformations.push_back(TransformationItem(trRule->getName(), tr(trRule->getName())));
	}

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&SoldierTransformState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(Options::oxceQuickSearchButton);

	_btnCancel->onKeyboardRelease((ActionHandler)&SoldierTransformState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	_transformationOrder = TRANSFORM_SORT_NONE;
	sortList();

	_lstTransformations->onMouseClick((ActionHandler)&SoldierTransformState::lstTransformationClick);
	_lstTransformations->onMouseClick((ActionHandler)&SoldierTransformState::lstTransformationClickMiddle, SDL_BUTTON_MIDDLE);
}

/**
 *
 */
SoldierTransformState::~SoldierTransformState()
{

}

/**
 * Updates the sorting arrows based on the current setting.
 */
void SoldierTransformState::updateArrows()
{
	_sortName->setShape(ARROW_NONE);
	switch (_transformationOrder)
	{
	case TRANSFORM_SORT_NAME_ASC:
		_sortName->setShape(ARROW_SMALL_UP);
		break;
	case TRANSFORM_SORT_NAME_DESC:
		_sortName->setShape(ARROW_SMALL_DOWN);
		break;
	default:
		break;
	}
}

/**
 * Sorts the transformations list.
 */
void SoldierTransformState::sortList()
{
	updateArrows();

	switch (_transformationOrder)
	{
	case TRANSFORM_SORT_NAME_ASC:
		std::stable_sort(_transformations.begin(), _transformations.end(), compareTransformationName());
		break;
	case TRANSFORM_SORT_NAME_DESC:
		std::stable_sort(_transformations.rbegin(), _transformations.rend(), compareTransformationName());
		break;
	default:
		break;
	}

	updateList();
}

/**
 * Updates the transformations list.
 */
void SoldierTransformState::updateList()
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	_lstTransformations->clearList();
	_indices.clear();

	int index = -1;
	for (const auto& TransformationItem : _transformations)
	{
		++index;

		// quick search
		if (!searchString.empty())
		{
			std::string trType = TransformationItem.name;
			Unicode::upperCase(trType);
			if (trType.find(searchString) == std::string::npos)
			{
				continue;
			}
		}

		_lstTransformations->addRow(1, TransformationItem.name.c_str());
		_indices.push_back(index);
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierTransformState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Quick search toggle.
 * @param action Pointer to an action.
 */
void SoldierTransformState::btnQuickSearchToggle(Action* action)
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
 * Quick search.
 * @param action Pointer to an action.
 */
void SoldierTransformState::btnQuickSearchApply(Action*)
{
	updateList();
}

/**
 * Opens the SoldierTransformation state.
 * @param action Pointer to an action.
 */
void SoldierTransformState::lstTransformationClick(Action *)
{
	std::string trType = _transformations[_indices[_lstTransformations->getSelectedRow()]].type;
	RuleSoldierTransformation* transformationRule = _game->getMod()->getSoldierTransformation(trType, false);

	if (transformationRule)
	{
		Soldier* soldier = _base->getSoldiers()->at(_soldier);

		_game->popState();
		_game->pushState(new SoldierTransformationState(transformationRule, _base, soldier, nullptr));
	}
}

/**
 * Shows the corresponding Ufopaedia article.
 * @param action Pointer to an action.
 */
void SoldierTransformState::lstTransformationClickMiddle(Action*)
{
	std::string trType = _transformations[_indices[_lstTransformations->getSelectedRow()]].type;
	RuleSoldierTransformation* transformationRule = _game->getMod()->getSoldierTransformation(trType, false);

	if (transformationRule)
	{
		Ufopaedia::openArticle(_game, transformationRule->getName());
	}
}

/**
 * Sorts the transformations by name.
 * @param action Pointer to an action.
 */
void SoldierTransformState::sortNameClick(Action*)
{
	_transformationOrder = _transformationOrder == TRANSFORM_SORT_NAME_ASC ? TRANSFORM_SORT_NAME_DESC : TRANSFORM_SORT_NAME_ASC;
	sortList();
}

}
