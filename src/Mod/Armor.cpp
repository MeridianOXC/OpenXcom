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
#include "../Engine/ScriptBind.h"
#include "Mod.h"
#include "RuleSoldier.h"

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
	_type(type), _frontArmor(0), _sideArmor(0), _leftArmorDiff(0), _rearArmor(0), _underArmor(0),
	_drawingRoutine(0), _drawBubbles(false), _movementType(MT_WALK), _moveSound(-1), _size(1), _weight(0),
	_visibilityAtDark(0), _visibilityAtDay(0), _personalLight(15),
	_camouflageAtDay(0), _camouflageAtDark(0), _antiCamouflageAtDay(0), _antiCamouflageAtDark(0), _heatVision(0), _psiVision(0),
	_deathFrames(3), _constantAnimation(false), _canHoldWeapon(false), _hasInventory(true), _forcedTorso(TORSO_USE_GENDER),
	_faceColorGroup(0), _hairColorGroup(0), _utileColorGroup(0), _rankColorGroup(0),
	_fearImmune(defTriBool), _bleedImmune(defTriBool), _painImmune(defTriBool), _zombiImmune(defTriBool),
	_ignoresMeleeThreat(defTriBool), _createsMeleeThreat(defTriBool),
	_overKill(0.5f), _meleeDodgeBackPenalty(0),
	_allowsRunning(defTriBool), _allowsStrafing(defTriBool), _allowsKneeling(defTriBool), _allowsMoving(1),
	_instantWoundRecovery(false),
	_standHeight(-1), _kneelHeight(-1), _floatHeight(-1)
{
	for (int i=0; i < DAMAGE_TYPES; i++)
		_damageModifier[i] = 1.0f;

	_psiDefence.setPsiDefense();
	_timeRecovery.setTimeRecovery();
	_energyRecovery.setEnergyRecovery();
	_stunRecovery.setStunRecovery();

	_customArmorPreviewIndex.push_back(0);
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
	_type = node["type"].as<std::string>(_type);
	_spriteSheet = node["spriteSheet"].as<std::string>(_spriteSheet);
	_spriteInv = node["spriteInv"].as<std::string>(_spriteInv);
	_hasInventory = node["allowInv"].as<bool>(_hasInventory);
	if (node["corpseItem"])
	{
		_corpseBattle.clear();
		_corpseBattle.push_back(node["corpseItem"].as<std::string>());
		_corpseGeo = _corpseBattle[0];
	}
	else if (node["corpseBattle"])
	{
		_corpseBattle = node["corpseBattle"].as< std::vector<std::string> >();
		_corpseGeo = _corpseBattle[0];
	}
	_builtInWeapons = node["builtInWeapons"].as<std::vector<std::string> >(_builtInWeapons);
	_corpseGeo = node["corpseGeo"].as<std::string>(_corpseGeo);
	_storeItem = node["storeItem"].as<std::string>(_storeItem);
	_specWeapon = node["specialWeapon"].as<std::string>(_specWeapon);
	_requires = node["requires"].as<std::string>(_requires);

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
	_stats.merge(node["stats"].as<UnitStats>(_stats));
	if (const YAML::Node &dmg = node["damageModifier"])
	{
		for (size_t i = 0; i < dmg.size() && i < (size_t)DAMAGE_TYPES; ++i)
		{
			_damageModifier[i] = dmg[i].as<float>();
		}
	}
	_loftempsSet = node["loftempsSet"].as< std::vector<int> >(_loftempsSet);
	if (node["loftemps"])
		_loftempsSet.push_back(node["loftemps"].as<int>());
	_deathFrames = node["deathFrames"].as<int>(_deathFrames);
	_constantAnimation = node["constantAnimation"].as<bool>(_constantAnimation);
	_forcedTorso = (ForcedTorso)node["forcedTorso"].as<int>(_forcedTorso);
	_canHoldWeapon =
		(_drawingRoutine == 0 ||
		_drawingRoutine == 1 ||
		_drawingRoutine == 4 ||
		_drawingRoutine == 6 ||
		_drawingRoutine == 10 ||
		_drawingRoutine == 13 ||
		_drawingRoutine == 14 ||
		_drawingRoutine == 15 ||
		_drawingRoutine == 17 ||
		_drawingRoutine == 18);

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
	_faceColor = node["spriteFaceColor"].as<std::vector<int> >(_faceColor);
	_hairColor = node["spriteHairColor"].as<std::vector<int> >(_hairColor);
	_rankColor = node["spriteRankColor"].as<std::vector<int> >(_rankColor);
	_utileColor = node["spriteUtileColor"].as<std::vector<int> >(_utileColor);

	_battleUnitScripts.load(_type, node, parsers.battleUnitScripts);

	_units = node["units"].as< std::vector<std::string> >(_units);
	_scriptValues.load(node, parsers.getShared());
	mod->loadSpriteOffset(_type, _customArmorPreviewIndex, node["customArmorPreviewIndex"], "CustomArmorPreviews");
	loadTriBoolHelper(_allowsRunning, node["allowsRunning"]);
	loadTriBoolHelper(_allowsStrafing, node["allowsStrafing"]);
	loadTriBoolHelper(_allowsKneeling, node["allowsKneeling"]);
	_allowsMoving = node["allowsMoving"].as<bool>(_allowsMoving);
	_instantWoundRecovery = node["instantWoundRecovery"].as<bool>(_instantWoundRecovery);
	_standHeight = node["standHeight"].as<int>(_standHeight);
	_kneelHeight = node["kneelHeight"].as<int>(_kneelHeight);
	_floatHeight = node["floatHeight"].as<int>(_floatHeight);
	if (const YAML::Node& tst = node["inventorySlots"])
		_inventorySlots = tst.as< std::vector<std::string> >(_inventorySlots);
}

/**
 * Returns the language string that names
 * this armor. Each armor has a unique name. Coveralls, Power Suit,...
 * @return The armor name.
 */
std::string Armor::getType() const
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
std::string Armor::getCorpseGeoscape() const
{
	return _corpseGeo;
}

/**
 * Gets the list of corpse items dropped by the unit
 * in the Battlescape (one per unit tile).
 * @return The list of corpse items.
 */
const std::vector<std::string> &Armor::getCorpseBattlescape() const
{
	return _corpseBattle;
}

/**
 * Gets the storage item needed to equip this.
 * Every soldier armor needs an item.
 * @return The name of the store item (STR_NONE for infinite armor).
 */
std::string Armor::getStoreItem() const
{
	return _storeItem;
}

/**
 * Gets the type of special weapon.
 * @return The name of the special weapon.
 */
std::string Armor::getSpecialWeapon() const
{
	return _specWeapon;
}

/**
 * Gets the research required to be able to equip this armor.
 * @return The name of the research topic.
 */
const std::string &Armor::getRequiredResearch() const
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
 * Gets if armor can hold weapon.
 * @return if it can hold weapon
 */
bool Armor::getCanHoldWeapon() const
{
	return _canHoldWeapon;
}

/*
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
const std::vector<std::string> &Armor::getBuiltInWeapons() const
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
const std::vector<std::string> &Armor::getUnits() const
{
	return _units;
}

namespace
{

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

}

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

	ar.add<&Armor::getDrawingRoutine>("getDrawingRoutine");
	ar.add<&Armor::getVisibilityAtDark>("getVisibilityAtDark");
	ar.add<&Armor::getVisibilityAtDay>("getVisibilityAtDay");
	ar.add<&Armor::getPersonalLight>("getPersonalLight");
	ar.add<&Armor::getSize>("getSize");

	UnitStats::addGetStatsScript<&Armor::_stats>(ar, "Stats.");

	ar.add<&getArmorValueScript>("getArmor");

	ar.addScriptValue<&Armor::_scriptValues>(false);
	ar.addDebugDisplay<&debugDisplayScript>();
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

/**
* Cross link with other Rules.
* computes if the slots overlaps the paperdoll
* sets the armor's real inventory slots SORTED wrt
*  * transfer for usage speed (min (move to RH, move to LH, if any))
*  * pick up speed (move from ground, if any)
*/
void Armor::afterLoad(const Mod* mod)
{
	// find out if paperdoll overlaps with inventory slots
	const int x1 = RuleInventory::PAPERDOLL_X, y1 = RuleInventory::PAPERDOLL_Y, w1 = RuleInventory::PAPERDOLL_W, h1 = RuleInventory::PAPERDOLL_H;
	std::vector<RuleInventory*> realInvs;
	if (! mod->getExplicitInventoriesByArmor())
	{
		_inventorySlots.clear();
		_inventorySlots = mod->getInvsList();
	}
	for (auto inventory : _inventorySlots)
	{
		RuleInventory* invCategory = mod->getInventory(inventory);
		if (invCategory->isRightHand())
			_rhs = invCategory;
		else if (invCategory->isLeftHand())
			_lhs = invCategory;
		else if (invCategory->getType() == INV_GROUND)
			_gnd = invCategory;
		else realInvs.push_back(invCategory);
		for (auto invSlot : *invCategory->getSlots())
		{
			int x2 = invCategory->getX() + (invSlot.x * RuleInventory::SLOT_W);
			int y2 = invCategory->getY() + (invSlot.y * RuleInventory::SLOT_H);
			int w2 = RuleInventory::SLOT_W;
			int h2 = RuleInventory::SLOT_H;
			if (x1 + w1 < x2 || x2 + w2 < x1 || y1 + h1 < y2 || y2 + h2 < y1)
			{
				// intersection is empty
			}
			else
			{
				_inventoryOverlapsPaperdoll = true;
			}
		}
	}
	for (auto inv : realInvs)
	{
		std::vector<RuleInventory*>::iterator pos = _invRules.end();
		for (std::vector<RuleInventory*>::iterator i = _invRules.begin(); i != _invRules.end(); i++)
		{
			if (compCost(inv, *i))
			{
				pos = i;
				break;
			}
		}
		_invRules.insert(pos, inv);
	}
	if (!_lhs || !_rhs)
		throw Exception("Armor " + _type + " hasn't two hands!!!!");
	_invRules.insert(_invRules.begin(), _lhs);
	_invRules.insert(_invRules.begin(), _rhs);
	if (!_gnd)
		_gnd = mod->getInventory("STR_GROUND");
	_invRules.push_back(_gnd);
}

/**
* Is the coordinate in an inventory slot.
* @return true if inside an inventory slot.
*/
bool Armor::isCoordInInventory(int x, int y) const
{
	if (_inventoryOverlapsPaperdoll)
	{
		for (auto inventory : _invRules)
		{
			for (std::vector<struct RuleSlot>::const_iterator invSlot = inventory->getSlots()->cbegin(); invSlot != inventory->getSlots()->cend(); invSlot++)
			{
				int x1 = inventory->getX() + (invSlot->x * RuleInventory::SLOT_W);
				int y1 = inventory->getY() + (invSlot->y * RuleInventory::SLOT_H);
				if (x1 <= x && x <= x1 + RuleInventory::SLOT_W && y1 <= y && y <= y1 + RuleInventory::SLOT_H)
				{
					return true;
				}
			}
		}

	}
	return false;
}

bool Armor::compCost(const RuleInventory* lop, const RuleInventory* rop) const
{
	int lch = std::min(lop->getCost(_rhs), lop->getCost(_lhs)), rch = std::min(rop->getCost(_rhs), rop->getCost(_lhs)), lcg = lop->getCost(_gnd), rcg = rop->getCost(_gnd);
	return (lch < rch) || (lch == rch) && (lcg <= rcg);
}
}
