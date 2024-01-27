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
#include <vector>
#include <string>
#include <memory>

#include "ArticleState.h"

namespace OpenXcom
{
	class Game;
	class SavedGame;
	class Mod;
	class ArticleDefinition;
	class ArticleState;

	/// definition of an article list
	typedef std::vector<ArticleDefinition *> ArticleDefinitionList;

	// This section is meant for articles, that have to be activated,
	// but have no own entry in a list. E.g. Ammunition items.
	// Maybe others as well, that should just not be selectable.
	static const std::string UFOPAEDIA_NOT_AVAILABLE = "STR_NOT_AVAILABLE";
	static const std::string UFOPAEDIA_COMMENDATIONS = "STR_COMMENDATIONS_UC";

	/**
	 * This static class encapsulates all functions related to Ufopaedia
	 * for the game.
	 * Main purpose is to open Ufopaedia from Geoscape, navigate between articles
	 * and release new articles after successful research.
	 */

	class Ufopaedia
	{
	public:
		/// check, if a specific article is currently available.
		static bool isArticleAvailable(SavedGame *save, ArticleDefinition *article);

		/// open Ufopaedia on a certain entry.
		static void openArticle(Game *game, const std::string &article_id);
		static void openArticleDetail(Game *game, const std::string &article_id);

		/// open Ufopaedia article from a given article definition.
		static void openArticle(Game *game, ArticleDefinition *article);

		/// open Ufopaedia with selection dialog.
		static void open(Game *game);

		/// article navigation to next article.
		static void next(Game *game, std::shared_ptr<ArticleCommonState> state);
		static void nextDetail(Game *game, std::shared_ptr<ArticleCommonState> state, bool debug, bool ids, bool defaults);

		/// article navigation to previous article.
		static void prev(Game *game, std::shared_ptr<ArticleCommonState> state);
		static void prevDetail(Game *game, std::shared_ptr<ArticleCommonState> state, bool debug, bool ids, bool defaults);

		/// load a vector with article ids that are currently visible of a given section.
		static void list(SavedGame *save, Mod *rule, const std::string &section, ArticleDefinitionList &data);

		/// check if this is an invisible commendation article.
		static bool isCommendationArticleInvisible(SavedGame *save, ArticleDefinition *article);

		/// check if the article corresponds to an awarded commendation.
		static bool isAwardedCommendation(SavedGame *save, ArticleDefinition *article);

	protected:

		/// get index of the given article id in the visible list.
		static size_t getArticleIndex(const ArticleDefinitionList& article, const std::string &article_id);

		/// get list of researched articles
		static std::shared_ptr<ArticleCommonState> createCommonArticleState(SavedGame *save, Mod *rule);

		/// create a new state object from article definition.
		static ArticleState *createArticleState(std::shared_ptr<ArticleCommonState> state);
	};
}
