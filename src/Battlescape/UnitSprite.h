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
#include "../Engine/Surface.h"
#include "../Engine/Script.h"

namespace OpenXcom
{

class BattleUnit;
class BattleItem;
class SavedBattleGame;
class SurfaceSet;
class Mod;

/**
 * A class that renders a specific unit, given its render rules
 * combining the right frames from the surfaceset.
 */
class UnitSprite
{
private:
	struct Part
	{
		const Surface *src;
		int bodyPart;
		int offX;
		int offY;

		Part(int body, const Surface *s = nullptr) : src{ s }, bodyPart{ body }, offX{ 0 }, offY{ 0 } { }

		void operator=(const Surface *s) { src = s; }
		explicit operator bool() { return src; }
	};

	const BattleUnit *_unit;
	const BattleItem *_itemR, *_itemL;
	const SurfaceSet *_unitSurface, *_itemSurface, *_fireSurface, *_breathSurface, *_facingArrowSurface;
	Surface *_dest;
	const SavedBattleGame *_save;
	const Mod *_mod;
	int _part, _animationFrame, _drawingRoutine;
	bool _helmet;
	int _x, _y, _shade, _burn;
	GraphSubset _mask;

	/// Drawing routine for XCom soldiers in overalls, sectoids (routine 0),
	/// mutons (routine 10),
	/// aquanauts (routine 13),
	/// aquatoids, calcinites, deep ones, gill men, lobster men, tasoths (routine 14).
	void drawRoutine0();
	/// Drawing routine for floaters.
	void drawRoutine1();
	/// Drawing routine for XCom tanks.
	void drawRoutine2();
	/// Drawing routine for cyberdiscs.
	void drawRoutine3();
	/// Drawing routine for civilians, ethereals, zombies (routine 4),
	/// tftd civilians, tftd zombies (routine 16), more tftd civilians (routine 17).
	void drawRoutine4();
	/// Drawing routine for sectopods and reapers.
	void drawRoutine5();
	/// Drawing routine for snakemen.
	void drawRoutine6();
	/// Drawing routine for chryssalid.
	void drawRoutine7();
	/// Drawing routine for silacoids.
	void drawRoutine8();
	/// Drawing routine for celatids.
	void drawRoutine9();
	/// Drawing routine for TFTD tanks.
	void drawRoutine11();
	/// Drawing routine for hallucinoids.
	void drawRoutine12();
	/// Drawing routine for biodrones.
	void drawRoutine16();
	/// Drawing routine for tentaculats.
	void drawRoutine19();
	/// Drawing routine for triscenes.
	void drawRoutine20();
	/// Drawing routine for xarquids.
	void drawRoutine21();
	/// Sort two handed sprites out.
	void sortRifles();
	/// Get graphic for unit part.
	void selectUnit(Part& p, int index, int offset);
	/// Get graphic for item part.
	void selectItem(Part& p, const BattleItem *item, int offset);
	/// Blit weapon sprite.
	void blitItem(Part& item);
	/// Blit body sprite.
	void blitBody(Part& body);
public:
	/// Creates a new UnitSprite at the specified position and size.
	UnitSprite(Surface* dest, const Mod* mod, const SavedBattleGame* save, int frame, bool helmet);
	/// Cleans up the UnitSprite.
	~UnitSprite();
	/// Draws the unit.
	void draw(const BattleUnit* unit, int part, int x, int y, int shade, GraphSubset mask, bool drawFacingIndicator);
};

} //namespace OpenXcom

