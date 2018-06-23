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

#include <locale>
#include "Ufopaedia.h"
#include "UfopaediaSelectState.h"
#include "../Mod/ArticleDefinition.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/TextList.h"
#include "../Mod/Mod.h"
#include "../Savegame/SavedGame.h"

namespace OpenXcom
{
	UfopaediaSelectState::UfopaediaSelectState(const std::string &section) : _section(section)
	{
		_screen = false;

		// set background window
		_window = new Window(this, 256, 180, 32, 10, POPUP_NONE);
		_btnQuickSearch = new TextEdit(this, 48, 9, 48, 30);

		// set title
		_txtTitle = new Text(224, 17, 48, 26);

		// set buttons
		_btnOk = new TextButton(108, 16, 164, 166);
		_btnShowOnlyNew = new ToggleTextButton(108, 16, 48, 166);
		_lstSelection = new TextList(224, 104, 40, 50);
		_lstScroll = 0;

		// Set palette
		setInterface("ufopaedia");

		add(_window, "window", "ufopaedia");
		add(_btnQuickSearch, "button2", "ufopaedia");
		add(_txtTitle, "text", "ufopaedia");
		add(_btnOk, "button2", "ufopaedia");
		add(_btnShowOnlyNew, "button2", "ufopaedia");
		add(_lstSelection, "list", "ufopaedia");

		centerAllSurfaces();

		_window->setBackground(_game->getMod()->getSurface("BACK01.SCR"));

		_txtTitle->setBig();
		_txtTitle->setAlign(ALIGN_CENTER);
		_txtTitle->setText(tr("STR_SELECT_ITEM"));

		_btnOk->setText(tr("STR_OK"));
		_btnOk->onMouseClick((ActionHandler)&UfopaediaSelectState::btnOkClick);
		_btnOk->onKeyboardPress((ActionHandler)&UfopaediaSelectState::btnOkClick,Options::keyCancel);
		_btnOk->onKeyboardPress((ActionHandler)&UfopaediaSelectState::btnMarkAllAsSeenClick, Options::keyMarkAllAsSeen);

		_btnShowOnlyNew->setText(tr("STR_SHOW_ONLY_NEW"));
		_btnShowOnlyNew->onMouseClick((ActionHandler)&UfopaediaSelectState::btnShowOnlyNewClick);

		_lstSelection->setColumns(1, 206);
		_lstSelection->setSelectable(true);
		_lstSelection->setBackground(_window);
		_lstSelection->setMargin(18);
		_lstSelection->setAlign(ALIGN_CENTER);
		_lstSelection->onMouseClick((ActionHandler)&UfopaediaSelectState::lstSelectionClick, SDL_BUTTON_LEFT);
		_lstSelection->onMouseClick((ActionHandler)&UfopaediaSelectState::lstSelectionClickRight, SDL_BUTTON_RIGHT);

		_btnQuickSearch->setText(L""); // redraw
		_btnQuickSearch->onEnter((ActionHandler)&UfopaediaSelectState::btnQuickSearchApply);
		_btnQuickSearch->setVisible(false);

		_btnOk->onKeyboardRelease((ActionHandler)&UfopaediaSelectState::btnQuickSearchToggle, Options::keyToggleQuickSearch);
	}

	UfopaediaSelectState::~UfopaediaSelectState()
	{}
	
	/**
	 * Initializes the state.
	 */
	void UfopaediaSelectState::init()
	{
		State::init();
		loadSelectionList(false);
	}

	/**
	 * Returns to the previous screen.
	 * @param action Pointer to an action.
	 */
	void UfopaediaSelectState::btnOkClick(Action *)
	{
		_game->popState();
	}

	/**
	 *
	 * @param action Pointer to an action.
	 */
	void UfopaediaSelectState::lstSelectionClick(Action *)
	{
		_lstScroll = _lstSelection->getScroll();
		printf("lstscroll is %zd\n", _lstScroll);
		Ufopaedia::openArticle(_game, _filtered_article_list[_lstSelection->getSelectedRow()]);
	}

	/**
	 * Toggles the topic status.
	 * @param action Pointer to an action.
	 */
	void UfopaediaSelectState::lstSelectionClickRight(Action *)
	{
		// change status
		const std::string rule = _filtered_article_list[_lstSelection->getSelectedRow()]->id;
		int oldState = _game->getSavedGame()->getUfopediaRuleStatus(rule);
		int newState = 1 - oldState;
		_game->getSavedGame()->setUfopediaRuleStatus(rule, newState);

		if (newState == ArticleDefinition::PEDIA_STATUS_NEW)
		{
			_lstSelection->setRowColor(_lstSelection->getSelectedRow(), _lstSelection->getSecondaryColor());
		}
		else
		{
			_lstSelection->setRowColor(_lstSelection->getSelectedRow(), _lstSelection->getColor());
		}
	}

	/**
	* Quick search toggle.
	* @param action Pointer to an action.
	*/
	void UfopaediaSelectState::btnQuickSearchToggle(Action *action)
	{
		if (_btnQuickSearch->getVisible())
		{
			_btnQuickSearch->setText(L"");
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
	void UfopaediaSelectState::btnQuickSearchApply(Action *)
	{
		loadSelectionList(false);
	}

	/**
	* Filter to display only new items.
	* @param action Pointer to an action.
	*/
	void UfopaediaSelectState::btnShowOnlyNewClick(Action *)
	{
		loadSelectionList(false);
	}

	/**
	* Marks all items as seen
	* @param action Pointer to an action.
	*/
	void UfopaediaSelectState::btnMarkAllAsSeenClick(Action *)
	{
		loadSelectionList(true);
	}

	void UfopaediaSelectState::loadSelectionList(bool markAllAsSeen)
	{
		std::locale myLocale = CrossPlatform::testLocale();
		std::wstring searchString = _btnQuickSearch->getText();
		CrossPlatform::upperCase(searchString, myLocale);

		ArticleDefinitionList::iterator it;

		_lstSelection->clearList();
		_article_list.clear();
		Ufopaedia::list(_game->getSavedGame(), _game->getMod(), _section, _article_list);
		_filtered_article_list.clear();

		int row = 0;
		bool hasUnseen = false;
		for (it = _article_list.begin(); it!=_article_list.end(); ++it)
		{
			// filter
			if (_btnShowOnlyNew->getPressed())
			{
				if (_game->getSavedGame()->getUfopediaRuleStatus((*it)->id) != ArticleDefinition::PEDIA_STATUS_NEW)
				{
					continue;
				}
			}

			// quick search
			if (searchString != L"")
			{
				std::wstring projectName = tr((*it)->title);
				CrossPlatform::upperCase(projectName, myLocale);
				if (projectName.find(searchString) == std::string::npos)
				{
					continue;
				}
			}

			_filtered_article_list.push_back((*it));
			_lstSelection->addRow(1, tr((*it)->title).c_str());

			if (markAllAsSeen)
			{
				// remember all listed articles as seen/normal
				_game->getSavedGame()->setUfopediaRuleStatus((*it)->id, ArticleDefinition::PEDIA_STATUS_NORMAL);
			}
			else if (_game->getSavedGame()->getUfopediaRuleStatus((*it)->id) == ArticleDefinition::PEDIA_STATUS_NEW)
			{
				// highlight as new
				_lstSelection->setCellColor(row, 0, _lstSelection->getSecondaryColor());
				hasUnseen = true;
			}
			row++;
		}

		std::wstring label = tr("STR_SHOW_ONLY_NEW");
		_btnShowOnlyNew->setText((hasUnseen ? L"* " : L"") + label);
		printf("setscroll to %zd\n", _lstScroll);
		_lstSelection->scrollTo(_lstScroll);
	}

}
