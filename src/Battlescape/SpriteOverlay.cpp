#include "SpriteOverlay.h"
#include "InventoryItemSprite.h"
#include "../Engine/Language.h"
#include "../Engine/Script.h"
#include "../Engine/ScriptBind.h"
#include "../Interface/Text.h"
#include "../Interface/NumberText.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"
#include "../Mod/ModScript.h"
#include "../Mod/RuleInventory.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Soldier.h"

namespace OpenXcom
{
/**
 * @brief Draws a scripted sprite overlay.
 * @tparam Battle The battlescape instance of the object associated with this script.
 * @tparam Context Context information to pass in to the script. Mutable.
 * @tparam Callback The ModScript struct to use when calling this script
 * @tparam Rule The rule object associated with the object this script targets.
 * @param animationFrame The current animation frame. Defaults to 0.
*/
template<typename Callback, typename Rule, typename Battle, typename Context>
void SpriteOverlay::draw(const Rule& rule, const Battle* battle, Context* context, int animationFrame)
{
	ModScript::scriptCallback<Callback>(&rule, battle, _save, this, context, animationFrame);
}

/**
 * @brief Draws a scripted sprite overlay for hooks lacking a context.
 * @tparam Battle The battlescape instance of the object associated with this script.
 * @tparam Callback The ModScript struct to use when calling this script
 * @tparam Rule The rule object associated with the object this script targets.
 * @param animationFrame The current animation frame. Defaults to 0.
*/
template<typename Callback, typename Rule, typename Battle>
void SpriteOverlay::draw(const Rule& rule, const Battle* battle, int animationFrame)
{
	ModScript::scriptCallback<Callback>(&rule, battle, _save, this, animationFrame);
}

/// Draw the paperdoll overlay.
template void SpriteOverlay::draw<ModScript::UnitPaperdollOverlay>(const Armor& armor, const BattleUnit* unit, int animationFrame);
/// Draw the unitRank overlay.
template void SpriteOverlay::draw<ModScript::UnitRankOverlay>(const Armor& armor, const BattleUnit* unit, int animationFrame);

/**
 * @brief Draws an overlay for an inventory sprite/bigobj.
 * @param context Struct containing information about the current rendering context and it's options. R/W by the script.
*/
void SpriteOverlay::draw(const RuleItem& ruleItem, const BattleItem* battleItem, InventorySpriteContext* context, int animationFrame)
{
	if (context->options & InventorySpriteContext::DRAW_HAND_OVERLAY) {
		ModScript::scriptCallback<ModScript::HandOverlay>(&ruleItem, battleItem, _save, this, context, animationFrame);
	}
	else {
		ModScript::scriptCallback<ModScript::InventorySpriteOverlay>(&ruleItem, battleItem, _save, this, context, animationFrame);
	}
};

/// Draws a number on the surface the sprite targets.
void SpriteOverlay::drawNumber(int value, int x, int y, int width, int height, int color)
{
	_numberRender.clear();
	_numberRender.setX(_bounds.x + x);
	_numberRender.setY(_bounds.y + y);

	// avoid resizing if possible.
	if (width > _numberRender.getWidth()) { _numberRender.setWidth(width); }
	if (height != _numberRender.getHeight()) { _numberRender.setWidth(height); }

	_numberRender.setPalette(_target.getPalette());
	_numberRender.setColor(static_cast<Uint8>(color));  
	_numberRender.setBordered(false);
	_numberRender.setValue(value);
	_numberRender.blit(_target.getSurface());
}

/// Draws text on on the surface the sprite targets.
void SpriteOverlay::drawText(const std::string& text, int x, int y, int width, int height, int color)
{
	auto surfaceText = Text(width, height, _bounds.x + x, _bounds.y + y);
	surfaceText.setPalette(_target.getPalette());
	auto temp = Language(); // note getting the selected language is tough here, and might not even be what is wanted.
	const auto mod = _save->getMod();
	surfaceText.initText(mod->getFont("FONT_BIG"), mod->getFont("FONT_SMALL"), &temp);
	surfaceText.setSmall();
	surfaceText.setColor(color);
	surfaceText.setText(text);
	surfaceText.blit(_target.getSurface());
}


void SpriteOverlay::blit(const Surface* source, int x, int y)
{
	source->blitNShade(&_target, _bounds.x + x, _bounds.y + y, 0, false, 0);
}

void SpriteOverlay::blitCrop(const Surface* source, int x1, int y1, int x2, int y2)
{
	const auto crop = GraphSubset({_bounds.x + x1, _bounds.x + x2}, {_bounds.y + y1, _bounds.y + y2});
	source->blitNShade(&_target, _bounds.x, _bounds.y, 0, crop);
}

void SpriteOverlay::blitShadeCrop(const Surface* source, int shade, int x, int y, int x1, int y1, int x2, int y2)
{
	const auto crop = GraphSubset({x1, x2}, {y1, y2});
	source->blitNShade(&_target, _bounds.x + x, _bounds.y + y, shade, crop);
}

void SpriteOverlay::blitShade(const Surface* source, int x, int y, int shade)
{
	source->blitNShade(&_target, _bounds.x + x, _bounds.y + y, shade, false, 0);
}

void SpriteOverlay::blitShadeRecolor(const Surface* source, int x, int y, int shade, int newColor)
{
	source->blitNShade(&_target, _bounds.x, _bounds.y, shade, false, newColor);
}

void SpriteOverlay::drawLine(int x1, int y1, int x2, int y2, int color)
{
	_target.drawLine(_bounds.x + x1, _bounds.y + y1, _bounds.x + x2, _bounds.y + y2, color);
}

void SpriteOverlay::drawRect(int x1, int y1, int x2, int y2, int color)
{
	_target.drawRect(_bounds.x + x1, _bounds.y + y1, _bounds.x + x2, _bounds.y + y2, color);
}

void SpriteOverlay::drawCirc(int x, int y, int radius, int color)
{
	_target.drawCircle(_bounds.x + x, _bounds.y + y, radius, color);
}

//// Script binding
namespace SpriteOverlayScript {

std::string debugDisplayScript(const SpriteOverlay* overlay)
{
	if (overlay == nullptr) { return "null"; }

	std::ostringstream output;
	output << " (x:" << overlay->_bounds.x << " y:" << overlay->_bounds.y << " w:" << overlay->_bounds.w << " h:" << overlay->_bounds.h << ")";
	return output.str();
}

} // SpriteOverlayScript

void SpriteOverlay::ScriptRegister(ScriptParserBase* parser)
{
	Bind<SpriteOverlay> spriteOverlayBinder = { parser };

	spriteOverlayBinder.add<&SpriteOverlay::blit>("blit", "Blits a sprite onto the overlay. (sprite x y)");
	spriteOverlayBinder.add<&SpriteOverlay::blitCrop>("blitCrop", "Blits a sprite onto the overlay with a crop. (sprite x y cropX1 cropY1 cropX2 cropY2)");
	spriteOverlayBinder.add<&SpriteOverlay::blitShade>("blitShade", "Blits and shades a sprite onto the overlay. (sprite x y shade)");
	spriteOverlayBinder.add<&SpriteOverlay::blitShadeRecolor>("blitShadeRecolor", "Blits, shades, and recolors a sprite onto the overlay. (sprite x y shade color)");

	spriteOverlayBinder.add<&SpriteOverlay::drawNumber>("drawNumber", "Draws number on the overlay. (number x y width height color)");
	spriteOverlayBinder.add<&SpriteOverlay::drawText>("drawText", "Draws text on the overlay. (text x y width height color");

	spriteOverlayBinder.add<&SpriteOverlay::drawLine>("drawLine", "Draws a line on the overlay. (x1 y1 x2 y2 color)");
	spriteOverlayBinder.add<&SpriteOverlay::drawRect>("drawRect", "Draws a rectangle on the overlay. (x1 y1 x2 y2 color)");
	spriteOverlayBinder.add<&SpriteOverlay::drawCirc>("drawCirc", "Draws a circle on the overlay. (x y radius color)");

	spriteOverlayBinder.add<&SpriteOverlay::getWidth>("getWidth", "Gets the width of this overlay.");
	spriteOverlayBinder.add<&SpriteOverlay::getHeight>("getHeight", "Gets the height of this overlay.");

	spriteOverlayBinder.addDebugDisplay<&SpriteOverlayScript::debugDisplayScript>();
}

/**
 * Constructor of inventory sprite overlay script parser.
 */
ModScript::InventorySpriteOverlayParser::InventorySpriteOverlayParser(ScriptGlobal* shared, const std::string& name, Mod* mod)
	: ScriptParserEvents{ shared, name, "item", "battle_game", "overlay", "render_context", "anim_frame" }
{
	BindBase bindBase{ this };
	bindBase.addCustomPtr<const Mod>("rules", mod);

	bindBase.parser->registerPointerType<InventorySpriteContext>();
}

/**
 * Constructor of hand overlay script parser.
 */
ModScript::HandOverlayParser::HandOverlayParser(ScriptGlobal* shared, const std::string& name, Mod* mod)
	: ScriptParserEvents{ shared, name, "item", "battle_game", "overlay", "render_context", "anim_frame" }
{
	BindBase bindBase{ this };
	bindBase.addCustomPtr<const Mod>("rules", mod);

	bindBase.parser->registerPointerType<InventorySpriteContext>();
}

/**
 * Constructor of hand overlay script parser.
 */
ModScript::UnitPaperdollOverlayParser::UnitPaperdollOverlayParser(ScriptGlobal* shared, const std::string& name, Mod* mod)
	: ScriptParserEvents{ shared, name, "unit", "battle_game", "overlay", "anim_frame" }
{
	BindBase bindBase{ this };
	bindBase.addCustomPtr<const Mod>("rules", mod);
}

/**
 * Constructor of hand overlay script parser.
 */
ModScript::UnitRankOverlayParser::UnitRankOverlayParser(ScriptGlobal* shared, const std::string& name, Mod* mod)
	: ScriptParserEvents{ shared, name, "unit", "battle_game", "overlay", "anim_frame" }
{
	BindBase bindBase{ this };
	bindBase.addCustomPtr<const Mod>("rules", mod);
}

}
