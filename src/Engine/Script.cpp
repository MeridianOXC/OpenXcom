/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#include <sstream>
#include <iomanip>
#include <tuple>
#include <algorithm>
#include "../fmath.h"
#include <bitset>
#include <array>
#include <numeric>
#include <climits>

#include "Logger.h"
#include "Options.h"
#include "Script.h"
#include "ScriptBind.h"
#include "Surface.h"
#include "ShaderDraw.h"
#include "ShaderMove.h"
#include "Exception.h"
#include "../fallthrough.h"
#include "Collections.h"

namespace OpenXcom
{

////////////////////////////////////////////////////////////
//						const definition
////////////////////////////////////////////////////////////

constexpr ScriptRef KnowNamesPrefix[] = {
	ScriptRef{ "ModList" },
	ScriptRef{ "Tag" },
};

constexpr bool isKnowNamePrefix(ScriptRef name)
{
	for (ScriptRef r : KnowNamesPrefix)
	{
		if (r == name)
		{
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////
//						arg definition
////////////////////////////////////////////////////////////
#define MACRO_QUOTE(...) __VA_ARGS__

#define MACRO_COPY_4(Func, Pos) \
	Func((Pos) + 0x0) \
	Func((Pos) + 0x1) \
	Func((Pos) + 0x2) \
	Func((Pos) + 0x3)
#define MACRO_COPY_16(Func, Pos) \
	MACRO_COPY_4(Func, (Pos) + 0x00) \
	MACRO_COPY_4(Func, (Pos) + 0x04) \
	MACRO_COPY_4(Func, (Pos) + 0x08) \
	MACRO_COPY_4(Func, (Pos) + 0x0C)
#define MACRO_COPY_64(Func, Pos) \
	MACRO_COPY_16(Func, (Pos) + 0x00) \
	MACRO_COPY_16(Func, (Pos) + 0x10) \
	MACRO_COPY_16(Func, (Pos) + 0x20) \
	MACRO_COPY_16(Func, (Pos) + 0x30)
#define MACRO_COPY_256(Func, Pos) \
	MACRO_COPY_64(Func, (Pos) + 0x00) \
	MACRO_COPY_64(Func, (Pos) + 0x40) \
	MACRO_COPY_64(Func, (Pos) + 0x80) \
	MACRO_COPY_64(Func, (Pos) + 0xC0)


////////////////////////////////////////////////////////////
//						proc definition
////////////////////////////////////////////////////////////
[[gnu::always_inline]]
static inline void addShade_h(int& reg, const int& var)
{
	const int newShade = (reg & 0xF) + var;
	if (newShade > 0xF)
	{
		// so dark it would flip over to another color - make it black instead
		reg = 0xF;
		return;
	}
	else if (newShade > 0)
	{
		reg = (reg & 0xF0) | newShade;
		return;
	}
	reg &= 0xF0;
	//prevent overflow to 0 or another color - make it white instead
	if (!reg || newShade < 0)
		reg = 0x01;
}

[[gnu::always_inline]]
static inline RetEnum mulAddMod_h(int& reg, const int& mul, const int& add, const int& mod)
{
	const int64_t a = ((int64_t)reg) * mul + add;
	if (mod)
	{
		reg = (a % mod + mod) % mod;
		return RetContinue;
	}
	return RetError;
}

[[gnu::always_inline]]
static inline RetEnum mulDiv_h(int& reg, const int& mul, const int& div)
{
	if (div)
	{
		reg = (((int64_t)reg) * mul) / div;
		return RetContinue;
	}
	return RetError;
}

[[gnu::always_inline]]
static inline RetEnum wavegen_rect_h(int& reg, const int& period, const int& size, const int& max)
{
	if (period <= 0)
		return RetError;
	reg %= period;
	if (reg < 0)
		reg += period;
	if (reg > size)
		reg = 0;
	else
		reg = max;
	return RetContinue;
}

[[gnu::always_inline]]
static inline RetEnum wavegen_saw_h(int& reg, const int& period, const int& size, const int& max)
{
	if (period <= 0)
		return RetError;
	reg %= period;
	if (reg < 0)
		reg += period;
	if (reg > size)
		reg = 0;
	else if (reg > max)
		reg = max;
	return RetContinue;
}

[[gnu::always_inline]]
static inline RetEnum wavegen_tri_h(int& reg, const int& period, const int& size, const int& max)
{
	if (period <= 0)
		return RetError;
	reg %= period;
	if (reg < 0)
		reg += period;
	if (reg > size)
		reg = 0;
	else
	{
		if (reg > size/2)
			reg = size - reg;
		if (reg > max)
			reg = max;
	}
	return RetContinue;
}

[[gnu::always_inline]]
static inline RetEnum wavegen_sin_h(int& reg, const int& period, const int& size)
{
	if (period <= 0)
		return RetError;
	reg = size * std::sin(2.0 * M_PI *  reg / period);
	return RetContinue;
}

[[gnu::always_inline]]
static inline RetEnum wavegen_cos_h(int& reg, const int& period, const int& size)
{
	if (period <= 0)
		return RetError;
	reg = size * std::cos(2.0 * M_PI *  reg  / period);
	return RetContinue;
}

[[gnu::always_inline]]
static inline RetEnum call_func_h(ScriptWorkerBase& c, ScriptFunc func, const Uint8* d, ProgPos& p)
{
	auto t = p;
	auto r = func(c, d, t);
	p = t;
	return r;
}

[[gnu::always_inline]]
static inline RetEnum bit_popcount_h(int& reg)
{
	constexpr size_t minBitsetSize = 8*sizeof(std::bitset<1>);
	constexpr size_t minRequiredSize = 8*sizeof(int);
	constexpr size_t optimalSize = minRequiredSize < minBitsetSize ? minRequiredSize : minBitsetSize;
	std::bitset<optimalSize> set = reg; //bitset with optimal size and without overhead
	reg = set.count();
	return RetContinue;
}


/**
 * Main macro defining all available operation in script engine.
 * @param IMPL macro function that access data. Take 3 args: Name, definition of operation and declaration of it's arguments.
 */
#define MACRO_PROC_DEFINITION(IMPL) \
	/*	Name,		Implementation,													End execution,				Args,					Description */ \
	IMPL(exit,		MACRO_QUOTE({													return RetEnd;		}),		(ScriptWorkerBase&),	"") \
	\
	IMPL(goto,		MACRO_QUOTE({ Prog = Label1;									return RetContinue; }),		(ScriptWorkerBase& c, ProgPos& Prog, ProgPos Label1),	"") \
	\
	IMPL(set,		MACRO_QUOTE({ Reg0 = Data1;										return RetContinue; }),		(ScriptWorkerBase& c, int& Reg0, int Data1),	"arg1 = arg2") \
	\
	IMPL(clear,		MACRO_QUOTE({ Reg0 = 0;											return RetContinue; }),		(ScriptWorkerBase& c, int& Reg0),				"arg1 = 0") \
	\
	IMPL(test_le,	MACRO_QUOTE({ Prog = (A <= B) ? LabelTrue : LabelFalse;			return RetContinue; }),		(ProgPos& Prog, int A, int B, ProgPos LabelTrue, ProgPos LabelFalse),	"") \
	IMPL(test_eq,	MACRO_QUOTE({ Prog = (A == B) ? LabelTrue : LabelFalse;			return RetContinue; }),		(ProgPos& Prog, int A, int B, ProgPos LabelTrue, ProgPos LabelFalse),	"") \
	\
	IMPL(swap,		MACRO_QUOTE({ std::swap(Reg0, Reg1);							return RetContinue; }),		(int& Reg0, int& Reg1),				"Swap value of arg1 and arg2") \
	IMPL(add,		MACRO_QUOTE({ Reg0 += Data1;									return RetContinue; }),		(int& Reg0, int Data1),				"arg1 = arg1 + arg2") \
	IMPL(sub,		MACRO_QUOTE({ Reg0 -= Data1;									return RetContinue; }),		(int& Reg0, int Data1),				"arg1 = arg1 - arg2") \
	IMPL(mul,		MACRO_QUOTE({ Reg0 *= Data1;									return RetContinue; }),		(int& Reg0, int Data1),				"arg1 = arg1 * arg2") \
	\
	IMPL(aggregate,	MACRO_QUOTE({ Reg0 = Reg0 + Data1 * Data2;						return RetContinue; }),		(int& Reg0, int Data1, int Data2),			"arg1 = arg1 + (arg2 * arg3)") \
	IMPL(offset,	MACRO_QUOTE({ Reg0 = Reg0 * Data1 + Data2;						return RetContinue; }),		(int& Reg0, int Data1, int Data2),			"arg1 = (arg1 * arg2) + arg3") \
	IMPL(offsetmod,	MACRO_QUOTE({ return mulAddMod_h(Reg0, Mul1, Add2, Mod3);							}),		(int& Reg0, int Mul1, int Add2, int Mod3),	"arg1 = ((arg1 * arg2) + arg3) % arg4") \
	\
	IMPL(div,		MACRO_QUOTE({ if (!Data1) return RetError; Reg0 /= Data1;		return RetContinue; }),		(int& Reg0, int Data1),		"arg1 = arg1 / arg2") \
	IMPL(mod,		MACRO_QUOTE({ if (!Data1) return RetError; Reg0 %= Data1;		return RetContinue; }),		(int& Reg0, int Data1),		"arg1 = arg1 % arg2") \
	IMPL(muldiv,	MACRO_QUOTE({ return mulDiv_h(Reg0, Data1, Data2);									}),		(int& Reg0, int Data1, int Data2),	"arg1 = (arg1 * arg2) / arg3") \
	\
	IMPL(shl,		MACRO_QUOTE({ Reg0 <<= Data1;									return RetContinue; }),		(int& Reg0, int Data1),		"Left bit shift of arg1 by arg2") \
	IMPL(shr,		MACRO_QUOTE({ Reg0 >>= Data1;									return RetContinue; }),		(int& Reg0, int Data1),		"Right bit shift of arg1 by arg2") \
	\
	IMPL(bit_and,		MACRO_QUOTE({ Reg0 = Reg0 & Data1;								return RetContinue; }),		(int& Reg0, int Data1),		"Bit And of arg1 and arg2") \
	IMPL(bit_or,		MACRO_QUOTE({ Reg0 = Reg0 | Data1;								return RetContinue; }),		(int& Reg0, int Data1),		"Bit Or of arg1 and arg2") \
	IMPL(bit_xor,		MACRO_QUOTE({ Reg0 = Reg0 ^ Data1;								return RetContinue; }),		(int& Reg0, int Data1),		"Bit Xor of arg1 and arg2") \
	IMPL(bit_not,		MACRO_QUOTE({ Reg0 = ~Reg0;										return RetContinue; }),		(int& Reg0),				"Bit Not of arg1") \
	IMPL(bit_count,		MACRO_QUOTE({ return bit_popcount_h(Reg0);											 }),	(int& Reg0),				"Count number of set bits of arg1") \
	\
	IMPL(pow,			MACRO_QUOTE({ Reg0 = std::pow(Reg0, std::max(0, Data1));		return RetContinue; }),		(int& Reg0, int Data1),		"Power of arg1 to arg2") \
	IMPL(sqrt,			MACRO_QUOTE({ Reg0 = Reg0 > 0 ? std::sqrt(Reg0) : 0;			return RetContinue; }),		(int& Reg0),				"Square root of arg1") \
	\
	IMPL(abs,			MACRO_QUOTE({ Reg0 = std::abs(Reg0);							return RetContinue; }),		(int& Reg0),						"Absolute value of arg1") \
	IMPL(limit,			MACRO_QUOTE({ Reg0 = std::max(std::min(Reg0, Data2), Data1);	return RetContinue; }),		(int& Reg0, int Data1, int Data2),	"Correct value in arg1 that is always between arg2 and arg3") \
	IMPL(limit_upper,	MACRO_QUOTE({ Reg0 = std::min(Reg0, Data1);						return RetContinue; }),		(int& Reg0, int Data1),				"Correct value in arg1 that is always lesser than arg2") \
	IMPL(limit_lower,	MACRO_QUOTE({ Reg0 = std::max(Reg0, Data1);						return RetContinue; }),		(int& Reg0, int Data1),				"Correct value in arg1 that is always greater than arg2") \
	\
	IMPL(wavegen_rect,	MACRO_QUOTE({ return wavegen_rect_h(Reg0, Period1, Size2, Max3);				}),		(int& Reg0, int Period1, int Size2, int Max3),		"Square wave function, arg1 - argument and result, arg2 - period, arg3 - length of square, arg4 - height of square") \
	IMPL(wavegen_saw,	MACRO_QUOTE({ return wavegen_saw_h(Reg0, Period1, Size2, Max3);					}),		(int& Reg0, int Period1, int Size2, int Max3),		"Saw wave function, arg1 - argument and result, arg2 - period, arg3 - size of saw, arg4 - cap value") \
	IMPL(wavegen_tri,	MACRO_QUOTE({ return wavegen_tri_h(Reg0, Period1, Size2, Max3);					}),		(int& Reg0, int Period1, int Size2, int Max3),		"Triangle wave function, arg1 - argument and result, arg2 - period, arg3 - size of triangle, arg4 - cap value") \
	IMPL(wavegen_sin,	MACRO_QUOTE({ return wavegen_sin_h(Reg0, Period1, Size2);						}),		(int& Reg0, int Period1, int Size2),				"Sin wave function, arg1 - argument and result, arg2 - period, arg3 - size of amplitude") \
	IMPL(wavegen_cos,	MACRO_QUOTE({ return wavegen_cos_h(Reg0, Period1, Size2);						}),		(int& Reg0, int Period1, int Size2),				"Cos wave function, arg1 - argument and result, arg2 - period, arg3 - size of amplitude") \
	\
	IMPL(get_color,		MACRO_QUOTE({ Reg0 = Data1 >> 4;							return RetContinue; }),		(int& Reg0, int Data1),		"Get color part to arg1 of pixel color in arg2") \
	IMPL(set_color,		MACRO_QUOTE({ Reg0 = (Reg0 & 0xF) | (Data1 << 4);			return RetContinue; }),		(int& Reg0, int Data1),		"Set color part to pixel color in arg1") \
	IMPL(get_shade,		MACRO_QUOTE({ Reg0 = Data1 & 0xF;							return RetContinue; }),		(int& Reg0, int Data1),		"Get shade part to arg1 of pixel color in arg2") \
	IMPL(set_shade,		MACRO_QUOTE({ Reg0 = (Reg0 & 0xF0) | (Data1 & 0xF);			return RetContinue; }),		(int& Reg0, int Data1),		"Set color part to pixel color in arg1") \
	IMPL(add_shade,		MACRO_QUOTE({ addShade_h(Reg0, Data1);						return RetContinue; }),		(int& Reg0, int Data1),		"Add value of shade to pixel color in arg1") \
	\
	IMPL(call,			MACRO_QUOTE({ return call_func_h(c, func, d, p);								}),		(ScriptFunc func, const Uint8* d, ScriptWorkerBase& c, ProgPos& p),		"") \


////////////////////////////////////////////////////////////
//					function definition
////////////////////////////////////////////////////////////

namespace
{

/**
 * Macro returning name of function
 */
#define MACRO_FUNC_ID(id) Func_##id

/**
 * Macro used for creating functions from MACRO_PROC_DEFINITION
 */
#define MACRO_CREATE_FUNC(NAME, Impl, Args, ...) \
	struct MACRO_FUNC_ID(NAME) \
	{ \
		[[gnu::always_inline]] \
		static RetEnum func Args \
			Impl \
	};

MACRO_PROC_DEFINITION(MACRO_CREATE_FUNC)

#undef MACRO_CREATE_FUNC


struct Func_test_eq_null
{
	[[gnu::always_inline]]
	static RetEnum func (ProgPos& Prog, std::nullptr_t, std::nullptr_t, ProgPos LabelTrue, ProgPos)
	{
		Prog = LabelTrue;
		return RetContinue;
	}
};

struct Func_debug_impl_int
{
	[[gnu::always_inline]]
	static RetEnum func (ScriptWorkerBase& c, int i)
	{
		auto f = [&]{ return std::to_string(i); };
		c.log_buffer_add(&f);
		return RetContinue;
	}
};

struct Func_debug_impl_text
{
	[[gnu::always_inline]]
	static RetEnum func (ScriptWorkerBase& c, ScriptText p)
	{
		auto f = [&]{ return std::string(p); };
		c.log_buffer_add(&f);
		return RetContinue;
	}
};

struct Func_debug_flush
{
	[[gnu::always_inline]]
	static RetEnum func (ScriptWorkerBase& c, ProgPos& p)
	{
		c.log_buffer_flush(p);
		return RetContinue;
	}
};

struct Func_set_text
{
	[[gnu::always_inline]]
	static RetEnum func (ScriptWorkerBase& c, ScriptText& a, ScriptText b)
	{
		a = b;
		return RetContinue;
	}
};

struct Func_clear_text
{
	[[gnu::always_inline]]
	static RetEnum func (ScriptWorkerBase& c, ScriptText& a)
	{
		a = ScriptText::empty;
		return RetContinue;
	}
};

struct Func_test_eq_text
{
	[[gnu::always_inline]]
	static RetEnum func (ProgPos& prog, ScriptText a, ScriptText b, ProgPos labelTrue, ProgPos labelFalse)
	{
		if (a.ptr == nullptr && b.ptr == nullptr)
		{
			prog = labelTrue;
		}
		else if (a.ptr == nullptr || b.ptr == nullptr)
		{
			prog = labelFalse;
		}
		else if (strcmp(a.ptr, b.ptr) == 0)
		{
			prog = labelTrue;
		}
		else
		{
			prog = labelFalse;
		}
		return RetContinue;
	}
};

} //namespace

////////////////////////////////////////////////////////////
//					Proc_Enum definition
////////////////////////////////////////////////////////////

/**
 * Macro returning enum from ProcEnum
 */
#define MACRO_PROC_ID(id) Proc_##id

/**
 * Macro used for creating ProcEnum from MACRO_PROC_DEFINITION
 */
#define MACRO_CREATE_PROC_ENUM(NAME, ...) \
	MACRO_PROC_ID(NAME), \
	Proc_##NAME##_end = MACRO_PROC_ID(NAME) + helper::FuncGroup<MACRO_FUNC_ID(NAME)>::ver() - 1,

/**
 * Enum storing id of all available operations in script engine
 */
enum ProcEnum : Uint8
{
	MACRO_PROC_DEFINITION(MACRO_CREATE_PROC_ENUM)
	Proc_EnumMax,
};

#undef MACRO_CREATE_PROC_ENUM

////////////////////////////////////////////////////////////
//					core loop function
////////////////////////////////////////////////////////////

/**
 * Core function in script engine used to executing scripts
 * @param proc array storing operation of script
 * @return Result of executing script
 */
static inline void scriptExe(ScriptWorkerBase& data, const Uint8* proc)
{
	ProgPos curr = ProgPos::Start;
	//--------------------------------------------------
	//			helper macros for this function
	//--------------------------------------------------
	#define MACRO_FUNC_ARRAY(NAME, ...) + helper::FuncGroup<MACRO_FUNC_ID(NAME)>::FuncList{}
	#define MACRO_FUNC_ARRAY_LOOP(POS) \
		case (POS): \
		{ \
			using currType = helper::GetType<func, POS>; \
			const auto p = proc + (int)curr; \
			curr += currType::offset; \
			const auto ret = currType::func(data, p, curr); \
			if (ret != RetContinue) \
			{ \
				if (ret == RetEnd) \
					goto endLabel; \
				else \
				{ \
					curr += - currType::offset - 1; \
					goto errorLabel; \
				} \
			} \
			else \
				continue; \
		}
	//--------------------------------------------------

	using func = decltype(MACRO_PROC_DEFINITION(MACRO_FUNC_ARRAY));

	while (true)
	{
		switch (proc[(int)curr++])
		{
		MACRO_COPY_256(MACRO_FUNC_ARRAY_LOOP, 0)
		}
	}

	//--------------------------------------------------
	//			removing helper macros
	//--------------------------------------------------
	#undef MACRO_FUNC_ARRAY_LOOP
	#undef MACRO_FUNC_ARRAY
	//--------------------------------------------------

	errorLabel:
	static int bugCount = 0;
	if (++bugCount < 100)
	{
		Log(LOG_ERROR) << "Invalid script operation for OpId: " << std::hex << std::showbase << (int)proc[(int)curr] <<" at "<< (int)curr;
	}

	endLabel:
	return;
}


////////////////////////////////////////////////////////////
//						Script class
////////////////////////////////////////////////////////////

void ScriptWorkerBlit::executeBlit(const Surface* src, Surface* dest, int x, int y, int shade)
{
	executeBlit(src, dest, x, y, shade, GraphSubset{ dest->getWidth(), dest->getHeight() } );
}
/**
 * Blitting one surface to another using script.
 * @param src source surface.
 * @param dest destination surface.
 * @param x x offset of source surface.
 * @param y y offset of source surface.
 */
void ScriptWorkerBlit::executeBlit(const Surface* src, Surface* dest, int x, int y, int shade, GraphSubset mask)
{
	ShaderMove<const Uint8> srcShader(src, x, y);
	ShaderMove<Uint8> destShader(dest, 0, 0);

	destShader.setDomain(mask);

	if (_proc)
	{
		if (_events)
		{
			ShaderDrawFunc(
				[&](Uint8& destStuff, const Uint8& srcStuff)
				{
					if (srcStuff)
					{
						ScriptWorkerBlit::Output arg = { srcStuff, destStuff };
						set(arg);
						auto ptr = _events;
						while (*ptr)
						{
							reset(arg);
							scriptExe(*this, ptr->data());
							++ptr;
						}
						++ptr;

						reset(arg);
						scriptExe(*this, _proc);

						while (*ptr)
						{
							reset(arg);
							scriptExe(*this, ptr->data());
							++ptr;
						}
						++ptr;

						get(arg);
						if (arg.getFirst()) destStuff = arg.getFirst();
					}
				},
				destShader,
				srcShader
			);
		}
		else
		{
			ShaderDrawFunc(
				[&](Uint8& destStuff, const Uint8& srcStuff)
				{
					if (srcStuff)
					{
						ScriptWorkerBlit::Output arg = { srcStuff, destStuff };
						set(arg);
						scriptExe(*this, _proc);
						get(arg);
						if (arg.getFirst()) destStuff = arg.getFirst();
					}
				},
				destShader,
				srcShader
			);
		}
	}
	else
	{
		ShaderDraw<helper::StandardShade>(destShader, srcShader, ShaderScalar(shade));
	}
}

/**
 * Execute script with two arguments.
 * @return Result value from script.
 */
void ScriptWorkerBase::executeBase(const Uint8* proc)
{
	if (proc)
	{
		scriptExe(*this, proc);
	}
}

constexpr int log_buffer_limit_max = 500;
static int log_buffer_limit_count = 0;

/**
 * Add text to log buffer.
 */
void ScriptWorkerBase::log_buffer_add(FuncRef<std::string()> func)
{
	if (log_buffer_limit_count > log_buffer_limit_max)
	{
		return;
	}
	if (!log_buffer.empty())
	{
		log_buffer += " ";
	}
	log_buffer += func();
}

/**
 * Flush buffer to log file.
 */
void ScriptWorkerBase::log_buffer_flush(ProgPos& p)
{
	if (++log_buffer_limit_count < log_buffer_limit_max)
	{
		Logger log;
		log.get(LOG_DEBUG) << "Script debug log: " << log_buffer;
		log_buffer.clear();
	}
	else if (log_buffer_limit_count == log_buffer_limit_max)
	{
		Logger log;
		log.get(LOG_DEBUG) << "Script debug log limit reach";
		log_buffer.clear();
	}
}


////////////////////////////////////////////////////////////
//				ParserWriter helpers
////////////////////////////////////////////////////////////

/**
 * Token type
 */
enum TokenEnum
{
	TokenNone,
	TokenInvalid,
	TokenColon,
	TokenSemicolon,
	TokenSymbol,
	TokenNumber,
	TokenText,
};

/**
 * Struct represents position of token in input string
 */
class SelectedToken : public ScriptRef
{
	/// type of this token.
	TokenEnum _type;
	/// line where token start.
	size_t _linePos;

public:

	/// Default constructor.
	SelectedToken() : ScriptRef{ }, _type{ TokenNone }, _linePos{ 0 }
	{

	}

	/// Constructor from range.
	SelectedToken(TokenEnum type, ScriptRef range, size_t linePos) : ScriptRef{ range }, _type{ type }, _linePos{linePos}
	{

	}

	/// Get token type.
	TokenEnum getType() const
	{
		return _type;
	}

	/// Get token line postion in script.
	size_t getLinePos() const
	{
		return _linePos;
	}

	/// Convert token to script ref.
	ScriptRefData parse(const ParserWriter& ph) const
	{
		if (getType() == TokenNumber)
		{
			auto str = toString();
			int value = 0;
			size_t offset = 0;
			std::stringstream ss(str);
			if (str[0] == '-' || str[0] == '+')
				offset = 1;
			if (str.size() > 2 + offset && str[offset] == '0' && (str[offset + 1] == 'x' || str[offset + 1] == 'X'))
				ss >> std::hex;
			if ((ss >> value))
				return ScriptRefData{ *this, ArgInt, value };
		}
		else if (getType() == TokenSymbol)
		{
			auto ref = ph.getReferece(*this);
			if (ref)
				return ref;

			auto type = ArgUnknowSimple;

			for (auto& c : *this)
			{
				if (c == '.')
				{
					// look like `abc.def`
					type = std::max(type, ArgUnknowSegment);
				}
			}

			return ScriptRefData{ *this, type };
		}
		else if (getType() == TokenText)
		{
			return ScriptRefData{ *this, ArgText, static_cast<const ScriptRef&>(*this) };
		}
		return ScriptRefData{ *this, ArgInvalid };
	}

};

class ScriptRefTokens : public ScriptRef
{
	/// Current line position.
	size_t _linePos = 1;

public:
	/// Using default constructors.
	using ScriptRef::ScriptRef;

	/// Extract new token from current object.
	SelectedToken getNextToken(TokenEnum excepted = TokenNone);
};

/**
 * ScriptRef that is glue from independent parts.
 * First empty ref mean end of list.
 */
class ScriptRefCompound
{

public:

	std::array<ScriptRef, 4> parts;

	/// Default constructor.
	constexpr ScriptRefCompound() = default;

	/// Constructor from one ref.
	constexpr ScriptRefCompound(ScriptRef r) : parts{ r }
	{

	}


	template<typename Callback>
	constexpr void interateMutate(Callback&& f)
	{
		for (auto& p : parts)
		{
			if constexpr (std::is_invocable_r_v<bool, Callback, ScriptRef&>)
			{
				if (!f(p))
				{
					return;
				}
			}
			else
			{
				f(p);
			}
		}
	}

	template<typename Callback>
	constexpr void interate(Callback&& f) const
	{
		for (const auto& p : parts)
		{
			if (!p)
			{
				return;
			}

			f(p);
		}
	}


	constexpr bool tryPopBack()
	{
		ScriptRef* prev = nullptr;
		interateMutate(
			[&](ScriptRef& r)
			{
				if (r)
				{
					prev = &r;
					return true;
				}
				else
				{
					return false;
				}
			}
		);
		if (prev) *prev = {};
		return prev;
	}

	constexpr bool tryPushBack(ScriptRef n)
	{
		ScriptRef* prev = nullptr;
		interateMutate(
			[&](ScriptRef& r)
			{
				if (r)
				{
					return true;
				}
				else
				{
					prev = &r;
					return false;
				}
			}
		);
		if (prev) *prev = n;
		return prev;
	}

	constexpr void clear()
	{
		interateMutate(
			[&](ScriptRef& r)
			{
				r = {};
			}
		);
	}


	constexpr bool haveParts() const
	{
		return !!parts[1];
	}

	constexpr size_t sizeParts() const
	{
		size_t s = 0;
		interate([&](const ScriptRef& r){ s += 1; });
		return s;
	}

	constexpr size_t size() const
	{
		size_t s = 0;
		interate([&](const ScriptRef& r){ s += r.size(); });
		return s;
	}

	constexpr ScriptRef last() const
	{
		ScriptRef l;
		interate([&](const ScriptRef& r){ l = r; });
		return l;
	}

	std::string toString() const
	{
		std::string s;
		s.reserve(size());
		interate([&](const ScriptRef& r){ s.append(r.begin(), r.size()); });
		return s;
	}


	constexpr explicit operator bool() const
	{
		return !!parts[0];
	}

	constexpr operator ScriptRange<ScriptRef>() const
	{
		return { parts.data(), parts.data() + parts.size() };
	}
};

class ScriptRefOperation
{
public:
	ScriptRange<ScriptProcData> procList;
	ScriptRefCompound procName;

	ScriptRefData argRef;
	ScriptRef argName;

	/// Check if whole object is correct
	explicit operator bool() const
	{
		return
			(procName && procList) && // have function name and have related overload set
			(!argName || (argRef && procName.haveParts())) // have optional argument embedded in original operation name
		;
	}

	bool haveProc() const
	{
		return !!procName;
	}

	bool haveArg() const
	{
		return !!argName;
	}
};

class ScriptArgList
{
	size_t argsLength = 0;
	ScriptRefData args[ScriptMaxArg] = { };

public:
	/// Default constructor.
	ScriptArgList() = default;


	/// Add one arg to list.
	constexpr bool tryPushBack(const ScriptRefData& d)
	{
		if (argsLength < std::size(args))
		{
			args[argsLength++] = d;
			return true;
		}
		else
		{
			return false;
		}
	}

	/// Add arg range to list.
	constexpr bool tryPushBack(ScriptRange<ScriptRefData> l)
	{
		if (l.size() + argsLength <= std::size(args))
		{
			for (const auto& d : l)
			{
				args[argsLength++] = d;
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	/// Add arg range to list.
	constexpr bool tryPushBack(const ScriptRefData* b, const ScriptRefData* e)
	{
		return tryPushBack(ScriptRange<ScriptRefData>{b, e});
	}



	constexpr size_t size() const
	{
		return argsLength;
	}

	constexpr const ScriptRefData* begin() const
	{
		return std::begin(args);
	}

	constexpr const ScriptRefData* end() const
	{
		return std::begin(args) + argsLength;
	}

	constexpr operator ScriptRange<ScriptRefData>() const
	{
		return { begin(), end() };
	}
};


/**
 * Function extracting token from range
 * @param excepted what token type we expecting now
 * @return extracted token
 */
SelectedToken ScriptRefTokens::getNextToken(TokenEnum excepted)
{
	//groups of different types of ASCII characters
	using CharClasses = Uint8;
	struct Array // workaround for MSVC v19.20 bug where `std::array` is not `constexpr`
	{
		CharClasses arr[256];

		constexpr CharClasses& operator[](size_t t) { return arr[t]; }
		constexpr const CharClasses& operator[](size_t t) const { return arr[t]; }
	};
	static constexpr CharClasses CC_none = 0x1;
	static constexpr CharClasses CC_spec = 0x2;
	static constexpr CharClasses CC_digit = 0x4;
	static constexpr CharClasses CC_digitHex = 0x8;
	static constexpr CharClasses CC_charRest = 0x10;
	static constexpr CharClasses CC_digitSign = 0x20;
	static constexpr CharClasses CC_digitHexX = 0x40;
	static constexpr CharClasses CC_quote = 0x80;

	static constexpr Array charDecoder = (
		[]
		{
			Array r = { };
			for (int i = 0; i < 256; ++i)
			{
				if (i == '#' || i == ' ' || i == '\r' || i == '\n' || i == '\t')	r[i] |= CC_none;
				if (i == ':' || i == ';')	r[i] |= CC_spec;

				if (i == '+' || i == '-')	r[i] |= CC_digitSign;
				if (i >= '0' && i <= '9')	r[i] |= CC_digit;
				if (i >= 'A' && i <= 'F')	r[i] |= CC_digitHex;
				if (i >= 'a' && i <= 'f')	r[i] |= CC_digitHex;
				if (i == 'x' || i == 'X')	r[i] |= CC_digitHexX;

				if (i >= 'A' && i <= 'Z')	r[i] |= CC_charRest;
				if (i >= 'a' && i <= 'z')	r[i] |= CC_charRest;
				if (i == '_' || i == '.')	r[i] |= CC_charRest;

				if (i == '"')				r[i] |= CC_quote;
			}
			return r;
		}
	)();

	struct NextSymbol
	{
		char c;
		CharClasses decode;

		/// Is valid symbol
		operator bool() const { return c; }

		/// Check type of symbol
		bool is(CharClasses t) const { return decode & t; }

		/// Is this symbol starting next token?
		bool isStartOfNextToken() const { return c == 0 || is(CC_spec | CC_none); }
	};

	auto peekCharacter = [&]() -> NextSymbol const
	{
		if (_begin != _end)
		{
			const auto c = *_begin;
			return NextSymbol{ c, charDecoder[(Uint8)c] };
		}
		else
		{
			return NextSymbol{ 0, 0 };
		}
	};

	auto readCharacter = [&]() -> NextSymbol const
	{
		auto curr = peekCharacter();
		//it will stop on `\0` character
		if (curr)
		{
			if (*_begin == '\n') ++_linePos;
			++_begin;
		}
		return curr;
	};

	auto backCharacter = [&]()
	{
		--_begin;
		if (*_begin == '\n') --_linePos;
	};

	//end of sequence, quit.
	if (_begin == _end)
	{
		return SelectedToken{ };
	}

	//find first no whitespace character.
	if (peekCharacter().is(CC_none))
	{
		while(const auto next = readCharacter())
		{
			if (next.c == '#')
			{
				while(const auto comment = readCharacter())
				{
					if (comment.c == '\n')
					{
						break;
					}
				}
				continue;
			}
			else if (next.is(CC_none))
			{
				continue;
			}
			else
			{
				//not empty character, put it back
				backCharacter();
				break;
			}
		}
		if (!peekCharacter())
		{
			return SelectedToken{ };
		}
	}


	//start of new token of unknown type
	auto type = TokenInvalid;
	auto begin = _begin;
	const auto first = readCharacter();

	//text like `"abcdef"`
	if (first.is(CC_quote))
	{
		type = TokenText;
		while (const auto next = readCharacter())
		{
			if (next.c == first.c)
			{
				break;
			}
			else if (next.c == '\\')
			{
				const auto escapedChar = readCharacter();
				if (escapedChar.c == first.c)
				{
					continue;
				}
				else if (escapedChar.c == '\\')
				{
					continue;
				}
				else
				{
					type = TokenInvalid;
					break;
				}
				continue;
			}
			else if (next.c == '\n')
			{
				type = TokenInvalid;
				break;
			}
			else
			{
				//eat all other chars
				continue;
			}
		}
		if (!peekCharacter().isStartOfNextToken())
		{
			type = TokenInvalid;
		}

	}
	//special symbol like `;` or `:`
	else if (first.is(CC_spec))
	{
		if (first.c == ':')
		{
			type = excepted == TokenColon ? TokenColon : TokenInvalid;
		}
		else if (first.c == ';')
		{
			//semicolon wait for his turn, returning empty token
			if (excepted != TokenSemicolon)
			{
				backCharacter();
				type = TokenNone;
			}
			else
			{
				type = TokenSemicolon;
			}
		}
		else
		{
			type = TokenInvalid;
		}
	}
	//number like `0x1234` or `5432` or `+232`
	else if (first.is(CC_digitSign | CC_digit))
	{
		auto firstDigit = first;
		//sign
		if (firstDigit.is(CC_digitSign))
		{
			firstDigit = readCharacter();
		}
		if (firstDigit.is(CC_digit))
		{
			const auto hex = firstDigit.c == '0' && peekCharacter().is(CC_digitHexX);
			if (hex)
			{
				//eat `x`
				readCharacter();
			}
			else
			{
				//at least we have already one digit
				type = TokenNumber;
			}

			const CharClasses serachClass = hex ? (CC_digitHex | CC_digit) : CC_digit;

			while (const auto next = readCharacter())
			{
				//end of symbol
				if (next.isStartOfNextToken())
				{
					backCharacter();
					break;
				}
				else if (next.is(serachClass))
				{
					type = TokenNumber;
				}
				else
				{
					type = TokenInvalid;
					break;
				}
			}
		}
	}
	//symbol like `abcd` or `p12345`
	else if (first.is(CC_charRest))
	{
		type = TokenSymbol;
		while (const auto next = readCharacter())
		{
			//end of symbol
			if (next.isStartOfNextToken())
			{
				backCharacter();
				break;
			}
			else if (!next.is(CC_charRest | CC_digit))
			{
				type = TokenInvalid;
				break;
			}
		}

	}
	auto end = _begin;
	return SelectedToken{ type, ScriptRef{ begin, end }, _linePos };
}


////////////////////////////////////////////////////////////
//					Helper functions
////////////////////////////////////////////////////////////


namespace
{


////////////////////////////////////////////////////////////
//			Handle of overload arguments
////////////////////////////////////////////////////////////


/**
 * Test for validity of arguments.
 */
bool validOverloadProc(const ScriptRange<ScriptRange<ArgEnum>>& overload)
{
	for (auto& p : overload)
	{
		for (auto& pp : p)
		{
			if (pp == ArgInvalid)
			{
				return false;
			}
		}
	}
	return true;
}

std::string displayType(const ScriptParserBase* spb, ArgEnum type)
{
	std::string result = "";
	result += "[";
	result += spb->getTypePrefix(type);
	result += spb->getTypeName(type).toString();
	result += "]";
	return result;
}

/**
 * Display arguments
 */
template<typename T, typename F>
std::string displayArgs(const ScriptParserBase* spb, const ScriptRange<T>& range, F getType)
{
	std::string result = "";
	for (auto& p : range)
	{
		auto type = getType(p);
		if (type != ArgInvalid)
		{
			result += "[";
			result += spb->getTypePrefix(type);
			result += spb->getTypeName(type).toString();
			result += "] ";
		}
	}
	if (!result.empty())
	{
		result.pop_back();
	}
	return result;
}

/**
 * Display arguments
 */
std::string displayOverloadProc(const ScriptParserBase* spb, const ScriptRange<ScriptRange<ArgEnum>>& overload)
{
	return displayArgs(spb, overload, [](const ScriptRange<ArgEnum>& o) { return o ? *o.begin() : ArgInvalid; });
}

/**
 * Accept all arguments.
 */
int overloadBuildinProc(const ScriptProcData& spd, const ScriptRefData* begin, const ScriptRefData* end)
{
	return 1;
}

/**
 * Reject all arguments.
 */
int overloadInvalidProc(const ScriptProcData& spd, const ScriptRefData* begin, const ScriptRefData* end)
{
	return 0;
}

/**
 * Verify arguments.
 */
int overloadCustomProc(const ScriptProcData& spd, const ScriptRefData* begin, const ScriptRefData* end)
{
	auto tempSorce = 255;
	auto curr = begin;
	for (auto& currOver : spd.overloadArg)
	{
		if (curr == end)
		{
			return 0;
		}
		const auto size = currOver.size();
		if (size)
		{
			if (ArgBase(curr->type) != ArgInvalid)
			{
				int oneArgTempScore = 0;
				for (auto& o : currOver)
				{
					oneArgTempScore = std::max(oneArgTempScore, ArgCompatible(o, curr->type, size - 1));
				}
				tempSorce = std::min(tempSorce, oneArgTempScore);
			}
			++curr;
		}
	}
	if (curr != end)
	{
		return 0;
	}
	return tempSorce;
}

/**
 * Return public argument number of given function.
 */
int getOverloadArgSize(ScriptRange<ScriptRange<ArgEnum>> over)
{
	int argSize = 0;

	for (auto& currOver : over)
	{
		if (currOver)
		{
			argSize += 1;
		}
	}

	return argSize;
}

/**
 * Return public argument number of given function.
 */
[[maybe_unused]]
int getOverloadArgSize(const ScriptProcData& spd)
{
	return getOverloadArgSize(spd.overloadArg);
}

/**
 * Return type of public argument of given function.
 */
ScriptRange<ArgEnum> getOverloadArgType(ScriptRange<ScriptRange<ArgEnum>> over, int argPos)
{
	for (auto& currOver : over)
	{
		if (currOver)
		{
			if (argPos == 0)
			{
				return currOver;
			}
			--argPos;
		}
	}

	return {};
}

/**
 * Return type of public argument of given function.
 */
[[maybe_unused]]
ScriptRange<ArgEnum> getOverloadArgType(const ScriptProcData& spd, int argPos)
{
	return getOverloadArgType(spd.overloadArg, argPos);
}

/**
 * Return tail type list of public arguments of given function.
 */
ScriptRange<ScriptRange<ArgEnum>> getOverloadArgTypeTail(ScriptRange<ScriptRange<ArgEnum>> over, int argPos)
{
	for (auto& currOver : over)
	{
		if (currOver)
		{
			if (argPos == 0)
			{
				return { &currOver, over.end() };
			}
			--argPos;
		}
	}

	return {};
}

/**
 * Return tail type list of public arguments of given function.
 */
ScriptRange<ScriptRange<ArgEnum>> getOverloadArgTypeTail(const ScriptProcData& spd, int argPos)
{
	return getOverloadArgTypeTail(spd.overloadArg, argPos);
}

std::tuple<int, const ScriptProcData*> findBestOverloadProc(const ScriptRange<ScriptProcData>& proc, const ScriptRefData* begin, const ScriptRefData* end)
{
	int bestSorce = 0;
	const ScriptProcData* bestValue = nullptr;
	for (auto& p : proc)
	{
		int tempSorce = p.overload(p, begin, end);
		if (tempSorce)
		{
			if (tempSorce == bestSorce)
			{
				bestValue = nullptr;
			}
			else if (tempSorce > bestSorce)
			{
				bestSorce = tempSorce;
				bestValue = &p;
			}
		}
	}

	return std::make_tuple(bestSorce, bestValue);
}

ScriptRefOperation findOperationAndArg(const ParserWriter& ph, ScriptRef op)
{
	ScriptRefOperation result;

	result.procName = op;
	result.procList = ph.parser.getProc(op);
	if (!result)
	{
		auto first_dot = op.find('.');
		if (first_dot == std::string::npos)
		{
			return result;
		}

		result.argName = op.head(first_dot);
		result.argRef = ph.getReferece(result.argName);
		if (!result.argRef)
		{
			auto origArgName = result.argName;

			++first_dot; //skip '.'
			auto second_dot = op.tail(first_dot).find('.');
			if (second_dot == std::string::npos)
			{
				return result;
			}
			second_dot += first_dot;
			result.argName = op.head(second_dot);
			result.argRef = ph.getReferece(result.argName);
			if (!result.argRef)
			{
				// restore initial name for error propose, but only if is unknown. Other wise typo should be in next part
				if (isKnowNamePrefix(origArgName) == false)
				{
					result.argName = origArgName;
				}
				return result;
			}
			first_dot = second_dot;
		}

		auto name = ph.parser.getTypeName(result.argRef.type);
		if (result.argRef.type < ArgMax || !name)
		{
			return result;
		}

		result.procName.parts = { name, op.tail(first_dot) };
		result.procList = ph.parser.getProc(result.procName);
	}

	return result;
}

ScriptRefOperation replaceOperation(const ParserWriter& ph, const ScriptRefOperation& op, ScriptRef from, ScriptRef to)
{
	ScriptRefOperation result = op;

	bool correct = false;
	if (result.procName.size())
	{
		auto last = result.procName.last();
		auto lastHead = last.headFromEnd(from.size());
		auto lastTail = last.tailFromEnd(from.size());
		if (lastHead == from)
		{
			correct = true;
			correct &= result.procName.tryPopBack();
			if (lastTail)
			{
				correct &= result.procName.tryPushBack(lastTail);
			}
			correct &= result.procName.tryPushBack(to);
			correct &= bool(result.procList = ph.parser.getProc(result.procName));
		}
	}

	if (correct)
	{
		return result;
	}
	else
	{
		return {};
	}
}

void logErrorOnOperationArg(const ScriptRefOperation& op)
{
	if (op)
	{
		return;
	}

	if (op.haveArg())
	{
		if (op.argRef)
		{
			if (op.procName.haveParts())
			{
				Log(LOG_ERROR) << "Unknown operation name '" << op.procName.toString() << "' for variable '" << op.argName.toString() << "'";
			}
			else
			{
				Log(LOG_ERROR) << "Unsupported type for variable '" << op.argName.toString() << "'";
			}
		}
		else
		{
			Log(LOG_ERROR) << "Unknown variable name '" << op.argName.toString() << "'";
		}
	}
}

////////////////////////////////////////////////////////////
//			Pushing operation on proc vector
////////////////////////////////////////////////////////////


/**
 * Helper choosing correct overload function to call.
 */
bool parseOverloadProc(ParserWriter& ph, const ScriptRange<ScriptProcData>& proc, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (!proc)
	{
		return false;
	}
	if ((size_t)std::distance(begin, end) > ScriptMaxArg)
	{
		return false;
	}

	int bestSorce = 0;
	const ScriptProcData* bestValue = nullptr;

	std::tie(bestSorce, bestValue) = findBestOverloadProc(proc, begin, end);

	if (bestSorce)
	{
		if (bestValue)
		{
			if ((*bestValue)(ph, begin, end) == false)
			{
				Log(LOG_ERROR) << "Error in matching arguments for operator '" + proc.begin()->name.toString() + "'";
				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			Log(LOG_ERROR) << "Conflicting overloads for operator '" + proc.begin()->name.toString() + "' for:";
			Log(LOG_ERROR) << "  " << displayArgs(&ph.parser, ScriptRange<ScriptRefData>{ begin, end }, [](const ScriptRefData& r){ return r.type; });
			Log(LOG_ERROR) << "Expected:";
			for (auto& p : proc)
			{
				if (p.parserArg != nullptr && p.overloadArg)
				{
					Log(LOG_ERROR) << "  " << displayOverloadProc(&ph.parser, p.overloadArg);
				}
			}
			return false;
		}
	}
	else
	{
		Log(LOG_ERROR) << "Can't match overload for operator '" + proc.begin()->name.toString() + "' for:";
		Log(LOG_ERROR) << "  " << displayArgs(&ph.parser, ScriptRange<ScriptRefData>{ begin, end }, [](const ScriptRefData& r){ return r.type; });
		Log(LOG_ERROR) << "Expected:";
		for (auto& p : proc)
		{
			if (p.parserArg != nullptr && p.overloadArg)
			{
				Log(LOG_ERROR) << "  " << displayOverloadProc(&ph.parser, p.overloadArg);
			}
		}
		return false;
	}
}

/**
 * Helper used to parse line for build in function.
 */
template<Uint8 procId, typename FuncGroup>
bool parseBuildinProc(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	auto opPos = ph.pushProc(procId);
	int ver = FuncGroup::parse(ph, begin, end);
	if (ver >= 0)
	{
		ph.updateProc(opPos, ver);
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Helper used to parse line for custom functions.
 */
bool parseCustomProc(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	using argFunc = typename helper::ArgSelector<ScriptFunc>::type;
	using argRaw = typename helper::ArgSelector<const Uint8*>::type;
	static_assert(helper::FuncGroup<Func_call>::ver() == argRaw::ver(), "Invalid size");
	static_assert(std::is_same<helper::GetType<helper::FuncGroup<Func_call>, 0>, argFunc>::value, "Invalid first argument");
	static_assert(std::is_same<helper::GetType<helper::FuncGroup<Func_call>, 1>, argRaw>::value, "Invalid second argument");

	auto opPos = ph.pushProc(Proc_call);

	auto funcPos = ph.pushReserved<ScriptFunc>();
	auto argPosBegin = ph.getCurrPos();

	auto argType = spd.parserArg(ph, begin, end);

	if (argType < 0)
	{
		return false;
	}

	auto argPosEnd = ph.getCurrPos();
	ph.updateReserved<ScriptFunc>(funcPos, spd.parserGet(argType));

	size_t diff = ph.getDiffPos(argPosBegin, argPosEnd);
	for (int i = 0; i < argRaw::ver(); ++i)
	{
		size_t off = argRaw::offset(i);
		if (off >= diff)
		{
			//align proc to fit fixed size.
			ph.push(off-diff);
			ph.updateProc(opPos, i);
			return true;
		}
	}
	return false;
}


////////////////////////////////////////////////////////////
//		Custom parsers of operation arguments
////////////////////////////////////////////////////////////


constexpr size_t ConditionSize = 6;
constexpr ScriptRef ConditionNames[ConditionSize] =
{
	ScriptRef{ "eq" }, ScriptRef{ "neq" },
	ScriptRef{ "le" }, ScriptRef{ "gt" },
	ScriptRef{ "ge" }, ScriptRef{ "lt" },
};

constexpr size_t ConditionSpecialSize = 2;
constexpr ScriptRef ConditionSpecNames[ConditionSpecialSize] =
{
	ScriptRef{ "or" },
	ScriptRef{ "and" },
};


/**
 * Helper used of condition operations.
 */
bool parseConditionImpl(ParserWriter& ph, ScriptRefData truePos, ScriptRefData falsePos, const ScriptRefData* begin, const ScriptRefData* end)
{
	constexpr size_t TempArgsSize = 4;

	if (std::distance(begin, end) != 3)
	{
		Log(LOG_ERROR) << "Invalid length of condition arguments";
		return false;
	}

	ScriptRefData conditionArgs[TempArgsSize] =
	{
		begin[1],
		begin[2],
		truePos, //success
		falsePos, //failure
	};

	bool equalFunc = false;
	size_t i = 0;
	for (; i < ConditionSize; ++i)
	{
		if (begin[0].name == ConditionNames[i])
		{
			if (i < 2) equalFunc = true;
			if (i & 1) std::swap(conditionArgs[2], conditionArgs[3]); //negate condition result
			if (i >= 4) std::swap(conditionArgs[0], conditionArgs[1]); //swap condition args
			break;
		}
	}
	if (i == ConditionSize)
	{
		Log(LOG_ERROR) << "Unknown condition: '" + begin[0].name.toString() + "'";
		return false;
	}

	const auto proc = ph.parser.getProc(equalFunc ? ScriptRef{ "test_eq" } : ScriptRef{ "test_le" });
	if (parseOverloadProc(ph, proc, std::begin(conditionArgs), std::end(conditionArgs)) == false)
	{
		Log(LOG_ERROR) << "Unsupported operator: '" + begin[0].name.toString() + "'";
		return false;
	}

	return true;
}

/**
 * Parse `or` or `and` conditions.
 */
bool parseFullConditionImpl(ParserWriter& ph, ScriptRefData falsePos, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (std::distance(begin, end) <= 1)
	{
		Log(LOG_ERROR) << "Invalid length of condition arguments";
		return false;
	}

	// each operation can fail, we can't prevent
	auto correct = true;

	const auto truePos = ph.addLabel();
	const auto orFunc = begin[0].name == ConditionSpecNames[0];
	const auto andFunc = begin[0].name == ConditionSpecNames[1];
	if (orFunc || andFunc)
	{
		++begin;
		for (; std::distance(begin, end) > 3; begin += 3)
		{
			auto temp = ph.addLabel();
			if (orFunc)
			{
				correct &= parseConditionImpl(ph, truePos, temp, begin, begin + 3);
			}
			else
			{
				correct &= parseConditionImpl(ph, temp, falsePos, begin, begin + 3);
			}
			correct &= ph.setLabel(temp, ph.getCurrPos());
		}
	}
	correct &= parseConditionImpl(ph, truePos, falsePos, begin, end);

	correct &= ph.setLabel(truePos, ph.getCurrPos());
	return correct;
}

/**
 * Helper used of variable operation declaration.
 */
bool parseVariableImpl(ParserWriter& ph, ScriptRefData reg, ScriptRefData val = {})
{
	if (!ArgIsReg(reg.type))
	{
		Log(LOG_ERROR) << "Invalid register";
		return false;
	}

	if (val)
	{
		ScriptRefData setArgs[] =
		{
			reg,
			val,
		};
		const auto proc = ph.parser.getProc(ScriptRef{ "set" });
		return parseOverloadProc(ph, proc, std::begin(setArgs), std::end(setArgs));
	}
	else
	{
		ScriptRefData setArgs[] =
		{
			reg,
		};
		const auto proc = ph.parser.getProc(ScriptRef{ "clear" });
		return parseOverloadProc(ph, proc, std::begin(setArgs), std::end(setArgs));
	}
}

/**
 * Parser of `if` operation.
 */
bool parseIf(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	auto& block = ph.pushScopeBlock(BlockIf);
	block.nextLabel = ph.addLabel();
	block.finalLabel = ph.addLabel();

	return parseFullConditionImpl(ph, block.nextLabel, begin, end);
}

/**
 * Parser of `else` operation.
 */
bool parseElse(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (ph.codeBlocks.back().type != BlockIf)
	{
		Log(LOG_ERROR) << "Unexpected 'else'";
		return false;
	}

	// each operation can fail, we can't prevent
	auto correct = true;

	auto& block = ph.clearScopeBlock();

	ph.pushProc(Proc_goto);
	correct &= ph.pushLabelTry(block.finalLabel);

	correct &= ph.setLabel(block.nextLabel, ph.getCurrPos());
	if (std::distance(begin, end) == 0)
	{
		block.nextLabel = block.finalLabel;
		block.type = BlockElse;
	}
	else
	{
		block.nextLabel = ph.addLabel();
		correct &= parseFullConditionImpl(ph, block.nextLabel, begin, end);
	}


	if (correct)
	{
		return true;
	}
	else
	{
		Log(LOG_ERROR) << "Error in processing 'else'";
		return false;
	}
}

/**
 * Parser of `begin` operation.
 */
bool parseBegin(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (std::distance(begin, end) != 0)
	{
		Log(LOG_ERROR) << "Unexpected symbols after 'begin'";
		return false;
	}

	ph.pushScopeBlock(BlockBegin);

	return true;
}

/**
 * Parser of `loop` operation.
 */
bool parseLoop(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (std::distance(begin, end) < 3)
	{
		Log(LOG_ERROR) << "Missing symbols after 'loop'";
		return false;
	}
	if (begin[0].name != ScriptRef{ "var" })
	{
		Log(LOG_ERROR) << "After 'loop' should be 'var'";
		return false;
	}

	// each operation can fail, we can't prevent
	auto correct = true;

	// we support simple `loop var x 100;` or complex like `loop var x obj.getInv.list "BIG_GUN";`
	const auto functionPostfix = ScriptRef{ ".list" };
	const auto functionName = begin[2].name;
	const auto functionArgSep = ph.getReferece(ScriptRef{ "__" });
	const auto functionArgPh = ph.getReferece(ScriptRef{ "_" });

	assert(!!functionArgSep);
	assert(!!functionArgPh);

	if (functionName.headFromEnd(functionPostfix.size()) == functionPostfix && !isKnowNamePrefix(functionName.tailFromEnd(functionPostfix.size())))
	{
		auto& loop = ph.pushScopeBlock(BlockLoop);
		loop.nextLabel = ph.addLabel();
		loop.finalLabel = ph.addLabel();

		ScriptArgList loopArgs = {};

		auto getProcAndRegTypes = [&](const ScriptRefOperation& proc, size_t placeHolders) -> std::tuple<const ScriptProcData*, ScriptRange<ScriptRange<ArgEnum>>>
		{
			ScriptArgList temp;
			temp.tryPushBack(loopArgs);
			size_t org = temp.size();
			for (size_t i = 0; i < placeHolders; ++i)
			{
				if (!temp.tryPushBack(functionArgPh))
				{
					return {};
				}
			}

			auto bestOverload = std::get<const ScriptProcData*>(findBestOverloadProc(proc.procList, std::begin(temp), std::end(temp)));
			if (!bestOverload)
			{
				Log(LOG_ERROR) << "Conflicting overloads for operator '" + proc.procList.begin()->name.toString() + "' for:";
				Log(LOG_ERROR) << "  " << displayArgs(&ph.parser, ScriptRange<ScriptRefData>{ temp }, [](const ScriptRefData& r){ return r.type; });
				Log(LOG_ERROR) << "Expected:";
				for (auto& p : proc.procList)
				{
					if (p.parserArg != nullptr && p.overloadArg)
					{
						Log(LOG_ERROR) << "  " << displayOverloadProc(&ph.parser, p.overloadArg);
					}
				}
				return {};
			}

			return std::make_tuple(bestOverload, getOverloadArgTypeTail(*bestOverload, org));
		};

		auto parseReg = [&](ScriptRef name, ScriptRange<ArgEnum> types) -> ScriptRefData
		{
			if (types.size() != 1)
			{
				return {};
			}

			auto c = true;
			auto r = ph.addReg(name, ArgSpecAdd(*types.begin(), ArgSpecVar));
			c &= !!r;
			c &= loopArgs.tryPushBack(r);
			c &= parseVariableImpl(ph, r);
			if (c)
			{
				return r;
			}
			else
			{
				return {};
			}
		};


		// now we known that parameter look like `obj.foo.list` but not like `Tag.list`
		auto loopFunction = findOperationAndArg(ph, functionName);
		auto initFunction = replaceOperation(ph, loopFunction, functionPostfix, ScriptRef{".init"});


		if (!loopFunction)
		{
			logErrorOnOperationArg(loopFunction);
			Log(LOG_ERROR) << "Unsupported function '" << functionName.toString() << "' for 'loop'";
			return false;
		}

		if (!initFunction)
		{
			Log(LOG_ERROR) << "Unsupported function '" << functionName.toString() << "' for 'loop'";
			return false;
		}

		correct &= loopArgs.tryPushBack(loopFunction.argRef);
		correct &= loopArgs.tryPushBack(begin + 3, end);
		correct &= loopArgs.tryPushBack(functionArgSep);

		// init part of loop, try parse arg types of control registers
		auto [initBestProc, initBestOverload] = getProcAndRegTypes(initFunction, 2);
		if (!correct || getOverloadArgSize(initBestOverload) != 2)
		{
			Log(LOG_ERROR) << "Error in processing init of 'loop'";
			return false;
		}
		auto curr = parseReg({}, getOverloadArgType(initBestOverload, 0));
		auto limit = parseReg({}, getOverloadArgType(initBestOverload, 1));
		correct &= !!curr;
		correct &= !!limit;
		correct &= parseCustomProc(*initBestProc, ph, std::begin(loopArgs), std::end(loopArgs));


		// check part of loop, break if control register are equal
		correct &= ph.setLabel(loop.nextLabel, ph.getCurrPos());
		ScriptRefData breakCond[] =
		{
			ScriptRefData { ScriptRef{ "lt" }, ArgInvalid },
			curr,
			limit,
		};
		correct &= parseFullConditionImpl(ph, loop.finalLabel, std::begin(breakCond), std::end(breakCond));


		// increment part and getting current element of loop
		correct &= loopArgs.tryPushBack(functionArgSep);
		auto [loopBestProc, loopBestOverload] = getProcAndRegTypes(loopFunction, 1);
		if (!correct || getOverloadArgSize(loopBestOverload) != 1)
		{
			Log(LOG_ERROR) << "Error in processing step of 'loop'";
			return false;
		}

		auto var = parseReg(begin[1].name, getOverloadArgType(loopBestOverload, 0));
		correct &= !!var;
		correct &= parseCustomProc(*loopBestProc, ph, std::begin(loopArgs), std::end(loopArgs));
	}
	else
	{
		auto& loop = ph.pushScopeBlock(BlockLoop);
		loop.nextLabel = ph.addLabel();
		loop.finalLabel = ph.addLabel();

		auto limit = ph.addReg({}, ArgSpecAdd(ArgInt, ArgSpecVar));
		auto curr = ph.addReg({}, ArgSpecAdd(ArgInt, ArgSpecVar));
		auto var = ph.addReg(begin[1].name, ArgSpecAdd(ArgInt, ArgSpecVar));

		correct &= !!limit;
		correct &= !!curr;
		correct &= !!var;

		correct &= parseVariableImpl(ph, limit, begin[2]);
		correct &= parseVariableImpl(ph, curr);

		correct &= ph.setLabel(loop.nextLabel, ph.getCurrPos());

		ScriptRefData breakCond[] =
		{
			ScriptRefData { ScriptRef{ "lt" }, ArgInvalid },
			curr,
			limit,
		};
		correct &= parseFullConditionImpl(ph, loop.finalLabel, std::begin(breakCond), std::end(breakCond));

		correct &= parseVariableImpl(ph, var, curr);

		ScriptRefData addArgs[] =
		{
			curr,
			{ {}, ArgInt, 1 },
		};
		correct &= parseOverloadProc(ph, ph.parser.getProc(ScriptRef{ "add" }), std::begin(addArgs), std::end(addArgs));
	}

	if (correct)
	{
		return true;
	}
	else
	{
		Log(LOG_ERROR) << "Error in processing 'loop'";
		return false;
	}
}

/**
 * Get first outer scope of given type.
 * @param ph Script writer
 * @param type Type of block
 * @return Pointer to block or null if there is not bock of given type.
 */
ParserWriter::Block* getTopBlockOfType(ParserWriter& ph, BlockEnum type)
{
	for (auto& b : Collections::reverse(Collections::range(ph.codeBlocks)))
	{
		if (b.type == type)
		{
			return &b;
		}
	}
	return nullptr;
}

/**
 * Parser of `break` operation.
 */
bool parseBreak(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (std::distance(begin, end) != 0)
	{
		Log(LOG_ERROR) << "Unexpected symbols after 'break'";
		return false;
	}

	auto* loopBlock = getTopBlockOfType(ph, BlockLoop);
	if (!loopBlock)
	{
		Log(LOG_ERROR) << "Operation 'break' outside 'loop'";
		return false;
	}

	// each operation can fail, we can't prevent
	auto correct = true;

	ph.pushProc(Proc_goto);
	correct &= ph.pushLabelTry(loopBlock->finalLabel);

	//TODO: add handling similar to `break eq x y;`


	if (correct)
	{
		return true;
	}
	else
	{
		Log(LOG_ERROR) << "Error in processing 'break'";
		return false;
	}
}

/**
 * Parser of `continue` operation.
 */
bool parseContinue(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (std::distance(begin, end) != 0)
	{
		Log(LOG_ERROR) << "Unexpected symbols after 'continue'";
		return false;
	}

	auto* loopBlock = getTopBlockOfType(ph, BlockLoop);
	if (!loopBlock)
	{
		Log(LOG_ERROR) << "Operation 'continue' outside 'loop'";
		return false;
	}

	// each operation can fail, we can't prevent
	auto correct = true;

	ph.pushProc(Proc_goto);
	correct &= ph.pushLabelTry(loopBlock->nextLabel);

	//TODO: add handling similar to `continue eq x y;`


	if (correct)
	{
		return true;
	}
	else
	{
		Log(LOG_ERROR) << "Error in processing 'continue'";
		return false;
	}
}

/**
 * Parser of `end` operation.
 */
bool parseEnd(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (ph.codeBlocks.back().type == BlockMain)
	{
		Log(LOG_ERROR) << "Unexpected 'end'";
		return false;
	}
	if (std::distance(begin, end) != 0)
	{
		Log(LOG_ERROR) << "Unexpected symbols after 'end'";
		return false;
	}

	// each operation can fail, we can't prevent
	auto correct = true;

	auto block = ph.popScopeBlock();

	switch (block.type)
	{
	case BlockIf:
	case BlockElse:
		if (block.nextLabel.value != block.finalLabel.value)
		{
			correct &= ph.setLabel(block.nextLabel, ph.getCurrPos());
		}
		correct &= ph.setLabel(block.finalLabel, ph.getCurrPos());
		break;

	case BlockBegin:
		// nothing
		break;

	case BlockLoop:
		ph.pushProc(Proc_goto);
		correct &= ph.pushLabelTry(block.nextLabel);

		correct &= ph.setLabel(block.finalLabel, ph.getCurrPos());
		break;

	default:
		throw Exception("Unsuported block type");
	}


	if (correct)
	{
		return true;
	}
	else
	{
		Log(LOG_ERROR) << "Error in processing 'end'";
		return false;
	}
}

/**
 * Parser of `var` operation that define local variables.
 */
bool parseVar(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	auto spec = ArgSpecVar;
	if (begin != end)
	{
		if (begin[0].name == ScriptRef{ "ptr" })
		{
			spec = spec | ArgSpecPtr;
			++begin;
		}
		else if (begin[0].name == ScriptRef{ "ptre" })
		{
			spec = spec | ArgSpecPtrE;
			++begin;
		}
	}
	auto size = std::distance(begin, end);
	if (size < 2 || 3 < size)
	{
		Log(LOG_ERROR) << "Invalid length of 'var' definition";
		return false;
	}

	// adding new custom variables of type selected type.
	auto type_curr = ph.parser.getType(begin[0].name);
	if (!type_curr)
	{
		Log(LOG_ERROR) << "Invalid type '" << begin[0].name.toString() << "'";
		return false;
	}

	if (type_curr->meta.size == 0 && !(spec & ArgSpecPtr))
	{
		Log(LOG_ERROR) << "Can't create variable of type '" << begin[0].name.toString() << "', require 'ptr'";
		return false;
	}

	++begin;
	if (begin[0].type != ArgUnknowSimple || !begin[0].name)
	{
		Log(LOG_ERROR) << "Invalid variable name '" << begin[0].name.toString() << "'";
		return false;
	}
	if (ph.parser.getType(begin[0].name))
	{
		Log(LOG_ERROR) << "Invalid variable name '" << begin[0].name.toString() << "' same as existing type";
		return false;
	}
	if (ph.parser.getProc(begin[0].name))
	{
		Log(LOG_ERROR) << "Invalid variable name '" << begin[0].name.toString() << "' same as existing function";
		return false;
	}

	auto reg = ph.addReg(begin[0].name, ArgSpecAdd(type_curr->type, spec));
	if (!reg)
	{
		Log(LOG_ERROR) << "Invalid type for variable '" << begin[0].name.toString() << "'";
		return false;
	}

	// each operation can fail, we can't prevent
	auto correct = true;

	if (size == 2)
	{
		correct &= parseVariableImpl(ph, reg);
	}
	else
	{
		correct &= parseVariableImpl(ph, reg, begin[1]);
	}


	if (correct)
	{
		return true;
	}
	else
	{
		Log(LOG_ERROR) << "Error in processing 'var'";
		return false;
	}
}

/**
 * Parser of `const` operation that define local variables.
 */
bool parseConst(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	auto spec = ArgSpecNone;
	if (begin != end)
	{
		if (begin[0].name == ScriptRef{ "ptr" })
		{
			spec = spec | ArgSpecPtr;
			++begin;
		}
		else if (begin[0].name == ScriptRef{ "ptre" })
		{
			spec = spec | ArgSpecPtrE;
			++begin;
		}
	}
	auto size = std::distance(begin, end);
	if (3 != size)
	{
		Log(LOG_ERROR) << "Invalid length of 'const' definition";
		return false;
	}

	// adding new custom variables of type selected type.
	auto type_curr = ph.parser.getType(begin[0].name);
	if (!type_curr)
	{
		Log(LOG_ERROR) << "Invalid type '" << begin[0].name.toString() << "'";
		return false;
	}

	if (type_curr->meta.size == 0 && !(spec & ArgSpecPtr))
	{
		Log(LOG_ERROR) << "Can't create const of type '" << begin[0].name.toString() << "', require 'ptr'";
		return false;
	}

	++begin;
	if (begin[0].type != ArgUnknowSimple || !begin[0].name)
	{
		Log(LOG_ERROR) << "Invalid const name '" << begin[0].name.toString() << "'";
		return false;
	}
	if (ph.parser.getType(begin[0].name))
	{
		Log(LOG_ERROR) << "Invalid variable name '" << begin[0].name.toString() << "' same as existing type";
		return false;
	}
	if (ph.parser.getProc(begin[0].name))
	{
		Log(LOG_ERROR) << "Invalid variable name '" << begin[0].name.toString() << "' same as existing function";
		return false;
	}

	auto type =  ArgSpecAdd(type_curr->type, spec);

	if (type != begin[1].type)
	{
		Log(LOG_ERROR) << "Invalid value '"<< begin[1].name.toString() << "' for const type '" << begin[0].name.toString() << "'";
		return false;
	}

	auto reg = ph.addConst(begin[0].name, type, begin[1].value);
	if (!reg)
	{
		Log(LOG_ERROR) << "Invalid type for const '" << begin[0].name.toString() << "'";
		return false;
	}

	return true;
}

/**
 * Parse return statement.
 */
bool parseReturn(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	const auto size = std::distance(begin, end);
	const auto returnSize = ph.parser.haveEmptyReturn() ? 0 : ph.parser.getParamSize();
	if (returnSize != size)
	{
		Log(LOG_ERROR) << "Invalid length of returns arguments";
		return false;
	}

	ScriptRefData outputRegsData[ScriptMaxOut] = { };
	RegEnum currValueIndex[ScriptMaxOut] = { };
	RegEnum newValueIndex[ScriptMaxOut] = { };

	for (int i = 0; i < size; ++i)
	{
		outputRegsData[i] = *ph.parser.getParamData(i);
		if (begin[i].isValueType<RegEnum>() && !ArgCompatible(outputRegsData[i].type, begin[i].type, 1))
		{
			Log(LOG_ERROR) << "Invalid return argument '" + begin[i].name.toString() + "'";
			return false;
		}
		currValueIndex[i] = outputRegsData[i].getValue<RegEnum>();
		newValueIndex[i] = begin[i].getValueOrDefulat<RegEnum>(RegInvalid);
		if (currValueIndex[i] == newValueIndex[i])
		{
			currValueIndex[i] = RegInvalid;
		}
	}

	// matching return arguments to return register,
	// sometimes current value in one register is needed in another.
	// we need find order of assignments that will not lose any value.
	auto any_changed = true;
	auto all_free = false;
	while (!all_free && any_changed)
	{
		all_free = true;
		any_changed = false;
		for (int i = 0; i < size; ++i)
		{
			if (currValueIndex[i] == RegInvalid)
			{
				continue;
			}
			auto free = true;
			for (int j = 0; j < size; ++j)
			{
				if (i != j && currValueIndex[i] == newValueIndex[j])
				{
					free = false;
					break;
				}
			}
			if (free)
			{
				any_changed = true;
				currValueIndex[i] = RegInvalid;
				ScriptRefData temp[] = { outputRegsData[i], begin[i] };

				const auto proc = ph.parser.getProc(ScriptRef{ "set" });
				if (!parseOverloadProc(ph, proc, std::begin(temp), std::end(temp)))
				{
					Log(LOG_ERROR) << "Invalid return argument '" + begin[i].name.toString() + "'";
					return false;
				}
			}
			else
			{
				all_free = false;
			}
		}
	}

	if (!all_free)
	{
		// now we can have only cases where return register have circular dependencies:
		// e.g. A need B, B need C and C need A.
		// using swap we can fix circular dependencies.
		for (int i = 0; i < size; ++i)
		{
			if (currValueIndex[i] == RegInvalid)
			{
				continue;
			}

			for (int j = 0; j < size; ++j)
			{
				if (i != j && newValueIndex[i] == currValueIndex[j])
				{
					ScriptRefData temp[] = { outputRegsData[i], outputRegsData[j] };

					const auto proc = ph.parser.getProc(ScriptRef{ "swap" });
					if (!parseOverloadProc(ph, proc, std::begin(temp), std::end(temp)))
					{
						return false;
					}

					// now value from 'i' is in 'j'
					currValueIndex[j] = currValueIndex[i];
					currValueIndex[i] = RegInvalid;
					break;
				}
			}
		}
	}

	ph.pushProc(Proc_exit);
	return true;
}

/**
 * Parse `debug_log` operator.
 */
bool parseDebugLog(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (!Options::debug)
	{
		return true;
	}

	for (auto i = begin; i != end; ++i)
	{
		const auto proc = ph.parser.getProc(ScriptRef{ "debug_impl" });
		if (!parseOverloadProc(ph, proc, i, std::next(i)))
		{
			Log(LOG_ERROR) << "Invalid debug argument '" + i->name.toString() + "'";
			return false;
		}
	}

	const auto proc = ph.parser.getProc(ScriptRef{ "debug_flush" });
	return proc.size() == 1 && (*proc.begin())(ph, nullptr, nullptr);
}

/**
 * Dummy operation.
 */
bool parseDummy(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	Log(LOG_ERROR) << "Reserved operation for future use";
	return false;
}


////////////////////////////////////////////////////////////
//					Sort helpers
////////////////////////////////////////////////////////////


/**
 * Add new value to sorted vector by name.
 * @param vec Vector with data.
 * @param name Name of new data.
 * @param value Data to add.
 */
template<typename R>
void addSortHelper(std::vector<R>& vec, R value)
{
	vec.push_back(value);
	std::sort(vec.begin(), vec.end(), [](const R& a, const R& b) { return ScriptRef::compare(a.name, b.name) < 0; });
}

template<bool upper, typename R>
auto boundSortHelper(R* begin, R* end, ScriptRange<ScriptRef> than)
{
	constexpr int limit = upper ? 1 : 0;
	const auto total_size = std::accumulate(than.begin(), than.end(), size_t{}, [](size_t acc, ScriptRef r) { return acc + r.size(); });
	const auto last_empty = std::find_if(than.begin(), than.end(), [](ScriptRef r){ return !r; });

	// some garbage, should not happened, for avoiding unexpected results make check for it
	assert(std::all_of(last_empty, than.end(), [](ScriptRef r){ return !r; }));

	const auto final_range = ScriptRange{ than.begin(), last_empty };

	return std::partition_point(begin, end,
		[&](const R& a)
		{
			const auto curr = a.name.size();
			if (curr < total_size)
			{
				return true;
			}
			else if (curr == total_size)
			{
				ScriptRef head = {};
				ScriptRef tail = a.name;
				auto comp = 0;
				for (ScriptRef r : final_range)
				{
					auto s = r.size();
					head = tail.head(s);
					tail = tail.tail(s);
					comp = ScriptRef::compare(head, r);
					if (comp < 0)
					{
						return true;
					}
					else if (comp > 0)
					{
						return false;
					}
					else // comp == 0
					{
						continue;
					}
				}
				return comp < limit;
			}
			else
			{
				return false;
			}
		}
	);
}

/**
 * Get bound of value, upper or lower based on template parameter.
 * @param begin begin of sorted range.
 * @param end end of sorted range.
 * @param prefix First part of name.
 * @param postfix Second part of name.
 * @return Found iterator or end iterator.
 */
template<bool upper, typename R>
R* boundSortHelper(R* begin, R* end, ScriptRef prefix, ScriptRef postfix = {})
{
	constexpr int limit = upper ? 1 : 0;
	if (postfix)
	{
		const auto size = prefix.size();
		const auto total_size = size + postfix.size();
		return std::partition_point(begin, end,
			[&](const R& a)
			{
				const auto curr = a.name.size();
				if (curr < total_size)
				{
					return true;
				}
				else if (curr == total_size)
				{
					const auto comp  = ScriptRef::compare(a.name.substr(0, size), prefix);
					return comp < 0 || (comp == 0 && ScriptRef::compare(a.name.substr(size), postfix) < limit);
				}
				else
				{
					return false;
				}
			}
		);
	}
	else
	{
		return std::partition_point(begin, end, [&](const R& a){ return ScriptRef::compare(a.name, prefix) < limit; });
	}
}

/**
 * Helper function finding data by name (that can be merge from multiple parts).
 * @param begin begin of sorted range.
 * @param end end of sorted range.
 * @param name Name split to parts.
 * @return Found data or null.
 */
template<typename R, typename... Args>
R* findSortHelper(R* begin, R* end, Args... args)
{
	auto f = boundSortHelper<false>(begin, end, args...);
	if (f != end)
	{
		// check upper bound, if is different than lower, its mean we have hit
		if (f != boundSortHelper<true>(f, f + 1, args...))
		{
			return &*f;
		}
	}
	return nullptr;
}

/**
 * Helper function finding data by name (that can be merge from multiple parts).
 * @param begin begin of sorted range.
 * @param end end of sorted range.
 * @param name Name split to parts.
 * @return Found data or null.
 */
template<typename R>
const R* findSortHelper(const std::vector<R>& vec, ScriptRange<ScriptRef> name)
{
	return findSortHelper(vec.data(), vec.data() + vec.size(), name);
}

/**
 * Helper function finding data by name (that can be merge from multiple parts).
 * @param begin begin of sorted range.
 * @param end end of sorted range.
 * @param name Name split to parts.
 * @return Found data or null.
 */
template<typename R>
R* findSortHelper(std::vector<R>& vec, ScriptRange<ScriptRef> name)
{
	return findSortHelper(vec.data(), vec.data() + vec.size(), name);
}

/**
 * Helper function finding data by name (that can be merge from two parts).
 * @param vec Vector with values.
 * @param prefix First part of name.
 * @param postfix Second part of name.
 * @return Found data or null.
 */
template<typename R>
const R* findSortHelper(const std::vector<R>& vec, ScriptRef prefix, ScriptRef postfix = {})
{
	return findSortHelper(vec.data(), vec.data() + vec.size(), prefix, postfix);
}

/**
 * Helper function finding data by name (that can be merge from two parts).
 * @param vec Vector with values.
 * @param prefix First part of name.
 * @param postfix Second part of name.
 * @return Found data or null.
 */
template<typename R>
R* findSortHelper(std::vector<R>& vec, ScriptRef prefix, ScriptRef postfix = {})
{
	return findSortHelper(vec.data(), vec.data() + vec.size(), prefix, postfix);
}

/**
 * Calculate space used by reg of that type.
 * @param parser
 * @param type
 * @return Type meta data.
 */
TypeInfo getRegMeta(const ScriptParserBase& parser, ArgEnum type)
{
	auto t = parser.getType(type);
	if (t == nullptr)
	{
		return { };
	}
	else
	{
		return ArgIsPtr(type) ? TypeInfo::getPtrTypeInfo() : t->meta;
	}
}

/**
 * Add new string to collection and return reference to it.
 * @param list List where strings are stored.
 * @param s New string to add.
 * @return Reference to stored string.
 */
ScriptRef addString(std::vector<std::vector<char>>& list, const std::string& s)
{
	std::vector<char> refData;
	refData.assign(s.begin(), s.end());
	ScriptRef ref{ refData.data(), refData.data() + refData.size() };

	//we need use char vector because its guaranteed that pointer in ref will not get invalidated when names list grown.
	list.push_back(std::move(refData));
	return ref;
}

} //namespace


////////////////////////////////////////////////////////////
//					ParserWriter class
////////////////////////////////////////////////////////////

/**
 * Constructor
 * @param regUsed current used reg space.
 * @param c container that will be fill with script data.
 * @param d parser having all meta data.
 */
ParserWriter::ParserWriter(
		size_t regUsed,
		ScriptContainerBase& c,
		const ScriptParserBase& d) :
	container(c),
	parser(d),
	regIndexUsed(static_cast<RegEnum>(regUsed))
{
	pushScopeBlock(BlockMain);
}

/**
 * Function finalizing parsing of script
 */
void ParserWriter::relese()
{
	pushProc(Proc_exit);
	refLabels.forEachPosition(
		[&](auto pos, ProgPos value)
		{
			if (value == ProgPos::Unknown)
			{
				throw Exception("Incorrect label position reference");
			}
			updateReserved<ProgPos>(pos, value);
		}
	);

	auto textTotalSize = 0u;
	refTexts.forEachPosition(
		[&](auto pos, ScriptRef value)
		{
			textTotalSize += value.size() + 1;
		}
	);
	auto charPtr = [&](ProgPos pos)
	{
		return (char*)&container._proc[static_cast<size_t>(pos)];
	};
	//preallocate space in vector to have stable pointers to strings
	auto currentText = push(textTotalSize);
	refTexts.forEachPosition(
		[&](auto pos, ScriptRef value)
		{
			auto start = currentText;

			//check beginning of string
			auto begin = value.begin();
			if (begin == value.end() || *begin != '"')
			{
				throw Exception("Invalid Text: >>" + value.toString() + "<<");
			}

			//check end of string
			auto end = value.end() - 1;
			if (begin == end || *end != '"')
			{
				throw Exception("Invalid Text: >>" + value.toString() + "<<");
			}

			++begin;
			bool escape = false;
			while (begin != end)
			{
				if (escape == true)
				{
					escape = false;
				}
				else
				{
					if (*begin == '\\')
					{
						escape = true;
						++begin;
						continue;
					}
				}
				*charPtr(currentText) = *begin;
				++currentText;
				++begin;
			}
			++currentText;

			updateReserved<ScriptText>(pos, ScriptText{ charPtr(start) });
		}
	);
}

/**
 * Returns reference based on name.
 * @param s name of reference.
 * @return reference data.
 */
ScriptRefData ParserWriter::getReferece(const ScriptRef& s) const
{
	const ScriptRefData* ptr = nullptr;
	for (const auto& r : regStack)
	{
		if (r.name == s)
		{
			ptr = &r;
			break;
		}
	}
	if (ptr == nullptr)
	{
		ptr = parser.getRef(s);
	}
	if (ptr == nullptr)
	{
		ptr = parser.getGlobal()->getRef(s);
	}
	if (ptr == nullptr)
	{
		return ScriptRefData{ };
	}
	return *ptr;
}

/**
 * Get current position in proc vector.
 * @return Position in proc vector.
 */
ProgPos ParserWriter::getCurrPos() const
{
	return static_cast<ProgPos>(container._proc.size());
}

/**
 * Get distance between two positions in proc vector.
 */
size_t ParserWriter::getDiffPos(ProgPos begin, ProgPos end) const
{
	if (begin > end)
	{
		throw Exception("Invalid ProgPos distance");
	}
	return static_cast<size_t>(end) - static_cast<size_t>(begin);
}

/**
 * Push zeros to fill empty space.
 * @param s Size of empty space.
 */
ProgPos ParserWriter::push(size_t s)
{
	auto curr = getCurrPos();
	container._proc.insert(container._proc.end(), s, 0);
	return static_cast<ProgPos>(curr);
}

/**
 * Update part of proc vector.
 * @param pos position to update.
 * @param data data to write.
 * @param s size of data.
 */
void ParserWriter::update(ProgPos pos, void* data, size_t s)
{
	memcpy(&container._proc[static_cast<size_t>(pos)], data, s);
}

/**
 * Push custom value on proc vector.
 * @param p pointer to value.
 * @param size size of value.
 */
void ParserWriter::pushValue(ScriptValueData v)
{
	update(push(v.size), &v.data, v.size);
}

/**
 * Pushing proc operation id on proc vector.
 */
ParserWriter::ReservedPos<ParserWriter::ProcOp> ParserWriter::pushProc(Uint8 procId)
{
	auto curr = getCurrPos();
	container._proc.push_back(procId);
	return { curr };
}

/**
 * Updating previously added proc operation id.
 * @param pos Position of operation.
 * @param procOffset Offset value.
 */
void ParserWriter::updateProc(ReservedPos<ProcOp> pos, int procOffset)
{
	container._proc[static_cast<size_t>(pos.getPos())] += procOffset;
}

/**
 * Try pushing label arg on proc vector.
 * @param s name of label.
 * @return true if label was successfully added.
 */
bool ParserWriter::pushLabelTry(const ScriptRefData& data)
{
	auto temp = data;
	if (!temp && temp.name)
	{
//		temp = addReferece(addLabel(data.name));
		return false;
	}
	if (temp.type != ArgLabel)
	{
		return false;
	}

	// Can't use this to create loop back named label.
	if (temp.name && refLabels.getValue(temp.value) != ProgPos::Unknown)
	{
		return false;
	}
	refLabels.pushPosition(*this, temp.value);
	return true;
}

/**
 * Create new label definition for proc vector.
 * @return index of new label.
 */
ScriptRefData ParserWriter::addLabel(const ScriptRef& name)
{
	return ScriptRefData{ name, ArgLabel, refLabels.addValue(ProgPos::Unknown) };
}

/**
 * Setting offset of label on proc vector.
 * @param s name of label
 * @param offset set value where label is pointing
 * @return true if operation success
 */
bool ParserWriter::setLabel(const ScriptRefData& data, ProgPos offset)
{
	auto temp = data;
	if (!temp && temp.name)
	{
//		temp = addReferece(addLabel(data.name));
		return false;
	}
	if (temp.type != ArgLabel)
	{
		return false;
	}

	if (refLabels.getValue(temp.value) != ProgPos::Unknown)
	{
		return false;
	}
	refLabels.setValue(temp.value, offset);
	return true;
}

/**
 * Try pushing text literal arg on proc vector.
 */
bool ParserWriter::pushTextTry(const ScriptRefData& data)
{
	if (data && data.type == ArgText)
	{
		refTexts.pushPosition(*this, refTexts.addValue(data.getValue<ScriptRef>()));
		return true;
	}
	return false;
}

/**
 * Try pushing data arg on proc vector.
 * @param s name of data
 * @return true if data exists and is valid
 */
bool ParserWriter::pushConstTry(const ScriptRefData& data, ArgEnum type)
{
	if (data && data.type == type && !ArgIsReg(data.type) && data.value.type == type)
	{
		pushValue(data.value);
		return true;
	}
	return false;
}

/**
 * Try pushing reg arg on proc vector.
 * @param s name of reg
 * @return true if reg exists and is valid
 */
bool ParserWriter::pushRegTry(const ScriptRefData& data, ArgEnum type)
{
	type = ArgSpecAdd(type, ArgSpecReg);
	if (data && ArgCompatible(type, data.type, 0) && data.getValue<RegEnum>() != RegInvalid)
	{
		pushValue(data.getValue<RegEnum>());
		return true;
	}
	return false;
}

/**
 * Add new reg arg definition.
 * @param s optional name of reg
 * @param type type of reg
 * @return Reg data
 */
ScriptRefData ParserWriter::addReg(const ScriptRef& s, ArgEnum type)
{
	if (s && getReferece(s))
	{
		return {};
	}

	type = ArgSpecAdd(type, ArgSpecReg);
	auto meta = getRegMeta(parser, type);
	if (!meta)
	{
		return {};
	}
	if (meta.needRegSpace(regIndexUsed) > ScriptMaxReg)
	{
		return {};
	}
	ScriptRefData data = { s, type, static_cast<RegEnum>(meta.nextRegPos(regIndexUsed)) };
	if (s)
	{
		regStack.push_back(data);
	}
	regIndexUsed = static_cast<RegEnum>(meta.needRegSpace(regIndexUsed));
	return data;
}

/**
 * Add new local const definition.
 * @param s optional name of const
 * @param type type of reg
 * @return Reg data
 */
ScriptRefData ParserWriter::addConst(const ScriptRef& s, ArgEnum type, ScriptValueData value)
{
	if (!s)
	{
		return {};
	}

	if (getReferece(s))
	{
		return {};
	}

	if (ArgIsReg(type))
	{
		return {};
	}

	auto meta = getRegMeta(parser, type);
	if (!meta)
	{
		return {};
	}

	ScriptRefData data = { s, type, value };

	regStack.push_back(data);

	return data;
}

/**
 * Add new code scope.
 * @return Reference to new block.
 */
ParserWriter::Block& ParserWriter::pushScopeBlock(BlockEnum type)
{
	return codeBlocks.emplace_back(ParserWriter::Block{ type, regStack.size(), regIndexUsed });
}

/**
 * Clear values in code scope.
 * @return Reference to current block.
 */
ParserWriter::Block& ParserWriter::clearScopeBlock()
{
	auto& current = codeBlocks.back();
	regStack.resize(current.regStackSizeFrom);
	regIndexUsed = current.regIndexUsedFrom;
	return current;
}

/**
 * Pop code scope.
 * @return Copy of old block.
 */
ParserWriter::Block ParserWriter::popScopeBlock()
{
	if (codeBlocks.back().type == BlockMain)
	{
		throw Exception("Invalid stack popup");
	}

	auto prev = clearScopeBlock();
	codeBlocks.pop_back();
	return prev;
}

/// Dump to log error info about ref.
void ParserWriter::logDump(const ScriptRefData& ref) const
{
	if (ref)
	{
		Log(LOG_ERROR) << "Incorrect type of argument '"<< ref.name.toString() <<"' of type "<< displayType(&parser, ref.type);
	}
	else
	{
		Log(LOG_ERROR) << "Unknown argument '"<< ref.name.toString() <<"'";
	}
}

////////////////////////////////////////////////////////////
//				ScriptParserBase class
////////////////////////////////////////////////////////////

/**
 * Constructor.
 */
ScriptParserBase::ScriptParserBase(ScriptGlobal* shared, const std::string& name) :
	_shared{ shared },
	_emptyReturn{ false },
	_regUsedSpace{ RegStartPos },
	_regOutSize{ 0 }, _regOutName{ },
	_name{ name }
{
	//--------------------------------------------------
	//					op_data init
	//--------------------------------------------------
	#define MACRO_ALL_INIT(NAME, IMPL, ARGS, DESC) \
		addParserBase(#NAME, DESC, nullptr, helper::FuncGroup<MACRO_FUNC_ID(NAME)>::overloadType(), &parseBuildinProc<MACRO_PROC_ID(NAME), helper::FuncGroup<MACRO_FUNC_ID(NAME)>>, nullptr, nullptr);

	MACRO_PROC_DEFINITION(MACRO_ALL_INIT)

	#undef MACRO_ALL_INIT

	auto buildin = [&](const std::string& s, ScriptProcData::parserFunc func)
	{
		addParserBase(s, {}, &overloadBuildinProc, {}, func, nullptr, nullptr);
	};

	buildin("if", &parseIf);
	buildin("else", &parseElse);
	buildin("end", &parseEnd);
	buildin("var", &parseVar);
	buildin("const", &parseConst);
	buildin("debug_log", &parseDebugLog);
	buildin("debug_assert", &parseDummy);
	buildin("loop", &parseLoop);
	buildin("break", &parseBreak);
	buildin("continue", &parseContinue);
	buildin("return", &parseReturn);
	buildin("begin", &parseBegin);

	addParser<helper::FuncGroup<Func_test_eq_null>>("test_eq", "");
	addParser<helper::FuncGroup<Func_debug_impl_int>>("debug_impl", "");
	addParser<helper::FuncGroup<Func_debug_impl_text>>("debug_impl", "");
	addParser<helper::FuncGroup<Func_debug_flush>>("debug_flush", "");

	addParser<helper::FuncGroup<Func_set_text>>("set", "");
	addParser<helper::FuncGroup<Func_clear_text>>("clear", "");
	addParser<helper::FuncGroup<Func_test_eq_text>>("test_eq", "");

	addType<ScriptInt>("int");
	addType<ScriptText>("text");
	addType<ScriptArgSeparator>("__");

	auto labelName = addNameRef("label");
	auto nullName = addNameRef("null");
	auto phName = addNameRef("_");
	auto seperatorName = addNameRef("__");
	auto varName = addNameRef("var");
	auto constName = addNameRef("const");

	addSortHelper(_typeList, { labelName, ArgLabel, { } });
	addSortHelper(_typeList, { nullName, ArgNull, { } });
	addSortHelper(_refList, { nullName, ArgNull });
	addSortHelper(_refList, { phName, ArgPlaceholder });
	addSortHelper(_refList, { seperatorName, ArgSep });
	addSortHelper(_refList, { varName, ArgInvalid });
	addSortHelper(_refList, { constName, ArgInvalid });

	_shared->initParserGlobals(this);
}

/**
 * Destructor
 */
ScriptParserBase::~ScriptParserBase()
{

}

/**
 * Test if that name is already in use.
 */
bool ScriptParserBase::haveNameRef(const std::string& s) const
{
	auto ref = ScriptRef{ s.data(), s.data() + s.size() };
	if (findSortHelper(_refList, ref) != nullptr)
	{
		return true;
	}
	if (findSortHelper(_procList, ref) != nullptr)
	{
		return true;
	}
	if (findSortHelper(_typeList, ref) != nullptr)
	{
		return true;
	}
	for (auto& r : ConditionNames)
	{
		if (ref == r)
		{
			return true;
		}
	}
	for (auto& r : ConditionSpecNames)
	{
		if (ref == r)
		{
			return true;
		}
	}
	return false;
}

/**
 * Store new name reference for future use.
 */
ScriptRef ScriptParserBase::addNameRef(const std::string& s)
{
	return addString(_strings, s);
}

/**
 * Add new function parsing arguments of script operation.
 * @param s function name
 * @param parser parsing function
 */
void ScriptParserBase::addParserBase(const std::string& s, const std::string& description, ScriptProcData::overloadFunc overload, ScriptRange<ScriptRange<ArgEnum>> overloadArg, ScriptProcData::parserFunc parser, ScriptProcData::argFunc arg, ScriptProcData::getFunc get)
{
	if (haveNameRef(s))
	{
		auto procs = getProc(ScriptRef{ s.data(), s.data() + s.size() });
		if (!procs)
		{
			throw Exception("Function name '" + s + "' already used");
		}
	}
	if (!parser)
	{
		parser = &parseCustomProc;
	}
	if (!overload)
	{
		overload = validOverloadProc(overloadArg) ? &overloadCustomProc : &overloadInvalidProc;
	}
	addSortHelper(_procList, { addNameRef(s), addNameRef(description), overload, overloadArg, parser, arg, get });
}

/**
 * Add new type to parser.
 * @param s Type name.
 * @param type
 * @param size
 */
void ScriptParserBase::addTypeBase(const std::string& s, ArgEnum type, TypeInfo meta)
{
	if (haveNameRef(s))
	{
		throw Exception("Type name '" + s + "' already used");
	}

	addSortHelper(_typeList, { addNameRef(s), ArgBase(type), meta });
}

/**
 * Test if type is already used.
 */
bool ScriptParserBase::haveTypeBase(ArgEnum type)
{
	type = ArgBase(type);
	for (auto& v : _typeList)
	{
		if (v.type == type)
		{
			return true;
		}
	}
	return false;
}

/**
 * Set name for custom script parameter.
 * @param s name for custom parameter.
 * @param type type of custom parameter.
 * @param outputReg is this reg used for script output.
 */
void ScriptParserBase::addScriptReg(const std::string& s, ArgEnum type, bool writableReg, bool outputReg)
{
	if (writableReg || outputReg)
	{
		if (outputReg && _regOutSize >= ScriptMaxOut)
		{
			throw Exception("Custom output reg limit reach for: '" + s + "'");
		}
		type = ArgSpecAdd(type, ArgSpecVar);
	}
	else
	{
		type = ArgSpecAdd(ArgSpecRemove(type, ArgSpecVar), ArgSpecReg);
	}
	auto t = getType(type);
	if (t == nullptr)
	{
		throw Exception("Invalid type for reg: '" + s + "'");
	}
	auto meta = getRegMeta(*this, type);
	if (!meta)
	{
		throw Exception("Invalid use of type '" + t->name.toString() + "' for reg: '" + s + "'");
	}
	if (meta.needRegSpace(_regUsedSpace) <= ScriptMaxReg)
	{
		if (haveNameRef(s))
		{
			throw Exception("Reg name '" + s + "' already used");
		}

		auto name = addNameRef(s);
		if (outputReg)
		{
			_regOutName[_regOutSize++] = name;
		}
		auto old = meta.nextRegPos(_regUsedSpace);
		_regUsedSpace = meta.needRegSpace(_regUsedSpace);
		addSortHelper(_refList, { name, type, static_cast<RegEnum>(old) });
	}
	else
	{
		throw Exception("Custom reg limit reach for: '" + s + "'");
	}
}

/**
 * Add const value to script.
 * @param s name for const.
 * @param i value.
 */
void ScriptParserBase::addConst(const std::string& s, ScriptValueData i)
{
	if (haveNameRef(s))
	{
		throw Exception("Const name '" + s + "' already used");
	}

	addSortHelper(_refList, { addNameRef(s), i.type, i });
}

/**
 * Update const value in script.
 * @param s name for const.
 * @param i new value.
 */
void ScriptParserBase::updateConst(const std::string& s, ScriptValueData i)
{
	ScriptRefData* f = findSortHelper(_refList, ScriptRef{ s.data(), s.data() + s.size() });
	if (!f)
	{
		throw Exception("Unknown const with name '" + s + "' to update");
	}
	if (f->type != i.type)
	{
		throw Exception("Incompatible const with name '" + s + "' to update");
	}
	f->value = i;
}
/**
 * Get name of type
 * @param type Type id.
 */
ScriptRef ScriptParserBase::getTypeName(ArgEnum type) const
{
	auto p = getType(type);
	if (p)
	{
		return p->name;
	}
	else
	{
		return ScriptRef{ };
	}
}

/**
 * Get full name of type.
 * @param type
 * @return Full name with e.g. `var` or `ptr`.
 */
std::string ScriptParserBase::getTypePrefix(ArgEnum type) const
{
	std::string prefix;
	if (ArgIsVar(type))
	{
		prefix = "var ";
	}
	if (ArgIsPtr(type))
	{
		if (ArgIsPtrE(type))
		{
			prefix += "ptre ";
		}
		else
		{
			prefix += "ptr ";
		}
	}
	return prefix;
}

/**
 * Get type data.
 * @param type Type id.
 */
const ScriptTypeData* ScriptParserBase::getType(ArgEnum type) const
{
	ArgEnum base = ArgBase(type);
	for (auto& t : _typeList)
	{
		if (t.type == base)
		{
			return &t;
		}
	}
	return nullptr;
}

/**
 * Get type data with name equal prefix + postfix.
 * @param name Name split in parts.
 * @return Pointer to data or null if not find.
 */
const ScriptTypeData* ScriptParserBase::getType(ScriptRange<ScriptRef> name) const
{
	return findSortHelper(_typeList, name);
}

/**
 * Get function data with name equal prefix + postfix.
 * @param name Name split in parts.
 * @return Pointer to data or null if not find.
 */
ScriptRange<ScriptProcData> ScriptParserBase::getProc(ScriptRange<ScriptRef> name) const
{
	auto lower = _procList.data();
	auto upper = _procList.data() + _procList.size();
	lower = boundSortHelper<false>(lower, upper, name);
	upper = boundSortHelper<true>(lower, upper, name);

	return { lower, upper };
}

/**
 * Get arguments data with name equal prefix + postfix.
 * @param name Name split in parts.
 * @return Pointer to data or null if not find.
 */
const ScriptRefData* ScriptParserBase::getRef(ScriptRange<ScriptRef> name) const
{
	return findSortHelper(_refList, name);
}

/**
 * Parse string and write script to ScriptBase
 * @param src struct where final script is write to
 * @param src_code string with script
 * @return true if string have valid script
 */
bool ScriptParserBase::parseBase(ScriptContainerBase& destScript, const std::string& parentName, const std::string& srcCode) const
{
	ScriptContainerBase tempScript;
	std::string err = "Error in parsing script '" + _name + "' for '" + parentName + "': ";
	ParserWriter help(
		_regUsedSpace,
		tempScript,
		*this
	);

	bool haveLastReturn = false;
	bool haveCodeNormal = false;
	ScriptRefTokens range = ScriptRefTokens{ srcCode.data(), srcCode.data() + srcCode.size() };
	if (!range)
	{
		return false;
	}

	while (true)
	{
		SelectedToken op = range.getNextToken();
		if (!op)
		{
//			for (auto i = help.refListCurr.begin(); i != help.refListCurr.end(); ++i)
//			{
//				if (i->type == ArgLabel && help.refLabels.getValue(i->value) == ProgPos::Unknown)
//				{
//					Log(LOG_ERROR) << err << "invalid use of label: '" << i->name.toString() << "' without declaration";
//					return false;
//				}
//			}
			if (help.codeBlocks.size() > 1)
			{
				Log(LOG_ERROR) << err << "script have missed 'end;'";
				return false;
			}
			if (!haveLastReturn)
			{
				Log(LOG_ERROR) << err << "script need to end with return statement";
				return false;
			}
			help.relese();
			destScript = std::move(tempScript);
			return true;
		}

		auto line_begin = op.begin();
		SelectedToken label = { };
		SelectedToken args[ScriptMaxArg];
		args[0] = range.getNextToken(TokenColon);
		if (args[0].getType() == TokenColon)
		{
			std::swap(op, label);
			op = range.getNextToken();
			args[0] = range.getNextToken();
		}

		ScriptRefOperation op_curr = findOperationAndArg(help, op);
		if (!op_curr)
		{
			logErrorOnOperationArg(op_curr);
			Log(LOG_ERROR) << "Invalid operation '" << op.toString() << "'";
		}

		// change form of `Reg.Function` to `Type.Function Reg`.
		if (op_curr.haveArg())
		{
			// we already loaded op_curr = "Reg.Function", args[0] = "X"
			// then switch it to op_curr = "Type.Function", args[0] = "Reg", args[1] = "X"
			args[1] = args[0];
			args[0] = { TokenSymbol, op_curr.argName, op.getLinePos() };
		}

		for (size_t i = (op_curr.haveArg() ? 2 : 1); i < ScriptMaxArg; ++i)
			args[i] = range.getNextToken();
		SelectedToken f = range.getNextToken(TokenSemicolon);

		//validation
		bool valid = true;
		valid &= label.getType() == TokenSymbol || label.getType() == TokenNone;
		valid &= op.getType() == TokenSymbol;
		for (size_t i = 0; i < ScriptMaxArg; ++i)
			valid &= args[i].getType() != TokenInvalid;
		valid &= f.getType() == TokenSemicolon;

		if (!valid)
		{
			auto line_end = range.begin();
			if (f.getType() != TokenSemicolon)
			{
				// fixing `line_end` position.
				while(line_end != range.end() && *line_end != ';')
					++line_end;
				if (line_end != range.end())
					++line_end;
			}

			for (size_t i = 0; i < ScriptMaxArg; ++i)
			{
				if (args[i].getType() == TokenInvalid)
				{
					Log(LOG_ERROR) << err << "invalid argument '"<<  args[i].toString() <<"' in line: '" << std::string(line_begin, line_end) << "' (at " + std::to_string(op.getLinePos()) + ")";
					return false;
				}
			}

			Log(LOG_ERROR) << err << "invalid line: '" << std::string(line_begin, line_end) << "' (at " + std::to_string(op.getLinePos()) + ")";
			return false;
		}

		ScriptRef line = ScriptRef{ line_begin, range.begin() };

		// test validity of operation positions
		auto isReturn = (op == ScriptRef{ "return" });
		auto isVarDef = (op == ScriptRef{ "var" }) || (op == ScriptRef{ "const" });
		auto isBegin = (op == ScriptRef{ "if" }) || (op == ScriptRef{ "else" }) || (op == ScriptRef{ "begin" }) || (op == ScriptRef{ "loop" });
		auto isEnd = (op == ScriptRef{ "end" }) || (op == ScriptRef{ "else" }); // `else;` is begin and end of scope
		auto isBreak = (op == ScriptRef{ "continue" } || op == ScriptRef{ "break" });

		if (haveLastReturn && !isEnd)
		{
			Log(LOG_ERROR) << err << "unreachable code after return in line: '" << line.toString() << "' (at " + std::to_string(op.getLinePos()) + ")";
			return false;
		}
		if (haveCodeNormal && isVarDef)
		{
			Log(LOG_ERROR) << err << "invalid variable definition after other operations in line: '" << line.toString() << "' (at " + std::to_string(op.getLinePos()) + ")";
			return false;
		}
		if (label && isVarDef)
		{
			Log(LOG_ERROR) << err << "label can't be before variable definition in line: '" << line.toString() << "' (at " + std::to_string(op.getLinePos()) + ")";
			return false;
		}

		haveLastReturn = isReturn || isBreak;
		haveCodeNormal = !(isVarDef || isBegin); // we can have `var` only on begining of new scope


		// matching args from operation definition with args available in string
		ScriptArgList argData = { };
		for (const SelectedToken& t : args)
		{
			if (t.getType() == TokenNone)
			{
				break;
			}

			if (!argData.tryPushBack(t.parse(help)))
			{
				Log(LOG_ERROR) << err << "too many arguments in line: '" << line.toString() << "' (at " + std::to_string(op.getLinePos()) + ")";
				return false;
			}
		}

		if (label && !help.setLabel(label.parse(help), help.getCurrPos()))
		{
			Log(LOG_ERROR) << err << "invalid label '"<< label.toString() <<"' in line: '" << line.toString() << "' (at " + std::to_string(op.getLinePos()) + ")";
			return false;
		}

		// create normal proc call
		if (parseOverloadProc(help, op_curr.procList, std::begin(argData), std::end(argData)) == false)
		{
			Log(LOG_ERROR) << err << "invalid operation in line: '" << line.toString() << "' (at " + std::to_string(op.getLinePos()) + ")";
			return false;
		}
	}
}

/**
 * Parse node and return new script.
 */
void ScriptParserBase::parseNode(ScriptContainerBase& container, const std::string& parentName, const YAML::Node& node) const
{
	if(const YAML::Node& scripts = node["scripts"])
	{
		if (const YAML::Node& curr = scripts[getName()])
		{
			if (false == parseBase(container, parentName, curr.as<std::string>()))
			{
				Log(LOG_ERROR) << "    for node with code at line " << node.Mark().line << " in " << getGlobal()->getCurrentFile();
				Log(LOG_ERROR) << ""; // dummy line to separate similar errors
			}
		}
	}
	if (!container && !getDefault().empty())
	{
		if (false == parseBase(container, parentName, getDefault()))
		{
			Log(LOG_ERROR) << ""; // dummy line to separate similar errors
		}
	}
}

/**
 * Parse string and return new script.
 */
void ScriptParserBase::parseCode(ScriptContainerBase& container, const std::string& parentName, const std::string& srcCode) const
{
	if (!srcCode.empty())
	{
		if (false == parseBase(container, parentName, srcCode))
		{
			Log(LOG_ERROR) << "    for code in " << getGlobal()->getCurrentFile();
			Log(LOG_ERROR) << ""; // dummy line to separate similar errors
		}
	}
	if (!container && !getDefault().empty())
	{
		if (false == parseBase(container, parentName, getDefault()))
		{
			Log(LOG_ERROR) << ""; // dummy line to separate similar errors
		}
	}
}

/**
 * Load global data from YAML.
 */
void ScriptParserBase::load(const YAML::Node& node)
{

}

/**
 * Print all metadata
 */
void ScriptParserBase::logScriptMetadata(bool haveEvents, const std::string& groupName) const
{
	if (Options::debug && Options::verboseLogging)
	{
		auto argType = [&](ArgEnum type) -> std::string
		{
			return getTypeName(type).toString();
		};

		const int tabSize = 8;
		static bool printOp = true;
		if (printOp)
		{
			size_t offset = 0;
			printOp = false;
			Logger opLog;
			#define MACRO_STRCAT(...) #__VA_ARGS__
			#define MACRO_ALL_LOG(NAME, Impl, Args, Desc, ...) \
				if (validOverloadProc(helper::FuncGroup<MACRO_FUNC_ID(NAME)>::overloadType()) && strlen(Desc) != 0) opLog.get(LOG_DEBUG) \
					<< "Op:   " << std::setw(tabSize*2) << #NAME \
					<< "OpId: " << std::setw(tabSize/2) << offset << " .. " <<  std::setw(tabSize) << (offset + helper::FuncGroup<MACRO_FUNC_ID(NAME)>::ver() - 1) \
					<< "Args: " << std::setw(tabSize*5) << displayOverloadProc(this, helper::FuncGroup<MACRO_FUNC_ID(NAME)>::overloadType()) \
					<< "Desc: " << Desc \
					<< "\n"; \
				offset += helper::FuncGroup<MACRO_FUNC_ID(NAME)>::ver();

			opLog.get(LOG_DEBUG) << "Available built-in script operations:\n" << std::left << std::hex << std::showbase;
			MACRO_PROC_DEFINITION(MACRO_ALL_LOG)

			#undef MACRO_ALL_LOG
			#undef MACRO_STRCAT

			opLog.get(LOG_DEBUG) << "Total size: " << offset << "\n";
		}

		Logger refLog;
		refLog.get(LOG_DEBUG) << "Script info for:  '" << _name << "'  in group:  '" << groupName << "'\n" << std::left;
		refLog.get(LOG_DEBUG) << "\n";
		if (haveEvents)
		{
			refLog.get(LOG_DEBUG) << "Have global events\n";
			refLog.get(LOG_DEBUG) << "\n";
		}
		if (!_description.empty())
		{
			refLog.get(LOG_DEBUG) << "Description:\n";
			refLog.get(LOG_DEBUG) << _description << "\n";
			refLog.get(LOG_DEBUG) << "\n";
		}
		if (!_defaultScript.empty())
		{
			refLog.get(LOG_DEBUG) << "Script default implementation:\n";
			refLog.get(LOG_DEBUG) << _defaultScript << "\n";
			refLog.get(LOG_DEBUG) << "\n";
		}
		if (_regOutSize > 0)
		{
			refLog.get(LOG_DEBUG) << "Script return values:\n";
			for (size_t i = 0; i < _regOutSize; ++i)
			{
				auto ref = getRef(_regOutName[i]);
				if (ref)
				{
					refLog.get(LOG_DEBUG) << "Name: " << std::setw(40) << ref->name.toString() << std::setw(9) << getTypePrefix(ref->type) << " " << std::setw(9) << argType(ref->type) << "\n";
				}
			}
			if (_emptyReturn)
			{
				refLog.get(LOG_DEBUG) << "In this script 'return' statement is empty, script returning values are edited directly\n";
			}
			refLog.get(LOG_DEBUG) << "\n";
		}
		refLog.get(LOG_DEBUG) << "Script data:\n";
		auto temp = _refList;
		std::sort(temp.begin(), temp.end(),
			[](const ScriptRefData& a, const ScriptRefData& b)
			{
				return std::lexicographical_compare(a.name.begin(), a.name.end(), b.name.begin(), b.name.end());
			}
		);
		for (auto& r : temp)
		{
			if ((!ArgIsReg(r.type) && !ArgIsPtr(r.type) && Logger::reportingLevel() != LOG_VERBOSE) || ArgBase(r.type) == ArgInvalid)
			{
				continue;
			}
			if (ArgBase(r.type) == ArgInt && !ArgIsReg(r.type))
			{
				refLog.get(LOG_DEBUG) << "Name: " << std::setw(40) << r.name.toString() << std::setw(9) << getTypePrefix(r.type) << " " << std::setw(9) << argType(r.type) << " " << r.value.getValue<int>() << "\n";
			}
			else
			{
				refLog.get(LOG_DEBUG) << "Name: " << std::setw(40) << r.name.toString() << std::setw(9) << getTypePrefix(r.type) << " " << std::setw(9) << argType(r.type) << "\n";
			}
		}
		if (Logger::reportingLevel() != LOG_VERBOSE)
		{
			refLog.get(LOG_DEBUG) << "To see const values and custom operations use 'verboseLogging'\n";
		}
		else
		{
			auto tmp = _procList;
			std::sort(tmp.begin(), tmp.end(),
				[](const ScriptProcData& a, const ScriptProcData& b)
				{
					return std::lexicographical_compare(a.name.begin(), a.name.end(), b.name.begin(), b.name.end());
				}
			);

			refLog.get(LOG_DEBUG) << "\n";
			refLog.get(LOG_DEBUG) << "Script operations:\n";
			for (const auto& p : tmp)
			{
				if (p.parserArg != nullptr && p.overloadArg && p.description != ScriptRef{ BindBase::functionInvisible })
				{
					const auto tabStop = 4; // alignment of next part
					const auto minSpace = 2; // min space to next part

					auto name = p.name.toString();
					auto nameTab = std::max(
						(((int)name.size() + minSpace + tabStop - 1) & -tabStop),
						40
					);

					auto args = displayOverloadProc(this, p.overloadArg);
					auto argsTab = std::max(
						(((int)args.size() + minSpace + tabStop - 1) & -tabStop),
						48
					);

					refLog.get(LOG_DEBUG) << "Name: " << std::setw(nameTab) << name << "Args: " << std::setw(argsTab) << args << (p.description != ScriptRef{ BindBase::functionWithoutDescription } ? std::string("Desc: ") + p.description.toString() + "\n" : "\n");
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////
//				ScriptParserEventsBase class
////////////////////////////////////////////////////////////

/**
 * Constructor.
 */
ScriptParserEventsBase::ScriptParserEventsBase(ScriptGlobal* shared, const std::string& name) : ScriptParserBase(shared, name)
{
	_events.reserve(EventsMax);
	_eventsData.push_back({ 0, {}, {} });
}

/**
 * Parse node and return new script.
 */
void ScriptParserEventsBase::parseNode(ScriptContainerEventsBase& container, const std::string& type, const YAML::Node& node) const
{
	ScriptParserBase::parseNode(container._current, type, node);
	container._events = getEvents();
}

/**
 * Parse string and return new script.
 */
void ScriptParserEventsBase::parseCode(ScriptContainerEventsBase& container, const std::string& type, const std::string& srcCode) const
{
	ScriptParserBase::parseCode(container._current, type, srcCode);
	container._events = getEvents();
}

/**
 * Load global data from YAML.
 */
void ScriptParserEventsBase::load(const YAML::Node& scripts)
{
	ScriptParserBase::load(scripts);

	// helper functions to get position in data vector
	auto findPos = [&](const std::string& n)
	{
		return std::find_if(std::begin(_eventsData), std::end(_eventsData), [&](const EventData& p) { return p.name == n; });
	};
	auto havePos = [&](std::vector<EventData>::iterator it)
	{
		return it != std::end(_eventsData);
	};
	auto removePos = [&](std::vector<EventData>::iterator it)
	{
		_eventsData.erase(it);
	};

	auto getNode = [&](const YAML::Node& i, const std::string& nodeName)
	{
		const auto& n = i[nodeName];
		return std::make_tuple(nodeName, n, !!n);
	};
	auto haveNode = [&](const std::tuple<std::string, YAML::Node, bool>& nn)
	{
		return std::get<bool>(nn);
	};
	auto getLineFromNode = [&](const YAML::Node& n)
	{
		return std::to_string(n.Mark().line);
	};
	auto getDescriptionNode = [&](const std::tuple<std::string, YAML::Node, bool>& nn)
	{
		return std::string("'") + std::get<std::string>(nn) + "' at line " + getLineFromNode(std::get<YAML::Node>(nn));
	};
	auto getNameFromNode = [&](const std::tuple<std::string, YAML::Node, bool>& nn)
	{
		auto name = std::get<YAML::Node>(nn).as<std::string>();
		if (name.empty())
		{
			throw Exception("Invalid name for " + getDescriptionNode(nn));
		}
		return name;
	};

	if (const YAML::Node& curr = scripts[getName()])
	{
		for (const YAML::Node& i : curr)
		{
			const auto deleteNode = getNode(i, "delete");
			const auto newNode = getNode(i, "new");
			const auto overrideNode = getNode(i, "override");
			const auto updateNode = getNode(i, "update");
			const auto ignoreNode = getNode(i, "ignore");

			// default name for case when use don't define node with name
			auto name = std::string{ };

			{
				// check for duplicates
				const std::tuple<std::string, YAML::Node, bool>* last = nullptr;
				for (auto* p : { &deleteNode, &newNode, &updateNode, &overrideNode, &ignoreNode })
				{
					if (haveNode(*p))
					{
						if (last)
						{
							throw Exception("Conflict of " + getDescriptionNode(*last) + " and " + getDescriptionNode(*p));
						}
						else
						{
							last = p;
							name = getNameFromNode(*p);
						}
					}
				}
			}

			if (haveNode(deleteNode))
			{
				auto it = findPos(name);
				if (havePos(it))
				{
					removePos(it);
				}
				else
				{
					Log(LOG_WARNING) << "Unknown script name '" + name  + "' for " + getDescriptionNode(deleteNode);
					Log(LOG_WARNING) << "    in " << getGlobal()->getCurrentFile();
					Log(LOG_WARNING) << ""; // dummy line to separate similar errors
				}
			}
			else
			{
				int offset = 0;
				ScriptContainerBase scp;


				offset = i["offset"].as<double>(0) * OffsetScale;
				if (offset == 0 || offset >= (int)OffsetMax || offset <= -(int)OffsetMax)
				{
					//TODO: make it a exception
					Log(LOG_ERROR) << "Invalid offset for '" << getName() << "' equal: '" << i["offset"].as<std::string>() << "'";
					Log(LOG_ERROR) << "    for node at line " << getLineFromNode(i["offset"]) << " in " << getGlobal()->getCurrentFile();
					Log(LOG_ERROR) << ""; // dummy line to separate similar errors
					continue;
				}

				{
					auto nameWithPrefix = name.size() ? "Global:" + name : "Global off: " + i["offset"].as<std::string>();
					if (false == parseBase(scp, nameWithPrefix, i["code"].as<std::string>("")))
					{
						Log(LOG_ERROR) << "    for node with code at line " << getLineFromNode(i["code"]) << " in " << getGlobal()->getCurrentFile();
						Log(LOG_ERROR) << ""; // dummy line to separate similar errors
						continue;
					}
				}


				if (haveNode(updateNode))
				{
					auto it = findPos(name);
					if (havePos(it))
					{
						it->offset = offset;
						it->script = std::move(scp);
					}
					else
					{
						Log(LOG_WARNING) << "Unknown script name '" + name + "' for " + getDescriptionNode(updateNode);
						Log(LOG_WARNING) << "    in " << getGlobal()->getCurrentFile();
						Log(LOG_WARNING) << ""; // dummy line to separate similar errors
					}
				}
				else if (haveNode(overrideNode))
				{
					auto it = findPos(name);
					if (havePos(it))
					{
						it->offset = offset;
						it->script = std::move(scp);
					}
					else
					{
						throw Exception("Unknown script name '" + name + "' for " + getDescriptionNode(overrideNode));
					}
				}
				else if (haveNode(ignoreNode))
				{
					// nothing to see there...
				}
				else
				{
					if (haveNode(newNode))
					{
						auto it = findPos(name);
						if (havePos(it))
						{
							throw Exception("Script script name '" + name + "' already used for " + getDescriptionNode(newNode));
						}
					}
					EventData data = EventData{};
					data.name = name;
					data.offset = offset;
					data.script = std::move(scp);
					_eventsData.push_back(std::move(data));
				}
			}
		}
	}
}

/**
 * Get pointer to events.
 */
const ScriptContainerBase* ScriptParserEventsBase::getEvents() const
{
	return _events.data();
}

/**
 * Release event data.
 */
std::vector<ScriptContainerBase> ScriptParserEventsBase::releseEvents()
{
	std::sort(std::begin(_eventsData), std::end(_eventsData), [](const EventData& a, const EventData& b) { return a.offset < b.offset; });
	for (auto& e : _eventsData)
	{
		const auto reservedSpaceForZero = e.offset < 0;
		if (_events.size() + (reservedSpaceForZero ?  2 : 1) < EventsMax)
		{
			_events.push_back(std::move(e.script));
		}
		else
		{
			Log(LOG_ERROR) << "Error in script parser '" << getName() << "': global script limit reach";
			if (reservedSpaceForZero)
			{
				_events.emplace_back();
			}
			break;
		}
	}
	_events.emplace_back();
	return std::move(_events);
}

////////////////////////////////////////////////////////////
//					ScriptValuesBase class
////////////////////////////////////////////////////////////

/**
 * Set value.
 */
void ScriptValuesBase::setBase(size_t t, int i)
{
	if (t)
	{
		if (t > values.size())
		{
			values.resize(t);
		}
		values[t - 1u] = i;
	}
}

/**
 * Get value.
 */
int ScriptValuesBase::getBase(size_t t) const
{
	if (t && t <= values.size())
	{
		return values[t - 1u];
	}
	return 0;
}

/**
 * Load values from yaml file.
 */
void ScriptValuesBase::loadBase(const YAML::Node &node, const ScriptGlobal* shared, ArgEnum type, const std::string& nodeName)
{
	if (const YAML::Node& tags = node[nodeName])
	{
		if (tags.IsMap())
		{
			for (const std::pair<YAML::Node, YAML::Node>& pair : tags)
			{
				size_t i = shared->getTag(type, ScriptRef::tempFrom("Tag." + pair.first.as<std::string>()));
				if (i)
				{
					auto temp = 0;
					auto data = shared->getTagValueData(type, i);
					shared->getTagValueTypeData(data.valueType).load(shared, temp, pair.second);
					setBase(i, temp);
				}
				else
				{
					Log(LOG_ERROR) << "Error in tags: '" << pair.first << "' unknown tag name not defined in current file";
				}
			}
		}
	}
}

/**
 * Save values to yaml file.
 */
void ScriptValuesBase::saveBase(YAML::Node &node, const ScriptGlobal* shared, ArgEnum type, const std::string& nodeName) const
{
	bool haveData = false;
	YAML::Node tags;
	for (size_t i = 1; i <= values.size(); ++i)
	{
		if (int v = getBase(i))
		{
			haveData = true;
			auto temp = YAML::Node{};
			auto data = shared->getTagValueData(type, i);
			shared->getTagValueTypeData(data.valueType).save(shared, v, temp);
			tags[data.name.substr(data.name.find('.') + 1u).toString()] = temp;
		}
	}
	if (haveData)
	{
		node[nodeName] = tags;
	}
}

////////////////////////////////////////////////////////////
//					ScriptGlobal class
////////////////////////////////////////////////////////////

/**
 * Default constructor.
 */
ScriptGlobal::ScriptGlobal()
{
	addTagValueTypeBase(
		"int",
		[](const ScriptGlobal* s, int& value, const YAML::Node& node)
		{
			if (node)
			{
				value = node.as<int>();
			}
		},
		[](const ScriptGlobal* s, const int& value, YAML::Node& node)
		{
			node = value;
		}
	);
}

/**
 * Destructor.
 */
ScriptGlobal::~ScriptGlobal()
{

}

/**
 * Get tag value.
 */
size_t ScriptGlobal::getTag(ArgEnum type, ScriptRef s) const
{
	auto data = _tagNames.find(type);
	if (data != _tagNames.end())
	{
		for (size_t i = 0; i < data->second.values.size(); ++i)
		{
			auto &name = data->second.values[i].name;
			if (name == s)
			{
				return i + 1;
			}
		}
	}
	return 0;
}

/**
 * Get name of tag value.
 */
ScriptGlobal::TagValueData ScriptGlobal::getTagValueData(ArgEnum type, size_t i) const
{
	auto data = _tagNames.find(type);
	if (data != _tagNames.end())
	{
		if (i && i <= data->second.values.size())
		{
			return data->second.values[i - 1];
		}
	}
	return {};
}

/**
 * Get tag value type data.
 */
ScriptGlobal::TagValueType ScriptGlobal::getTagValueTypeData(size_t valueType) const
{
	if (valueType < _tagValueTypes.size())
	{
		return _tagValueTypes[valueType];
	}
	return {};
}

/**
 * Get tag value type id.
 */
size_t ScriptGlobal::getTagValueTypeId(ScriptRef s) const
{
	for (size_t i = 0; i < _tagValueTypes.size(); ++i)
	{
		if (_tagValueTypes[i].name == s)
		{
			return i;
		}
	}
	return (size_t)-1;
}

/**
 * Add new tag name.
 */
size_t ScriptGlobal::addTag(ArgEnum type, ScriptRef s, size_t valueType)
{
	auto data = _tagNames.find(type);
	if (data == _tagNames.end())
	{
		throw Exception("Unknown tag type");
	}
	auto tag = getTag(type, s);
	if (tag == 0)
	{
		// is tag used for other tag type?
		if (findSortHelper(_refList, s))
		{
			return 0;
		}
		// is tag type valid?
		if (valueType >= _tagValueTypes.size())
		{
			return 0;
		}
		// test to prevent warp of index value
		if (data->second.values.size() < data->second.limit)
		{
			data->second.values.push_back(TagValueData{ s, valueType });
			addSortHelper(_refList, { s, type, data->second.crate(data->second.values.size()) });
			return data->second.values.size();
		}
		return 0;
	}
	else
	{
		return tag;
	}
}

/**
 * Store new name reference for future use.
 */
ScriptRef ScriptGlobal::addNameRef(const std::string& s)
{
	return addString(_strings, s);
}

/**
 * Store parser.
 */
void ScriptGlobal::pushParser(const std::string& groupName, ScriptParserBase* parser)
{
	parser->logScriptMetadata(false, groupName);
	_parserNames.insert(std::make_pair(parser->getName(), parser));
}

/**
 * Store parser with events.
 */
void ScriptGlobal::pushParser(const std::string& groupName, ScriptParserEventsBase* parser)
{
	parser->logScriptMetadata(true, groupName);
	_parserNames.insert(std::make_pair(parser->getName(), parser));
	_parserEvents.push_back(parser);
}

/**
 * Add new const value.
 */
void ScriptGlobal::addConst(const std::string& name, ScriptValueData i)
{
	for (auto& p : _parserNames)
	{
		p.second->addConst(name, i);
	}
}

/**
 * Update const value.
 */
void ScriptGlobal::updateConst(const std::string& name, ScriptValueData i)
{
	for (auto& p : _parserNames)
	{
		p.second->updateConst(name, i);
	}
}

/**
 * Get global ref data.
 */
const ScriptRefData* ScriptGlobal::getRef(ScriptRef name, ScriptRef postfix) const
{
	return findSortHelper(_refList, name, postfix);
}

/**
 * Prepare for loading data.
 */
void ScriptGlobal::beginLoad()
{

}

/**
 * Prepare for loading file from mod.
 */
void ScriptGlobal::fileLoad(const std::string& path)
{
	_currFile = path;
}

/**
 * Finishing loading data.
 */
void ScriptGlobal::endLoad()
{
	for (auto& p : _parserEvents)
	{
		_events.push_back(p->releseEvents());
	}
	_parserNames.clear();
	_parserEvents.clear();
}

/**
 * Load global data from YAML.
 */
void ScriptGlobal::load(const YAML::Node& node)
{
	if (const YAML::Node& t = node["tags"])
	{
		for (auto& p : _tagNames)
		{
			const auto nodeName = p.second.name.toString();
			const YAML::Node &tags = t[nodeName];
			if (tags && tags.IsMap())
			{
				for (const std::pair<YAML::Node, YAML::Node>& i : tags)
				{
					auto type = i.second.as<std::string>();
					auto name = i.first.as<std::string>();
					auto invalidType = _tagValueTypes.size();
					auto valueType = invalidType;
					for (size_t typei = 0; typei < invalidType; ++typei)
					{
						if (ScriptRef::tempFrom(type) == _tagValueTypes[typei].name)
						{
							valueType = typei;
							break;
						}
					}
					if (valueType != invalidType)
					{
						auto namePrefix = "Tag." + name;
						auto ref = getRef(ScriptRef::tempFrom(namePrefix));
						if (ref && ref->type != p.first)
						{
							Log(LOG_ERROR) << "Script variable '" + name + "' already used in '" + _tagNames[ref->type].name.toString() + "'.";
							continue;
						}
						auto tag = getTag(p.first, ScriptRef::tempFrom(namePrefix));
						if (tag)
						{
							auto data = getTagValueData(p.first, tag);
							if (valueType != data.valueType)
							{
								Log(LOG_ERROR) << "Script variable '" + name + "' have wrong type '" << _tagValueTypes[valueType].name.toString() << "' instead of '" << _tagValueTypes[data.valueType].name.toString() << "' in '" + nodeName + "'.";
							}
							continue;
						}
						tag = addTag(p.first, addNameRef(namePrefix), valueType);
						if (!tag)
						{
							Log(LOG_ERROR) << "Script variable '" + name + "' exceeds limit of " << (int)p.second.limit << " available variables in '" + nodeName + "'.";
							continue;
						}
					}
					else
					{
						Log(LOG_ERROR) << "Invalid type def '" + type + "' for script variable '" + name + "' in '" + nodeName +"'.";
					}
				}
			}
		}
	}
	if (const YAML::Node& s = node["scripts"])
	{
		for (auto& p : _parserNames)
		{
			p.second->load(s);
		}
	}
}




#ifdef OXCE_AUTO_TEST

namespace
{


struct Func_test_a
{
	[[gnu::always_inline]]
	static RetEnum func (ScriptWorkerBase& c, int p, int& b)
	{

		return RetContinue;
	}
};

struct Func_test_b
{
	[[gnu::always_inline]]
	static RetEnum func (int p, int& b)
	{

		return RetContinue;
	}
};

struct Func_test_c
{
	[[gnu::always_inline]]
	static RetEnum func (int p, int& b, ScriptWorkerBase& c)
	{

		return RetContinue;
	}
};

[[maybe_unused]]
static auto dummyTestScriptOverload = ([]
{
	ScriptProcData data_a {	};
	data_a.overload = &overloadCustomProc;
	data_a.overloadArg = helper::FuncGroup<Func_test_a>::overloadType();

	ScriptProcData data_b {	};
	data_b.overload = &overloadCustomProc;
	data_b.overloadArg = helper::FuncGroup<Func_test_b>::overloadType();

	ScriptProcData data_c {	};
	data_c.overload = &overloadCustomProc;
	data_c.overloadArg = helper::FuncGroup<Func_test_c>::overloadType();


	auto arg_any = ArgInvalid;
	auto arg_int = ArgInt;
	auto arg_int_ref = ArgSpecAdd(ArgInt, ArgSpecReg);
	auto arg_int_var = ArgSpecAdd(ArgInt, ArgSpecVar);

	auto test_overload = [](const ScriptProcData& a, std::initializer_list<ArgEnum> ref)
	{
		std::array<ScriptRefData, 10> arr = {};
		int i = 0;
		for (auto& p : ref)
		{
			arr[i++] = { {}, p };
		}
		return overloadCustomProc(a, arr.data(), arr.data() + ref.size());
	};

	assert(3 == data_a.overloadArg.size());
	assert(2 == data_b.overloadArg.size());
	assert(3 == data_c.overloadArg.size());

	assert(2 == getOverloadArgSize(data_a));
	assert(2 == getOverloadArgSize(data_b));
	assert(2 == getOverloadArgSize(data_c));

	assert(0 == test_overload(data_a, { }));

	assert(0 == test_overload(data_a, { arg_any, }));

	assert(255 == test_overload(data_a, { arg_any, arg_any, }));

	assert(255 - 1 == test_overload(data_a, { arg_int, arg_any, }));

	assert(0 == test_overload(data_a, { arg_int, arg_int, }));

	assert(255 - 1 == test_overload(data_a, { arg_int, arg_int_var, }));

	assert(255 == test_overload(data_a, { arg_any, arg_int_var, }));

	assert(0 == test_overload(data_a, { arg_any, arg_int_var, arg_any, }));

	assert(255 - 64 - 1 == test_overload(data_a, { arg_int_var, arg_any, }));

	assert(255 - 64 - 1 == test_overload(data_a, { arg_int_var, arg_int_var, }));

	auto test_arg = [](const ScriptProcData& a, int i, std::initializer_list<ArgEnum> ref)
	{
		auto args = getOverloadArgType(a, i);
		return std::equal(
			std::begin(args), std::end(args),
			std::begin(ref), std::end(ref)
		);
	};

	assert(test_arg(data_a, 0, { arg_int, arg_int_ref }));
	assert(test_arg(data_a, 1, { arg_int_var }));
	assert(test_arg(data_a, 2, { }));

	assert(test_arg(data_b, 0, { arg_int, arg_int_ref }));
	assert(test_arg(data_b, 1, { arg_int_var }));
	assert(test_arg(data_b, 2, { }));

	assert(test_arg(data_c, 0, { arg_int, arg_int_ref }));
	assert(test_arg(data_c, 1, { arg_int_var }));
	assert(test_arg(data_c, 2, { }));

	return 0;
})();


struct ScriptParserTest : ScriptParserBase
{
	ScriptParserTest(ScriptGlobal* g) : ScriptParserBase(g, "X")
	{

	}
};
struct DummyClass
{
	/// Name of class used in script.
	static constexpr const char *ScriptName = "DummyClass";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);
};
void dummyFunctionInt(int i, int j)
{

}
void dummyFunctionClass(const DummyClass* c)
{

}

[[maybe_unused]]
static auto dummyTestScriptFunctionParser = ([]
{
	ScriptGlobal g;
	ScriptParserTest f(&g);

	f.addType<DummyClass*>("DummyClass");

	Bind<DummyClass> bind{ &f };
	bind.addCustomFunc<helper::BindFunc<MACRO_CLANG_AUTO_HACK(&dummyFunctionInt)>>("test1");
	bind.add<&dummyFunctionClass>("test2");
	bind.add<&dummyFunctionClass>("test3");


	ScriptContainerBase tempScript;
	ParserWriter help(
		0,
		tempScript,
		f
	);
	help.addReg<DummyClass*&>(ScriptRef{"foo"});
	help.addReg<DummyClass*&>(ScriptRef{"bar.a"});
	help.addReg<DummyClass*&>(ScriptRef{"bar.b"});
	help.addReg<DummyClass*&>(ScriptRef{"Tag.foo"});


	{
		auto r = help.getReferece(ScriptRef{"foo"});
		assert(!!r && "reg 'foo'");
	}

	{
		auto r = help.getReferece(ScriptRef{"bar.a"});
		assert(!!r && "reg 'bar.a'");
	}

	{
		auto r = help.getReferece(ScriptRef{"bar.b"});
		assert(!!r && "reg 'bar.b'");
	}

	{
		auto r = help.getReferece(ScriptRef{"Tag.foo"});
		assert(!!r && "reg 'Tag.foo'");
	}



	{
		auto getProcFromParser = [&](std::initializer_list<ScriptRef> l)
		{
			return !!help.parser.getProc(ScriptRange{ l.begin(), l.end() });
		};
		assert(getProcFromParser({ ScriptRef{"DummyClass.test2"} }));
		assert(getProcFromParser({ ScriptRef{"DummyClass.test3"} }));
		assert(getProcFromParser({ ScriptRef{"DummyClass"}, ScriptRef{".test2"} }));
		assert(getProcFromParser({ ScriptRef{"DummyClass"}, ScriptRef{"."} , ScriptRef{"test2"} }));
		assert(getProcFromParser({ ScriptRef{"DummyClass"}, ScriptRef{"."} , ScriptRef{"te"} , ScriptRef{"st2"} }));
		assert(!getProcFromParser({ ScriptRef{"DummyClass.test1"} }));
	}



	{
		auto r = findOperationAndArg(help, ScriptRef{"if"});
		assert(!!r && "func 'if'");
		assert(r.haveArg() == false && "func 'if'");
		assert(r.haveProc() == true && "func 'if'");
	}

	{
		auto r = findOperationAndArg(help, ScriptRef{"test1"});
		assert(!!r && "func 'test1'");
		assert(r.haveArg() == false && "func 'test1'");
		assert(r.haveProc() == true && "func 'test1'");
	}

	{
		auto r = findOperationAndArg(help, ScriptRef{"DummyClass.test2"});
		assert(!!r && "func 'DummyClass.test2'");
		assert(r.haveArg() == false && "func 'DummyClass.test2'");
		assert(r.haveProc() == true && "func 'DummyClass.test2'");
	}

	{
		auto r = findOperationAndArg(help, ScriptRef{"foo.test2"});
		assert(!!r && "func 'foo.test2'");
		assert(r.haveArg() == true && "func 'foo.test2'");
		assert(r.argName == ScriptRef{"foo"} && "func 'foo.test2'");
		assert(r.haveProc() == true && "func 'foo.test2'");
	}

	{
		auto r = findOperationAndArg(help, ScriptRef{"bar.a.test2"});
		assert(!!r && "func 'bar.a.test2'");
		assert(r.haveArg() == true && "func 'bar.a.test2'");
		assert(r.argName == ScriptRef{"bar.a"} && "func 'bar.a.test2'");
		assert(r.haveProc() == true && "func 'bar.a.test2'");
	}

	{
		auto r = findOperationAndArg(help, ScriptRef{"Tag.foo.test2"});
		assert(!!r && "func 'Tag.foo.test2'");
		assert(r.haveArg() == true && "func 'Tag.foo.test2'");
		assert(r.argName == ScriptRef{"Tag.foo"} && "func 'Tag.foo.test2'");
		assert(r.haveProc() == true && "func 'Tag.foo.test2'");
	}

	{
		auto r = findOperationAndArg(help, ScriptRef{"bar.a2.test2"});
		assert(!r && "func 'bar.a2.test2'");
		assert(r.haveArg() == true && "func 'bar.a2.test2'");
		assert(r.argName == ScriptRef{"bar"} && "func 'bar.a2.test2'");
	}

	{
		auto r = findOperationAndArg(help, ScriptRef{"Tag.foo2.test2"});
		assert(!r && "func 'Tag.foo.test2'");
		assert(r.haveArg() == true && "func 'Tag.foo2.test2'");
		assert(r.argName == ScriptRef{"Tag.foo2"} && "func 'Tag.foo2.test2'");
	}


	{
		auto r = findOperationAndArg(help, ScriptRef{"Tag.foo.test2"});
		assert(!!r && "func 'Tag.foo.test2'");

		{
			auto u = replaceOperation(help, r, ScriptRef{"test2"}, ScriptRef{"test3"});
			assert(!!u && "updated 'test2' to 'Tag.foo.test3'");
		}

		{
			auto u = replaceOperation(help, r, ScriptRef{"2"}, ScriptRef{"3"});
			assert(!!u && "updated '2' to 'Tag.foo.test1'");
		}

		{
			auto u = replaceOperation(help, r, ScriptRef{"test3"}, ScriptRef{"test3"});
			assert(!u && "updated 'test3' to 'Tag.foo.test3'");
		}
	}

	return 0;
})();


void dummyFunctionSeperator0(int& i, int& j, int& k)
{
	i = 0;
}
void dummyFunctionSeperator1(int& i, ScriptArgSeparator, int& j, int& k)
{
	i = 1;
}
void dummyFunctionSeperator2(int& i, int& j, ScriptArgSeparator, int& k)
{
	i = 2;
}
void dummyFunctionSeperator3(int& i, int& j, int& k, ScriptArgSeparator)
{
	i = 3;
}

[[maybe_unused]]
static auto dummyTestScriptOverloadSeperator = ([]
{
	ScriptGlobal g;
	ScriptParserTest f(&g);

	Bind<DummyClass> bind{ &f };
	bind.addCustomFunc<helper::BindFunc<MACRO_CLANG_AUTO_HACK(&dummyFunctionSeperator0)>>("funcSep");
	bind.addCustomFunc<helper::BindFunc<MACRO_CLANG_AUTO_HACK(&dummyFunctionSeperator1)>>("funcSep");
	bind.addCustomFunc<helper::BindFunc<MACRO_CLANG_AUTO_HACK(&dummyFunctionSeperator2)>>("funcSep");
	bind.addCustomFunc<helper::BindFunc<MACRO_CLANG_AUTO_HACK(&dummyFunctionSeperator3)>>("funcSep");


	ScriptContainerBase tempScript;
	ParserWriter help(
		0,
		tempScript,
		f
	);
	auto arg_x = help.addReg<int&>(ScriptRef{"x"});
	auto arg_y = help.addReg<int&>(ScriptRef{"y"});
	auto arg_z = help.addReg<int&>(ScriptRef{"z"});
	auto arg_sep = help.getReferece(ScriptRef{"__"});

	assert(arg_x);
	assert(arg_y);
	assert(arg_z);
	assert(arg_sep);

	auto callFunc = [&](std::tuple<int, const ScriptProcData*> t, const ScriptRefData* begin, const ScriptRefData* end)
	{
		auto p = std::get<const ScriptProcData*>(t);
		if (p == nullptr)
		{
			return -1;
		}
		for (auto arg : p->overloadArg)
		{
			assert(arg.size() == 1);
		}
		auto func = p->parserGet(0);

		Uint8 dummy[64] = { };
		ScriptWorkerBase wb;
		ProgPos pos;

		wb.ref<int>(arg_x.getValue<RegEnum>()) = -1;
		func(wb, dummy, pos);
		return wb.ref<int>(arg_x.getValue<RegEnum>());
	};

	auto r = findOperationAndArg(help, ScriptRef{"funcSep"});
	assert(!!r && "func 'funcSep'");

	{
		ScriptRefData args[] = { arg_x, arg_y, arg_z };
		auto o = findBestOverloadProc(r.procList, std::begin(args), std::end(args));
		assert(std::get<int>(o) && "args 'funcSep x y z'");
		assert(callFunc(o, std::begin(args), std::end(args)) == 0);
	}

	{
		ScriptRefData args[] = { arg_x, arg_sep, arg_y, arg_z };
		auto o = findBestOverloadProc(r.procList, std::begin(args), std::end(args));
		assert(std::get<int>(o) && "args 'funcSep x __ y z'");
		assert(callFunc(o, std::begin(args), std::end(args)) == 1);
	}

	{
		ScriptRefData args[] = { arg_x, arg_y, arg_sep, arg_z };
		auto o = findBestOverloadProc(r.procList, std::begin(args), std::end(args));
		assert(std::get<int>(o) && "args 'funcSep x y __ z'");
		assert(callFunc(o, std::begin(args), std::end(args)) == 2);
	}

	{
		ScriptRefData args[] = { arg_x, arg_y, arg_z, arg_sep };
		auto o = findBestOverloadProc(r.procList, std::begin(args), std::end(args));
		assert(std::get<int>(o) && "args 'funcSep x y z __'");
		assert(callFunc(o, std::begin(args), std::end(args)) == 3);
	}

	return 0;
})();


[[maybe_unused]]
static auto dummyTestScriptRefTokens = ([]
{
	{
		ScriptRefTokens srt{"aaaa bb"};
		{
			SelectedToken next = srt.getNextToken();
			assert(next == ScriptRef{"aaaa"} && next.getType() == TokenSymbol);
		}
		{
			SelectedToken next = srt.getNextToken();
			assert(next == ScriptRef{"bb"} && next.getType() == TokenSymbol);
		}
		{
			SelectedToken next = srt.getNextToken();
			assert(next.getType() == TokenNone);
		}
	}

	{
		ScriptRefTokens srt{"0x10 1234"};
		{
			SelectedToken next = srt.getNextToken();
			assert(next == ScriptRef{"0x10"} && next.getType() == TokenNumber);
		}
		{
			SelectedToken next = srt.getNextToken();
			assert(next == ScriptRef{"1234"} && next.getType() == TokenNumber);
		}
		{
			SelectedToken next = srt.getNextToken();
			assert(next.getType() == TokenNone);
		}
	}

	auto getType = [](ScriptRef ref, TokenEnum next = TokenNone)
	{
		ScriptRefTokens srt{ref.begin(), ref.end()};
		return srt.getNextToken(next).getType();
	};

	assert(getType(ScriptRef{":"}) == TokenInvalid);
	assert(getType(ScriptRef{":"}, TokenColon) == TokenColon);
	assert(getType(ScriptRef{";"}) == TokenNone);
	assert(getType(ScriptRef{";"}, TokenSemicolon) == TokenSemicolon);
	assert(getType(ScriptRef{"\"aaa\""}) == TokenText);
	assert(getType(ScriptRef{"0x1"}) == TokenNumber);
	assert(getType(ScriptRef{""}) == TokenNone);
	assert(getType(ScriptRef{" "}) == TokenNone);
	assert(getType(ScriptRef{"#aaaaa"}) == TokenNone);
	assert(getType(ScriptRef{"  #  1235"}) == TokenNone);
	assert(getType(ScriptRef{"  #  \n1235"}) == TokenNumber);
	assert(getType(ScriptRef{" a"}) == TokenSymbol);
	assert(getType(ScriptRef{" \na"}) == TokenSymbol);
	assert(getType(ScriptRef{"a111"}) == TokenSymbol);


	assert(getType(ScriptRef{"0x"}) == TokenInvalid);
	assert(getType(ScriptRef{"0xk"}) == TokenInvalid);

	return 0;
})();


[[maybe_unused]]
static auto dummyTestScriptStringRef = ([]
{
	assert(ScriptRef{"foo"} == ScriptRef{"foo"}.substr(0));
	assert(ScriptRef{"oo"} == ScriptRef{"foo"}.substr(1));
	assert(ScriptRef{"o"} == ScriptRef{"foo"}.substr(2));
	assert(ScriptRef{""} == ScriptRef{"foo"}.substr(3));
	assert(ScriptRef{""} == ScriptRef{"foo"}.substr(4));

	assert(ScriptRef{""} == ScriptRef{"foo1234"}.substr(3, 0));
	assert(ScriptRef{"1"} == ScriptRef{"foo1234"}.substr(3, 1));
	assert(ScriptRef{"12"} == ScriptRef{"foo1234"}.substr(3, 2));
	assert(ScriptRef{"123"} == ScriptRef{"foo1234"}.substr(3, 3));
	assert(ScriptRef{"1234"} == ScriptRef{"foo1234"}.substr(3, 4));
	assert(ScriptRef{"1234"} == ScriptRef{"foo1234"}.substr(3, 5));

	assert(ScriptRef{""} == ScriptRef{"12345"}.head(0));
	assert(ScriptRef{"1"} == ScriptRef{"12345"}.head(1));
	assert(ScriptRef{"12"} == ScriptRef{"12345"}.head(2));
	assert(ScriptRef{"123"} == ScriptRef{"12345"}.head(3));
	assert(ScriptRef{"1234"} == ScriptRef{"12345"}.head(4));
	assert(ScriptRef{"12345"} == ScriptRef{"12345"}.head(5));
	assert(ScriptRef{"12345"} == ScriptRef{"12345"}.head(6));

	assert(ScriptRef{"12345"} == ScriptRef{"12345"}.tail(0));
	assert(ScriptRef{"2345"} == ScriptRef{"12345"}.tail(1));
	assert(ScriptRef{"345"} == ScriptRef{"12345"}.tail(2));
	assert(ScriptRef{"45"} == ScriptRef{"12345"}.tail(3));
	assert(ScriptRef{"5"} == ScriptRef{"12345"}.tail(4));
	assert(ScriptRef{""} == ScriptRef{"12345"}.tail(5));
	assert(ScriptRef{""} == ScriptRef{"12345"}.tail(6));

	assert(ScriptRef{""} == ScriptRef{"12345"}.headFromEnd(0));
	assert(ScriptRef{"5"} == ScriptRef{"12345"}.headFromEnd(1));
	assert(ScriptRef{"45"} == ScriptRef{"12345"}.headFromEnd(2));
	assert(ScriptRef{"345"} == ScriptRef{"12345"}.headFromEnd(3));
	assert(ScriptRef{"2345"} == ScriptRef{"12345"}.headFromEnd(4));
	assert(ScriptRef{"12345"} == ScriptRef{"12345"}.headFromEnd(5));
	assert(ScriptRef{"12345"} == ScriptRef{"12345"}.headFromEnd(6));

	assert(ScriptRef{"12345"} == ScriptRef{"12345"}.tailFromEnd(0));
	assert(ScriptRef{"1234"} == ScriptRef{"12345"}.tailFromEnd(1));
	assert(ScriptRef{"123"} == ScriptRef{"12345"}.tailFromEnd(2));
	assert(ScriptRef{"12"} == ScriptRef{"12345"}.tailFromEnd(3));
	assert(ScriptRef{"1"} == ScriptRef{"12345"}.tailFromEnd(4));
	assert(ScriptRef{""} == ScriptRef{"12345"}.tailFromEnd(5));
	assert(ScriptRef{""} == ScriptRef{"12345"}.tailFromEnd(6));

	return 0;
})();


[[maybe_unused]]
static auto dummyTestScriptRefCompound = ([]
{
	ScriptRefCompound t;
	assert(t.toString() == "");
	assert(t.tryPushBack(ScriptRef{"f1"}));
	assert(t.toString() == "f1");
	assert(t.tryPushBack(ScriptRef{"f2"}));
	assert(t.toString() == "f1f2");
	assert(t.tryPushBack(ScriptRef{"f3"}));
	assert(t.toString() == "f1f2f3");
	assert(t.tryPushBack(ScriptRef{"f4"}));
	assert(t.toString() == "f1f2f3f4");
	assert(!t.tryPushBack(ScriptRef{"f5"}));
	assert(t.toString() == "f1f2f3f4");
	assert(t.tryPopBack());
	assert(t.toString() == "f1f2f3");
	assert(t.tryPopBack());
	assert(t.toString() == "f1f2");
	assert(t.tryPopBack());
	assert(t.toString() == "f1");
	assert(t.tryPopBack());
	assert(t.toString() == "");
	assert(!t.tryPopBack());
	assert(t.toString() == "");
	assert(t.tryPushBack(ScriptRef{"f6"}));
	assert(t.toString() == "f6");
	return 0;
})();


[[maybe_unused]]
static auto dummyTestScriptArgList = ([]
{
	ScriptArgList list1;
	ScriptArgList list2;
	ScriptArgList list3;
	ScriptRefData arg_a = { ScriptRef{ "a" }, ArgInvalid };
	ScriptRefData arg_b = { ScriptRef{ "b" }, ArgInvalid };

	assert(list1.tryPushBack(arg_a));
	assert(list1.tryPushBack(arg_b));
	assert(list1.size() == 2);
	assert(list2.tryPushBack(list1));
	assert(list2.tryPushBack(list1));
	assert(list2.size() == 4);
	assert(list3.tryPushBack(list2));
	assert(list3.tryPushBack(list2));
	assert(list3.size() == 8);
	assert(list3.tryPushBack(list3));
	assert(list3.size() == 16);
	assert(!list3.tryPushBack(list3));
	assert(list3.size() == 16);
	assert(!list3.tryPushBack(arg_a));
	assert(list3.size() == 16);

	return 0;
})();


[[maybe_unused]]
static auto dummyTestFunctions = ([]
{
	auto call_mulDiv_h = [](int reg, int mul, int div)
	{
		mulDiv_h(reg, mul, div);
		return reg;
	};

	assert(1 == call_mulDiv_h(1, 100, 100));
	assert(1 == call_mulDiv_h(2, 50, 100));
	assert(INT_MAX == call_mulDiv_h(INT_MAX, 100, 100));
	assert(INT_MAX / 2 == call_mulDiv_h(INT_MAX, 50, 100));

	auto call_mulAddMod_h = [](int reg, int mul, int add, int div)
	{
		mulAddMod_h(reg, mul, add, div);
		return reg;
	};

	assert(1 == call_mulAddMod_h(100, 100, 1, 100));
	assert(1 == call_mulAddMod_h(INT_MAX, 100, 1, 100));


	return 0;
})();


[[maybe_unused]]
static auto dummyTestScriptLowerBound = ([]
{
	std::vector<ScriptTypeData> test;
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "b" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "bb" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "bbb" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "bbbb" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "c" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "cc" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "ccc" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "a" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "aa" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "aaa" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "aaaa" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "aaab" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "aaaba" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "aaaaa" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "abcde" }  });
	addSortHelper(test, ScriptTypeData{ ScriptRef{ "abcdf" }  });

	auto pairRange = [&](const auto &pr, const auto &po)
	{
		ScriptRef prefix{ pr };
		ScriptRef postfix{ po };
		auto lower = test.data();
		auto upper = test.data() + test.size();
		lower = boundSortHelper<false>(lower, upper, prefix, postfix);
		upper = boundSortHelper<true>(lower, upper, prefix, postfix);
		return std::make_pair(lower, upper);
	};
	auto listRange = [&](std::initializer_list<const char*> l)
	{
		ScriptRef prefix[64] = { };
		int i = 0;
		for (auto* p : l)
		{
			prefix[i] = ScriptRef{ p, p + std::strlen(p) };
			++i;
		}
		auto lower = test.data();
		auto upper = test.data() + test.size();
		lower = boundSortHelper<false>(lower, upper, ScriptRange{ prefix, prefix + i });
		upper = boundSortHelper<true>(lower, upper, ScriptRange{ prefix, prefix + i });
		return std::make_pair(lower, upper);
	};
	auto foundSomething = [](std::pair<const ScriptTypeData*, const ScriptTypeData*> p)
	{
		return p.first != p.second;
	};

	assert(true == foundSomething(pairRange("aa", "")));
	assert(true == foundSomething(pairRange("aaaa", "")));
	assert(true == foundSomething(pairRange("aa", "aa")));
	assert((pairRange("aaaa", "")) == (pairRange("aa", "aa")));
	assert((pairRange("abcde", "")) == (pairRange("abc", "de")));
	assert((pairRange("abcde", "")) == (pairRange("ab", "cde")));

	assert(false == foundSomething(listRange({"www"})));
	assert(false == foundSomething(listRange({"www", ""})));
	assert(true == foundSomething(listRange({"aa"})));
	assert(true == foundSomething(listRange({"aa", ""})));
	assert(true == foundSomething(listRange({"aaaa", ""})));
	assert(true == foundSomething(listRange({"aa", "aa"})));
	assert((listRange({"aaaa", ""})) == (listRange({"aa", "aa"})));
	assert((listRange({"abcde", ""})) == (listRange({"abc", "de"})));
	assert((listRange({"abcde", ""})) == (listRange({"ab", "cde"})));
	assert((listRange({"abcde", ""})) == (listRange({"a", "b", "cde"})));
	assert((listRange({"abcde", ""})) == (listRange({"a", "b", "c", "d", "e"})));
	assert(false == foundSomething(listRange({"www", ""})));

	assert((pairRange("abcde", "")) == (listRange({"a", "b", "cde"})));

	assert((pairRange("aaaba", "").second) == (pairRange("abcde", "").first));
	assert((pairRange("abcde", "").second) == (pairRange("ab", "cdf").first));

	return 0;
})();


} //namespace


#endif

} //namespace OpenXcom
