#pragma once
/*
 * Copyright 2010-2018 OpenXcom Developers.
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
#include "../Battlescape/Position.h"
#include "../Engine/State.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/Armor.h"
#include "../Mod/MapData.h"
#include "../Mod/RuleDamageType.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/BattleUnit.h"

namespace OpenXcom
{

class Window;
class Text;
class ComboBox;
class TextList;
class ToggleTextButton;
class TextButton;
class ScriptParserBase;
struct ArticleCommonState;
struct ArmorMoveCost;
template<typename T, typename I> class ScriptValues;

/**
 * A screen, where you can see the (almost) raw ruleset corresponding to the given Ufopedia article.
 */
class StatsForNerdsState : public State
{
private:
	Window *_window;
	Text *_txtTitle;
	ComboBox *_cbxRelatedStuff;
	Text *_txtArticle;
	TextButton *_btnPrev, *_btnNext;
	TextList *_lstRawData;
	ToggleTextButton *_btnIncludeDebug, *_btnIncludeIds, *_btnIncludeDefaults;
	TextButton *_btnOk;
	TextButton *_btnPreview;

	Uint8 _purple, _pink, _blue, _white, _gold;

	UfopaediaTypeId _typeId;
	std::string _topicId;
	bool _mainArticle;

	std::shared_ptr<ArticleCommonState> _state;
	std::vector<std::string> _filterOptions;
	bool _showDebug, _showIds, _showDefaults;
	int _counter;
	bool _indent;

	void buildUI(bool debug, bool ids, bool defaults);
	void initLists();
	void resetStream(std::ostringstream &ss);
	void addTranslation(std::ostringstream &ss, const std::string &id);
	std::string trp(const std::string &propertyName);
	void addSection(const std::string &name, const std::string &desc, Uint8 color, bool forceShow = false);
	void addHeading(const std::string &propertyName, const std::string &moreDetail = "", bool addDifficulty = false);
	void endHeading();

	template<typename T, typename Callback>
	void addVectorOfGeneric(std::ostringstream &ss, const std::vector<T> &vec, const std::string &propertyName, Callback&& func, bool translate = true);

	void addSingleString(std::ostringstream &ss, const std::string &id, const std::string &propertyName, const std::string &defaultId = "", bool translate = true);
	void addVectorOfStrings(std::ostringstream &ss, const std::vector<std::string> &vec, const std::string &propertyName, bool translate = true);

	void addVectorOfResearch(std::ostringstream &ss, const std::vector<const RuleResearch *> &vec, const std::string &propertyName);

	template<typename T>
	void addRule(std::ostringstream &ss, T* rule, const std::string &propertyName);
	template<typename T>
	void addRuleId(std::ostringstream &ss, T* rule, const std::string &propertyName);
	template<typename T>
	void addRuleNamed(std::ostringstream &ss, T* rule, const std::string &propertyName);

	template<typename T>
	void addVectorOfRules(std::ostringstream &ss, const std::vector<T*> &vec, const std::string &propertyName);
	template<typename T>
	void addVectorOfRulesId(std::ostringstream &ss, const std::vector<T*> &vec, const std::string &propertyName);
	template<typename T>
	void addVectorOfRulesNamed(std::ostringstream &ss, const std::vector<T*> &vec, const std::string &propertyName);

	template<typename T, typename I>
	void addScriptTags(std::ostringstream &ss, const ScriptValues<T, I> &vec);

	void addIntegerScript(const std::string &propertyName, const int &value);
	void addTextScript(const std::string &propertyName, const std::string &value);
	void addTextFormat1Script(const std::string &propertyName, const std::string &format, const int &value1);
	void addTextFormat2Script(const std::string &propertyName, const std::string &format, const int &value1, const int &value2);

	void addBoolean(std::ostringstream &ss, const bool &value, const std::string &propertyName, const bool &defaultvalue = false);
	void addFloat(std::ostringstream &ss, const float &value, const std::string &propertyName, const float &defaultvalue = 0.0f);
	void addFloatAsPercentage(std::ostringstream &ss, const float &value, const std::string &propertyName, const float &defaultvalue = 0.0f);
	void addDouble(std::ostringstream &ss, const double &value, const std::string &propertyName, const double &defaultvalue = 0.0);
	void addInteger(std::ostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0, bool formatAsMoney = false, const std::string &specialTranslation = "", const int &specialvalue = -1);
	void addIntegerScriptTag(std::ostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0);
	void addIntegerPercent(std::ostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0);
	void addIntegerNauticalMiles(std::ostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0);
	void addIntegerKnots(std::ostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0);
	void addIntegerKm(std::ostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0);
	void addIntegerSeconds(std::ostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0, const int &value2 = -1);
	void addVectorOfIntegers(std::ostringstream &ss, const std::vector<int> &vec, const std::string &propertyName);
	void addBattleType(std::ostringstream &ss, const BattleType &value, const std::string &propertyName, const BattleType &defaultvalue = BT_NONE);
	void addDamageType(std::ostringstream &ss, const ItemDamageType &value, const std::string &propertyName, const ItemDamageType &defaultvalue = DT_NONE);
	void addDamageRandomType(std::ostringstream &ss, const ItemDamageRandomType &value, const std::string &propertyName, const ItemDamageRandomType &defaultvalue = DRT_DEFAULT);
	void addBattleFuseType(std::ostringstream &ss, const BattleFuseType &value, const std::string &propertyName, const BattleFuseType &defaultvalue = BFT_NONE);
	void addRuleItemUseCostBasic(std::ostringstream &ss, const RuleItemUseCost &value, const std::string &propertyName, const int &defaultvalue = 0);
	void addBoolOrInteger(std::ostringstream &ss, const int &value, bool formatAsBoolean);
	void addPercentageSignOrNothing(std::ostringstream &ss, const int &value, bool smartFormat);
	template<typename T>
	void addRuleItemUseCostFull(std::ostringstream &ss, const RuleItemUseRuleBase<T> &value, const std::string &propertyName, const RuleItemUseRuleBase<T> &defaultvalue = RuleItemUseRuleBase<T>(), bool smartFormat = false, const RuleItemUseFlat &formatBy = RuleItemUseFlat());
	void addBattleMediKitType(std::ostringstream &ss, const BattleMediKitType &value, const std::string &propertyName, const BattleMediKitType &defaultvalue = BMT_NORMAL);
	void addMediKitTargets(std::ostringstream& ss, const RuleItem* value, const std::string& propertyName, const int& defaultvalue);
	void addItemTargets(std::ostringstream& ss, const RuleItem* value, const std::string& propertyName, const int& defaultvalue);
	void addExperienceTrainingMode(std::ostringstream &ss, const ExperienceTrainingMode &value, const std::string &propertyName, const ExperienceTrainingMode &defaultvalue = ETM_DEFAULT);
	void addRuleStatBonus(std::ostringstream &ss, const RuleStatBonus &value, const std::string &propertyName);
	void addRuleArmorMoveCost(std::ostringstream &ss, const ArmorMoveCost &value, const std::string &propertyName, const ArmorMoveCost &defaultvalue = ArmorMoveCost());
	void addSpriteResourcePath(std::ostringstream &ss, Mod *mod, const std::string &resourceSetName, const int &resourceId);
	void addSoundVectorResourcePaths(std::ostringstream &ss, Mod *mod, const std::string &resourceSetName, const std::vector<int> &resourceIds);
	void initItemList();

	void addUnitStatFormatted(std::ostringstream &ss, const int &value, const std::string &label, bool &isFirst);
	void addUnitStatBonus(std::ostringstream &ss, const UnitStats &value, const std::string &propertyName);
	void addArmorDamageModifiers(std::ostringstream &ss, const std::vector<float> &vec, const std::string &propertyName);
	void addSpecialAbility(std::ostringstream &ss, const SpecialAbility &value, const std::string &propertyName, const SpecialAbility &defaultvalue = SPECAB_NONE);
	void addMovementType(std::ostringstream &ss, const MovementType &value, const std::string &propertyName, const MovementType &defaultvalue = MT_WALK);
	void addForcedTorso(std::ostringstream &ss, const ForcedTorso &value, const std::string &propertyName, const ForcedTorso &defaultvalue = TORSO_USE_GENDER);
	void addDrawingRoutine(std::ostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0);
	void initArmorList();
	void initSoldierBonusList();

	void addVectorOfPositions(std::ostringstream &ss, const std::vector<Position> &vec, const std::string &propertyName);
	void addBuildCostItem(std::ostringstream &ss, const std::pair<const std::string, std::pair<int, int> > &costItem);
	void addRightClickActionType(std::ostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0);
	void initFacilityList();
	void initCraftList();

	void addHuntMode(std::ostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0);
	void addHuntBehavior(std::ostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0);
	void initUfoList();
	void initCraftWeaponList();

public:
	/// Name of class used in script.
	static constexpr const char *ScriptName = "StatsForNerds";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	static const std::map<std::string, std::string> translationMap;
	static const std::map<std::string, std::string> shortTranslationMap;


	/// Creates the StatsForNerdsState state.
	StatsForNerdsState(std::shared_ptr<ArticleCommonState> state, bool debug, bool ids, bool defaults);
	StatsForNerdsState(const UfopaediaTypeId typeId, const std::string topicId, bool debug, bool ids, bool defaults);
	/// Cleans up the StatsForNerdsState state.
	~StatsForNerdsState();
	/// Initializes the state.
	void init() override;

	/// Get ufopaedia type Id.
	UfopaediaTypeId getTypeId() const { return _typeId; }
	/// Get current topic Id.
	const std::string& getTopicId() const { return _topicId; }

	/// Handler for selecting an item from the [Compatible ammo] combobox.
	void cbxAmmoSelect(Action *action);
	/// Handler for clicking the toggle buttons.
	void btnRefreshClick(Action *action);
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the [Preview] button.
	void btnPreviewClick(Action *action);
	/// Handler for clicking the [Previous] button.
	void btnPrevClick(Action *action);
	/// Handler for clicking the [Next] button.
	void btnNextClick(Action *action);
	/// Handler for clicking the [Scroll Up] button.
	void btnScrollUpClick(Action *action);
	/// Handler for clicking the [Scroll Down] button.
	void btnScrollDownClick(Action *action);
};

}
