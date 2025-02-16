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
#include "TestState.h"
#include "TestPaletteState.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Mod/Mod.h"
#include "../Engine/Exception.h"
#include "../Engine/FileMap.h"
#include "../Engine/Logger.h"
#include "../Engine/Palette.h"
#include "../Mod/Armor.h"
#include "../Mod/ExtraSprites.h"
#include "../Mod/RuleTerrain.h"
#include "../Mod/MapBlock.h"
#include "../Mod/MapDataSet.h"
#include "../Mod/MapData.h"
#include "../Mod/RuleUfo.h"
#include "../Mod/RuleGlobe.h"
#include "../Mod/Texture.h"
#include "../Mod/MapScript.h"
#include "../Mod/AlienDeployment.h"

namespace OpenXcom
{

// items are (roughly) ordered by frequency of use... to save a few CPU cycles when looking for a match
static std::map<int, PaletteTestMetadata> _paletteMetadataMap =
{
	{ 1, PaletteTestMetadata("UFO_PAL_BATTLEPEDIA", 1, 255, 0, "Palettes/UFO-JASC/PAL_BATTLEPEDIA.pal", false) },
	{ 2, PaletteTestMetadata("UFO_PAL_BATTLESCAPE", 1, 255, 0, "Palettes/UFO-JASC/PAL_BATTLESCAPE.pal", false) },

	{ 3, PaletteTestMetadata("UFO_PAL_BATTLE_COMMON", 1, 239, 0, "Palettes/UFO-JASC/PAL_BATTLE_COMMON.pal", false) }, // ignore last 16 colors

	{ 4, PaletteTestMetadata("TFTD_PAL_BATTLESCAPE", 1, 254, 3, "Palettes/TFTD-JASC/PAL_BATTLESCAPE.pal", false) }, // ignore last color too

	{ 5, PaletteTestMetadata("UFO_PAL_UFOPAEDIA", 1, 255, 0, "Palettes/UFO-JASC/PAL_UFOPAEDIA.pal", false) },
	{ 6, PaletteTestMetadata("TFTD_PAL_BASESCAPE", 1, 255, 0, "Palettes/TFTD-JASC/PAL_BASESCAPE.pal", true) }, // TFTD's ufopedia

	{ 7, PaletteTestMetadata("UFO_PAL_BASESCAPE", 1, 255, 0, "Palettes/UFO-JASC/PAL_BASESCAPE.pal", true) },

	{ 8, PaletteTestMetadata("UFO_PAL_GEOSCAPE", 1, 255, 0, "Palettes/UFO-JASC/PAL_GEOSCAPE.pal", true) },
	{ 9, PaletteTestMetadata("TFTD_PAL_GEOSCAPE", 1, 255, 0, "Palettes/TFTD-JASC/PAL_GEOSCAPE.pal", true) },

	{ 10, PaletteTestMetadata("TFTD_PAL_BATTLESCAPE_1", 1, 254, 3, "Palettes/TFTD-JASC/PAL_BATTLESCAPE_1.pal", false) },
	{ 11, PaletteTestMetadata("TFTD_PAL_BATTLESCAPE_2", 1, 254, 3, "Palettes/TFTD-JASC/PAL_BATTLESCAPE_2.pal", false) },
	{ 12, PaletteTestMetadata("TFTD_PAL_BATTLESCAPE_3", 1, 254, 3, "Palettes/TFTD-JASC/PAL_BATTLESCAPE_3.pal", false) },

	{ 13, PaletteTestMetadata("UFO_PAL_BACKGROUND_SAFE", 0, 255, 0, "Palettes/UFO-JASC-SAFE/PAL_BACKGROUND_SAFE.pal", false) },
	{ 14, PaletteTestMetadata("TFTD_PAL_BACKGROUND_SAFE", 0, 255, 0, "Palettes/TFTD-JASC-SAFE/PAL_BACKGROUND_SAFE.pal", false) },

	{ 15, PaletteTestMetadata("UFO_PAL_GRAPHS", 1, 255, 0, "Palettes/UFO-JASC/PAL_GRAPHS.pal", false) },
	{ 16, PaletteTestMetadata("TFTD_PAL_GRAPHS", 1, 255, 0, "Palettes/TFTD-JASC/PAL_GRAPHS.pal", false) }
};

/**
 * Initializes all the elements in the test screen.
 */
TestState::TestState()
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(300, 17, 10, 7);
	_txtPalette = new Text(66, 9, 10, 30);
	_cbxPalette = new ComboBox(this, 114, 16, 78, 26);
	_cbxPaletteAction = new ComboBox(this, 114, 16, 198, 26);
	_txtTestCase = new Text(66, 9, 10, 50);
	_cbxTestCase = new ComboBox(this, 234, 16, 78, 46);
	_txtDescription = new Text(300, 25, 10, 66);
	_lstOutput = new TextList(284, 80, 10, 94);
	_btnRun = new TextButton(146, 16, 10, 176);
	_btnCancel = new TextButton(146, 16, 164, 176);

	// Set palette
	setInterface("tests");

	add(_window, "window", "tests");
	add(_txtTitle, "heading", "tests");
	add(_txtPalette, "text", "tests");
	add(_txtTestCase, "text", "tests");
	add(_txtDescription, "heading", "tests");
	add(_lstOutput, "text", "tests");
	add(_btnRun, "button2", "tests");
	add(_btnCancel, "button2", "tests");
	add(_cbxTestCase, "button1", "tests"); // add as last (display over all other components)
	add(_cbxPalette, "button1", "tests"); // add as last (display over all other components)
	add(_cbxPaletteAction, "button1", "tests"); // add as last (display over all other components)

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "tests");

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TEST_SCREEN"));

	_txtPalette->setText(tr("STR_PALETTE"));

	for (auto& pal : _game->getMod()->getPalettes())
	{
		if (pal.first.find("BACKUP_") != 0)
		{
			_paletteList.push_back(pal.first);
		}
	}

	_cbxPalette->setOptions(_paletteList, true);

	std::vector<std::string> _actionList;
	_actionList.push_back("STR_PREVIEW");
	_actionList.push_back("STR_TINY_BORDERLESS");
	_actionList.push_back("STR_TINY_BORDER");
	_actionList.push_back("STR_SMALL_LOW_CONTRAST");
	_actionList.push_back("STR_SMALL_HIGH_CONTRAST");
	_actionList.push_back("STR_BIG_LOW_CONTRAST");
	_actionList.push_back("STR_BIG_HIGH_CONTRAST");

	_cbxPaletteAction->setOptions(_actionList, true);
	_cbxPaletteAction->onChange((ActionHandler)&TestState::cbxPaletteAction);

	_txtTestCase->setText(tr("STR_TEST_CASE"));

	_testCases.push_back("STR_BAD_NODES");
	_testCases.push_back("STR_MCD_CHECK");
	_testCases.push_back("STR_PALETTE_CHECK");
	_testCases.push_back("STR_SCRIPT_TAGS");
	_testCases.push_back("STR_MAP_RESOURCES");

	_cbxTestCase->setOptions(_testCases, true);
	_cbxTestCase->onChange((ActionHandler)&TestState::cbxTestCaseChange);

	_txtDescription->setWordWrap(true);
	cbxTestCaseChange(0); // update description

	_lstOutput->setColumns(1, 284);
	_lstOutput->setBackground(_window);
	_lstOutput->setWordWrap(true);

	_btnRun->setText(tr("STR_RUN"));
	_btnRun->onMouseClick((ActionHandler)&TestState::btnRunClick);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&TestState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&TestState::btnCancelClick, Options::keyCancel);
}

TestState::~TestState()
{
	for (auto& item : _vanillaPalettes)
	{
		delete item.second;
	}
}

/**
* Updates the test case description.
* @param action Pointer to an action.
*/
void TestState::cbxTestCaseChange(Action *)
{
	_lstOutput->clearList();

	size_t index = _cbxTestCase->getSelected();
	_txtDescription->setText(tr(_testCases[index]+"_DESC"));
}

/**
* Returns to the previous screen.
* @param action Pointer to an action.
*/
void TestState::btnRunClick(Action *action)
{
	_lstOutput->clearList();

	size_t index = _cbxTestCase->getSelected();
	switch (index)
	{
		case 0: testCase0(); break;
		case 1: testCase1(); break;
		case 2: testCase2(); break;
		case 3: testCase3(); break;
		case 4: testCase4(); break;
		default: break;
	}
}

/**
* Returns to the previous screen.
* @param action Pointer to an action.
*/
void TestState::btnCancelClick(Action *action)
{
	_game->popState();
}

/**
 * Shows palette preview.
 * @param action Pointer to an action.
 */
void TestState::cbxPaletteAction(Action *action)
{
	size_t index = _cbxPalette->getSelected();
	const std::string palette = _paletteList[index];

	PaletteActionType type = (PaletteActionType)_cbxPaletteAction->getSelected();

	_game->pushState(new TestPaletteState(palette, type));
}

void TestState::testCase4()
{
	_lstOutput->addRow(1, tr("STR_TESTS_STARTING").c_str());

	int total = 0;

	// build a list of all terrains
	std::map<std::string, int> terrainMap;
	for (auto& texturePair : _game->getMod()->getGlobe()->getTexturesRaw())
	{
		for (auto& terrainCrit : *texturePair.second->getTerrain())
			terrainMap[terrainCrit.name] += 1;
		for (auto& baseTerrainCrit : *texturePair.second->getBaseTerrain())
			terrainMap[baseTerrainCrit.name] += 1;
	}
	for (auto& terrainName : _game->getMod()->getTerrainList())
	{
		terrainMap[terrainName] += 1;
	}
	for (auto& deployName : _game->getMod()->getDeploymentsList())
	{
		AlienDeployment *deployRule = _game->getMod()->getDeployment(deployName);
		for (auto& terrainName : deployRule->getTerrains())
			terrainMap[terrainName] += 1;
	}
	for (auto& mapScript : _game->getMod()->getMapScriptsRaw())
	{
		for (auto* mapScriptCommand : mapScript.second)
		{
			for (auto& terrainName : mapScriptCommand->getRandomAlternateTerrain())
			{
				terrainMap[terrainName] += 1;
			}
		}
	}

	// erase false positives
	terrainMap.erase("");
	terrainMap.erase("baseTerrain");
	terrainMap.erase("globeTerrain");

	// 1. check terrain existence in ruleset
	Log(LOG_INFO) << "----------------------------------------------1. check terrain existence in ruleset";
	for (auto& pair : terrainMap)
	{
		RuleTerrain *tRule = _game->getMod()->getTerrain(pair.first);
		if (!tRule)
		{
			++total;
			Log(LOG_INFO) << "Terrain '" << pair.first << "' does not exist!";
		}
	}

	// build a list of all mapblocks and mapdatasets
	std::map<std::string, int> blockMap;
	std::map<std::string, int> datasetMap;

	auto addMapblockAndDataset = [](RuleTerrain *terrainRule, std::map<std::string, int> &blockMapRef, std::map<std::string, int> &datasetMapRef, int maxSkinIndex = 0)
	{
		if (terrainRule)
		{
			for (auto* mapblock : *terrainRule->getMapBlocks())
			{
				std::string uc = mapblock->getName();
				Unicode::upperCase(uc);
				blockMapRef[uc] += 1;
			}
			for (int skinIndex = 0; skinIndex <= maxSkinIndex; ++skinIndex)
			{
				for (auto* dataset : *terrainRule->getMapDataSets())
				{
					std::string uc = dataset->getName();
					Unicode::upperCase(uc);
					if (skinIndex > 0 && uc != "BLANKS")
					{
						uc = uc + "_" + std::to_string(skinIndex);
					}
					datasetMapRef[uc] += 1;
				}
			}
		}
	};

	for (auto& pair : terrainMap)
	{
		RuleTerrain *terrainRule = _game->getMod()->getTerrain(pair.first);
		addMapblockAndDataset(terrainRule, blockMap, datasetMap);
	}
	for (auto& ufoName : _game->getMod()->getUfosList())
	{
		RuleUfo *ufoRule = _game->getMod()->getUfo(ufoName);
		RuleTerrain *terrainRule = ufoRule->getBattlescapeTerrainData();
		addMapblockAndDataset(terrainRule, blockMap, datasetMap);
	}
	for (auto& craftName : _game->getMod()->getCraftsList())
	{
		RuleCraft *craftRule = _game->getMod()->getCraft(craftName);
		RuleTerrain *terrainRule = craftRule->getBattlescapeTerrainData();
		addMapblockAndDataset(terrainRule, blockMap, datasetMap, craftRule->getMaxSkinIndex());
	}

	// 2. check for existence of mapblock MAP and RMP files
	Log(LOG_INFO) << "----------------------------------------------2. check for existence of mapblock MAP and RMP files";

	auto checkExistence = [](std::map<std::string, int> &mapRef, const std::string &dir, const std::string &ext, int &totalRef)
	{
		for (auto& mapItem : mapRef)
		{
			std::ostringstream filename;
			filename << dir << mapItem.first << ext;
			if (!FileMap::fileExists(filename.str()))
			{
				++totalRef;
				Log(LOG_INFO) << filename.str() << " not found";
			}
		}
	};

	checkExistence(blockMap, "MAPS/", ".MAP", total);
	checkExistence(blockMap, "ROUTES/", ".RMP", total);

	// 3. check for existence of mapdataset MCD, PCK and TAB files
	Log(LOG_INFO) << "----------------------------------------------3. check for existence of mapdataset MCD, PCK and TAB files";

	checkExistence(datasetMap, "TERRAIN/", ".MCD", total);
	checkExistence(datasetMap, "TERRAIN/", ".PCK", total);
	checkExistence(datasetMap, "TERRAIN/", ".TAB", total);

	// 4. check for unused mapblock MAP and RMP files
	Log(LOG_INFO) << "----------------------------------------------4. check for unused mapblock MAP and RMP files";

	auto findUnusedFiles = [](std::map<std::string, int> &mapRef, const std::string &dir, int &totalRef)
	{
		const auto& contents = FileMap::getVFolderContents(dir);
		for (const auto& name : contents)
		{
			std::string upper = name; // copy
			Unicode::upperCase(upper);
			std::string noExt = CrossPlatform::noExt(upper);
			if (mapRef.find(noExt) == mapRef.end())
			{
				std::string fullpath = FileMap::at(dir + upper)->fullpath;

				// exceptions
				if (fullpath.find("/standard/xcom1/") != std::string::npos) continue;
				if (fullpath.find("/UFO/MAPS/") != std::string::npos) continue;
				if (fullpath.find("/UFO/ROUTES/") != std::string::npos) continue;
				if (fullpath.find("/TFTD/MAPS/") != std::string::npos) continue;
				if (fullpath.find("/TFTD/ROUTES/") != std::string::npos) continue;
				if (fullpath.find("/TFTD/TERRAIN/") != std::string::npos) continue;

				++totalRef;
				Log(LOG_INFO) << fullpath << " not used anywhere.";
			}
		}
	};

	findUnusedFiles(blockMap, "MAPS/", total);
	findUnusedFiles(blockMap, "ROUTES/", total);

	// 5. check for unused mapdataset MCD, PCK and TAB files
	Log(LOG_INFO) << "----------------------------------------------5. check for unused mapdataset MCD, PCK and TAB files";

	findUnusedFiles(datasetMap, "TERRAIN/", total);


	if (total > 0)
	{
		_lstOutput->addRow(1, tr("STR_TESTS_ERRORS_FOUND").arg(total).c_str());
		_lstOutput->addRow(1, tr("STR_DETAILED_INFO_IN_LOG_FILE").c_str());
	}
	else
	{
		_lstOutput->addRow(1, tr("STR_TESTS_NO_ERRORS_FOUND").c_str());
	}
	_lstOutput->addRow(1, tr("STR_TESTS_FINISHED").c_str());
}

void TestState::testCase3()
{
	_lstOutput->addRow(1, tr("STR_TESTS_STARTING").c_str());

	std::map<const Armor *, std::map<std::string, int> > tagMatrix;
	for (auto& armorName : _game->getMod()->getArmorsList())
	{
		auto* armorRule = _game->getMod()->getArmor(armorName, true);
		auto& tagValues = armorRule->getScriptValuesRaw().getValuesRaw();
		ArgEnum index = ScriptParserBase::getArgType<ScriptTag<Armor>>();
		auto& tagNames = _game->getMod()->getScriptGlobal()->getTagNames().at(index);
		for (size_t i = 0; i < tagValues.size(); ++i)
		{
			std::string nameAsString = tagNames.values[i].name.toString().substr(4);
			tagMatrix[armorRule][nameAsString] = tagValues.at(i);
		}
	}

	std::map<std::string, bool> tagNames;
	for (auto& a : tagMatrix)
	{
		for (auto& t : a.second)
		{
			tagNames[t.first] = true;
		}
	}

	Log(LOG_INFO) << "----------------------------------------------";
	Log(LOG_INFO) << "Copy/paste into a CSV file";
	Log(LOG_INFO) << "----------------------------------------------";
	{
		std::ostringstream ssNames;
		ssNames << ";Armor";
		for (auto& n : tagNames)
		{
			ssNames << ";" << n.first;
		}
		Log(LOG_INFO) << ssNames.str();
	}
	for (auto& armorName : _game->getMod()->getArmorsList())
	{
		auto* armorRule = _game->getMod()->getArmor(armorName, true);
		auto& armorData = tagMatrix[armorRule];
		std::ostringstream ss;
		ss << ";" << armorName;
		for (auto& t : tagNames)
		{
			ss << ";";
			auto findIt = armorData.find(t.first);
			if (findIt != armorData.end())
			{
				ss << (*findIt).second;
			}
		}
		Log(LOG_INFO) << ss.str();
	}
	Log(LOG_INFO) << "----------------------------------------------";
	Log(LOG_INFO) << "End of export";
	Log(LOG_INFO) << "----------------------------------------------";

	_lstOutput->addRow(1, tr("STR_TESTS_FINISHED").c_str());
}

void TestState::testCase2()
{
	_lstOutput->addRow(1, tr("STR_TESTS_STARTING").c_str());
	if (_vanillaPalettes.empty())
	{
		for (auto& item : _paletteMetadataMap)
		{
			_vanillaPalettes[item.first] = new Palette();
			_vanillaPalettes[item.first]->initBlack();

			// Load from JASC file
			auto palFile = FileMap::getIStream(item.second.palettePath);
			if (palFile)
			{
				std::string line;
				std::getline(*palFile, line); // header
				std::getline(*palFile, line); // file format
				std::getline(*palFile, line); // number of colors
				int r = 0, g = 0, b = 0;
				for (int j = 0; j < 256; ++j)
				{
					std::getline(*palFile, line); // j-th color index
					std::stringstream ss(line);
					ss >> r;
					ss >> g;
					ss >> b;
					_vanillaPalettes[item.first]->copyColor(j, r, g, b); // raw RGB copy, no side effects!
				}
			}
			else
			{
				throw Exception(item.second.palettePath + " not found");
			}
		}
	}

	int total = 0;
	for (auto& i : _game->getMod()->getExtraSprites())
	{
		std::string sheetName = i.first;
		if (sheetName.find("_CPAL") != std::string::npos)
		{
			// custom palettes cannot be matched, skip
			continue;
		}

		for (auto* spritePack : i.second)
		{
		if (spritePack->getSingleImage())
		{
			const std::string& relPath = (*spritePack->getSprites())[0];
			total += checkPalette(relPath, spritePack->getWidth(), spritePack->getHeight());
		}
		else
		{
			for (auto& j : *spritePack->getSprites())
			{
				int startFrame = j.first;
				std::string fileName = j.second;
				if (fileName.substr(fileName.length() - 1, 1) == "/")
				{
					const auto& contents = FileMap::getVFolderContents(fileName);
					for (const auto& name : contents)
					{
						if (!ExtraSprites::isImageFile(name))
							continue;
						try
						{
							std::string relPath = fileName + name;
							total += checkPalette(relPath, spritePack->getWidth(), spritePack->getHeight());
						}
						catch (Exception &e)
						{
							Log(LOG_WARNING) << e.what();
						}
					}
				}
				else
				{
					if (spritePack->getSubX() == 0 && spritePack->getSubY() == 0)
					{
						total += checkPalette(fileName, spritePack->getWidth(), spritePack->getHeight());
					}
					else
					{
						const std::string& relPath = (*spritePack->getSprites())[startFrame];
						total += checkPalette(relPath, spritePack->getWidth(), spritePack->getHeight());
					}
				}
			}
		}
		}
	}

	if (total > 0)
	{
		_lstOutput->addRow(1, tr("STR_TESTS_ERRORS_FOUND").arg(total).c_str());
		_lstOutput->addRow(1, tr("STR_DETAILED_INFO_IN_LOG_FILE").c_str());
	}
	else
	{
		_lstOutput->addRow(1, tr("STR_TESTS_NO_ERRORS_FOUND").c_str());
	}
	_lstOutput->addRow(1, tr("STR_TESTS_FINISHED").c_str());
}

int TestState::checkPalette(const std::string& fullPath, int width, int height)
{
	Surface *image = new Surface(width, height);
	image->loadImage(fullPath);

	SDL_Palette *palette = image->getSurface()->format->palette;
	if (!palette)
	{
		Log(LOG_ERROR) << "Image doesn't have a palette at all! Full path: " << fullPath;
		delete image;
		return 1;
	}

	int ncolors = image->getSurface()->format->palette->ncolors;
	if (ncolors != 256)
	{
		Log(LOG_ERROR) << "Image palette doesn't have 256 colors! Full path: " << fullPath;
	}

	int bestMatch = 0;
	int matchedPaletteIndex = 0;
	for (auto& item : _vanillaPalettes)
	{
		int match = matchPalette(image, item.first, item.second);
		if (match > bestMatch)
		{
			bestMatch = match;
			matchedPaletteIndex = item.first;
		}
		if (match == 100)
		{
			break;
		}
	}

	delete image;

	if (bestMatch < 100)
	{
		Log(LOG_INFO) << "Best match: " << bestMatch << "%; palette: " << _paletteMetadataMap[matchedPaletteIndex].paletteName << "; path: " << fullPath;
		return 1;
	}

	return 0;
}

int TestState::matchPalette(Surface *image, int index, Palette *test)
{
	SDL_Color *colors = image->getSurface()->format->palette->colors;
	int matched = 0;

	int firstIndexToCheck = _paletteMetadataMap[index].firstIndexToCheck;
	int lastIndexToCheck = _paletteMetadataMap[index].lastIndexToCheck;
	int maxTolerance = _paletteMetadataMap[index].maxTolerance;
	bool usesBackPals = _paletteMetadataMap[index].usesBackPals;

	for (int i = firstIndexToCheck; i <= lastIndexToCheck; i++)
	{
		if (usesBackPals)
		{
			if (i >= 224 && i <= 239)
			{
				// don't check and consider matched
				matched++;
				continue;
			}
		}

		Uint8 rdiff = (colors[i].r > test->getColors(i)->r ? colors[i].r - test->getColors(i)->r : test->getColors(i)->r - colors[i].r);
		Uint8 gdiff = (colors[i].g > test->getColors(i)->g ? colors[i].g - test->getColors(i)->g : test->getColors(i)->g - colors[i].g);
		Uint8 bdiff = (colors[i].b > test->getColors(i)->b ? colors[i].b - test->getColors(i)->b : test->getColors(i)->b - colors[i].b);

		if (rdiff <= maxTolerance && gdiff <= maxTolerance && bdiff <= maxTolerance)
		{
			matched++;
		}
	}

	return matched * 100 / (lastIndexToCheck - firstIndexToCheck + 1);
}

void TestState::testCase1()
{
	_lstOutput->addRow(1, tr("STR_TESTS_STARTING").c_str());
	_lstOutput->addRow(1, tr("STR_CHECKING_TERRAIN").c_str());
	int total = 0;
	std::map<std::string, std::set<int>> uniqueResults;
	for (auto& terrainName : _game->getMod()->getTerrainList())
	{
		RuleTerrain *terrainRule = _game->getMod()->getTerrain(terrainName);
		total += checkMCD(terrainRule, uniqueResults);
	}
	_lstOutput->addRow(1, tr("STR_CHECKING_UFOS").c_str());
	for (auto& ufoName : _game->getMod()->getUfosList())
	{
		RuleUfo *ufoRule = _game->getMod()->getUfo(ufoName);
		RuleTerrain *terrainRule = ufoRule->getBattlescapeTerrainData();
		if (!terrainRule)
		{
			continue;
		}
		total += checkMCD(terrainRule, uniqueResults);
	}
	_lstOutput->addRow(1, tr("STR_CHECKING_CRAFT").c_str());
	for (auto& craftName : _game->getMod()->getCraftsList())
	{
		RuleCraft *craftRule = _game->getMod()->getCraft(craftName);
		RuleTerrain *terrainRule = craftRule->getBattlescapeTerrainData();
		if (!terrainRule)
		{
			continue;
		}
		for (int skinIndex = 0; skinIndex <= craftRule->getMaxSkinIndex(); ++skinIndex)
		{
			terrainRule->refreshMapDataSets(skinIndex, _game->getMod()); // change skin
			total += checkMCD(terrainRule, uniqueResults);
		}
	}
	if (total > 0)
	{
		_lstOutput->addRow(1, tr("STR_TESTS_ERRORS_FOUND").arg(total).c_str());
		_lstOutput->addRow(1, tr("STR_DETAILED_INFO_IN_LOG_FILE").c_str());
	}
	else
	{
		_lstOutput->addRow(1, tr("STR_TESTS_NO_ERRORS_FOUND").c_str());
	}

	// summary (unique)
	if (total > 0)
	{
		Log(LOG_INFO) << "----------";
		Log(LOG_INFO) << "SUMMARY";
		Log(LOG_INFO) << "----------";
		for (auto& mapItem : uniqueResults)
		{
			std::ostringstream ss;
			ss << mapItem.first << ": ";
			bool first = true;
			for (int setItem : mapItem.second)
			{
				if (!first)
				{
					ss << ", ";
				}
				else
				{
					first = false;
				}
				ss << setItem;
			}
			std::string line = ss.str();
			Log(LOG_INFO) << line;
		}
	}
	_lstOutput->addRow(1, tr("STR_TESTS_FINISHED").c_str());
}

int TestState::checkMCD(RuleTerrain *terrainRule, std::map<std::string, std::set<int>> &uniqueResults)
{
	int MAX_LOFT_ID = (_game->getMod()->getVoxelData()->size() / 16) - 1;
	int errors = 0;
	for (auto* myMapDataSet : *terrainRule->getMapDataSets())
	{
		int index = 0;
		myMapDataSet->loadData(_game->getMod()->getMCDPatch(myMapDataSet->getName()), false);
		int size = (int)(myMapDataSet->getObjectsRaw()->size());
		for (auto* myMapData : *myMapDataSet->getObjectsRaw())
		{
			// Check for 0 armor
			if (myMapData->getArmor() == 0)
			{
				std::ostringstream ss;
				ss << "terrain: " << terrainRule->getName() << " dataset: " << myMapDataSet->getName() << " object " << index << " has armor: " << myMapData->getArmor();
				std::string str = ss.str();
				Log(LOG_ERROR) << "Error in " << str << ". Found using OXCE test cases.";
				errors++;
				uniqueResults[myMapDataSet->getName()].insert(index);
			}

			// Check for invalid LOFTemps
			for (int layer = 0; layer < 12; ++layer)
			{
				if (myMapData->getLoftID(layer) < 0 || myMapData->getLoftID(layer) > MAX_LOFT_ID)
				{
					std::ostringstream ss;
					ss << "terrain: " << terrainRule->getName() << " dataset: " << myMapDataSet->getName() << " object " << index << " has LoftID: " << myMapData->getLoftID(layer);
					std::string str = ss.str();
					Log(LOG_ERROR) << "Error in " << str << ". MAX_LOFT_ID = " << MAX_LOFT_ID << ". Found using OXCE test cases.";
					errors++;
					uniqueResults[myMapDataSet->getName()].insert(index);
				}
			}

			// Validate MCD references
			if (myMapData->getDieMCD() >= size)
			{
				std::ostringstream ss;
				ss << "terrain: " << terrainRule->getName() << " dataset: " << myMapDataSet->getName() << " object " << index << " has invalid DieMCD: " << myMapData->getDieMCD();
				std::string str = ss.str();
				Log(LOG_ERROR) << "Error in " << str << ". Found using OXCE test cases.";
				errors++;
				uniqueResults[myMapDataSet->getName()].insert(index);
			}
			if (myMapData->getAltMCD() >= size)
			{
				std::ostringstream ss;
				ss << "terrain: " << terrainRule->getName() << " dataset: " << myMapDataSet->getName() << " object " << index << " has invalid AltMCD: " << myMapData->getAltMCD();
				std::string str = ss.str();
				Log(LOG_ERROR) << "Error in " << str << ". Found using OXCE test cases.";
				errors++;
				uniqueResults[myMapDataSet->getName()].insert(index);
			}

			if (myMapData->getObjectType() == O_FLOOR)
			{
				if (myMapData->isNoFloor() && myMapData->getTUCost(MT_WALK) > 0 && myMapData->getTUCost(MT_WALK) < 200 && myMapDataSet->getName() != "BLANKS")
				{
					// Note: pathfinding and autoshots don't work on such ground tiles
					// (such objects were intended to be used only as holes on upper floors/roofs, they should NOT be used on ground floor)
					std::ostringstream ss;
					ss << "terrain: " << terrainRule->getName() << " dataset: " << myMapDataSet->getName() << " object " << index << " is a walkable floor tile with 'No_Floor' flag set to true.";
					std::string str = ss.str();
					Log(LOG_WARNING) << "Potential issue in " << str << ". Found using OXCE test cases.";
					errors++;
					uniqueResults[myMapDataSet->getName()].insert(index);
				}
				if (myMapData->getTUCost(MT_WALK) < 1)
				{
					std::ostringstream ss;
					ss << " walk:" << myMapData->getTUCost(MT_WALK) << " terrain:" << terrainRule->getName() << " dataset:" << myMapDataSet->getName() << " index:" << index;
					std::string str = ss.str();
					Log(LOG_INFO) << "Zero movement cost on floor object: " << str << ". Found using OXCE test cases.";
					errors++;
					uniqueResults[myMapDataSet->getName()].insert(index);
				}
				if (myMapData->getTUCost(MT_FLY) < 1)
				{
					std::ostringstream ss;
					ss << "  fly:" << myMapData->getTUCost(MT_FLY) << " terrain:" << terrainRule->getName() << " dataset:" << myMapDataSet->getName() << " index:" << index;
					std::string str = ss.str();
					Log(LOG_INFO) << "Zero movement cost on floor object: " << str << ". Found using OXCE test cases.";
					errors++;
					uniqueResults[myMapDataSet->getName()].insert(index);
				}
				if (myMapData->getTUCost(MT_SLIDE) < 1)
				{
					std::ostringstream ss;
					ss << "slide:" << myMapData->getTUCost(MT_SLIDE) << " terrain:" << terrainRule->getName() << " dataset:" << myMapDataSet->getName() << " index:" << index;
					std::string str = ss.str();
					Log(LOG_INFO) << "Zero movement cost on floor object: " << str << ". Found using OXCE test cases.";
					errors++;
					uniqueResults[myMapDataSet->getName()].insert(index);
				}
			}
			index++;
		}
		//myMapDataSet->unloadData();
	}
	return errors;
}

void TestState::testCase0()
{
	_lstOutput->addRow(1, tr("STR_TESTS_STARTING").c_str());
	_lstOutput->addRow(1, tr("STR_CHECKING_TERRAIN").c_str());
	int total = 0;
	for (auto& terrainName : _game->getMod()->getTerrainList())
	{
		RuleTerrain *terrRule = _game->getMod()->getTerrain(terrainName);
		for (auto* mapblock : *terrRule->getMapBlocks())
		{
			total += checkRMP(mapblock);
		}
	}
	_lstOutput->addRow(1, tr("STR_CHECKING_UFOS").c_str());
	for (auto& ufoName : _game->getMod()->getUfosList())
	{
		RuleUfo *ufoRule = _game->getMod()->getUfo(ufoName);
		if (ufoRule->getBattlescapeTerrainData())
		{
			for (auto* mapblock : *ufoRule->getBattlescapeTerrainData()->getMapBlocks())
			{
				total += checkRMP(mapblock);
			}
		}
	}
	_lstOutput->addRow(1, tr("STR_CHECKING_CRAFT").c_str());
	for (auto& craftName : _game->getMod()->getCraftsList())
	{
		RuleCraft *craftRule = _game->getMod()->getCraft(craftName);
		if (craftRule->getBattlescapeTerrainData())
		{
			for (auto* mapblock : *craftRule->getBattlescapeTerrainData()->getMapBlocks())
			{
				total += checkRMP(mapblock);
			}
		}
	}
	if (total > 0)
	{
		_lstOutput->addRow(1, tr("STR_TESTS_ERRORS_FOUND").arg(total).c_str());
		_lstOutput->addRow(1, tr("STR_DETAILED_INFO_IN_LOG_FILE").c_str());
	}
	else
	{
		_lstOutput->addRow(1, tr("STR_TESTS_NO_ERRORS_FOUND").c_str());
	}
	_lstOutput->addRow(1, tr("STR_TESTS_FINISHED").c_str());
}

int TestState::checkRMP(MapBlock *mapblock)
{
	// Z-dimension is not in the ruleset, it's hacked on-the-fly
	int mapblockSizeZ = loadMAP(mapblock);

	unsigned char value[24];
	std::ostringstream filename;
	filename << "ROUTES/" << mapblock->getName() << ".RMP";

	// Load file
	auto mapFile = FileMap::getIStream(filename.str());
	if (!mapFile)
	{
		throw Exception(filename.str() + " not found");
	}

	int nodesAdded = 0;
	int errors = 0;
	while (mapFile->read((char*)&value, sizeof(value)))
	{
		int pos_x = value[1];
		int pos_y = value[0];
		int pos_z = value[2];
		if (pos_x >= 0 && pos_x < mapblock->getSizeX() &&
			pos_y >= 0 && pos_y < mapblock->getSizeY() &&
			pos_z >= 0 && pos_z < mapblockSizeZ) // using "mapblockSizeZ" instead of "mapblock->getSizeZ()"!!
		{
			// all ok
		}
		else
		{
			Log(LOG_INFO) << "Bad node in RMP file: " << filename.str() << " Node #" << nodesAdded << " is outside map boundaries at X:" << pos_x << " Y:" << pos_y << " Z:" << pos_z << ". Found using OXCE test cases.";
			errors++;
		}
		nodesAdded++;
	}

	if (!mapFile->eof())
	{
		throw Exception("Invalid RMP file: " + filename.str());
	}

	return errors;
}

int TestState::loadMAP(MapBlock *mapblock)
{
	int sizez;
	char size[3];
	std::ostringstream filename;
	filename << "MAPS/" << mapblock->getName() << ".MAP";

	// Load file
	auto mapFile = FileMap::getIStream(filename.str());
	if (!mapFile)
	{
		throw Exception(filename.str() + " not found");
	}

	mapFile->read((char*)&size, sizeof(size));
	//sizey = (int)size[0];
	//sizex = (int)size[1];
	sizez = (int)size[2];

	//mapblock->setSizeZ(sizez); // commented out, for testing purposes we don't need to HACK it like in real code

	return sizez;
}

}
