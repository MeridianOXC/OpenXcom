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
#include "UnitSprite.h"
#include "../Engine/SurfaceSet.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Armor.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/Mod.h"
#include "../Engine/Exception.h"

namespace OpenXcom
{

/**
 * Sets up a UnitSprite with the specified size and position.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
UnitSprite::UnitSprite(Surface* dest, const Mod* mod, const SavedBattleGame* save, int frame, bool helmet) :
	_unit(0), _itemR(0), _itemL(0),
	_unitSurface(0),
	_itemSurface(const_cast<Mod*>(mod)->getSurfaceSet("HANDOB.PCK")),
	_fireSurface(const_cast<Mod*>(mod)->getSurfaceSet("SMOKE.PCK")),
	_breathSurface(const_cast<Mod*>(mod)->getSurfaceSet("BREATH-1.PCK", false)),
	_facingArrowSurface(const_cast<Mod*>(mod)->getSurfaceSet("DETBLOB.DAT")),
	_dest(dest), _save(save), _mod(mod),
	_part(0), _animationFrame(frame), _drawingRoutine(0),
	_helmet(helmet),
	_x(0), _y(0), _shade(0), _burn(0),
	_mask(0, 0)
{

}

/**
 * Deletes the UnitSprite.
 */
UnitSprite::~UnitSprite()
{

}

namespace
{

/**
 * Placeholder for surface index for body part not used by normal OpenXcom.
 */
const int InvalidSpriteIndex = -256;

/**
 * Get item if can be visible on sprite.
 */
const BattleItem *getIfVisible(const BattleItem *item)
{
	if (item && (!item->getRules()->isFixed() || item->getRules()->getFixedShow()))
	{
		return item;
	}
	return 0;
}

} //namespace

/**
 * Get item sprite for item.
 * @param item item what we want draw.
 * @return Graphic part.
 */
void UnitSprite::selectItem(Part& p, const BattleItem *item, int dir)
{
	const auto* rule = item->getRules();
	auto index = item->getRules()->getHandSprite();

	//enforce compatibility with basic version
	if (!_itemSurface->getFrame(index + dir))
	{
		throw Exception("Frame(s) missing in 'HANDOB.PCK' for item '" + item->getRules()->getName() + "'");
	}

	auto result = ModScript::scriptFunc2<ModScript::SelectItemSprite>(
		rule,
		index, dir,
		item, _save, p.bodyPart, _animationFrame, _shade
	);

	p.src = _itemSurface->getFrame(result);
}

/**
 * Get item sprite for unit body part.
 * @param index index of item sprite.
 * @return Graphic part.
 */
void UnitSprite::selectUnit(Part& p, int index, int dir)
{
	const auto* armor = _unit->getArmor();

	//enforce compatibility with basic version
	if (InvalidSpriteIndex != index && !_unitSurface->getFrame(index + dir))
	{
		throw Exception("Frame(s) missing in '" + armor->getSpriteSheet() + "' for armor '" + armor->getType() + "'");
	}

	auto result = ModScript::scriptFunc2<ModScript::SelectUnitSprite>(
		armor,
		index, dir,
		_unit, _save, p.bodyPart, _animationFrame, _shade
	);

	p.src = _unitSurface->getFrame(result);
}

/**
 * Blit item sprite onto surface.
 * @param item item sprite, can be null.
 */
void UnitSprite::blitItem(Part& item)
{
	if (!item.src)
	{
		return;
	}
	ScriptWorkerBlit work;
	BattleItem::ScriptFill(&work, (item.bodyPart == BODYPART_ITEM_RIGHTHAND ? _itemR : _itemL), _save, item.bodyPart, _animationFrame, _shade);

	_dest->lock();

	work.executeBlit(item.src, _dest,  _x + item.offX, _y + item.offY, _shade, _mask);

	_dest->unlock();
}

/**
 * Blit body sprite onto surface with optional recoloring.
 * @param body body part sprite, can be null.
 */
void UnitSprite::blitBody(Part& body)
{
	if (!body.src)
	{
		return;
	}
	ScriptWorkerBlit work;
	BattleUnit::ScriptFill(&work, _unit, _save, body.bodyPart, _animationFrame, _shade, _burn);

	_dest->lock();

	work.executeBlit(body.src, _dest,  _x + body.offX, _y + body.offY, _shade, _mask);

	_dest->unlock();
}

/**
 * Draws a unit, using the drawing rules of the unit.
 * This function is called by Map, for each unit on the screen.
 */
void UnitSprite::draw(const BattleUnit* unit, int part, int x, int y, int shade, GraphSubset mask, bool isAltPressed)
{
	_x = x;
	_y = y;

	_unit = unit;
	_part = part;
	_shade = shade;
	_mask = mask;

	if (_unit->isOut())
	{
		// unit is drawn as an item
		return;
	}

	auto* armor = _unit->getArmor();

	_itemR = getIfVisible(_unit->getRightHandWeapon());
	_itemL = getIfVisible(_unit->getLeftHandWeapon());

	_unitSurface = const_cast<Mod*>(_mod)->getSurfaceSet(armor->getSpriteSheet());

	_drawingRoutine = armor->getDrawingRoutine();

	_burn = 0;
	int overkill = _unit->getOverKillDamage();
	int maxHp = _unit->getBaseStats()->health;
	if (overkill)
	{
		if (overkill > maxHp)
		{
			_burn = 16 * (_unit->getFallingPhase() + 1) / armor->getDeathFrames();
		}
		else
		{
			_burn = 16 * overkill * (_unit->getFallingPhase() + 1) / armor->getDeathFrames() / maxHp;
		}
	}

	// Array of drawing routines
	void (UnitSprite::*routines[])() =
	{
		&UnitSprite::drawRoutine0,
		&UnitSprite::drawRoutine1,
		&UnitSprite::drawRoutine2,
		&UnitSprite::drawRoutine3,
		&UnitSprite::drawRoutine4,
		&UnitSprite::drawRoutine5,
		&UnitSprite::drawRoutine6,
		&UnitSprite::drawRoutine7,
		&UnitSprite::drawRoutine8,
		&UnitSprite::drawRoutine9,
		&UnitSprite::drawRoutine0,
		&UnitSprite::drawRoutine11,
		&UnitSprite::drawRoutine12,
		&UnitSprite::drawRoutine0,
		&UnitSprite::drawRoutine0,
		&UnitSprite::drawRoutine0,
		&UnitSprite::drawRoutine16,
		&UnitSprite::drawRoutine4,
		&UnitSprite::drawRoutine4,
		&UnitSprite::drawRoutine19,
		&UnitSprite::drawRoutine20,
		&UnitSprite::drawRoutine21,
		&UnitSprite::drawRoutine3,
	};
	// Call the matching routine
	(this->*(routines[_drawingRoutine]))();
	// draw fire
	if (unit->getFire() > 0)
	{
		_fireSurface->getFrame(4 + (_animationFrame / 2) % 4)->blitNShade(_dest, _x, _y, 0, _mask);
	}
	if (_breathSurface && _helmet && unit->getBreathExhaleFrame() >= 0 && armor->drawBubbles() && !unit->getFloorAbove())
	{
		auto tmpSurface = _breathSurface->getFrame(unit->getBreathExhaleFrame());
		if (tmpSurface)
		{
			// lower the bubbles for shorter or kneeling units.
			tmpSurface->blitNShade(_dest, _x, _y- 30 + (22 - unit->getHeight()), shade, _mask);
		}
	}
	if (isAltPressed)
	{
		// draw unit facing indicator
		auto tmpSurface = _facingArrowSurface->getFrame(7 + ((unit->getDirection() + 1) % 8));
		tmpSurface->blitNShade(_dest, _x, _y, 0);
	}
}

/**
 * Drawing routine for XCom soldiers in overalls, sectoids (routine 0),
 * mutons (routine 10),
 * aquanauts (routine 13),
 * calcinites, deep ones, gill men, lobster men, tasoths (routine 14),
 * aquatoids (routine 15) (this one is no different, it just precludes breathing animations.
 */
void UnitSprite::drawRoutine0()
{
	Part torso{ BODYPART_TORSO }, legs{ BODYPART_LEGS }, leftArm{ BODYPART_LEFTARM }, rightArm{ BODYPART_RIGHTARM }, itemR { BODYPART_ITEM_RIGHTHAND }, itemL { BODYPART_ITEM_LEFTHAND };
	// magic numbers
	const int legsStand = 16, legsKneel = 24;
	int maleTorso, femaleTorso, die, rarm1H, larm2H, rarm2H, rarmShoot, legsFloat, torsoHandsWeaponY = 0;
	if (_drawingRoutine <= 10)
	{
		die = 264; // ufo:eu death frame
		maleTorso = 32;
		femaleTorso = 267;
		rarm1H = 232;
		larm2H = 240;
		rarm2H = 248;
		rarmShoot = 256;
		legsFloat = 275;
	}
	else if (_drawingRoutine == 13)
	{
		if (_helmet)
		{
			die = 259; // aquanaut underwater death frame
			maleTorso = 32; // aquanaut underwater ion armour torso

			if (_unit->getArmor()->getForcedTorso() == TORSO_USE_GENDER)
			{
				femaleTorso = 32; // aquanaut underwater plastic aqua armour torso
			}
			else
			{
				femaleTorso = 286; // aquanaut underwater magnetic ion armour torso
			}
			rarm1H = 248;
			larm2H = 232;
			rarm2H = rarmShoot = 240;
			legsFloat = 294;
		}
		else
		{
			die = 256; // aquanaut land death frame
			// aquanaut land torso
			maleTorso = 270;
			femaleTorso = 262;
			rarm1H = 248;
			larm2H = 232;
			rarm2H = rarmShoot = 240;
			legsFloat = 294;
		}
	}
	else
	{
		die = 256; // tftd unit death frame
		// tftd unit torso
		maleTorso = 32;
		femaleTorso = 262;
		rarm1H = 248;
		larm2H = 232;
		rarm2H = rarmShoot = 240;
		legsFloat = 294;
	}
	const int larmStand = 0, rarmStand = 8;
	const int legsWalk = 56;
	const int larmWalk = 40;
	const int rarmWalk = 48;
	const int YoffWalk[8] = {1, 0, -1, 0, 1, 0, -1, 0}; // bobbing up and down
	const int mutonYoffWalk[8] = {1, 1, 0, 0, 1, 1, 0, 0}; // bobbing up and down (muton)
	const int aquatoidYoffWalk[8] = {1, 0, 0, 1, 2, 1, 0, 0}; // bobbing up and down (aquatoid)
	const int offX[8] = { 8, 10, 7, 4, -9, -11, -7, -3 }; // for the weapons
	const int offY[8] = { -6, -3, 0, 2, 0, -4, -7, -9 }; // for the weapons
	const int offX2[8] = { -8, 3, 5, 12, 6, -1, -5, -13 }; // for the left handed weapons
	const int offY2[8] = { 1, -4, -2, 0, 3, 3, 5, 0 }; // for the left handed weapons
	const int offX3[8] = { 0, 0, 2, 2, 0, 0, 0, 0 }; // for the weapons (muton)
	const int offY3[8] = { -3, -3, -1, -1, -1, -3, -3, -2 }; // for the weapons (muton)
	const int offX4[8] = { -8, 2, 7, 14, 7, -2, -4, -8 }; // for the left handed weapons
	const int offY4[8] = { -3, -3, -1, 0, 3, 3, 0, 1 }; // for the left handed weapons
	const int offX5[8] = { -1, 1, 1, 2, 0, -1, 0, 0 }; // for the weapons (muton)
	const int offY5[8] = { 1, -1, -1, -1, -1, -1, -3, 0 }; // for the weapons (muton)
	const int offX6[8] = { 0, 6, 6, 12, -4, -5, -5, -13 }; // for the left handed rifles
	const int offY6[8] = { -4, -4, -1, 0, 5, 0, 1, 0 }; // for the left handed rifles
	const int offX7[8] = { 0, 6, 8, 12, 2, -5, -5, -13 }; // for the left handed rifles (muton)
	const int offY7[8] = { -4, -6, -1, 0, 3, 0, 1, 0 }; // for the left handed rifles (muton)
	const int offYKneel = 4;
	const int offXAiming = 0;
	const int soldierHeight = 22;

	const int unitDir = _unit->getDirection();
	const int walkPhase = _unit->getWalkingPhase();

	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		Part coll{ BODYPART_COLLAPSING };
		selectUnit(coll, die, _unit->getFallingPhase());
		blitBody(coll);
		return;
	}
	if (_drawingRoutine == 0 || _helmet)
	{
		if ((_unit->getGender() == GENDER_FEMALE && _unit->getArmor()->getForcedTorso() != TORSO_ALWAYS_MALE)
			|| _unit->getArmor()->getForcedTorso() == TORSO_ALWAYS_FEMALE)
		{
			selectUnit(torso, femaleTorso, unitDir);
		}
		else
		{
			selectUnit(torso, maleTorso, unitDir);
		}
	}
	else
	{
		if (_unit->getGender() == GENDER_FEMALE)
		{
			selectUnit(torso, femaleTorso, unitDir);
		}
		else
		{
			selectUnit(torso, maleTorso, unitDir);
		}
	}


	// when walking, torso(fixed sprite) has to be animated up/down
	if (_unit->getStatus() == STATUS_WALKING)
	{
		if (_drawingRoutine == 10)
			torsoHandsWeaponY = mutonYoffWalk[walkPhase];
		else if (_drawingRoutine == 13 || _drawingRoutine == 14)
			torsoHandsWeaponY = YoffWalk[walkPhase]+1;
		else if (_drawingRoutine == 15)
			torsoHandsWeaponY = aquatoidYoffWalk[walkPhase];
		else
			torsoHandsWeaponY = YoffWalk[walkPhase];
		torso.offY = (torsoHandsWeaponY);
		selectUnit(legs, legsWalk, 24 * unitDir + walkPhase);
		selectUnit(leftArm, larmWalk, 24 * unitDir + walkPhase);
		selectUnit(rightArm, rarmWalk, 24 * unitDir + walkPhase);
		if (_drawingRoutine == 10 && unitDir == 3)
		{
			leftArm.offY = (-1);
		}
	}
	else
	{
		if (_unit->isKneeled())
		{
			selectUnit(legs, legsKneel, unitDir);
		}
		else if (_unit->isFloating() && _unit->getMovementType() == MT_FLY)
		{
			selectUnit(legs, legsFloat, unitDir);
		}
		else
		{
			selectUnit(legs, legsStand, unitDir);
		}
		selectUnit(leftArm, larmStand, unitDir);
		selectUnit(rightArm, rarmStand, unitDir);
	}

	sortRifles();

	// holding an item
	if (_itemR)
	{
		// draw handob item
		if (_unit->getStatus() == STATUS_AIMING && _itemR->getRules()->isTwoHanded())
		{
			int dir = (unitDir + 2)%8;
			selectItem(itemR, _itemR, dir);
			itemR.offX = (offX[unitDir]);
			itemR.offY = (offY[unitDir]);
		}
		else
		{
			selectItem(itemR, _itemR, unitDir);
			if (_drawingRoutine == 10)
			{
				if (_itemR->getRules()->isTwoHanded())
				{
					itemR.offX = (offX3[unitDir]);
					itemR.offY = (offY3[unitDir]);
				}
				else
				{
					itemR.offX = (offX5[unitDir]);
					itemR.offY = (offY5[unitDir]);
				}
			}
			else
			{
				itemR.offX = (0);
				itemR.offY = (0);
			}
		}

		// draw arms holding the item
		if (_itemR->getRules()->isTwoHanded())
		{
			selectUnit(leftArm, larm2H, unitDir);
			if (_unit->getStatus() == STATUS_AIMING)
			{
				selectUnit(rightArm, rarmShoot, unitDir);
			}
			else
			{
				selectUnit(rightArm, rarm2H, unitDir);
			}
		}
		else
		{
			if (_drawingRoutine == 10)
				selectUnit(rightArm, rarm2H, unitDir);
			else
				selectUnit(rightArm, rarm1H, unitDir);
		}


		// the fixed arm(s) have to be animated up/down when walking
		if (_unit->getStatus() == STATUS_WALKING)
		{
			itemR.offY = (itemR.offY + torsoHandsWeaponY);
			rightArm.offY = (torsoHandsWeaponY);
			if (_itemR->getRules()->isTwoHanded())
				leftArm.offY = (torsoHandsWeaponY);
		}
	}
	//if we are left handed or dual wielding...
	if (_itemL)
	{
		selectUnit(leftArm, larm2H, unitDir);
		selectItem(itemL, _itemL, unitDir);
		if (!_itemL->getRules()->isTwoHanded())
		{
			if (_drawingRoutine == 10)
			{
				itemL.offX = (offX4[unitDir]);
				itemL.offY = (offY4[unitDir]);
			}
			else
			{
				itemL.offX = (offX2[unitDir]);
				itemL.offY = (offY2[unitDir]);
			}
		}
		else
		{
			itemL.offX = (0);
			itemL.offY = (0);
			selectUnit(rightArm, rarm2H, unitDir);
		}

		if (_unit->getStatus() == STATUS_AIMING && _itemL->getRules()->isTwoHanded())
		{
			int dir = (unitDir + 2)%8;
			selectItem(itemL, _itemL, dir);
			if (_drawingRoutine == 10)
			{
				itemL.offX = (offX7[unitDir]);
				itemL.offY = (offY7[unitDir]);
			}
			else
			{
				itemL.offX = (offX6[unitDir]);
				itemL.offY = (offY6[unitDir]);
			}
			selectUnit(rightArm, rarmShoot, unitDir);
		}

		if (_unit->getStatus() == STATUS_WALKING)
		{
			itemL.offY = (itemL.offY + torsoHandsWeaponY);
			leftArm.offY = (torsoHandsWeaponY);
			if (_itemL->getRules()->isTwoHanded())
				rightArm.offY = (torsoHandsWeaponY);
		}
	}
	// offset everything but legs when kneeled
	if (_unit->isKneeled())
	{
		int offsetTFTD = (_drawingRoutine == 13) ? 1 : 0; // tftd torsos are stubby.
		leftArm.offY = (offYKneel + offsetTFTD);
		rightArm.offY = (offYKneel + offsetTFTD);
		torso.offY = (offYKneel + offsetTFTD);
		itemR.offY = (itemR.offY + offYKneel + offsetTFTD);
		itemL.offY = (itemL.offY + offYKneel + offsetTFTD);
	}
	else if (_unit->getStatus() != STATUS_WALKING)
	{
		leftArm.offY = (0);
		rightArm.offY = (0);
		torso.offY = (0);
	}

	// items are calculated for soldier height (22) - some aliens are smaller, so item is drawn lower.
	if (itemR)
	{
		itemR.offY = (itemR.offY + (soldierHeight - _unit->getStandHeight()));
	}
	if (itemL)
	{
		itemL.offY = (itemL.offY + (soldierHeight - _unit->getStandHeight()));
	}

	if (_unit->getStatus() == STATUS_AIMING)
	{
		torso.offX = (offXAiming);
		legs.offX = (offXAiming);
		leftArm.offX = (offXAiming);
		rightArm.offX = (offXAiming);
		if (itemR)
			itemR.offX = (itemR.offX + offXAiming);
		if (itemL)
			itemL.offX = (itemL.offX + offXAiming);
	}
	else if (!itemR && _drawingRoutine == 10 && _unit->getStatus() == STATUS_WALKING && unitDir == 2)
	{
		rightArm.offX = (-6);
	}

	// blit order depends on unit direction, and whether we are holding a 2 handed weapon.
	switch (unitDir)
	{
	case 0: blitItem(itemR); blitItem(itemL); blitBody(leftArm); blitBody(legs); blitBody(torso); blitBody(rightArm); break;
	case 1: blitBody(leftArm); blitBody(legs); blitItem(itemL); blitBody(torso); blitItem(itemR); blitBody(rightArm); break;
	case 2: blitBody(leftArm); blitBody(legs); blitBody(torso); blitItem(itemL); blitItem(itemR); blitBody(rightArm); break;
	case 3:
		if (_unit->getStatus() != STATUS_AIMING  && ((_itemR && _itemR->getRules()->isTwoHanded()) || (_itemL && _itemL->getRules()->isTwoHanded())))
		{
			blitBody(legs); blitBody(torso); blitBody(leftArm); blitItem(itemR); blitItem(itemL); blitBody(rightArm);
		}
		else
		{
			blitBody(legs); blitBody(torso); blitBody(leftArm); blitBody(rightArm); blitItem(itemR); blitItem(itemL);
		}
		break;
	case 4:	blitBody(legs); blitBody(rightArm); blitBody(torso); blitBody(leftArm); blitItem(itemR); blitItem(itemL);	break;
	case 5:
		if (_unit->getStatus() != STATUS_AIMING  && ((_itemR && _itemR->getRules()->isTwoHanded()) || (_itemL && _itemL->getRules()->isTwoHanded())))
		{
			blitBody(rightArm); blitBody(legs); blitBody(torso); blitBody(leftArm); blitItem(itemR); blitItem(itemL);
		}
		else
		{
			blitBody(rightArm); blitBody(legs); blitItem(itemR); blitItem(itemL); blitBody(torso); blitBody(leftArm);
		}
		break;
	case 6: blitBody(rightArm); blitItem(itemR); blitItem(itemL); blitBody(legs); blitBody(torso); blitBody(leftArm); break;
	case 7:
		if (_unit->getStatus() != STATUS_AIMING  && ((_itemR && _itemR->getRules()->isTwoHanded()) || (_itemL && _itemL->getRules()->isTwoHanded())))
		{
			blitBody(rightArm); blitItem(itemR); blitItem(itemL); blitBody(leftArm); blitBody(legs); blitBody(torso);
		}
		else
		{
			blitItem(itemR); blitItem(itemL); blitBody(leftArm); blitBody(rightArm); blitBody(legs); blitBody(torso);
		}
		break;
	}
}


/**
 * Drawing routine for floaters.
 */
void UnitSprite::drawRoutine1()
{
	Part torso{ BODYPART_TORSO }, leftArm{ BODYPART_LEFTARM }, rightArm{ BODYPART_RIGHTARM }, itemR{ BODYPART_ITEM_RIGHTHAND }, itemL{ BODYPART_ITEM_LEFTHAND };
	// magic numbers
	constexpr static int stand = 16, walk = 24, die = 64;
	constexpr static int larm = 8, rarm = 0, larm2H = 67, rarm2H = 75, rarmShoot = 83, rarm1H= 91; // note that arms are switched vs "normal" sheets
	constexpr static int yoffWalk[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // bobbing up and down
	constexpr static int offX[8] = { 8, 10, 7, 4, -9, -11, -7, -3 }; // for the weapons
	constexpr static int offY[8] = { -6, -3, 0, 2, 0, -4, -7, -9 }; // for the weapons
	constexpr static int offX2[8] = { -8, 3, 7, 13, 6, -3, -5, -13 }; // for the weapons
	constexpr static int offY2[8] = { 1, -4, -1, 0, 3, 3, 5, 0 }; // for the weapons
	constexpr static int offX3[8] = { 0, 6, 6, 12, -4, -5, -5, -13 }; // for the left handed rifles
	constexpr static int offY3[8] = { -4, -4, -1, 0, 5, 0, 1, 0 }; // for the left handed rifles
	constexpr static int offXAiming = 0;

	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		Part coll{ BODYPART_COLLAPSING };
		selectUnit(coll, die, _unit->getFallingPhase());
		blitBody(coll);
		return;
	}

	const int unitDir = _unit->getDirection();
	const int walkPhase = _unit->getWalkingPhase();

	selectUnit(leftArm, larm, unitDir);
	selectUnit(rightArm, rarm, unitDir);
	// when walking, torso(fixed sprite) has to be animated up/down
	if (_unit->getStatus() == STATUS_WALKING)
	{
		selectUnit(torso, walk, (5 * unitDir) + (walkPhase / 1.6)); // floater only has 5 walk animations instead of 8
		torso.offY = (yoffWalk[walkPhase]);
	}
	else
	{
		selectUnit(torso, stand, unitDir);
	}

	sortRifles();

	// holding an item
	if (_itemR)
	{
		// draw handob item
		if (_unit->getStatus() == STATUS_AIMING && _itemR->getRules()->isTwoHanded())
		{
			int dir = (_unit->getDirection() + 2)%8;
			selectItem(itemR, _itemR, dir);
			itemR.offX = (offX[unitDir]);
			itemR.offY = (offY[unitDir]);
		}
		else
		{
			selectItem(itemR, _itemR, unitDir);
			itemR.offX = (0);
			itemR.offY = (0);
		}
		// draw arms holding the item
		if (_itemR->getRules()->isTwoHanded())
		{
			selectUnit(leftArm, larm2H, unitDir);
			if (_unit->getStatus() == STATUS_AIMING)
			{
				selectUnit(rightArm, rarmShoot, unitDir);
			}
			else
			{
				selectUnit(rightArm, rarm2H, unitDir);
			}
		}
		else
		{
			selectUnit(rightArm, rarm1H, unitDir);
		}
	}

	//if we are left handed or dual wielding...
	if (_itemL)
	{
		selectUnit(leftArm, larm2H, unitDir);
		selectItem(itemL, _itemL, unitDir);
		if (!_itemL->getRules()->isTwoHanded())
		{
			itemL.offX = (offX2[unitDir]);
			itemL.offY = (offY2[unitDir]);
		}
		else
		{
			itemL.offX = (0);
			itemL.offY = (0);
			selectUnit(rightArm, rarm2H, unitDir);
		}

		if (_unit->getStatus() == STATUS_AIMING && _itemL->getRules()->isTwoHanded())
		{
			int dir = (unitDir + 2)%8;
			selectItem(itemL, _itemL, dir);
			itemL.offX = (offX3[unitDir]);
			itemL.offY = (offY3[unitDir]);
			selectUnit(rightArm, rarmShoot, unitDir);
		}

		if (_unit->getStatus() == STATUS_WALKING)
		{
			leftArm.offY = (yoffWalk[walkPhase]);
			itemL.offY = (itemL.offY + yoffWalk[walkPhase]);
			if (_itemL->getRules()->isTwoHanded())
				rightArm.offY = (yoffWalk[walkPhase]);
		}
	}

	if (_unit->getStatus() != STATUS_WALKING)
	{
		leftArm.offY = (0);
		rightArm.offY = (0);
		torso.offY = (0);
	}
	if (_unit->getStatus() == STATUS_AIMING)
	{
		torso.offX = (offXAiming);
		leftArm.offX = (offXAiming);
		rightArm.offX = (offXAiming);
		if (itemR)
			itemR.offX = (itemR.offX + offXAiming);
		if (itemL)
			itemL.offX = (itemL.offX + offXAiming);
	}
	// blit order depends on unit direction.
	switch (unitDir)
	{
	case 0: blitItem(itemR); blitItem(itemL); blitBody(leftArm); blitBody(torso); blitBody(rightArm); break;
	case 1: blitBody(leftArm); blitBody(torso); blitBody(rightArm); blitItem(itemR); blitItem(itemL); break;
	case 2: blitBody(leftArm); blitBody(torso); blitBody(rightArm); blitItem(itemR); blitItem(itemL); break;
	case 3: blitBody(torso); blitBody(leftArm); blitBody(rightArm); blitItem(itemR); blitItem(itemL); break;
	case 4: blitBody(torso); blitBody(leftArm); blitBody(rightArm); blitItem(itemR); blitItem(itemL); break;
	case 5: blitBody(rightArm); blitBody(torso); blitBody(leftArm); blitItem(itemR); blitItem(itemL); break;
	case 6: blitBody(rightArm); blitItem(itemR); blitItem(itemL); blitBody(torso); blitBody(leftArm); break;
	case 7: blitBody(rightArm); blitItem(itemR); blitItem(itemL); blitBody(leftArm); blitBody(torso); break;
	}
}

/**
 * Drawing routine for XCom tanks.
 */
void UnitSprite::drawRoutine2()
{
	constexpr static int offX[8] = { -2, -7, -5, 0, 5, 7, 2, 0 }; // hovertank offsets
	constexpr static int offy[8] = { -1, -3, -4, -5, -4, -3, -1, -1 }; // hovertank offsets

	Part s{ BODYPART_LARGE_TORSO + _part };

	const int hoverTank = _unit->getOriginalMovementType() == MT_FLY ? 32 : 0;
	const int turret = _unit->getTurretType();

	// draw the animated propulsion below the hwp
	if (hoverTank != 0)
	{
		if (_part > 0)
		{
			Part p{ BODYPART_LARGE_PROPULSION + _part };
			selectUnit(p, 104 + ((_part-1) * 8), _animationFrame % 8);
			blitBody(p);
		}
		else
		{
			// draw nothing, can be override by script
			Part p{ BODYPART_LARGE_PROPULSION + _part };
			selectUnit(p, InvalidSpriteIndex, _animationFrame % 8);
			blitBody(p);
		}
	}

	// draw the tank itself
	selectUnit(s, hoverTank + (_part * 8), _unit->getDirection());
	blitBody(s);

	// draw the turret, together with the last part
	if (_part == 3 && turret != -1)
	{
		Part t{ BODYPART_LARGE_TURRET };
		selectUnit(t, 64 + (turret * 8), _unit->getTurretDirection());
		int turretOffsetX = 0;
		int turretOffsetY = -4;
		if (hoverTank)
		{
			turretOffsetX += offX[_unit->getDirection()];
			turretOffsetY += offy[_unit->getDirection()];
		}
		t.offX = (turretOffsetX);
		t.offY = (turretOffsetY);
		blitBody(t);
	}

}

/**
 * Drawing routine for cyberdiscs. (3)
 * and helicopters (22)
 */
void UnitSprite::drawRoutine3()
{
	Part s{ BODYPART_LARGE_TORSO + _part };

	// draw the animated propulsion below the hwp
	if (_drawingRoutine == 3)
	{
		if (_part > 0)
		{
			Part p{ BODYPART_LARGE_PROPULSION + _part };
			selectUnit(p, 32 + ((_part-1) * 8), _animationFrame % 8);
			blitBody(p);
		}
		else
		{
			// draw nothing, can be override by script
			Part p{ BODYPART_LARGE_PROPULSION + _part };
			selectUnit(p, InvalidSpriteIndex, _animationFrame % 8);
			blitBody(p);
		}
	}

	selectUnit(s, (_part * 8), _unit->getDirection());

	blitBody(s);

	// draw the animated propulsion above the hwp
	if (_drawingRoutine == 22)
	{
		if (_part > 0)
		{
			Part p{ BODYPART_LARGE_PROPULSION + _part };
			selectUnit(p, 32 + ((_part-1) * 8), _animationFrame % 8);
			blitBody(p);
		}
		else
		{
			// draw nothing, can be override by script
			Part p{ BODYPART_LARGE_PROPULSION + _part };
			selectUnit(p, InvalidSpriteIndex, _animationFrame % 8);
			blitBody(p);
		}
	}
}

/**
 * Drawing routine for civilians, ethereals, zombies (routine 4),
 * tftd civilians, tftd zombies (routine 17), more tftd civilians (routine 18).
 * Very easy: first 8 is standing positions, then 8 walking sequences of 8, finally death sequence of 3
 */
void UnitSprite::drawRoutine4()
{
	Part s{ BODYPART_TORSO }, itemR{ BODYPART_ITEM_RIGHTHAND }, itemL{ BODYPART_ITEM_LEFTHAND };
	int stand = 0, walk = 8, die = 72;
	constexpr static int offX[8] = { 8, 10, 7, 4, -9, -11, -7, -3 }; // for the weapons
	constexpr static int offY[8] = { -6, -3, 0, 2, 0, -4, -7, -9 }; // for the weapons
	constexpr static int offX2[8] = { -8, 3, 5, 12, 6, -1, -5, -13 }; // for the weapons
	constexpr static int offY2[8] = { 1, -4, -2, 0, 3, 3, 5, 0 }; // for the weapons
	constexpr static int offX3[8] = { 0, 6, 6, 12, -4, -5, -5, -13 }; // for the left handed rifles
	constexpr static int offY3[8] = { -4, -4, -1, 0, 5, 0, 1, 0 }; // for the left handed rifles
	constexpr static int standConvert[8] = { 3, 2, 1, 0, 7, 6, 5, 4 }; // array for converting stand frames for some tftd civilians
	constexpr static int offXAiming = 0;

	if (_drawingRoutine == 17) // tftd civilian - first set
	{
		stand = 64;
		walk = 0;
	}
	else if (_drawingRoutine == 18) // tftd civilian - second set
	{
		stand = 140;
		walk = 76;
		die = 148;
	}

	const int unitDir = _unit->getDirection();

	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		Part coll{ BODYPART_COLLAPSING };
		selectUnit(coll, die, _unit->getFallingPhase());
		blitBody(coll);
		return;
	}
	else if (_unit->getStatus() == STATUS_WALKING)
	{
		selectUnit(s, walk, (8 * unitDir) + _unit->getWalkingPhase());
	}
	else if (_drawingRoutine != 17)
	{
		selectUnit(s, stand, unitDir);
	}
	else
	{
		selectUnit(s, stand, standConvert[unitDir]);
	}

	sortRifles();

	if (_itemR && !_itemR->getRules()->isFixed())
	{
		// draw handob item
		if (_unit->getStatus() == STATUS_AIMING && _itemR->getRules()->isTwoHanded())
		{
			int dir = (unitDir + 2)%8;
			selectItem(itemR, _itemR, dir);
			itemR.offX = (offX[unitDir]);
			itemR.offY = (offY[unitDir]);
		}
		else
		{
			if (_itemR->getSlot()->isRightHand())
			{
				selectItem(itemR, _itemR, unitDir);
				itemR.offX = (0);
				itemR.offY = (0);
			}
			else
			{
				selectItem(itemR, _itemR, unitDir);
				itemR.offX = (offX2[unitDir]);
				itemR.offY = (offY2[unitDir]);
			}
		}
	}

	//if we are dual wielding...
	if (_itemL && !_itemL->getRules()->isFixed())
	{
		selectItem(itemL, _itemL, unitDir);
		if (!_itemL->getRules()->isTwoHanded())
		{
			itemL.offX = (offX2[unitDir]);
			itemL.offY = (offY2[unitDir]);
		}
		else
		{
			itemL.offX = (0);
			itemL.offY = (0);
		}

		if (_unit->getStatus() == STATUS_AIMING && _itemL->getRules()->isTwoHanded())
		{
			int dir = (unitDir + 2)%8;
			selectItem(itemL, _itemL, dir);
			itemL.offX = (offX3[unitDir]);
			itemL.offY = (offY3[unitDir]);
		}
	}

	if (_unit->getStatus() == STATUS_AIMING)
	{
		s.offX = (offXAiming);
		if (itemR)
			itemR.offX = (itemR.offX + offXAiming);
		if (itemL)
			itemL.offX = (itemL.offX + offXAiming);
	}
	switch (unitDir)
	{
	case 0: blitItem(itemL); blitItem(itemR); blitBody(s); break;
	case 1: blitItem(itemL); blitBody(s); blitItem(itemR); break;
	case 2: blitBody(s); blitItem(itemL); blitItem(itemR); break;
	case 3: blitBody(s); blitItem(itemR); blitItem(itemL); break;
	case 4: blitBody(s); blitItem(itemR); blitItem(itemL); break;
	case 5: blitItem(itemR); blitBody(s); blitItem(itemL); break;
	case 6: blitItem(itemR); blitBody(s); blitItem(itemL); break;
	case 7: blitItem(itemR); blitItem(itemL); blitBody(s); break;
	}
}

/**
 * Drawing routine for sectopods and reapers.
 */
void UnitSprite::drawRoutine5()
{
	Part s{ BODYPART_LARGE_TORSO + _part };

	if (_unit->getStatus() == STATUS_WALKING)
	{
		selectUnit(s, 32 + (_part * 4), (_unit->getDirection() * 16) + ((_unit->getWalkingPhase() / 2) % 4));
	}
	else
	{
		selectUnit(s, 0 + (_part * 8), _unit->getDirection());
	}

	blitBody(s);
}

/**
 * Drawing routine for snakemen.
 */
void UnitSprite::drawRoutine6()
{
	Part torso{ BODYPART_TORSO }, legs{ BODYPART_LEGS }, leftArm{ BODYPART_LEFTARM }, rightArm{ BODYPART_RIGHTARM }, itemR{ BODYPART_ITEM_RIGHTHAND }, itemL{ BODYPART_ITEM_LEFTHAND };
	// magic numbers
	constexpr static int Torso = 24, legsStand = 16, die = 96;
	constexpr static int larmStand = 0, rarmStand = 8, rarm1H = 99, larm2H = 107, rarm2H = 115, rarmShoot = 123;
	constexpr static int legsWalk = 32;
	constexpr static int yoffWalk[8] = {3, 3, 2, 1, 0, 0, 1, 2}; // bobbing up and down
	constexpr static int xoffWalka[8] = {0, 0, 1, 2, 3, 3, 2, 1};
	constexpr static int xoffWalkb[8] = {0, 0, -1, -2, -3, -3, -2, -1};
	constexpr static int yoffStand[8] = {2, 1, 1, 0, 0, 0, 0, 0};
	constexpr static int offX[8] = { 8, 10, 5, 2, -8, -10, -5, -2 }; // for the weapons
	constexpr static int offY[8] = { -6, -3, 0, 0, 2, -3, -7, -9 }; // for the weapons
	constexpr static int offX2[8] = { -8, 2, 7, 13, 7, 0, -3, -15 }; // for the weapons
	constexpr static int offY2[8] = { 1, -4, -2, 0, 3, 3, 5, 0 }; // for the weapons
	constexpr static int offX3[8] = { 0, 6, 6, 12, -4, -5, -5, -13 }; // for the left handed rifles
	constexpr static int offY3[8] = { -4, -4, -1, 0, 5, 0, 1, 0 }; // for the left handed rifles
	constexpr static int offXAiming = 0;

	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		Part coll{ BODYPART_COLLAPSING };
		selectUnit(coll, die, _unit->getFallingPhase());
		blitBody(coll);
		return;
	}

	const int unitDir = _unit->getDirection();
	const int walkPhase = _unit->getWalkingPhase();

	selectUnit(torso, Torso, unitDir);
	selectUnit(leftArm, larmStand, unitDir);
	selectUnit(rightArm, rarmStand, unitDir);


	// when walking, torso(fixed sprite) has to be animated up/down
	if (_unit->getStatus() == STATUS_WALKING)
	{
		int xoffWalk = 0;
		if (unitDir < 3)
			xoffWalk = xoffWalka[walkPhase];
		if (unitDir < 7 && unitDir > 3)
			xoffWalk = xoffWalkb[walkPhase];
		torso.offY = (yoffWalk[walkPhase]);
		torso.offX = (xoffWalk);
		selectUnit(legs, legsWalk, 8 * unitDir + walkPhase);
		rightArm.offY = (yoffWalk[walkPhase]);
		leftArm.offY = (yoffWalk[walkPhase]);
		rightArm.offX = (xoffWalk);
		leftArm.offX = (xoffWalk);
	}
	else
	{
		selectUnit(legs, legsStand, unitDir);
	}

	sortRifles();

	// holding an item
	if (_itemR)
	{
		// draw handob item
		if (_unit->getStatus() == STATUS_AIMING && _itemR->getRules()->isTwoHanded())
		{
			int dir = (unitDir + 2)%8;
			selectItem(itemR, _itemR, dir);
			itemR.offX = (offX[unitDir]);
			itemR.offY = (offY[unitDir]);
		}
		else
		{
			selectItem(itemR, _itemR, unitDir);
			itemR.offX = (0);
			itemR.offY = (0);
			if (!_itemR->getRules()->isTwoHanded())
			{
				itemR.offY = (yoffStand[unitDir]);
			}
		}


		// draw arms holding the item
		if (_itemR->getRules()->isTwoHanded())
		{
			selectUnit(leftArm, larm2H, unitDir);
			if (_unit->getStatus() == STATUS_AIMING)
			{
				selectUnit(rightArm, rarmShoot, unitDir);
			}
			else
			{
				selectUnit(rightArm, rarm2H, unitDir);
			}
		}
		else
		{
			selectUnit(rightArm, rarm1H, unitDir);
		}


		// the fixed arm(s) have to be animated up/down when walking
		if (_unit->getStatus() == STATUS_WALKING)
		{
			itemR.offY = (yoffWalk[walkPhase]);
			rightArm.offY = (yoffWalk[walkPhase]);
			if (_itemR->getRules()->isTwoHanded())
				leftArm.offY = (yoffWalk[walkPhase]);
		}
	}
	//if we are left handed or dual wielding...
	if (_itemL)
	{
		selectUnit(leftArm, larm2H, unitDir);
		selectItem(itemL, _itemL, unitDir);
		if (!_itemL->getRules()->isTwoHanded())
		{
			itemL.offX = (offX2[unitDir]);
			itemL.offY = (offY2[unitDir]);
		}
		else
		{
			itemL.offX = (0);
			itemL.offY = (0);
			if (!_itemL->getRules()->isTwoHanded())
			{
				itemL.offY = (yoffStand[unitDir]);
			}
			selectUnit(rightArm, rarm2H, unitDir);
		}

		if (_unit->getStatus() == STATUS_AIMING && _itemL->getRules()->isTwoHanded())
		{
			int dir = (unitDir + 2)%8;
			selectItem(itemL, _itemL, dir);
			itemL.offX = (offX3[unitDir]);
			itemL.offY = (offY3[unitDir]);
			selectUnit(rightArm, rarmShoot, unitDir);
		}

		if (_unit->getStatus() == STATUS_WALKING)
		{
			leftArm.offY = (yoffWalk[walkPhase]);
			itemL.offY = (offY2[unitDir] + yoffWalk[walkPhase]);
			if (_itemL->getRules()->isTwoHanded())
				rightArm.offY = (yoffWalk[walkPhase]);
		}
	}
	// offset everything but legs when kneeled
	if (_unit->getStatus() != STATUS_WALKING)
	{
		leftArm.offY = (0);
		rightArm.offY = (0);
		torso.offY = (0);
	}
	if (_unit->getStatus() == STATUS_AIMING)
	{
		torso.offX = (offXAiming);
		legs.offX = (offXAiming);
		leftArm.offX = (offXAiming);
		rightArm.offX = (offXAiming);
		if (itemR)
			itemR.offX = (itemR.offX + offXAiming);
		if (itemL)
			itemL.offX = (itemL.offX + offXAiming);
	}

	// blit order depends on unit direction.
	switch (unitDir)
	{
	case 0: blitItem(itemR); blitItem(itemL); blitBody(leftArm); blitBody(legs); blitBody(torso); blitBody(rightArm); break;
	case 1: blitBody(leftArm); blitBody(legs); blitItem(itemL); blitBody(torso); blitItem(itemR); blitBody(rightArm); break;
	case 2: blitBody(leftArm); blitBody(legs); blitBody(torso); blitBody(rightArm); blitItem(itemR); blitItem(itemL); break;
	case 3: blitBody(legs); blitBody(torso); blitBody(leftArm); blitBody(rightArm); blitItem(itemR); blitItem(itemL); break;
	case 4: blitBody(rightArm); blitBody(legs); blitBody(torso); blitBody(leftArm); blitItem(itemR); blitItem(itemL); break;
	case 5: blitBody(rightArm); blitBody(legs); blitBody(torso); blitBody(leftArm); blitItem(itemR); blitItem(itemL); break;
	case 6: blitBody(rightArm); blitBody(legs); blitItem(itemR); blitItem(itemL); blitBody(torso); blitBody(leftArm); break;
	case 7: blitItem(itemR); blitItem(itemL); blitBody(leftArm); blitBody(rightArm); blitBody(legs); blitBody(torso); break;
	}
}

/**
 * Drawing routine for chryssalid.
 */
void UnitSprite::drawRoutine7()
{
	Part torso{ BODYPART_TORSO }, legs{ BODYPART_LEGS }, leftArm{ BODYPART_LEFTARM }, rightArm{ BODYPART_RIGHTARM };
	// magic numbers
	constexpr static int Torso = 24, legsStand = 16, die = 224;
	constexpr static int larmStand = 0, rarmStand = 8;
	constexpr static int legsWalk = 48;
	constexpr static int larmWalk = 32;
	constexpr static int rarmWalk = 40;
	constexpr static int yoffWalk[8] = {1, 0, -1, 0, 1, 0, -1, 0}; // bobbing up and down

	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		Part coll{ BODYPART_COLLAPSING };
		selectUnit(coll, die, _unit->getFallingPhase());
		blitBody(coll);
		return;
	}

	const int unitDir = _unit->getDirection();
	const int walkPhase = _unit->getWalkingPhase();

	selectUnit(torso, Torso, unitDir);


	// when walking, torso(fixed sprite) has to be animated up/down
	if (_unit->getStatus() == STATUS_WALKING)
	{
		torso.offY = (yoffWalk[walkPhase]);
		selectUnit(legs, legsWalk, 24 * unitDir + walkPhase);
		selectUnit(leftArm, larmWalk, 24 * unitDir + walkPhase);
		selectUnit(rightArm, rarmWalk, 24 * unitDir + walkPhase);
	}
	else
	{

		selectUnit(legs, legsStand, unitDir);
		selectUnit(leftArm, larmStand, unitDir);
		selectUnit(rightArm, rarmStand, unitDir);
		leftArm.offY = (0);
		rightArm.offY = (0);
		torso.offY = (0);
	}

	// blit order depends on unit direction
	switch (unitDir)
	{
	case 0: blitBody(leftArm); blitBody(legs); blitBody(torso); blitBody(rightArm); break;
	case 1: blitBody(leftArm); blitBody(legs); blitBody(torso); blitBody(rightArm); break;
	case 2: blitBody(leftArm); blitBody(legs); blitBody(torso); blitBody(rightArm); break;
	case 3: blitBody(legs); blitBody(torso); blitBody(leftArm); blitBody(rightArm); break;
	case 4: blitBody(rightArm); blitBody(legs); blitBody(torso); blitBody(leftArm); break;
	case 5: blitBody(rightArm); blitBody(legs); blitBody(torso); blitBody(leftArm); break;
	case 6: blitBody(rightArm); blitBody(legs); blitBody(torso); blitBody(leftArm); break;
	case 7: blitBody(leftArm); blitBody(rightArm); blitBody(legs); blitBody(torso); break;
	}
}

/**
 * Drawing routine for silacoids.
 */
void UnitSprite::drawRoutine8()
{
	Part legs{ BODYPART_TORSO };
	// magic numbers
	constexpr static int Body = 0, aim = 5, die = 6;
	constexpr static int Pulsate[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };

	selectUnit(legs, Body, Pulsate[_animationFrame % 8]);

	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		Part coll{ BODYPART_COLLAPSING };
		selectUnit(coll, die, _unit->getFallingPhase());
		blitBody(coll);
		return;
	}

	if (_unit->getStatus() == STATUS_AIMING)
		selectUnit(legs, aim, 0);

	blitBody(legs);
}

/**
 * Drawing routine for celatids.
 */
void UnitSprite::drawRoutine9()
{
	Part torso{ BODYPART_TORSO };
	// magic numbers
	constexpr static int Body = 0, die = 25;

	selectUnit(torso, Body, _animationFrame % 8);

	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		Part coll{ BODYPART_COLLAPSING };
		selectUnit(coll, die, _unit->getFallingPhase());
		blitBody(coll);
		return;
	}

	blitBody(torso);
}

/**
 * Drawing routine for tftd tanks.
 */
void UnitSprite::drawRoutine11()
{
	// magic numbers
	constexpr static int offTurretX[8] = { -2, -6, -5, 0, 5, 6, 2, 0 }; // turret offsets
	constexpr static int offTurretYAbove[8] = { 5, 3, 0, 0, 0, 3, 5, 4 }; // turret offsets
	constexpr static int offTurretYBelow[8] = { -11, -13, -16, -16, -16, -13, -11, -12 }; // turret offsets

	int body = 0;
	int animFrame = _unit->getWalkingPhase() % 4;
	if (_unit->getOriginalMovementType() == MT_FLY)
	{
		body = 128;
		animFrame = _animationFrame % 4;
	}

	Part s{ BODYPART_LARGE_TORSO + _part };
	selectUnit(s, body + (_part * 4), 16 * _unit->getDirection() + animFrame);
	s.offY = (4);
	blitBody(s);

	int turret = _unit->getTurretType();
	// draw the turret, overlapping all 4 parts
	if ((_part == 3 || _part == 0) && turret != -1 && !_unit->getFloorAbove())
	{
		Part t{ BODYPART_LARGE_TURRET };
		selectUnit(t, 256 + (turret * 8), _unit->getTurretDirection());
		t.offX = (offTurretX[_unit->getDirection()]);
		if (_part == 3)
			t.offY = (offTurretYBelow[_unit->getDirection()]);
		else
			t.offY = (offTurretYAbove[_unit->getDirection()]);
		blitBody(t);
	}

}

/**
 * Drawing routine for hallucinoids.
 */
void UnitSprite::drawRoutine12()
{
	Part s{ BODYPART_LARGE_TORSO + _part };

	selectUnit(s, (_part * 8), _animationFrame % 8);

	blitBody(s);
}

/**
* Drawing routine for biodrones.
*/
void UnitSprite::drawRoutine16()
{
	Part s{ BODYPART_TORSO };
	// magic numbers
	constexpr static int die = 8;

	selectUnit(s, 0, _animationFrame % 8);

	if ( (_unit->getStatus() == STATUS_COLLAPSING))
	{
		Part coll{ BODYPART_COLLAPSING };
		selectUnit(coll, die, _unit->getFallingPhase());
		blitBody(coll);
		return;
	}

	blitBody(s);
}

/**
 * Drawing routine for tentaculats.
 */
void UnitSprite::drawRoutine19()
{
	Part s{ BODYPART_TORSO };
	// magic numbers
	constexpr static int stand = 0, move = 8, die = 16;

	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		Part coll{ BODYPART_COLLAPSING };
		selectUnit(coll, die, _unit->getFallingPhase());
		blitBody(coll);
		return;
	}
	else if (_unit->getStatus() == STATUS_WALKING)
	{
		selectUnit(s, move, _unit->getDirection());
	}
	else
	{
		selectUnit(s, stand, _unit->getDirection());
	}

	blitBody(s);
}

/**
 * Drawing routine for triscenes.
 */
void UnitSprite::drawRoutine20()
{
	Part s{ BODYPART_LARGE_TORSO + _part };

	if (_unit->getStatus() == STATUS_WALKING)
	{
		selectUnit(s, (_part * 5), (_unit->getWalkingPhase()/2%4) + 5 * (4 * _unit->getDirection()));
	}
	else
	{
		selectUnit(s, (_part * 5), 5 * (4 * _unit->getDirection()));
	}

	blitBody(s);
}

/**
 * Drawing routine for xarquids.
 */
void UnitSprite::drawRoutine21()
{
	Part s{ BODYPART_LARGE_TORSO + _part };

	selectUnit(s, (_part * 4), (_unit->getDirection() * 16) + (_animationFrame % 4));

	blitBody(s);
}

/**
 * Determines which weapons to display in the case of two-handed weapons.
 */
void UnitSprite::sortRifles()
{
	if (_itemR && _itemR->getRules()->isTwoHanded())
	{
		if (_itemL && _itemL->getRules()->isTwoHanded())
		{
			_itemR = _unit->getActiveHand(_itemL, _itemR);
			_itemL = 0;
		}
		else if (_unit->getStatus() != STATUS_AIMING)
		{
			_itemL = 0;
		}
	}
	else if (_itemL && _itemL->getRules()->isTwoHanded())
	{
		if (_unit->getStatus() != STATUS_AIMING)
		{
			_itemR = 0;
		}
	}
}

} //namespace OpenXcom
