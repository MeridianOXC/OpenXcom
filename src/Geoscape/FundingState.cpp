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
#include "FundingState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Unicode.h"
#include "../Interface/ArrowButton.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Country.h"
#include "../Mod/RuleCountry.h"
#include "../Savegame/SavedGame.h"
#include "../Engine/Options.h"

namespace OpenXcom
{

struct compareFundingCountryName
{
	bool operator()(const FundingCountry &a, const FundingCountry &b) const
	{
		return Unicode::naturalCompare(a.name, b.name);
	}
};

struct compareFundingCountryFunding
{
	bool operator()(const FundingCountry &a, const FundingCountry &b) const
	{
		return a.funding < b.funding;
	}
};

struct compareFundingCountryChange
{
	bool operator()(const FundingCountry &a, const FundingCountry &b) const
	{
		return a.change < b.change;
	}
};

/**
 * Initializes all the elements in the Funding screen.
 * @param game Pointer to the core game.
 */
FundingState::FundingState()
{
	_screen = false;

	// Create objects
	_window = new Window(this, 320, 200, 0, 0, POPUP_BOTH);
	_btnOk = new TextButton(50, 12, 135, 180);
	_txtTitle = new Text(320, 17, 0, 8);
	_txtCountry = new Text(100, 9, 32, 30);
	_txtFunding = new Text(100, 9, 140, 30);
	_txtChange = new Text(72, 9, 240, 30);
	_lstCountries = new TextList(260, 136, 32, 40);
	_sortName = new ArrowButton(ARROW_NONE, 11, 8, 32, 30);
	_sortFunding = new ArrowButton(ARROW_NONE, 11, 8, 140, 30);
	_sortChange = new ArrowButton(ARROW_NONE, 11, 8, 240, 30);

	// Set palette
	setInterface("fundingWindow");

	add(_window, "window", "fundingWindow");
	add(_btnOk, "button", "fundingWindow");
	add(_txtTitle, "text1", "fundingWindow");
	add(_txtCountry, "text2", "fundingWindow");
	add(_txtFunding, "text2", "fundingWindow");
	add(_txtChange, "text2", "fundingWindow");
	add(_lstCountries, "list", "fundingWindow");
	add(_sortName, "text2", "fundingWindow");
	add(_sortFunding, "text2", "fundingWindow");
	add(_sortChange, "text2", "fundingWindow");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "fundingWindow");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&FundingState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&FundingState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&FundingState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&FundingState::btnOkClick, Options::keyGeoFunding);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_INTERNATIONAL_RELATIONS"));

	_txtCountry->setText(tr("STR_COUNTRY"));

	_txtFunding->setText(tr("STR_FUNDING"));

	_txtChange->setText(tr("STR_CHANGE"));

	_lstCountries->setColumns(3, 108, 100, 52);
	_lstCountries->setDot(true);

	_sortName->setX(_sortName->getX() + _txtCountry->getTextWidth() + 4);
	_sortName->onMouseClick((ActionHandler)&FundingState::sortNameClick);

	_sortFunding->setX(_sortFunding->getX() + _txtFunding->getTextWidth() + 4);
	_sortFunding->onMouseClick((ActionHandler)&FundingState::sortFundingClick);

	_sortChange->setX(_sortChange->getX() + _txtChange->getTextWidth() + 4);
	_sortChange->onMouseClick((ActionHandler)&FundingState::sortChangeClick);

	_fundingCountryOrder = FC_NONE;

	for (auto* country : *_game->getSavedGame()->getCountries())
	{
		_fundingCountryList.push_back(FundingCountry(
			tr(country->getRules()->getType()),
			country->getFunding().back(),
			country->getFunding().size() > 1 ? country->getFunding().back() - country->getFunding().at(country->getFunding().size() - 2) : 0)
		);
	}
}

/**
 *
 */
FundingState::~FundingState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void FundingState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Refreshes the funding countries list.
 */
void FundingState::init()
{
	State::init();

	sortList();
}

/**
 * Updates the sorting arrows based on the current setting.
 */
void FundingState::updateArrows()
{
	_sortName->setShape(ARROW_NONE);
	_sortFunding->setShape(ARROW_NONE);
	_sortChange->setShape(ARROW_NONE);
	switch (_fundingCountryOrder)
	{
	case FC_NONE:
		break;
	case FC_NAME_ASC:
		_sortName->setShape(ARROW_SMALL_UP);
		break;
	case FC_NAME_DESC:
		_sortName->setShape(ARROW_SMALL_DOWN);
		break;
	case FC_FUNDING_ASC:
		_sortFunding->setShape(ARROW_SMALL_UP);
		break;
	case FC_FUNDING_DESC:
		_sortFunding->setShape(ARROW_SMALL_DOWN);
		break;
	case FC_CHANGE_ASC:
		_sortChange->setShape(ARROW_SMALL_UP);
		break;
	case FC_CHANGE_DESC:
		_sortChange->setShape(ARROW_SMALL_DOWN);
		break;
	default:
		break;
	}
}

/**
 * Sorts the funding countries list.
 */
void FundingState::sortList()
{
	updateArrows();

	switch (_fundingCountryOrder)
	{
	case FC_NONE:
		break;
	case FC_NAME_ASC:
		std::stable_sort(_fundingCountryList.begin(), _fundingCountryList.end(), compareFundingCountryName());
		break;
	case FC_NAME_DESC:
		std::stable_sort(_fundingCountryList.rbegin(), _fundingCountryList.rend(), compareFundingCountryName());
		break;
	case FC_FUNDING_ASC:
		std::stable_sort(_fundingCountryList.begin(), _fundingCountryList.end(), compareFundingCountryFunding());
		break;
	case FC_FUNDING_DESC:
		std::stable_sort(_fundingCountryList.rbegin(), _fundingCountryList.rend(), compareFundingCountryFunding());
		break;
	case FC_CHANGE_ASC:
		std::stable_sort(_fundingCountryList.begin(), _fundingCountryList.end(), compareFundingCountryChange());
		break;
	case FC_CHANGE_DESC:
		std::stable_sort(_fundingCountryList.rbegin(), _fundingCountryList.rend(), compareFundingCountryChange());
		break;
	}

	updateList();
}

/**
 * Updates the funding countries list.
 */
void FundingState::updateList()
{
	_lstCountries->clearList();
	for (const auto& country : _fundingCountryList)
	{
		std::ostringstream ss;
		ss << Unicode::TOK_COLOR_FLIP << Unicode::formatFunding(country.funding) << Unicode::TOK_COLOR_FLIP;

		std::ostringstream ss2;
		if (country.change != 0) ss2 << Unicode::TOK_COLOR_FLIP;
		if (country.change > 0)  ss2 << '+';
		ss2 << Unicode::formatFunding(country.change);
		if (country.change != 0) ss2 << Unicode::TOK_COLOR_FLIP;

		_lstCountries->addRow(3, country.name.c_str(), ss.str().c_str(), ss2.str().c_str());
	}
	_lstCountries->addRow(2, tr("STR_TOTAL_UC").c_str(), Unicode::formatFunding(_game->getSavedGame()->getCountryFunding()).c_str());
	_lstCountries->setRowColor(_game->getSavedGame()->getCountries()->size(), _txtCountry->getColor());
}

/**
 * Sorts the funding countries by name.
 */
void FundingState::sortNameClick(Action*)
{
	_fundingCountryOrder = _fundingCountryOrder == FC_NAME_ASC ? FC_NAME_DESC : FC_NAME_ASC;
	sortList();
}

/**
 * Sorts the funding countries by funding.
 */
void FundingState::sortFundingClick(Action*)
{
	_fundingCountryOrder = _fundingCountryOrder == FC_FUNDING_ASC ? FC_FUNDING_DESC : FC_FUNDING_ASC;
	sortList();
}

/**
 * Sorts the funding countries by funding change.
 */
void FundingState::sortChangeClick(Action*)
{
	_fundingCountryOrder = _fundingCountryOrder == FC_CHANGE_ASC ? FC_CHANGE_DESC : FC_CHANGE_ASC;
	sortList();
}

}
