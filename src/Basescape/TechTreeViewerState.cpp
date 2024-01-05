/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#include "TechTreeViewerState.h"
#include "TechTreeSelectState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleArcScript.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleEventScript.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleManufacture.h"
#include "../Mod/RuleMissionScript.h"
#include "../Mod/RuleResearch.h"
#include "../Mod/RuleSoldierTransformation.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Unicode.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Savegame/SavedGame.h"
#include <algorithm>
#include <unordered_set>

namespace OpenXcom
{

/**
 * Initializes all the elements on the UI.
 */
TechTreeViewerState::TechTreeViewerState(const RuleResearch *r, const RuleManufacture *m, const RuleBaseFacility *f, const RuleCraft *c)
{
	if (r != 0)
	{
		_selectedTopic = r->getName();
		_selectedFlag = TTV_RESEARCH;
	}
	else if (m != 0)
	{
		_selectedTopic = m->getName();
		_selectedFlag = TTV_MANUFACTURING;
	}
	else if (f != 0)
	{
		_selectedTopic = f->getType();
		_selectedFlag = TTV_FACILITIES;
	}
	else if (c != 0)
	{
		_selectedTopic = c->getType();
		_selectedFlag = TTV_CRAFTS;
	}

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(304, 17, 8, 7);
	_txtSelectedTopic = new Text(204, 9, 8, 24);
	_txtProgress = new Text(100, 9, 212, 24);
	_txtCostIndicator = new Text(100, 9, 16, 32); // experimental cost indicator
	_lstLeft = new TextList(132, 128, 8, 40);
	_lstRight = new TextList(132, 128, 164, 40);
	_lstFull = new TextList(288, 128, 8, 40);
	_btnNew = new TextButton(148, 16, 8, 176);
	_btnOk = new TextButton(148, 16, 164, 176);

	// Set palette
	setInterface("techTreeViewer");

	_purple = _game->getMod()->getInterface("techTreeViewer")->getElement("list")->color;
	_pink = _game->getMod()->getInterface("techTreeViewer")->getElement("list")->color2;
	_blue = _game->getMod()->getInterface("techTreeViewer")->getElement("list")->border;
	_white = _game->getMod()->getInterface("techTreeViewer")->getElement("listExtended")->color;
	_gold = _game->getMod()->getInterface("techTreeViewer")->getElement("listExtended")->color2;
	_grey = _game->getMod()->getInterface("techTreeViewer")->getElement("listExtended")->border;

	add(_window, "window", "techTreeViewer");
	add(_txtTitle, "text", "techTreeViewer");
	add(_txtSelectedTopic, "text", "techTreeViewer");
	add(_txtProgress, "text", "techTreeViewer");
	add(_txtCostIndicator, "text", "techTreeViewer");
	add(_lstLeft, "list", "techTreeViewer");
	add(_lstRight, "list", "techTreeViewer");
	add(_lstFull, "list", "techTreeViewer");
	add(_btnNew, "button", "techTreeViewer");
	add(_btnOk, "button", "techTreeViewer");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "techTreeViewer");

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TECH_TREE_VIEWER"));

	_txtSelectedTopic->setText(tr("STR_TOPIC").arg(""));

	_lstLeft->setColumns(1, 132);
	_lstLeft->setSelectable(true);
	_lstLeft->setBackground(_window);
	_lstLeft->setWordWrap(true);
	_lstLeft->onMouseClick((ActionHandler)&TechTreeViewerState::onSelectLeftTopic);

	_lstRight->setColumns(1, 132);
	_lstRight->setSelectable(true);
	_lstRight->setBackground(_window);
	_lstRight->setWordWrap(true);
	_lstRight->onMouseClick((ActionHandler)&TechTreeViewerState::onSelectRightTopic);

	_lstFull->setColumns(1, 288);
	_lstFull->setSelectable(true);
	_lstFull->setBackground(_window);
	_lstFull->setWordWrap(true);
	_lstFull->onMouseClick((ActionHandler)&TechTreeViewerState::onSelectFullTopic);

	_btnNew->setText(tr("STR_SELECT_TOPIC"));
	_btnNew->onMouseClick((ActionHandler)&TechTreeViewerState::btnNewClick);
	_btnNew->onKeyboardPress((ActionHandler)&TechTreeViewerState::btnNewClick, Options::keyToggleQuickSearch);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&TechTreeViewerState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&TechTreeViewerState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&TechTreeViewerState::btnBackClick, SDLK_BACKSPACE);

	if (Options::oxceDisableTechTreeViewer)
	{
		_txtTitle->setHeight(_txtTitle->getHeight() * 9);
		_txtTitle->setWordWrap(true);
		_txtTitle->setText(tr("STR_THIS_FEATURE_IS_DISABLED_1"));
		_txtSelectedTopic->setVisible(false);
		_txtProgress->setVisible(false);
		_txtCostIndicator->setVisible(false);
		_lstLeft->setVisible(false);
		_lstRight->setVisible(false);
		_lstFull->setVisible(false);
		_btnNew->setVisible(false);
		return;
	}

	int discoveredSum = 0;
	// pre-calculate globally
	for (auto* discoveredResearchRule : _game->getSavedGame()->getDiscoveredResearch())
	{
		_alreadyAvailableResearch.insert(discoveredResearchRule->getName());
		discoveredSum += discoveredResearchRule->getCost();
	}
	for (auto& info : _game->getSavedGame()->getResearchRuleStatusRaw())
	{
		if (info.second == RuleResearch::RESEARCH_STATUS_DISABLED)
		{
			auto* rr = _game->getMod()->getResearch(info.first, false);
			if (rr)
			{
				_disabledResearch.insert(rr->getName());
			}
		}
	}

	int totalSum = 0;
	RuleResearch *resRule = 0;
	for (auto& resType : _game->getMod()->getResearchList())
	{
		resRule = _game->getMod()->getResearch(resType);
		if (resRule != 0)
		{
			totalSum += resRule->getCost();
		}
	}

	RuleManufacture *manRule = 0;
	for (auto& manuf : _game->getMod()->getManufactureList())
	{
		manRule = _game->getMod()->getManufacture(manuf);
		if (_game->getSavedGame()->isResearched(manRule->getRequirements()))
		{
			_alreadyAvailableManufacture.insert(manRule->getName());
		}
	}

	RuleBaseFacility *facRule = 0;
	for (auto& facType : _game->getMod()->getBaseFacilitiesList())
	{
		facRule = _game->getMod()->getBaseFacility(facType);
		if (_game->getSavedGame()->isResearched(facRule->getRequirements()))
		{
			_alreadyAvailableFacilities.insert(facRule->getType());
		}
	}

	RuleItem *itemRule = 0;
	for (auto& itemType : _game->getMod()->getItemsList())
	{
		itemRule = _game->getMod()->getItem(itemType);
		if (!itemRule->getRequirements().empty() || !itemRule->getBuyRequirements().empty())
		{
			_protectedItems.insert(itemRule->getType());
			if (_game->getSavedGame()->isResearched(itemRule->getRequirements()) && _game->getSavedGame()->isResearched(itemRule->getBuyRequirements()))
			{
				_alreadyAvailableItems.insert(itemRule->getType());
			}
		}
	}

	RuleCraft *craftRule = 0;
	for (auto& craftType : _game->getMod()->getCraftsList())
	{
		craftRule = _game->getMod()->getCraft(craftType);
		if (_game->getSavedGame()->isResearched(craftRule->getRequirements()))
		{
			_alreadyAvailableCrafts.insert(craftRule->getType());
		}
	}

	_txtProgress->setAlign(ALIGN_RIGHT);
	_txtProgress->setText(tr("STR_RESEARCH_PROGRESS").arg(discoveredSum * 100 / totalSum));
}

/**
 *
 */
TechTreeViewerState::~TechTreeViewerState()
{
}

/**
* Initializes the screen (fills the lists).
*/
void TechTreeViewerState::init()
{
	State::init();

	if (!Options::oxceDisableTechTreeViewer)
	{
		initLists();
	}
}

/**
* Returns to the previous screen.
* @param action Pointer to an action.
*/
void TechTreeViewerState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Navigates to the previous topic from the browsing history.
 * @param action Pointer to an action.
 */
void TechTreeViewerState::btnBackClick(Action *)
{
	if (!_history.empty())
	{
		_selectedFlag = _history.back().second;
		_selectedTopic = _history.back().first;
		_history.pop_back();
		initLists();
	}
}

/**
* Opens the Select Topic screen.
* @param action Pointer to an action.
*/
void TechTreeViewerState::btnNewClick(Action *)
{
	_game->pushState(new TechTreeSelectState(this));
}

/**
 * Shows the filtered topics.
 */
void TechTreeViewerState::initLists()
{
	// Set topic name
	{
		std::ostringstream ss;
		ss << tr(_selectedTopic);
		if (_selectedFlag == TTV_MANUFACTURING)
		{
			ss << tr("STR_M_FLAG");
		}
		else if (_selectedFlag == TTV_FACILITIES)
		{
			ss << tr("STR_F_FLAG");
		}
		else if (_selectedFlag == TTV_ITEMS)
		{
			ss << tr("STR_I_FLAG");
		}
		else if (_selectedFlag == TTV_CRAFTS)
		{
			ss << tr("STR_C_FLAG");
		}
		_txtSelectedTopic->setText(tr("STR_TOPIC").arg(ss.str()));
		_txtCostIndicator->setText("");
	}

	// reset
	_leftTopics.clear();
	_rightTopics.clear();
	_leftFlags.clear();
	_rightFlags.clear();
	_lstLeft->clearList();
	_lstRight->clearList();
	_lstFull->clearList();
	_lstLeft->setVisible(true);
	_lstRight->setVisible(true);
	_lstFull->setVisible(false);

	if (_selectedFlag == TTV_NONE)
	{
		return;
	}
	else if (_selectedFlag == TTV_RESEARCH)
	{
		int row = 0;
		RuleResearch *rule = _game->getMod()->getResearch(_selectedTopic);
		if (rule == 0)
			return;

		// Cost indicator
		{
			std::ostringstream ss;
			int cost = rule->getCost();
			std::vector<std::pair<int, std::string>> symbol_values
					({{100, "#"}, {20, "="}, {5, "-"}});

			for (const auto& sym : symbol_values)
			{
				while (cost >= std::get<0>(sym))
				{
					cost -= std::get<0>(sym);
					ss << std::get<1>(sym);
				}
			}
			if (rule->getCost() < 10)
			{
				while (cost >= 1)
				{
					cost -= 1;
					ss << ".";
				}
			}
			_txtCostIndicator->setText(ss.str());
		}
		//

		const std::vector<std::string> &researchList = _game->getMod()->getResearchList();
		const std::vector<std::string> &manufactureList = _game->getMod()->getManufactureList();

		// 0. common pre-calculation
		const std::vector<const RuleResearch*>& reqs = rule->getRequirements();
		const std::vector<const RuleResearch*>& deps = rule->getDependencies();
		std::vector<std::string> unlockedBy;
		std::vector<std::string> disabledBy;
		std::vector<std::string> reenabledBy;
		std::vector<std::string> getForFreeFrom;
		std::vector<std::string> lookupOf;
		std::vector<std::string> requiredByResearch;
		std::vector<std::string> requiredByManufacture;
		std::vector<std::string> requiredByFacilities;
		std::vector<std::string> requiredByItems;
		std::vector<std::string> requiredByTransformations;
		std::vector<std::string> requiredByCrafts;
		std::vector<std::string> leadsTo;
		const std::vector<const RuleResearch*>& unlocks = rule->getUnlocked();
		const std::vector<const RuleResearch*>& disables = rule->getDisabled();
		const std::vector<const RuleResearch*>& reenables = rule->getReenabled();
		const std::vector<const RuleResearch*>& free = rule->getGetOneFree();
		auto& freeProtected = rule->getGetOneFreeProtected();

		for (auto& j : manufactureList)
		{
			RuleManufacture *temp = _game->getMod()->getManufacture(j);
			for (auto& i : temp->getRequirements())
			{
				if (i == rule)
				{
					requiredByManufacture.push_back(j);
				}
			}
		}

		for (auto& f : _game->getMod()->getBaseFacilitiesList())
		{
			RuleBaseFacility *temp = _game->getMod()->getBaseFacility(f);
			for (auto& i : temp->getRequirements())
			{
				if (i == rule->getName())
				{
					requiredByFacilities.push_back(f);
				}
			}
		}

		for (auto& item : _game->getMod()->getItemsList())
		{
			RuleItem *temp = _game->getMod()->getItem(item);
			for (auto& i : temp->getRequirements())
			{
				if (i == rule)
				{
					requiredByItems.push_back(item);
				}
			}
			for (auto& i : temp->getBuyRequirements())
			{
				if (i == rule)
				{
					requiredByItems.push_back(item);
				}
			}
		}

		for (auto& transf : _game->getMod()->getSoldierTransformationList())
		{
			RuleSoldierTransformation* temp = _game->getMod()->getSoldierTransformation(transf);
			for (auto& i : temp->getRequiredResearch())
			{
				if (i == rule->getName())
				{
					requiredByTransformations.push_back(transf);
				}
			}
		}

		for (auto& c : _game->getMod()->getCraftsList())
		{
			RuleCraft *temp = _game->getMod()->getCraft(c);
			for (auto& i : temp->getRequirements())
			{
				if (i == rule->getName())
				{
					requiredByCrafts.push_back(c);
				}
			}
		}

		for (auto& j : researchList)
		{
			RuleResearch *temp = _game->getMod()->getResearch(j);
			for (auto& i : temp->getUnlocked())
			{
				if (i == rule)
				{
					unlockedBy.push_back(j);
				}
			}
			for (auto& i : temp->getDisabled())
			{
				if (i == rule)
				{
					disabledBy.push_back(j);
				}
			}
			for (auto& i : temp->getReenabled())
			{
				if (i == rule)
				{
					reenabledBy.push_back(j);
				}
			}
			for (auto& i : temp->getGetOneFree())
			{
				if (i == rule)
				{
					getForFreeFrom.push_back(j);
				}
			}
			for (auto& itMap : temp->getGetOneFreeProtected())
			{
				for (auto& i : itMap.second)
				{
					if (i == rule)
					{
						getForFreeFrom.push_back(j);
					}
				}
			}
			if (!Mod::isEmptyRuleName(temp->getLookup()))
			{
				if (temp->getLookup() == rule->getName())
				{
					lookupOf.push_back(j);
				}
			}
			for (auto& i : temp->getRequirements())
			{
				if (i == rule)
				{
					requiredByResearch.push_back(j);
				}
			}
			for (auto& i : temp->getDependencies())
			{
				if (i == rule)
				{
					leadsTo.push_back(j);
				}
			}
		}

		// 1. item required
		if (rule->needItem())
		{
			if (rule->destroyItem())
			{
				_lstLeft->addRow(1, tr("STR_ITEM_DESTROYED").c_str());
			}
			else
			{
				_lstLeft->addRow(1, tr("STR_ITEM_REQUIRED").c_str());
			}
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			if (rule->getNeededItem())
			{
				std::string itemName = tr(rule->getNeededItem()->getType());
				itemName.insert(0, "  ");
				_lstLeft->addRow(1, itemName.c_str());
				_lstLeft->setRowColor(row, getResearchColor(rule->getNeededItem()->getType()));
				_leftTopics.push_back(rule->getNeededItem()->getType());
				_leftFlags.push_back(TTV_ITEMS);
			}
			else
			{
				std::string itemName = "  -";
				_lstLeft->addRow(1, itemName.c_str());
				_lstLeft->setRowColor(row, _white);
				_leftTopics.push_back("-");
				_leftFlags.push_back(TTV_NONE);
			}
			++row;
		}

		// 1b. requires services (from base facilities)
		if (rule->getRequireBaseFunc().any())
		{
			_lstLeft->addRow(1, tr("STR_SERVICES_REQUIRED").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& reqServiceType : _game->getMod()->getBaseFunctionNames(rule->getRequireBaseFunc()))
			{
				std::string name = tr(reqServiceType);
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, _gold);
				_leftTopics.push_back("-");
				_leftFlags.push_back(TTV_NONE);
				++row;
			}
		}

		// 2. requires
		if (reqs.size() > 0)
		{
			_lstLeft->addRow(1, tr("STR_REQUIRES").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (auto* i : reqs)
			{
				std::string name = tr(i->getName());
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, getResearchColor(i->getName()));
				_leftTopics.push_back(i->getName());
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 3. depends on
		if (deps.size() > 0)
		{
			_lstLeft->addRow(1, tr("STR_DEPENDS_ON").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (auto* i : deps)
			{
				if (std::find(unlockedBy.begin(), unlockedBy.end(), i->getName()) != unlockedBy.end())
				{
					// if the same item is also in the "Unlocked by" section, skip it
					continue;
				}
				std::string name = tr(i->getName());
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, getResearchColor(i->getName()));
				_leftTopics.push_back(i->getName());
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 4a. unlocked by
		if (unlockedBy.size() > 0)
		{
			_lstLeft->addRow(1, tr("STR_UNLOCKED_BY").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& res : unlockedBy)
			{
				std::string name = tr(res);
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, getResearchColor(res));
				_leftTopics.push_back(res);
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 4b. disabled by
		if (disabledBy.size() > 0)
		{
			_lstLeft->addRow(1, tr("STR_DISABLED_BY").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& res : disabledBy)
			{
				std::string name = tr(res);
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, getResearchColor(res));
				_leftTopics.push_back(res);
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 4c. reenabled by
		if (reenabledBy.size() > 0)
		{
			_lstLeft->addRow(1, tr("STR_REENABLED_BY").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& res : reenabledBy)
			{
				std::string name = tr(res);
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, getResearchColor(res));
				_leftTopics.push_back(res);
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 5. get for free from
		if (getForFreeFrom.size() > 0)
		{
			_lstLeft->addRow(1, tr("STR_GET_FOR_FREE_FROM").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& res : getForFreeFrom)
			{
				std::string name = tr(res);
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, getResearchColor(res));
				_leftTopics.push_back(res);
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// is lookup of
		if (lookupOf.size() > 0)
		{
			_lstLeft->addRow(1, tr("STR_IS_LOOKUP_OF").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& res : lookupOf)
			{
				std::string name = tr(res);
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, getResearchColor(res));
				_leftTopics.push_back(res);
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		row = 0;

		// lookup link
		if (!Mod::isEmptyRuleName(rule->getLookup()))
		{
			_lstRight->addRow(1, tr("STR_LOOKUP").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;

			std::string name = tr(rule->getLookup());
			name.insert(0, "  ");
			_lstRight->addRow(1, name.c_str());
			_lstRight->setRowColor(row, getResearchColor(rule->getLookup()));
			_rightTopics.push_back(rule->getLookup());
			_rightFlags.push_back(TTV_RESEARCH);
			++row;
		}

		// spawned item
		if (!Mod::isEmptyRuleName(rule->getSpawnedItem()) || !rule->getSpawnedItemList().empty())
		{
			_lstRight->addRow(1, tr("STR_SPAWNED_ITEMS").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
		}
		if (!Mod::isEmptyRuleName(rule->getSpawnedItem()))
		{
			std::string name = tr(rule->getSpawnedItem());
			name.insert(0, "  ");
			if (rule->getSpawnedItemCount() > 1)
			{
				name.append(" x");
				name.append(std::to_string(rule->getSpawnedItemCount()));
			}
			_lstRight->addRow(1, name.c_str());
			_lstRight->setRowColor(row, _white);
			_rightTopics.push_back(rule->getSpawnedItem());
			_rightFlags.push_back(TTV_ITEMS);
			++row;
		}
		for (auto& sil : rule->getSpawnedItemList())
		{
			std::string name = tr(sil);
			name.insert(0, "  ");
			_lstRight->addRow(1, name.c_str());
			_lstRight->setRowColor(row, _white);
			_rightTopics.push_back(sil);
			_rightFlags.push_back(TTV_ITEMS);
			++row;
		}

		// spawned event
		if (!Mod::isEmptyRuleName(rule->getSpawnedEvent()))
		{
			_lstRight->addRow(1, tr("STR_SPAWNED_EVENT").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;

			std::string name = tr(rule->getSpawnedEvent());
			name.insert(0, "  ");
			_lstRight->addRow(1, name.c_str());
			_lstRight->setRowColor(row, _white);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
		}

		// 6. required by
		if (requiredByResearch.size() > 0 || requiredByManufacture.size() > 0 || requiredByFacilities.size() > 0 || requiredByItems.size() > 0 || requiredByCrafts.size() > 0)
		{
			_lstRight->addRow(1, tr("STR_REQUIRED_BY").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
		}

		// 6a. required by research
		if (requiredByResearch.size() > 0)
		{
			for (const auto& res : requiredByResearch)
			{
				std::string name = tr(res);
				name.insert(0, "  ");
				_lstRight->addRow(1, name.c_str());
				_lstRight->setRowColor(row, getResearchColor(res));
				_rightTopics.push_back(res);
				_rightFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 6b. required by manufacture
		if (requiredByManufacture.size() > 0)
		{
			for (const auto& manuf : requiredByManufacture)
			{
				std::string name = tr(manuf);
				name.insert(0, "  ");
				name.append(tr("STR_M_FLAG"));
				_lstRight->addRow(1, name.c_str());
				if (!isDiscoveredManufacture(manuf))
				{
					_lstRight->setRowColor(row, _pink);
				}
				_rightTopics.push_back(manuf);
				_rightFlags.push_back(TTV_MANUFACTURING);
				++row;
			}
		}

		// 6c. required by facilities
		if (requiredByFacilities.size() > 0)
		{
			for (const auto& facType : requiredByFacilities)
			{
				std::string name = tr(facType);
				name.insert(0, "  ");
				name.append(tr("STR_F_FLAG"));
				_lstRight->addRow(1, name.c_str());
				if (!isDiscoveredFacility(facType))
				{
					_lstRight->setRowColor(row, _pink);
				}
				_rightTopics.push_back(facType);
				_rightFlags.push_back(TTV_FACILITIES);
				++row;
			}
		}

		// 6d. required by items
		if (requiredByItems.size() > 0)
		{
			for (const auto& itemType : requiredByItems)
			{
				std::string name = tr(itemType);
				name.insert(0, "  ");
				name.append(tr("STR_I_FLAG"));
				_lstRight->addRow(1, name.c_str());
				if (!isProtectedAndDiscoveredItem(itemType))
				{
					_lstRight->setRowColor(row, _pink);
				}
				_rightTopics.push_back(itemType);
				_rightFlags.push_back(TTV_ITEMS);
				++row;
			}
		}

		// 6e. required by crafts
		if (requiredByCrafts.size() > 0)
		{
			for (const auto& craftType : requiredByCrafts)
			{
				std::string name = tr(craftType);
				name.insert(0, "  ");
				name.append(tr("STR_C_FLAG"));
				_lstRight->addRow(1, name.c_str());
				if (!isDiscoveredCraft(craftType))
				{
					_lstRight->setRowColor(row, _pink);
				}
				_rightTopics.push_back(craftType);
				_rightFlags.push_back(TTV_CRAFTS);
				++row;
			}
		}

		// 6f. required by transformations
		if (requiredByTransformations.size() > 0)
		{
			_lstRight->addRow(1, tr("STR_REQUIRED_BY_TRANSFORMATIONS").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;

			for (const auto& transformationType : requiredByTransformations)
			{
				std::string name = tr(transformationType);
				name.insert(0, "  ");
				_lstRight->addRow(1, name.c_str());
				_lstRight->setRowColor(row, _white);
				_rightTopics.push_back("-");
				_rightFlags.push_back(TTV_NONE);
				++row;
			}
		}

		// 7. leads to
		if (leadsTo.size() > 0)
		{
			_lstRight->addRow(1, tr("STR_LEADS_TO").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
			for (const auto& res : leadsTo)
			{
				const RuleResearch *iTemp = _game->getMod()->getResearch(res);
				if (std::find(unlocks.begin(), unlocks.end(), iTemp) != unlocks.end())
				{
					// if the same topic is also in the "Unlocks" section, skip it
					continue;
				}
				std::string name = tr(res);
				name.insert(0, "  ");
				_lstRight->addRow(1, name.c_str());
				_lstRight->setRowColor(row, getResearchColor(res));
				_rightTopics.push_back(res);
				_rightFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 8a. unlocks
		if (unlocks.size() > 0)
		{
			_lstRight->addRow(1, tr("STR_UNLOCKS").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
			for (auto* i : unlocks)
			{
				std::string name = tr(i->getName());
				name.insert(0, "  ");
				_lstRight->addRow(1, name.c_str());
				_lstRight->setRowColor(row, getResearchColor(i->getName()));
				_rightTopics.push_back(i->getName());
				_rightFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 8b. disables
		if (disables.size() > 0)
		{
			_lstRight->addRow(1, tr("STR_DISABLES").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
			for (auto* i : disables)
			{
				std::string name = tr(i->getName());
				name.insert(0, "  ");
				_lstRight->addRow(1, name.c_str());
				_lstRight->setRowColor(row, getResearchColor(i->getName()));
				_rightTopics.push_back(i->getName());
				_rightFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 8c. reenables
		if (reenables.size() > 0)
		{
			_lstRight->addRow(1, tr("STR_REENABLES").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
			for (auto* i : reenables)
			{
				std::string name = tr(i->getName());
				name.insert(0, "  ");
				_lstRight->addRow(1, name.c_str());
				_lstRight->setRowColor(row, getResearchColor(i->getName()));
				_rightTopics.push_back(i->getName());
				_rightFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 9. gives one for free
		if (free.size() > 0 || freeProtected.size() > 0)
		{
			int remaining = 0;
			int total = 0;
			for (auto* i : free)
			{
				if (getResearchColor(i->getName()) == _pink)
				{
					++remaining;
				}
				++total;
			}
			for (auto& itMap : freeProtected)
			{
				for (auto* i : itMap.second)
				{
					if (getResearchColor(i->getName()) == _pink)
					{
						++remaining;
					}
					++total;
				}
			}
			std::ostringstream ssFree;
			if (rule->sequentialGetOneFree())
			{
				ssFree << tr("STR_GIVES_ONE_FOR_FREE_SEQ");
			}
			else
			{
				ssFree << tr("STR_GIVES_ONE_FOR_FREE");
			}
			ssFree << " " << remaining << "/" << total;
			_lstRight->addRow(1, ssFree.str().c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
			for (auto* i : free)
			{
				std::string name = tr(i->getName());
				name.insert(0, "  ");
				_lstRight->addRow(1, name.c_str());
				_lstRight->setRowColor(row, getResearchColor(i->getName()));
				_rightTopics.push_back(i->getName());
				_rightFlags.push_back(TTV_RESEARCH);
				++row;
			}
			for (auto& itMap : freeProtected)
			{
				std::string name2 = tr(itMap.first->getName());
				name2.insert(0, " ");
				name2.append(":");
				_lstRight->addRow(1, name2.c_str());
				_lstRight->setRowColor(row, getAltResearchColor(itMap.first->getName()));
				_rightTopics.push_back(itMap.first->getName());
				_rightFlags.push_back(TTV_RESEARCH);
				++row;
				for (auto* i : itMap.second)
				{
					std::string name = tr(i->getName());
					name.insert(0, "  ");
					_lstRight->addRow(1, name.c_str());
					_lstRight->setRowColor(row, getResearchColor(i->getName()));
					_rightTopics.push_back(i->getName());
					_rightFlags.push_back(TTV_RESEARCH);
					++row;
				}
			}
		}

		// 10. unlocks/disables alien missions, game arcs or geoscape events
		std::unordered_set<std::string> unlocksArcs, disablesArcs;
		std::unordered_set<std::string> unlocksEvents, disablesEvents;
		std::unordered_set<std::string> unlocksMissions, disablesMissions;
		bool affectsGameProgression = false;

		for (auto& arcScriptId : *_game->getMod()->getArcScriptList())
		{
			auto* arcScript = _game->getMod()->getArcScript(arcScriptId, false);
			if (arcScript)
			{
				for (auto& trigger : arcScript->getResearchTriggers())
				{
					if (trigger.first == _selectedTopic)
					{
						if (trigger.second)
							unlocksArcs.insert(arcScriptId);
						else
							disablesArcs.insert(arcScriptId);
					}
				}
			}
		}
		for (auto& eventScriptId : *_game->getMod()->getEventScriptList())
		{
			auto* eventScript = _game->getMod()->getEventScript(eventScriptId, false);
			if (eventScript)
			{
				for (auto& trigger : eventScript->getResearchTriggers())
				{
					if (trigger.first == _selectedTopic)
					{
						if (eventScript->getAffectsGameProgression()) affectsGameProgression = true; // remember for later
						if (trigger.second)
							unlocksEvents.insert(eventScriptId);
						else
							disablesEvents.insert(eventScriptId);
					}
				}
			}
		}
		for (auto& missionScriptId : *_game->getMod()->getMissionScriptList())
		{
			auto* missionScript = _game->getMod()->getMissionScript(missionScriptId, false);
			if (missionScript)
			{
				for (auto& trigger : missionScript->getResearchTriggers())
				{
					if (trigger.first == _selectedTopic)
					{
						if (trigger.second)
							unlocksMissions.insert(missionScriptId);
						else
							disablesMissions.insert(missionScriptId);
					}
				}
			}
		}
		bool showDetails = false;
		if (Options::isPasswordCorrect() && _game->isAltPressed())
		{
			showDetails = true;
		}
		if (showDetails)
		{
			auto addGameProgressionEntry = [&](const std::unordered_set<std::string>& list, const std::string& label)
			{
				if (!list.empty())
				{
					_lstRight->addRow(1, tr(label).c_str());
					_lstRight->setRowColor(row, _blue);
					_rightTopics.push_back("-");
					_rightFlags.push_back(TTV_NONE);
					++row;
					for (auto& i : list)
					{
						std::ostringstream name;
						name << "  " << tr(i);
						_lstRight->addRow(1, name.str().c_str());
						_lstRight->setRowColor(row, _white);
						_rightTopics.push_back("-");
						_rightFlags.push_back(TTV_NONE);
						++row;
					}
				}
			};

			addGameProgressionEntry(unlocksArcs, "STR_UNLOCKS_ARCS");
			addGameProgressionEntry(disablesArcs, "STR_DISABLES_ARCS");

			addGameProgressionEntry(unlocksMissions, "STR_UNLOCKS_MISSIONS");
			addGameProgressionEntry(disablesMissions, "STR_DISABLES_MISSIONS");

			addGameProgressionEntry(unlocksEvents, "STR_UNLOCKS_EVENTS");
			addGameProgressionEntry(disablesEvents, "STR_DISABLES_EVENTS");
		}
		else
		{
			int showDisclaimer = 0;
			if (!unlocksMissions.empty() || !disablesMissions.empty() || !unlocksArcs.empty() || !disablesArcs.empty())
			{
				showDisclaimer = 1; // STR_AFFECTS_GAME_PROGRESSION
			}
			else if (!unlocksEvents.empty() || !disablesEvents.empty())
			{
				if (affectsGameProgression)
				{
					showDisclaimer = 1; // STR_AFFECTS_GAME_PROGRESSION
				}
				else if (Options::isPasswordCorrect())
				{
					showDisclaimer = 2; // .
				}
			}
			if (showDisclaimer > 0)
			{
				_lstRight->addRow(1, tr("STR_AFFECTS_GAME_PROGRESSION").c_str());
				if (showDisclaimer == 1)
					_lstRight->setRowColor(row, _gold);
				else
					_lstRight->setRowColor(row, _white);
				_rightTopics.push_back("-");
				_rightFlags.push_back(TTV_NONE);
				++row;
			}
		}
	}
	else if (_selectedFlag == TTV_MANUFACTURING)
	{
		int row = 0;
		RuleManufacture *rule = _game->getMod()->getManufacture(_selectedTopic);
		if (rule == 0)
			return;

		// 1. requires
		const std::vector<const RuleResearch*> reqs = rule->getRequirements();
		if (reqs.size() > 0)
		{
			_lstLeft->addRow(1, tr("STR_RESEARCH_REQUIRED").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (auto* i : reqs)
			{
				std::string name = tr(i->getName());
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, getResearchColor(i->getName()));
				_leftTopics.push_back(i->getName());
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 2. requires services (from base facilities)
		if (rule->getRequireBaseFunc().any())
		{
			_lstLeft->addRow(1, tr("STR_SERVICES_REQUIRED").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& requiredServiceType : _game->getMod()->getBaseFunctionNames(rule->getRequireBaseFunc()))
			{
				std::string name = tr(requiredServiceType);
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, _gold);
				_leftTopics.push_back("-");
				_leftFlags.push_back(TTV_NONE);
				++row;
			}
		}

		// 3. inputs
		const std::map<const RuleCraft*, int> craftInputs = rule->getRequiredCrafts();
		const std::map<const RuleItem*, int> inputs = rule->getRequiredItems();
		if (inputs.size() > 0 || craftInputs.size() > 0)
		{
			_lstLeft->addRow(1, tr("STR_MATERIALS_REQUIRED").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (auto& i : craftInputs)
			{
				std::ostringstream name;
				name << "  ";
				name << tr(i.first->getType());
				name << ": ";
				name << i.second;
				_lstLeft->addRow(1, name.str().c_str());
				_lstLeft->setRowColor(row, _white);
				_leftTopics.push_back("-");
				_leftFlags.push_back(TTV_NONE);
				++row;
			}
			for (auto& i : inputs)
			{
				std::ostringstream name;
				name << "  ";
				name << tr(i.first->getType());
				name << ": ";
				name << i.second;
				_lstLeft->addRow(1, name.str().c_str());
				_lstLeft->setRowColor(row, _white);
				_leftTopics.push_back(i.first->getType());
				_leftFlags.push_back(TTV_ITEMS);
				++row;
			}
		}

		// empty line
		_lstLeft->addRow(1, "");
		_leftTopics.push_back("-");
		_leftFlags.push_back(TTV_NONE);
		++row;

		// cost per unit
		if (rule->getManufactureCost() > 0)
		{
			_lstLeft->addRow(1, tr("STR_TTV_COST_PER_UNIT").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;

			std::ostringstream txt;
			txt << "  ";
			txt << Unicode::formatFunding(rule->getManufactureCost());
			_lstLeft->addRow(1, txt.str().c_str());
			_lstLeft->setRowColor(row, _white);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
		}
		// engineer hours
		if (rule->getManufactureTime() > 0)
		{
			_lstLeft->addRow(1, tr("STR_TTV_ENGINEER_HOURS").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;

			int days = rule->getManufactureTime() / 24;
			int hours = rule->getManufactureTime() % 24;
			std::ostringstream txt;
			txt << "  ";
			txt << rule->getManufactureTime();
			txt << " (";
			if (days > 0)
			{
				txt << tr("STR_DAY_SHORT").arg(days);
			}
			if (hours > 0)
			{
				if (days > 0)
				{
					txt << "/";
				}
				txt << tr("STR_HOUR_SHORT").arg(hours);
			}
			txt << ")";
			_lstLeft->addRow(1, txt.str().c_str());
			_lstLeft->setRowColor(row, _white);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
		}
		// work space required
		if (rule->getRequiredSpace() > 0)
		{
			_lstLeft->addRow(1, tr("STR_TTV_WORK_SPACE_REQUIRED").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;

			std::ostringstream txt;
			txt << "  ";
			txt << rule->getRequiredSpace();
			_lstLeft->addRow(1, txt.str().c_str());
			_lstLeft->setRowColor(row, _white);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
		}

		row = 0;

		// 4. outputs
		const std::map<const RuleItem*, int> outputs = rule->getProducedItems();
		if (outputs.size() > 0 || rule->getProducedCraft())
		{
			_lstRight->addRow(1, tr("STR_ITEMS_PRODUCED").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
			if (rule->getProducedCraft())
			{
				std::ostringstream name;
				name << "  ";
				name << tr(rule->getProducedCraft()->getType());
				name << ": 1";
				_lstRight->addRow(1, name.str().c_str());
				_lstRight->setRowColor(row, _white);
				_rightTopics.push_back("-");
				_rightFlags.push_back(TTV_NONE);
				++row;
			}
			for (auto& i : outputs)
			{
				std::ostringstream name;
				name << "  ";
				name << tr(i.first->getType());
				name << ": ";
				name << i.second;
				_lstRight->addRow(1, name.str().c_str());
				_lstRight->setRowColor(row, _white);
				_rightTopics.push_back(i.first->getType());
				_rightFlags.push_back(TTV_ITEMS);
				++row;
			}
		}

		// 4b. random outputs
		auto& randomOutputs = rule->getRandomProducedItems();
		if (randomOutputs.size() > 0)
		{
			_lstRight->addRow(1, tr("STR_RANDOM_PRODUCTION_DISCLAIMER").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
			int total = 0;
			for (auto& n : randomOutputs)
			{
				total += n.first;
			}
			for (auto& randomOutput : randomOutputs)
			{
				std::ostringstream chance;
				chance << " " << randomOutput.first * 100 / total << "%";
				_lstRight->addRow(1, chance.str().c_str());
				_lstRight->setRowColor(row, _gold);
				_rightTopics.push_back("-");
				_rightFlags.push_back(TTV_NONE);
				++row;
				for (auto& i : randomOutput.second)
				{
					std::ostringstream name;
					name << "  ";
					name << tr(i.first->getType());
					name << ": ";
					name << i.second;
					_lstRight->addRow(1, name.str().c_str());
					_lstRight->setRowColor(row, _white);
					_rightTopics.push_back(i.first->getType());
					_rightFlags.push_back(TTV_ITEMS);
					++row;
				}
			}
		}

		// 5. person joining
		if (rule->getSpawnedPersonType() != "")
		{
			_lstRight->addRow(1, tr("STR_PERSON_RECRUITED").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;

			// person joining
			std::ostringstream name;
			name << "  ";
			if (!rule->getSpawnedSoldierTemplate().IsNull())
			{
				name << "*";
			}
			name << tr(rule->getSpawnedPersonName() != "" ? rule->getSpawnedPersonName() : rule->getSpawnedPersonType());
			_lstRight->addRow(1, name.str().c_str());
			_lstRight->setRowColor(row, _white);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
		}
	}
	else if (_selectedFlag == TTV_FACILITIES)
	{
		int row = 0;
		RuleBaseFacility *rule = _game->getMod()->getBaseFacility(_selectedTopic);
		if (rule == 0)
			return;

		// 1. requires
		const std::vector<std::string> reqs = rule->getRequirements();
		if (reqs.size() > 0)
		{
			_lstLeft->addRow(1, tr("STR_RESEARCH_REQUIRED").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& res : reqs)
			{
				std::string name = tr(res);
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, getResearchColor(res));
				_leftTopics.push_back(res);
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 2. requires services (from other base facilities)
		if (rule->getRequireBaseFunc().any())
		{
			_lstLeft->addRow(1, tr("STR_SERVICES_REQUIRED").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& requiredServiceType : _game->getMod()->getBaseFunctionNames(rule->getRequireBaseFunc()))
			{
				std::string name = tr(requiredServiceType);
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, _gold);
				_leftTopics.push_back("-");
				_leftFlags.push_back(TTV_NONE);
				++row;
			}
		}

		row = 0;

		// 3. provides services
		if (rule->getProvidedBaseFunc().any())
		{
			_lstRight->addRow(1, tr("STR_SERVICES_PROVIDED").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
			for (const auto& providedServiceType : _game->getMod()->getBaseFunctionNames(rule->getProvidedBaseFunc()))
			{
				std::string name = tr(providedServiceType);
				name.insert(0, "  ");
				_lstRight->addRow(1, name.c_str());
				_lstRight->setRowColor(row, _gold);
				_rightTopics.push_back("-");
				_rightFlags.push_back(TTV_NONE);
				++row;
			}
		}

		// 4. forbids services
		if (rule->getForbiddenBaseFunc().any())
		{
			_lstRight->addRow(1, tr("STR_SERVICES_FORBIDDEN").c_str());
			_lstRight->setRowColor(row, _blue);
			_rightTopics.push_back("-");
			_rightFlags.push_back(TTV_NONE);
			++row;
			for (const auto& forbiddenServiceType : _game->getMod()->getBaseFunctionNames(rule->getForbiddenBaseFunc()))
			{
				std::string name = tr(forbiddenServiceType);
				name.insert(0, "  ");
				_lstRight->addRow(1, name.c_str());
				_lstRight->setRowColor(row, _white);
				_rightTopics.push_back("-");
				_rightFlags.push_back(TTV_NONE);
				++row;
			}
		}
	}
	else if (_selectedFlag == TTV_ITEMS)
	{
		_lstLeft->setVisible(false);
		_lstRight->setVisible(false);
		_lstFull->setVisible(true);

		int row = 0;
		RuleItem *rule = _game->getMod()->getItem(_selectedTopic);
		if (rule == 0)
			return;

		// 1. requires to use/equip
		const std::vector<const RuleResearch *> reqs = rule->getRequirements();
		if (reqs.size() > 0)
		{
			_lstFull->addRow(1, tr("STR_RESEARCH_REQUIRED_USE").c_str());
			_lstFull->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (auto* i : reqs)
			{
				std::string name = tr(i->getName());
				name.insert(0, "  ");
				_lstFull->addRow(1, name.c_str());
				_lstFull->setRowColor(row, getResearchColor(i->getName()));
				_leftTopics.push_back(i->getName());
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 2. requires to buy
		const std::vector<const RuleResearch *> reqsBuy = rule->getBuyRequirements();
		if (reqsBuy.size() > 0)
		{
			_lstFull->addRow(1, tr("STR_RESEARCH_REQUIRED_BUY").c_str());
			_lstFull->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (auto* i : reqsBuy)
			{
				std::string name = tr(i->getName());
				name.insert(0, "  ");
				_lstFull->addRow(1, name.c_str());
				_lstFull->setRowColor(row, getResearchColor(i->getName()));
				_leftTopics.push_back(i->getName());
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 3. services (from base facilities) required to buy
		if (rule->getRequiresBuyBaseFunc().any())
		{
			const std::vector<std::string> servicesBuy = _game->getMod()->getBaseFunctionNames(rule->getRequiresBuyBaseFunc());
			_lstFull->addRow(1, tr("STR_SERVICES_REQUIRED_BUY").c_str());
			_lstFull->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (auto& i : servicesBuy)
			{
				std::string name = tr(i);
				name.insert(0, "  ");
				_lstFull->addRow(1, name.c_str());
				_lstFull->setRowColor(row, _gold);
				_leftTopics.push_back("-");
				_leftFlags.push_back(TTV_NONE);
				++row;
			}
		}

		// 4. produced by
		std::vector<std::string> producedBy;
		for (auto& j : _game->getMod()->getManufactureList())
		{
			RuleManufacture* temp = _game->getMod()->getManufacture(j);
			bool found = false;
			for (auto& i : temp->getProducedItems())
			{
				if (i.first == rule)
				{
					producedBy.push_back(j);
					found = true;
					break;
				}
			}
			if (!found)
			{
				for (auto& itMap : temp->getRandomProducedItems())
				{
					for (auto& i : itMap.second)
					{
						if (i.first == rule)
						{
							producedBy.push_back(j);
							found = true;
							break;
						}
					}
					if (found) break;
				}
			}
		}
		if (producedBy.size() > 0)
		{
			_lstFull->addRow(1, tr("STR_PRODUCED_BY").c_str());
			_lstFull->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& manuf : producedBy)
			{
				std::string name = tr(manuf);
				name.insert(0, "  ");
				name.append(tr("STR_M_FLAG"));
				_lstFull->addRow(1, name.c_str());
				if (!isDiscoveredManufacture(manuf))
				{
					_lstFull->setRowColor(row, _pink);
				}
				_leftTopics.push_back(manuf);
				_leftFlags.push_back(TTV_MANUFACTURING);
				++row;
			}
		}

		// 5. spawned by
		std::vector<std::string> spawnedBy;
		for (auto& j : _game->getMod()->getResearchList())
		{
			RuleResearch* temp = _game->getMod()->getResearch(j);
			if (temp->getSpawnedItem() == rule->getType())
			{
				spawnedBy.push_back(j);
			}
			else
			{
				for (auto& sil : temp->getSpawnedItemList())
				{
					if (sil == rule->getType())
					{
						spawnedBy.push_back(j);
						break;
					}
				}
			}
		}
		if (spawnedBy.size() > 0)
		{
			_lstFull->addRow(1, tr("STR_SPAWNED_BY").c_str());
			_lstFull->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& res : spawnedBy)
			{
				std::string name = tr(res);
				name.insert(0, "  ");
				_lstFull->addRow(1, name.c_str());
				_lstFull->setRowColor(row, getResearchColor(res));
				_leftTopics.push_back(res);
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// empty line
		_lstFull->addRow(1, "");
		_leftTopics.push_back("-");
		_leftFlags.push_back(TTV_NONE);
		++row;

		// cost to buy
		if (rule->getBuyCost() > 0)
		{
			_lstFull->addRow(1, tr("STR_TTV_COST_PER_UNIT").c_str());
			_lstFull->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;

			std::ostringstream txt;
			txt << "  ";
			txt << Unicode::formatFunding(rule->getBuyCost());
			_lstFull->addRow(1, txt.str().c_str());
			_lstFull->setRowColor(row, _white);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
		}
	}
	else if (_selectedFlag == TTV_CRAFTS)
	{
		int row = 0;
		RuleCraft *rule = _game->getMod()->getCraft(_selectedTopic);
		if (rule == 0)
			return;

		// 1. requires
		const std::vector<std::string> reqs = rule->getRequirements();
		if (reqs.size() > 0)
		{
			_lstLeft->addRow(1, tr("STR_RESEARCH_REQUIRED").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& res : reqs)
			{
				std::string name = tr(res);
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, getResearchColor(res));
				_leftTopics.push_back(res);
				_leftFlags.push_back(TTV_RESEARCH);
				++row;
			}
		}

		// 2. services (from base facilities) required to buy
		if (rule->getRequiresBuyBaseFunc().any())
		{
			_lstLeft->addRow(1, tr("STR_SERVICES_REQUIRED_BUY").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& serviceReqToBuy : _game->getMod()->getBaseFunctionNames(rule->getRequiresBuyBaseFunc()))
			{
				std::string name = tr(serviceReqToBuy);
				name.insert(0, "  ");
				_lstLeft->addRow(1, name.c_str());
				_lstLeft->setRowColor(row, _gold);
				_leftTopics.push_back("-");
				_leftFlags.push_back(TTV_NONE);
				++row;
			}
		}

		// 3. produced by
		std::vector<std::string> producedBy;
		for (auto& j : _game->getMod()->getManufactureList())
		{
			RuleManufacture* temp = _game->getMod()->getManufacture(j);
			if (temp->getProducedCraft() == rule)
			{
				producedBy.push_back(j);
				break;
			}
		}
		if (producedBy.size() > 0)
		{
			_lstLeft->addRow(1, tr("STR_PRODUCED_BY").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
			for (const auto& manuf : producedBy)
			{
				std::string name = tr(manuf);
				name.insert(0, "  ");
				name.append(tr("STR_M_FLAG"));
				_lstLeft->addRow(1, name.c_str());
				if (!isDiscoveredManufacture(manuf))
				{
					_lstLeft->setRowColor(row, _pink);
				}
				_leftTopics.push_back(manuf);
				_leftFlags.push_back(TTV_MANUFACTURING);
				++row;
			}
		}

		// empty line
		_lstLeft->addRow(1, "");
		_leftTopics.push_back("-");
		_leftFlags.push_back(TTV_NONE);
		++row;

		// cost to buy
		if (rule->getBuyCost() > 0)
		{
			_lstLeft->addRow(1, tr("STR_TTV_COST_PER_UNIT").c_str());
			_lstLeft->setRowColor(row, _blue);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;

			std::ostringstream txt;
			txt << "  ";
			txt << Unicode::formatFunding(rule->getBuyCost());
			_lstLeft->addRow(1, txt.str().c_str());
			_lstLeft->setRowColor(row, _white);
			_leftTopics.push_back("-");
			_leftFlags.push_back(TTV_NONE);
			++row;
		}
	}
}

/**
* Selects the topic.
* @param action Pointer to an action.
*/
void TechTreeViewerState::onSelectLeftTopic(Action *)
{
	int index = _lstLeft->getSelectedRow();
	if (_leftFlags[index] > TTV_NONE)
	{
		_history.push_back(std::make_pair(_selectedTopic, _selectedFlag));
		_selectedFlag = _leftFlags[index];
		_selectedTopic = _leftTopics[index];
		initLists();
	}
}

/**
* Selects the topic.
* @param action Pointer to an action.
*/
void TechTreeViewerState::onSelectRightTopic(Action *)
{
	int index = _lstRight->getSelectedRow();
	if (_rightFlags[index] > TTV_NONE)
	{
		_history.push_back(std::make_pair(_selectedTopic, _selectedFlag));
		_selectedFlag = _rightFlags[index];
		_selectedTopic = _rightTopics[index];
		initLists();
	}
}

/**
 * Selects the topic.
 * @param action Pointer to an action.
 */
void TechTreeViewerState::onSelectFullTopic(Action *)
{
	int index = _lstFull->getSelectedRow();
	if (_leftFlags[index] > TTV_NONE)
	{
		_history.push_back(std::make_pair(_selectedTopic, _selectedFlag));
		_selectedFlag = _leftFlags[index];
		_selectedTopic = _leftTopics[index];
		initLists();
	}
}

/**
* Changes the selected topic.
*/
void TechTreeViewerState::setSelectedTopic(const std::string &selectedTopic, TTVMode topicType)
{
	_history.push_back(std::make_pair(_selectedTopic, _selectedFlag));
	_selectedTopic = selectedTopic;
	_selectedFlag = topicType;
}

/**
 * Gets the color coding for the given research topic.
 */
Uint8 TechTreeViewerState::getResearchColor(const std::string &topic) const
{
	if (_disabledResearch.find(topic) != _disabledResearch.end())
	{
		return _grey; // disabled
	}
	if (_alreadyAvailableResearch.find(topic) == _alreadyAvailableResearch.end())
	{
		return _pink; // not discovered
	}
	return _purple; // discovered
}

/**
 * Gets the alternative color coding for the given research topic.
 */
Uint8 TechTreeViewerState::getAltResearchColor(const std::string &topic) const
{
	if (_disabledResearch.find(topic) != _disabledResearch.end())
	{
		return _grey; // disabled
	}
	if (_alreadyAvailableResearch.find(topic) == _alreadyAvailableResearch.end())
	{
		return _gold; // not discovered
	}
	return _white; // discovered
}

/**
* Is given research topic discovered/available?
*/
bool TechTreeViewerState::isDiscoveredResearch(const std::string &topic) const
{
	if (_alreadyAvailableResearch.find(topic) == _alreadyAvailableResearch.end())
	{
		return false;
	}
	return true;
}

/**
* Is given manufacture topic discovered/available?
*/
bool TechTreeViewerState::isDiscoveredManufacture(const std::string &topic) const
{
	if (_alreadyAvailableManufacture.find(topic) == _alreadyAvailableManufacture.end())
	{
		return false;
	}
	return true;
}

/**
* Is given base facility discovered/available?
*/
bool TechTreeViewerState::isDiscoveredFacility(const std::string &topic) const
{
	if (_alreadyAvailableFacilities.find(topic) == _alreadyAvailableFacilities.end())
	{
		return false;
	}
	return true;
}

/**
* Is given item protected by any research?
*/
bool TechTreeViewerState::isProtectedItem(const std::string &topic) const
{
	if (_protectedItems.find(topic) == _protectedItems.end())
	{
		return false;
	}
	return true;
}

/**
* Is given protected item discovered/available for both purchase and usage/equipment?
*/
bool TechTreeViewerState::isProtectedAndDiscoveredItem(const std::string &topic) const
{
	if (_alreadyAvailableItems.find(topic) == _alreadyAvailableItems.end())
	{
		return false;
	}
	return true;
}

/**
* Is given craft discovered/available?
*/
bool TechTreeViewerState::isDiscoveredCraft(const std::string &topic) const
{
	if (_alreadyAvailableCrafts.find(topic) == _alreadyAvailableCrafts.end())
	{
		return false;
	}
	return true;
}

}
