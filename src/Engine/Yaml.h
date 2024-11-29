#pragma once
/*
 * Copyright 2010-2024 OpenXcom Developers.
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

// the support for vector/map that rapidyaml provides is sadly not backward-compatible with yaml-cpp's due to
// the fact that yaml-cpp clears collections before deserializing into them. These define's disable the support.
#define _C4_YML_STD_MAP_HPP_
#define _C4_YML_STD_VECTOR_HPP_

#pragma warning(push)
#pragma warning(disable : 4668 6011 6255 6293 6386 26439 26495 26498 26819)
#include "../../libs/rapidyaml/ryml.hpp"
#include "../../libs/rapidyaml/ryml_std.hpp"
#pragma warning(pop)
#include <map>
#include <vector>
#include <unordered_map>
#include <c4/format.hpp>
#include <c4/type_name.hpp>
#include "../Engine/CrossPlatform.h"

//hash function for ryml::csubstr for unordered_map -> just calls the same thing std::hash<std::string> does
template <>
struct std::hash<ryml::csubstr> { std::size_t operator()(const ryml::csubstr& k) const; };

namespace OpenXcom
{

namespace YAML
{

const std::string Null = "~"; // "~" or "null" or "Null" or "NULL"; serializing null is the same as serializing a string

void setGlobalErrorHandler();

class YamlRootNodeReader;
class YamlRootNodeWriter;

/// Basic string wrapper to differentiate from normal strings
struct YamlString
{
	std::string yaml;
	YamlString(std::string yamlString);
};

/// Basic exception class to distinguish YAML exceptions from the rest.
class Exception : public std::runtime_error
{
public:
	Exception(const std::string& msg);
};

class YamlNodeReader
{
protected:
	ryml::ConstNodeRef _node;
	const YamlRootNodeReader* _root;
	bool _invalid;
	std::unordered_map<ryml::csubstr, ryml::id_type>* _index;

	ryml::ConstNodeRef getChildNode(const ryml::csubstr& key) const;

public:
	YamlNodeReader(); // vector demands a default constructor despite it never being used
	YamlNodeReader(const YamlRootNodeReader* root, const ryml::ConstNodeRef& node);
	YamlNodeReader(const YamlRootNodeReader* root, const ryml::ConstNodeRef& node, bool useIndex);
	~YamlNodeReader();

	/// Returns a copy of the current mapping container with O(1) access to the children. O(n) is spent building the index.
	const YamlNodeReader useIndex() const;


	/// Deserializes the value of the found child into the outputValue. If the node is invalid or the key doesn't exist, outputValue is set to defaultValue.
	template <typename OutputType> // Name conflicts if renamed to "read"
	void readN(ryml::csubstr key, OutputType& outputValue, const OutputType& defaultValue) const;

	/// Returns a deserialized key of the current node. Throws if the node is invalid or itself has no key.
	template <typename OutputType>
	OutputType readKey() const;

	/// Returns a deserialized key of the current node, or a default value if the node is invalid or itself has no key
	template <typename OutputType>
	OutputType readKey(const OutputType& defaultValue) const;

	/// Returns a deserialized value of the current node. Throws if the node is invalid.
	template <typename OutputType>
	OutputType readVal() const;

	/// Returns a deserialized value of the current node, or a default value if the node is invalid
	template <typename OutputType>
	OutputType readVal(const OutputType& defaultValue) const;

	/// Returns a deserialized binary value of the current node. Throws if the node is invalid.
	std::vector<char> readValBase64() const;


	/// Returns false if the node is invalid or the key doesn't exist. Otherwise returns true and deserializes the value of the found child into the outputValue.
	template <typename OutputType>
	bool tryRead(ryml::csubstr key, OutputType& outputValue) const;

	/// Returns false if the node is invalid or itself has no key. Otherwise returns true and deserializes the key of the current node into the outputValue.
	template <typename OutputType>
	bool tryReadKey(OutputType& outputValue) const;

	/// Returns false if the node is invalid. Otherwise returns true and deserializes the value of the current node into the outputValue.
	template <typename OutputType>
	bool tryReadVal(OutputType& outputValue) const;


	/// Returns the number of children of the current node. O(n) complexity, or O(1) if index is used.
	size_t childrenCount() const;

	/// Builds a vector of children and retuns it
	std::vector<YamlNodeReader> children() const;

	/// Returns whether the current node is valid. Just use the bool operator instead.
	bool isValid() const;
	/// Returns true if the current node is a mapping container
	bool isMap() const;
	/// Returns true if the current node is a sequence container
	bool isSeq() const;
	/// Returns true if the current node has a scalar value (empty strings and null constants count)
	bool hasVal() const;
	/// Returns true if the current node has a scalar value and this value is one of the null constants
	bool hasNullVal() const;
	/// Returns true if the current node has a scalar value and an explicit tag
	bool hasValTag() const;

	/// Returns true if the node is valid, has a tag, and the tag is a core tag
	bool hasValTag(ryml::YamlTag_e tag) const;
	/// Returns true if the node is valid, has a tag, and the tag's name equals tagName
	bool hasValTag(std::string tagName) const;
	/// Returns node's value's tag, or an empty string if there is none
	std::string getValTag() const;

	/// Serializes the node and its descendants to a YamlString
	const YamlString emit() const;
	/// Serializes the node's descendants to a YamlString
	const YamlString emitDescendants() const;
	/// Returns an object that contains data on where the current node is located in the original yaml
	ryml::Location getLocationInFile() const;

	/// Returns a child in the current mapping container or an invalid child
	const YamlNodeReader operator[](ryml::csubstr key) const;
	/// Returns a child at a specific position or an invalid child
	const YamlNodeReader operator[](size_t pos) const;
	/// Returns whether the current node is valid
	explicit operator bool() const;

	friend YamlRootNodeReader;
	friend YamlRootNodeWriter;
};

class YamlRootNodeReader : public YamlNodeReader
{
private:
	ryml::Tree* _tree;
	ryml::Parser* _parser;
	ryml::EventHandlerTree* _eventHandler;
	std::string _fileName;

	ryml::Location getLocationInFile(const ryml::ConstNodeRef& node) const;

	void Parse(ryml::csubstr yaml, std::string fileName, bool withNodeLocations);

public:
	YamlRootNodeReader(std::string fullFilePath, bool onlyInfoHeader = false);
	YamlRootNodeReader(const RawData& data, std::string fileNameForError);
	YamlRootNodeReader(const YamlString& yamlString, std::string description);
	~YamlRootNodeReader();

	/// Returns base class to avoid slicing
	YamlNodeReader sansRoot() const;

	friend YamlNodeReader;
	friend YamlRootNodeWriter;
};

class YamlNodeWriter
{
protected:
	const YamlRootNodeWriter* _root;
	ryml::NodeRef _node;

public:
	YamlNodeWriter(const YamlRootNodeWriter* root, ryml::NodeRef node);

	/// Converts writer to a reader
	YamlNodeReader toReader();

	/// Adds a container child to the current sequence container
	YamlNodeWriter write();
	/// Adds a container child to the current mapping container
	YamlNodeWriter operator[](ryml::csubstr key);
	/// Adds a scalar value child to the current sequence container, serializing the provided value
	template <typename InputType>
	YamlNodeWriter write(const InputType& inputValue);
	/// Adds a scalar value child to the current mapping container, serializing the provided value
	template <typename InputType>
	YamlNodeWriter write(ryml::csubstr key, const InputType& inputValue);
	/// If the inputVector is not empty, adds a sequence container child to the current mapping container.
	/// The callback (YamlNodeWriter w, InputType val) should specify how to write a vector element to the sequence container.
	template <typename InputType, typename Func>
	void write(ryml::csubstr key, const std::vector<InputType>& inputVector, Func callback);
	/// Adds a scalar value child to the current mapping container, serializing the provided binary data
	YamlNodeWriter writeBase64(ryml::csubstr key, char* data, size_t size);
	/// Adds a value to the current node.
	template <typename InputType>
	void setValue(const InputType& inputValue);

	/// Marks the current node as a mapping container
	void setAsMap();
	/// Marks the current node as a sequence container
	void setAsSeq();
	/// Marks the current node to serialize as single-line flow-style
	void setFlowStyle();
	/// Marks the current node to serialize as multi-line block-style
	void setBlockStyle();
	/// Marks the current node to serialize the scalar in double quotes
	void setAsQuoted();

	void unsetAsMap();
	void unsetAsSeq();

	/// Saves a string to the internal buffer. In a rare case when a key isn't a string literal, this ensures its lifetime until the serialization is done.
	ryml::csubstr saveString(const std::string& string);

	/// Emits a yaml string based on the current node and its subtree
	YamlString emit();

	friend YamlRootNodeReader;
	friend YamlRootNodeWriter;
};

class YamlRootNodeWriter : public YamlNodeWriter
{
private:
	ryml::Tree* _tree;
	ryml::Parser* _parser;
	ryml::EventHandlerTree* _eventHandler;

public:
	YamlRootNodeWriter();
	YamlRootNodeWriter(size_t bufferCapacity);
	~YamlRootNodeWriter();

	/// Returns base class to avoid slicing
	YamlNodeWriter sansRoot();

	friend YamlNodeReader;
	friend YamlNodeWriter;
	friend YamlRootNodeReader;
};

/* Template implementations below  */

template <typename OutputType>
void YamlNodeReader::readN(ryml::csubstr key, OutputType& outputValue, const OutputType& defaultValue) const
{
	if (!tryRead(key, outputValue))
		outputValue = defaultValue;
}

template <typename OutputType>
OutputType YamlNodeReader::readKey() const
{
	OutputType output;
	if (!tryReadKey(output))
	{
		if (_root)
			throw Exception(c4::formatrs<std::string>("{} ERROR: {}", _root->_fileName, "Tried to deserialize invalid node's key!"));
		throw Exception("Tried to deserialize invalid node's key!");
	}
	return output;
}

template <typename OutputType>
inline OutputType YamlNodeReader::readKey(const OutputType& defaultValue) const
{
	OutputType output;
	if (!tryReadKey(output))
		output = defaultValue;
	return output;
}

template <typename OutputType>
OutputType YamlNodeReader::readVal() const
{
	OutputType output;
	if (!tryReadVal(output))
	{
		if (_root)
			throw Exception(c4::formatrs<std::string>("{} ERROR: {}", _root->_fileName, "Tried to deserialize invalid node!"));
		throw Exception("Tried to deserialize invalid node!");
	}
	return output;
}

template <typename OutputType>
OutputType YamlNodeReader::readVal(const OutputType& defaultValue) const
{
	OutputType output;
	if (!tryReadVal(output))
		output = defaultValue;
	return output;
}

template <typename OutputType>
bool YamlNodeReader::tryRead(ryml::csubstr key, OutputType& outputValue) const
{
	if (_invalid || !_node.is_map())
		return false;
	if (!_index)
	{
		const auto& child = _node.find_child(key);
		if (child.invalid())
			return false;
		if (!read(child, &outputValue))
		{
			ryml::Location loc = _root->getLocationInFile(child);
			c4::cspan<char> typeName = c4::type_name<OutputType>();
			throw Exception(c4::formatrs<std::string>("{}:{}:{} ERROR: Could not deserialize value to type <{}>!", loc.name, loc.line, loc.col, ryml::csubstr(typeName.data(), typeName.size())));
		}
		return true;
	}
	if (const auto& keyNodeIdPair = _index->find(key); keyNodeIdPair != _index->end())
	{
		if (!read(_node.tree()->cref(keyNodeIdPair->second), &outputValue))
		{
			ryml::Location loc = _root->getLocationInFile(_node.tree()->cref(_index->at(key)));
			c4::cspan<char> typeName = c4::type_name<OutputType>();
			throw Exception(c4::formatrs<std::string>("{}:{}:{} ERROR: Could not deserialize value to type <{}>!", loc.name, loc.line, loc.col, ryml::csubstr(typeName.data(), typeName.size())));
		}
		return true;
	}
	return false;
}

template <typename OutputType>
bool YamlNodeReader::tryReadKey(OutputType& outputValue) const
{
	if (_invalid || !_node.has_key())
		return false;
	_node >> ryml::key(outputValue);
	return true;
}

template <typename OutputType>
bool YamlNodeReader::tryReadVal(OutputType& outputValue) const
{
	if (_invalid)
		return false;
	if (!read(_node, &outputValue))
	{
		ryml::Location loc = getLocationInFile();
		c4::cspan<char> typeName = c4::type_name<OutputType>();
		throw Exception(c4::formatrs<std::string>("{}:{}:{} ERROR: Could not deserialize value to type <{}>!", loc.name, loc.line, loc.col, ryml::csubstr(typeName.data(), typeName.size())));
	}
	return true;
}

template <typename InputType>
inline YamlNodeWriter YamlNodeWriter::write(const InputType& inputValue)
{
	return YamlNodeWriter(_root, _node.append_child() << inputValue);
}

template <typename InputType>
YamlNodeWriter YamlNodeWriter::write(ryml::csubstr key, const InputType& inputValue)
{
	return YamlNodeWriter(_root, _node.append_child({ ryml::KEY, key }) << inputValue);
}

template <typename InputType, typename Func>
void YamlNodeWriter::write(ryml::csubstr key, const std::vector<InputType>& inputVector, Func callback)
{
	if (inputVector.empty())
		return;
	YamlNodeWriter sequenceWriter(_root, _node.append_child({ ryml::KEY, key }));
	sequenceWriter.setAsSeq();
	for (const InputType& vectorElement : inputVector)
	{
		callback(sequenceWriter, vectorElement);
	}
}

template <typename InputType>
inline void YamlNodeWriter::setValue(const InputType& inputValue)
{
	_node << inputValue;
}

} //namespace YAML end

// Deserialization template for enums. We have to overload from_chars() instead of read(), because read() already has a template for all types -> template conflict
template <typename EnumType>
typename std::enable_if<std::is_enum<EnumType>::value, bool>::type inline from_chars(ryml::csubstr buf, EnumType* v) noexcept
{
	return ryml::atoi(buf, (int*)v);
}

template <typename EnumType>
typename std::enable_if<std::is_enum<EnumType>::value, size_t>::type inline to_chars(ryml::substr buf, EnumType v) noexcept
{
	return ryml::itoa(buf, (int)v);
}

} //namespace OpenXcom end

// r/w overloads need to be defined in the same namespace the type is defined in
namespace std
{

// Deserializing "" should succeed when output type is std::string
bool read(ryml::ConstNodeRef const& n, std::string* str);

// for backwards compatibility, pairs should be serialized as sequences with 2 elements.
template <class T1, class T2>
bool read(ryml::ConstNodeRef const& n, std::pair<T1, T2>* pair)
{
	n.first_child() >> pair->first;
	n.last_child() >> pair->second;
	return true;
}

template <class T1, class T2>
void write(ryml::NodeRef* n, std::pair<T1, T2> const& pair)
{
	*n |= ryml::SEQ;
	n->append_child() << pair.first;
	n->append_child() << pair.second;
}

} // namespace std end

namespace c4::yml
{
// Serializing bool should output the string version instead of 0 and 1
void write(ryml::NodeRef* n, bool const& val);

// Copy from c4/yml/std/vector.hpp
template <class V, class Alloc>
void write(c4::yml::NodeRef* n, std::vector<V, Alloc> const& vec)
{
	*n |= c4::yml::SEQ;
	for (V const& v : vec)
		n->append_child() << v;
}

// Backwards-compatibility: deserializing into a vector should clear the collection before adding to it
template <class V, class Alloc>
bool read(c4::yml::ConstNodeRef const& n, std::vector<V, Alloc>* vec)
{
	vec->clear();
	C4_SUPPRESS_WARNING_GCC_WITH_PUSH("-Wuseless-cast")
	vec->resize(static_cast<size_t>(n.num_children()));
	C4_SUPPRESS_WARNING_GCC_POP
	size_t pos = 0;
	for (ConstNodeRef const child : n)
		child >> (*vec)[pos++];
	return true;
}

// Backwards-compatibility: deserializing into a vector should clear the collection before adding to it
/** specialization: std::vector<bool> uses std::vector<bool>::reference as
 * the return value of its operator[]. */
template <class Alloc>
bool read(c4::yml::ConstNodeRef const& n, std::vector<bool, Alloc>* vec)
{
	vec->clear();
	C4_SUPPRESS_WARNING_GCC_WITH_PUSH("-Wuseless-cast")
	vec->resize(static_cast<size_t>(n.num_children()));
	C4_SUPPRESS_WARNING_GCC_POP
	size_t pos = 0;
	bool tmp = {};
	for (ConstNodeRef const child : n)
	{
		child >> tmp;
		(*vec)[pos++] = tmp;
	}
	return true;
}

// Copy from c4/yml/std/map.hpp
template <class K, class V, class Less, class Alloc>
void write(c4::yml::NodeRef* n, std::map<K, V, Less, Alloc> const& m)
{
	*n |= c4::yml::MAP;
	for (auto const& C4_RESTRICT p : m)
	{
		auto ch = n->append_child();
		ch << c4::yml::key(p.first);
		ch << p.second;
	}
}

// Backwards-compatibility: deserializing into maps should clear the collection before adding to it
// Also, element constructor inside the loop
template <class K, class V, class Less, class Alloc>
bool read(c4::yml::ConstNodeRef const& n, std::map<K, V, Less, Alloc>* m)
{
	m->clear();
	for (auto const& C4_RESTRICT ch : n)
	{
		K k{};
		V v{};
		ch >> c4::yml::key(k);
		ch >> v;
		m->emplace(std::make_pair(std::move(k), std::move(v)));
	}
	return true;
}

}
