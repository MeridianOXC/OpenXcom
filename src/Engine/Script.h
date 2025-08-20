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
#include <map>
#include <limits>
#include <vector>
#include <string>
#include <cstring>
#include "../Engine/Yaml.h"
#include <SDL_stdinc.h>
#include <cassert>

#include "HelperMeta.h"
#include "Logger.h"
#include "Exception.h"
#include "GraphSubset.h"
#include "Functions.h"


namespace OpenXcom
{
//for Surface.h
class Surface;

//for Script.h
class ScriptGlobal;
class ScriptParserBase;
class ScriptParserEventsBase;
class ScriptContainerBase;
class ScriptContainerEventsBase;

struct ParserWriter;
class SelectedToken;
class ScriptWorkerBase;
class ScriptWorkerBlit;
template<typename, typename...> class ScriptWorker;
template<typename, typename> struct ScriptTag;
template<typename, typename> class ScriptValues;

enum RegEnum : Uint16;
enum RetEnum : Uint8;
enum class ProgPos : size_t;

//for ScriptBind.h
struct BindBase;
template<typename T> struct Bind;
template<typename T, typename N, N T::*X> struct BindNested;

namespace helper
{

template<typename T>
struct ArgSelector;

template<typename Z>
struct ArgName
{
	ArgName(const char *n) : name{ n } { }

	const char *name;
};

}

constexpr size_t ScriptMaxOut = 9;
constexpr size_t ScriptMaxArg = 16;
constexpr size_t ScriptMaxReg = 64*sizeof(void*);

////////////////////////////////////////////////////////////
//					script base types
////////////////////////////////////////////////////////////

/**
 * Script null type, alias to std nullptr.
 */
using ScriptNull = std::nullptr_t;

/**
 * Script numeric type, alias to int.
 */
using ScriptInt = Sint32;

/**
 * Script const text, always zero terminated.
 */
struct ScriptText
{
	const char* ptr;

	operator std::string()
	{
		return ptr ? ptr : "";
	}

	const static ScriptText empty;
};

const inline ScriptText ScriptText::empty = { "" };


using ScriptFunc = RetEnum (*)(ScriptWorkerBase&, const Uint8*, ProgPos&);

/**
 * Script execution counter.
 */
enum class ProgPos : size_t
{
	Unknown = (size_t)-1,
	Start = 0,
};

inline ProgPos& operator+=(ProgPos& pos, int offset)
{
	pos = static_cast<ProgPos>(static_cast<size_t>(pos) + offset);
	return pos;
}
inline ProgPos& operator++(ProgPos& pos)
{
	pos += 1;
	return pos;
}
inline ProgPos operator++(ProgPos& pos, int)
{
	ProgPos old = pos;
	++pos;
	return old;
}

/**
 * Dummy type used to separate function args groups.
 */
class ScriptArgSeparator
{
};

////////////////////////////////////////////////////////////
//					enum definitions
////////////////////////////////////////////////////////////

/**
 * Base type for Arg enum.
 */
using ArgEnumBase = Uint16;

/**
 * Args special types.
 */
enum ArgSpecEnum : ArgEnumBase
{
	ArgSpecNone = 0x0,
	ArgSpecReg = 0x1,
	ArgSpecVar = 0x2 + ArgSpecReg,
	ArgSpecPtr = 0x4,
	ArgSpecPtrE = 0x8 + ArgSpecPtr,
	ArgSpecSize = 0x10,
};
constexpr ArgSpecEnum operator|(ArgSpecEnum a, ArgSpecEnum b)
{
	return static_cast<ArgSpecEnum>(static_cast<ArgEnumBase>(a) | static_cast<ArgEnumBase>(b));
}
constexpr ArgSpecEnum operator&(ArgSpecEnum a, ArgSpecEnum b)
{
	return static_cast<ArgSpecEnum>(static_cast<ArgEnumBase>(a) & static_cast<ArgEnumBase>(b));
}
constexpr ArgSpecEnum operator^(ArgSpecEnum a, ArgSpecEnum b)
{
	return static_cast<ArgSpecEnum>(static_cast<ArgEnumBase>(a) ^ static_cast<ArgEnumBase>(b));
}

/**
 * Args types.
 */
enum ArgEnum : ArgEnumBase
{
	ArgInvalid = ArgSpecSize * 0,
	ArgPlaceholder,
	ArgUnknowSimple,
	ArgUnknowSegment,
	ArgInvalidMax,

	ArgNull = ArgSpecSize * 1,
	ArgInt = ArgSpecSize * 2,
	ArgLabel = ArgSpecSize * 3,
	ArgText = ArgSpecSize * 4,
	ArgSep = ArgSpecSize * 5,

	ArgMax = ArgSpecSize * 6,
};

static_assert(ArgInvalidMax < ArgNull, "Overlap of different ranges");

/**
 * Next available value for arg type.
 */
constexpr ArgEnum ArgNext(ArgEnum arg)
{
	return static_cast<ArgEnum>(static_cast<ArgEnumBase>(arg) + static_cast<ArgEnumBase>(ArgSpecSize));
}

/**
 * Base version of argument type.
 */
constexpr ArgEnum ArgBase(ArgEnum arg)
{
	return static_cast<ArgEnum>((static_cast<ArgEnumBase>(arg) & ~(static_cast<ArgEnumBase>(ArgSpecSize) - 1)));
}
/**
 * Specialized version of argument type.
 */
constexpr ArgEnum ArgSpecAdd(ArgEnum arg, ArgSpecEnum spec)
{
	return ArgBase(arg) != ArgInvalid ? static_cast<ArgEnum>(static_cast<ArgEnumBase>(arg) | static_cast<ArgEnumBase>(spec)) : arg;
}
/**
 * Remove specialization from argument type.
 */
constexpr ArgEnum ArgSpecRemove(ArgEnum arg, ArgSpecEnum spec)
{
	return ArgBase(arg) != ArgInvalid ? static_cast<ArgEnum>(static_cast<ArgEnumBase>(arg) & ~static_cast<ArgEnumBase>(spec)) : arg;
}
/**
 * Test if argument type is register (readonly or writeable).
 */
constexpr bool ArgIsReg(ArgEnum arg)
{
	return ArgBase(arg) != ArgInvalid && (static_cast<ArgEnumBase>(arg) & static_cast<ArgEnumBase>(ArgSpecReg)) == static_cast<ArgEnumBase>(ArgSpecReg);
}
/**
 * Test if argument type is variable (writeable register).
 */
constexpr bool ArgIsVar(ArgEnum arg)
{
	return ArgBase(arg) != ArgInvalid && (static_cast<ArgEnumBase>(arg) & static_cast<ArgEnumBase>(ArgSpecVar)) == static_cast<ArgEnumBase>(ArgSpecVar);
}
/**
 * Test if argument type is pointer.
 */
constexpr bool ArgIsPtr(ArgEnum arg)
{
	return ArgBase(arg) != ArgInvalid && (static_cast<ArgEnumBase>(arg) & static_cast<ArgEnumBase>(ArgSpecPtr)) == static_cast<ArgEnumBase>(ArgSpecPtr);
}
/**
 * Test if argument type is editable pointer.
 */
constexpr bool ArgIsPtrE(ArgEnum arg)
{
	return ArgBase(arg) != ArgInvalid && (static_cast<ArgEnumBase>(arg) & static_cast<ArgEnumBase>(ArgSpecPtrE)) == static_cast<ArgEnumBase>(ArgSpecPtrE);
}
/**
 * Compatibility between operation argument type and variable type. Greater numbers mean bigger compatibility.
 * @param argType Type of operation argument.
 * @param varType Type of variable/value we try pass to operation.
 * @return Zero if incompatible, 255 if both types are same.
 */
constexpr int ArgCompatible(ArgEnum argType, ArgEnum varType, size_t overloadSize)
{
	return
		ArgBase(argType) == ArgInvalid ? 0 :
		ArgIsVar(argType) && argType != varType ? 0 :
		ArgBase(argType) != ArgBase(varType) ? 0 :
		ArgIsReg(argType) != ArgIsReg(varType) ? 0 :
		ArgIsPtr(argType) != ArgIsPtr(varType) ? 0 :
		ArgIsPtrE(argType) && !ArgIsPtrE(varType) ? 0 :
			255 - (ArgIsPtrE(argType) != ArgIsPtrE(varType) ? 128 : 0) - (ArgIsVar(argType) != ArgIsVar(varType) ? 64 : 0) - (overloadSize > 8 ? 8 : overloadSize);
}

/**
 * Function returning next unique value for ArgEnum.
 */
inline ArgEnum ArgNextUniqueValue()
{
	static ArgEnum curr = ArgMax;
	ArgEnum old = curr;
	curr = ArgNext(curr);
	return old;
}

/**
 * Function matching some Type to ArgEnum.
 */
template<typename T>
inline ArgEnum ArgRegisteType()
{
	static_assert(std::is_same<T, std::remove_pointer_t<std::decay_t<T>>>::value, "Only simple types are allowed");

	if (std::is_same<T, ScriptInt>::value)
	{
		return ArgInt;
	}
	else if (std::is_same<T, ScriptNull>::value)
	{
		return ArgNull;
	}
	else if (std::is_same<T, ScriptText>::value)
	{
		return ArgText;
	}
	else if (std::is_same<T, ProgPos>::value)
	{
		return ArgLabel;
	}
	else if (std::is_same<T, ScriptArgSeparator>::value)
	{
		return ArgSep;
	}
	else
	{
		static ArgEnum curr = ArgNextUniqueValue();
		return curr;
	}
}

/**
 * Available regs.
 */
enum RegEnum : Uint16
{
	RegInvalid = (Uint16)-1,

	RegStartPos = 0,
};

static_assert(ScriptMaxReg < RegInvalid, "RegInvalid could be interpreted as correct register");

/**
 * Return value from script operation.
 */
enum RetEnum : Uint8
{
	RetContinue = 0,
	RetEnd = 1,
	RetError = 2,
};

/**
 * Common type meta data.
 */
struct TypeInfo
{
	size_t size;
	size_t alignment;

	/// is type valid?
	constexpr explicit operator bool() const { return size; }

	/// get next valid position for type
	constexpr size_t nextRegPos(size_t prev) const
	{
		return ((prev + alignment - 1) & ~(alignment - 1));
	}
	/// get total space used by this type with considering alignment
	constexpr size_t needRegSpace(size_t prev) const
	{
		return nextRegPos(prev) + size;
	}

	/// default value for pointers
	constexpr static TypeInfo getPtrTypeInfo()
	{
		return TypeInfo
		{
			sizeof(void*),
			alignof(void*),
		};
	}
};

////////////////////////////////////////////////////////////
//				containers definitions
////////////////////////////////////////////////////////////

/**
 * Common base of script execution.
 */
class ScriptContainerBase
{
	friend struct ParserWriter;
	std::vector<Uint8> _proc;

public:
	/// Constructor.
	ScriptContainerBase() = default;
	/// Copy constructor.
	ScriptContainerBase(const ScriptContainerBase&) = delete;
	/// Move constructor.
	ScriptContainerBase(ScriptContainerBase&&) = default;

	/// Destructor.
	~ScriptContainerBase() = default;

	/// Copy.
	ScriptContainerBase &operator=(const ScriptContainerBase&) = delete;
	/// Move.
	ScriptContainerBase &operator=(ScriptContainerBase&&) = default;

	/// Test if is any script there.
	explicit operator bool() const
	{
		return !_proc.empty();
	}

	/// Get pointer to proc data.
	const Uint8* data() const
	{
		return *this ? _proc.data() : nullptr;
	}
};

/**
 * Strong typed script.
 */
template<typename Parent, typename... Args>
class ScriptContainer : public ScriptContainerBase
{
public:
	/// Load code from string in YAML node.
	void load(const std::string& parentName, const YAML::YamlNodeReader& reader, const Parent& parent)
	{
		parent.parseNode(*this, parentName, reader);
	}
	/// Load data from string.
	void load(const std::string& parentName, const std::string& srcCode, const Parent& parent)
	{
		parent.parseCode(*this, parentName, srcCode);
	}
};

/**
 * Common base of typed script with events.
 */
class ScriptContainerEventsBase
{
	friend class ScriptParserEventsBase;
	ScriptContainerBase _current;
	const ScriptContainerBase* _events = nullptr;

public:
	/// Test if is any script there.
	explicit operator bool() const
	{
		return true;
	}

	/// Get pointer to proc data.
	const Uint8* data() const
	{
		return _current.data();
	}
	/// Get pointer to proc data.
	const ScriptContainerBase* dataEvents() const
	{
		return _events;
	}
};

/**
 * Strong typed script with events.
 */
template<typename Parent, typename... Args>
class ScriptContainerEvents : public ScriptContainerEventsBase
{
public:
	/// Load code from string in YAML node.
	void load(const std::string& parentName, const YAML::YamlNodeReader& reader, const Parent& parent)
	{
		parent.parseNode(*this, parentName, reader);
	}
	/// Load data from string.
	void load(const std::string& parentName, const std::string& srcCode, const Parent& parent)
	{
		parent.parseCode(*this, parentName, srcCode);
	}
};

////////////////////////////////////////////////////////////
//					worker definition
////////////////////////////////////////////////////////////

namespace helper
{

template<int I, typename Tuple, bool valid = ((size_t)I < std::tuple_size<Tuple>::value)>
struct GetTupleImpl
{
	using type = typename std::tuple_element<I, Tuple>::type;
	static type get(Tuple& t) { return std::get<I>(t); }
};

template<int I, typename Tuple>
struct GetTupleImpl<I, Tuple, false>
{
	using type = void;
	static type get(Tuple& t) { }
};

template<int I, typename Tuple>
using GetTupleType = typename GetTupleImpl<I, Tuple>::type;

template<int I, typename Tuple>
GetTupleType<I, Tuple> GetTupleValue(Tuple& t) { return GetTupleImpl<I, Tuple>::get(t); }


/**
 * Helper class extracting needed data from type T
 */
template<typename T>
struct TypeInfoImpl
{
	using t1 = typename std::decay<T>::type;
	using t2 = typename std::remove_pointer<t1>::type;
	using t3 = typename std::decay<t2>::type;

	static constexpr bool isRef = std::is_reference<T>::value;
	static constexpr bool isOutput = isRef && !std::is_const<T>::value;
	static constexpr bool isPtr = std::is_pointer<t1>::value;
	static constexpr bool isEditable = isPtr && !std::is_const<t2>::value;

	enum
	{
		metaIsSimple = std::is_trivially_copyable_v<t3> && std::is_nothrow_default_constructible_v<t3> && std::is_trivially_destructible_v<t3>,
		metaDestSize = metaIsSimple ? sizeof(t3) : 0,
		metaDestAlign = metaIsSimple ? alignof(t3) : 0
	};

	/// meta data of destination type (without pointer), invalid if type is not POD
	static constexpr TypeInfo metaDest =
	{
		metaDestSize,
		metaDestAlign,
	};
	/// meta data of base type (with pointer if it is)
	static constexpr TypeInfo metaBase =
	{
		sizeof(t1),
		alignof(t1),
	};

	static_assert(+metaDestSize || isPtr, "Type need to be POD to be used as reg or const value.");
	static_assert((alignof(t1) & (alignof(t1) - 1)) == 0, "Type alignment is not power of two");
};

template<typename T>
constexpr TypeInfo TypeInfoImpl<T>::metaDest;
template<typename T>
constexpr TypeInfo TypeInfoImpl<T>::metaBase;

} //namespace helper

/**
 * Raw memory used by scripts.
 */
template<int size>
using ScriptRawMemory = typename std::aligned_storage<size, alignof(void*)>::type;

/**
 * Script output and input arguments.
 */
template<typename... OutputArgs>
struct ScriptOutputArgs
{
	std::tuple<helper::Decay<OutputArgs>...> data;

	/// Default constructor.
	ScriptOutputArgs() : data{ }
	{

	}

	/// Constructor.
	ScriptOutputArgs(const helper::Decay<OutputArgs>&... args) : data{ args... }
	{

	}

	/// Getter for first element.
	auto getFirst() -> helper::GetTupleType<0, decltype(data)> { return helper::GetTupleValue<0>(data); }
	/// Getter for second element.
	auto getSecond() -> helper::GetTupleType<1, decltype(data)> { return helper::GetTupleValue<1>(data); }
	/// Getter for third element.
	auto getThird() -> helper::GetTupleType<2, decltype(data)> { return helper::GetTupleValue<2>(data); }
};

/**
 * Specialization of script output and input arguments.
 */
template<>
struct ScriptOutputArgs<>
{
	std::tuple<> data;

	/// Default constructor.
	ScriptOutputArgs() : data{ }
	{

	}
};

/**
 * Class execute scripts and store its data.
 */
class ScriptWorkerBase
{
	std::string log_buffer;
	ScriptRawMemory<ScriptMaxReg> reg;

	static constexpr int RegSet = 1;
	static constexpr int RegNone = 0;
	static constexpr int RegGet = -1;


	template<typename>
	using SetAllRegs = helper::PosTag<RegSet>;

	template<typename T>
	using GetWritableRegs = helper::PosTag<helper::TypeInfoImpl<T>::isOutput ? RegGet : RegNone>;

	template<typename T>
	using SetReadonlyRegs = helper::PosTag<!helper::TypeInfoImpl<T>::isOutput ? RegSet : RegNone>;

	template<size_t BaseOffset, int I, typename... Args, typename T>
	void forRegImplLoop(helper::PosTag<RegSet>, const T& arg)
	{
		using CurrentType =helper::Decay<typename std::tuple_element<I, T>::type>;
		constexpr int CurrentOffset = offset<void, Args...>(I, BaseOffset);

		ref<CurrentType>(CurrentOffset) = std::get<I>(arg);
	}
	template<size_t BaseOffset, int I, typename... Args, typename T>
	void forRegImplLoop(helper::PosTag<RegNone>, const T& arg)
	{
		// nothing
	}
	template<size_t BaseOffset, int I, typename... Args, typename T>
	void forRegImplLoop(helper::PosTag<RegGet>, T& arg)
	{
		using CurrentType = helper::Decay<typename std::tuple_element<I, T>::type>;
		constexpr size_t CurrentOffset = offset<void, Args...>(I, BaseOffset);

		std::get<I>(arg) = ref<CurrentType>(CurrentOffset);
	}

	template<size_t BaseOffset, template<typename> class Filter, typename... Args, typename T, int... I>
	void forRegImpl(T&& arg, helper::ListTag<I...>)
	{
		(forRegImplLoop<BaseOffset, I, Args...>(Filter<Args>{}, std::forward<T>(arg)), ...);
	}

	template<size_t BaseOffset, template<typename> class Filter, typename... Args, typename T>
	void forReg(T&& arg)
	{
		forRegImpl<BaseOffset, Filter, Args...>(std::forward<T>(arg), helper::MakeListTag<sizeof...(Args)>{});
	}

	/// Count offset.
	template<typename, typename First, typename... Rest>
	static constexpr size_t offset(int i, size_t prevOffset)
	{
		using typeInfoImp = typename helper::TypeInfoImpl<First>;
		return offset<void, Rest...>(
			i - 1,
			(i > 0 ? typeInfoImp::metaBase.needRegSpace(prevOffset) :
			i == 0 ? typeInfoImp::metaBase.nextRegPos(prevOffset) :
				prevOffset)
		);
	}
	/// Final function of counting offset.
	template<typename>
	static constexpr size_t offset(int i, size_t prevOffset)
	{
		return prevOffset;
	}

	template<typename... Args>
	static constexpr size_t offsetOutput(helper::TypeTag<ScriptOutputArgs<Args...>>)
	{
		return offset<void, Args...>(sizeof...(Args), 0);
	}

protected:
	/// Update values in script.
	template<typename Output, typename... Args>
	void updateBase(Args... args)
	{
		memset(&reg, 0, ScriptMaxReg);
		forReg<offsetOutput(helper::TypeTag<Output>{}), SetAllRegs, Args...>(std::tie(args...));
	}

	template<typename... Args>
	void set(const ScriptOutputArgs<Args...>& arg)
	{
		forReg<0, SetAllRegs, Args...>(arg.data);
	}
	template<typename... Args>
	void get(ScriptOutputArgs<Args...>& arg)
	{
		forReg<0, GetWritableRegs, Args...>(arg.data);
	}
	template<typename... Args>
	void reset(const ScriptOutputArgs<Args...>& arg)
	{
		forReg<0, SetReadonlyRegs, Args...>(arg.data);
	}

	/// Call script.
	void executeBase(const Uint8* proc);

public:
	/// Default constructor.
	ScriptWorkerBase()
	{

	}

	/// Get value from reg.
	template<typename T>
	T& ref(size_t off)
	{
		return *reinterpret_cast<typename std::decay<T>::type*>(reinterpret_cast<char*>(&reg) + off);
	}
	/// Get value from proc vector.
	template<typename T>
	T const_val(const Uint8 *ptr, size_t off = 0)
	{
		T ret;
		std::memcpy(&ret, ptr + off, sizeof(T));
		return ret;
	}

	/// Add text to log buffer.
	void log_buffer_add(FuncRef<std::string()> func);
	/// Flush buffer to log file.
	void log_buffer_flush(ProgPos& p);
};

/**
 * Strong typed script executor base template.
 */
template<typename Output, typename... Args>
class ScriptWorker;

/**
 * Strong typed script executor.
 */
template<typename... OutputArgs, typename... Args>
class ScriptWorker<ScriptOutputArgs<OutputArgs...>, Args...> : public ScriptWorkerBase
{
public:
	/// Type of output value from script.
	using Output = ScriptOutputArgs<OutputArgs...>;

	/// Default constructor.
	ScriptWorker(Args... args) : ScriptWorkerBase()
	{
		updateBase<Output>(args...);
	}

	/// Execute standard script.
	template<typename Parent>
	void execute(const ScriptContainer<Parent, Args...>& c, Output& arg)
	{
		static_assert(std::is_same<typename Parent::Output, Output>::value, "Incompatible script output type");

		set(arg);
		executeBase(c.data());
		get(arg);
	}

	/// Execute standard script with global events.
	template<typename Parent>
	void execute(const ScriptContainerEvents<Parent, Args...>& c, Output& arg)
	{
		static_assert(std::is_same<typename Parent::Output, Output>::value, "Incompatible script output type");

		set(arg);
		auto ptr = c.dataEvents();
		if (ptr)
		{
			while (*ptr)
			{
				reset(arg);
				executeBase(ptr->data());
				++ptr;
			}
			++ptr;
		}
		reset(arg);
		executeBase(c.data());
		if (ptr)
		{
			while (*ptr)
			{
				reset(arg);
				executeBase(ptr->data());
				++ptr;
			}
		}
		get(arg);
	}
};

/**
 * Strong typed blit script executor.
 */
class ScriptWorkerBlit : public ScriptWorkerBase
{
	/// Current script set in worker.
	const Uint8* _proc;
	const ScriptContainerBase* _events;

public:
	/// Type of output value from script.
	using Output = ScriptOutputArgs<int&, int>;

	/// Default constructor.
	ScriptWorkerBlit() : ScriptWorkerBase(), _proc(nullptr), _events(nullptr)
	{

	}

	/// Update data from container script.
	template<typename Parent, typename... Args>
	void update(const ScriptContainer<Parent, Args...>& c, helper::Decay<Args>... args)
	{
		static_assert(std::is_same<typename Parent::Output, Output>::value, "Incompatible script output type");
		clear();
		if (c)
		{
			_proc = c.data();
			_events = nullptr;
			updateBase<Output>(args...);
		}
	}

	/// Update data from container script.
	template<typename Parent, typename... Args>
	void update(const ScriptContainerEvents<Parent, Args...>& c, helper::Decay<Args>... args)
	{
		static_assert(std::is_same<typename Parent::Output, Output>::value, "Incompatible script output type");
		clear();
		if (c)
		{
			_proc = c.data();
			_events = c.dataEvents();
			updateBase<Output>(args...);
		}
	}

	/// Programmable blitting using script.
	void executeBlit(const Surface* src, Surface* dest, int x, int y, int shade);
	/// Programmable blitting using script.
	void executeBlit(const Surface* src, Surface* dest, int x, int y, int shade, GraphSubset mask);

	/// Clear all worker data.
	void clear()
	{
		_proc = nullptr;
		_events = nullptr;
	}
};

////////////////////////////////////////////////////////////
//					objects ranges
////////////////////////////////////////////////////////////

/**
 * Range of values.
 */
template<typename T>
class ScriptRange
{
protected:
	using ptr = const T*;

	/// Pointer pointing place of first element.
	ptr _begin;
	/// pointer pointing place past of last element.
	ptr _end;

public:
	/// Default constructor.
	constexpr ScriptRange() noexcept : _begin{ nullptr }, _end{ nullptr }
	{

	}
	/// Constructor.
	constexpr ScriptRange(ptr b, ptr e) noexcept : _begin{ b }, _end{ e }
	{

	}

	/// Beginning of string range.
	constexpr ptr begin() const
	{
		return _begin;
	}
	/// End of string range.
	constexpr ptr end() const
	{
		return _end;
	}
	/// Size of string range.
	constexpr size_t size() const
	{
		return _end - _begin;
	}
	/// Bool operator.
	constexpr explicit operator bool() const
	{
		return _begin != _end;
	}
};

/**
 * Symbol in script.
 */
class ScriptRef : public ScriptRange<char>
{
public:
	/// Default constructor.
	constexpr ScriptRef() noexcept = default;

	/// Copy constructor.
	constexpr ScriptRef(const ScriptRef&) noexcept = default;

	/// Constructor from char array.
	template<int I>
	constexpr explicit ScriptRef(const char (&p)[I]) noexcept : ScriptRange{ p , p + I - 1 }
	{

	}

	/// Constructor from range of pointers.
	constexpr ScriptRef(ptr b, ptr e) noexcept : ScriptRange{ b, e }
	{

	}

	/// Copy assignment.
	constexpr ScriptRef& operator=(const ScriptRef& r) noexcept = default;



	/// Find first occurrence of character in string range.
	constexpr size_t find(char c) const
	{
		for (auto &curr : *this)
		{
			if (curr == c)
			{
				return &curr - _begin;
			}
		}
		return std::string::npos;
	}

	/// Return sub range of current range.
	constexpr ScriptRef substr(size_t p, size_t s = std::string::npos) const
	{
		const size_t totalSize = size();
		if (p >= totalSize)
		{
			return ScriptRef{ };
		}

		const auto b = _begin + p;
		if (s > totalSize - p)
		{
			return ScriptRef{ b, _end };
		}
		else
		{
			return ScriptRef{ b, b + s };
		}
	}

	constexpr ScriptRef head(size_t p) const
	{
		return substr(0, p);
	}

	constexpr ScriptRef tail(size_t p) const
	{
		return substr(p);
	}

	constexpr ScriptRef headFromEnd(size_t p) const
	{
		const size_t totalSize = size();
		if (p >= totalSize)
		{
			return *this;
		}

		return substr(totalSize - p);
	}

	constexpr ScriptRef tailFromEnd(size_t p) const
	{
		const size_t totalSize = size();
		if (p >= totalSize)
		{
			return ScriptRef{ };
		}

		return substr(0, totalSize - p);
	}

	/// Create string based on current range.
	std::string toString() const
	{
		return *this ? std::string(_begin, _end) : std::string{ };
	}

	/// Create temporary ref based on script.
	static ScriptRef tempFrom(const std::string& s)
	{
		return { s.data(), s.data() + s.size() };
	}

	/// Create pernamet ref based on script.
	constexpr static ScriptRef staticFrom(ptr p)
	{
		return { p, p + std::char_traits<char>::length(p) };
	}

	/// Compare two ranges.
	constexpr static int compare(ScriptRef a, ScriptRef b)
	{
		const auto size_a = a.size();
		const auto size_b = b.size();

		if (size_a == size_b)
		{
			//check for GCC warnings, it thinks that we try use extremely long strings there...
			assert(size_a < SIZE_MAX/2);

			return memcmp(a._begin, b._begin, size_a);
		}
		else if (size_a < size_b)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}

	/// Equal operator.
	constexpr bool operator==(const ScriptRef& s) const
	{
		return compare(*this, s) == 0;
	}

	/// Not-equal operator.
	constexpr bool operator!=(const ScriptRef& s) const
	{
		return compare(*this, s) != 0;
	}

	/// Less operator.
	constexpr bool operator<(const ScriptRef& s) const
	{
		return compare(*this, s) < 0;
	}
};

////////////////////////////////////////////////////////////
//					parser definitions
////////////////////////////////////////////////////////////

/**
 * Struct storing script type data.
 */
struct ScriptTypeData
{
	ScriptRef name;
	ArgEnum type;
	TypeInfo meta;
};

/**
 * Struct storing value used by script.
 */
struct ScriptValueData
{
	ScriptRawMemory<2*sizeof(void*)> data = { };
	ArgEnum type = ArgInvalid;
	Uint8 size = 0;

	/// Copy constructor.
	template<typename T>
	inline ScriptValueData(const T& t);
	/// Copy constructor.
	inline ScriptValueData(const ScriptValueData& t);
	/// Default constructor.
	inline ScriptValueData() { }

	/// Assign operator.
	template<typename T>
	inline ScriptValueData& operator=(const T& t);
	/// Assign operator.
	inline ScriptValueData& operator=(const ScriptValueData& t);

	/// Test if value have have selected type.
	template<typename T>
	inline bool isValueType() const;
	/// Get current stored value.
	template<typename T>
	inline const T& getValue() const;

	bool operator==(const ScriptValueData& other) const
	{
		return type == other.type && memcmp(&data, &other.data, size) == 0;
	}
	bool operator!=(const ScriptValueData& other) const
	{
		return !(*this == other);
	}
};

/**
 * Struct used to store named definition used by script.
 */
struct ScriptRefData
{
	ScriptRef name;
	ArgEnum type = ArgInvalid;
	ScriptValueData value;

	/// Default constructor.
	ScriptRefData() { }
	/// Constructor.
	ScriptRefData(ScriptRef n, ArgEnum t) : name{ n }, type{ t } {  }
	/// Constructor.
	ScriptRefData(ScriptRef n, ArgEnum t, ScriptValueData v) : name{ n }, type{ t }, value{ v } {  }

	/// Get true if this valid reference.
	explicit operator bool() const
	{
		return type != ArgInvalid;
	}

	template<typename T>
	bool isValueType() const
	{
		return value.isValueType<T>();
	}
	/// Get current stored value.
	template<typename T>
	const T& getValue() const
	{
		return value.getValue<T>();
	}
	/// Get current stored value if have that type or default value otherwise.
	template<typename T>
	const T& getValueOrDefulat(const T& def) const
	{
		return value.isValueType<T>() ? value.getValue<T>() : def;
	}
};

/**
 * Struct storing available operation to scripts.
 */
struct ScriptProcData
{
	using argFunc = int (*)(ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end);
	using getFunc = ScriptFunc (*)(int version);
	using parserFunc = bool (*)(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end);
	using overloadFunc = int (*)(const ScriptProcData& spd, const ScriptRefData* begin, const ScriptRefData* end);

	ScriptRef name;
	ScriptRef description;

	overloadFunc overload;
	ScriptRange<ScriptRange<ArgEnum>> overloadArg;

	parserFunc parser;
	argFunc parserArg;
	getFunc parserGet;

	bool operator()(ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end) const
	{
		return parser(*this, ph, begin, end);
	}
};

/**
 * Common base of script parser.
 */
class ScriptParserBase
{
	ScriptGlobal* _shared;
	bool _emptyReturn;
	size_t _regUsedSpace;
	Uint8 _regOutSize;
	ScriptRef _regOutName[ScriptMaxOut];
	std::string _name;
	std::string _defaultScript;
	std::string _description;
	std::vector<std::vector<char>> _strings;
	std::vector<ScriptTypeData> _typeList;
	std::vector<ScriptProcData> _procList;
	std::vector<ScriptRefData> _refList;

protected:
	template<typename First, typename... Rest>
	void addRegImpl(bool writable, helper::ArgName<First>& n, Rest&... t)
	{
		addTypeImpl(helper::TypeTag<helper::Decay<First>>{});
		addScriptReg(n.name, ScriptParserBase::getArgType<First>(), writable, helper::TypeInfoImpl<First>::isOutput);
		addRegImpl(writable, t...);
	}
	void addRegImpl(bool writable)
	{
		//end loop
	}

	/// Function for SFINAE, type need to have ScriptRegister function.
	template<typename First, typename = decltype(&First::ScriptRegister)>
	void addTypeImpl(helper::TypeTag<First*>)
	{
		registerPointerType<First>();
	}
	/// Function for SFINAE, type need to have ScriptRegister function.
	template<typename First, typename = decltype(&First::ScriptRegister)>
	void addTypeImpl(helper::TypeTag<const First*>)
	{
		registerPointerType<First>();
	}
	/// Function for SFINAE, type need to have ScriptRegister function.
	template<typename First, typename Index, typename = decltype(&First::ScriptRegister)>
	void addTypeImpl(helper::TypeTag<ScriptTag<First, Index>>)
	{
		registerPointerType<First>();
	}
	/// Basic version.
	template<typename First>
	void addTypeImpl(helper::TypeTag<First>)
	{
		//nothing to do for rest
	}

	/// Default constructor.
	ScriptParserBase(ScriptGlobal* shared, const std::string& name);
	/// Destructor.
	~ScriptParserBase();

	/// Common typeless part of parsing string.
	bool parseBase(ScriptContainerBase& scr, const std::string& parentName, const std::string& srcCode) const;

	/// Parse node and return new script.
	void parseNode(ScriptContainerBase& container, const std::string& parentName, const YAML::YamlNodeReader& reader) const;

	/// Parse string and return new script.
	void parseCode(ScriptContainerBase& container, const std::string& parentName, const std::string& srcCode) const;

	/// Test if name is free.
	bool haveNameRef(const std::string& s) const;
	/// Add new name that can be used in data lists.
	ScriptRef addNameRef(const std::string& s);

	/// Add name for custom parameter.
	void addScriptReg(const std::string& s, ArgEnum type, bool writableReg, bool outputReg);
	/// Add parsing function.
	void addParserBase(const std::string& s, const std::string& description, ScriptProcData::overloadFunc overload, ScriptRange<ScriptRange<ArgEnum>> overloadArg, ScriptProcData::parserFunc parser, ScriptProcData::argFunc parserArg, ScriptProcData::getFunc parserGet);
	/// Add new type implementation.
	void addTypeBase(const std::string& s, ArgEnum type, TypeInfo meta);
	/// Test if type was added implementation.
	bool haveTypeBase(ArgEnum type);
	/// Set default script for type.
	void setDefault(const std::string& s) { _defaultScript = s; }
	/// Set description for script.
	void setDescription(const std::string& s) { _description = s; }
	/// Set mode where return does not accept any value.
	void setEmptyReturn() { _emptyReturn = true; }

public:
	/// Register type to get run time value representing it.
	template<typename T>
	static ArgEnum getArgType()
	{
		using info = helper::TypeInfoImpl<T>;
		using t3 = typename info::t3;

		auto spec = ArgSpecNone;

		if (info::isRef) spec = spec | ArgSpecVar;
		if (info::isPtr) spec = spec | ArgSpecPtr;
		if (info::isEditable) spec = spec | ArgSpecPtrE;

		return ArgSpecAdd(ArgRegisteType<t3>(), spec);
	}
	/// Add const value.
	void addConst(const std::string& s, ScriptValueData i);
	/// Update const value.
	void updateConst(const std::string& s, ScriptValueData i);
	/// Add line parsing function.
	template<typename T>
	void addParser(const std::string& s, const std::string& description)
	{
		addParserBase(s, description, nullptr, T::overloadType(), nullptr, &T::parse, &T::getDynamic);
	}
	/// Test if type was already added.
	template<typename T>
	bool haveType()
	{
		return haveTypeBase(getArgType<T>());
	}
	/// Add new type.
	template<typename T>
	void addType(const std::string& s)
	{
		using info = helper::TypeInfoImpl<T>;
		using t3 = typename info::t3;

		addTypeBase(s, ArgRegisteType<t3>(), info::metaDest);
	}

	/// Register pointer type in parser.
	template<typename P>
	void registerPointerType()
	{
		if (!haveType<P*>())
		{
			addType<P*>(P::ScriptName);
			P::ScriptRegister(this);
		}
	}
	/// Register pointer type with name in parser but without any automatic registrations.
	template<typename P>
	void registerRawPointerType(const std::string& s)
	{
		if (!haveType<P*>())
		{
			addType<P*>(s);
		}
	}
	/// Register value type with name in parser but without any automatic registrations.
	template<typename P>
	void registerRawValueType(const std::string& s)
	{
		if (!haveType<P>())
		{
			addType<P>(s);
		}
	}

	/// Load global data from YAML.
	virtual void load(const YAML::YamlNodeReader& reader);

	/// Show all script informations.
	void logScriptMetadata(bool haveEvents, const std::string& groupName) const;

	/// Get name of script.
	const std::string& getName() const { return _name; }
	/// Get default script.
	const std::string& getDefault() const { return _defaultScript; }

	/// Get number of parameters.
	Uint8 getParamSize() const { return _regOutSize; }
	/// Get parameter data.
	const ScriptRefData* getParamData(Uint8 i) const { return getRef(_regOutName[i]); }

	/// Get name of type.
	ScriptRef getTypeName(ArgEnum type) const;
	/// Get full name of type.
	std::string getTypePrefix(ArgEnum type) const;
	/// Get type data.
	const ScriptTypeData* getType(ArgEnum type) const;
	/// Get type data.
	const ScriptTypeData* getType(ScriptRef name) const
	{
		ScriptRef r[] = { name };
		return getType({ std::begin(r), std::end(r)});
	}
	/// Get function data.
	ScriptRange<ScriptProcData> getProc(ScriptRef name) const
	{
		ScriptRef r[] = { name };
		return getProc({ std::begin(r), std::end(r)});
	}
	/// Get arguments data.
	const ScriptRefData* getRef(ScriptRef name) const
	{
		ScriptRef r[] = { name };
		return getRef({ std::begin(r), std::end(r)});
	}
	/// Get type data.
	const ScriptTypeData* getType(ScriptRange<ScriptRef> name) const;
	/// Get function data.
	ScriptRange<ScriptProcData> getProc(ScriptRange<ScriptRef> name) const;
	/// Get arguments data.
	const ScriptRefData* getRef(ScriptRange<ScriptRef> name) const;

	/// Get script shared data.
	ScriptGlobal* getGlobal() { return _shared; }
	/// Get script shared data.
	const ScriptGlobal* getGlobal() const { return _shared; }
	/// Get true if return does not accept any arguments.
	bool haveEmptyReturn() const { return _emptyReturn; }
};

/**
 * Copy constructor from pod type.
 */
template<typename T>
inline ScriptValueData::ScriptValueData(const T& t)
{
	static_assert(sizeof(T) <= sizeof(data), "Value have too big size!");
	type = ScriptParserBase::getArgType<T>();
	size = sizeof(T);
	memcpy(&data, &t, sizeof(T));
}

/**
 * Copy constructor.
 */
inline ScriptValueData::ScriptValueData(const ScriptValueData& t)
{
	*this = t;
}

/**
 * Assign operator from pod type.
 */
template<typename T>
inline ScriptValueData& ScriptValueData::operator=(const T& t)
{
	*this = ScriptValueData{ t };
	return *this;
}

/**
 * Assign operator.
 */
inline ScriptValueData& ScriptValueData::operator=(const ScriptValueData& t)
{
	type = t.type;
	size = t.size;
	memcpy(&data, &t.data, sizeof(data));
	return *this;
}

/**
 * Test if value have have selected type.
 */
template<typename T>
inline bool ScriptValueData::isValueType() const
{
	return type == ScriptParserBase::getArgType<T>();
}

/**
 * Get current stored value.
 */
template<typename T>
inline const T& ScriptValueData::getValue() const
{
	if (!isValueType<T>())
	{
		throw Exception("Invalid cast of value");
	}
	return *reinterpret_cast<const T*>(&data);
}

/**
 * Base template of strong typed parser.
 */
template<typename OutputPar, typename... Args>
class ScriptParser
{
public:
	using Container = ScriptContainerEvents<ScriptParser, Args...>;
	using Output = OutputPar;
	using Worker = ScriptWorker<Output, Args...>;

	static_assert(helper::StaticError<ScriptParser>::value, "Invalid parameters to template");
};

/**
 * Strong typed parser.
 */
template<typename... OutputArgs, typename... Args>
class ScriptParser<ScriptOutputArgs<OutputArgs...>, Args...> : public ScriptParserBase
{
public:
	using Container = ScriptContainer<ScriptParser, Args...>;
	using Output = ScriptOutputArgs<OutputArgs...>;
	using Worker = ScriptWorker<Output, Args...>;
	friend Container;

	/// Constructor.
	ScriptParser(ScriptGlobal* shared, const std::string& name, helper::ArgName<OutputArgs>... argOutputNames, helper::ArgName<Args>... argNames) : ScriptParserBase(shared, name)
	{
		addRegImpl(true, argOutputNames...);
		addRegImpl(false, argNames...);
	}
};

/**
 * Common base for strong typed event parser.
 */
class ScriptParserEventsBase : public ScriptParserBase
{
	constexpr static size_t EventsMax = 256;
	constexpr static size_t OffsetScale = 100;
	constexpr static size_t OffsetMax = 100 * OffsetScale;

	struct EventData
	{
		int offset;
		std::string name;
		ScriptContainerBase script;
	};

	/// Final list of events.
	std::vector<ScriptContainerBase> _events;
	/// Meta data of events.
	std::vector<EventData> _eventsData;

protected:
	/// Parse node and return new script.
	void parseNode(ScriptContainerEventsBase& container, const std::string& type, const YAML::YamlNodeReader& reader) const;
	/// Parse string and return new script.
	void parseCode(ScriptContainerEventsBase& container, const std::string& type, const std::string& srcCode) const;

public:
	/// Constructor.
	ScriptParserEventsBase(ScriptGlobal* shared, const std::string& name);

	/// Load global data from YAML.
	virtual void load(const YAML::YamlNodeReader& reader) override;
	/// Get pointer to events.
	const ScriptContainerBase* getEvents() const;
	/// Release event data.
	std::vector<ScriptContainerBase> releseEvents();
};

/**
 * Base template of strong typed event parser.
 */
template<typename OutputPar, typename... Args>
class ScriptParserEvents
{
public:
	using Container = ScriptContainerEvents<ScriptParserEvents, Args...>;
	using Output = OutputPar;
	using Worker = ScriptWorker<Output, Args...>;

	static_assert(helper::StaticError<ScriptParserEvents>::value, "Invalid parameters to template");
};

/**
 * Strong typed event parser.
 */
template<typename... OutputArgs, typename... Args>
class ScriptParserEvents<ScriptOutputArgs<OutputArgs...>, Args...> : public ScriptParserEventsBase
{
public:
	using Container = ScriptContainerEvents<ScriptParserEvents, Args...>;
	using Output = ScriptOutputArgs<OutputArgs...>;
	using Worker = ScriptWorker<Output, Args...>;
	friend Container;

	// Constructor.
	ScriptParserEvents(ScriptGlobal* shared, const std::string& name, helper::ArgName<OutputArgs>... argOutputNames, helper::ArgName<Args>... argNames) : ScriptParserEventsBase(shared, name)
	{
		addRegImpl(true, argOutputNames...);
		addRegImpl(false, argNames...);
	}
};

////////////////////////////////////////////////////////////
//					tags definitions
////////////////////////////////////////////////////////////

/**
 * Strong typed tag.
 */
template<typename T, typename I = Uint16>
struct ScriptTag
{
	static_assert(!std::numeric_limits<I>::is_signed, "Type should be unsigned");
	static_assert(sizeof(I) <= sizeof(size_t), "Type need be smaller than size_t");

	using Parent = T;

	/// Index that identify value in ScriptValues.
	I index;

	/// Get value.
	constexpr size_t get() const { return static_cast<size_t>(index); }
	/// Test if tag have valid value.
	constexpr explicit operator bool() const { return this->index; }
	/// Equal operator.
	constexpr bool operator==(ScriptTag t) const
	{
		return index == t.index;
	}
	/// Not-equal operator.
	constexpr bool operator!=(ScriptTag t) const
	{
		return !(*this == t);
	}

	/// Get run time value for type.
	static ArgEnum type() { return ScriptParserBase::getArgType<ScriptTag<T, I>>(); }
	/// Test if value can be used.
	static constexpr bool isValid(size_t i) { return i && i <= limit(); }
	/// Fake constructor.
	static constexpr ScriptTag make(size_t i) { return { static_cast<I>(i) }; }
	/// Max supported value.
	static constexpr size_t limit() { return static_cast<size_t>(std::numeric_limits<I>::max()); }
	/// Null value.
	static constexpr ScriptTag getNullTag() { return make(0); }
};

/**
 * Global data shared by all scripts.
 */
class ScriptGlobal
{
protected:
	using LoadFunc = void (*)(const ScriptGlobal*, int&, const YAML::YamlNodeReader&);
	using SaveFunc = void (*)(const ScriptGlobal*, const int&, YAML::YamlNodeWriter&);
	using CrateFunc = ScriptValueData (*)(size_t i);

	friend class ScriptValuesBase;

	struct TagValueType
	{
		ScriptRef name;
		LoadFunc load;
		SaveFunc save;
	};
	struct TagValueData
	{
		ScriptRef name;
		size_t valueType;
	};
	struct TagData
	{
		ScriptRef name;
		size_t limit;
		CrateFunc crate;
		std::vector<TagValueData> values;
	};

	template <typename ThisType, void (ThisType::*LoadValue)(int&, const YAML::YamlNodeReader&) const>
	static void loadHelper(const ScriptGlobal* base, int& value, const YAML::YamlNodeReader& reader)
	{
		(static_cast<const ThisType*>(base)->*LoadValue)(value, reader);
	}
	template <typename ThisType, void (ThisType::*SaveValue)(const int&, YAML::YamlNodeWriter&) const>
	static void saveHelper(const ScriptGlobal* base, const int& value, YAML::YamlNodeWriter& writer)
	{
		(static_cast<const ThisType*>(base)->*SaveValue)(value, writer);
	}

	void addTagValueTypeBase(const std::string& name, LoadFunc loadFunc, SaveFunc saveFunc)
	{
		_tagValueTypes.push_back(TagValueType{ addNameRef(name), loadFunc, saveFunc });
	}
	template <typename ThisType, void (ThisType::*LoadValue)(int&, const YAML::YamlNodeReader&) const, void (ThisType::*SaveValue)(const int&, YAML::YamlNodeWriter&) const>
	void addTagValueType(const std::string& name)
	{
		static_assert(std::is_base_of<ScriptGlobal, ThisType>::value, "Type must be derived");
		addTagValueTypeBase(name, &loadHelper<ThisType, LoadValue>, &saveHelper<ThisType, SaveValue>);
	}

private:
	std::string _currFile;
	std::vector<std::vector<char>> _strings;
	std::vector<std::vector<ScriptContainerBase>> _events;
	std::map<std::string, ScriptParserBase*> _parserNames;
	std::vector<ScriptParserEventsBase*> _parserEvents;
	std::map<ArgEnum, TagData> _tagNames;
	std::vector<TagValueType> _tagValueTypes;
	std::vector<ScriptRefData> _refList;

	/// Get tag value.
	size_t getTag(ArgEnum type, ScriptRef s) const;
	/// Get data of tag value.
	TagValueData getTagValueData(ArgEnum type, size_t i) const;
	/// Get tag value type data.
	TagValueType getTagValueTypeData(size_t valueType) const;
	/// Get tag value type id.
	size_t getTagValueTypeId(ScriptRef s) const;
	/// Add new tag name.
	size_t addTag(ArgEnum type, ScriptRef s, size_t valueType);
	/// Add new name ref.
	ScriptRef addNameRef(const std::string& s);

public:
	/// Default constructor.
	ScriptGlobal();
	/// Destructor.
	virtual ~ScriptGlobal();

	/// Store parser.
	void pushParser(const std::string& groupName, ScriptParserBase* parser);
	/// Store parser.
	void pushParser(const std::string& groupName, ScriptParserEventsBase* parser);

	/// Add new const value.
	void addConst(const std::string& name, ScriptValueData i);
	/// Update const value.
	void updateConst(const std::string& name, ScriptValueData i);

	/// Get global ref data.
	const ScriptRefData* getRef(ScriptRef name, ScriptRef postfix = {}) const;

	/// Get all tag names
	const std::map<ArgEnum, TagData> &getTagNames() const { return _tagNames; }

	/// Get tag based on it name.
	template<typename Tag>
	Tag getTag(ScriptRef s) const
	{
		return Tag::make(getTag(Tag::type(), s));
	}
	/// Get tag based on it name.
	template<typename Tag>
	Tag getTag(const std::string& s) const
	{
		return getTag<Tag>(ScriptRef::tempFrom(s));
	}
	/// Add new tag name.
	template<typename Tag>
	Tag addTag(const std::string& s, const std::string& valueTypeName)
	{
		return Tag::make(addTag(Tag::type(), addNameRef(s), getTagValueTypeId(ScriptRef::tempFrom(valueTypeName))));
	}
	/// Add new type of tag.
	template<typename Tag>
	void addTagType()
	{
		if (_tagNames.find(Tag::type()) == _tagNames.end())
		{
			_tagNames.insert(
				std::make_pair(
					Tag::type(),
					TagData
					{
						ScriptRef::staticFrom(Tag::Parent::ScriptName),
						Tag::limit(),
						[](size_t i) { return ScriptValueData{ Tag::make(i) }; },
						std::vector<TagValueData>{},
					}
				)
			);
		}
	}

	/// Get current file that is loaded.
	const std::string& getCurrentFile() const { return _currFile; }

	/// Initialize shared globals like types.
	virtual void initParserGlobals(ScriptParserBase* parser) { }
	/// Prepare for loading data.
	virtual void beginLoad();
	/// Prepare for loading file.
	virtual void fileLoad(const std::string& path);
	/// Finishing loading data.
	virtual void endLoad();

	/// Load global data from YAML.
	void load(const YAML::YamlNodeReader& reader);
};

/**
 * Collection of values for script usage.
 */
class ScriptValuesBase
{
	/// Vector with all available values for script.
	std::vector<int> values;

protected:
	/// Get all values
	const std::vector<int> &getValues() const { return values; }
	/// Set value.
	void setBase(size_t t, int i);
	/// Get value.
	int getBase(size_t t) const;
	/// Load values from yaml file.
	void loadBase(const YAML::YamlNodeReader& reader, const ScriptGlobal* shared, ArgEnum type, const std::string& nodeName);
	/// Save values to yaml file.
	void saveBase(YAML::YamlNodeWriter& writer, const ScriptGlobal* shared, ArgEnum type, const std::string& nodeName) const;
};

/**
 * Strong typed collection of values for script.
 */
template<typename T, typename I = Uint16>
class ScriptValues : ScriptValuesBase
{
public:
	using Tag = ScriptTag<T, I>;
	using Parent = T;

	/// Load values from yaml file.
	void load(const YAML::YamlNodeReader& reader, const ScriptGlobal* shared, const std::string& nodeName = "tags")
	{
		loadBase(reader, shared, Tag::type(), nodeName);
	}
	/// Save values to yaml file.
	void save(YAML::YamlNodeWriter writer, const ScriptGlobal* shared, const std::string& nodeName = "tags") const
	{
		saveBase(writer, shared, Tag::type(), nodeName);
	}

	/// Get value.
	int get(Tag t) const
	{
		return getBase(t.get());
	}
	/// Set value.
	void set(Tag t, int i)
	{
		return setBase(t.get(), i);
	}
	/// Get all values
	const std::vector<int> &getValuesRaw() const { return getValues(); }
};

////////////////////////////////////////////////////////////
//					script groups
////////////////////////////////////////////////////////////

template<typename Parent, typename... Parsers>
class ScriptGroupContainer : public Parsers::ContainerWarper...
{
public:
	/// Get container by type.
	template<typename SelectedParser>
	typename SelectedParser::Container& get()
	{
		return *static_cast<typename SelectedParser::ContainerWarper*>(this);
	}

	/// Get container by type.
	template<typename SelectedParser>
	const typename SelectedParser::Container& get() const
	{
		return *static_cast<const typename SelectedParser::ContainerWarper*>(this);
	}

	/// Load scripts.
	void load(const std::string& type, const YAML::YamlNodeReader& reader, const Parent& parsers)
	{
		(get<Parsers>().load(type, reader, parsers.template get<Parsers>()), ...);
	}
};

template<typename Parser, char... NameChars>
class ScriptGroupNamedParser : public Parser
{
	template<typename... C>
	static constexpr unsigned length(unsigned curr, char head, C... tail)
	{
		return head ? length(curr + 1, tail...) : curr;
	}
	static constexpr unsigned length(unsigned curr, char head)
	{
		return head ? throw "Script name too long!" : curr;
	}

	static constexpr unsigned nameLenght = length(0, NameChars...);

public:
	using BaseType = Parser;
	struct ContainerWarper : Parser::Container
	{

	};

	template<typename Master>
	ScriptGroupNamedParser(ScriptGlobal* shared, Master* master) : Parser{ shared, std::string{ { NameChars... }, 0, nameLenght, }, master, }
	{

	}

	BaseType& getBase() { return *this; }
};

template<typename Master, typename... Parsers>
class ScriptGroup : Parsers...
{
public:
	using Container = ScriptGroupContainer<ScriptGroup, Parsers...>;

	/// Constructor.
	ScriptGroup(ScriptGlobal* shared, Master* master, const std::string& groupName) : Parsers{ shared, master, }...
	{
		(void)master;
		(void)groupName;
		(shared->pushParser(groupName, &get<Parsers>()), ...);
	}

	/// Get parser by type.
	template<typename SelectedParser>
	typename SelectedParser::BaseType& get()
	{
		return *static_cast<SelectedParser*>(this);
	}

	/// Get parser by type.
	template<typename SelectedParser>
	const typename SelectedParser::BaseType& get() const
	{
		return *static_cast<const SelectedParser*>(this);
	}
};

#define MACRO_GET_STRING_1(str, i) \
	(sizeof(str) > (i) ? str[(i)] : 0)

#define MACRO_GET_STRING_4(str, i) \
	MACRO_GET_STRING_1(str, i+0),  \
	MACRO_GET_STRING_1(str, i+1),  \
	MACRO_GET_STRING_1(str, i+2),  \
	MACRO_GET_STRING_1(str, i+3)

#define MACRO_GET_STRING_16(str, i) \
	MACRO_GET_STRING_4(str, i+0),   \
	MACRO_GET_STRING_4(str, i+4),   \
	MACRO_GET_STRING_4(str, i+8),   \
	MACRO_GET_STRING_4(str, i+12)

#define MACRO_NAMED_SCRIPT(nameString, type) ScriptGroupNamedParser<type, MACRO_GET_STRING_16(nameString, 0), MACRO_GET_STRING_16(nameString, 16), MACRO_GET_STRING_16(nameString, 32)>

} //namespace OpenXcom
