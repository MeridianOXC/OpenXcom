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
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "MapData.h"
#include "Unit.h"
#include "RuleStatBonus.h"
#include "RuleDamageType.h"
#include "ModScript.h"

namespace OpenXcom
{

enum ForcedTorso : Uint8 { TORSO_USE_GENDER, TORSO_ALWAYS_MALE, TORSO_ALWAYS_FEMALE };
enum UnitSide : Uint8 { SIDE_FRONT, SIDE_LEFT, SIDE_RIGHT, SIDE_REAR, SIDE_UNDER, SIDE_MAX };

class BattleUnit;

/**
 * Represents a specific type of armor.
 * Not only soldier armor, but also alien armor - some alien races wear
 * Soldier Armor, Leader Armor or Commander Armor depending on their rank.
 */
class Armor
{
public:

	/// Name of class used in script.
	static constexpr const char *ScriptName = "RuleArmor";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	static const std::string NONE;
private:
	std::string _type, _spriteSheet, _spriteInv, _corpseGeo, _storeItem, _specWeapon;
	std::string _layersDefaultPrefix;
	std::map<int, std::string> _layersSpecificPrefix;
	std::map<std::string, std::vector<std::string> > _layersDefinition;
	std::vector<std::string> _corpseBattle;
	std::vector<std::string> _builtInWeapons;
	int _frontArmor, _sideArmor, _leftArmorDiff, _rearArmor, _underArmor, _drawingRoutine;
	MovementType _movementType;
	int _moveSound;
	int _size, _weight, _visibilityAtDark, _visibilityAtDay, _personalLight;
	int _camouflageAtDay, _camouflageAtDark, _antiCamouflageAtDay, _antiCamouflageAtDark, _heatVision, _psiVision;
	float _damageModifier[DAMAGE_TYPES];
	std::vector<int> _loftempsSet;
	UnitStats _stats;
	int _deathFrames;
	bool _constantAnimation, _canHoldWeapon, _hasInventory;
	ForcedTorso _forcedTorso;
	int _faceColorGroup, _hairColorGroup, _utileColorGroup, _rankColorGroup;
	std::vector<int> _faceColor, _hairColor, _utileColor, _rankColor;
	int _fearImmune, _bleedImmune, _painImmune, _zombiImmune;
	int _ignoresMeleeThreat, _createsMeleeThreat;
	float _overKill, _meleeDodgeBackPenalty;
	RuleStatBonus _psiDefence, _meleeDodge;
	RuleStatBonus _timeRecovery, _energyRecovery, _moraleRecovery, _healthRecovery, _stunRecovery;
	ModScript::BattleUnitScripts::Container _battleUnitScripts;

	ScriptValues<Armor> _scriptValues;
	std::vector<int> _customArmorPreviewIndex;
	bool _allowsRunning, _allowsStrafing, _allowsKneeling, _allowsMoving;
	bool _instantWoundRecovery;
	int _standHeight, _kneelHeight, _floatHeight;
	std::vector<std::string> _units, _inventorySlots;
	std::map<std::string, std::string> _defaultInventoriesMap;
	bool _inventoryOverlapsPaperdoll;
public:
	/// Creates a blank armor ruleset.
	Armor(const std::string &type);
	/// Cleans up the armor ruleset.
	~Armor();
	/// Cross link with other rules.
	void afterLoad(const Mod* mod);
	/// Loads the armor data from YAML.
	void load(const YAML::Node& node, const ModScript& parsers, Mod *mod);
	/// Gets the armor's type.
	std::string getType() const;
	/// Gets the unit's sprite sheet.
	std::string getSpriteSheet() const;
	/// Gets the unit's inventory sprite.
	std::string getSpriteInventory() const;
	/// Gets the front armor level.
	int getFrontArmor() const;
	/// Gets the left side armor level.
	int getLeftSideArmor() const;
	/// Gets the right side armor level.
	int getRightSideArmor() const;
	/// Gets the rear armor level.
	int getRearArmor() const;
	/// Gets the under armor level.
	int getUnderArmor() const;
	/// Gets the armor level of armor side.
	int getArmor(UnitSide side) const;
	/// Gets the Geoscape corpse item.
	std::string getCorpseGeoscape() const;
	/// Gets the Battlescape corpse item.
	const std::vector<std::string> &getCorpseBattlescape() const;
	/// Gets the stores item.
	std::string getStoreItem() const;
	/// Gets the special weapon type.
	std::string getSpecialWeapon() const;

	/// Gets the default prefix for layered armor sprite names.
	const std::string &getLayersDefaultPrefix() const { return _layersDefaultPrefix; }
	/// Gets the overrides for layered armor sprite name prefix, per layer.
	const std::map<int, std::string> &getLayersSpecificPrefix() const { return _layersSpecificPrefix; }
	/// Gets the layered armor definition.
	const std::map<std::string, std::vector<std::string> > &getLayersDefinition() const { return _layersDefinition; }

	/// Gets the battlescape drawing routine ID.
	int getDrawingRoutine() const;
	/// DO NOT USE THIS FUNCTION OUTSIDE THE BATTLEUNIT CONSTRUCTOR OR I WILL HUNT YOU DOWN.
	MovementType getMovementType() const;
	/// Gets the move sound id. Overrides default/unit's move sound. To be used in BattleUnit constructors only too!
	int getMoveSound() const;
	/// Gets whether this is a normal or big unit.
	int getSize() const;
	/// Gets how big space armor ocupy in craft.
	int getTotalSize() const;
	/// Gets damage modifier.
	float getDamageModifier(ItemDamageType dt) const;
	const std::vector<float> getDamageModifiersRaw() const;
	/// Gets loftempSet
	const std::vector<int> &getLoftempsSet() const;
	/// Gets the armor's stats.
	const UnitStats *getStats() const;
	/// Gets unit psi defense.
	int getPsiDefence(const BattleUnit* unit) const;
	const RuleStatBonus *getPsiDefenceRaw() const { return &_psiDefence; }
	/// Gets unit melee dodge chance.
	int getMeleeDodge(const BattleUnit* unit) const;
	const RuleStatBonus *getMeleeDodgeRaw() const { return &_meleeDodge; }
	/// Gets unit dodge penalty if hit from behind.
	float getMeleeDodgeBackPenalty() const;

	/// Gets unit TU recovery.
	int getTimeRecovery(const BattleUnit* unit) const;
	const RuleStatBonus *getTimeRecoveryRaw() const { return &_timeRecovery; }
	/// Gets unit Energy recovery.
	int getEnergyRecovery(const BattleUnit* unit) const;
	const RuleStatBonus *getEnergyRecoveryRaw() const { return &_energyRecovery; }
	/// Gets unit Morale recovery.
	int getMoraleRecovery(const BattleUnit* unit) const;
	const RuleStatBonus *getMoraleRecoveryRaw() const { return &_moraleRecovery; }
	/// Gets unit Health recovery.
	int getHealthRecovery(const BattleUnit* unit) const;
	const RuleStatBonus *getHealthRecoveryRaw() const { return &_healthRecovery; }
	/// Gets unit Stun recovery.
	int getStunRegeneration(const BattleUnit* unit) const;
	const RuleStatBonus *getStunRegenerationRaw() const { return &_stunRecovery; }

	/// Gets the armor's weight.
	int getWeight() const;
	/// Gets number of death frames.
	int getDeathFrames() const;
	/// Gets if armor uses constant animation.
	bool getConstantAnimation() const;
	/// Gets if armor can hold weapon.
	bool getCanHoldWeapon() const;
	/// Checks if this armor ignores gender (power suit/flying suit).
	ForcedTorso getForcedTorso() const;
	/// Gets buildin weapons of armor.
	const std::vector<std::string> &getBuiltInWeapons() const;
	/// Gets max view distance at dark in BattleScape.
	int getVisibilityAtDark() const;
	/// Gets max view distance at day in BattleScape.
	int getVisibilityAtDay() const;
	/// Gets info about camouflage at day.
	int getCamouflageAtDay() const;
	/// Gets info about camouflage at dark.
	int getCamouflageAtDark() const;
	/// Gets info about anti camouflage at day.
	int getAntiCamouflageAtDay() const;
	/// Gets info about anti camouflage at dark.
	int getAntiCamouflageAtDark() const;
	/// Gets info about heat vision.
	int getHeatVision() const;
	/// Gets info about psi vision.
	int getPsiVision() const;
	/// Gets personal ligth radius;
	int getPersonalLight() const;
	/// Gets how armor react to fear.
	bool getFearImmune(bool def = false) const;
	/// Gets how armor react to bleeding.
	bool getBleedImmune(bool def = false) const;
	/// Gets how armor react to inflicted pain.
	bool getPainImmune(bool def = false) const;
	/// Gets how armor react to zombification.
	bool getZombiImmune(bool def = false) const;
	/// Gets whether or not this unit ignores close quarters threats.
	bool getIgnoresMeleeThreat(bool def = false) const;
	/// Gets whether or not this unit is a close quarters threat.
	bool getCreatesMeleeThreat(bool def = true) const;
	/// Gets how much negative hp is require to gib unit.
	float getOverKill() const;
	/// Get face base color
	int getFaceColorGroup() const;
	/// Get hair base color
	int getHairColorGroup() const;
	/// Get utile base color
	int getUtileColorGroup() const;
	/// Get rank base color
	int getRankColorGroup() const;
	/// Get face base color
	int getFaceColor(int i) const;
	const std::vector<int> &getFaceColorRaw() const { return _faceColor; }
	/// Get hair base color
	int getHairColor(int i) const;
	const std::vector<int> &getHairColorRaw() const { return _hairColor; }
	/// Get utile base color
	int getUtileColor(int i) const;
	const std::vector<int> &getUtileColorRaw() const { return _utileColor; }
	/// Get rank base color
	int getRankColor(int i) const;
	const std::vector<int> &getRankColorRaw() const { return _rankColor; }
	/// Can we access this unit's inventory?
	bool hasInventory() const;
	/// Gets script.
	template<typename Script>
	const typename Script::Container &getScript() const { return _battleUnitScripts.get<Script>(); }
	const ScriptValues<Armor> &getScriptValuesRaw() const { return _scriptValues; }
	/// Gets the armor's units.
	const std::vector<std::string> &getUnits() const;
	/// Gets the index of the sprite in the CustomArmorPreview sprite set
	const std::vector<int> &getCustomArmorPreviewIndex() const;
	/// Can you run while wearing this armor?
	bool allowsRunning() const;
	/// Can you strafe while wearing this armor?
	bool allowsStrafing() const;
	/// Can you kneel while wearing this armor?
	bool allowsKneeling() const;
	/// Can you move while wearing this armor?
	bool allowsMoving() const;
	/// Does this armor instantly recover any wounds after the battle?
	bool getInstantWoundRecovery() const;
	/// Gets a unit's height when standing while wearing this armor.
	int getStandHeight() const;
	/// Gets a unit's height when kneeling while wearing this armor.
	int getKneelHeight() const;
	/// Gets a unit's float elevation while wearing this armor.
	int getFloatHeight() const;
	/// Gets the armor's inventory slots.
	const std::vector<std::string> &getInventorySlots() const;
	/// Gets the default inventory rules mapping
	std::string getDefaultInventoryMap(std::string s) const;
	/// Inventory overlaps paperdoll
	bool inventoryOverlapsPaperdoll() const;
};

}
