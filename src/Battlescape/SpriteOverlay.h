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
#include "InventoryItemSprite.h"
#include "../Interface/NumberText.h"

namespace OpenXcom
{

class SpriteOverlay;
class Surface;
class ScriptParserBase;
class SavedBattleGame;

/// Namespace for segregating SpriteOverlay scripting functions.
namespace SpriteOverlayScript
{
std::string debugDisplayScript(const SpriteOverlay* overlay);
}

/**
 * @brief Class for rendering an overlay on top of an existing sprite via scripting.
*/
class SpriteOverlay
{
private:
	/// Surface this item should be draw to.
	Surface& _target;
	/// Bounding box for the sprite.
	const SDL_Rect _bounds;
	/// Battlescape Instance we are working with.
	const SavedBattleGame* _save;
	/// Number Text context provided for rendering/scripting.
	NumberText _numberRender = NumberText(4, 5);

public:
	/**
	 * @brief Creates a new InventoryItemSprite should not be stored or persist beyond the local scope.
	 * @param target The surface the sprite is render to.
	 * @param spriteBounds The bounds the sprite and scripting should work with, relative to target. The sprite is rendered at x, y of this rect, and that coordinate is treated as 0,0 for scripting.
	*/
	SpriteOverlay(Surface& target, const SDL_Rect& spriteBounds, const SavedBattleGame *save)
		: _target(target), _bounds(spriteBounds), _save(save) {}

	// no copy or move.
	SpriteOverlay(SpriteOverlay&) = delete;
	SpriteOverlay& operator=(SpriteOverlay&) = delete;

	/// Draw the scripted overlay.
	template<typename Callback, typename Rule, typename Battle, typename Context>
	void draw(const Rule& rule, const Battle* battle, Context* context, int animationFrame = 0);

	/// Draw the scripted overlay for hooks lacking a context object.
	template<typename Callback, typename Rule, typename Battle>
	void draw(const Rule& rule, const Battle* battle, int animationFrame = 0);

	void draw(const RuleItem& ruleItem, const BattleItem* battleItem, InventorySpriteContext* context, int animationFrame);

	// these methods are exposed for scripting.

	/// Gets the width of this overlay.
	[[nodiscard]] int getWidth() const { return _bounds.w; }
	/// Gets the height of this overlay.
	[[nodiscard]] int getHeight() const { return _bounds.h; }

	friend std::string SpriteOverlayScript::debugDisplayScript(const SpriteOverlay* overlay);

	/// Gets a bounding box based on a surface's size, with the x and y set to 0.
	static SDL_Rect getSurfaceBounds(const Surface& surface)
	{
		return SDL_Rect{ 0, 0, static_cast<Uint16>(surface.getWidth()), static_cast<Uint16>(surface.getHeight()) };
	}

	/// Name of class used in script.
	static constexpr const char* ScriptName = "SpriteOverlay";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

private:
	void drawNumber(int value, int x, int y, int width, int height, int color);
	void drawText(const std::string& text, int x, int y, int width, int height, int color);

	void blit(const Surface* source, int x, int y);
	void blitCrop(const Surface* source, int x1, int y1, int x2, int y2);
	void blitShadeCrop(const Surface* source, int shade, int x, int y, int x1, int y1, int x2, int y2);
	void blitShade(const Surface* source, int x, int y, int shade);
	void blitShadeRecolor(const Surface* source, int x, int y, int shade, int newColor);
	void drawLine(int x1, int y1, int x2, int y2, int color);
	void drawRect(int x1, int y1, int x2, int y2, int color);
	void drawCirc(int x, int y, int radius, int color);
};

}
