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
#include "../Mod/RuleItem.h"
#include <string>
#include <memory>

namespace OpenXcom
{
	class Game;
	class Action;
	class Surface;
	class TextButton;
	class ArticleDefinition;


	/// Current state of ufopedia
	struct ArticleCommonState
	{
		/// Invalid index.
		static constexpr size_t invalid = -1;

		/// Current selected article index (for previous/next navigation).
		size_t current_index = invalid;

		/// Current sub page of article.
		size_t current_page = 0;

		/// List of all available articles.
		std::vector<ArticleDefinition *> articleList;
		std::vector<bool> articleStatusList;

		/// Get current Article definition for current index position.
		ArticleDefinition* getCurrentArticle() const
		{
			return articleList[current_index];
		}
		bool isCurrentArticleHidden() const
		{
			return articleStatusList[current_index];
		}

		/// Change index position to next article
		void nextArticle();

		/// Change page to next in article or move to next index position.
		void nextArticlePage();
		bool hasNextArticlePage();

		/// Change index position to previous article.
		void prevArticle();

		/// Change page to previous in article or move to previous index position.
		void prevArticlePage();
		bool hasPrevArticlePage();
	};

	/**
	 * UfopaediaArticle is the base class for all articles of various types.
	 *
	 * It encapsulates the basic characteristics.
	 */

	class ArticleState : public State
	{
	protected:
		/// constructor (protected, so it can only be instantiated by derived classes)
		ArticleState(const std::string &article_id, std::shared_ptr<ArticleCommonState> state);
		/// destructor
		virtual ~ArticleState();

	public:
		/// return the article id
		std::string getId() const { return _id; }

	protected:

		/// converts damage type to string
		std::string getDamageTypeText(ItemDamageType dt) const;

		/// screen layout helpers
		void initLayout();

		/// callback for OK button
		void btnOkClick(Action *action);
		void btnResetMusicClick(Action *action);

		/// callback for PREV button
		void btnPrevClick(Action *action);

		/// callback for NEXT button
		void btnNextClick(Action *action);

		/// callback for INFO button
		void btnInfoClick(Action *action);

		/// the article id
		std::string _id;

		/// screen elements common to all articles!
		Surface *_bg;
		TextButton *_btnOk;
		TextButton *_btnPrev;
		TextButton *_btnNext;
		TextButton *_btnInfo;

		/// Shared state.
		std::shared_ptr<ArticleCommonState> _state;
	};
}
