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
RuleSoldier::RuleSoldier(const std::string &type, int listOrder) : _type(type), _group(0), _listOrder(listOrder), _armor(nullptr), _specWeapon(nullptr),
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
	for (auto* namepool : _names)
	{
		delete namepool;
	}
	for (auto* statString : _statStrings)
	{
		delete statString;
	}
}

/**
 * Loads the soldier from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the unit.
 */
void RuleSoldier::load(const YAML::YamlNodeReader& node, Mod *mod, const ModScript &parsers)
{
	const auto& reader = node.useIndex();
	if (const auto& parent = reader["refNode"])
	{
		load(parent, mod, parsers);
	}

	//requires
	mod->loadUnorderedNames(_type, _requires, reader["requires"]);
	mod->loadBaseFunction(_type, _requiresBuyBaseFunc, reader["requiresBuyBaseFunc"]);
	reader.tryRead("requiresBuyCountry", _requiresBuyCountry);


	_minStats.merge(reader["minStats"].readVal(_minStats));
	_maxStats.merge(reader["maxStats"].readVal(_maxStats));
	_statCaps.merge(reader["statCaps"].readVal(_statCaps));
	if (reader["trainingStatCaps"])
	{
		_trainingStatCaps.merge(reader["trainingStatCaps"].readVal(_trainingStatCaps));
	}
	else
	{
		_trainingStatCaps.merge(reader["statCaps"].readVal(_trainingStatCaps));
	}
	_dogfightExperience.merge(reader["dogfightExperience"].readVal(_dogfightExperience));
	mod->loadName(_type, _armorName, reader["armor"]);
	reader.tryRead("specialWeapon", _specWeaponName);
	reader.tryRead("armorForAvatar", _armorForAvatar);
	reader.tryRead("avatarOffsetX", _avatarOffsetX);
	reader.tryRead("avatarOffsetY", _avatarOffsetY);
	reader.tryRead("flagOffset", _flagOffset);
	reader.tryRead("allowPromotion", _allowPromotion);
	reader.tryRead("allowPiloting", _allowPiloting);
	reader.tryRead("monthlyBuyLimit", _monthlyBuyLimit);
	reader.tryRead("costBuy", _costBuy);
	reader.tryRead("costSalary", _costSalary);
	reader.tryRead("costSalarySquaddie", _costSalarySquaddie);
	reader.tryRead("costSalarySergeant", _costSalarySergeant);
	reader.tryRead("costSalaryCaptain", _costSalaryCaptain);
	reader.tryRead("costSalaryColonel", _costSalaryColonel);
	reader.tryRead("costSalaryCommander", _costSalaryCommander);
	reader.tryRead("standHeight", _standHeight);
	reader.tryRead("kneelHeight", _kneelHeight);
	reader.tryRead("floatHeight", _floatHeight);
	reader.tryRead("femaleFrequency", _femaleFrequency);
	reader.tryRead("value", _value);
	reader.tryRead("transferTime", _transferTime);
	reader.tryRead("moraleLossWhenKilled", _moraleLossWhenKilled);
	reader.tryRead("showTypeInInventory", _showTypeInInventory);

	mod->loadSoundOffset(_type, _deathSoundMale, reader["deathMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _deathSoundFemale, reader["deathFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _panicSoundMale, reader["panicMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _panicSoundFemale, reader["panicFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _berserkSoundMale, reader["berserkMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _berserkSoundFemale, reader["berserkFemale"], "BATTLE.CAT");

	mod->loadSoundOffset(_type, _selectUnitSoundMale, reader["selectUnitMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _selectUnitSoundFemale, reader["selectUnitFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _startMovingSoundMale, reader["startMovingMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _startMovingSoundFemale, reader["startMovingFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _selectWeaponSoundMale, reader["selectWeaponMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _selectWeaponSoundFemale, reader["selectWeaponFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _annoyedSoundMale, reader["annoyedMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _annoyedSoundFemale, reader["annoyedFemale"], "BATTLE.CAT");

	for (const auto& fileNameOrCommand : reader["soldierNames"].children())
	{
		std::string fileName = fileNameOrCommand.readVal<std::string>();
		if (fileName == "delete")
		{
			for (auto* namepool : _names)
			{
				delete namepool;
			}
			_names.clear();
		}
		else
		{
			if (fileName[fileName.length() - 1] == '/')
			{
				// load all *.nam files in given directory
				std::vector<std::string> names;
				for (const auto& f: FileMap::filterFiles(FileMap::getVFolderContents(fileName), "nam")) { names.push_back(f); }
				std::sort(names.begin(), names.end(), Unicode::naturalCompare);
				for (const auto& name : names)
				{
					addSoldierNamePool(fileName + name);
				}
			}
			else
			{
				// load given file
				addSoldierNamePool(fileName);
			}
		}
	}

	for (const auto& statStringReader : reader["statStrings"].children())
	{
		StatString *statString = new StatString();
		statString->load(statStringReader);
		_statStrings.push_back(statString);
	}

	mod->loadNames(_type, _rankStrings, reader["rankStrings"]);
	mod->loadSpriteOffset(_type, _rankSprite, reader["rankSprite"], "BASEBITS.PCK");
	mod->loadSpriteOffset(_type, _rankSpriteBattlescape, reader["rankBattleSprite"], "SMOKE.PCK");
	mod->loadSpriteOffset(_type, _rankSpriteTiny, reader["rankTinySprite"], "TinyRanks");
	mod->loadSpriteOffset(_type, _skillIconSprite, reader["skillIconSprite"], "SPICONS.DAT");

	mod->loadNames(_type, _skillNames, reader["skills"]);

	if (reader["spawnedSoldier"])
	{
		_spawnedSoldier = reader["spawnedSoldier"].emitDescendants(YAML::YamlRootNodeReader(_spawnedSoldier, "(spawned soldier template)"));
	}
	reader.tryRead("group", _group);
	reader.tryRead("listOrder", _listOrder);

	_scriptValues.load(reader, parsers.getShared());
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
	if (_totalSoldierNamePoolWeight < 1)
	{
		Log(LOG_ERROR) << _type << ": total soldier name pool weight is invalid. Forgotten 'soldierNames:' ?";
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
