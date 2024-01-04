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
class TextList;
class ArrowButton;

/// Funding country sorting modes.
enum FundingCountrySort
{
	FC_NONE,
	FC_NAME_ASC,
	FC_NAME_DESC,
	FC_FUNDING_ASC,
	FC_FUNDING_DESC,
	FC_CHANGE_ASC,
	FC_CHANGE_DESC
};

struct FundingCountry
{
	FundingCountry(const std::string& _name, int _funding, int _change)
		: name(_name), funding(_funding), change(_change)
	{
	}
	std::string name;
	int funding;
	int change;
};

/**
 * Funding screen accessible from the Geoscape
 * that shows all the countries' funding.
 */
class FundingState : public State
{
private:
	TextButton *_btnOk;
	Window *_window;
	Text *_txtTitle, *_txtCountry, *_txtFunding, *_txtChange;
	TextList *_lstCountries;
	ArrowButton *_sortName, *_sortFunding, *_sortChange;

	std::vector<FundingCountry> _fundingCountryList;
	FundingCountrySort _fundingCountryOrder;
	void updateArrows();
public:
	/// Creates the Funding state.
	FundingState();
	/// Cleans up the Funding state.
	~FundingState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Sets up the funding countries list.
	void init() override;
	/// Sorts the funding countries list.
	void sortList();
	/// Updates the funding countries list.
	virtual void updateList();
	/// Handler for clicking the Name arrow.
	void sortNameClick(Action* action);
	/// Handler for clicking the Funding arrow.
	void sortFundingClick(Action* action);
	/// Handler for clicking the Change arrow.
	void sortChangeClick(Action* action);
};

}
