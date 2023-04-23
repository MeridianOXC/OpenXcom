/*
 * Copyright 2010-2023 OpenXcom Developers.
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
#include <SDL.h>
#include "InventoryItemSprite.h"
#include "SpriteOverlay.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Script.h"
#include "../Engine/ScriptBind.h"
#include "../Mod/Mod.h"
#include "../Mod/ModScript.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleEnviroEffects.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Interface/Text.h"
#include "../Interface/NumberText.h"

namespace OpenXcom
{

/**
 * @brief Draws the inventory item sprite in the current context.
 * @param surfaceSet The set that contains this sprite (should be BIGOBS.PCK).
*/
void InventoryItemSprite::draw(const SurfaceSet& surfaceSet, InventorySpriteContext context, int animationFrame)
{
	const Surface* sprite = getBigSprite(&surfaceSet, _save, animationFrame);

	if (!sprite)
	{
		Log(LOG_WARNING) << "Attempted to draw item without sprite. Item: " << _ruleItem.getType();
		return;
	}

	BattleItem::ScriptFill(&_scriptWorker, _battleItem, _save, BODYPART_ITEM_INVENTORY, animationFrame, 0);
	_scriptWorker.executeBlit(sprite, &_target, _bounds.x, _bounds.y, 0);

	SpriteOverlay(_target, _bounds, _save).draw(_ruleItem, _battleItem, &context, animationFrame);

	if (context.options & InventorySpriteContext::DRAW_GRENADE) { drawGrenadePrimedIndicator(animationFrame); }
	if (context.options & InventorySpriteContext::DRAW_FATAL_WOUNDS) { drawFatalWoundIndicator(); }
	if (context.options & InventorySpriteContext::DRAW_CORPSE_STATE) { drawCorpseIndicator(animationFrame); }
}

/**
 * @brief Draws the hand-slot overlays, including related scripting.
*/
void InventoryItemSprite::drawHandOverlay(InventorySpriteContext context, int animationFrame)
{
	context.options = static_cast<InventorySpriteContext::OverlayOptions>(context.options | InventorySpriteContext::DRAW_HAND_OVERLAY);
	SpriteOverlay(_target, _bounds, _save).draw(_ruleItem, _battleItem, &context, animationFrame);

	if (context.options & InventorySpriteContext::DRAW_AMMO) { drawAmmoIndicator(); }
	if (context.options & InventorySpriteContext::DRAW_MEDIKIT) { drawMedkitIndicator(); }
	if (context.options & InventorySpriteContext::DRAW_TWOHAND) { drawTwoHandIndicator(); }
}

std::tuple<int, int, Uint16, Uint16> getDimensions(const RuleItem& ruleItem)
{
	// item dimensions in inventory units.
	int invSlotW = ruleItem.getInventoryWidth();
	int invSlotH = ruleItem.getInventoryHeight();

	// sprite bounding dimensions in pixels.
	Uint16 itemW = invSlotW * RuleInventory::SLOT_W;
	Uint16 itemH = invSlotH * RuleInventory::SLOT_H;

	return std::tuple{ invSlotW ,invSlotH, itemW, itemH };
}

/**
 * @brief Gets the bounds for proper display of an inventory sprite relative to it's inventory slot.
 * @param groundOffset the number of inventory units the ground inventory slot is offset.
 * @return A SDL_Rect describing the sprite's bounds, relative to it's inventory slot.
*/
SDL_Rect InventoryItemSprite::getInvSpriteBounds(const BattleItem& battleItem)
{
	assert(battleItem.getSlot()->getType() == INV_SLOT && "getInvSpriteBounds called on non-inventory item.");
	const auto itemRules = battleItem.getRules();

	// sprite bounding dimensions in pixels.
	Uint16 itemW = itemRules->getInventoryWidth()  * RuleInventory::SLOT_W;
	Uint16 itemH = itemRules->getInventoryHeight() * RuleInventory::SLOT_H;

	// offset the box by its place in inventory container * slot size.
	Sint16 itemX = battleItem.getSlotX() * RuleInventory::SLOT_W;
	Sint16 itemY = battleItem.getSlotY() * RuleInventory::SLOT_H;

	return SDL_Rect{ itemX, itemY, itemW, itemH };
}

SDL_Rect InventoryItemSprite::getGroundSlotSpriteBounds(const BattleItem& battleItem, int groundOffset)
{
	assert(battleItem.getSlot()->getType() == INV_GROUND && "getGroundSlotSpriteBounds called on non-ground item.");
	const auto itemRules = battleItem.getRules();

	// sprite bounding dimensions in pixels.
	Uint16 itemW = itemRules->getInventoryWidth()  * RuleInventory::SLOT_W;
	Uint16 itemH = itemRules->getInventoryHeight() * RuleInventory::SLOT_H;

	// offset the box by place in the ground container, after taking into account the ground offset (in inventory units)
	Sint16 itemX = (battleItem.getSlotX() - groundOffset) * RuleInventory::SLOT_W;
	Sint16 itemY = battleItem.getSlotY() * RuleInventory::SLOT_H;

	return SDL_Rect{ itemX, itemY, itemW, itemH };
}

SDL_Rect InventoryItemSprite::getHandCenteredSpriteBounds(const RuleItem& ruleItem)
{
	// no assert, calling this on items not in hand is potentially valid.

	// item dimensions in inventory units.
	int invSlotW = ruleItem.getInventoryWidth();
	int invSlotH = ruleItem.getInventoryHeight();

	// sprite bounding dimensions in pixels.
	Uint16 itemW = invSlotW * RuleInventory::SLOT_W;
	Uint16 itemH = invSlotH * RuleInventory::SLOT_H;

	// position item by half the difference in item size and hand slot size in order to center.
	Sint16 itemX = (RuleInventory::HAND_W - invSlotW) * RuleInventory::SLOT_W / 2;
	Sint16 itemY = (RuleInventory::HAND_H - invSlotH) * RuleInventory::SLOT_H / 2;

	return SDL_Rect{ itemX, itemY, itemW, itemH };
}

/**
 * Gets the item's inventory sprite.
 * @return Return current inventory sprite.
 */
const Surface* InventoryItemSprite::getBigSprite(const SurfaceSet* set, const SavedBattleGame* save, int animFrame) const
{
	int spriteIndex = _ruleItem.getBigSprite();
	if (spriteIndex == -1) { return nullptr; }

	const Surface* ruleSurf = set->getFrame(spriteIndex);
	//enforce compatibility with basic version
	if (ruleSurf == nullptr)
	{
		throw Exception("Image missing in 'BIGOBS.PCK' for item '" + _ruleItem.getType() + "'");
	}

	spriteIndex = ModScript::scriptFunc2<ModScript::SelectItemSprite>(
		&_ruleItem,
		spriteIndex, 0,
		_battleItem, save, BODYPART_ITEM_INVENTORY, animFrame, 0
	);

	auto* scriptSurf = set->getFrame(spriteIndex);
	return scriptSurf != nullptr ? scriptSurf : ruleSurf;
}

namespace // some short helper functions.
{
/**
 * @brief Transforms a number into its position in a triangle wave.
 * For example, triangleWave(x, 8, 4) => (0, 1, 2, 3, 4, 3, 2, 1)
*/
int triangleWave(int number, int period, int amplitude){ return abs((number % period) - amplitude); }

/// Gets the number of digits in a positive number less than 1000.
int getDigits(int number) { return number < 10 ? 1 : number < 100 ? 2 : 3; }

std::pair<int, int> topLeft(const SDL_Rect& bounds, int numW, int numH, int spacing)
{
	return std::pair{bounds.x + spacing, bounds.y + spacing};
}

std::pair<int, int> topRight(const SDL_Rect& bounds, int numW, int numH, int spacing)
{
	return std::pair{bounds.x + bounds.w - numW - spacing, bounds.y + spacing};
}

std::pair<int, int> bottomLeft(const SDL_Rect& bounds, int numW, int numH, int spacing)
{
	return std::pair{bounds.x + spacing, bounds.y + bounds.h - numH - spacing};
}

std::pair<int, int> bottomRight(const SDL_Rect& bounds, int numW, int numH, int spacing)
{
	return std::pair{bounds.x + bounds.w - numW - spacing, bounds.y + bounds.h - numH - spacing};
}

} // namespace

/**
 * @brief Draws a numberText relative to a corner of this overlays bounding box.
 * @param getChildCoord Function to get the appropriate corner offset.
 * @param spacing Amount of additional space to move from the corner.
 * @param number The number to draw (must be positive and less than 1000).
 * @param yRowOffset an optional additional y-offset. The number will be rendered at (numHeight + 1) * yRowOffset.
 * @param bordered if the number should be bordered or not.
*/
template <typename CornerFunc>
void InventoryItemSprite::drawNumberCorner(const CornerFunc getChildCoord, int spacing, int number, int color, int yRowOffset, bool bordered)
{
	int numW = getDigits(number) * (bordered ? 5 : 4); /// width of number.
	int numH = bordered ? 6 : 5;                       /// height of number.

	auto [x, y] = getChildCoord(_bounds, numW, numH, spacing);
	_numberRender.setX(x);
	_numberRender.setY(y + yRowOffset * (numH + 1));

	// avoid resizing if possible.
	if (numW > _numberRender.getWidth()) { _numberRender.setWidth(numW); }
	if (numH != _numberRender.getHeight()) { _numberRender.setHeight(numH); }

	_numberRender.setColor(color);
	_numberRender.setBordered(bordered);
	_numberRender.setValue(number);
	_numberRender.setPalette(_target.getPalette());

	_numberRender.blit(_target.getSurface());
}

void InventoryItemSprite::drawGrenadePrimedIndicator(int animationFrame) const
{
	if (_battleItem->getFuseTimer() < 0) { return; }

	/// TODO: This should be const, but the get methods are not.
	const Surface* primedIndicator = const_cast<Mod&>(_mod).getSurfaceSet("SCANG.DAT")->getFrame(6);
	// if the grenade is primed, but without the fuse enabled, it gets a grey indicator. This is used for flares in XCF.
	int newColor = _battleItem->isFuseEnabled() ? 0 : 32; /// TODO: these colors should be moved to the interface.

	primedIndicator->blitNShade(&_target, _bounds.x, _bounds.y, triangleWave(animationFrame, 8, 4), false, newColor);
}

namespace
{
Surface* getCorpseStateIndicator(const SavedBattleGame& save, const Mod& mod, const BattleUnit& unit) {
	/// TODO: This should be const, but the get methods are not.
	Mod& mutableMod = const_cast<Mod&>(*save.getMod());
	const auto enviro = save.getEnviroEffects();

	if (unit.getFire() > 0) { return mutableMod.getSurface("BigBurnIndicator", false); }
	if (unit.getFatalWounds() > 0) { return mutableMod.getSurface("BigWoundIndicator", false); }
	if (unit.hasNegativeHealthRegen())
	{
		return enviro && !enviro->getInventoryShockIndicator().empty() ? mutableMod.getSurface(enviro->getInventoryShockIndicator(), false)
																	   : mutableMod.getSurface("BigShockIndicator", false);
	}
	return mutableMod.getSurface("BigStunIndicator", false);
}
}

void InventoryItemSprite::drawCorpseIndicator(int animationFrame) const
{
	/// TODO: Implemented to prevent a regression in functionality, but this might be better as a script.
	const auto unit = _battleItem->getUnit();
	if (!unit || unit->getStatus() != STATUS_UNCONSCIOUS) { return; }

	if (const Surface* corpseStateIndicator = getCorpseStateIndicator(*_save, _mod, *unit))
	{
		corpseStateIndicator->blitNShade(&_target, _bounds.x, _bounds.y, triangleWave(animationFrame, 8, 4));
	}
}

void InventoryItemSprite::drawFatalWoundIndicator()
{
	const auto unit = _battleItem->getUnit();
	if (!unit || unit->getStatus() != STATUS_UNCONSCIOUS) { return; }

	int woundCount = unit->getFatalWounds();

	/// TODO: this element should be replaced with a more descriptively named element.
	int woundColor = getInterfaceElementMember(_mod, "inventory", "numStack", &Element::color2).value_or(0);

	drawNumberCorner(bottomRight, 0, woundCount, woundColor, 0, true);
}

void InventoryItemSprite::drawAmmoIndicator()
{
	int ammoColor = _battleItem->getSlot()->isRightHand() ? getInterfaceElementMember(_mod, "battlescape", "numAmmoRight", &Element::color).value_or(0) :
					_battleItem->getSlot()->isLeftHand()  ? getInterfaceElementMember(_mod, "battlescape", "numAmmoLeft", &Element::color).value_or(0)
														  : throw std::logic_error("item in hand with bad hand value.");

	for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
	{
		if (_battleItem->isAmmoVisibleForSlot(slot))
		{
			const BattleItem* ammo = _battleItem->getAmmoForSlot(slot);
			int ammoQuant = ammo ? ammo->getAmmoQuantity() : 0;

			drawNumberCorner(topLeft, 0, ammoQuant, ammoColor, slot);
		}
	}
}

void InventoryItemSprite::drawMedkitIndicator()
{
	if (_ruleItem.getBattleType() != BT_MEDIKIT) { return; }

	int medkitColor = _battleItem->getSlot()->isRightHand() ? getInterfaceElementMember(_mod, "battlescape", "numMedikitRight", &Element::color).value_or(0) :
					  _battleItem->getSlot()->isLeftHand()  ? getInterfaceElementMember(_mod, "battlescape", "numMedikitLeft", &Element::color).value_or(0)
														    : throw std::logic_error("item in hand with bad hand value.");

	drawNumberCorner(bottomLeft, 1, _battleItem->getPainKillerQuantity(), medkitColor, -2);
	drawNumberCorner(bottomLeft, 1, _battleItem->getStimulantQuantity(), medkitColor, -1);
	drawNumberCorner(bottomLeft, 1, _battleItem->getHealQuantity(), medkitColor, 0);
}

void InventoryItemSprite::drawTwoHandIndicator()
{
	if (!_ruleItem.isTwoHanded()) { return; }

	int color = _ruleItem.isBlockingBothHands() ? getInterfaceElementMember(_mod, "battlescape", "twoHandedRed", &Element::color).value_or(0)
			  									: getInterfaceElementMember(_mod, "battlescape", "twoHandedGreen", &Element::color).value_or(0);

	drawNumberCorner(bottomRight, 1, 2, color);
}

/// Register the constants and options appropriate for InventorySpriteContext in scripting. Be aware this has some limited r/w capabilities.
void InventorySpriteContext::ScriptRegister(ScriptParserBase* parser)
{
	Bind<InventorySpriteContext> invSpriteContextBinder = { parser };

	invSpriteContextBinder.addCustomConst("INV_SLOT_W", RuleInventory::SLOT_W);
	invSpriteContextBinder.addCustomConst("INV_SLOT_H", RuleInventory::SLOT_H);
	invSpriteContextBinder.addCustomConst("INV_HAND_SLOT_COUNT_W", RuleInventory::HAND_W);
	invSpriteContextBinder.addCustomConst("INV_HAND_SLOT_COUNT_H", RuleInventory::HAND_H);
	invSpriteContextBinder.addCustomConst("INV_HAND_OVERLAY_W", RuleInventory::HAND_SLOT_W);
	invSpriteContextBinder.addCustomConst("INV_HAND_OVERLAY_H", RuleInventory::HAND_SLOT_H);

	invSpriteContextBinder.addCustomConst("DRAW_GRENADE_INDICATOR", InventorySpriteContext::DRAW_GRENADE);
	invSpriteContextBinder.addCustomConst("DRAW_CORPSE_STATE", InventorySpriteContext::DRAW_CORPSE_STATE);
	invSpriteContextBinder.addCustomConst("DRAW_FATAL_WOUNDS", InventorySpriteContext::DRAW_FATAL_WOUNDS);
	invSpriteContextBinder.addCustomConst("DRAW_AMMO", InventorySpriteContext::DRAW_AMMO);
	invSpriteContextBinder.addCustomConst("DRAW_MEDIKIT", InventorySpriteContext::DRAW_MEDIKIT);
	invSpriteContextBinder.addCustomConst("DRAW_TWOHAND_INDICATOR", InventorySpriteContext::DRAW_TWOHAND);

	invSpriteContextBinder.add<&InventorySpriteContextScript::isRenderOptionSet>("isRenderOptionSet", "Gets if a render option is set or not (via bitwise &). (result, option)");
	invSpriteContextBinder.add<&InventorySpriteContextScript::getRenderOptions>("getRenderOptions", "Gets the current render options.");
	invSpriteContextBinder.add<&InventorySpriteContextScript::setRenderOptions>("setRenderOptions", "Sets (turns on) a given render option or options.");
	invSpriteContextBinder.add<&InventorySpriteContextScript::unsetRenderOptions>("unsetRenderOptions", "Unsets (turns off) a given render option or options.");

	invSpriteContextBinder.addCustomConst("CURSOR_HOVER", InventorySpriteContext::CURSOR_HOVER);
	invSpriteContextBinder.addCustomConst("CURSOR_SELECTED", InventorySpriteContext::CURSOR_SELECTED);
	invSpriteContextBinder.addCustomConst("INVENTORY_AMMO", InventorySpriteContext::INVENTORY_AMMO);
	invSpriteContextBinder.addCustomConst("SCREEN_INVENTORY", InventorySpriteContext::SCREEN_INVENTORY);
	invSpriteContextBinder.addCustomConst("SCREEN_ALIEN_INV", InventorySpriteContext::SCREEN_ALIEN_INV);
	invSpriteContextBinder.addCustomConst("SCREEN_BATTSCAPE", InventorySpriteContext::SCREEN_BATTSCAPE);

	invSpriteContextBinder.add<&InventorySpriteContextScript::isInContext>("isInContext", "Gets if the overlay is in a given context or not (via bitwise &). (result option)");
	invSpriteContextBinder.add<&InventorySpriteContextScript::getContext>("getContext", "Gets the current render context.");
}

}
