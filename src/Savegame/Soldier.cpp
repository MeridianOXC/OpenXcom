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
#include <cmath>
#include <algorithm>
#include "Soldier.h"
#include "../Engine/Collections.h"
#include "../Engine/RNG.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/ScriptBind.h"
#include "Craft.h"
#include "EquipmentLayoutItem.h"
#include "SoldierDeath.h"
#include "SoldierDiary.h"
#include "../Mod/SoldierNamePool.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleSoldierBonus.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"
#include "../Mod/StatString.h"
#include "../Engine/Unicode.h"
#include "../Mod/RuleSoldierTransformation.h"
#include "../Mod/RuleCommendations.h"
#include "Base.h"
#include "ItemContainer.h"

namespace OpenXcom
{

/**
 * Initializes a new soldier, either blank or randomly generated.
 * @param rules Soldier ruleset.
 * @param armor Soldier armor.
 * @param id Unique soldier id for soldier generation.
 */
Soldier::Soldier(RuleSoldier *rules, Armor *armor, int nationality, int id) :
	_id(id), _nationality(0),
	_improvement(0), _psiStrImprovement(0), _rules(rules), _rank(RANK_ROOKIE), _craft(0),
	_gender(GENDER_MALE), _look(LOOK_BLONDE), _lookVariant(0), _missions(0), _kills(0), _stuns(0),
	_recentlyPromoted(false), _psiTraining(false), _training(false), _returnToTrainingWhenHealed(false),
	_armor(armor), _replacedArmor(0), _transformedArmor(0), _personalEquipmentArmor(nullptr), _death(0), _diary(new SoldierDiary()),
	_corpseRecovered(false)
{
	if (id != 0)
	{
		UnitStats minStats = rules->getMinStats();
		UnitStats maxStats = rules->getMaxStats();

		_initialStats.tu = RNG::generate(minStats.tu, maxStats.tu);
		_initialStats.stamina = RNG::generate(minStats.stamina, maxStats.stamina);
		_initialStats.health = RNG::generate(minStats.health, maxStats.health);
		_initialStats.mana = RNG::generate(minStats.mana, maxStats.mana);
		_initialStats.bravery = RNG::generate(minStats.bravery/10, maxStats.bravery/10)*10;
		_initialStats.reactions = RNG::generate(minStats.reactions, maxStats.reactions);
		_initialStats.firing = RNG::generate(minStats.firing, maxStats.firing);
		_initialStats.throwing = RNG::generate(minStats.throwing, maxStats.throwing);
		_initialStats.strength = RNG::generate(minStats.strength, maxStats.strength);
		_initialStats.psiStrength = RNG::generate(minStats.psiStrength, maxStats.psiStrength);
		_initialStats.melee = RNG::generate(minStats.melee, maxStats.melee);
		_initialStats.psiSkill = minStats.psiSkill;

		_currentStats = _initialStats;

		const std::vector<SoldierNamePool*> &names = rules->getNames();
		if (!names.empty())
		{
			if (nationality > -1)
			{
				// nationality by location, or hardcoded/technical nationality
				_nationality = nationality;
			}
			else
			{
				// nationality by name pool weights
				int tmp = RNG::generate(0, rules->getTotalSoldierNamePoolWeight());
				int nat = 0;
				for (auto* namepool : names)
				{
					if (tmp <= namepool->getGlobalWeight())
					{
						break;
					}
					tmp -= namepool->getGlobalWeight();
					++nat;
				}
				_nationality = nat;
			}
			if ((size_t)_nationality >= names.size())
			{
				// handling weird cases, e.g. corner cases in soldier transformations
				_nationality = RNG::generate(0, names.size() - 1);
			}
			_name = names.at(_nationality)->genName(&_gender, rules->getFemaleFrequency());
			_callsign = generateCallsign(rules->getNames());
			_look = (SoldierLook)names.at(_nationality)->genLook(4); // Once we add the ability to mod in extra looks, this will need to reference the ruleset for the maximum amount of looks.
		}
		else
		{
			// No possible names, just wing it
			_gender = (RNG::percent(rules->getFemaleFrequency()) ? GENDER_FEMALE : GENDER_MALE);
			_look = (SoldierLook)RNG::generate(0,3);
			_name = (_gender == GENDER_FEMALE) ? "Jane" : "John";
			_name += " Doe";
			_callsign = "";
		}
	}
	_lookVariant = RNG::seedless(0, RuleSoldier::LookVariantMax - 1);
}

/**
 *
 */
Soldier::~Soldier()
{
	for (auto* entry : _equipmentLayout)
	{
		delete entry;
	}
	Collections::deleteAll(_personalEquipmentLayout);
	delete _death;
	delete _diary;
}

/**
 * Loads the soldier from a YAML file.
 * @param node YAML node.
 * @param mod Game mod.
 * @param save Pointer to savegame.
 */
void Soldier::load(const YAML::Node& node, const Mod *mod, SavedGame *save, const ScriptGlobal *shared, bool soldierTemplate)
{
	if (!soldierTemplate)
	{
		_id = node["id"].as<int>(_id);
	}
	_name = node["name"].as<std::string>(_name);
	if (node["callsign"])
	{
		_callsign = node["callsign"].as<std::string>();
	}
	_nationality = node["nationality"].as<int>(_nationality);
	if (soldierTemplate)
	{
		UnitStats ii, cc;
		if (node["initialStats"])
			ii = node["initialStats"].as<UnitStats>(ii);
		if (node["currentStats"])
			cc = node["currentStats"].as<UnitStats>(cc);
		_initialStats = UnitStats::templateMerge(_initialStats, ii);
		_currentStats = UnitStats::templateMerge(_currentStats, cc);
	}
	else
	{
		_initialStats = node["initialStats"].as<UnitStats>(_initialStats);
		_currentStats = node["currentStats"].as<UnitStats>(_currentStats);
	}
	_dailyDogfightExperienceCache = node["dailyDogfightExperienceCache"].as<UnitStats>(_dailyDogfightExperienceCache);

	// re-roll mana stats when upgrading saves
	if (_currentStats.mana == 0 && _rules->getMaxStats().mana > 0)
	{
		int reroll = RNG::generate(_rules->getMinStats().mana, _rules->getMaxStats().mana);
		_currentStats.mana = reroll;
		_initialStats.mana = reroll;
	}

	_rank = (SoldierRank)node["rank"].as<int>(_rank);
	_gender = (SoldierGender)node["gender"].as<int>(_gender);
	_look = (SoldierLook)node["look"].as<int>(_look);
	_lookVariant = node["lookVariant"].as<int>(_lookVariant);
	_missions = node["missions"].as<int>(_missions);
	_kills = node["kills"].as<int>(_kills);
	_stuns = node["stuns"].as<int>(_stuns);
	_manaMissing = node["manaMissing"].as<int>(_manaMissing);
	_healthMissing = node["healthMissing"].as<int>(_healthMissing);
	_recovery = node["recovery"].as<float>(_recovery);
	Armor *armor = _armor;
	if (node["armor"])
	{
		armor = mod->getArmor(node["armor"].as<std::string>());
	}
	if (armor == 0)
	{
		armor = mod->getSoldier(mod->getSoldiersList().front())->getDefaultArmor();
	}
	_armor = armor;
	if (node["replacedArmor"])
		_replacedArmor = mod->getArmor(node["replacedArmor"].as<std::string>());
	if (node["transformedArmor"])
		_transformedArmor = mod->getArmor(node["transformedArmor"].as<std::string>());
	_psiTraining = node["psiTraining"].as<bool>(_psiTraining);
	_training = node["training"].as<bool>(_training);
	_returnToTrainingWhenHealed = node["returnToTrainingWhenHealed"].as<bool>(_returnToTrainingWhenHealed);

	_improvement = node["improvement"].as<int>(_improvement);
	_psiStrImprovement = node["psiStrImprovement"].as<int>(_psiStrImprovement);
	if (const YAML::Node &layout = node["equipmentLayout"])
	{
		for (YAML::const_iterator i = layout.begin(); i != layout.end(); ++i)
		{
			EquipmentLayoutItem *layoutItem = new EquipmentLayoutItem(*i);
			if (mod->getInventory(layoutItem->getSlot()))
			{
				_equipmentLayout.push_back(layoutItem);
			}
			else
			{
				delete layoutItem;
			}
		}
	}
	if (const YAML::Node &layout = node["personalEquipmentLayout"])
	{
		for (YAML::const_iterator i = layout.begin(); i != layout.end(); ++i)
		{
			EquipmentLayoutItem *layoutItem = new EquipmentLayoutItem(*i);
			if (mod->getInventory(layoutItem->getSlot()))
			{
				_personalEquipmentLayout.push_back(layoutItem);
			}
			else
			{
				delete layoutItem;
			}
		}
	}
	if (node["personalEquipmentArmor"])
	{
		_personalEquipmentArmor = mod->getArmor(node["personalEquipmentArmor"].as<std::string>());
	}
	if (node["death"])
	{
		_death = new SoldierDeath();
		_death->load(node["death"]);
	}
	if (node["diary"])
	{
		_diary = new SoldierDiary();
		_diary->load(node["diary"], mod);
	}
	calcStatString(mod->getStatStrings(), (Options::psiStrengthEval && save->isResearched(mod->getPsiRequirements())));
	_corpseRecovered = node["corpseRecovered"].as<bool>(_corpseRecovered);
	_previousTransformations = node["previousTransformations"].as<std::map<std::string, int > >(_previousTransformations);
	_transformationBonuses = node["transformationBonuses"].as<std::map<std::string, int > >(_transformationBonuses);
	_scriptValues.load(node, shared);
}

/**
 * Saves the soldier to a YAML file.
 * @return YAML node.
 */
YAML::Node Soldier::save(const ScriptGlobal *shared) const
{
	YAML::Node node;
	node["type"] = _rules->getType();
	node["id"] = _id;
	node["name"] = _name;
	if (!_callsign.empty())
	{
		node["callsign"] = _callsign;
	}
	node["nationality"] = _nationality;
	node["initialStats"] = _initialStats;
	node["currentStats"] = _currentStats;
	if (_dailyDogfightExperienceCache.firing > 0 || _dailyDogfightExperienceCache.reactions > 0 || _dailyDogfightExperienceCache.bravery > 0)
	{
		node["dailyDogfightExperienceCache"] = _dailyDogfightExperienceCache;
	}
	node["rank"] = (int)_rank;
	if (_craft != 0)
	{
		node["craft"] = _craft->saveId();
	}
	node["gender"] = (int)_gender;
	node["look"] = (int)_look;
	node["lookVariant"] = _lookVariant;
	node["missions"] = _missions;
	node["kills"] = _kills;
	node["stuns"] = _stuns;
	if (_manaMissing > 0)
		node["manaMissing"] = _manaMissing;
	if (_healthMissing > 0)
		node["healthMissing"] = _healthMissing;
	if (_recovery > 0.0f)
		node["recovery"] = _recovery;
	node["armor"] = _armor->getType();
	if (_replacedArmor != 0)
		node["replacedArmor"] = _replacedArmor->getType();
	if (_transformedArmor != 0)
		node["transformedArmor"] = _transformedArmor->getType();
	if (_psiTraining)
		node["psiTraining"] = _psiTraining;
	if (_training)
		node["training"] = _training;
	if (_returnToTrainingWhenHealed)
		node["returnToTrainingWhenHealed"] = _returnToTrainingWhenHealed;
	node["improvement"] = _improvement;
	node["psiStrImprovement"] = _psiStrImprovement;
	if (!_equipmentLayout.empty())
	{
		for (const auto* entry : _equipmentLayout)
			node["equipmentLayout"].push_back(entry->save());
	}
	if (!_personalEquipmentLayout.empty())
	{
		for (const auto* entry : _personalEquipmentLayout)
			node["personalEquipmentLayout"].push_back(entry->save());
	}
	if (_personalEquipmentArmor)
	{
		node["personalEquipmentArmor"] = _personalEquipmentArmor->getType();
	}
	if (_death != 0)
	{
		node["death"] = _death->save();
	}
	if (Options::soldierDiaries && (!_diary->getMissionIdList().empty() || !_diary->getSoldierCommendations()->empty() || _diary->getMonthsService() > 0))
	{
		node["diary"] = _diary->save();
	}
	if (_corpseRecovered)
		node["corpseRecovered"] = _corpseRecovered;
	if (!_previousTransformations.empty())
		node["previousTransformations"] = _previousTransformations;
	if (!_transformationBonuses.empty())
		node["transformationBonuses"] = _transformationBonuses;

	_scriptValues.save(node, shared);

	return node;
}

YAML::Node Soldier::asYaml() const
{
	YAML::Node node;
	node["type"] = _rules->getType();
	node["id"] = _id;
	node["name"] = _name;
	if (!_callsign.empty())
	{
		node["callsign"] = _callsign;
	}
	node["nationality"] = _nationality;
	node["initialStats"] = _initialStats;
	node["currentStats"] = _currentStats;
	if (_dailyDogfightExperienceCache.firing > 0 || _dailyDogfightExperienceCache.reactions > 0 || _dailyDogfightExperienceCache.bravery > 0)
	{
		node["dailyDogfightExperienceCache"] = _dailyDogfightExperienceCache;
	}
	node["rank"] = (int)_rank;
	if (_craft != 0)
	{
		node["craft"] = _craft->saveId();
	}
	node["gender"] = (int)_gender;
	node["look"] = (int)_look;
	node["lookVariant"] = _lookVariant;
	node["missions"] = _missions;
	node["kills"] = _kills;
	node["stuns"] = _stuns;
	if (_manaMissing > 0)
		node["manaMissing"] = _manaMissing;
	if (_healthMissing > 0)
		node["healthMissing"] = _healthMissing;
	if (_recovery > 0.0f)
		node["recovery"] = _recovery;
	node["armor"] = _armor->getType();
	if (_replacedArmor != 0)
		node["replacedArmor"] = _replacedArmor->getType();
	if (_transformedArmor != 0)
		node["transformedArmor"] = _transformedArmor->getType();
	if (_psiTraining)
		node["psiTraining"] = _psiTraining;
	if (_training)
		node["training"] = _training;
	if (_returnToTrainingWhenHealed)
		node["returnToTrainingWhenHealed"] = _returnToTrainingWhenHealed;
	node["improvement"] = _improvement;
	node["psiStrImprovement"] = _psiStrImprovement;
	if (!_equipmentLayout.empty())
	{
		for (const auto* entry : _equipmentLayout)
			node["equipmentLayout"].push_back(entry->save());
	}
	if (!_personalEquipmentLayout.empty())
	{
		for (const auto* entry : _personalEquipmentLayout)
			node["personalEquipmentLayout"].push_back(entry->save());
	}
	if (_personalEquipmentArmor)
	{
		node["personalEquipmentArmor"] = _personalEquipmentArmor->getType();
	}
	if (_death != 0)
	{
		node["death"] = _death->save();
	}
	if (Options::soldierDiaries && (!_diary->getMissionIdList().empty() || !_diary->getSoldierCommendations()->empty() || _diary->getMonthsService() > 0))
	{
		node["diary"] = _diary->save();
	}
	if (_corpseRecovered)
		node["corpseRecovered"] = _corpseRecovered;
	if (!_previousTransformations.empty())
		node["previousTransformations"] = _previousTransformations;
	if (!_transformationBonuses.empty())
		node["transformationBonuses"] = _transformationBonuses;

	return node;
}

/**
 * Returns the soldier's full name (and, optionally, statString).
 * @param statstring Add stat string?
 * @param maxLength Restrict length to a certain value.
 * @return Soldier name.
 */
std::string Soldier::getName(bool statstring, unsigned int maxLength) const
{
	if (statstring && !_statString.empty())
	{
		auto nameCodePointLength = Unicode::codePointLengthUTF8(_name);
		auto statCodePointLength = Unicode::codePointLengthUTF8(_statString);
		if (nameCodePointLength + statCodePointLength > maxLength)
		{
			return Unicode::codePointSubstrUTF8(_name, 0, maxLength - statCodePointLength) + "/" + _statString;
		}
		else
		{
			return _name + "/" + _statString;
		}
	}
	else
	{
		return _name;
	}
}

/**
 * Changes the soldier's full name.
 * @param name Soldier name.
 */
void Soldier::setName(const std::string &name)
{
	_name = name;
}

/**
 * Generates a new name based on nationality.
 */
void Soldier::genName()
{
	const std::vector<SoldierNamePool*>& names = _rules->getNames();
	if (!names.empty())
	{
		// clamp (and randomize) nationality if needed (i.e. if the modder messed up)
		if ((size_t)_nationality >= names.size())
		{
			_nationality = RNG::generate(0, names.size() - 1);
		}
		_name = names.at(_nationality)->genName(&_gender, _rules->getFemaleFrequency());
		_callsign = generateCallsign(_rules->getNames());
		_look = (SoldierLook)names.at(_nationality)->genLook(4); // Once we add the ability to mod in extra looks, this will need to reference the ruleset for the maximum amount of looks.
	}
	else
	{
		_nationality = 0;
	}
}

/**
 * Returns the soldier's callsign.
 * @param maxLength Restrict length to a certain value.
 * @return Soldier callsign.
 */
std::string Soldier::getCallsign(unsigned int maxLength) const
{
	std::ostringstream ss;
	ss << "\"";
	ss << Unicode::codePointSubstrUTF8(_callsign, 0, maxLength);
	ss << "\"";

	return ss.str();
}

/**
 * Changes the soldier's callsign.
 * @param callsign Soldier callsign.
 */
void Soldier::setCallsign(const std::string &callsign)
{
	_callsign = callsign;
}

/**
 * Check whether the soldier has a callsign assigned.
 * @return true, if the soldier has a callsign, false, if he has not.
 */
bool Soldier::hasCallsign() const
{
	return !_callsign.empty();
}

/**
 * Generate a random callsign from the pool of names. Tries to fallback to the first entry in
 * in the namepool list if no callsigns for the given nationality are defined.
 * @return generated callsign.
 */
std::string Soldier::generateCallsign(const std::vector<SoldierNamePool*> &names)
{
	std::string callsign = names.at(_nationality)->genCallsign(_gender);
	if (callsign.empty())
	{
		callsign = names.at(0)->genCallsign(_gender);
	}
	return callsign;
}

/**
* Returns the soldier's nationality.
* @return Nationality ID.
*/
int Soldier::getNationality() const
{
	return _nationality;
}

/**
* Changes the soldier's nationality.
* @param nationality Nationality ID.
*/
void Soldier::setNationality(int nationality)
{
	_nationality = nationality;
}

/**
 * Returns the craft the soldier is assigned to.
 * @return Pointer to craft.
 */
Craft *Soldier::getCraft() const
{
	return _craft;
}

/**
 * Automatically move equipment between the craft and the base when assigning/deassigning/reassigning soldiers.
 */
void Soldier::autoMoveEquipment(Craft* craft, Base* base, int toBase)
{
	auto* inTheBase = base->getStorageItems();
	auto* onTheCraft = _craft->getItems();
	auto* reservedForTheCraft = _craft->getSoldierItems();

	// Disclaimer: no checks for items not allowed on crafts; no checks for any craft limits (item number or weight). I'm not willing to spend the next 5+ years fixing it!
	for (auto* invItem : _equipmentLayout)
	{
		// ignore fixed weapons...
		if (!invItem->isFixed())
		{
			const std::string& invItemMain = invItem->getItemType();
			if (toBase > 0)
			{
				if (onTheCraft->getItem(invItemMain) > 0)
				{
					inTheBase->addItem(invItemMain, 1);
					onTheCraft->removeItem(invItemMain, 1);
				}
				reservedForTheCraft->removeItem(invItemMain, 1);
			}
			else if (toBase < 0)
			{
				if (inTheBase->getItem(invItemMain) > 0)
				{
					inTheBase->removeItem(invItemMain, 1);
					onTheCraft->addItem(invItemMain, 1);
				}
				reservedForTheCraft->addItem(invItemMain, 1);
			}
		}
		// ...but not their ammo
		for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
		{
			const std::string& invItemAmmo = invItem->getAmmoItemForSlot(slot);
			if (invItemAmmo != "NONE")
			{
				if (toBase > 0)
				{
					if (onTheCraft->getItem(invItemAmmo) > 0)
					{
						inTheBase->addItem(invItemAmmo, 1);
						onTheCraft->removeItem(invItemAmmo, 1);
					}
					reservedForTheCraft->removeItem(invItemAmmo, 1);
				}
				else if (toBase < 0)
				{
					if (inTheBase->getItem(invItemAmmo) > 0)
					{
						inTheBase->removeItem(invItemAmmo, 1);
						onTheCraft->addItem(invItemAmmo, 1);
					}
					reservedForTheCraft->addItem(invItemAmmo, 1);
				}
			}
		}
	}
}

/**
 * Assigns the soldier to a new craft.
 * @param craft Pointer to craft.
 */
void Soldier::setCraft(Craft *craft, bool resetCustomDeployment)
{
	_craft = craft;

	if (resetCustomDeployment && _craft)
	{
		// adding a soldier into a craft invalidates a custom craft deployment
		_craft->resetCustomDeployment();
	}
}
/**
 * Assigns the soldier to a new craft and automatically moves the equipment (if enabled).
 */
void Soldier::setCraftAndMoveEquipment(Craft* craft, Base* base, bool isNewBattle, bool resetCustomDeployment)
{
	bool notTheSameCraft = (_craft != craft);

	if (Options::oxceAlternateCraftEquipmentManagement && !isNewBattle && notTheSameCraft && base)
	{
		if (_craft)
		{
			autoMoveEquipment(_craft, base, 1); // move from old craft to base
		}
	}

	setCraft(craft, resetCustomDeployment);

	if (Options::oxceAlternateCraftEquipmentManagement && !isNewBattle && notTheSameCraft && base)
	{
		if (craft)
		{
			autoMoveEquipment(craft, base, -1); // move from base to new craft
		}
	}
}

/**
 * Returns the soldier's craft string, which
 * is either the soldier's wounded status,
 * the assigned craft name, or none.
 * @param lang Language to get strings from.
 * @return Full name.
 */
std::string Soldier::getCraftString(Language *lang, const BaseSumDailyRecovery& recovery) const
{
	std::string s;
	if (_death)
	{
		if (_death->getCause())
		{
			s = lang->getString("STR_KILLED_IN_ACTION", _gender);
		}
		else
		{
			s = lang->getString("STR_MISSING_IN_ACTION", _gender);
		}
	}
	else if (isWounded())
	{
		std::ostringstream ss;
		ss << lang->getString("STR_WOUNDED");
		ss << ">";
		int days = getNeededRecoveryTime(recovery);
		if (days < 0)
		{
			ss << "∞";
		}
		else
		{
			ss << days;
		}
		s = ss.str();
	}
	else if (_craft == 0)
	{
		s = lang->getString("STR_NONE_UC");
	}
	else
	{
		s = _craft->getName(lang);
	}
	return s;
}

/**
 * Returns a localizable-string representation of
 * the soldier's military rank.
 * @return String ID for rank.
 */
std::string Soldier::getRankString() const
{
	const std::vector<std::string> &rankStrings = _rules->getRankStrings();
	if (!_rules->getAllowPromotion())
	{
		// even if promotion is not allowed, we allow to use a different "Rookie" translation per soldier type
		if (rankStrings.empty())
		{
			return "STR_RANK_NONE";
		}
	}

	switch (_rank)
	{
	case RANK_ROOKIE:
		if (rankStrings.size() > 0)
			return rankStrings.at(0);
		return "STR_ROOKIE";
	case RANK_SQUADDIE:
		if (rankStrings.size() > 1)
			return rankStrings.at(1);
		return "STR_SQUADDIE";
	case RANK_SERGEANT:
		if (rankStrings.size() > 2)
			return rankStrings.at(2);
		return "STR_SERGEANT";
	case RANK_CAPTAIN:
		if (rankStrings.size() > 3)
			return rankStrings.at(3);
		return "STR_CAPTAIN";
	case RANK_COLONEL:
		if (rankStrings.size() > 4)
			return rankStrings.at(4);
		return "STR_COLONEL";
	case RANK_COMMANDER:
		if (rankStrings.size() > 5)
			return rankStrings.at(5);
		return "STR_COMMANDER";
	default:
		return "";
	}
}

/**
 * Returns a graphic representation of
 * the soldier's military rank from BASEBITS.PCK.
 * @note THE MEANING OF LIFE (is now obscured as a default)
 * @return Sprite ID for rank.
 */
int Soldier::getRankSprite() const
{
	return _rules->getRankSprite() + _rank;
}

/**
 * Returns a graphic representation of
 * the soldier's military rank from SMOKE.PCK
 * @return Sprite ID for rank.
 */
int Soldier::getRankSpriteBattlescape() const
{
	return _rules->getRankSpriteBattlescape() + _rank;
}

/**
 * Returns a graphic representation of
 * the soldier's military rank from TinyRanks
 * @return Sprite ID for rank.
 */
int Soldier::getRankSpriteTiny() const
{
	return _rules->getRankSpriteTiny() + _rank;
}

/**
 * Returns the soldier's military rank.
 * @return Rank enum.
 */
SoldierRank Soldier::getRank() const
{
	return _rank;
}

/**
 * Increase the soldier's military rank.
 */
void Soldier::promoteRank()
{
	if (!_rules->getAllowPromotion())
		return;

	const std::vector<std::string> &rankStrings = _rules->getRankStrings();
	if (!rankStrings.empty())
	{
		// stop if the soldier already has the maximum possible rank for his soldier type
		if ((size_t)_rank >= rankStrings.size() - 1)
		{
			return;
		}
	}

	_rank = (SoldierRank)((int)_rank + 1);
	if (_rank > RANK_SQUADDIE)
	{
		// only promotions above SQUADDIE are worth to be mentioned
		_recentlyPromoted = true;
	}
}

/**
 * Promotes/demotes a soldier to a specific rank.
 */
void Soldier::setRank(const SoldierRank newRank)
{
	if (!_rules->getAllowPromotion())
		return;

	const std::vector<std::string> &rankStrings = _rules->getRankStrings();
	if (!rankStrings.empty())
	{
		// abort if the desired rank is not indexed in the rank strings
		if ((size_t)newRank >= rankStrings.size())
		{
			return;
		}
	}

	_rank = newRank;

	// Note: we don't need to show a notification for this style of promotion
}

/**
 * Returns the soldier's amount of missions.
 * @return Missions.
 */
int Soldier::getMissions() const
{
	return _missions;
}

/**
 * Returns the soldier's amount of kills.
 * @return Kills.
 */
int Soldier::getKills() const
{
	return _kills;
}

/**
 * Returns the soldier's amount of stuns.
 * @return Stuns.
 */
int Soldier::getStuns() const
{
	return _stuns;
}

/**
 * Returns the soldier's gender.
 * @return Gender.
 */
SoldierGender Soldier::getGender() const
{
	return _gender;
}

/**
 * Changes the soldier's gender (1/3 of avatar).
 * @param gender Gender.
 */
void Soldier::setGender(SoldierGender gender)
{
	_gender = gender;
}

/**
 * Returns the soldier's look.
 * @return Look.
 */
SoldierLook Soldier::getLook() const
{
	return _look;
}

/**
 * Changes the soldier's look (2/3 of avatar).
 * @param look Look.
 */
void Soldier::setLook(SoldierLook look)
{
	_look = look;
}

/**
 * Returns the soldier's look sub type.
 * @return Look.
 */
int Soldier::getLookVariant() const
{
	return _lookVariant;
}

/**
 * Changes the soldier's look variant (3/3 of avatar).
 * @param lookVariant Look sub type.
 */
void Soldier::setLookVariant(int lookVariant)
{
	_lookVariant = lookVariant;
}

/**
 * Returns the soldier's rules.
 * @return rule soldier
 */
RuleSoldier *Soldier::getRules() const
{
	return _rules;
}

/**
 * Returns the soldier's unique ID. Each soldier
 * can be identified by its ID. (not it's name)
 * @return Unique ID.
 */
int Soldier::getId() const
{
	return _id;
}

/**
 * Add a mission to the counter.
 */
void Soldier::addMissionCount()
{
	_missions++;
}

/**
 * Add a kill to the counter.
 */
void Soldier::addKillCount(int count)
{
	_kills += count;
}

/**
 * Add a stun to the counter.
 */
void Soldier::addStunCount(int count)
{
	_stuns += count;
}

/**
 * Get pointer to initial stats.
 */
const UnitStats* Soldier::getInitStats() const
{
	return &_initialStats;
}

/**
 * Get pointer to current stats.
 */
UnitStats *Soldier::getCurrentStatsEditable()
{
	return &_currentStats;
}
const UnitStats* Soldier::getCurrentStats() const
{
	return &_currentStats;
}

void Soldier::setBothStats(UnitStats *stats)
{
	_currentStats = *stats;
	_initialStats = *stats;
}

/**
 * Returns the unit's promotion status and resets it.
 * @return True if recently promoted, False otherwise.
 */
bool Soldier::isPromoted()
{
	bool promoted = _recentlyPromoted;
	_recentlyPromoted = false;
	return promoted;
}

/**
 * Returns the unit's current armor.
 * @return Pointer to armor data.
 */
Armor *Soldier::getArmor() const
{
	return _armor;
}

/**
 * Changes the unit's current armor.
 * @param armor Pointer to armor data.
 */
void Soldier::setArmor(Armor *armor, bool resetCustomDeployment)
{
	if (resetCustomDeployment && _craft && _armor && armor && _armor->getSize() < armor->getSize())
	{
		// increasing the size of a soldier's armor invalidates a custom craft deployment
		_craft->resetCustomDeployment();
	}

	_armor = armor;
}

/**
 * Returns a list of armor layers (sprite names).
 */
const std::vector<std::string>& Soldier::getArmorLayers(Armor *customArmor) const
{
	std::stringstream ss;

	const Armor *armor = customArmor ? customArmor : _armor;

	const std::string gender = _gender == GENDER_MALE ? "M" : "F";
	const auto& layoutDefinition = armor->getLayersDefinition();

	// find relevant layer
	for (int i = 0; i <= RuleSoldier::LookVariantBits; ++i)
	{
		ss.str("");
		ss << gender;
		ss << (int)_look + (_lookVariant & (RuleSoldier::LookVariantMask >> i)) * 4;
		auto it = layoutDefinition.find(ss.str());
		if (it != layoutDefinition.end())
		{
			return it->second;
		}
	}

	{
		// try also gender + hardcoded look 0
		ss.str("");
		ss << gender << "0";
		auto it = layoutDefinition.find(ss.str());
		if (it != layoutDefinition.end())
		{
			return it->second;
		}
	}

	throw Exception("Layered armor sprite definition (" + armor->getType() + ") not found!");
}

/**
* Gets the soldier's original armor (before replacement).
* @return Pointer to armor data.
*/
Armor *Soldier::getReplacedArmor() const
{
	return _replacedArmor;
}

/**
* Backs up the soldier's original armor (before replacement).
* @param armor Pointer to armor data.
*/
void Soldier::setReplacedArmor(Armor *armor)
{
	_replacedArmor = armor;
}

/**
* Gets the soldier's original armor (before transformation).
* @return Pointer to armor data.
*/
Armor *Soldier::getTransformedArmor() const
{
	return _transformedArmor;
}

/**
* Backs up the soldier's original armor (before transformation).
* @param armor Pointer to armor data.
*/
void Soldier::setTransformedArmor(Armor *armor)
{
	_transformedArmor = armor;
}

namespace
{

/**
 * Calculates absolute threshold based on percentage threshold.
 * @param base base value
 * @param threshold threshold in percent
 * @return threshold in absolute value
 */
int valueThreshold(int base, int threshold)
{
	return base * threshold / 100;
}

/**
 * Calculates the amount that exceeds the threshold of the base value.
 * @param value value to check
 * @param base base value
 * @param threshold threshold of the base value in percent
 * @return absolute value over the threshold
 */
int valueOverThreshold(int value, int base, int threshold)
{
	return std::max(0, value - valueThreshold(base, threshold));
}

/**
 * Calculates how long will it take to recover.
 * @param current Total amount of days.
 * @param recovery Recovery per day.
 * @return How many days will it take to recover, can return -1 meaning infinity.
 */
int recoveryTime(int current, int recovery)
{
	if (current <= 0)
	{
		return 0;
	}

	if (recovery <= 0)
		return -1; // represents infinity

	int days = current / recovery;
	if (current % recovery > 0)
		++days;

	return days;
}


}


/**
* Is the soldier wounded or not?.
* @return True if wounded.
*/
bool Soldier::isWounded() const
{
	if (_manaMissing > 0)
	{
		if (valueOverThreshold(_manaMissing, _currentStats.mana, _rules->getManaWoundThreshold()))
		{
			return true;
		}
	}
	if (_healthMissing > 0)
	{
		if (valueOverThreshold(_healthMissing, _currentStats.health, _rules->getHealthWoundThreshold()))
		{
			return true;
		}
	}
	return _recovery > 0.0f;
}

/**
* Is the soldier wounded or not?.
* @return False if wounded.
*/
bool Soldier::hasFullHealth() const
{
	return !isWounded();
}

/**
 * Is the soldier capable of defending a base?.
 * @return False if wounded too much.
 */
bool Soldier::canDefendBase() const
{
	int currentHealthPercentage = std::max(0, _currentStats.health - getWoundRecoveryInt() - getHealthMissing()) * 100 / _currentStats.health;
	return currentHealthPercentage >= Options::oxceWoundedDefendBaseIf;
}


/**
 * Returns the amount of missing mana.
 * @return Missing mana.
 */
int Soldier::getManaMissing() const
{
	return _manaMissing;
}

/**
 * Sets the amount of missing mana.
 * @param manaMissing Missing mana.
 */
void Soldier::setManaMissing(int manaMissing)
{
	_manaMissing = std::max(manaMissing, 0);
}

/**
 * Returns the amount of time until the soldier's mana is fully replenished.
 * @return Number of days. -1 represents infinity.
 */
int Soldier::getManaRecovery(int manaRecoveryPerDay) const
{
	return recoveryTime(_manaMissing, manaRecoveryPerDay);
}


/**
 * Returns the amount of missing health.
 * @return Missing health.
 */
int Soldier::getHealthMissing() const
{
	return _healthMissing;
}

/**
 * Sets the amount of missing health.
 * @param healthMissing Missing health.
 */
void Soldier::setHealthMissing(int healthMissing)
{
	_healthMissing = std::max(healthMissing, 0);
}

/**
 * Returns the amount of time until the soldier's health is fully replenished.
 * @return Number of days. -1 represents infinity.
 */
int Soldier::getHealthRecovery(int healthRecoveryPerDay) const
{
	return recoveryTime(_healthMissing, healthRecoveryPerDay);
}


/**
 * Returns the amount of time until the soldier is healed.
 * @return Number of days.
 */
int Soldier::getWoundRecoveryInt() const
{
	// Note: only for use in Yankes scripts! ...and in base defense HP calculations :/
	return (int)(std::ceil(_recovery));
}

int Soldier::getWoundRecovery(float absBonus, float relBonus) const
{
	float hpPerDay = 1.0f + absBonus + (relBonus * _currentStats.health * 0.01f);
	return (int)(std::ceil(_recovery / hpPerDay));
}

/**
 * Changes the amount of time until the soldier is healed.
 * @param recovery Number of days.
 */
void Soldier::setWoundRecovery(int recovery)
{
	_recovery = std::max(recovery, 0);
}


/**
 * Heals soldier wounds.
 */
void Soldier::healWound(float absBonus, float relBonus)
{
	// 1 hp per day as minimum
	_recovery -= 1.0f;

	// absolute bonus from sick bay facilities
	_recovery -= absBonus;

	// relative bonus from sick bay facilities
	_recovery -= (relBonus * _currentStats.health * 0.01f);

	if (_recovery < 0.0f)
		_recovery = 0.0f;
}

/**
 * Replenishes the soldier's mana.
 */
void Soldier::replenishMana(int manaRecoveryPerDay)
{
	_manaMissing -= manaRecoveryPerDay;

	if (_manaMissing < 0)
		_manaMissing = 0;

	// maximum amount of mana missing can be up to 2x the current mana pool (WITHOUT armor and bonuses!); at least 100
	int maxThreshold = std::max(100, _currentStats.mana * 2);
	if (_manaMissing > maxThreshold)
		_manaMissing = maxThreshold;
}

/**
 * Replenishes the soldier's health.
 */
void Soldier::replenishHealth(int healthRecoveryPerDay)
{
	_healthMissing -= healthRecoveryPerDay;

	if (_healthMissing < 0)
		_healthMissing = 0;
}

/**
 * Daily stat replenish and healing of the soldier based on the facilities available in the base.
 * @param recovery Recovery values provided by the base.
 */
void Soldier::replenishStats(const BaseSumDailyRecovery& recovery)
{
	if (_recovery > 0.0f)
	{
		healWound(recovery.SickBayAbsoluteBonus, recovery.SickBayRelativeBonus);
	}
	else
	{
		if (getManaMissing() > 0 && recovery.ManaRecovery > 0)
		{
			// positive mana recovery only when NOT wounded
			replenishMana(recovery.ManaRecovery);
		}

		if (getHealthMissing() > 0 && recovery.HealthRecovery > 0)
		{
			// health recovery only when NOT wounded
			replenishHealth(recovery.HealthRecovery);
		}
	}

	if (recovery.ManaRecovery  < 0)
	{
		// negative mana recovery always
		replenishMana(recovery.ManaRecovery);
	}
}

/**
 * Gets number of days until the soldier is ready for action again.
 * @return Number of days. -1 represents infinity.
 */
int Soldier::getNeededRecoveryTime(const BaseSumDailyRecovery& recovery) const
{
	int time = getWoundRecovery(recovery.SickBayAbsoluteBonus, recovery.SickBayRelativeBonus);

	int bonusTime = 0;
	if (_healthMissing > 0)
	{
		int t = recoveryTime(
			valueOverThreshold(_healthMissing, _currentStats.health, _rules->getHealthWoundThreshold()),
			recovery.HealthRecovery
		);

		if (t < 0)
		{
			return t;
		}

		bonusTime = std::max(bonusTime, t);
	}
	if (_manaMissing > 0)
	{
		int t = recoveryTime(
			valueOverThreshold(_manaMissing, _currentStats.mana, _rules->getManaWoundThreshold()),
			recovery.ManaRecovery
		);

		if (t < 0)
		{
			return t;
		}

		bonusTime = std::max(bonusTime, t);
	}

	return time + bonusTime;
}



/**
 * Returns the list of EquipmentLayoutItems of a soldier.
 * @return Pointer to the EquipmentLayoutItem list.
 */
std::vector<EquipmentLayoutItem*> *Soldier::getEquipmentLayout()
{
	return &_equipmentLayout;
}

/**
 * Trains a soldier's Psychic abilities after 1 month.
 */
void Soldier::trainPsi()
{
	UnitStats::Type psiSkillCap = _rules->getStatCaps().psiSkill;
	UnitStats::Type psiStrengthCap = _rules->getStatCaps().psiStrength;

	_improvement = _psiStrImprovement = 0;
	// -10 days - tolerance threshold for switch from anytimePsiTraining option.
	// If soldier has psi skill -10..-1, he was trained 20..59 days. 81.7% probability, he was trained more that 30 days.
	if (_currentStats.psiSkill < -10 + _rules->getMinStats().psiSkill)
		_currentStats.psiSkill = _rules->getMinStats().psiSkill;
	else if (_currentStats.psiSkill <= _rules->getMaxStats().psiSkill)
	{
		int max = _rules->getMaxStats().psiSkill + _rules->getMaxStats().psiSkill / 2;
		_improvement = RNG::generate(_rules->getMaxStats().psiSkill, max);
	}
	else
	{
		if (_currentStats.psiSkill <= (psiSkillCap / 2)) _improvement = RNG::generate(5, 12);
		else if (_currentStats.psiSkill < psiSkillCap) _improvement = RNG::generate(1, 3);

		if (Options::allowPsiStrengthImprovement)
		{
			if (_currentStats.psiStrength <= (psiStrengthCap / 2)) _psiStrImprovement = RNG::generate(5, 12);
			else if (_currentStats.psiStrength < psiStrengthCap) _psiStrImprovement = RNG::generate(1, 3);
		}
	}
	_currentStats.psiSkill = std::max(_currentStats.psiSkill, std::min<UnitStats::Type>(_currentStats.psiSkill+_improvement, psiSkillCap));
	_currentStats.psiStrength = std::max(_currentStats.psiStrength, std::min<UnitStats::Type>(_currentStats.psiStrength+_psiStrImprovement, psiStrengthCap));
}

/**
 * Trains a soldier's Psychic abilities after 1 day.
 * (anytimePsiTraining option)
 */
void Soldier::trainPsi1Day()
{
	if (!_psiTraining)
	{
		_improvement = 0;
		return;
	}

	if (_currentStats.psiSkill > 0) // yes, 0. _rules->getMinStats().psiSkill was wrong.
	{
		if (8 * 100 >= _currentStats.psiSkill * RNG::generate(1, 100) && _currentStats.psiSkill < _rules->getStatCaps().psiSkill)
		{
			++_improvement;
			++_currentStats.psiSkill;
		}

		if (Options::allowPsiStrengthImprovement)
		{
			if (8 * 100 >= _currentStats.psiStrength * RNG::generate(1, 100) && _currentStats.psiStrength < _rules->getStatCaps().psiStrength)
			{
				++_psiStrImprovement;
				++_currentStats.psiStrength;
			}
		}
	}
	else if (_currentStats.psiSkill < _rules->getMinStats().psiSkill)
	{
		if (++_currentStats.psiSkill == _rules->getMinStats().psiSkill)	// initial training is over
		{
			_improvement = _rules->getMaxStats().psiSkill + RNG::generate(0, _rules->getMaxStats().psiSkill / 2);
			_currentStats.psiSkill = _improvement;
		}
	}
	else // minStats.psiSkill <= 0 && _currentStats.psiSkill == minStats.psiSkill
		_currentStats.psiSkill -= RNG::generate(30, 60);	// set initial training from 30 to 60 days
}

/**
 * Is the soldier already fully psi-trained?
 * @return True, if the soldier cannot gain any more stats in the psi-training facility.
 */
bool Soldier::isFullyPsiTrained()
{
	if (_currentStats.psiSkill >= _rules->getStatCaps().psiSkill)
	{
		if (Options::allowPsiStrengthImprovement)
		{
			if (_currentStats.psiStrength >= _rules->getStatCaps().psiStrength)
			{
				return true;
			}
		}
		else
		{
			return true;
		}
	}
	return false;
}

/**
 * returns whether or not the unit is in psi training
 * @return true/false
 */
bool Soldier::isInPsiTraining() const
{
	return _psiTraining;
}

/**
 * changes whether or not the unit is in psi training
 */
void Soldier::setPsiTraining(bool psi)
{
	_psiTraining = psi;
}

/**
 * returns this soldier's psionic skill improvement score for this month.
 * @return score
 */
int Soldier::getImprovement() const
{
	return _improvement;
}

/**
 * returns this soldier's psionic strength improvement score for this month.
 */
int Soldier::getPsiStrImprovement() const
{
	return _psiStrImprovement;
}

/**
 * Returns the soldier's death details.
 * @return Pointer to death data. NULL if no death has occurred.
 */
SoldierDeath *Soldier::getDeath() const
{
	return _death;
}

/**
 * Kills the soldier in the Geoscape.
 * @param death Pointer to death data.
 */
void Soldier::die(SoldierDeath *death)
{
	delete _death;
	_death = death;

	// Clean up associations
	_craft = 0;
	_psiTraining = false;
	_training = false;
	_returnToTrainingWhenHealed = false;
	_recentlyPromoted = false;
	_manaMissing = 0;
	_healthMissing = 0;
	_recovery = 0.0f;
	clearEquipmentLayout();
	Collections::deleteAll(_personalEquipmentLayout);
}

/**
 * Clears the equipment layout.
 */
void Soldier::clearEquipmentLayout()
{
	for (auto* entry : _equipmentLayout)
	{
		delete entry;
	}
	_equipmentLayout.clear();
}

/**
 * Returns the soldier's diary.
 * @return Diary.
 */
SoldierDiary *Soldier::getDiary()
{
	return _diary;
}
const SoldierDiary* Soldier::getDiary() const
{
	return _diary;
}

/**
* Resets the soldier's diary.
*/
void Soldier::resetDiary()
{
	delete _diary;
	_diary = new SoldierDiary();
}

/**
 * Calculates the soldier's statString
 * Calculates the soldier's statString.
 * @param statStrings List of statString rules.
 * @param psiStrengthEval Are psi stats available?
 */
void Soldier::calcStatString(const std::vector<StatString *> &statStrings, bool psiStrengthEval)
{
	if (_rules->getStatStrings().empty())
	{
		_statString = StatString::calcStatString(_currentStats, statStrings, psiStrengthEval, _psiTraining);
	}
	else
	{
		_statString = StatString::calcStatString(_currentStats, _rules->getStatStrings(), psiStrengthEval, _psiTraining);
	}
}

/**
 * Trains a soldier's Physical abilities
 */
void Soldier::trainPhys(int customTrainingFactor)
{
	UnitStats caps1 = _rules->getStatCaps();
	UnitStats caps2 = _rules->getTrainingStatCaps();
	// no P.T. for the wounded
	if (hasFullHealth())
	{
		if(_currentStats.firing < caps1.firing && RNG::generate(0, caps2.firing) > _currentStats.firing && RNG::percent(customTrainingFactor))
			_currentStats.firing++;
		if(_currentStats.health < caps1.health && RNG::generate(0, caps2.health) > _currentStats.health && RNG::percent(customTrainingFactor))
			_currentStats.health++;
		if(_currentStats.melee < caps1.melee && RNG::generate(0, caps2.melee) > _currentStats.melee && RNG::percent(customTrainingFactor))
			_currentStats.melee++;
		if(_currentStats.throwing < caps1.throwing && RNG::generate(0, caps2.throwing) > _currentStats.throwing && RNG::percent(customTrainingFactor))
			_currentStats.throwing++;
		if(_currentStats.strength < caps1.strength && RNG::generate(0, caps2.strength) > _currentStats.strength && RNG::percent(customTrainingFactor))
			_currentStats.strength++;
		if(_currentStats.tu < caps1.tu && RNG::generate(0, caps2.tu) > _currentStats.tu && RNG::percent(customTrainingFactor))
			_currentStats.tu++;
		if(_currentStats.stamina < caps1.stamina && RNG::generate(0, caps2.stamina) > _currentStats.stamina && RNG::percent(customTrainingFactor))
			_currentStats.stamina++;
	}
}

/**
 * Is the soldier already fully trained?
 * @return True, if the soldier cannot gain any more stats in the training facility.
 */
bool Soldier::isFullyTrained() const
{
	UnitStats trainingCaps = _rules->getTrainingStatCaps();

	if (_currentStats.firing < trainingCaps.firing
		|| _currentStats.health < trainingCaps.health
		|| _currentStats.melee < trainingCaps.melee
		|| _currentStats.throwing < trainingCaps.throwing
		|| _currentStats.strength < trainingCaps.strength
		|| _currentStats.tu < trainingCaps.tu
		|| _currentStats.stamina < trainingCaps.stamina)
	{
		return false;
	}
	return true;
}

/**
 * returns whether or not the unit is in physical training
 */
bool Soldier::isInTraining() const
{
	return _training;
}

/**
 * changes whether or not the unit is in physical training
 */
void Soldier::setTraining(bool training)
{
	_training = training;
}

/**
 * Should the soldier return to martial training automatically when fully healed?
 */
bool Soldier::getReturnToTrainingWhenHealed() const
{
	return _returnToTrainingWhenHealed;
}

/**
 * Sets whether the soldier should return to martial training automatically when fully healed.
 */
void Soldier::setReturnToTrainingWhenHealed(bool returnToTrainingWhenHealed)
{
	_returnToTrainingWhenHealed = returnToTrainingWhenHealed;
}

/**
 * Sets whether or not the unit's corpse was recovered from a battle
 */
void Soldier::setCorpseRecovered(bool corpseRecovered)
{
	_corpseRecovered = corpseRecovered;
}

/**
 * Gets the previous transformations performed on this soldier
 * @return The map of previous transformations
 */
std::map<std::string, int> &Soldier::getPreviousTransformations()
{
	return _previousTransformations;
}

/**
 * Checks whether or not the soldier is eligible for a certain transformation
 */
bool Soldier::isEligibleForTransformation(const RuleSoldierTransformation *transformationRule) const
{
	// rank check
	if ((int)_rank < transformationRule->getMinRank())
		return false;

	// alive and well
	if (!_death && !isWounded() && !transformationRule->isAllowingAliveSoldiers())
		return false;

	// alive and wounded
	if (!_death && isWounded() && !transformationRule->isAllowingWoundedSoldiers())
		return false;

	// dead
	if (_death && !transformationRule->isAllowingDeadSoldiers())
		return false;

	// dead and vaporized, or missing in action
	if (_death && !_corpseRecovered && transformationRule->needsCorpseRecovered())
		return false;

	// Is the soldier of the correct type?
	const auto& allowedTypes = transformationRule->getAllowedSoldierTypes();
	auto it = std::find(allowedTypes.begin(), allowedTypes.end(), _rules->getType());
	if (it == allowedTypes.end())
		return false;

	// Does this soldier's transformation history preclude this new project?
	for (const auto& reqd_trans : transformationRule->getRequiredPreviousTransformations())
	{
		if (_previousTransformations.find(reqd_trans) == _previousTransformations.end())
			return false;
	}

	for (const auto& forb_trans : transformationRule->getForbiddenPreviousTransformations())
	{
		if (_previousTransformations.find(forb_trans) != _previousTransformations.end())
			return false;
	}

	// Does this soldier meet the minimum stat requirements for the project?
	UnitStats currentStats = transformationRule->getIncludeBonusesForMinStats() ? _tmpStatsWithSoldierBonuses: _currentStats;
	UnitStats minStats = transformationRule->getRequiredMinStats();
	if (currentStats.tu < minStats.tu ||
		currentStats.stamina < minStats.stamina ||
		currentStats.health < minStats.health ||
		currentStats.bravery < minStats.bravery ||
		currentStats.reactions < minStats.reactions ||
		currentStats.firing < minStats.firing ||
		currentStats.throwing < minStats.throwing ||
		currentStats.melee < minStats.melee ||
		currentStats.mana < minStats.mana ||
		currentStats.strength < minStats.strength ||
		currentStats.psiStrength < minStats.psiStrength ||
		(currentStats.psiSkill < minStats.psiSkill && minStats.psiSkill != 0)) // The != 0 is required for the "psi training at any time" option, as it sets skill to negative in training
		return false;

	// Does the soldier have the required commendations?
	for (const auto& reqd_comm : transformationRule->getRequiredCommendations())
	{
		bool found = false;
		for (const auto* comm : *_diary->getSoldierCommendations())
		{
			if (comm->getDecorationLevelInt() >= reqd_comm.second && comm->getType() == reqd_comm.first)
			{
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}

	return true;
}

/**
 * Performs a transformation on this unit
 */
void Soldier::transform(const Mod *mod, RuleSoldierTransformation *transformationRule, Soldier *sourceSoldier, Base *base)
{
	if (_death)
	{
		_corpseRecovered = false; // They're not a corpse anymore!
		delete _death;
		_death = 0;
	}

	if (transformationRule->getRecoveryTime() > 0)
	{
		_recovery = transformationRule->getRecoveryTime();
	}
	_training = false;
	_returnToTrainingWhenHealed = false;
	_psiTraining = false;

	// needed, because the armor size may change (also, it just makes sense)
	sourceSoldier->setCraftAndMoveEquipment(0, base, false);

	if (transformationRule->isCreatingClone())
	{
		// a clone already has the correct soldier type, but random stats
		// if we don't want random stats, let's copy them from the source soldier
		UnitStats sourceStats = *sourceSoldier->getCurrentStats() + calculateStatChanges(mod, transformationRule, sourceSoldier, 0, sourceSoldier->getRules());
		UnitStats mergedStats = UnitStats::combine(transformationRule->getRerollStats(), sourceStats, _currentStats);
		setBothStats(&mergedStats);
	}
	else
	{
		// backup original soldier type, it will still be needed later for stat change calculations
		RuleSoldier* sourceSoldierType = _rules;

		// change soldier type if needed
		if (!Mod::isEmptyRuleName(transformationRule->getProducedSoldierType()) && _rules->getType() != transformationRule->getProducedSoldierType())
		{
			_rules = mod->getSoldier(transformationRule->getProducedSoldierType());

			// demote soldier if needed (i.e. when new soldier type doesn't support the current rank)
			if (!_rules->getAllowPromotion())
			{
				_rank = RANK_ROOKIE;
			}
			else if (!_rules->getRankStrings().empty() && (size_t)_rank > _rules->getRankStrings().size() - 1)
			{
				switch (_rules->getRankStrings().size() - 1)
				{
				case 1:
					_rank = RANK_SQUADDIE;
					break;
				case 2:
					_rank = RANK_SERGEANT;
					break;
				case 3:
					_rank = RANK_CAPTAIN;
					break;
				case 4:
					_rank = RANK_COLONEL;
					break;
				case 5:
					_rank = RANK_COMMANDER; // I hereby demote you to commander! :P
					break;
				default:
					_rank = RANK_ROOKIE;
					break;
				}
			}

			// clamp (and randomize) nationality if needed
			{
				const std::vector<SoldierNamePool *> &names = _rules->getNames();
				if (!names.empty())
				{
					if ((size_t)_nationality >= _rules->getNames().size())
					{
						_nationality = RNG::generate(0, names.size() - 1);
					}
				}
				else
				{
					_nationality = 0;
				}
			}
		}

		// change stats
		_currentStats += calculateStatChanges(mod, transformationRule, sourceSoldier, 0, sourceSoldierType);

		// and randomize stats where needed
		{
			Soldier *tmpSoldier = new Soldier(_rules, nullptr, 0 /*nationality*/, _id);
			_currentStats = UnitStats::combine(transformationRule->getRerollStats(), _currentStats, *tmpSoldier->getCurrentStats());
			delete tmpSoldier;
			tmpSoldier = 0;
		}
	}

	if (!transformationRule->isKeepingSoldierArmor())
	{
		auto* oldArmor = _armor;
		if (Mod::isEmptyRuleName(transformationRule->getProducedSoldierArmor()))
		{
			// default armor of the soldier's type
			_armor = _rules->getDefaultArmor();
		}
		else
		{
			// explicitly defined armor
			_armor = mod->getArmor(transformationRule->getProducedSoldierArmor());
		}
		if (oldArmor != _armor && !transformationRule->isCreatingClone())
		{
			if (oldArmor->getStoreItem())
			{
				base->getStorageItems()->addItem(oldArmor->getStoreItem());
			}
		}
	}

	// Reset performed transformations (on the destination soldier), if needed
	if (transformationRule->getReset())
	{
		_previousTransformations.clear();
	}

	// Remember the performed transformation (on the source soldier)
	auto& history = sourceSoldier->getPreviousTransformations();
	auto it = history.find(transformationRule->getName());
	if (it != history.end())
	{
		it->second += 1;
	}
	else
	{
		history[transformationRule->getName()] = 1;
	}

	// Reset soldier bonuses, if needed
	if (transformationRule->getReset())
	{
		_transformationBonuses.clear();
	}

	// Award a soldier bonus, if defined
	if (!Mod::isEmptyRuleName(transformationRule->getSoldierBonusType()))
	{
		auto it2 = _transformationBonuses.find(transformationRule->getSoldierBonusType());
		if (it2 != _transformationBonuses.end())
		{
			it2->second += 1;
		}
		else
		{
			_transformationBonuses[transformationRule->getSoldierBonusType()] = 1;
		}
	}
}

/**
 * Calculates the stat changes a soldier undergoes from this project
 * @param mod Pointer to the mod
 * @param mode 0 = final, 1 = min, 2 = max
 * @return The stat changes
 */
UnitStats Soldier::calculateStatChanges(const Mod *mod, RuleSoldierTransformation *transformationRule, Soldier *sourceSoldier, int mode, const RuleSoldier *sourceSoldierType)
{
	UnitStats statChange;

	UnitStats initialStats = *sourceSoldier->getInitStats();
	UnitStats currentStats = *sourceSoldier->getCurrentStats();
	UnitStats gainedStats = currentStats - initialStats;

	// Flat stat changes
	statChange += transformationRule->getFlatOverallStatChange();
	UnitStats rnd0;
	if (mode == 2)
		rnd0 = UnitStats::max(transformationRule->getFlatMin(), transformationRule->getFlatMax());
	else if (mode == 1)
		rnd0 = UnitStats::min(transformationRule->getFlatMin(), transformationRule->getFlatMax());
	else
		rnd0 = UnitStats::random(transformationRule->getFlatMin(), transformationRule->getFlatMax());
	statChange += rnd0;

	// Stat changes based on current stats
	statChange += UnitStats::percent(currentStats, transformationRule->getPercentOverallStatChange());
	UnitStats rnd1;
	if (mode == 2)
		rnd1 = UnitStats::max(transformationRule->getPercentMin(), transformationRule->getPercentMax());
	else if (mode == 1)
		rnd1 = UnitStats::min(transformationRule->getPercentMin(), transformationRule->getPercentMax());
	else
		rnd1 = UnitStats::random(transformationRule->getPercentMin(), transformationRule->getPercentMax());
	statChange += UnitStats::percent(currentStats, rnd1);

	// Stat changes based on gained stats
	statChange += UnitStats::percent(gainedStats, transformationRule->getPercentGainedStatChange());
	UnitStats rnd2;
	if (mode == 2)
		rnd2 = UnitStats::max(transformationRule->getPercentGainedMin(), transformationRule->getPercentGainedMax());
	else if (mode == 1)
		rnd2 = UnitStats::min(transformationRule->getPercentGainedMin(), transformationRule->getPercentGainedMax());
	else
		rnd2 = UnitStats::random(transformationRule->getPercentGainedMin(), transformationRule->getPercentGainedMax());
	statChange += UnitStats::percent(gainedStats, rnd2);

	// round (mathematically) to whole tens
	int sign = statChange.bravery < 0 ? -1 : 1;
	statChange.bravery = ((statChange.bravery + (sign * 5)) / 10) * 10;

	RuleSoldier *transformationSoldierType = _rules;
	if (!Mod::isEmptyRuleName(transformationRule->getProducedSoldierType()))
	{
		transformationSoldierType = mod->getSoldier(transformationRule->getProducedSoldierType());
	}

	if (transformationRule->hasLowerBoundAtMinStats())
	{
		UnitStats lowerBound = transformationSoldierType->getMinStats();
		UnitStats cappedChange = lowerBound - currentStats;

		statChange = UnitStats::max(statChange, cappedChange);
	}

	if (transformationRule->hasUpperBoundAtMaxStats() || transformationRule->hasUpperBoundAtStatCaps())
	{
		UnitStats upperBound = transformationRule->hasUpperBoundAtMaxStats()
			? transformationSoldierType->getMaxStats()
			: transformationSoldierType->getStatCaps();
		UnitStats cappedChange = upperBound - currentStats;

		bool isSameSoldierType = (transformationSoldierType == sourceSoldierType);
		bool softLimit = transformationRule->isSoftLimit(isSameSoldierType);
		if (softLimit)
		{
			// soft limit
			statChange = UnitStats::softLimit(statChange, currentStats, upperBound);
		}
		else
		{
			// hard limit
			statChange = UnitStats::min(statChange, cappedChange);
		}
	}

	return statChange;
}

/**
 * Gets all the soldier bonuses
 * @return The map of soldier bonuses
 */
const std::vector<const RuleSoldierBonus*> *Soldier::getBonuses(const Mod *mod)
{
	if (mod)
	{
		_bonusCache.clear();
		auto addSorted = [&](const RuleSoldierBonus* b)
		{
			if (!b)
			{
				return;
			}

			auto sort = [](const RuleSoldierBonus* l, const RuleSoldierBonus* r){ return l->getListOrder() < r->getListOrder(); };

			auto p = std::lower_bound(_bonusCache.begin(), _bonusCache.end(), b, sort);
			if (p == _bonusCache.end() || *p != b)
			{
				_bonusCache.insert(p, b);
			}
		};

		for (const auto& bonusName : _transformationBonuses)
		{
			auto* bonusRule = mod->getSoldierBonus(bonusName.first, false);

			addSorted(bonusRule);
		}
		for (auto* commendation : *_diary->getSoldierCommendations())
		{
			auto* bonusRule = commendation->getRule()->getSoldierBonus(commendation->getDecorationLevelInt());

			addSorted(bonusRule);
		}
	}

	return &_bonusCache;
}

/**
 * Get pointer to current stats with soldier bonuses, but without armor bonuses.
 */
const UnitStats *Soldier::getStatsWithSoldierBonusesOnly() const
{
	return &_tmpStatsWithSoldierBonuses;
}

/**
 * Get pointer to current stats with armor and soldier bonuses.
 */
const UnitStats *Soldier::getStatsWithAllBonuses() const
{
	return &_tmpStatsWithAllBonuses;
}

/**
 * Pre-calculates soldier stats with various bonuses.
 */
bool Soldier::prepareStatsWithBonuses(const Mod *mod)
{
	bool hasSoldierBonus = false;

	// 1. current stats
	UnitStats tmp = _currentStats;
	auto basePsiSkill = _currentStats.psiSkill;

	// 2. refresh soldier bonuses
	auto* bonuses = getBonuses(mod); // this is the only place where bonus cache is rebuilt

	// 3. apply soldier bonuses
	for (const auto* bonusRule : *bonuses)
	{
		hasSoldierBonus = true;
		tmp += *(bonusRule->getStats());
	}

	// 4. stats with soldier bonuses, but without armor bonuses
	_tmpStatsWithSoldierBonuses = UnitStats::obeyFixedMinimum(tmp);

	// if the psi skill has not been "unlocked" yet by training, do not allow soldier bonuses to unlock it
	if (basePsiSkill <= 0 && _tmpStatsWithSoldierBonuses.psiSkill > 0)
	{
		_tmpStatsWithSoldierBonuses.psiSkill = basePsiSkill;
	}

	// 5. apply armor bonus
	tmp += *_armor->getStats();

	// 6. stats with all bonuses
	_tmpStatsWithAllBonuses = UnitStats::obeyFixedMinimum(tmp);

	// 7. pilot armors count as soldier bonuses
	if (_armor->isPilotArmor())
	{
		_tmpStatsWithSoldierBonuses = _tmpStatsWithAllBonuses;
	}

	return hasSoldierBonus;
}

/**
 * Gets a pointer to the daily dogfight experience cache.
 */
UnitStats* Soldier::getDailyDogfightExperienceCache()
{
	return &_dailyDogfightExperienceCache;
}

/**
 * Resets the daily dogfight experience cache.
 */
void Soldier::resetDailyDogfightExperienceCache()
{
	_dailyDogfightExperienceCache = UnitStats::scalar(0);
}


////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

void getGenderScript(const Soldier *so, int &ret)
{
	if (so)
	{
		ret = so->getGender();
		return;
	}
	ret = 0;
}
void getRankScript(const Soldier *so, int &ret)
{
	if (so)
	{
		ret = so->getRank();
		return;
	}
	ret = 0;
}
void getLookScript(const Soldier *so, int &ret)
{
	if (so)
	{
		ret = so->getLook();
		return;
	}
	ret = 0;
}
void getLookVariantScript(const Soldier *so, int &ret)
{
	if (so)
	{
		ret = so->getLookVariant();
		return;
	}
	ret = 0;
}
struct getRuleSoldierScript
{
	static RetEnum func(const Soldier *so, const RuleSoldier* &ret)
	{
		if (so)
		{
			ret = so->getRules();
		}
		else
		{
			ret = nullptr;
		}
		return RetContinue;
	}
};

std::string debugDisplayScript(const Soldier* so)
{
	if (so)
	{
		std::string s;
		s += Soldier::ScriptName;
		s += "(type: \"";
		s += so->getRules()->getType();
		s += "\" id: ";
		s += std::to_string(so->getId());
		s += " name: \"";
		s += so->getName(false, 0);
		s += "\")";
		return s;
	}
	else
	{
		return "null";
	}
}

} // namespace

/**
 * Register Soldier in script parser.
 * @param parser Script parser.
 */
void Soldier::ScriptRegister(ScriptParserBase* parser)
{
	parser->registerPointerType<RuleSoldier>();

	Bind<Soldier> so = { parser };


	so.addField<&Soldier::_id>("getId");
	so.add<&getRankScript>("getRank");
	so.add<&getGenderScript>("getGender");
	so.add<&getLookScript>("getLook");
	so.add<&getLookVariantScript>("getLookVariant");


	UnitStats::addGetStatsScript<&Soldier::_currentStats>(so, "Stats.");
	UnitStats::addSetStatsScript<&Soldier::_currentStats>(so, "Stats.");


	so.addFunc<getRuleSoldierScript>("getRuleSoldier");
	so.add<&Soldier::getWoundRecoveryInt>("getWoundRecovery");
	so.add<&Soldier::setWoundRecovery>("setWoundRecovery");
	so.add<&Soldier::getManaMissing>("getManaMissing");
	so.add<&Soldier::setManaMissing>("setManaMissing");
	so.add<&Soldier::getHealthMissing>("getHealthMissing");
	so.add<&Soldier::setHealthMissing>("setHealthMissing");


	so.addScriptValue<BindBase::OnlyGet, &Soldier::_rules, &RuleSoldier::getScriptValuesRaw>();
	so.addScriptValue<&Soldier::_scriptValues>();
	so.addDebugDisplay<&debugDisplayScript>();
}

} // namespace OpenXcom
