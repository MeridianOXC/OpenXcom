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

#include "Language.h"
#include <algorithm>
#include <cassert>
#include <set>
#include <climits>
#include <algorithm>
#include "CrossPlatform.h"
#include "Logger.h"
#include "Options.h"
#include "LanguagePlurality.h"
#include "Unicode.h"
#include "../Mod/ExtraStrings.h"
#include "../Savegame/Soldier.h"
#include "FileMap.h"

namespace OpenXcom
{

std::map<std::string, std::string> Language::_names;
std::vector<std::string> Language::_rtl, Language::_cjk;

/**
 * Initializes an empty language file.
 */
Language::Language() : _handler(0), _direction(DIRECTION_LTR), _wrap(WRAP_WORDS)
{
	// maps don't have initializers :(
	if (_names.empty())
	{
		_names["en-US"] = "English (US)";
		_names["en-GB"] = "English (UK)";
		// _names["ar"] = "العربية"; needs fonts
		_names["bg"] = "Български";
		_names["ca-ES"] = "Català";
		_names["cs"] = "Česky";
		_names["cy"] = "Cymraeg";
		_names["da"] = "Dansk";
		_names["de"] = "Deutsch";
		_names["el"] = "Ελληνικά";
		_names["et"] = "Eesti";
		_names["es-ES"] = "Español (ES)";
		_names["es-419"] = "Español (AL)";
		_names["fr"] = "Français (FR)";
		_names["fr-CA"] = "Français (CA)";
		_names["fi"] = "Suomi";
		_names["ga"] = "Gaeilge";
		_names["hr"] = "Hrvatski";
		_names["hu"] = "Magyar";
		_names["it"] = "Italiano";
		_names["is"] = "Íslenska";
		_names["ja"] = "日本語";
		_names["ko"] = "한국어";
		_names["lb"] = "Lëtzebuergesch";
		_names["lv"] = "Latviešu";
		_names["nl"] = "Nederlands";
		_names["no"] = "Norsk";
		_names["pl"] = "Polski";
		_names["pt-BR"] = "Português (BR)";
		_names["pt-PT"] = "Português (PT)";
		_names["ro"] = "Română";
		_names["ru"] = "Русский";
		_names["sk"] = "Slovenčina";
		_names["sl"] = "Slovenščina";
		_names["sv"] = "Svenska";
		// _names["th"] = "ไทย"; needs fonts
		_names["tr"] = "Türkçe";
		_names["uk"] = "Українська";
		_names["vi"] = "Tiếng Việt";
		_names["zh-CN"] = "中文";
		_names["zh-TW"] = "文言";
	}
	if (_rtl.empty())
	{
		//_rtl.push_back("he"); needs translation
	}
	if (_cjk.empty())
	{
		_cjk.push_back("ja");
		_cjk.push_back("ko");
		_cjk.push_back("zh-CN");
		_cjk.push_back("zh-TW");
	}

	std::string id = Options::language;
	_handler = LanguagePlurality::create(id);
	if (std::find(_rtl.begin(), _rtl.end(), id) == _rtl.end())
	{
		_direction = DIRECTION_LTR;
	}
	else
	{
		_direction = DIRECTION_RTL;
	}
	if (Options::wordwrap == WRAP_AUTO)
	{
		if (std::find(_cjk.begin(), _cjk.end(), id) == _cjk.end())
		{
			_wrap = WRAP_WORDS;
		}
		else
		{
			_wrap = WRAP_LETTERS;
		}
	}
	else
	{
		_wrap = Options::wordwrap;
	}
}

/**
 *
 */
Language::~Language()
{
	delete _handler;
}

/**
 * Extracts language id from a filename
 * @param  fileName some filename
 * @return 'pt' or 'en-US' or the like
 */
static std::string fileNameToId(const std::string& fileName) {
	auto noExtName = CrossPlatform::noExt(CrossPlatform::baseFilename(fileName));
	auto found = noExtName.find_first_of('-');
	if (found != noExtName.npos) {
		// uppercase it after dash
		for (found += 1; found < noExtName.size(); ++found) {
			noExtName[found] = toupper(noExtName[found]);
		}
	}
	return noExtName;
}

/**
 * Gets all the languages found in the
 * Data folder and returns their properties.
 * @param ids List of language ids.
 * @param names List of language human-readable names.
 */
void Language::getList(std::vector<std::string> &ids, std::vector<std::string> &names)
{
	auto nset = FileMap::filterFiles(FileMap::getVFolderContents("Language", 0), "yml");
	ids.clear();
	for (const auto& filename : nset) {
		ids.push_back(fileNameToId(filename));
	}
	std::sort(ids.begin(), ids.end());
	names.clear();

	for (const auto& id: ids)
	{
		if (_names.find(id) != _names.end()) {
			names.push_back(_names[id]);
		} else {
			names.push_back(id);
		}
	}
}

/**
 * Loads a language file in Ruby-on-Rails YAML format.
 * Not that this has anything to do with Ruby, but since it's a
 * widely-supported format and we already have YAML, it was convenient.
 * @param filename Filename of the YAML file.
 */
void Language::loadFile(const FileMap::FileRecord *frec)
{
	const YAML::YamlRootNodeReader& reader = frec->getYAML();
	YAML::YamlNodeReader langMap = reader[0].isMap() ? reader[0] : reader.toBase();

	for (const auto& langReader : langMap.children())
	{
		// Regular strings
		if (langReader.hasVal())
		{
			std::string value = langReader.readVal<std::string>();
			if (!value.empty())
			{
				std::string key = langReader.readKey<std::string>();
				_strings[key] = loadString(value);
			}
		}
		// Strings with plurality
		else if (langReader.isMap())
		{
			for (const auto& pluralityReader : langReader.children())
			{
				std::string value = pluralityReader.readVal<std::string>("");
				if (!value.empty())
				{
					std::string key = langReader.readKey<std::string>() + "_" + pluralityReader.readKey<std::string>();
					_strings[key] = loadString(value);
				}
			}
		}
	}
}

/**
 * Loads a language file from a mod's ExtraStrings.
 * @param extraStrings List of ExtraStrings.
 * @param id Language ID.
 */
void Language::loadRule(const std::map<std::string, ExtraStrings*> &extraStrings, const std::string &id)
{
	auto it = extraStrings.find(id);
	if (it != extraStrings.end())
	{
		for (const auto& pair : *it->second->getStrings())
		{
			_strings[pair.first] = loadString(pair.second);
		}
	}
}

/**
 * Replaces all special string markers with the appropriate characters.
 * @param string Original string.
 * @return New converted string.
 */
std::string Language::loadString(const std::string &string) const
{
	std::string s = string;
	Unicode::replace(s, "{NEWLINE}", "\n");
	Unicode::replace(s, "{SMALLLINE}", "\x02"); // Unicode::TOK_NL_SMALL
	Unicode::replace(s, "{ALT}", "\x01"); // Unicode::TOK_COLOR_FLIP
	return s;
}

/**
 * Returns the localized text with the specified ID.
 * If it's not found, just returns the ID.
 * @param id ID of the string.
 * @return String with the requested ID.
 */
LocalizedText Language::getString(const std::string &id) const
{
	if (id.empty())
	{
		return id;
	}
	auto s = _strings.find(id);
	// Check if translation strings recently learned pluralization.
	if (s == _strings.end())
	{
		return getString(id, UINT_MAX);
	}
	else
	{
		return s->second;
	}
}

/**
 * Returns the localized text with the specified ID, in the proper form for @a n.
 * The substitution of @a n has already happened in the returned LocalizedText.
 * If it's not found, just returns the ID.
 * @param id ID of the string.
 * @param n Number to use to decide the proper form.
 * @return String with the requested ID.
 */
LocalizedText Language::getString(const std::string &id, unsigned n) const
{
	assert(!id.empty());
	static std::set<std::string> notFoundIds;
	auto s = _strings.end();
	// Try specialized form.
	if (n == 0)
	{
		s = _strings.find(id + "_zero");
	}
	// Try proper form by language
	if (s == _strings.end())
	{
		s = _strings.find(id + _handler->getSuffix(n));
	}
	// Try default form
	if (s == _strings.end())
	{
		s = _strings.find(id + "_other");
	}
	// Give up
	if (s == _strings.end())
	{
		if (notFoundIds.end() == notFoundIds.find(id))
		{
			notFoundIds.insert(id);
			Log(LOG_WARNING) << id << " not found in " << Options::language;
		}
		return id;
	}
	if (n == UINT_MAX) // Special case
	{
		if (notFoundIds.end() == notFoundIds.find(id))
		{
			notFoundIds.insert(id);
			Log(LOG_WARNING) << id << " has plural format in ``" << Options::language << "``. Code assumes singular format.";
//		Hint: Change ``getstring(ID).arg(value)`` to ``getString(ID, value)`` in appropriate files.
		}
		return s->second;
	}
	else
	{
		std::ostringstream ss;
		ss << n;
		std::string marker("{N}"), val(ss.str()), txt(s->second);
		Unicode::replace(txt, marker, val);
		return txt;
	}

}

/**
 * Returns the localized text with the specified ID, in the proper form for the gender.
 * If it's not found, just returns the ID.
 * @param id ID of the string.
 * @param gender Current soldier gender.
 * @return String with the requested ID.
 */
LocalizedText Language::getString(const std::string &id, SoldierGender gender) const
{
	std::string genderId;
	if (gender == GENDER_MALE)
	{
		genderId = id + "_MALE";
	}
	else
	{
		genderId = id + "_FEMALE";
	}
	return getString(genderId);
}

/**
 * Outputs all the language IDs and strings
 * to an HTML table.
 * @param filename HTML file.
 */
void Language::toHtml(const std::string &filename) const
{
	std::stringstream htmlFile;
	htmlFile << "<table border=\"1\" width=\"100%\">" << std::endl;
	htmlFile << "<tr><th>ID String</th><th>English String</th></tr>" << std::endl;
	for (auto& pair : _strings)
	{
		htmlFile << "<tr><td>" << pair.first << "</td><td>";
		std::string s = pair.second;
		for (std::string::const_iterator j = s.begin(); j != s.end(); ++j)
		{
			if (*j == Unicode::TOK_NL_SMALL || *j == '\n')
			{
				htmlFile << "<br />";
			}
			else
			{
				htmlFile << *j;
			}
		}
		htmlFile << "</td></tr>" << std::endl;
	}
	htmlFile << "</table>" << std::endl;
	CrossPlatform::writeFile(filename, htmlFile.str());
}

/**
 * Returns the direction to use for rendering
 * text in this language.
 * @return Text direction.
 */
TextDirection Language::getTextDirection() const
{
	return _direction;
}

/**
 * Returns the wrapping rules to use for rendering
 * text in this language.
 * @return Text wrapping.
 */
TextWrapping Language::getTextWrapping() const
{
	return _wrap;
}

}

/** @page LanguageFiles Format of the language files.

Language files are formatted as YAML (.yml) containing UTF-8 (no BOM) text.
The first line in a language file is the language's identifier.
The rest of the file are key-value pairs. The key of each pair
contains the ID string (dictionary key), and the value contains the localized
text for the given key in quotes.

The localized text may contain the following special markers:
<table>
<tr>
 <td><tt>{</tt><i>0, 1, 2, ...</i> <tt>}</tt></td>
 <td>These markers will be replaced by programmer-supplied values before the
 message is displayed.</td></tr>
<tr>
 <td><tt>{ALT}</tt></td>
 <td>The rest of the text will be in an alternate color. Using this again will
 switch back to the primary color.</td></tr>
<tr>
 <td><tt>{NEWLINE}</tt></td>
 <td>It will be replaced with a line break in the game.</td></tr>
<tr>
 <td><tt>{SMALLLINE}</tt></td>
 <td>The rest of the text will be in a small font.</td></tr>
</table>

There is an additional marker sequence, that should only appear in texts that
depend on a number. This marker <tt>{N}</tt> will be replaced by the actual
number used. The keys for texts that depend on numbers also have special
suffixes, that depend on the language. For all languages, a suffix of
<tt>_zero</tt> is tried if the number is zero, before trying the actual key
according to the language rules. The rest of the suffixes depend on the language,
as described <a href="http://www.unicode.org/cldr/charts/latest/supplemental/language_plural_rules.html">here</a>.

So, you would write (for English):
<pre>
STR_ENEMIES:
  zero:  "There are no enemies left."
  one:   "There is a single enemy left."
  other: "There are {N} enemies left."
</pre>

 */
