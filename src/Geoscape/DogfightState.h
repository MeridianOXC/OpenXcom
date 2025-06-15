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
#include "../Mod/RuleCraft.h"
#include <vector>
#include <string>

namespace OpenXcom
{

const int STANDOFF_DIST = 560;
const int AGGRESSIVE_DIST = 64;
enum ColorNames { CRAFT_MIN, CRAFT_MAX, RADAR_MIN, RADAR_MAX, DAMAGE_MIN, DAMAGE_MAX, BLOB_MIN, RANGE_METER, DISABLED_WEAPON, DISABLED_AMMO, DISABLED_RANGE, SHIELD_MIN, SHIELD_MAX };

class ImageButton;
class Text;
class Surface;
class InteractiveSurface;
class Timer;
class GeoscapeState;
class Craft;
class Ufo;
class CraftWeaponProjectile;

/**
 * Shows a dogfight (interception) between a
 * player craft and an UFO.
 */
class DogfightState : public State
{
private:
	GeoscapeState *_state;
	Timer *_craftDamageAnimTimer;
	Surface *_window, *_battle, *_range[RuleCraft::WeaponMax], *_damage, *_craftSprite, *_craftShield;
	InteractiveSurface *_btnMinimize, *_preview, *_weapon[RuleCraft::WeaponMax];
	ImageButton *_btnStandoff, *_btnCautious, *_btnStandard, *_btnAggressive, *_btnDisengage, *_btnUfo;
	ImageButton *_mode;
	InteractiveSurface *_btnMinimizedIcon;
	Text *_txtAmmo[RuleCraft::WeaponMax], *_txtDistance, *_txtStatus, *_txtInterceptionNumber;
	Text *_txtOceanIndicator;
	Craft *_craft;
	Ufo *_ufo;
	bool _ufoIsAttacking, _missileCraft, _missileImpact;
	bool _disableDisengage, _disableStandoff, _disableCautious, _disableStandard, _disableAggressive;
	bool _craftIsDefenseless, _selfDestructPressed;
	int _timeout, _currentDist, _targetDist, _weaponFireInterval[RuleCraft::WeaponMax], _weaponFireCountdown[RuleCraft::WeaponMax];
	bool _end, _endUfoHandled, _endCraftHandled, _ufoBreakingOff, _destroyUfo, _destroyCraft, _weaponEnabled[RuleCraft::WeaponMax];
	bool _minimized, _endDogfight, _animatingHit, _waitForPoly, _waitForAltitude;
	std::vector<CraftWeaponProjectile*> _projectiles;
	static const int _ufoBlobs[8][13][13];
	static const int _projectileBlobs[4][6][3];
	int _ufoSize, _ufoBlobSize, _craftHeight, _currentCraftDamageColor, _interceptionNumber;
	size_t _interceptionsCount;
	int _x, _y, _minimizedIconX, _minimizedIconY;
	int _weaponNum;
	int _pilotAccuracyBonus, _pilotDodgeBonus, _pilotApproachSpeedModifier, _craftAccelerationBonus;
	bool _firedAtLeastOnce, _experienceAwarded;
	bool _delayedRecolorDone;
	// craft min/max, radar min/max, damage min/max, shield min/max
	int _colors[13];
	// Ends the dogfight.
	void endDogfight();
	bool _tractorLockedOn[RuleCraft::WeaponMax];
	void updateOceanIndicator();

public:
	/// Creates the Dogfight state.
	DogfightState(GeoscapeState *state, Craft *craft, Ufo *ufo, bool ufoIsAttacking = false);
	/// Cleans up the Dogfight state.
	~DogfightState();
	/// Returns true if this is a hunter-killer dogfight.
	bool isUfoAttacking() const;
	/// Runs the timers.
	void think() override;
	/// Animates the window.
	void animate();
	/// Moves the craft.
	void update();
	// Fires the weapons.
	void fireWeapon(int i);
	// Fires UFO weapon.
	void ufoFireWeapon();
	// Sets the craft to minimum distance.
	void minimumDistance();
	// Sets the craft to maximum distance.
	void maximumDistance();
	// Sets the craft to maximum distance or 8 km, whichever is smaller.
	void aggressiveDistance();
	/// Changes the status text.
	void setStatus(const std::string &status);
	/// Handler for clicking the Minimize button.
	void btnMinimizeClick(Action *action);
	/// Handler for pressing the Standoff button.
	void btnStandoffPress(Action *action);
	void btnStandoffRightPress(Action *action);
	void btnStandoffSimulateLeftPress(Action *action);
	/// Handler for pressing the Cautious Attack button.
	void btnCautiousPress(Action *action);
	void btnCautiousRightPress(Action *action);
	void btnCautiousSimulateLeftPress(Action *action);
	/// Handler for pressing the Standard Attack button.
	void btnStandardPress(Action *action);
	void btnStandardRightPress(Action *action);
	void btnStandardSimulateLeftPress(Action *action);
	/// Handler for pressing the Aggressive Attack button.
	void btnAggressivePress(Action *action);
	void btnAggressiveRightPress(Action *action);
	void btnAggressiveSimulateLeftPress(Action *action);
	/// Handler for pressing the Disengage button.
	void btnDisengagePress(Action *action);
	void btnDisengageRightPress(Action *action);
	void btnDisengageSimulateLeftPress(Action *action);
	/// Handler for clicking the Ufo button.
	void btnUfoClick(Action *action);
	/// Handler for clicking the Preview graphic.
	void previewClick(Action *action);
	/// Draws UFO.
	void drawUfo();
	/// Draws projectiles.
	void drawProjectile(const CraftWeaponProjectile* p);
	/// Animates craft damage.
	void animateCraftDamage();
	/// Updates craft damage.
	void drawCraftDamage();
	/// Draws craft shield on sprite
	void drawCraftShield();
	/// Toggles usage of weapons.
	void weaponClick(Action *action);
	/// Changes colors of weapon icons, range indicators and ammo texts base on current weapon state.
	void recolor(const int weaponNo, const bool currentState);
	/// Returns true if state is minimized.
	bool isMinimized() const;
	/// Sets state minimized or maximized.
	void setMinimized(const bool minimized);
	/// Handler for clicking the minimized interception window icon.
	void btnMinimizedIconClick(Action *action);
	/// Gets interception number.
	int getInterceptionNumber() const;
	/// Sets interception number.
	void setInterceptionNumber(const int number);
	/// Sets interceptions count.
	void setInterceptionsCount(const size_t count);
	/// Calculates window position according to opened interception windows.
	void calculateWindowPosition();
	/// Moves window to new position.
	void moveWindow();
	/// Checks if the dogfight should be ended.
	bool dogfightEnded() const;
	/// Gets pointer to the UFO in this dogfight.
	Ufo *getUfo() const;
	/// Gets pointer to the craft in this dogfight.
	Craft *getCraft() const;
	/// Waits until the UFO reaches a polygon.
	void setWaitForPoly(bool wait);
	/// Waits until the UFO reaches a polygon.
	bool getWaitForPoly() const;
	/// Waits until the UFO reaches the right altitude.
	void setWaitForAltitude(bool wait);
	/// Waits until the UFO reaches the right altitude.
	bool getWaitForAltitude() const;
	/// Award experience to the pilots.
	void awardExperienceToPilots();
};

}
