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
void drawNumber(SpriteOverlay* dest, int value, int x, int y, int width, int height, int color);
void drawText(SpriteOverlay* dest, const std::string& text, int width, int height, int x, int y, int color);

void blit(SpriteOverlay* dest, const Surface* source, int x, int y);
void blitCrop(SpriteOverlay* dest, const Surface* source, int x1, int y1, int x2, int y2);
void blitShadeCrop(SpriteOverlay* dest, const Surface* source, int shade, int x, int y, int x1, int y1, int x2, int y2);
void blitShade(SpriteOverlay* dest, const Surface* source, int x, int y, int shade);
void blitShadeRecolor(SpriteOverlay* dest, const Surface* source, int x, int y, int shade, int newColor);

void drawLine(SpriteOverlay* dest, int x1, int y1, int x2, int y2, int color);
void drawRect(SpriteOverlay* dest, int x1, int y1, int x2, int y2, int color);
void drawCirc(SpriteOverlay* dest, int x, int y, int radius, int color);

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
	/**
	 * @brief Creates a new InventoryItemSprite should not be stored or persist beyond the local scope.
	 * @param target The surface the sprite is rendered to. The bounds are infered to be the size of this surface, with a x,y of 0, 0.
	*/
	SpriteOverlay(Surface& target, const SavedBattleGame* save)
		: _target(target), _bounds(SDL_Rect{ 0, 0, static_cast<Uint8>(_target.getWidth()), static_cast<Uint8>(_target.getHeight()) }), _save(save) { }

	// no copy or move.
	SpriteOverlay(SpriteOverlay&) = delete;
	SpriteOverlay& operator=(SpriteOverlay&) = delete;

	/// Draw the scripted overlay.
	template<typename Callback, typename Rule, typename Battle, typename Context>
	void draw(const Rule& rule, const Battle* battle, Context* context, int animationFrame = 0);

	void draw(const RuleItem& ruleItem, const BattleItem* battleItem, InventorySpriteContext* context, int animationFrame);

	// these methods are exposed for scripting.

	/// Gets the width of this overlay.
	[[nodiscard]] int getWidth() const { return _bounds.w; }
	/// Gets the height of this overlay.
	[[nodiscard]] int getHeight() const { return _bounds.h; }

	friend void SpriteOverlayScript::drawNumber(SpriteOverlay* dest, int value, int x, int y, int width, int height, int color);
	friend void SpriteOverlayScript::drawText(SpriteOverlay* dest, const std::string& text, int x, int y, int width, int height, int color);

	friend void SpriteOverlayScript::blit(SpriteOverlay* dest, const Surface* source, int x, int y);
	friend void SpriteOverlayScript::blitCrop(SpriteOverlay* dest, const Surface* source, int x1, int y1, int x2, int y2);
	friend void SpriteOverlayScript::blitShadeCrop(SpriteOverlay* dest, const Surface* source, int shade, int x, int y, int x1, int y1, int x2, int y2);
	friend void SpriteOverlayScript::blitShade(SpriteOverlay* dest, const Surface* source, int x, int y, int shade);
	friend void SpriteOverlayScript::blitShadeRecolor(SpriteOverlay* dest, const Surface* source, int x, int y, int shade, int newColor);
	friend void SpriteOverlayScript::drawLine(SpriteOverlay* dest, int x1, int y1, int x2, int y2, int color);
	friend void SpriteOverlayScript::drawRect(SpriteOverlay* dest, int x1, int y1, int x2, int y2, int color);
	friend void SpriteOverlayScript::drawCirc(SpriteOverlay* dest, int x, int y, int radius, int color);

	friend std::string SpriteOverlayScript::debugDisplayScript(const SpriteOverlay* overlay);

	/// Name of class used in script.
	static constexpr const char* ScriptName = "SpriteOverlay";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);
};

}
