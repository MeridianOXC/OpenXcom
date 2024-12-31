#pragma once
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
#include "Script.h"
#include "Exception.h"
#include "Logger.h"
#include <functional>
#include <utility>

namespace OpenXcom
{


/**
 * Hack needed by Clang 6.0 to properly transfer `auto` template parameters,
 * This need be macro because GCC 7.4 fail on this hack,
 * VS look it could handle both cases
 */
#ifdef __clang__
template<auto T>
static constexpr decltype(T) _clang_auto_hack()
{
	return T;
}
#define MACRO_CLANG_AUTO_HACK(T) _clang_auto_hack<T>()
#else
#define MACRO_CLANG_AUTO_HACK(T) T
#endif


/**
 * Type of code block
 */
enum BlockEnum
{
	/// Block that represents whole body of script
	BlockMain,
	/// Normal block
	BlockBegin,
	/// Conditional block
	BlockIf,
	/// Else block of conditional block
	BlockElse,
	/// Iteraion block
	BlockLoop,
};


/**
 * Helper structure used by ScriptParser::parse
 */
struct ParserWriter
{
	struct Block
	{
		BlockEnum type;
		size_t regStackSizeFrom;
		RegEnum regIndexUsedFrom;
		ScriptRefData nextLabel = { };
		ScriptRefData finalLabel = { };
	};

	template<typename T, typename = typename std::enable_if_t<std::is_pod<T>::value>>
	class ReservedPos
	{
		ProgPos _pos;
		ReservedPos(ProgPos pos) : _pos{ pos }
		{

		}
		ProgPos getPos()
		{
			return _pos;
		}

		friend struct ParserWriter;
	};

	/// Tag type representing position script operation id in proc vector.
	class ProcOp { };

	/// List of all places in proc vector where we need have same values
	template<typename T, typename CompType = T>
	class ReservedCrossRefrenece
	{
		enum Ref : std::size_t { };

		/// list of places of usage.
		std::vector<std::pair<ReservedPos<T>, Ref>> positions;
		/// list of final values.
		std::vector<CompType> values;
	public:

		ReservedCrossRefrenece() {}

		ScriptValueData addValue(CompType defaultValue)
		{
			auto index = static_cast<Ref>(values.size());
			values.push_back(defaultValue);
			return index;
		}

		CompType getValue(ScriptValueData data)
		{
			auto index = data.getValue<Ref>();
			return values[index];
		}

		void setValue(ScriptValueData data, CompType value)
		{
			auto index = data.getValue<Ref>();
			values[index] = value;
		}

		void pushPosition(ParserWriter& pw, ScriptValueData data)
		{
			auto index = data.getValue<Ref>();
			positions.push_back(std::make_pair(pw.pushReserved<T>(), index));
		}

		template<typename Func>
		void forEachPosition(Func&& f)
		{
			for (const auto& pos : positions)
			{
				f(pos.first, values[static_cast<std::size_t>(pos.second)]);
			}
		}
	};

	/// member pointer accessing script operations.
	ScriptContainerBase& container;
	/// all available data for script.
	const ScriptParserBase& parser;
	/// list of labels.
	ReservedCrossRefrenece<ProgPos> refLabels;
	/// list of texts.
	ReservedCrossRefrenece<ScriptText, ScriptRef> refTexts;

	/// index of used script registers.
	RegEnum regIndexUsed;

	/// Stack of registers limited to code blocks.
	std::vector<ScriptRefData> regStack;
	/// Store position of blocks of code like "if" or "while".
	std::vector<Block> codeBlocks;



	/// Constructor.
	ParserWriter(
			size_t regUsed,
			ScriptContainerBase& c,
			const ScriptParserBase& d);

	/// Final fixes of data.
	void relese();

	/// Get reference based on name.
	ScriptRefData getReferece(const ScriptRef& s) const;

	/// Get current position in proc vector.
	ProgPos getCurrPos() const;
	/// Get distance between two positions in proc vector.
	size_t getDiffPos(ProgPos begin, ProgPos end) const;


	/// Push zeros to fill empty space.
	ProgPos push(size_t s);
	/// Update space on proc vector.
	void update(ProgPos pos, void* data, size_t s);

	/// Preparing place and position on proc vector for some value and return position of it.
	template<typename T>
	ReservedPos<T> pushReserved()
	{
		return { ParserWriter::push(sizeof(T)) };
	}
	/// Setting previously prepared place with value.
	template<typename T>
	void updateReserved(ReservedPos<T> pos, T value)
	{
		update(pos.getPos(), &value, sizeof(T));
	}


	/// Push custom value on proc vector.
	void pushValue(ScriptValueData v);

	/// Pushing proc operation id on proc vector.
	ReservedPos<ProcOp> pushProc(Uint8 procId);

	/// Updating previously added proc operation id.
	void updateProc(ReservedPos<ProcOp> pos, int procOffset);

	/// Try pushing label arg on proc vector. Can't use this to create loop back label.
	bool pushLabelTry(const ScriptRefData& data);

	/// Create new label for proc vector.
	ScriptRefData addLabel(const ScriptRef& data = {});

	/// Setting offset of label on proc vector.
	bool setLabel(const ScriptRefData& data, ProgPos offset);

	/// Try pushing text literal arg on proc vector.
	bool pushTextTry(const ScriptRefData& data);

	/// Try pushing reg arg on proc vector.
	template<typename T>
	bool pushConstTry(const ScriptRefData& data)
	{
		return pushConstTry(data, ScriptParserBase::getArgType<T>());
	}

	/// Try pushing data arg on proc vector.
	bool pushConstTry(const ScriptRefData& data, ArgEnum type);

	/// Try pushing reg arg on proc vector.
	template<typename T>
	bool pushRegTry(const ScriptRefData& data)
	{
		return pushRegTry(data, ScriptParserBase::getArgType<T>());
	}

	/// Try pushing reg arg on proc vector.
	bool pushRegTry(const ScriptRefData& data, ArgEnum type);

	/// Add new reg arg.
	template<typename T>
	ScriptRefData addReg(const ScriptRef& s)
	{
		return addReg(s, ScriptParserBase::getArgType<T>());
	}

	/// Add new reg arg.
	ScriptRefData addReg(const ScriptRef& s, ArgEnum type);

	/// Add new local const.
	ScriptRefData addConst(const ScriptRef& s, ArgEnum type, ScriptValueData value);



	/// Add new code scope.
	Block& pushScopeBlock(BlockEnum type);
	/// Clear values in code scope.
	Block& clearScopeBlock();
	/// Pop code scope.
	Block popScopeBlock();



	/// Dump to log error info about ref.
	void logDump(const ScriptRefData&) const;

}; //struct ParserWriter


////////////////////////////////////////////////////////////
//				Mata helper classes
////////////////////////////////////////////////////////////

namespace helper
{

template<typename T, int P>
using GetType = decltype(T::typeFunc(PosTag<P>{}));


template<int MaxSize, typename... T>
struct SumListIndexImpl;

template<int MaxSize, typename T1, typename... T>
struct SumListIndexImpl<MaxSize, T1, T...> : SumListIndexImpl<MaxSize, T...>
{
	using tag = PosTag<MaxSize - (1 + sizeof...(T))>;

	using SumListIndexImpl<MaxSize, T...>::typeFunc;
	static T1 typeFunc(tag);

	static constexpr ScriptFunc getDynamic(int i)
	{
		return tag::value == i ? T1::func : SumListIndexImpl<MaxSize, T...>::getDynamic(i);
	}
};

template<int MaxSize>
struct SumListIndexImpl<MaxSize>
{
	struct End
	{
		static constexpr int offset = 0;

		[[gnu::always_inline]]
		static RetEnum func(ScriptWorkerBase &, const Uint8 *, ProgPos &)
		{
			return RetError;
		}
	};
	static End typeFunc(...);
	static constexpr ScriptFunc getDynamic(int i)
	{
		return End::func;
	}
};

template<typename... V>
struct SumList : SumListIndexImpl<sizeof...(V), V...>
{
	static constexpr size_t size = sizeof...(V);

	using SumListIndexImpl<sizeof...(V), V...>::typeFunc;
	using SumListIndexImpl<sizeof...(V), V...>::getDynamic;

	template<typename... F2>
	constexpr SumList<V..., F2...> operator+(SumList<F2...>) const { return {}; }
	constexpr SumList<V...> operator+() const { return {}; }
};

////////////////////////////////////////////////////////////
//						Arg class
////////////////////////////////////////////////////////////

template<typename... A>
struct Arg;

template<typename A1, typename... A2>
struct Arg<A1, A2...> : public Arg<A2...>
{
	using next = Arg<A2...>;
	using tag = PosTag<sizeof...(A2)>;

	static constexpr int offset(int i)
	{
		return i == tag::value ? A1::size : next::offset(i);
	}
	static constexpr int ver()
	{
		return tag::value + 1;
	}

	using next::typeFunc;
	static A1 typeFunc(tag);

	static int parse(ParserWriter& ph, const ScriptRefData* t)
	{
		if (A1::parse(ph, *t))
		{
			return tag::value;
		}
		else
		{
			return next::parse(ph, t);
		}
	}
	static int parse(ParserWriter& ph, const ScriptRefData** begin, const ScriptRefData* end)
	{
		if (*begin == end)
		{
			Log(LOG_ERROR) << "Not enough args in operation";
			return -1;
		}
		else
		{
			int curr = parse(ph, *begin);
			if (curr < 0)
			{
				ph.logDump(**begin);
				return -1;
			}
			++*begin;
			return curr;
		}
	}
	template<typename A>
	static ArgEnum typeHelper()
	{
		return A::type();
	}
	static ScriptRange<ArgEnum> argTypes()
	{
		enum
		{
			ver_temp = ver()
		};
		const static ArgEnum types[ver_temp] = { typeHelper<A1>(), typeHelper<A2>()... };
		return { std::begin(types), std::end(types) };
	}
};

template<>
struct Arg<>
{
	static constexpr int offset(int i)
	{
		return 0;
	}
	static constexpr int ver() = delete;
	static void typeFunc() = delete;

	static int parse(ParserWriter& ph, const ScriptRefData* begin)
	{
		return -1;
	}
	static ScriptRange<ArgEnum> argTypes()
	{
		return { };
	}
};

template<typename T>
struct ArgInternal
{
	using tag = PosTag<0>;

	static constexpr int offset(int i)
	{
		return 0;
	}
	static constexpr int ver()
	{
		return 1;
	};

	static T typeFunc(tag);

	static int parse(ParserWriter& ph, const ScriptRefData** begin, const ScriptRefData* end)
	{
		return 0;
	}
	static ScriptRange<ArgEnum> argTypes()
	{
		return { };
	}
};

////////////////////////////////////////////////////////////
//					ArgColection class
////////////////////////////////////////////////////////////

template<int MaxSize, typename... T>
struct ArgColection;

template<int MaxSize, typename T1, typename... T2>
struct ArgColection<MaxSize, T1, T2...> : public ArgColection<MaxSize, T2...>
{
	using next = ArgColection<MaxSize, T2...>;
	using tag = PosTag<MaxSize - (sizeof...(T2) + 1)>;

	static constexpr int pos(int ver, int pos)
	{
		return (pos == tag::value ? ver % T1::ver() : next::pos(ver / T1::ver(), pos));
	}
	static constexpr int offset(int ver, int pos)
	{
		return (pos > tag::value ? T1::offset(ver % T1::ver()) : 0) + next::offset(ver / T1::ver(), pos);
	}
	static constexpr int ver()
	{
		return T1::ver() * next::ver();
	}
	static constexpr int arg()
	{
		return MaxSize;
	}
	static int parse(ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
	{
		int curr = T1::parse(ph, &begin, end);
		if (curr < 0)
		{
			return -1;
		}
		int lower = next::parse(ph, begin, end);
		if (lower < 0)
		{
			return -1;
		}
		return lower * T1::ver() + curr;
	}
	template<typename T>
	static ScriptRange<ArgEnum> argHelper()
	{
		return T::argTypes();
	}
	static ScriptRange<ScriptRange<ArgEnum>> overloadType()
	{
		const static ScriptRange<ArgEnum> types[MaxSize] =
		{
			argHelper<T1>(),
			argHelper<T2>()...,
		};
		return { std::begin(types), std::end(types) };
	}

	using next::typeFunc;
	static T1 typeFunc(tag);
};

template<int MaxSize>
struct ArgColection<MaxSize>
{
	using tag = PosTag<-1>;

	static constexpr int pos(int ver, int pos)
	{
		return 0;
	}
	static constexpr int offset(int ver, int pos)
	{
		return 0;
	}
	static constexpr int ver()
	{
		return 1;
	}
	static constexpr int arg()
	{
		return 0;
	}
	static int parse(ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
	{
		//we should have used all available tokens.
		if (begin == end)
		{
			return 0;
		}
		else
		{
			Log(LOG_ERROR) << "Too many args in operation";
			return -1;
		}
	}
	static ScriptRange<ScriptRange<ArgEnum>> overloadType()
	{
		return { };
	}
	static void typeFunc() = delete;
};

////////////////////////////////////////////////////////////
//						Arguments implementation
////////////////////////////////////////////////////////////

struct ArgContextDef : ArgInternal<ArgContextDef>
{
	using ReturnType = ScriptWorkerBase&;
	static constexpr size_t size = 0;
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw;
	}
};

struct ArgProgDef : ArgInternal<ArgProgDef>
{
	using ReturnType = ProgPos&;
	static constexpr size_t size = 0;
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return curr;
	}
};

template<typename T>
struct ArgRegDef
{
	using ReturnType = T;
	static constexpr size_t size = sizeof(RegEnum);
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.ref<ReturnType>(sw.const_val<RegEnum>(arg));
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return ph.pushRegTry<ReturnType>(t);
	}

	static ArgEnum type()
	{
		return ArgSpecAdd(ScriptParserBase::getArgType<ReturnType>(), ArgSpecReg);
	}
};
template<typename T>
struct ArgValueDef
{
	using ReturnType = T;
	static constexpr size_t size = sizeof(T);
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.const_val<ReturnType>(arg);
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return ph.pushConstTry<ReturnType>(t);
	}

	static ArgEnum type()
	{
		return ScriptParserBase::getArgType<ReturnType>();
	}
};

struct ArgLabelDef
{
	using ReturnType = ProgPos;
	static constexpr size_t size = sizeof(ReturnType);
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.const_val<ReturnType>(arg);
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return ph.pushLabelTry(t);
	}

	static ArgEnum type()
	{
		return ArgLabel;
	}
};

struct ArgTextDef
{
	using ReturnType = ScriptText;
	static constexpr size_t size = sizeof(ReturnType);
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.const_val<ReturnType>(arg);
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return ph.pushTextTry(t);
	}

	static ArgEnum type()
	{
		return ArgText;
	}
};

struct ArgFuncDef
{
	using ReturnType = ScriptFunc;
	static constexpr size_t size = sizeof(ReturnType);
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.const_val<ReturnType>(arg);
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return false;
	}

	static ArgEnum type()
	{
		return ArgInvalid;
	}
};

template<int Size>
struct ArgRawDef
{
	using ReturnType = const Uint8*;
	static constexpr size_t size = Size;
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return arg;
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return false;
	}

	static ArgEnum type()
	{
		return ArgInvalid;
	}
};

template<typename T>
struct ArgNullDef
{
	using ReturnType = T;
	static constexpr size_t size = 0;
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return ReturnType{};
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return t.type == ArgNull;
	}

	static ArgEnum type()
	{
		return ArgNull;
	}
};

struct ArgSepDef
{
	using ReturnType = ScriptArgSeparator;
	static constexpr size_t size = 0;
	static ReturnType get(ScriptWorkerBase& sw, const Uint8* arg, ProgPos& curr)
	{
		return ReturnType{};
	}

	static bool parse(ParserWriter& ph, const ScriptRefData& t)
	{
		return t.type == ArgSep;
	}

	static ArgEnum type()
	{
		return ArgSep;
	}
};

////////////////////////////////////////////////////////////
//					ArgSelector class
////////////////////////////////////////////////////////////



template<typename T>
struct ArgSelector
{
	using type = Arg<ArgRegDef<T>, ArgValueDef<T>>;
};

template<typename T>
struct ArgSelector<T&>
{
	using type = Arg<ArgRegDef<T&>>;
};

template<typename T>
struct ArgSelector<const T&>
{
	using type = Arg<ArgRegDef<T>, ArgValueDef<T>>;
};



template<>
struct ArgSelector<ScriptWorkerBase&>
{
	using type = ArgContextDef;
};



template<>
struct ArgSelector<ScriptInt&>
{
	using type = Arg<ArgRegDef<ScriptInt&>>;
};

template<>
struct ArgSelector<ScriptInt>
{
	using type = Arg<ArgValueDef<ScriptInt>, ArgRegDef<ScriptInt>>;
};

template<>
struct ArgSelector<const ScriptInt&> : ArgSelector<ScriptInt>
{

};

template<>
struct ArgSelector<bool>
{
	using type = Arg<ArgValueDef<ScriptInt>, ArgRegDef<ScriptInt>>;
};



template<>
struct ArgSelector<ScriptText&>
{
	using type = Arg<ArgRegDef<ScriptText&>>;
};

template<>
struct ArgSelector<ScriptText>
{
	using type = Arg<ArgTextDef, ArgRegDef<ScriptText>>;
};

template<>
struct ArgSelector<const std::string&>
{
	using type = Arg<ArgTextDef, ArgRegDef<ScriptText>>;
};



template<typename T, typename I>
struct ArgSelector<ScriptTag<T, I>>
{
	using type = Arg<ArgValueDef<ScriptTag<T, I>>, ArgRegDef<ScriptTag<T, I>>, ArgNullDef<ScriptTag<T, I>>>;
};

template<typename T, typename I>
struct ArgSelector<ScriptTag<T, I>&>
{
	using type = Arg<ArgRegDef<ScriptTag<T, I>&>>;
};

template<typename T, typename I>
struct ArgSelector<const ScriptTag<T, I>&> : ArgSelector<ScriptTag<T, I>>
{

};




template<>
struct ArgSelector<ProgPos&>
{
	using type = ArgProgDef;
};

template<>
struct ArgSelector<ProgPos>
{
	using type = Arg<ArgLabelDef>;
};




template<>
struct ArgSelector<ScriptFunc>
{
	using type = Arg<ArgFuncDef>;
};


template<>
struct ArgSelector<const Uint8*>
{
	using type = Arg<ArgRawDef<64>, ArgRawDef<32>, ArgRawDef<16>, ArgRawDef<8>, ArgRawDef<4>, ArgRawDef<0>>;
};


template<typename T>
struct ArgSelector<T*>
{
	using type = Arg<ArgRegDef<T*>, ArgValueDef<T*>, ArgNullDef<T*>>;
};

template<typename T>
struct ArgSelector<T*&>
{
	using type = Arg<ArgRegDef<T*&>>;
};

template<typename T>
struct ArgSelector<const T*>
{
	using type = Arg<ArgRegDef<const T*>, ArgValueDef<const T*>, ArgNullDef<const T*>>;
};

template<typename T>
struct ArgSelector<const T*&>
{
	using type = Arg<ArgRegDef<const T*&>>;
};


template<>
struct ArgSelector<ScriptNull>
{
	using type = Arg<ArgNullDef<ScriptNull>>;
};

template<>
struct ArgSelector<ScriptArgSeparator>
{
	using type = Arg<ArgSepDef>;
};



template<typename T>
struct GetArgsImpl;

template<typename... Args>
struct GetArgsImpl<RetEnum(Args...)>
{
	using type = ArgColection<sizeof...(Args), typename ArgSelector<Args>::type...>;
};

template<typename Func>
using GetArgs = typename GetArgsImpl<decltype(Func::func)>::type;

////////////////////////////////////////////////////////////
//				FuncVer and FuncGroup class
////////////////////////////////////////////////////////////

template<typename Func, int Ver, typename PosList = MakeListTag<GetArgs<Func>::arg()>>
struct FuncVer;

template<typename Func, int Ver, int... Pos>
struct FuncVer<Func, Ver, ListTag<Pos...>>
{
	using Args = GetArgs<Func>;
	static constexpr int offset = Args::offset(Ver, Args::arg());

	template<int CurrPos>
	using GetTypeAt = GetType<GetType<Args, CurrPos>, Args::pos(Ver, CurrPos)>;

	template<int CurrPos>
	[[gnu::always_inline]]
	static typename GetTypeAt<CurrPos>::ReturnType get(ScriptWorkerBase& sw, const Uint8* procArgs, ProgPos& curr)
	{
		constexpr int offs = Args::offset(Ver, CurrPos);
		return GetTypeAt<CurrPos>::get(sw, procArgs + offs, curr);
	}

	[[gnu::always_inline]]
	static RetEnum func(ScriptWorkerBase& sw, const Uint8* procArgs, ProgPos& curr)
	{
		return Func::func(get<Pos>(sw, procArgs, curr)...);
	}
};

template<typename Func, typename VerList = MakeListTag<GetArgs<Func>::ver()>>
struct FuncGroup;

template<typename Func, int... Ver>
struct FuncGroup<Func, ListTag<Ver...>> : GetArgs<Func>
{
	using FuncList = SumList<FuncVer<Func, Ver>...>;
	using GetArgs<Func>::ver;
	using GetArgs<Func>::arg;
	using GetArgs<Func>::parse;
	using GetArgs<Func>::overloadType;

	static constexpr ScriptFunc getDynamic(int i) { return FuncList::getDynamic(i); }
};

////////////////////////////////////////////////////////////
//					Bind helper classes
////////////////////////////////////////////////////////////

template<auto F>
struct WarpValue
{
	/// Default case, not normal pointer, pass without change
	template<typename T>
	static auto optDeref(T t) { return t; }

	/// We have normal pointer, dereference it to get value
	template<typename T>
	static auto optDeref(T* t) -> T& { return *t; }

	/// Get value from template parameter
	constexpr static auto val() { return optDeref(F); }

	/// Type of returned value
	using Type = decltype(val());
};

template<typename Ptr, typename... Rest>
struct BindMemberInvokeImpl //Work araound ICC 19.0.1 bug
{
	template<typename T, typename... TRest>
	static auto f(T&& a, TRest&&... b) -> decltype(auto)
	{
		if constexpr (sizeof...(Rest) > 0)
		{
			return BindMemberInvokeImpl<Rest...>::f(
				std::invoke(Ptr::val(), std::forward<T>(a), std::forward<TRest>(b)...)
			);
		}
		else
		{
			return std::invoke(Ptr::val(), std::forward<T>(a), std::forward<TRest>(b)...);
		}
	}
};

template<auto... Rest>
struct BindMemberInvoke : BindMemberInvokeImpl<WarpValue<Rest>...>
{

};

template<typename T, auto... Rest>
using BindMemberFinalType = std::decay_t<decltype(BindMemberInvoke<Rest...>::f(std::declval<T>()))>;




template<typename T>
struct BindMemberOverrideUnsuportedTypeImpl
{
	using Type = T;
};

template<>
struct BindMemberOverrideUnsuportedTypeImpl<bool>
{
	using Type = int;
};

template<>
struct BindMemberOverrideUnsuportedTypeImpl<Uint8>
{
	using Type = int;
};

template<>
struct BindMemberOverrideUnsuportedTypeImpl<Sint8>
{
	using Type = int;
};

template<>
struct BindMemberOverrideUnsuportedTypeImpl<Uint16>
{
	using Type = int;
};

template<>
struct BindMemberOverrideUnsuportedTypeImpl<Sint16>
{
	using Type = int;
};

template<typename T>
using BindMemberOverrideUnsuportedType = typename BindMemberOverrideUnsuportedTypeImpl<T>::Type;




template<typename T, typename... Args>
struct BindConstructor
{
	static RetEnum func(T& t, Args... args)
	{
		t = T{ std::forward<Args>(args)... };
		return RetContinue;
	}
};

template<typename T>
struct BindSet
{
	static RetEnum func(T& t, T r)
	{
		t = r;
		return RetContinue;
	}
};
template<typename T>
struct BindSwap
{
	static RetEnum func(T& t, T& r)
	{
		std::swap(t, r);
		return RetContinue;
	}
};
template<typename T>
struct BindEq
{
	static RetEnum func(ProgPos& Prog, T t1, T t2, ProgPos LabelTrue, ProgPos LabelFalse)
	{
		Prog = (t1 == t2) ? LabelTrue : LabelFalse;
		return RetContinue;
	}
};
template<typename T>
struct BindClear
{
	static RetEnum func(T& t)
	{
		t = T{};
		return RetContinue;
	}
};


template<typename T, auto... X>
struct BindPropGet
{
	static RetEnum func(const T* t, BindMemberOverrideUnsuportedType<BindMemberFinalType<T, X...>>& p)
	{
		if (t) p = BindMemberInvoke<X...>::f(t); else p = {};
		return RetContinue;
	}
};
template<typename T, auto... X>
struct BindPropSet
{
	static RetEnum func(T* t, BindMemberOverrideUnsuportedType<BindMemberFinalType<T, X...>> p)
	{
		if (t) BindMemberInvoke<X...>::f(t) = p;
		return RetContinue;
	}
};
template<typename T, auto... X>
struct BindPropAdd
{
	static RetEnum func(T* t, BindMemberOverrideUnsuportedType<BindMemberFinalType<T, X...>> p)
	{
		if (t) BindMemberInvoke<X...>::f(t) += p;
		return RetContinue;
	}
};


template<typename T, auto... X>
struct BindValuePropGet
{
	static RetEnum func(T t, BindMemberOverrideUnsuportedType<BindMemberFinalType<T, X...>>& p)
	{
		p = BindMemberInvoke<X...>::f(&t);
		return RetContinue;
	}
};
template<typename T, auto... X>
struct BindValuePropSet
{
	static RetEnum func(T& t, BindMemberOverrideUnsuportedType<BindMemberFinalType<T, X...>> p)
	{
		BindMemberInvoke<X...>::f(&t) = p;
		return RetContinue;
	}
};
template<typename T, auto... X>
struct BindValuePropAdd
{
	static RetEnum func(T& t, BindMemberOverrideUnsuportedType<BindMemberFinalType<T, X...>> p)
	{
		BindMemberInvoke<X...>::f(&t) += p;
		return RetContinue;
	}
};


template<typename T, auto... X>
struct BindPropCustomGet
{
	static RetEnum func(const T* t, int& p, typename BindMemberFinalType<T, X...>::Tag st)
	{
		if (t) p = BindMemberInvoke<X...>::f(t).get(st); else p = int{};
		return RetContinue;
	}
};
template<typename T, auto... X>
struct BindPropCustomSet
{
	static RetEnum func(T* t, typename BindMemberFinalType<T, X...>::Tag st, int p)
	{
		if (t) BindMemberInvoke<X...>::f(t).set(st, p);
		return RetContinue;
	}
};

template<typename T, typename P, P I>
struct BindConst
{
	static RetEnum func(T* t, P& p)
	{
		if (t) p = I; else p = P{};
		return RetContinue;
	}
};




template<typename T, std::string (*X)(const T*)>
struct BindDebugDisplay
{
	static RetEnum func(ScriptWorkerBase& swb, const T* t)
	{
		auto f = std::bind(X, t);
		swb.log_buffer_add(&f);
		return RetContinue;
	}
};

template<typename T, std::string (*X)(const T*)>
struct BindValueDebugDisplay
{
	static RetEnum func(ScriptWorkerBase& swb, T t)
	{
		auto f = std::bind(X, &t);
		swb.log_buffer_add(&f);
		return RetContinue;
	}
};




template<typename T, T X>
struct BindFuncImpl;

template<typename... Args, void(*X)(Args...)>
struct BindFuncImpl<void(*)(Args...), X>
{
	static RetEnum func(Args... a)
	{
		X(std::forward<Args>(a)...);
		return RetContinue;
	}
};

template<typename T, typename... Args, bool(T::*X)(Args...)>
struct BindFuncImpl<bool(T::*)(Args...), X>
{
	static RetEnum func(T* t, int& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = 0;
		return RetContinue;
	}
};

template<typename T, typename... Args, bool(T::*X)(Args...) const>
struct BindFuncImpl<bool(T::*)(Args...) const, X>
{
	static RetEnum func(const T* t, int& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = 0;
		return RetContinue;
	}
};

template<typename T, typename R, typename... Args, R(T::*X)(Args...)>
struct BindFuncImpl<R(T::*)(Args...), X>
{
	static RetEnum func(T* t, R& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = {};
		return RetContinue;
	}
};

template<typename T, typename R, typename... Args, R(T::*X)(Args...) const>
struct BindFuncImpl<R(T::*)(Args...) const, X>
{
	static RetEnum func(const T* t, R& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = {};
		return RetContinue;
	}
};

template<typename T, typename P, typename... Args, P*(T::*X)(Args...)>
struct BindFuncImpl<P*(T::*)(Args...), X>
{
	static RetEnum func(T* t, P*& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = {};
		return RetContinue;
	}
};

template<typename T, typename P, typename... Args, P*(T::*X)(Args...) const>
struct BindFuncImpl<P*(T::*)(Args...) const, X>
{
	static RetEnum func(const T* t, const P*& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = {};
		return RetContinue;
	}
};

template<typename T, typename... Args, void(T::*X)(Args...)>
struct BindFuncImpl<void(T::*)(Args...), X>
{
	static RetEnum func(T* t, Args... a)
	{
		if (t) (t->*X)(std::forward<Args>(a)...);
		return RetContinue;
	}
};

template<auto F>
struct BindFunc : BindFuncImpl<decltype(F), F> //Work araound ICC 19.0.1 bug
{

};


template<typename T, auto Func, auto... X>
struct BindListInitImpl
{
	static RetEnum func() = delete;
};

template<typename T, typename V, typename... Args, auto... X, bool (*Func)(T*, V* v, Args...)>
struct BindListInitImpl<bool (*)(T*, V* v, Args...), Func, X...>
{
	static RetEnum func(T* t, Args... args, ScriptArgSeparator, int& curr, int& limit)
	{
		if (t)
		{
			auto& obj = BindMemberInvoke<X...>::f(t);
			curr = 0;
			limit = std::size(obj);
			for (auto* u : obj)
			{
				if (Func(t, u, std::forward<Args>(args)...))
				{
					break;
				}
				++curr;
			}
		}
		else
		{
			curr = 0;
			limit = 0;
		}
		return RetContinue;
	}
};
template<auto Func, auto... X>
struct BindListInit : BindListInitImpl<decltype(Func), Func, X...>
{

};

template<typename T, auto Func, auto... X>
struct BindListLoopImpl
{
	static RetEnum func() = delete;
};

template<typename T, typename V, typename... Args, auto... X, bool (*Func)(T*, V* v, Args...)>
struct BindListLoopImpl<bool (*)(T*, V* v, Args...), Func, X...>
{
	static RetEnum func(T* t, Args... args, ScriptArgSeparator, int& curr, int& limit, ScriptArgSeparator, V*& r)
	{
		if (t)
		{
			auto& obj = BindMemberInvoke<X...>::f(t);
			size_t l = std::size(obj);
			if ((size_t)curr < l)
			{
				r = obj[curr];
			}
			else
			{
				r = nullptr;
			}
			++curr;
			for (;(size_t)curr < l; ++curr)
			{
				if (Func(t, obj[curr], std::forward<Args>(args)...))
				{
					break;
				}
			}
		}
		else
		{
			r = nullptr;
			curr = 0;
			limit = 0;
		}
		return RetContinue;
	}
};

template<auto Func, auto... X>
struct BindListLoop : BindListLoopImpl<decltype(Func), Func, X...>
{

};




template<typename T, T X>
struct BindValueFuncImpl;

template<typename... Args, void(*X)(Args...)>
struct BindValueFuncImpl<void(*)(Args...), X>
{
	static RetEnum func(Args... a)
	{
		X(std::forward<Args>(a)...);
		return RetContinue;
	}
};

template<typename T, typename... Args, bool(T::*X)(Args...)>
struct BindValueFuncImpl<bool(T::*)(Args...), X>
{
	static RetEnum func(T& t, int& r, Args... a)
	{
		r = (t.*X)(std::forward<Args>(a)...);
		return RetContinue;
	}
};

template<typename T, typename... Args, bool(T::*X)(Args...) const>
struct BindValueFuncImpl<bool(T::*)(Args...) const, X>
{
	static RetEnum func(T t, int& r, Args... a)
	{
		r = (t.*X)(std::forward<Args>(a)...);
		return RetContinue;
	}
};

template<typename T, typename R, typename... Args, R(T::*X)(Args...)>
struct BindValueFuncImpl<R(T::*)(Args...), X>
{
	static RetEnum func(T& t, R& r, Args... a)
	{
		r = (t.*X)(std::forward<Args>(a)...);
		return RetContinue;
	}
};

template<typename T, typename R, typename... Args, R(T::*X)(Args...) const>
struct BindValueFuncImpl<R(T::*)(Args...) const, X>
{
	static RetEnum func(T t, R& r, Args... a)
	{
		r = (t.*X)(std::forward<Args>(a)...);
		return RetContinue;
	}
};

template<typename T, typename P, typename... Args, P*(T::*X)(Args...)>
struct BindValueFuncImpl<P*(T::*)(Args...), X>
{
	/// Value should not return any pointers, this would allow dangling pointers
	static RetEnum func(T& t, P*& r, Args... a) = delete;
};

template<typename T, typename P, typename... Args, P*(T::*X)(Args...) const>
struct BindValueFuncImpl<P*(T::*)(Args...) const, X>
{
	/// Value should not return any pointers, this would allow dangling pointers
	static RetEnum func(T t, const P*& r, Args... a) = delete;
};

template<typename T, typename... Args, void(T::*X)(Args...)>
struct BindValueFuncImpl<void(T::*)(Args...), X>
{
	static RetEnum func(T& t, Args... a)
	{
		(t.*X)(std::forward<Args>(a)...);
		return RetContinue;
	}
};

template<auto F>
struct BindValueFunc : BindValueFuncImpl<decltype(F), F> //Work araound ICC 19.0.1 bug
{

};

} //namespace helper

////////////////////////////////////////////////////////////
//						Bind class
////////////////////////////////////////////////////////////

struct BindBase
{
	constexpr static const char functionWithoutDescription[] = "-";
	constexpr static const char functionInvisible[] = "";

	/// Tag type to choose allowed operations
	struct SetAndGet{};
	/// Tag type to choose allowed operations
	struct OnlyGet{};
	/// Tag type to skip adding default operations
	struct ExtensionBinding{ explicit ExtensionBinding() = default; };

	ScriptParserBase* parser;
	BindBase(ScriptParserBase* p) : parser{ p }
	{

	}

	template<typename X>
	void addCustomFunc(const std::string& name, const std::string& description = functionWithoutDescription)
	{
		parser->addParser<helper::FuncGroup<X>>(name, description);
	}
	void addCustomConst(const std::string& name, int i)
	{
		parser->addConst(name, i);
	}
	template<typename T>
	void addCustomPtr(const std::string& name, T* p)
	{
		parser->addConst(name, p);
	}
};

template<typename T>
struct Bind : BindBase
{
	using Type = T;

	std::string prefix;

	Bind(ScriptParserBase* p) : Bind{ p, T::ScriptName }
	{

	}

	Bind(ScriptParserBase* p, ExtensionBinding e) : Bind{ p, T::ScriptName, e }
	{

	}

	Bind(ScriptParserBase* p, std::string r) : BindBase{ p }, prefix{ std::move(r) }
	{
		parser->addParser<helper::FuncGroup<helper::BindSet<T*>>>("set", BindBase::functionInvisible);
		parser->addParser<helper::FuncGroup<helper::BindSet<const T*>>>("set", BindBase::functionInvisible);
		parser->addParser<helper::FuncGroup<helper::BindSwap<T*>>>("swap", BindBase::functionInvisible);
		parser->addParser<helper::FuncGroup<helper::BindSwap<const T*>>>("swap", BindBase::functionInvisible);
		parser->addParser<helper::FuncGroup<helper::BindClear<T*>>>("clear", BindBase::functionInvisible);
		parser->addParser<helper::FuncGroup<helper::BindClear<const T*>>>("clear", BindBase::functionInvisible);
		parser->addParser<helper::FuncGroup<helper::BindEq<const T*>>>("test_eq", BindBase::functionInvisible);
	}

	Bind(ScriptParserBase* p, std::string r, ExtensionBinding) : BindBase{ p }, prefix{ std::move(r) }
	{
		// no default operations defined!
	}


	std::string getName(const std::string& s)
	{
		return prefix + "." + s;
	}


	template<typename X>
	void addFunc(const std::string& name)
	{
		addCustomFunc<X>(getName(name));
	}
	template<typename X>
	void addFunc(const std::string& name, const std::string& description)
	{
		addCustomFunc<X>(getName(name), description);
	}


	template<auto MemPtr0, auto... MemPtrR>
	void addField(const std::string& getFuncName)
	{
		addCustomFunc<helper::BindPropGet<T, MACRO_CLANG_AUTO_HACK(MemPtr0), MACRO_CLANG_AUTO_HACK(MemPtrR)...>>(getName(getFuncName), "Get field of " + prefix);
	}
	template<auto MemPtr0, auto... MemPtrR>
	void addField(const std::string& getFuncName, const std::string& setFuncName)
	{
		addField<MemPtr0, MemPtrR...>(getFuncName);
		addCustomFunc<helper::BindPropSet<T, MACRO_CLANG_AUTO_HACK(MemPtr0), MACRO_CLANG_AUTO_HACK(MemPtrR)...>>(getName(setFuncName), "Set field of " + prefix);
	}
	template<auto MemPtr0, auto... MemPtrR>
	void addField(const std::string& getFuncName, const std::string& setFuncName, const std::string& addFuncName)
	{
		addField<MemPtr0, MemPtrR...>(getFuncName, setFuncName);
		addCustomFunc<helper::BindPropAdd<T, MACRO_CLANG_AUTO_HACK(MemPtr0), MACRO_CLANG_AUTO_HACK(MemPtrR)...>>(getName(addFuncName), "Add to field of " + prefix);
	}


	template<typename TagValues = ScriptValues<T>, typename Parent = typename TagValues::Parent*>
	void addScriptTag()
	{
		using Tag = typename TagValues::Tag;
		parser->getGlobal()->addTagType<Tag>();
		if (!parser->haveType<Tag>())
		{
			const ScriptTypeData* conf = parser->getType(ScriptParserBase::getArgType<Parent>());
			if (conf == nullptr)
			{
				throw Exception("Errow with adding script tag to unknown type");
			}
			parser->addType<Tag>(conf->name.toString() + ".Tag");
			parser->addParser<helper::FuncGroup<helper::BindSet<Tag>>>("set", BindBase::functionInvisible);
			parser->addParser<helper::FuncGroup<helper::BindSwap<Tag>>>("swap", BindBase::functionInvisible);
			parser->addParser<helper::FuncGroup<helper::BindClear<Tag>>>("clear", BindBase::functionInvisible);
			parser->addParser<helper::FuncGroup<helper::BindEq<Tag>>>("test_eq", BindBase::functionInvisible);
		}
	}
	template<auto MemPtr0, auto... MemPtrR>
	void addScriptValue()
	{
		return addScriptValue<BindBase::SetAndGet, MemPtr0, MemPtrR...>();
	}
	template<typename canEdit, auto MemPtr0, auto... MemPtrR>
	void addScriptValue()
	{
		using TagValues = helper::BindMemberFinalType<T, MACRO_CLANG_AUTO_HACK(MemPtr0), MACRO_CLANG_AUTO_HACK(MemPtrR)...>;
		addScriptTag<TagValues>();
		addCustomFunc<helper::BindPropCustomGet<T, MACRO_CLANG_AUTO_HACK(MemPtr0), MACRO_CLANG_AUTO_HACK(MemPtrR)...>>(getName("getTag"), "Get tag of " + prefix);
		if constexpr (std::is_same_v<canEdit, BindBase::SetAndGet>)
		{
			addCustomFunc<helper::BindPropCustomSet<T, MACRO_CLANG_AUTO_HACK(MemPtr0), MACRO_CLANG_AUTO_HACK(MemPtrR)...>>(getName("setTag"), "Set tag of " + prefix);
		}
		else
		{
			static_assert(std::is_same_v<canEdit, BindBase::OnlyGet>, "You can only use OnlyGet or SetAndGet types");
		}
	}

	template<std::string (*X)(const T*)>
	void addDebugDisplay()
	{
		addCustomFunc<helper::BindDebugDisplay<T, X>>("debug_impl", BindBase::functionInvisible);
	}

	template<auto X>
	void addFreeFunction(const std::string& func, const std::string& description = BindBase::functionWithoutDescription)
	{
		addCustomFunc<helper::BindFunc<MACRO_CLANG_AUTO_HACK(X)>>(func, description);
	}

	template<int X>
	void addFake(const std::string& get)
	{
		addCustomFunc<helper::BindConst<T, int, X>>(getName(get), "Get int field of " + prefix);
	}

	template<typename P, P* (T::*X)(), const P* (T::*Y)() const>
	void addPair(const std::string& get)
	{
		add<X>(get);
		add<Y>(get);
	}
	template<typename P, void (*X)(T*, P*&), void (*Y)(const T*, const P*&)>
	void addPair(const std::string& get)
	{
		add<X>(get);
		add<Y>(get);
	}

	template<typename P, const P* (T::*Y)() const>
	void addRules(const std::string& get)
	{
		add<Y>(get);
	}

	template<auto Func, auto... MemPtrR>
	void addList(const std::string& name, const std::string& description = BindBase::functionWithoutDescription)
	{
		addCustomFunc<helper::BindListInit<MACRO_CLANG_AUTO_HACK(Func), MACRO_CLANG_AUTO_HACK(MemPtrR)...>>(getName(name) + ".init", BindBase::functionInvisible);
		addCustomFunc<helper::BindListLoop<MACRO_CLANG_AUTO_HACK(Func), MACRO_CLANG_AUTO_HACK(MemPtrR)...>>(getName(name) + ".list", description);
	}

	template<auto X>
	void add(const std::string& func, const std::string& description = BindBase::functionWithoutDescription)
	{
		addCustomFunc<helper::BindFunc<MACRO_CLANG_AUTO_HACK(X)>>(getName(func), description);
	}
	template<typename TX, TX X>
	void add(const std::string& func, const std::string& description = BindBase::functionWithoutDescription)
	{
		addCustomFunc<helper::BindFunc<MACRO_CLANG_AUTO_HACK(X)>>(getName(func), description);
	}
	template<auto MemPtr0, auto MemPtr1, auto... MemPtrR>
	void add(const std::string& func, const std::string& description = BindBase::functionWithoutDescription)
	{
		addCustomFunc<helper::BindPropGet<T, MACRO_CLANG_AUTO_HACK(MemPtr0), MACRO_CLANG_AUTO_HACK(MemPtr1), MACRO_CLANG_AUTO_HACK(MemPtrR)...>>(getName(func), description);
	}
};

template<typename T>
struct BindValue : BindBase
{
	using Type = T;

	std::string prefix;

	BindValue(ScriptParserBase* p) : BindValue{ p, T::ScriptName }
	{

	}

	BindValue(ScriptParserBase* p, ExtensionBinding e) : BindValue{ p, T::ScriptName, e }
	{

	}

	BindValue(ScriptParserBase* p, std::string r) : BindBase{ p }, prefix{ std::move(r) }
	{
		parser->addParser<helper::FuncGroup<helper::BindSet<T>>>("set", BindBase::functionInvisible);
		parser->addParser<helper::FuncGroup<helper::BindSwap<T>>>("swap", BindBase::functionInvisible);
		parser->addParser<helper::FuncGroup<helper::BindClear<T>>>("clear", BindBase::functionInvisible);

		parser->addParser<helper::FuncGroup<helper::BindEq<T>>>("test_eq", BindBase::functionInvisible);
	}

	BindValue(ScriptParserBase* p, std::string r, ExtensionBinding) : BindBase{ p }, prefix{ std::move(r) }
	{
		// no default operations defined!
	}


	std::string getName(const std::string& s)
	{
		return prefix + "." + s;
	}


	template<typename X>
	void addFunc(const std::string& name)
	{
		addCustomFunc<X>(getName(name));
	}
	template<typename X>
	void addFunc(const std::string& name, const std::string& description)
	{
		addCustomFunc<X>(getName(name), description);
	}


	template<auto MemPtr0, auto... MemPtrR>
	void addField(const std::string& getFuncName)
	{
		addCustomFunc<helper::BindValuePropGet<T, MACRO_CLANG_AUTO_HACK(MemPtr0), MACRO_CLANG_AUTO_HACK(MemPtrR)...>>(getName(getFuncName), "Get field of " + prefix);
	}
	template<auto MemPtr0, auto... MemPtrR>
	void addField(const std::string& getFuncName, const std::string& setFuncName)
	{
		addField<MemPtr0, MemPtrR...>(getFuncName);
		addCustomFunc<helper::BindValuePropSet<T, MACRO_CLANG_AUTO_HACK(MemPtr0),  MACRO_CLANG_AUTO_HACK(MemPtrR)...>>(getName(setFuncName), "Set field of " + prefix);
	}
	template<auto MemPtr0, auto... MemPtrR>
	void addField(const std::string& getFuncName, const std::string& setFuncName, const std::string& addFuncName)
	{
		addField<MemPtr0, MemPtrR...>(getFuncName, setFuncName);
		addCustomFunc<helper::BindValuePropAdd<T, MACRO_CLANG_AUTO_HACK(MemPtr0), MACRO_CLANG_AUTO_HACK(MemPtrR)...>>(getName(addFuncName), "Add to field of " + prefix);
	}


	template<std::string (*X)(const T*)>
	void addDebugValueDisplay()
	{
		addCustomFunc<helper::BindValueDebugDisplay<T, X>>("debug_impl", BindBase::functionInvisible);
	}

	template<auto X>
	void addFreeFunction(const std::string& func, const std::string& description = BindBase::functionWithoutDescription)
	{
		addCustomFunc<helper::BindValueFunc<MACRO_CLANG_AUTO_HACK(X)>>(func, description);
	}

	template<auto X>
	void add(const std::string& func, const std::string& description = BindBase::functionWithoutDescription)
	{
		addCustomFunc<helper::BindValueFunc<MACRO_CLANG_AUTO_HACK(X)>>(getName(func), description);
	}
	template<typename TX, TX X>
	void add(const std::string& func, const std::string& description = BindBase::functionWithoutDescription)
	{
		addCustomFunc<helper::BindValueFunc<MACRO_CLANG_AUTO_HACK(X)>>(getName(func), description);
	}

	template<typename... Args>
	void addConstructor(const std::string& description = BindBase::functionWithoutDescription)
	{
		addCustomFunc<helper::BindConstructor<T, Args...>>("set", description);
	}
};

} //namespace OpenXcom
