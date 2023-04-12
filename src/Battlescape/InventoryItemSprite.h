#pragma once
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
#include "../Engine/Script.h"
#include "../Interface/NumberText.h"
#include "../Savegame/BattleItem.h"

namespace OpenXcom
{

class Surface;
class SurfaceSet;
class SavedBattleGame;
class BattleItem;
class RuleItem;
class ScriptParserBase;

/// Struct associating the sprites render context with its render options.
struct InventorySpriteContext
{
	/// The context in which this class is being rendered. Immutable.
	const enum RenderContext : int
	{
		SCREEN_INVENTORY	= 1 << 0,
		SCREEN_BATTSCAPE	= 1 << 1,
		SCREEN_ALIEN_INV	= 1 << 2,
		SCREEN_UFOPEDIA		= 1 << 3,
		CURSOR_HOVER		= 1 << 4, /// The cursor is hovering over this item in the inventory window.
		CURSOR_SELECTED		= 1 << 5, /// Item is grabed/selected by the cursor in the inventory window.
		INVENTORY_AMMO		= 1 << 6, /// Item is being render in the inventory ammo window.
	} renderContext;

	/// The options to use when displaying this sprite. Can be changed via scripting.
	enum OverlayOptions : int
	{
		DRAW_NONE			= 0,
		DRAW_GRENADE		= 1 << 0, /// draw the grenade primed indicator
		DRAW_CORPSE_STATE	= 1 << 1, /// draw the corpse state indicator. Note by default there are no sprites associated with this.
		DRAW_FATAL_WOUNDS	= 1 << 2, /// draw the number of fatal wounds.
		DRAW_AMMO			= 1 << 3, /// draw the ammo indicator.
		DRAW_MEDIKIT		= 1 << 4, /// draw the medikit quantity indicator.
		DRAW_TWOHAND		= 1 << 5, /// draw the two-hand indicator.
		DRAW_HAND_OVERLAY	= 1 << 6, /// Item is being rendered with a hand-overlay. (This value should only get set by Sprite Overlay)
		DRAW_ALL = INT_MAX,
	} options;

	// standard contexts

	static const InventorySpriteContext SOLDIER_INV_HAND;
	static const InventorySpriteContext SOLDIER_INV_SLOT;
	static const InventorySpriteContext SOLDIER_INV_GROUND;
	static const InventorySpriteContext ALIEN_INV_HAND;
	static const InventorySpriteContext BATTSCAPE_HAND;
	static const InventorySpriteContext SOLDIER_INV_AMMO;
	static const InventorySpriteContext SOLDIER_INV_CURSOR;

	/// returns a copy of the object with the new value or-ed in.
	InventorySpriteContext with(RenderContext otherContext) const
	{
		return InventorySpriteContext{ static_cast<RenderContext>(renderContext | otherContext), options };
	}

	static constexpr const char* ScriptName = "InvSpriteContext";
	static void ScriptRegister(ScriptParserBase* parser);
};

inline constexpr InventorySpriteContext InventorySpriteContext::SOLDIER_INV_HAND{ RenderContext::SCREEN_INVENTORY, static_cast<OverlayOptions>(DRAW_GRENADE | DRAW_TWOHAND)};
inline constexpr InventorySpriteContext InventorySpriteContext::SOLDIER_INV_SLOT{ RenderContext::SCREEN_INVENTORY, OverlayOptions::DRAW_GRENADE};
inline constexpr InventorySpriteContext InventorySpriteContext::SOLDIER_INV_GROUND{	RenderContext::SCREEN_INVENTORY, static_cast<OverlayOptions>(DRAW_GRENADE | DRAW_FATAL_WOUNDS | DRAW_CORPSE_STATE )};
inline constexpr InventorySpriteContext InventorySpriteContext::SOLDIER_INV_CURSOR{	static_cast<RenderContext>(CURSOR_SELECTED | SCREEN_INVENTORY), OverlayOptions::DRAW_NONE};
inline constexpr InventorySpriteContext InventorySpriteContext::SOLDIER_INV_AMMO{ RenderContext::SCREEN_INVENTORY, OverlayOptions::DRAW_NONE};
inline constexpr InventorySpriteContext InventorySpriteContext::ALIEN_INV_HAND{	RenderContext::SCREEN_ALIEN_INV, OverlayOptions::DRAW_NONE};
inline constexpr InventorySpriteContext InventorySpriteContext::BATTSCAPE_HAND{	RenderContext::SCREEN_BATTSCAPE, static_cast<OverlayOptions>(DRAW_GRENADE | DRAW_AMMO | DRAW_MEDIKIT | DRAW_TWOHAND)};

/// Namespace for segregating InventorySpriteContext scripting functions.
namespace InventorySpriteContextScript
{
	inline void getContext(InventorySpriteContext* context, int& result) { result = context->renderContext; }
	/// Checks if a given render context is valid for the current context.
	inline void isInContext(InventorySpriteContext* context, int& result, int contextOption) { result = contextOption & context->renderContext; }

	typedef InventorySpriteContext::OverlayOptions OverlayOptions;
	inline void getRenderOptions(InventorySpriteContext* context, int& result) { result = context->options; }
	inline void isRenderOptionSet(InventorySpriteContext* context, int& result, int options) { result = options & context->options; }
	inline void setRenderOptions(InventorySpriteContext* context, int options) { context->options = static_cast<OverlayOptions>(context->options | options); }
	inline void unsetRenderOptions(InventorySpriteContext* context, int options) { context->options = static_cast<OverlayOptions>(context->options & ~options); }
}

/**
 * @brief Handles rendering of an inventory item sprite, including all scripting. Non-owening and should not leave the local scope.
*/
class InventoryItemSprite
{
private:
	/// Worker for pixel level script blitting.
	ScriptWorkerBlit _scriptWorker{};
	const BattleItem* _battleItem;
	const RuleItem& _ruleItem;
	const SavedBattleGame* _save;
	/// Surface this item should be draw to.
	Surface& _target;
	/// Bounding box for the sprite.
	const SDL_Rect _bounds;
	/// Number Text context provided for rendering/scripting.
	NumberText _numberRender = NumberText(4, 5);

public:
	/**
	 * @brief Creates a new inventory item sprite.
	 * @param target The surface the item is going to be rendered to.
	 * @param spriteBounds The bounds of the sprite. Not necessarily equal to target's area.
	*/
	InventoryItemSprite(const BattleItem& battleItem, const SavedBattleGame* save, Surface& target, const SDL_Rect& spriteBounds)
		: _battleItem(&battleItem), _ruleItem(*battleItem.getRules()), _save(save), _target(target), _bounds(spriteBounds) {}

	/**
	 * @brief Creates a new inventory item sprite. Bounds are assumed to be equal to target's dimensions with x and y of 0, 0.
	 * @param target The surface the item is going to be rendered to.
	*/
	InventoryItemSprite(const BattleItem& battleItem, const SavedBattleGame* save, Surface& target)
		: _battleItem(&battleItem), _ruleItem(*battleItem.getRules()), _save(save), _target(target),
		  _bounds(SDL_Rect{ 0, 0, static_cast<Uint16>(_target.getWidth()), static_cast<Uint16>(target.getHeight()) }) {}
	

	/// Draw the sprite, including scripted recolor and blitting.
	void draw(const SurfaceSet& surfaceSet, InventorySpriteContext context, int animationFrame = 0);
	/// Draw the hand overlay, which does not include the sprite.
	void drawHandOverlay(InventorySpriteContext context, int animationFrame = 0);

private:
	void drawGrenadePrimedIndicator(int animationFrame) const;
	void drawAmmoIndicator();
	void drawMedkitIndicator();
	void drawTwoHandIndicator();
	void drawCorpseIndicator(int animationFrame) const;
	void drawFatalWoundIndicator();

	/// Draws a number in a given corner.
	template <typename CornerFunc>
	void drawNumberCorner(CornerFunc getChildCoord, int spacing, int number, int color, int yRowOffset = 0, bool bordered = false);
};

}
