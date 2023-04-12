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

/// Draw the paperdoll overlay.
template void SpriteOverlay::draw<ModScript::UnitPaperdollOverlay>(const Armor& armor, const BattleUnit* unit, Soldier* soldier, int animationFrame);
/// Draw the unitRank overlay.
template void SpriteOverlay::draw<ModScript::UnitRankOverlay>(const Armor& armor, const BattleUnit* unit, Soldier* soldier, int animationFrame);

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

//// Script binding
namespace SpriteOverlayScript {

/// Draws a number on the surface the sprite targets.
void drawNumber(SpriteOverlay* dest, int value, int x, int y, int width, int height, int color)
{
	dest->_numberRender.clear();
	dest->_numberRender.setX(dest->_bounds.x + x);
	dest->_numberRender.setY(dest->_bounds.y + y);

	// avoid resizing if possible.
	if (width > dest->_numberRender.getWidth()) { dest->_numberRender.setWidth(width); }
	if (height != dest->_numberRender.getHeight()) { dest->_numberRender.setWidth(height); }

	dest->_numberRender.setPalette(dest->_target.getPalette());
	dest->_numberRender.setColor(static_cast<Uint8>(color));  
	dest->_numberRender.setBordered(false);
	dest->_numberRender.setValue(value);
	dest->_numberRender.blit(dest->_target.getSurface());
}

/// Draws text on on the surface the sprite targets.
void drawText(SpriteOverlay* dest, const std::string& text, int x, int y, int width, int height, int color)
{
	auto surfaceText = Text(width, height, dest->_bounds.x + x, dest->_bounds.y + y);
	surfaceText.setPalette(dest->_target.getPalette());
	auto temp = Language(); // note getting the selected language is tough here, and might not even be what is wanted.
	const auto mod = dest->_save->getMod();
	surfaceText.initText(mod->getFont("FONT_BIG"), mod->getFont("FONT_SMALL"), &temp);
	surfaceText.setSmall();
	surfaceText.setColor(color);
	surfaceText.setText(text);
	surfaceText.blit(dest->_target.getSurface());
}


void blit(SpriteOverlay* dest, const Surface* source, int x, int y)
{
	source->blitNShade(&dest->_target, dest->_bounds.x + x, dest->_bounds.y + y, 0, false, 0);
}

void blitCrop(SpriteOverlay* dest, const Surface* source, int x1, int y1, int x2, int y2)
{
	const auto crop = GraphSubset({dest->_bounds.x + x1, dest->_bounds.x + x2}, {dest->_bounds.y + y1, dest->_bounds.y + y2});
	source->blitNShade(&dest->_target, dest->_bounds.x, dest->_bounds.y, 0, crop);
}

void blitShadeCrop(SpriteOverlay* dest, const Surface* source, int shade, int x, int y, int x1, int y1, int x2, int y2)
{
	const auto crop = GraphSubset({x1, x2}, {y1, y2});
	source->blitNShade(&dest->_target, dest->_bounds.x + x, dest->_bounds.y + y, shade, crop);
}

void blitShade(SpriteOverlay* dest, const Surface* source, int x, int y, int shade)
{
	source->blitNShade(&dest->_target, dest->_bounds.x + x, dest->_bounds.y + y, shade, false, 0);
}

void blitShadeRecolor(SpriteOverlay* dest, const Surface* source, int x, int y, int shade, int newColor)
{
	source->blitNShade(&dest->_target, dest->_bounds.x, dest->_bounds.y, shade, false, newColor);
}

void drawLine(SpriteOverlay* dest, int x1, int y1, int x2, int y2, int color)
{
	dest->_target.drawLine(dest->_bounds.x + x1, dest->_bounds.y + y1, dest->_bounds.x + x2, dest->_bounds.y + y2, color);
}

void drawRect(SpriteOverlay* dest, int x1, int y1, int x2, int y2, int color)
{
	dest->_target.drawRect(dest->_bounds.x + x1, dest->_bounds.y + y1, dest->_bounds.x + x2, dest->_bounds.y + y2, color);
}

void drawCirc(SpriteOverlay* dest, int x, int y, int radius, int color)
{
	dest->_target.drawCircle(dest->_bounds.x + x, dest->_bounds.y + y, radius, color);
}

std::string debugDisplayScript(const SpriteOverlay* overlay)
{
	if (overlay)
	{
		std::ostringstream output;
		output << " (x:" << overlay->_bounds.x << " y:" << overlay->_bounds.y << " w:" << overlay->_bounds.w << " h:" << overlay->_bounds.h << ")";
		return output.str();
	}
	else
	{
		return "null";
	}
}

} // SpriteOverlayScript

void SpriteOverlay::ScriptRegister(ScriptParserBase* parser)
{
	Bind<SpriteOverlay> spriteOverlayBinder = { parser };

	spriteOverlayBinder.add<&SpriteOverlayScript::blit>("blit", "Blits a sprite onto the overlay. (sprite x y)");
	spriteOverlayBinder.add<&SpriteOverlayScript::blitCrop>("blitCrop", "Blits a sprite onto the overlay with a crop. (sprite x y cropX1 cropY1 cropX2 cropY2)");
	spriteOverlayBinder.add<&SpriteOverlayScript::blitShade>("blitShade", "Blits and shades a sprite onto the overlay. (sprite x y shade)");
	spriteOverlayBinder.add<&SpriteOverlayScript::blitShadeRecolor>("blitShadeRecolor", "Blits, shades, and recolors a sprite onto the overlay. (sprite x y shade color)");

	spriteOverlayBinder.add<&SpriteOverlayScript::drawNumber>("drawNumber", "Draws number on the overlay. (number x y width height color)");
	spriteOverlayBinder.add<&SpriteOverlayScript::drawText>("drawText", "Draws text on the overlay. (text x y width height color");

	spriteOverlayBinder.add<&SpriteOverlayScript::drawLine>("drawLine", "Draws a line on the overlay. (x1 y1 x2 y2 color)");
	spriteOverlayBinder.add<&SpriteOverlayScript::drawRect>("drawRect", "Draws a rectangle on the overlay. (x1 y1 x2 y2 color)");
	spriteOverlayBinder.add<&SpriteOverlayScript::drawCirc>("drawCirc", "Draws a circle on the overlay. (x y radius color)");

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
	: ScriptParserEvents{ shared, name, "unit", "battle_game", "overlay", "soldier", "anim_frame" }
{
	BindBase bindBase{ this };
	bindBase.addCustomPtr<const Mod>("rules", mod);
}

/**
 * Constructor of hand overlay script parser.
 */
ModScript::UnitRankOverlayParser::UnitRankOverlayParser(ScriptGlobal* shared, const std::string& name, Mod* mod)
	: ScriptParserEvents{ shared, name, "unit", "battle_game", "overlay", "soldier", "anim_frame" }
{
	BindBase bindBase{ this };
	bindBase.addCustomPtr<const Mod>("rules", mod);
}

}
