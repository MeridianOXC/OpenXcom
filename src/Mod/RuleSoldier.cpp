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

#include <algorithm>
#include "RuleSoldier.h"
#include "RuleSkill.h"
#include "Mod.h"
#include "ModScript.h"
#include "RuleItem.h"
#include "Armor.h"
#include "SoldierNamePool.h"
#include "StatString.h"
#include "../Engine/FileMap.h"
#include "../Engine/ScriptBind.h"
#include "../Engine/Unicode.h"

namespace OpenXcom
{

/**
 * Creates a blank RuleSoldier for a certain
 * type of soldier.
 * @param type String defining the type.
 */
RuleSoldier::RuleSoldier(const std::string &type) : _type(type), _listOrder(0), _armor(nullptr), _specWeapon(nullptr),
	_monthlyBuyLimit(0), _costBuy(0), _costSalary(0),
	_costSalarySquaddie(0), _costSalarySergeant(0), _costSalaryCaptain(0), _costSalaryColonel(0), _costSalaryCommander(0),
	_standHeight(0), _kneelHeight(0), _floatHeight(0), _femaleFrequency(50), _value(20), _transferTime(0), _moraleLossWhenKilled(100),
	_totalSoldierNamePoolWeight(0),
	_avatarOffsetX(67), _avatarOffsetY(48), _flagOffset(0),
	_allowPromotion(true), _allowPiloting(true), _showTypeInInventory(false),
	_rankSprite(42), _rankSpriteBattlescape(20), _rankSpriteTiny(0), _skillIconSprite(1)
{
}

/**
 *
 */
RuleSoldier::~RuleSoldier()
{
	for (std::vector<SoldierNamePool*>::iterator i = _names.begin(); i != _names.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<StatString*>::iterator i = _statStrings.begin(); i != _statStrings.end(); ++i)
	{
		delete *i;
	}
}

/**
 * Loads the soldier from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the unit.
 */
void RuleSoldier::load(const YAML::Node &node, Mod *mod, int listOrder, const ModScript &parsers)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, mod, listOrder, parsers);
	}
	_type = node["type"].as<std::string>(_type);
	// Just in case
	if (_type == "XCOM")
		_type = "STR_SOLDIER";

	//requires
	mod->loadUnorderedNames(_type, _requires, node["requires"]);
	mod->loadBaseFunction(_type, _requiresBuyBaseFunc, node["requiresBuyBaseFunc"]);


	_minStats.merge(node["minStats"].as<UnitStats>(_minStats));
	_maxStats.merge(node["maxStats"].as<UnitStats>(_maxStats));
	_statCaps.merge(node["statCaps"].as<UnitStats>(_statCaps));
	if (node["trainingStatCaps"])
	{
		_trainingStatCaps.merge(node["trainingStatCaps"].as<UnitStats>(_trainingStatCaps));
	}
	else
	{
		_trainingStatCaps.merge(node["statCaps"].as<UnitStats>(_trainingStatCaps));
	}
	_dogfightExperience.merge(node["dogfightExperience"].as<UnitStats>(_dogfightExperience));
	mod->loadName(_type, _armorName, node["armor"]);
	_specWeaponName = node["specialWeapon"].as<std::string>(_specWeaponName);
	_armorForAvatar = node["armorForAvatar"].as<std::string>(_armorForAvatar);
	_avatarOffsetX = node["avatarOffsetX"].as<int>(_avatarOffsetX);
	_avatarOffsetY = node["avatarOffsetY"].as<int>(_avatarOffsetY);
	_flagOffset = node["flagOffset"].as<int>(_flagOffset);
	_allowPromotion = node["allowPromotion"].as<bool>(_allowPromotion);
	_allowPiloting = node["allowPiloting"].as<bool>(_allowPiloting);
	_monthlyBuyLimit = node["monthlyBuyLimit"].as<int>(_monthlyBuyLimit);
	_costBuy = node["costBuy"].as<int>(_costBuy);
	_costSalary = node["costSalary"].as<int>(_costSalary);
	_costSalarySquaddie = node["costSalarySquaddie"].as<int>(_costSalarySquaddie);
	_costSalarySergeant = node["costSalarySergeant"].as<int>(_costSalarySergeant);
	_costSalaryCaptain = node["costSalaryCaptain"].as<int>(_costSalaryCaptain);
	_costSalaryColonel = node["costSalaryColonel"].as<int>(_costSalaryColonel);
	_costSalaryCommander = node["costSalaryCommander"].as<int>(_costSalaryCommander);
	_standHeight = node["standHeight"].as<int>(_standHeight);
	_kneelHeight = node["kneelHeight"].as<int>(_kneelHeight);
	_floatHeight = node["floatHeight"].as<int>(_floatHeight);
	_femaleFrequency = node["femaleFrequency"].as<int>(_femaleFrequency);
	_value = node["value"].as<int>(_value);
	_transferTime = node["transferTime"].as<int>(_transferTime);
	_moraleLossWhenKilled = node["moraleLossWhenKilled"].as<int>(_moraleLossWhenKilled);
	_showTypeInInventory = node["showTypeInInventory"].as<bool>(_showTypeInInventory);

	mod->loadSoundOffset(_type, _deathSoundMale, node["deathMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _deathSoundFemale, node["deathFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _panicSoundMale, node["panicMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _panicSoundFemale, node["panicFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _berserkSoundMale, node["berserkMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _berserkSoundFemale, node["berserkFemale"], "BATTLE.CAT");

	mod->loadSoundOffset(_type, _selectUnitSoundMale, node["selectUnitMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _selectUnitSoundFemale, node["selectUnitFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _startMovingSoundMale, node["startMovingMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _startMovingSoundFemale, node["startMovingFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _selectWeaponSoundMale, node["selectWeaponMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _selectWeaponSoundFemale, node["selectWeaponFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _annoyedSoundMale, node["annoyedMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _annoyedSoundFemale, node["annoyedFemale"], "BATTLE.CAT");

	for (YAML::const_iterator i = node["soldierNames"].begin(); i != node["soldierNames"].end(); ++i)
	{
		std::string fileName = (*i).as<std::string>();
		if (fileName == "delete")
		{
			for (std::vector<SoldierNamePool*>::iterator j = _names.begin(); j != _names.end(); ++j)
			{
				delete *j;
			}
			_names.clear();
		}
		else
		{
			if (fileName[fileName.length() - 1] == '/')
			{
				// load all *.nam files in given directory
				std::vector<std::string> names;
				for (auto f: FileMap::filterFiles(FileMap::getVFolderContents(fileName), "nam")) { names.push_back(f); }
				std::sort(names.begin(), names.end(), Unicode::naturalCompare);
				for (auto j = names.begin(); j != names.end(); ++j)
				{
					addSoldierNamePool(fileName + *j);
				}
			}
			else
			{
				// load given file
				addSoldierNamePool(fileName);
			}
		}
	}

	for (YAML::const_iterator i = node["statStrings"].begin(); i != node["statStrings"].end(); ++i)
	{
		StatString *statString = new StatString();
		statString->load(*i);
		_statStrings.push_back(statString);
	}

	mod->loadNames(_type, _rankStrings, node["rankStrings"]);
	mod->loadSpriteOffset(_type, _rankSprite, node["rankSprite"], "BASEBITS.PCK");
	mod->loadSpriteOffset(_type, _rankSpriteBattlescape, node["rankBattleSprite"], "SMOKE.PCK");
	mod->loadSpriteOffset(_type, _rankSpriteTiny, node["rankTinySprite"], "TinyRanks");
	mod->loadSpriteOffset(_type, _skillIconSprite, node["skillIconSprite"], "SPICONS.DAT");

	mod->loadNames(_type, _skillNames, node["skills"]);

	_listOrder = node["listOrder"].as<int>(_listOrder);
	if (!_listOrder)
	{
		_listOrder = listOrder;
	}

	_scriptValues.load(node, parsers.getShared());
}

/**
 * Cross link with other Rules.
 */
void RuleSoldier::afterLoad(const Mod* mod)
{
	_totalSoldierNamePoolWeight = 0;
	for (auto* namepool : _names)
	{
		_totalSoldierNamePoolWeight += namepool->getGlobalWeight();
	}

	mod->linkRule(_armor, _armorName);
	mod->checkForSoftError(_armor == nullptr, _type, "Soldier type is missing the default armor", LOG_ERROR);

	mod->verifySoundOffset(_type, _deathSoundMale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _deathSoundFemale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _panicSoundMale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _panicSoundFemale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _berserkSoundMale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _berserkSoundFemale, "BATTLE.CAT");

	mod->verifySoundOffset(_type, _selectUnitSoundMale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _selectUnitSoundFemale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _startMovingSoundMale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _startMovingSoundFemale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _selectWeaponSoundMale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _selectWeaponSoundFemale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _annoyedSoundMale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _annoyedSoundFemale, "BATTLE.CAT");

	mod->verifySpriteOffset(_type, _rankSprite, "BASEBITS.PCK");
	mod->verifySpriteOffset(_type, _rankSpriteBattlescape, "SMOKE.PCK");
	mod->verifySpriteOffset(_type, _rankSpriteTiny, "TinyRanks");
	mod->verifySpriteOffset(_type, _skillIconSprite, "SPICONS.DAT");

	if (!_specWeaponName.empty())
	{
		mod->linkRule(_specWeapon, _specWeaponName);

		if ((_specWeapon->getBattleType() == BT_FIREARM || _specWeapon->getBattleType() == BT_MELEE) && !_specWeapon->getClipSize())
		{
			throw Exception("Weapon " + _specWeaponName + " is used as a special weapon, but doesn't have its own ammo - give it a clipSize!");
		}
	}
	mod->linkRule(_skills, _skillNames);

	_manaMissingWoundThreshold = mod->getManaWoundThreshold();
	_healthMissingWoundThreshold = mod->getHealthWoundThreshold();
}

void RuleSoldier::addSoldierNamePool(const std::string &namFile)
{
	SoldierNamePool *pool = new SoldierNamePool();
	pool->load(namFile);
	_names.push_back(pool);
}

/**
 * Returns the language string that names
 * this soldier. Each soldier type has a unique name.
 * @return Soldier name.
 */
const std::string& RuleSoldier::getType() const
{
	return _type;
}

/**
 * Gets the list/sort order of the soldier's type.
 * @return The list/sort order.
 */
int RuleSoldier::getListOrder() const
{
	return _listOrder;
}

/**
 * Gets the list of research required to
 * acquire this soldier.
 * @return The list of research IDs.
*/
const std::vector<std::string> &RuleSoldier::getRequirements() const
{
	return _requires;
}

/**
 * Gets the minimum stats for the random stats generator.
 * @return The minimum stats.
 */
UnitStats RuleSoldier::getMinStats() const
{
	return _minStats;
}

/**
 * Gets the maximum stats for the random stats generator.
 * @return The maximum stats.
 */
UnitStats RuleSoldier::getMaxStats() const
{
	return _maxStats;
}

/**
 * Gets the stat caps.
 * @return The stat caps.
 */
UnitStats RuleSoldier::getStatCaps() const
{
	return _statCaps;
}

/**
* Gets the training stat caps.
* @return The training stat caps.
*/
UnitStats RuleSoldier::getTrainingStatCaps() const
{
	return _trainingStatCaps;
}

/**
* Gets the improvement chances for pilots (after dogfight).
* @return The improvement changes.
*/
UnitStats RuleSoldier::getDogfightExperience() const
{
	return _dogfightExperience;
}

/**
 * Gets the cost of hiring this soldier.
 * @return The cost.
 */
int RuleSoldier::getBuyCost() const
{
	return _costBuy;
}

/**
* Does salary depend on rank?
* @return True if salary depends on rank, false otherwise.
*/
bool RuleSoldier::isSalaryDynamic() const
{
	return _costSalarySquaddie || _costSalarySergeant || _costSalaryCaptain || _costSalaryColonel || _costSalaryCommander;
}

/**
 * Is a skill menu defined?
 * @return True if a skill menu has been defined, false otherwise.
 */
bool RuleSoldier::isSkillMenuDefined() const
{
	return !_skills.empty();
}

/**
 * Gets the list of defined skills.
 * @return The list of defined skills.
 */
const std::vector<const RuleSkill*> &RuleSoldier::getSkills() const
{
	return _skills;
}

/**
 * Gets the sprite index into SPICONS for the skill icon sprite.
 * @return The sprite index.
 */
int RuleSoldier::getSkillIconSprite() const
{
	return _skillIconSprite;
}

/**
 * Gets the cost of salary for a month (for a given rank).
 * @param rank Soldier rank.
 * @return The cost.
 */
int RuleSoldier::getSalaryCost(int rank) const
{
	int total = _costSalary;
	switch (rank)
	{
		case 1: total += _costSalarySquaddie; break;
		case 2: total += _costSalarySergeant; break;
		case 3: total += _costSalaryCaptain; break;
		case 4: total += _costSalaryColonel; break;
		case 5: total += _costSalaryCommander; break;
		default: break;
	}
	return total;
}

/**
 * Gets the height of the soldier when it's standing.
 * @return The standing height.
 */
int RuleSoldier::getStandHeight() const
{
	return _standHeight;
}

/**
 * Gets the height of the soldier when it's kneeling.
 * @return The kneeling height.
 */
int RuleSoldier::getKneelHeight() const
{
	return _kneelHeight;
}

/**
 * Gets the elevation of the soldier when it's flying.
 * @return The floating height.
 */
int RuleSoldier::getFloatHeight() const
{
	return _floatHeight;
}

/**
 * Gets the default armor name.
 * @return The armor name.
 */
Armor* RuleSoldier::getDefaultArmor() const
{
	return const_cast<Armor*>(_armor); //TODO: fix this function usage to remove const cast
}

/**
* Gets the armor for avatar.
* @return The armor name.
*/
const std::string& RuleSoldier::getArmorForAvatar() const
{
	return _armorForAvatar;
}

/**
* Gets the avatar's X offset.
* @return The X offset.
*/
int RuleSoldier::getAvatarOffsetX() const
{
	return _avatarOffsetX;
}

/**
* Gets the avatar's Y offset.
* @return The Y offset.
*/
int RuleSoldier::getAvatarOffsetY() const
{
	return _avatarOffsetY;
}

/**
* Gets the flag offset.
* @return The flag offset.
*/
int RuleSoldier::getFlagOffset() const
{
	return _flagOffset;
}

/**
* Gets the allow promotion flag.
* @return True if promotion is allowed.
*/
bool RuleSoldier::getAllowPromotion() const
{
	return _allowPromotion;
}

/**
* Gets the allow piloting flag.
* @return True if piloting is allowed.
*/
bool RuleSoldier::getAllowPiloting() const
{
	return _allowPiloting;
}

/**
 * Gets the female appearance ratio.
 * @return The percentage ratio.
 */
int RuleSoldier::getFemaleFrequency() const
{
	return _femaleFrequency;
}

/**
 * Gets the death sounds for male soldiers.
 * @return List of sound IDs.
 */
const std::vector<int> &RuleSoldier::getMaleDeathSounds() const
{
	return _deathSoundMale;
}

/**
 * Gets the death sounds for female soldiers.
 * @return List of sound IDs.
 */
const std::vector<int> &RuleSoldier::getFemaleDeathSounds() const
{
	return _deathSoundFemale;
}

/**
 * Gets the panic sounds for male soldiers.
 * @return List of sound IDs.
 */
const std::vector<int> &RuleSoldier::getMalePanicSounds() const
{
	return _panicSoundMale;
}

/**
 * Gets the panic sounds for female soldiers.
 * @return List of sound IDs.
 */
const std::vector<int> &RuleSoldier::getFemalePanicSounds() const
{
	return _panicSoundFemale;
}

/**
 * Gets the berserk sounds for male soldiers.
 * @return List of sound IDs.
 */
const std::vector<int> &RuleSoldier::getMaleBerserkSounds() const
{
	return _berserkSoundMale;
}

/**
 * Gets the berserk sounds for female soldiers.
 * @return List of sound IDs.
 */
const std::vector<int> &RuleSoldier::getFemaleBerserkSounds() const
{
	return _berserkSoundFemale;
}

/**
 * Returns the list of soldier name pools.
 * @return Pointer to soldier name pool list.
 */
const std::vector<SoldierNamePool*> &RuleSoldier::getNames() const
{
	return _names;
}

/*
 * Gets the soldier's base value, without experience modifiers.
 * @return The soldier's value.
 */
int RuleSoldier::getValue() const
{
	return _value;
}

/*
 * Gets the amount of time this item
 * takes to arrive at a base.
 * @return The time in hours.
 */
int RuleSoldier::getTransferTime() const
{
	return _transferTime;
}

/**
* Gets the list of StatStrings.
* @return The list of StatStrings.
*/
const std::vector<StatString *> &RuleSoldier::getStatStrings() const
{
	return _statStrings;
}

/**
 * Gets the list of strings for this soldier's ranks
 * @return The list rank strings.
 */
const std::vector<std::string> &RuleSoldier::getRankStrings() const
{
	return _rankStrings;
}

/**
 * Gets the index of the sprites to use to represent this soldier's rank in BASEBITS.PCK
 * @return The sprite index.
 */
int RuleSoldier::getRankSprite() const
{
	return _rankSprite;
}

/**
 * Gets the index of the sprites to use to represent this soldier's rank in SMOKE.PCK
 * @return The sprite index.
 */
int RuleSoldier::getRankSpriteBattlescape() const
{
	return _rankSpriteBattlescape;
}

/**
 * Gets the index of the sprites to use to represent this soldier's rank in TinyRanks
 * @return The sprite index.
 */
int RuleSoldier::getRankSpriteTiny() const
{
	return _rankSpriteTiny;
}


////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////


namespace
{

void getTypeScript(const RuleSkill* r, ScriptText& txt)
{
	if (r)
	{
		txt = { r->getType().c_str() };
		return;
	}
	else
	{
		txt = ScriptText::empty;
	}
}

std::string debugDisplayScript(const RuleSoldier* rs)
{
	if (rs)
	{
		std::string s;
		s += RuleSoldier::ScriptName;
		s += "(name: \"";
		s += rs->getType();
		s += "\")";
		return s;
	}
	else
	{
		return "null";
	}
}

}

/**
 * Register Armor in script parser.
 * @param parser Script parser.
 */
void RuleSoldier::ScriptRegister(ScriptParserBase* parser)
{
	Bind<RuleSoldier> ra = { parser };

	UnitStats::addGetStatsScript<&RuleSoldier::_statCaps>(ra, "StatsCap.");
	UnitStats::addGetStatsScript<&RuleSoldier::_minStats>(ra, "StatsMin.");
	UnitStats::addGetStatsScript<&RuleSoldier::_maxStats>(ra, "StatsMax.");

	ra.add<&getTypeScript>("getType");

	ra.addScriptValue<BindBase::OnlyGet, &RuleSoldier::_scriptValues>();
	ra.addDebugDisplay<&debugDisplayScript>();
}

}
