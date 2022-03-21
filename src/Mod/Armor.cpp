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
#include "Armor.h"
#include "Unit.h"
#include "../Engine/ScriptBind.h"
#include "Mod.h"
#include "RuleSoldier.h"
#include "../Savegame/BattleUnit.h"

namespace OpenXcom
{

namespace
{

const Sint8 defTriBool = -1;

void loadTriBoolHelper(Sint8& value, const YAML::Node& node)
{
	if (node)
	{
		if (node.IsNull())
		{
			value = defTriBool;
		}
		else
		{
			value = node.as<bool>();
		}
	}
}

bool useTriBoolHelper(Sint8 value, bool def)
{
	return value == defTriBool ? def : value;
}

}

const std::string Armor::NONE = "STR_NONE";

/**
 * Creates a blank ruleset for a certain
 * type of armor.
 * @param type String defining the type.
 */
Armor::Armor(const std::string &type) :
	_type(type), _infiniteSupply(false), _frontArmor(0), _sideArmor(0), _leftArmorDiff(0), _rearArmor(0), _underArmor(0),
	_drawingRoutine(0), _drawBubbles(false), _movementType(MT_WALK), _specab(SPECAB_NONE), _turnBeforeFirstStep(false), _turnCost(1), _moveSound(-1), _size(1), _weight(0),
	_visibilityAtDark(0), _visibilityAtDay(0), _personalLight(15),
	_camouflageAtDay(0), _camouflageAtDark(0), _antiCamouflageAtDay(0), _antiCamouflageAtDark(0), _heatVision(0), _psiVision(0), _psiCamouflage(0),
	_deathFrames(3), _constantAnimation(false), _hasInventory(true), _forcedTorso(TORSO_USE_GENDER),
	_faceColorGroup(0), _hairColorGroup(0), _utileColorGroup(0), _rankColorGroup(0),
	_fearImmune(defTriBool), _bleedImmune(defTriBool), _painImmune(defTriBool), _zombiImmune(defTriBool),
	_ignoresMeleeThreat(defTriBool), _createsMeleeThreat(defTriBool),
	_overKill(0.5f), _meleeDodgeBackPenalty(0),
	_allowsRunning(defTriBool), _allowsStrafing(defTriBool), _allowsKneeling(defTriBool), _allowsMoving(1),
	_isPilotArmor(false), _allowTwoMainWeapons(false), _instantWoundRecovery(false),
	_standHeight(-1), _kneelHeight(-1), _floatHeight(-1)
{
	for (int i=0; i < DAMAGE_TYPES; i++)
		_damageModifier[i] = 1.0f;

	_psiDefence.setPsiDefense();
	_timeRecovery.setTimeRecovery();
	_energyRecovery.setEnergyRecovery();
	_stunRecovery.setStunRecovery();

	_customArmorPreviewIndex.push_back(Mod::NO_SURFACE);
}

/**
 *
 */
Armor::~Armor()
{

}

/**
 * Loads the armor from a YAML file.
 * @param node YAML node.
 */
void Armor::load(const YAML::Node &node, const ModScript &parsers, Mod *mod)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, parsers, mod);
	}
	_ufopediaType = node["ufopediaType"].as<std::string>(_ufopediaType);
	_type = node["type"].as<std::string>(_type);
	_spriteSheet = node["spriteSheet"].as<std::string>(_spriteSheet);
	_spriteInv = node["spriteInv"].as<std::string>(_spriteInv);
	_hasInventory = node["allowInv"].as<bool>(_hasInventory);
	if (node["corpseItem"])
	{
		_corpseBattleNames.clear();
		_corpseBattleNames.push_back(node["corpseItem"].as<std::string>());
		_corpseGeoName = _corpseBattleNames[0];
	}
	else if (node["corpseBattle"])
	{
		mod->loadNames(_type, _corpseBattleNames, node["corpseBattle"]);
		_corpseGeoName = _corpseBattleNames.at(0);
	}
	mod->loadNames(_type, _builtInWeaponsNames, node["builtInWeapons"]);
	mod->loadName(_type, _corpseGeoName, node["corpseGeo"]);
	mod->loadNameNull(_type, _storeItemName, node["storeItem"]);
	mod->loadNameNull(_type, _specWeaponName, node["specialWeapon"]);
	mod->loadNameNull(_type, _requiresName, node["requires"]);

	_layersDefaultPrefix = node["layersDefaultPrefix"].as<std::string>(_layersDefaultPrefix);
	_layersSpecificPrefix = node["layersSpecificPrefix"].as< std::map<int, std::string> >(_layersSpecificPrefix);
	_layersDefinition = node["layersDefinition"].as< std::map<std::string, std::vector<std::string> > >(_layersDefinition);

	_frontArmor = node["frontArmor"].as<int>(_frontArmor);
	_sideArmor = node["sideArmor"].as<int>(_sideArmor);
	_leftArmorDiff = node["leftArmorDiff"].as<int>(_leftArmorDiff);
	_rearArmor = node["rearArmor"].as<int>(_rearArmor);
	_underArmor = node["underArmor"].as<int>(_underArmor);
	_drawingRoutine = node["drawingRoutine"].as<int>(_drawingRoutine);
	_drawBubbles = node["drawBubbles"].as<bool>(_drawBubbles);
	_movementType = (MovementType)node["movementType"].as<int>(_movementType);
	_specab = (SpecialAbility)node["specab"].as<int>(_specab);

	_turnBeforeFirstStep = node["turnBeforeFirstStep"].as<bool>(_turnBeforeFirstStep);
	_turnCost = node["turnCost"].as<int>(_turnCost);

	mod->loadSoundOffset(_type, _moveSound, node["moveSound"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _deathSoundMale, node["deathMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _deathSoundFemale, node["deathFemale"], "BATTLE.CAT");

	mod->loadSoundOffset(_type, _selectUnitSoundMale, node["selectUnitMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _selectUnitSoundFemale, node["selectUnitFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _startMovingSoundMale, node["startMovingMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _startMovingSoundFemale, node["startMovingFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _selectWeaponSoundMale, node["selectWeaponMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _selectWeaponSoundFemale, node["selectWeaponFemale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _annoyedSoundMale, node["annoyedMale"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _annoyedSoundFemale, node["annoyedFemale"], "BATTLE.CAT");

	_weight = node["weight"].as<int>(_weight);
	_visibilityAtDark = node["visibilityAtDark"].as<int>(_visibilityAtDark);
	_visibilityAtDay = node["visibilityAtDay"].as<int>(_visibilityAtDay);
	_personalLight = node["personalLight"].as<int>(_personalLight);
	_camouflageAtDay = node["camouflageAtDay"].as<int>(_camouflageAtDay);
	_camouflageAtDark = node["camouflageAtDark"].as<int>(_camouflageAtDark);
	_antiCamouflageAtDay = node["antiCamouflageAtDay"].as<int>(_antiCamouflageAtDay);
	_antiCamouflageAtDark = node["antiCamouflageAtDark"].as<int>(_antiCamouflageAtDark);
	_heatVision = node["heatVision"].as<int>(_heatVision);
	_psiVision = node["psiVision"].as<int>(_psiVision);
	_psiCamouflage = node["psiCamouflage"].as<int>(_psiCamouflage);
	_isAlwaysVisible =  node["alwaysVisible"].as<bool>(_isAlwaysVisible);

	_stats.merge(node["stats"].as<UnitStats>(_stats));
	if (const YAML::Node &dmg = node["damageModifier"])
	{
		for (size_t i = 0; i < dmg.size() && i < (size_t)DAMAGE_TYPES; ++i)
		{
			_damageModifier[i] = dmg[i].as<float>();
		}
	}
	mod->loadInts(_type, _loftempsSet, node["loftempsSet"]);
	if (node["loftemps"])
		_loftempsSet = { node["loftemps"].as<int>() };
	_deathFrames = node["deathFrames"].as<int>(_deathFrames);
	_constantAnimation = node["constantAnimation"].as<bool>(_constantAnimation);
	_forcedTorso = (ForcedTorso)node["forcedTorso"].as<int>(_forcedTorso);

	if (const YAML::Node &size = node["size"])
	{
		_size = size.as<int>(_size);
		if (_size != 1)
		{
			_fearImmune = 1;
			_bleedImmune = 1;
			_painImmune = 1;
			_zombiImmune = 1;
			_ignoresMeleeThreat = 1;
			_createsMeleeThreat = 0;
		}
	}
	loadTriBoolHelper(_fearImmune, node["fearImmune"]);
	loadTriBoolHelper(_bleedImmune, node["bleedImmune"]);
	loadTriBoolHelper(_painImmune, node["painImmune"]);
	if (_size == 1) //Big units are always immune, because game we don't have 2x2 unit zombie
	{
		loadTriBoolHelper(_zombiImmune, node["zombiImmune"]);
	}
	loadTriBoolHelper(_ignoresMeleeThreat, node["ignoresMeleeThreat"]);
	loadTriBoolHelper(_createsMeleeThreat, node["createsMeleeThreat"]);

	_overKill = node["overKill"].as<float>(_overKill);
	_meleeDodgeBackPenalty = node["meleeDodgeBackPenalty"].as<float>(_meleeDodgeBackPenalty);

	_psiDefence.load(_type, node, parsers.bonusStatsScripts.get<ModScript::PsiDefenceStatBonus>());
	_meleeDodge.load(_type, node, parsers.bonusStatsScripts.get<ModScript::MeleeDodgeStatBonus>());

	const YAML::Node &rec = node["recovery"];
	{
		_timeRecovery.load(_type, rec, parsers.bonusStatsScripts.get<ModScript::TimeRecoveryStatBonus>());
		_energyRecovery.load(_type, rec, parsers.bonusStatsScripts.get<ModScript::EnergyRecoveryStatBonus>());
		_moraleRecovery.load(_type, rec, parsers.bonusStatsScripts.get<ModScript::MoraleRecoveryStatBonus>());
		_healthRecovery.load(_type, rec, parsers.bonusStatsScripts.get<ModScript::HealthRecoveryStatBonus>());
		_manaRecovery.load(_type, rec, parsers.bonusStatsScripts.get<ModScript::ManaRecoveryStatBonus>());
		_stunRecovery.load(_type, rec, parsers.bonusStatsScripts.get<ModScript::StunRecoveryStatBonus>());
	}
	_faceColorGroup = node["spriteFaceGroup"].as<int>(_faceColorGroup);
	_hairColorGroup = node["spriteHairGroup"].as<int>(_hairColorGroup);
	_rankColorGroup = node["spriteRankGroup"].as<int>(_rankColorGroup);
	_utileColorGroup = node["spriteUtileGroup"].as<int>(_utileColorGroup);
	mod->loadInts(_type, _faceColor, node["spriteFaceColor"]);
	mod->loadInts(_type, _hairColor, node["spriteHairColor"]);
	mod->loadInts(_type, _rankColor, node["spriteRankColor"]);
	mod->loadInts(_type, _utileColor, node["spriteUtileColor"]);

	_battleUnitScripts.load(_type, node, parsers.battleUnitScripts);

	mod->loadUnorderedNames(_type, _unitsNames, node["units"]);
	_scriptValues.load(node, parsers.getShared());
	mod->loadSpriteOffset(_type, _customArmorPreviewIndex, node["customArmorPreviewIndex"], "CustomArmorPreviews");
	loadTriBoolHelper(_allowsRunning, node["allowsRunning"]);
	loadTriBoolHelper(_allowsStrafing, node["allowsStrafing"]);
	loadTriBoolHelper(_allowsKneeling, node["allowsKneeling"]);
	_allowsMoving = node["allowsMoving"].as<bool>(_allowsMoving);
	_isPilotArmor = node["isPilotArmor"].as<bool>(_isPilotArmor);
	_allowTwoMainWeapons = node["allowTwoMainWeapons"].as<bool>(_allowTwoMainWeapons);
	_instantWoundRecovery = node["instantWoundRecovery"].as<bool>(_instantWoundRecovery);
	_standHeight = node["standHeight"].as<int>(_standHeight);
	_kneelHeight = node["kneelHeight"].as<int>(_kneelHeight);
	_floatHeight = node["floatHeight"].as<int>(_floatHeight);
}

/**
 * Cross link with other rules.
 */
void Armor::afterLoad(const Mod* mod)
{
	mod->verifySoundOffset(_type, _moveSound, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _deathSoundMale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _deathSoundFemale, "BATTLE.CAT");

	mod->verifySoundOffset(_type, _selectUnitSoundMale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _selectUnitSoundFemale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _startMovingSoundMale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _startMovingSoundFemale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _selectWeaponSoundMale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _selectWeaponSoundFemale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _annoyedSoundMale, "BATTLE.CAT");
	mod->verifySoundOffset(_type, _annoyedSoundFemale, "BATTLE.CAT");

	mod->verifySpriteOffset(_type, _customArmorPreviewIndex, "CustomArmorPreviews");


	mod->linkRule(_corpseBattle, _corpseBattleNames);
	mod->linkRule(_corpseGeo, _corpseGeoName);
	mod->linkRule(_builtInWeapons, _builtInWeaponsNames);
	mod->linkRule(_units, _unitsNames);
	mod->linkRule(_requires, _requiresName);
	if (_storeItemName == Armor::NONE)
	{
		_infiniteSupply = true;
	}
	mod->linkRule(_storeItem, _storeItemName); //special logic there: "STR_NONE" -> nullptr
	mod->linkRule(_specWeapon, _specWeaponName);


	{
		auto totalSize = (size_t)getTotalSize();

		mod->checkForSoftError(_corpseBattle.size() != totalSize, _type, "Number of battle corpse items for 'corpseBattle' does not match the armor size.", LOG_ERROR);
		mod->checkForSoftError(_loftempsSet.size() != totalSize, _type, "Number of defined templates for 'loftempsSet' or 'loftemps' does not match the armor size.", LOG_ERROR);

		auto s = mod->getVoxelData()->size() / 16;
		for (auto& lof : _loftempsSet)
		{
			mod->checkForSoftError((size_t)lof >= s, _type, "Value " + std::to_string(lof) + " in 'loftempsSet' or 'loftemps' is larger than number of avaiable templates.", LOG_ERROR);
		}
	}

	int numCorpse = 0;
	for (auto& c : _corpseBattle)
	{
		if (!c)
		{
			throw Exception("Battle corpse item(s) cannot be empty.");
		}

		if (!numCorpse++)
		{
			// only the first item needs to be a corpse item
			mod->checkForSoftError(c->getBattleType() != BT_CORPSE, _type, "The first battle corpse item must be of item type 'corpse' (battleType: 11)");
		}
		else
		{
			mod->checkForSoftError(c->isRecoverable(), _type, "Multiple recoverable battle corpse item(s)");
		}
	}
	if (!_corpseGeo)
	{
		throw Exception("Geo corpse item cannot be empty.");
	}

	// calcualte final surfaces used by layers
	if (!_layersDefaultPrefix.empty())
	{
		std::stringstream ss;
		for (auto& version : _layersDefinition)
		{
			int layerIndex = 0;
			for (auto& layerItem : version.second)
			{
				if (!layerItem.empty())
				{
					ss.str("");
					auto pre = _layersSpecificPrefix.find(layerIndex);
					if (pre != _layersSpecificPrefix.end())
					{
						ss << pre->second;
					}
					else
					{
						ss << _layersDefaultPrefix;
					}
					ss << "__" << layerIndex << "__" << layerItem;

					//override element in vector
					layerItem = ss.str();

					//check if surface is valid
					if (Options::lazyLoadResources == false)
					{
						//TODO: remove `const_cast`
						mod->checkForSoftError(const_cast<Mod*>(mod)->getSurface(layerItem, false) == nullptr, _type, "Missing surface definition for '" + layerItem + "'", LOG_ERROR);
					}
				}
				layerIndex++;
			}
			//clean unused layers
			Collections::removeIf(version.second, [](const std::string& s) { return s.empty(); });
			version.second.shrink_to_fit();
		}
	}

	Collections::sortVector(_units);
}



/**
 * Gets the custom name of the Ufopedia article related to this armor.
 * @return The ufopedia article name.
 */
const std::string& Armor::getUfopediaType() const
{
	if (!_ufopediaType.empty())
		return _ufopediaType;

	return _type;
}

/**
 * Returns the language string that names
 * this armor. Each armor has a unique name. Coveralls, Power Suit,...
 * @return The armor name.
 */
const std::string& Armor::getType() const
{
	return _type;
}

/**
 * Gets the unit's sprite sheet.
 * @return The sprite sheet name.
 */
std::string Armor::getSpriteSheet() const
{
	return _spriteSheet;
}

/**
 * Gets the unit's inventory sprite.
 * @return The inventory sprite name.
 */
std::string Armor::getSpriteInventory() const
{
	return _spriteInv;
}

/**
 * Gets the front armor level.
 * @return The front armor level.
 */
int Armor::getFrontArmor() const
{
	return _frontArmor;
}

/**
 * Gets the left side armor level.
 * @return The left side armor level.
 */
int Armor::getLeftSideArmor() const
{
	return _sideArmor + _leftArmorDiff;
}

/**
* Gets the right side armor level.
* @return The right side armor level.
*/
int Armor::getRightSideArmor() const
{
	return _sideArmor;
}

/**
 * Gets the rear armor level.
 * @return The rear armor level.
 */
int Armor::getRearArmor() const
{
	return _rearArmor;
}

/**
 * Gets the under armor level.
 * @return The under armor level.
 */
int Armor::getUnderArmor() const
{
	return _underArmor;
}

/**
 * Gets the armor level of part.
 * @param side Part of armor.
 * @return The armor level of part.
 */
int Armor::getArmor(UnitSide side) const
{
	switch (side)
	{
	case SIDE_FRONT:	return _frontArmor;
	case SIDE_LEFT:		return _sideArmor + _leftArmorDiff;
	case SIDE_RIGHT:	return _sideArmor;
	case SIDE_REAR:		return _rearArmor;
	case SIDE_UNDER:	return _underArmor;
	default: return 0;
	}
}


/**
 * Gets the corpse item used in the Geoscape.
 * @return The name of the corpse item.
 */
const RuleItem* Armor::getCorpseGeoscape() const
{
	return _corpseGeo;
}

/**
 * Gets the list of corpse items dropped by the unit
 * in the Battlescape (one per unit tile).
 * @return The list of corpse items.
 */
const std::vector<const RuleItem*> &Armor::getCorpseBattlescape() const
{
	return _corpseBattle;
}

/**
 * Gets the storage item needed to equip this.
 * Every soldier armor needs an item.
 * @return The name of the store item (STR_NONE for infinite armor).
 */
const RuleItem* Armor::getStoreItem() const
{
	return _storeItem;
}

/**
 * Gets the type of special weapon.
 * @return The name of the special weapon.
 */
const RuleItem* Armor::getSpecialWeapon() const
{
	return _specWeapon;
}

/**
 * Gets the research required to be able to equip this armor.
 * @return The name of the research topic.
 */
const RuleResearch* Armor::getRequiredResearch() const
{
	return _requires;
}

/**
 * Gets the drawing routine ID.
 * @return The drawing routine ID.
 */
int Armor::getDrawingRoutine() const
{
	return _drawingRoutine;
}

/**
 * Gets whether or not to draw bubbles (breathing animation).
 * @return True if breathing animation is enabled, false otherwise.
 */
bool Armor::drawBubbles() const
{
	return _drawBubbles;
}

/**
 * Gets the movement type of this armor.
 * Useful for determining whether the armor can fly.
 * @important: do not use this function outside the BattleUnit constructor,
 * unless you are SURE you know what you are doing.
 * for more information, see the BattleUnit constructor.
 * @return The movement type.
 */
MovementType Armor::getMovementType() const
{
	return _movementType;
}

/**
 * Gets the armor's special ability.
 * @return The armor's specab.
 */
int Armor::getSpecialAbility() const
{
	return (int)_specab;
}

/**
* Gets the armor's move sound.
* @return The id of the armor's move sound.
*/
int Armor::getMoveSound() const
{
	return _moveSound;
}

/**
 * Gets the size of the unit. Normally this is 1 (small) or 2 (big).
 * @return The unit's size.
 */
int Armor::getSize() const
{
	return _size;
}

/**
 * Gets the total size of the unit. Normally this is 1 for small or 4 for big.
 * @return The unit's size.
 */
int Armor::getTotalSize() const
{
	return _size * _size;
}
/**
 * Gets the damage modifier for a certain damage type.
 * @param dt The damageType.
 * @return The damage modifier 0->1.
 */
float Armor::getDamageModifier(ItemDamageType dt) const
{
	return _damageModifier[(int)dt];
}

const std::vector<float> Armor::getDamageModifiersRaw() const
{
	std::vector<float> result;
	for (int i = 0; i < DAMAGE_TYPES; i++)
	{
		result.push_back(_damageModifier[i]);
	}
	return result;
}

/** Gets the loftempSet.
 * @return The loftempsSet.
 */
const std::vector<int>& Armor::getLoftempsSet() const
{
	return _loftempsSet;
}

/**
  * Gets pointer to the armor's stats.
  * @return stats Pointer to the armor's stats.
  */
const UnitStats *Armor::getStats() const
{
	return &_stats;
}

/**
 * Gets unit psi defense.
 */
int Armor::getPsiDefence(const BattleUnit* unit) const
{
	return _psiDefence.getBonus(unit);
}

/**
 * Gets unit melee dodge chance.
 */
int Armor::getMeleeDodge(const BattleUnit* unit) const
{
	return _meleeDodge.getBonus(unit);
}

/**
 * Gets unit dodge penalty if hit from behind.
 */
float Armor::getMeleeDodgeBackPenalty() const
{
	return _meleeDodgeBackPenalty;
}

/**
 *  Gets unit TU recovery.
 */
int Armor::getTimeRecovery(const BattleUnit* unit, int externalBonuses) const
{
	return _timeRecovery.getBonus(unit, externalBonuses);
}

/**
 *  Gets unit Energy recovery.
 */
int Armor::getEnergyRecovery(const BattleUnit* unit, int externalBonuses) const
{
	return _energyRecovery.getBonus(unit, externalBonuses);
}

/**
 *  Gets unit Morale recovery.
 */
int Armor::getMoraleRecovery(const BattleUnit* unit, int externalBonuses) const
{
	return _moraleRecovery.getBonus(unit, externalBonuses);
}

/**
 *  Gets unit Health recovery.
 */
int Armor::getHealthRecovery(const BattleUnit* unit, int externalBonuses) const
{
	return _healthRecovery.getBonus(unit, externalBonuses);
}

/**
 *  Gets unit Mana recovery.
 */
int Armor::getManaRecovery(const BattleUnit* unit, int externalBonuses) const
{
	return _manaRecovery.getBonus(unit, externalBonuses);
}

/**
 *  Gets unit Stun recovery.
 */
int Armor::getStunRegeneration(const BattleUnit* unit, int externalBonuses) const
{
	return _stunRecovery.getBonus(unit, externalBonuses);
}

/**
 * Gets the armor's weight.
 * @return the weight of the armor.
 */
int Armor::getWeight() const
{
	return _weight;
}

/**
 * Gets number of death frames.
 * @return number of death frames.
 */
int Armor::getDeathFrames() const
{
	return _deathFrames;
}

/**
 * Gets if armor uses constant animation.
 * @return if it uses constant animation
 */
bool Armor::getConstantAnimation() const
{
	return _constantAnimation;
}

/**
 * Checks if this armor ignores gender (power suit/flying suit).
 * @return which torso to force on the sprite.
 */
ForcedTorso Armor::getForcedTorso() const
{
	return _forcedTorso;
}

/**
 * What weapons does this armor have built in?
 * this is a vector of strings representing any
 * weapons that may be inherent to this armor.
 * note: unlike "livingWeapon" this is used in ADDITION to
 * any loadout or living weapon item that may be defined.
 * @return list of weapons that are integral to this armor.
 */
const std::vector<const RuleItem*> &Armor::getBuiltInWeapons() const
{
	return _builtInWeapons;
}

/**
 * Gets max view distance at dark in BattleScape.
 * @return The distance to see at dark.
 */
int Armor::getVisibilityAtDark() const
{
	return _visibilityAtDark;
}

/**
* Gets max view distance at day in BattleScape.
* @return The distance to see at day.
*/
int Armor::getVisibilityAtDay() const
{
	return _visibilityAtDay;
}

/**
* Gets info about camouflage at day.
* @return The vision distance modifier.
*/
int Armor::getCamouflageAtDay() const
{
	return _camouflageAtDay;
}

/**
* Gets info about camouflage at dark.
* @return The vision distance modifier.
*/
int Armor::getCamouflageAtDark() const
{
	return _camouflageAtDark;
}

/**
* Gets info about anti camouflage at day.
* @return The vision distance modifier.
*/
int Armor::getAntiCamouflageAtDay() const
{
	return _antiCamouflageAtDay;
}

/**
* Gets info about anti camouflage at dark.
* @return The vision distance modifier.
*/
int Armor::getAntiCamouflageAtDark() const
{
	return _antiCamouflageAtDark;
}

/**
* Gets info about heat vision.
* @return How much smoke is ignored, in percent.
*/
int Armor::getHeatVision() const
{
	return _heatVision;
}

/**
* Gets info about psi vision.
* @return How many tiles can units be sensed even through solid obstacles (e.g. walls).
*/
int Armor::getPsiVision() const
{
	return _psiVision;
}

/**
 * Gets info about psi camouflage.
 * @return psi camo data.
 */
int Armor::getPsiCamouflage() const
{
	return _psiCamouflage;
}

/**
* Gets personal light radius created by solders.
* @return Return light radius.
*/
int Armor::getPersonalLight() const
{
	return _personalLight;
}

/**
 * Gets how armor react to fear.
 * @param def Default value.
 * @return Can ignored fear?
 */
bool Armor::getFearImmune(bool def) const
{
	return useTriBoolHelper(_fearImmune, def);
}

/**
 * Gets how armor react to bleeding.
 * @param def Default value.
 * @return Can ignore bleed?
 */
bool Armor::getBleedImmune(bool def) const
{
	return useTriBoolHelper(_bleedImmune, def);
}

/**
 * Gets how armor react to inflicted pain.
 * @param def
 * @return Can ignore pain?
 */
bool Armor::getPainImmune(bool def) const
{
	return useTriBoolHelper(_painImmune, def);
}

/**
 * Gets how armor react to zombification.
 * @param def Default value.
 * @return Can't be turn to zombie?
 */
bool Armor::getZombiImmune(bool def) const
{
	return useTriBoolHelper(_zombiImmune, def);
}

/**
 * Gets whether or not this unit ignores close quarters threats.
 * @param def Default value.
 * @return Ignores CQB check?
 */
bool Armor::getIgnoresMeleeThreat(bool def) const
{
	return useTriBoolHelper(_ignoresMeleeThreat, def);
}

/**
 * Gets whether or not this unit is a close quarters threat.
 * @param def Default value.
 * @return Creates CQB check for others?
 */
bool Armor::getCreatesMeleeThreat(bool def) const
{
	return useTriBoolHelper(_createsMeleeThreat, def);
}

/**
 * Gets how much damage (over the maximum HP) is needed to vaporize/disintegrate a unit.
 * @return Percent of require hp.
 */
float Armor::getOverKill() const
{
	return _overKill;
}

/**
 * Gets hair base color group for replacement, if 0 then don't replace colors.
 * @return Color group or 0.
 */
int Armor::getFaceColorGroup() const
{
	return _faceColorGroup;
}

/**
 * Gets hair base color group for replacement, if 0 then don't replace colors.
 * @return Color group or 0.
 */
int Armor::getHairColorGroup() const
{
	return _hairColorGroup;
}

/**
 * Gets utile base color group for replacement, if 0 then don't replace colors.
 * @return Color group or 0.
 */
int Armor::getUtileColorGroup() const
{
	return _utileColorGroup;
}

/**
 * Gets rank base color group for replacement, if 0 then don't replace colors.
 * @return Color group or 0.
 */
int Armor::getRankColorGroup() const
{
	return _rankColorGroup;
}

namespace
{

/**
 * Helper function finding value in vector with fallback if vector is shorter.
 * @param vec Vector with values we try get.
 * @param pos Position in vector that can be greater than size of vector.
 * @return Value in vector.
 */
int findWithFallback(const std::vector<int> &vec, size_t pos)
{
	//if pos == 31 then we test for 31, 15, 7
	//if pos == 36 then we test for 36, 4
	//we stop on p < 8 for compatibility reasons.
	for (int i = 0; i <= RuleSoldier::LookVariantBits; ++i)
	{
		size_t p = (pos & (RuleSoldier::LookTotalMask >> i));
		if (p < vec.size())
		{
			return vec[p];
		}
	}
	return 0;
}

} //namespace

/**
 * Gets new face colors for replacement, if 0 then don't replace colors.
 * @return Color index or 0.
 */
int Armor::getFaceColor(int i) const
{
	return findWithFallback(_faceColor, i);
}

/**
 * Gets new hair colors for replacement, if 0 then don't replace colors.
 * @return Color index or 0.
 */
int Armor::getHairColor(int i) const
{
	return findWithFallback(_hairColor, i);
}

/**
 * Gets new utile colors for replacement, if 0 then don't replace colors.
 * @return Color index or 0.
 */
int Armor::getUtileColor(int i) const
{
	return findWithFallback(_utileColor, i);
}

/**
 * Gets new rank colors for replacement, if 0 then don't replace colors.
 * @return Color index or 0.
 */
int Armor::getRankColor(int i) const
{
	return findWithFallback(_rankColor, i);
}

/**
 * Can this unit's inventory be accessed for any reason?
 * @return if we can access the inventory.
 */
bool Armor::hasInventory() const
{
	return _hasInventory;
}

/**
* Gets the list of units this armor applies to.
* @return The list of unit IDs (empty = applies to all).
*/
const std::vector<const RuleSoldier*> &Armor::getUnits() const
{
	return _units;
}

/**
 * Check if a soldier can use this armor.
 */
bool Armor::getCanBeUsedBy(const RuleSoldier* soldier) const
{
	return _units.empty() || Collections::sortVectorHave(_units, soldier);
}

/**
 * Gets the index of the sprite in the CustomArmorPreview sprite set.
 * @return Sprite index.
 */
const std::vector<int> &Armor::getCustomArmorPreviewIndex() const
{
	return _customArmorPreviewIndex;
}

/**
 * Can you run while wearing this armor?
 * @return True if you are allowed to run.
 */
bool Armor::allowsRunning(bool def) const
{
	return useTriBoolHelper(_allowsRunning, def);
}

/**
 * Can you strafe while wearing this armor?
 * @return True if you are allowed to strafe.
 */
bool Armor::allowsStrafing(bool def) const
{
	return useTriBoolHelper(_allowsStrafing, def);
}

/**
 * Can you kneel while wearing this armor?
 * @return True if you are allowed to kneel.
 */
bool Armor::allowsKneeling(bool def) const
{
	return useTriBoolHelper(_allowsKneeling, def);
}

/**
 * Can you move while wearing this armor?
 * @return True if you are allowed to move.
 */
bool Armor::allowsMoving() const
{
	return _allowsMoving;
}

/**
 * Does this armor instantly recover any wounds after the battle?
 * @return True if soldier should not get any recovery time.
 */
bool Armor::getInstantWoundRecovery() const
{
	return _instantWoundRecovery;
}

/**
 * Returns a unit's height at standing in this armor.
 * @return The unit's height.
 */
int Armor::getStandHeight() const
{
	return _standHeight;
}

/**
 * Returns a unit's height at kneeling in this armor.
 * @return The unit's kneeling height.
 */
int Armor::getKneelHeight() const
{
	return _kneelHeight;
}

/**
 * Returns a unit's floating elevation in this armor.
 * @return The unit's floating height.
 */
int Armor::getFloatHeight() const
{
	return _floatHeight;
}


////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

void getTypeScript(const Armor* r, ScriptText& txt)
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

void getArmorValueScript(const Armor *ar, int &ret, int side)
{
	if (ar && 0 <= side && side < SIDE_MAX)
	{
		ret = ar->getArmor((UnitSide)side);
		return;
	}
	ret = 0;
}

std::string debugDisplayScript(const Armor* ar)
{
	if (ar)
	{
		std::string s;
		s += Armor::ScriptName;
		s += "(name: \"";
		s += ar->getType();
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
 * Register Armor in script parser.
 * @param parser Script parser.
 */
void Armor::ScriptRegister(ScriptParserBase* parser)
{
	Bind<Armor> ar = { parser };

	ar.addCustomConst("SIDE_FRONT", SIDE_FRONT);
	ar.addCustomConst("SIDE_LEFT", SIDE_LEFT);
	ar.addCustomConst("SIDE_RIGHT", SIDE_RIGHT);
	ar.addCustomConst("SIDE_REAR", SIDE_REAR);
	ar.addCustomConst("SIDE_UNDER", SIDE_UNDER);

	ar.add<&getTypeScript>("getType");

	ar.add<&Armor::getDrawingRoutine>("getDrawingRoutine");
	ar.add<&Armor::getVisibilityAtDark>("getVisibilityAtDark");
	ar.add<&Armor::getVisibilityAtDay>("getVisibilityAtDay");
	ar.add<&Armor::getPersonalLight>("getPersonalLight");
	ar.add<&Armor::getSize>("getSize");

	UnitStats::addGetStatsScript<&Armor::_stats>(ar, "Stats.");

	ar.add<&getArmorValueScript>("getArmor");

	ar.addScriptValue<BindBase::OnlyGet, &Armor::_scriptValues>();
	ar.addDebugDisplay<&debugDisplayScript>();
}

}
