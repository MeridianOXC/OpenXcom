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

#include "Ufopaedia.h"
#include "UfopaediaStartState.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDiary.h"
#include "../Mod/Mod.h"
#include "../Mod/ArticleDefinition.h"
#include "ArticleState.h"
#include "ArticleStateBaseFacility.h"
#include "ArticleStateCraft.h"
#include "ArticleStateCraftWeapon.h"
#include "ArticleStateItem.h"
#include "ArticleStateArmor.h"
#include "ArticleStateText.h"
#include "ArticleStateTextImage.h"
#include "ArticleStateUfo.h"
#include "ArticleStateVehicle.h"
#include "ArticleStateTFTD.h"
#include "ArticleStateTFTDArmor.h"
#include "ArticleStateTFTDVehicle.h"
#include "ArticleStateTFTDItem.h"
#include "ArticleStateTFTDFacility.h"
#include "ArticleStateTFTDCraft.h"
#include "ArticleStateTFTDCraftWeapon.h"
#include "ArticleStateTFTDUso.h"
#include "StatsForNerdsState.h"
#include "../Engine/Game.h"

namespace OpenXcom
{
	/**
	 * Checks, if an article has already been released.
	 * @param save Pointer to saved game.
	 * @param article Article definition to release.
	 * @returns true, if the article is available.
	 */
	bool Ufopaedia::isArticleAvailable(SavedGame *save, ArticleDefinition *article)
	{
		if (!article->disabledBy.empty())
		{
			// Note: yes, check for non-empty vector is necessary
			if (save->isResearched(article->disabledBy, false))
			{
				return false; // article is disabled, i.e. not available
			}
		}
		return save->isResearched(article->_requires);
	}

	/**
	 * Gets the index of the selected article_id in the visible list.
	 * If the id is not found, returns -1.
	 * @param save Pointer to saved game.
	 * @param mod Pointer to mod.
	 * @param article_id Article id to find.
	 * @returns Index of the given article id in the internal list, -1 if not found.
	 */
	size_t Ufopaedia::getArticleIndex(const ArticleDefinitionList& articles, const std::string &article_id)
	{
		std::string UC_ID = article_id + "_UC";
		for (size_t it=0; it<articles.size(); ++it)
		{
			if (articles[it]->id == article_id)
			{
				return it;
			}
		}
		for (size_t it = 0; it<articles.size(); ++it)
		{
			if (articles[it]->id == UC_ID)
			{
				return it;
			}
		}
		for (size_t it = 0; it<articles.size(); ++it)
		{
			for (const auto& req : articles[it]->_requires)
			{
				if (article_id == req)
				{
					return it;
				}
			}
		}
		return ArticleCommonState::invalid;
	}

	/**
	 * Creates a new article state dependent on the given article definition.
	 * @param game Pointer to actual game.
	 * @param article Article definition to create from.
	 * @returns Article state object if created, 0 otherwise.
	 */
	ArticleState *Ufopaedia::createArticleState(std::shared_ptr<ArticleCommonState> state)
	{
		ArticleDefinition *article = state->getCurrentArticle();
		switch (article->getType())
		{
			case UFOPAEDIA_TYPE_CRAFT:
				return new ArticleStateCraft(dynamic_cast<ArticleDefinitionCraft *>(article), std::move(state));
			case UFOPAEDIA_TYPE_CRAFT_WEAPON:
				return new ArticleStateCraftWeapon(dynamic_cast<ArticleDefinitionCraftWeapon *>(article), std::move(state));
			case UFOPAEDIA_TYPE_VEHICLE:
				return new ArticleStateVehicle(dynamic_cast<ArticleDefinitionVehicle *>(article), std::move(state));
			case UFOPAEDIA_TYPE_ITEM:
				return new ArticleStateItem(dynamic_cast<ArticleDefinitionItem *>(article), std::move(state));
			case UFOPAEDIA_TYPE_ARMOR:
				return new ArticleStateArmor(dynamic_cast<ArticleDefinitionArmor *>(article), std::move(state));
			case UFOPAEDIA_TYPE_BASE_FACILITY:
				return new ArticleStateBaseFacility(dynamic_cast<ArticleDefinitionBaseFacility *>(article), std::move(state));
			case UFOPAEDIA_TYPE_TEXT:
				return new ArticleStateText(dynamic_cast<ArticleDefinitionText *>(article), std::move(state));
			case UFOPAEDIA_TYPE_TEXTIMAGE:
				return new ArticleStateTextImage(dynamic_cast<ArticleDefinitionTextImage *>(article), std::move(state));
			case UFOPAEDIA_TYPE_UFO:
				return new ArticleStateUfo(dynamic_cast<ArticleDefinitionUfo *>(article), std::move(state));
			case UFOPAEDIA_TYPE_TFTD:
				return new ArticleStateTFTD(dynamic_cast<ArticleDefinitionTFTD *>(article), std::move(state));
			case UFOPAEDIA_TYPE_TFTD_CRAFT:
				return new ArticleStateTFTDCraft(dynamic_cast<ArticleDefinitionTFTD *>(article), std::move(state));
			case UFOPAEDIA_TYPE_TFTD_CRAFT_WEAPON:
				return new ArticleStateTFTDCraftWeapon(dynamic_cast<ArticleDefinitionTFTD *>(article), std::move(state));
			case UFOPAEDIA_TYPE_TFTD_VEHICLE:
				return new ArticleStateTFTDVehicle(dynamic_cast<ArticleDefinitionTFTD *>(article), std::move(state));
			case UFOPAEDIA_TYPE_TFTD_ITEM:
				return new ArticleStateTFTDItem(dynamic_cast<ArticleDefinitionTFTD *>(article), std::move(state));
			case UFOPAEDIA_TYPE_TFTD_ARMOR:
				return new ArticleStateTFTDArmor(dynamic_cast<ArticleDefinitionTFTD *>(article), std::move(state));
			case UFOPAEDIA_TYPE_TFTD_BASE_FACILITY:
				return new ArticleStateTFTDFacility(dynamic_cast<ArticleDefinitionTFTD *>(article), std::move(state));
			case UFOPAEDIA_TYPE_TFTD_USO:
				return new ArticleStateTFTDUso(dynamic_cast<ArticleDefinitionTFTD *>(article), std::move(state));
			default:
				throw new Exception("Unknown type for article '" + article->id + "'");
		}
	}

	/**
	 * Set UPSaved index and open the new state.
	 * @param game Pointer to actual game.
	 * @param article Article definition of the article to open.
	 */
	void Ufopaedia::openArticle(Game *game, ArticleDefinition *article)
	{
		auto state = createCommonArticleState(game->getSavedGame(), game->getMod());
		state->current_index = getArticleIndex(state->articleList, article->id);
		if (state->current_index != ArticleCommonState::invalid)
		{
			game->pushState(createArticleState(std::move(state)));
		}
	}

	/**
	 * Checks if selected article_id is available -> if yes, open it.
	 * @param game Pointer to actual game.
	 * @param article_id Article id to find.
	 */
	void Ufopaedia::openArticle(Game *game, const std::string &article_id)
	{
		auto state = createCommonArticleState(game->getSavedGame(), game->getMod());
		state->current_index = getArticleIndex(state->articleList, article_id);
		if (state->current_index != ArticleCommonState::invalid)
		{
			game->pushState(createArticleState(std::move(state)));
		}
	}

	/**
	 * Checks if selected article_id is available -> if yes, open its (raw) details.
	 * @param game Pointer to actual game.
	 * @param article_id Article id to find.
	 */
	void Ufopaedia::openArticleDetail(Game *game, const std::string &article_id)
	{
		auto state = createCommonArticleState(game->getSavedGame(), game->getMod());
		state->current_index = getArticleIndex(state->articleList, article_id);
		if (state->current_index != ArticleCommonState::invalid)
		{
			game->pushState(new StatsForNerdsState(std::move(state), false, false, false));
		}
	}

	/**
	 * Open Ufopaedia start state, presenting the section selection buttons.
	 * @param game Pointer to actual game.
	 */
	void Ufopaedia::open(Game *game)
	{
		game->pushState(new UfopaediaStartState);
	}

	/**
	 * Open the next article in the list. Loops to the first.
	 * @param game Pointer to actual game.
	 */
	void Ufopaedia::next(Game *game, std::shared_ptr<ArticleCommonState> state)
	{
		if (state->isCurrentArticleHidden()) return;
		state->nextArticlePage();
		game->popState();
		game->pushState(createArticleState(std::move(state)));
	}

	/**
	 * Open the next article detail (Stats for Nerds) in the list. Loops to the first.
	 * @param game Pointer to actual game.
	 */
	void Ufopaedia::nextDetail(Game *game, std::shared_ptr<ArticleCommonState> state, bool debug, bool ids, bool defaults)
	{
		if (state->isCurrentArticleHidden()) return;
		state->nextArticle();
		game->popState();
		game->pushState(new StatsForNerdsState(std::move(state), debug, ids, defaults));
	}

	/**
	 * Open the previous article in the list. Loops to the last.
	 * @param game Pointer to actual game.
	 */
	void Ufopaedia::prev(Game *game, std::shared_ptr<ArticleCommonState> state)
	{
		if (state->isCurrentArticleHidden()) return;
		state->prevArticlePage();
		game->popState();
		game->pushState(createArticleState(std::move(state)));
	}

	/**
	 * Open the previous article detail (Stats for Nerds) in the list. Loops to the last.
	 * @param game Pointer to actual game.
	 */
	void Ufopaedia::prevDetail(Game *game, std::shared_ptr<ArticleCommonState> state, bool debug, bool ids, bool defaults)
	{
		if (state->isCurrentArticleHidden()) return;
		state->prevArticle();
		game->popState();
		game->pushState(new StatsForNerdsState(std::move(state), debug, ids, defaults));
	}

	/**
	 * Fill an ArticleList with the currently visible ArticleIds of the given section.
	 * @param save Pointer to saved game.
	 * @param mod Pointer to mod.
	 * @param section Article section to find, e.g. "XCOM Crafts & Armaments", "Alien Lifeforms", etc.
	 * @param data Article definition list object to fill data in.
	 */
	void Ufopaedia::list(SavedGame *save, Mod *mod, const std::string &section, ArticleDefinitionList &data)
	{
		auto state = createCommonArticleState(save, mod);
		for (auto* a : state->articleList)
		{
			if (a->section == section)
			{
				data.push_back(a);
			}
		}
	}

	/**
	 * Check if this is an invisible commendation article.
	 * @param save Pointer to saved game.
	 * @param article Article to check.
	 */
	bool Ufopaedia::isCommendationArticleInvisible(SavedGame *save, ArticleDefinition *article)
	{
		// show hidden Commendations entries if:
		if (article->hiddenCommendation)
		{
			// 1. debug mode is on
			if (save->getDebugMode())
			{
				return false;
			}

			// 2. or if the article was opened already
			if (save->getUfopediaRuleStatus(article->id) != ArticleDefinition::PEDIA_STATUS_NEW)
			{
				return false;
			}

			// 3. or if the medal was awarded at least once
			if (isAwardedCommendation(save, article))
			{
				return false;
			}

			return true;
		}

		return false;
	}

	/**
	 * Check if the article corresponds to an awarded commendation.
	 * @param save Pointer to saved game.
	 * @param article Article to check.
	 */
	bool Ufopaedia::isAwardedCommendation(SavedGame *save, ArticleDefinition *article)
	{
		if (article->section == UFOPAEDIA_COMMENDATIONS)
		{
			// 1. check living soldiers
			for (auto* xbase : *save->getBases())
			{
				for (auto* soldier : *xbase->getSoldiers())
				{
					for (auto* comm : *soldier->getDiary()->getSoldierCommendations())
					{
						if (comm->getType() == article->getMainTitle())
						{
							return true;
						}
					}
				}
			}

			// 2. check dead soldiers
			for (std::vector<Soldier*>::reverse_iterator deadManIt = save->getDeadSoldiers()->rbegin(); deadManIt != save->getDeadSoldiers()->rend(); ++deadManIt)
			{
				for (auto* comm : *(*deadManIt)->getDiary()->getSoldierCommendations())
				{
					if (comm->getType() == article->getMainTitle())
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	/**
	 * Return an ArticleList with all the currently visible ArticleIds.
	 * @param save Pointer to saved game.
	 * @param mod Pointer to mod.
	 * @return List of visible ArticleDefinitions.
	 */
	std::shared_ptr<ArticleCommonState> Ufopaedia::createCommonArticleState(SavedGame *save, Mod *mod)
	{
		auto shared = std::make_shared<ArticleCommonState>();
		for (const auto& articleName : mod->getUfopaediaList())
		{
			ArticleDefinition *article = mod->getUfopaediaArticle(articleName);
			if (isArticleAvailable(save, article) && article->section != UFOPAEDIA_NOT_AVAILABLE && !isCommendationArticleInvisible(save, article))
			{
				shared->articleList.push_back(article);
				shared->articleStatusList.push_back(save->getUfopediaRuleStatus(articleName) == ArticleDefinition::PEDIA_STATUS_HIDDEN);
			}
		}
		return shared;
	}

}
