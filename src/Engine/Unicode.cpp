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
#include "Unicode.h"
#include <sstream>
#include <locale>
#include <stdexcept>
#include <cstring>
#include <assert.h>
#include "Logger.h"
#include "Exception.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#else

#if defined(__CYGWIN__)
#include <algorithm>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

namespace OpenXcom
{

std::string OXCE_CURRENCY_SYMBOL = "$";

namespace Unicode
{

std::locale utf8;

/**
 * Store a UTF-8 locale to use when dealing with character conversions.
 * Windows doesn't have a UTF-8 locale so we just use its APIs directly.
 */
void getUtf8Locale()
{
	std::string loc;
#ifndef _WIN32
	// Find any UTF-8 locale
	FILE *fp = popen("locale -a", "r");
	if (fp != NULL)
	{
		char buf[50];
		while (fgets(buf, sizeof(buf), fp) != NULL)
		{
			if (strstr(buf, ".utf8") != NULL ||
				strstr(buf, ".UTF-8") != NULL)
			{
				// Trim newline
				size_t end = strlen(buf) - 1;
				if (buf[end] == '\n')
					buf[end] = '\0';

				loc = buf;
				break;
			}
		}
		pclose(fp);
	}
#endif
	// Try a UTF-8 locale (or default if none was found)
	try
	{
		Log(LOG_INFO) << "Attempted locale: " << loc.c_str();
		utf8 = std::locale(loc.c_str());
	}
	catch (const std::runtime_error &)
	{
		// Well we're stuck with the C locale, hope for the best
	}
	Log(LOG_INFO) << "Detected locale: " << utf8.name();
}

/**
 * Takes a Unicode 32-bit string and converts it
 * to a 8-bit string encoded in UTF-8.
 * Used for rendering text.
 * @note Adapted from https://stackoverflow.com/a/148766/2683561
 * @param src UTF-8 string.
 * @return Unicode string.
 */
UString convUtf8ToUtf32(const std::string &src)
{
	if (src.empty())
		return UString();
	UString out;
	out.reserve(src.size());
	UCode codepoint = 0;
	for (std::string::const_iterator i = src.begin(); i != src.end();)
	{
		unsigned char ch = static_cast<unsigned char>(*i);
		if (ch <= 0x7f)
			codepoint = ch;
		else if (ch <= 0xbf)
			codepoint = (codepoint << 6) | (ch & 0x3f);
		else if (ch <= 0xdf)
			codepoint = ch & 0x1f;
		else if (ch <= 0xef)
			codepoint = ch & 0x0f;
		else
			codepoint = ch & 0x07;
		++i;
		if (i == src.end() || ((*i & 0xc0) != 0x80 && codepoint <= 0x10ffff))
		{
			out.append(1, codepoint);
		}
	}
	return out;
}

/**
 * Takes a Unicode 32-bit string and converts it
 * to a 8-bit string encoded in UTF-8.
 * Used for rendering text.
 * @note Adapted from https://stackoverflow.com/a/148766/2683561
 * @param src Unicode string.
 * @return UTF-8 string.
 */
std::string convUtf32ToUtf8(const UString &src)
{
	if (src.empty())
		return std::string();
	std::string out;
	out.reserve(src.size());
	for (UString::const_iterator i = src.begin(); i != src.end(); ++i)
	{
		UCode codepoint = *i;
		if (codepoint <= 0x7f)
		{
			out.append(1, static_cast<char>(codepoint));
		}
		else if (codepoint <= 0x7ff)
		{
			out.append(1, static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f)));
			out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
		}
		else if (codepoint <= 0xffff)
		{
			out.append(1, static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f)));
			out.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
			out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
		}
		else
		{
			out.append(1, static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07)));
			out.append(1, static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
			out.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
			out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
		}
	}
	return out;
}

/**
 * Takes a wide-character string and converts it to a
 * multibyte 8-bit string in a given encoding.
 * Used for Win32 APIs.
 * @param src Wide-character string.
 * @param cp Codepage of the destination string.
 * @return Multibyte string.
 */
std::string convWcToMb(const std::wstring &src, unsigned int cp)
{
	if (src.empty())
		return std::string();
#ifdef _WIN32
	int size = WideCharToMultiByte(cp, 0, &src[0], (int)src.size(), NULL, 0, NULL, NULL);
	std::string str(size, 0);
	WideCharToMultiByte(cp, 0, &src[0], (int)src.size(), &str[0], size, NULL, NULL);
	return str;
#elif defined(__CYGWIN__)
	(void)cp;
	assert(sizeof(wchar_t) == sizeof(Uint16));
	UString ustr(src.size(), 0);
	std::transform(src.begin(), src.end(), ustr.begin(),
		[](wchar_t c) -> UCode
		{
			//TODO: dropping surrogates, do proper implementation when someone will need that range
			if (c <= 0xD7FF || c >= 0xE000)
			{
				return c;
			}
			else
			{
				return '?';
			}
		}
	);
	return convUtf32ToUtf8(ustr);
#else
	(void)cp;
	assert(sizeof(wchar_t) == sizeof(UCode));
	const UString *ustr = reinterpret_cast<const UString*>(&src);
	return convUtf32ToUtf8(*ustr);
#endif
}

/**
 * Takes a multibyte 8-bit string in a given encoding
 * and converts it to a wide-character string.
 * Used for Win32 APIs.
 * @param src Multibyte string.
 * @param cp Codepage of the source string.
 * @return Wide-character string.
 */
std::wstring convMbToWc(const std::string &src, unsigned int cp)
{
	if (src.empty())
		return std::wstring();
#ifdef _WIN32
	int size = MultiByteToWideChar(cp, 0, &src[0], (int)src.size(), NULL, 0);
	std::wstring wstr(size, 0);
	MultiByteToWideChar(cp, 0, &src[0], (int)src.size(), &wstr[0], size);
	return wstr;
#elif defined(__CYGWIN__)
	(void)cp;
	assert(sizeof(wchar_t) == sizeof(Uint16));
	const UString ustr = convUtf8ToUtf32(src);

	std::wstring wstr(ustr.size(), 0);
	std::transform(ustr.begin(), ustr.end(), wstr.begin(),
		[](UCode c) -> wchar_t
		{
			//TODO: dropping surrogates, do proper implementation when someone will need that range
			if (c <= 0xD7FF || (c >= 0xE000 && c <= 0xFFFF))
			{
				return c;
			}
			else
			{
				return '?';
			}
		}
	);
	return wstr;
#else
	(void)cp;
	assert(sizeof(wchar_t) == sizeof(UCode));
	UString ustr = convUtf8ToUtf32(src);
	const std::wstring *wstr = reinterpret_cast<const std::wstring*>(&ustr);
	return *wstr;
#endif
}


/**
 * Iterate pointer range without checking for correct range
 * @param begin_ptr In/Out current pointer
 * @param end End pointer
 * @param callback_1 callback for ASCII char
 * @param callback_2 callback for 2byte UTF code point
 * @param callback_3 callback for 3byte UTF code point
 * @param callback_4 callback for 4byte UTF code point
 * @return True if iteration reach end, False if function find UTF8 error or callback request ending
 */
template<typename F1, typename F2, typename F3, typename F4>
static bool iterateUTF8CodePointsUnsafe(const unsigned char** begin_ptr, const unsigned char* end, F1 callback_1, F2 callback_2, F3 callback_3, F4 callback_4)
{
	auto begin = *begin_ptr;
	while (begin < end) {
		if (begin[0] < 0x80) {
			/* 0xxxxxxx */
			if (false == callback_1(begin[0]))
			{
				*begin_ptr = begin;
				return false;
			}
			begin += 1;
		} else if ((begin[0] & 0xe0) == 0xc0) {
			/* 110XXXXx 10xxxxxx */
			if ((begin[1] & 0xc0) != 0x80 ||
				false == callback_2(begin[0], begin[1]))
			{
				*begin_ptr = begin;
				return false;
			}
			begin += 2;
		} else if ((begin[0] & 0xf0) == 0xe0) {
			/* 1110XXXX 10Xxxxxx 10xxxxxx */
			if ((begin[1] & 0xc0) != 0x80 ||
				(begin[2] & 0xc0) != 0x80 ||
				false == callback_3(begin[0], begin[1], begin[2]))
			{
				*begin_ptr = begin;
				return false;
			}
			begin +=  3;
		} else if ((begin[0] & 0xf8) == 0xf0) {
			/* 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx */
			if ((begin[1] & 0xc0) != 0x80 ||
				(begin[2] & 0xc0) != 0x80 ||
				(begin[3] & 0xc0) != 0x80 ||
				false == callback_4(begin[0], begin[1], begin[2], begin[3]))
			{
				*begin_ptr = begin;
				return false;
			}
			begin += 4;
		} else {
			*begin_ptr = begin;
			return false;
		}
	}

	*begin_ptr = begin;
	return true;
}

/**
 * Iterate pointer range, function is checking range for correction
 * @param begin_ptr In/Out current pointer
 * @param end End pointer
 * @param callback_1 callback for ASCII char
 * @param callback_2 callback for 2byte UTF code point
 * @param callback_3 callback for 3byte UTF code point
 * @param callback_4 callback for 4byte UTF code point
 * @return True if iteration reach end, False if function find UTF8 error or callback request ending
 */
template<typename F1, typename F2, typename F3, typename F4>
static bool iterateUTF8CodePoints(const unsigned char* begin, const unsigned char* end, F1 callback_1, F2 callback_2, F3 callback_3, F4 callback_4)
{
	assert(begin <= end && "Invalid UTF8 text pointers");

	if (begin < end)
	{
		// we need buffer to avoid dereferencing memory after end of string
		const auto fake_end = end - 3;
		auto curr = begin;
		if (false == iterateUTF8CodePointsUnsafe(&curr, fake_end, callback_1, callback_2, callback_3, callback_4))
		{
			return false;
		}

		if (curr != end)
		{
			// custom buffer for last 4 chars for case of corrupted UTF8
			unsigned char buffer[8] = { };

			const std::size_t tail_size = end - curr;
			std::memcpy(buffer, curr, tail_size);
			curr = buffer;
			if (false == iterateUTF8CodePointsUnsafe(&curr, buffer + tail_size, callback_1, callback_2, callback_3, callback_4))
			{
				return false;
			}
		}
	}

	return true;
}

/**
 * Checks if UTF-8 string is well-formed.
 * @param ss candidate string
 * @return true if valid
 * @note based on https://www.cl.cam.ac.uk/~mgk25/ucs/utf8_check.c
 */
bool isValidUTF8(const std::string& ss)
{
	if (false == iterateUTF8CodePoints(
			reinterpret_cast<const unsigned char*>(ss.data()),
			reinterpret_cast<const unsigned char*>(ss.data() + ss.size()),
			[&](unsigned char s0)
			{
				/* 0xxxxxxx */
				return true;
			},
			[&](unsigned char s0, unsigned char s1)
			{
				/* 110XXXXx 10xxxxxx */
				if ((s0 & 0xfe) == 0xc0)	/* overlong? */
				{
					return false;
				}

				return true;
			},
			[&](unsigned char s0, unsigned char s1, unsigned char s2)
			{
				/* 1110XXXX 10Xxxxxx 10xxxxxx */
				if ((s0 == 0xe0 && (s1 & 0xe0) == 0x80) ||		/* overlong? */
					(s0 == 0xed && (s1 & 0xe0) == 0xa0) ||		/* surrogate? */
					(s0 == 0xef && s1 == 0xbf &&
					(s2 & 0xfe) == 0xbe))						/* U+FFFE or U+FFFF? */
				{
					return false;
				}

				return true;
			},
			[&](unsigned char s0, unsigned char s1, unsigned char s2, unsigned char s3)
			{
				/* 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx */
				if ((s0 == 0xf0 && (s1 & 0xf0) == 0x80) ||		/* overlong? */
					(s0 == 0xf4 && s1 > 0x8f) || s0 > 0xf4) 	/* > U+10FFFF? */
				{
					return false;
				}

				return true;
			}
		)
	)
	{
		return false;
	}

	return true;
}

/**
 * Count code points in utf8 string.
 * @param str Base string
 * @return Number of utf8 codepoints in given string.
 */
std::size_t codePointLengthUTF8(const std::string &str)
{
	std::size_t size = 0;

	if (false == iterateUTF8CodePoints(
			reinterpret_cast<const unsigned char*>(str.data()),
			reinterpret_cast<const unsigned char*>(str.data() + str.size()),
			[&](unsigned char s0)
			{
				/* 0xxxxxxx */
				size += 1;
				return true;
			},
			[&](unsigned char s0, unsigned char s1)
			{
				/* 110XXXXx 10xxxxxx */
				size += 1;
				return true;
			},
			[&](unsigned char s0, unsigned char s1, unsigned char s2)
			{
				/* 1110XXXX 10Xxxxxx 10xxxxxx */
				size += 1;
				return true;
			},
			[&](unsigned char s0, unsigned char s1, unsigned char s2, unsigned char s3)
			{
				/* 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx */
				size += 1;
				return true;
			}
		)
	)
	{
		throw Exception("Invalid utf8 string for length");
	}

	return size;
}

/**
 * Substring based on code points in utf8 string.
 * @param str Base string
 * @param pos Start postion in codepoints, need be less or equal total number of code points in string.
 * @param count Number of codepoints to return.
 * @return utf8 substring from `str`.
 */
std::string codePointSubstrUTF8(const std::string &str, std::size_t pos, std::size_t count)
{
	bool found_end = false;
	std::size_t curr = 0;
	std::size_t byte_curr = 0;
	std::size_t byte_begin = std::string::npos;
	std::size_t byte_end = std::string::npos;

	auto loop = [&]
	{
		if (byte_begin == std::string::npos)
		{
			if (pos == curr)
			{
				byte_begin = byte_curr;

#if 0
				// potential optimalization but it could propagate invalid bytes
				if (count == std::string::npos)
				{
					found_end = true;
					return false;
				}
#endif

				if (count-- == 0)
				{
					found_end = true;
					byte_end = byte_curr;
					return false;
				}
			}
		}
		else
		{
			if (count-- == 0)
			{
				found_end = true;
				byte_end = byte_curr;
				return false;
			}
		}

		return true;
	};

	if (false == iterateUTF8CodePoints(
			reinterpret_cast<const unsigned char*>(str.data()),
			reinterpret_cast<const unsigned char*>(str.data() + str.size()),
			[&](unsigned char s0)
			{
				/* 0xxxxxxx */
				if (false == loop())
				{
					return false;
				}

				curr += 1;
				byte_curr += 1;
				return true;
			},
			[&](unsigned char s0, unsigned char s1)
			{
				/* 110XXXXx 10xxxxxx */
				if (false == loop())
				{
					return false;
				}

				curr += 1;
				byte_curr += 2;
				return true;
			},
			[&](unsigned char s0, unsigned char s1, unsigned char s2)
			{
				/* 1110XXXX 10Xxxxxx 10xxxxxx */
				if (false == loop())
				{
					return false;
				}

				curr += 1;
				byte_curr += 3;
				return true;
			},
			[&](unsigned char s0, unsigned char s1, unsigned char s2, unsigned char s3)
			{
				/* 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx */
				if (false == loop())
				{
					return false;
				}

				curr += 1;
				byte_curr += 4;
				return true;
			}
		)
	)
	{
		// early end, check if we have invalid utf8
		if (false == found_end)
		{
			throw Exception("Invalid utf8 string for substr");
		}
	}
	else
	{
		// handle `pos == curr` after iterating all code points
		loop();
	}

	// for big `count` we can do not find correct `byte_end`, it will be `npos` but it still should work
	// in some cases for big `pos > curr` we have `byte_begin == std::string::npos` this will throw from `substr` and we want this.
	return str.substr(byte_begin, byte_end - byte_begin);
}

/**
 * Compares two UTF-8 strings using natural human ordering.
 * @param a String A.
 * @param b String B.
 * @return String A comes before String B.
 */
bool naturalCompare(const std::string &a, const std::string &b)
{
#ifdef _WIN32
	typedef int (WINAPI *WinStrCmp)(PCWSTR, PCWSTR);
	WinStrCmp pWinStrCmp = (WinStrCmp)GetProcAddress(GetModuleHandleA("shlwapi.dll"), "StrCmpLogicalW");
	if (pWinStrCmp)
	{
		std::wstring wa = convMbToWc(a, CP_UTF8);
		std::wstring wb = convMbToWc(b, CP_UTF8);
		return (pWinStrCmp(wa.c_str(), wb.c_str()) < 0);
	}
	else
#endif
	{
		// fallback to lexical sort
		return caseCompare(a, b);
	}
}

/**
 * Compares two UTF-8 strings ignoring case.
 * @param a String A.
 * @param b String B.
 * @return String A comes before String B.
 */
bool caseCompare(const std::string &a, const std::string &b)
{
#ifdef _WIN32
	std::wstring wa = convMbToWc(a, CP_UTF8);
	std::wstring wb = convMbToWc(b, CP_UTF8);
	return (StrCmpIW(wa.c_str(), wb.c_str()) < 0);
#else
	return (std::use_facet< std::collate<char> >(utf8).compare(&a[0], &a[0] + a.size(), &b[0], &b[0] + b.size()) < 0);
#endif
}

/**
 * Searches for a substring in another string ignoring case.
 * @param haystack String to search.
 * @param needle String to find.
 * @return True if the needle is in the haystack.
 */
bool caseFind(const std::string &haystack, const std::string &needle)
{
#ifdef _WIN32
	std::wstring wa = convMbToWc(haystack, CP_UTF8);
	std::wstring wb = convMbToWc(needle, CP_UTF8);
	return (StrStrIW(wa.c_str(), wb.c_str()) != NULL);
#else
	std::wstring wa = convMbToWc(haystack, 0);
	std::wstring wb = convMbToWc(needle, 0);
	std::use_facet< std::ctype<wchar_t> >(utf8).toupper(&wa[0], &wa[0] + wa.size());
	std::use_facet< std::ctype<wchar_t> >(utf8).toupper(&wb[0], &wb[0] + wb.size());
	return (wa.find(wb) != std::wstring::npos);
#endif
}

/**
 * Uppercases a UTF-8 string, modified in place.
 * Used for case-insensitive comparisons.
 * @param s Source string.
 */
void upperCase(std::string &s)
{
	if (s.empty())
		return;
#ifdef _WIN32
	std::wstring ws = convMbToWc(s, CP_UTF8);
	CharUpperW(&ws[0]);
	s = convWcToMb(ws, CP_UTF8);
#else
	std::wstring ws = convMbToWc(s, 0);
	std::use_facet< std::ctype<wchar_t> >(utf8).toupper(&ws[0], &ws[0] + ws.size());
	s = convWcToMb(ws, 0);
#endif
}

/**
 * Lowercases a UTF-8 string, modified in place.
 * Used for case-insensitive comparisons.
 * @param s Source string.
 */
void lowerCase(std::string &s)
{
	if (s.empty())
		return;
#ifdef _WIN32
	std::wstring ws = convMbToWc(s, CP_UTF8);
	CharLowerW(&ws[0]);
	s = convWcToMb(ws, CP_UTF8);
#else
	std::wstring ws = convMbToWc(s, 0);
	std::use_facet< std::ctype<wchar_t> >(utf8).tolower(&ws[0], &ws[0] + ws.size());
	s = convWcToMb(ws, 0);
#endif
}

/**
 * Replaces every instance of a substring.
 * @param str The string to modify.
 * @param find The substring to find.
 * @param replace The substring to replace it with.
 */
void replace(std::string &str, const std::string &find, const std::string &replace)
{
	for (size_t i = str.find(find); i != std::string::npos; i = str.find(find, i + replace.length()))
	{
		str.replace(i, find.length(), replace);
	}
}

/**
 * Takes an integer value and formats it as number with separators (spacing the thousands).
 * @param value The value.
 * @param currency Currency symbol.
 * @return The formatted string.
 */
std::string formatNumber(int64_t value, const std::string &currency)
{
	const std::string thousands_sep = "\xC2\xA0"; // TOK_NBSP

	bool negative = (value < 0);
	std::ostringstream ss;
	ss << (negative ? -value : value);
	std::string s = ss.str();
	size_t spacer = s.size() - 3;
	while (spacer > 0 && spacer < s.size())
	{
		s.insert(spacer, thousands_sep);
		spacer -= 3;
	}
	if (!currency.empty())
	{
		if (currency.length() > 1) // Note: size in bytes, not characters!
			s.append(currency);
		else
			s.insert(0, currency);
	}
	if (negative)
	{
		s.insert(0, "-");
	}
	return s;
}

/**
 * Takes an integer value and formats it as currency,
 * spacing the thousands and adding a $ sign to the front.
 * @param funds The funding value.
 * @return The formatted string.
 */
std::string formatFunding(int64_t funds)
{
	return formatNumber(funds, OXCE_CURRENCY_SYMBOL);
}

/**
 * Takes an integer value and formats it as percentage,
 * adding a % sign.
 * @param value The percentage value.
 * @return The formatted string.
 */
std::string formatPercentage(int value)
{
	std::ostringstream ss;
	ss << value << "%";
	return ss.str();
}



#ifdef OXCE_AUTO_TEST
#define assert_throw(A) try { A; assert(false && "No throw from " #A ); } catch (...){ /*nothing*/ }

static auto dummy = ([]
{
	assert(isValidUTF8("012345") == true);
	assert(codePointLengthUTF8("012345") == 6);
	assert(isValidUTF8("很烫烫的一锅汤") == true);
	assert(codePointLengthUTF8("很烫烫的一锅汤") == 7);

	assert(isValidUTF8("ÐðŁłŠšÝýÞþŽž") == true);
	assert(codePointLengthUTF8("ÐðŁłŠšÝýÞþŽž") == 12);

	assert(isValidUTF8("\xf0\x9f\x92\xa9") == true);
	assert(codePointLengthUTF8("\xf0\x9f\x92\xa9") == 1);

	assert(isValidUTF8("\x7f") == true);
	assert(codePointLengthUTF8("\x7f") == 1);

	assert(isValidUTF8("\xff") == false);
	assert_throw(codePointLengthUTF8("\xff"));

	assert(isValidUTF8("\x80") == false);
	assert_throw(codePointLengthUTF8("\x80"));

	assert(isValidUTF8("\xc5\x9b") == true);
	assert(isValidUTF8("\xc5\xc5") == false);
	assert(isValidUTF8("\xc5") == false);

	assert(isValidUTF8("\xe5\xbe\x88") == true);
	assert(isValidUTF8("\xe5\xbe") == false);
	assert(isValidUTF8("\xe5") == false);

	assert(isValidUTF8("\xc5\x9b\xe5\xbe\x88") == true);
	assert(isValidUTF8("\xc5\x9b\xe5\xbe") == false);
	assert(isValidUTF8("\xc5\x9b\xe5") == false);

	assert(isValidUTF8("A\xc5\x9b\xe5\xbe\x88") == true);
	assert(isValidUTF8("A\xc5\x9b\xe5\xbe") == false);
	assert(isValidUTF8("A\xc5\x9b\xe5") == false);

	assert(isValidUTF8("\xc4\x99\xc5\x9b\xe5\xbe\x88") == true);
	assert(isValidUTF8("\xc4\x99\xc5\x9b\xe5\xbe") == false);
	assert(isValidUTF8("\xc4\x99\xc5\x9b\xe5") == false);

	assert(isValidUTF8("\xe5\xbe\x88    ") == true);
	assert(isValidUTF8("\xe5\xbe     ") == false);
	assert(isValidUTF8("\xe5    ") == false);

	assert(isValidUTF8("    \xe5\xbe\x88") == true);
	assert(isValidUTF8("    \xe5\xbe") == false);
	assert(isValidUTF8("    \xe5") == false);

	assert(isValidUTF8("\xe5\xbe\x88\xc4\x99") == true);
	assert(isValidUTF8("\xe5\xbe\xc4\x99") == false);
	assert(isValidUTF8("\xe5\xc4\x99") == false);

	assert(isValidUTF8("\xf0\x9f\x92\xa9") == true);
	assert(isValidUTF8("\xf0\x9f\x92") == false);
	assert(isValidUTF8("\xf0\x9f") == false);
	assert(isValidUTF8("\xf0") == false);

	//handling of embedded zeros
	assert(isValidUTF8(std::string{'\0','\0','\xc5','\x9b'}) == true);
	assert(codePointLengthUTF8(std::string{'\0','\0','\xc5','\x9b'}) == 3);

	assert(isValidUTF8(std::string{'\0','\0','\xc5'}) == false);
	assert_throw(codePointLengthUTF8(std::string{'\0','\0','\xc5'}));


	//test substr
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 0) == std::string("很烫烫的一锅汤"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 1) == std::string("烫烫的一锅汤"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 2) == std::string("烫的一锅汤"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 3) == std::string("的一锅汤"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 4) == std::string("一锅汤"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 5) == std::string("锅汤"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 6) == std::string("汤"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 7) == std::string(""));
	assert_throw(codePointSubstrUTF8("很烫烫的一锅汤", 8));

	assert(codePointSubstrUTF8("很烫烫的一锅汤", 0, 8) == std::string("很烫烫的一锅汤"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 0, 7) == std::string("很烫烫的一锅汤"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 0, 6) == std::string("很烫烫的一锅"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 0, 5) == std::string("很烫烫的一"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 0, 4) == std::string("很烫烫的"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 0, 3) == std::string("很烫烫"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 0, 2) == std::string("很烫"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 0, 1) == std::string("很"));
	assert(codePointSubstrUTF8("很烫烫的一锅汤", 0, 0) == std::string(""));

	assert(codePointSubstrUTF8("ÐðŁłŠšÝýÞþŽž", 0, 3) == std::string("ÐðŁ"));
	assert(codePointSubstrUTF8("ÐðŁłŠšÝýÞþŽž", 3, 3) == std::string("łŠš"));
	assert(codePointSubstrUTF8("ÐðŁłŠšÝýÞþŽž", 6, 3) == std::string("ÝýÞ"));
	assert(codePointSubstrUTF8("ÐðŁłŠšÝýÞþŽž", 9, 3) == std::string("þŽž"));
	assert(codePointSubstrUTF8("ÐðŁłŠšÝýÞþŽž", 12, 3) == std::string(""));

	assert(codePointSubstrUTF8("ÐðŁłŠšÝýÞþŽž", 11, 5) == std::string("ž"));
	assert(codePointSubstrUTF8("ÐðŁłŠšÝýÞþŽž", 10, 5) == std::string("Žž"));
	assert(codePointSubstrUTF8("ÐðŁłŠšÝýÞþŽž", 9, 5) == std::string("þŽž"));
	assert(codePointSubstrUTF8("ÐðŁłŠšÝýÞþŽž", 8, 5) == std::string("ÞþŽž"));
	assert(codePointSubstrUTF8("ÐðŁłŠšÝýÞþŽž", 7, 5) == std::string("ýÞþŽž"));
	assert(codePointSubstrUTF8("ÐðŁłŠšÝýÞþŽž", 6, 5) == std::string("ÝýÞþŽ"));
	assert(codePointSubstrUTF8("ÐðŁłŠšÝýÞþŽž", 5, 5) == std::string("šÝýÞþ"));

	assert(codePointSubstrUTF8("012", 0, 1) == std::string("0"));
	assert(codePointSubstrUTF8("012", 1, 1) == std::string("1"));
	assert(codePointSubstrUTF8("012", 2, 1) == std::string("2"));

	return 0;
})();
#endif

}
}
