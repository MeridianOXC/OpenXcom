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

#include "Yaml.h"
#include "../Engine/CrossPlatform.h"
#include <string>
#include <c4/format.hpp>

namespace OpenXcom
{

namespace YAML
{

Exception::Exception(const std::string& msg) : runtime_error(msg)
{
}

/// Custom error handler; For now it doesn't do anything
struct YamlErrorHandler
{
	ryml::Callbacks callbacks();
	C4_NORETURN void on_error(const char* msg, size_t len, ryml::Location loc);
	C4_NORETURN static void s_error(const char* msg, size_t len, ryml::Location loc, void* this_);
	YamlErrorHandler() : defaults(ryml::get_callbacks()) {}
	ryml::Callbacks defaults;
};
C4_NORETURN void YamlErrorHandler::s_error(const char* msg, size_t len, ryml::Location loc, void* this_)
{
	((YamlErrorHandler*)this_)->on_error(msg, len, loc);
}
C4_NORETURN void YamlErrorHandler::on_error(const char* msg, size_t len, ryml::Location loc)
{
	/*
	std::string full_msg = c4::formatrs<std::string>(
		"File:{} Line:{} Column:{} ERROR: {}",
		loc.name, loc.line, loc.col, ryml::csubstr(msg, len));*/
	throw Exception(msg); // This function must not return
}
static void* s_allocate(size_t len, void* /*hint*/, void* this_)
{
	return SDL_malloc(len);
}
static void s_free(void* mem, size_t len, void* this_)
{
	SDL_free(mem);
}
ryml::Callbacks YamlErrorHandler::callbacks()
{
	return ryml::Callbacks(this, s_allocate, s_free, YamlErrorHandler::s_error);
}

void setGlobalErrorHandler()
{
	YamlErrorHandler errh;
	ryml::set_callbacks(errh.callbacks());
}

YamlString::YamlString(std::string yamlString) : yaml(yamlString) {}

YamlNodeReader::YamlNodeReader()
	: _node(ryml::ConstNodeRef(nullptr, ryml::NONE)), _invalid(true), _index(nullptr), _root(nullptr)
{
}

YamlNodeReader::YamlNodeReader(const YamlRootNodeReader* root, const ryml::ConstNodeRef& node)
	: _root(root), _node(node), _invalid(node.invalid()), _index(nullptr)
{
}

YamlNodeReader::YamlNodeReader(const YamlRootNodeReader* root, const ryml::ConstNodeRef& node, bool useIndex)
	: _root(root), _node(node), _invalid(node.invalid()), _index(nullptr)
{
	if (!useIndex)
		return;
	// build and use an index to avoid [] operator's O(n) complexity
	_index = new std::unordered_map<ryml::csubstr, ryml::id_type>();
	_index->reserve(_node.num_children());
	for (const ryml::ConstNodeRef& childNode : _node.children())
		(*_index)[childNode.key()] = childNode.id();
}

YamlNodeReader::~YamlNodeReader()
{
	if (_index)
	{
		delete _index;
	}
}

const YamlNodeReader YamlNodeReader::useIndex() const
{
	return YamlNodeReader(_root, _node, true);
}

std::vector<char> YamlNodeReader::readValBase64() const
{
	// first run figure out the decoded length
	size_t len = _node.deserialize_val(c4::fmt::base64(ryml::substr()));
	std::vector<char> decodedData(len);
	ryml::blob buf(decodedData.data(), len);
	// deserialize
	_node.deserialize_val(c4::fmt::base64(buf));
	return decodedData;
}

size_t YamlNodeReader::childrenCount() const
{
	if (_invalid)
		return 0;
	return _index ? _index->size() : _node.num_children();
}

ryml::ConstNodeRef YamlNodeReader::getChildNode(const ryml::csubstr& key) const
{
	if (_invalid)
		return ryml::ConstNodeRef(_node.tree(), ryml::NONE);
	if (!_index)
	{
		if (!_node.is_map())
			return ryml::ConstNodeRef(_node.tree(), ryml::NONE);
		return _node.find_child(key);
	}
	if (const auto& keyNodeIdPair = _index->find(key); keyNodeIdPair != _index->end())
		return _node.tree()->cref(keyNodeIdPair->second);
	return ryml::ConstNodeRef(_node.tree(), ryml::NONE);
}

std::vector<YamlNodeReader> YamlNodeReader::children() const
{
	std::vector<YamlNodeReader> children;
	if (_invalid)
		return children;
	children.reserve(_node.num_children());
	for (const ryml::ConstNodeRef& child : _node.cchildren())
		children.emplace_back(_root, child);
	return children;
}

bool YamlNodeReader::isValid() const
{
	return !_invalid;
}

bool YamlNodeReader::isMap() const
{
	return _node.is_map();
}

bool YamlNodeReader::isSeq() const
{
	return _node.is_seq();
}

bool YamlNodeReader::hasVal() const
{
	return _node.has_val();
}

bool YamlNodeReader::hasNullVal() const
{
	return _node.has_val() && _node.val_is_null();
}

bool YamlNodeReader::hasValTag() const
{
	return _node.has_val_tag();
}

bool YamlNodeReader::hasValTag(ryml::YamlTag_e tag) const
{
	if (_invalid || !_node.has_val_tag())
		return false;
	return ryml::to_tag(_node.val_tag()) == tag;
}

bool YamlNodeReader::hasValTag(std::string tagName) const
{
	if (_invalid || !_node.has_val_tag())
		return false;
	return _node.val_tag() == tagName;
}

std::string YamlNodeReader::getValTag() const
{
	if (_invalid || !_node.has_val_tag())
		return std::string();
	ryml::csubstr valTag = _node.val_tag();
	return std::string(valTag.str, valTag.len);
}

const YamlString YamlNodeReader::emit() const
{
	return YamlString(ryml::emitrs_yaml<std::string>(_node));
}

const YamlString YamlNodeReader::emitDescendants() const
{
	YAML::YamlRootNodeWriter writer;
	if (isMap())
		writer.setAsMap();
	else if (isSeq())
		writer.setAsSeq();
	else
		return YamlString(std::string());
	writer._tree->duplicate_children(_root->_tree, _node.id(), writer._node.id(), ryml::NONE);
	return writer.emit();
}

ryml::Location YamlNodeReader::getLocationInFile() const
{
	return _root->getLocationInFile(_node);
}

const YamlNodeReader YamlNodeReader::operator[](ryml::csubstr key) const
{
	return YamlNodeReader(_root, getChildNode(key));
}

const YamlNodeReader YamlNodeReader::operator[](size_t pos) const
{
	if (_invalid)
		return YamlNodeReader(_root, ryml::ConstNodeRef(_node.tree(), ryml::NONE));
	return YamlNodeReader(_root, _node.child(pos));
}

YamlNodeReader::operator bool() const
{
	return !_invalid;
}

YamlRootNodeReader::YamlRootNodeReader(std::string fullFilePath, bool onlyInfoHeader) : YamlNodeReader(), _tree(new ryml::Tree()), _parser(nullptr), _eventHandler(nullptr)
{
	RawData data = onlyInfoHeader ? CrossPlatform::getYamlSaveHeaderRaw(fullFilePath) : CrossPlatform::readFileRaw(fullFilePath);
	ryml::csubstr str = ryml::csubstr((char*)data.data(), data.size());
	if (onlyInfoHeader)
		str = ryml::csubstr((char*)data.data(), str.find("\n---") + 1);
	Parse(str, fullFilePath, true);
}

YamlRootNodeReader::YamlRootNodeReader(const RawData& data, std::string fileNameForError) : YamlNodeReader(), _tree(new ryml::Tree()), _parser(nullptr), _eventHandler(nullptr)
{
	Parse(ryml::csubstr((char*)data.data(), data.size()), fileNameForError, true);
}

YamlRootNodeReader::YamlRootNodeReader(const YamlString& yamlString, std::string description) : YamlNodeReader(), _tree(new ryml::Tree()), _parser(nullptr), _eventHandler(nullptr)
{
	Parse(ryml::to_csubstr(yamlString.yaml), description, false);
}

void YamlRootNodeReader::Parse(ryml::csubstr yaml, std::string fileNameForError, bool withNodeLocations)
{
	if (yaml.len > 3 && yaml.first(3) == "\xEF\xBB\xBF") // skip UTF-8 BOM
		yaml = yaml.offs(3, 0);
	_eventHandler = new ryml::EventHandlerTree(_tree->callbacks());
	_parser = new ryml::Parser(_eventHandler, ryml::ParserOptions().locations(withNodeLocations));
	_fileName = fileNameForError;
	_tree->reserve(yaml.len / 16);
	ryml::parse_in_arena(_parser, ryml::to_csubstr(_fileName), yaml, _tree);
	_tree->resolve();
	_node = _tree->crootref();
	_root = this;
	_invalid = _node.invalid();
}

YamlRootNodeReader::~YamlRootNodeReader()
{
	delete _tree;
	if (_parser)
		delete _parser;
	if (_eventHandler)
		delete _eventHandler;
}

YamlNodeReader YamlRootNodeReader::sansRoot() const
{
	return YamlNodeReader(this, _node);
}

ryml::Location YamlRootNodeReader::getLocationInFile(const ryml::ConstNodeRef& node) const
{
	if (_parser && _root)
	{
		// line and column here are 0-based, which isn't correct.
		ryml::Location loc = _parser->location(node);
		loc.line += 1;
		loc.col += 1;
		return loc;
	}
	else
		throw Exception("Parsed yaml without location data logging enabled");
}

YamlNodeWriter::YamlNodeWriter(const YamlRootNodeWriter* root, ryml::NodeRef node) : _node(node), _root(root)
{
}

YamlNodeReader YamlNodeWriter::toReader()
{
	return YamlNodeReader(nullptr, _node);
}

YamlNodeWriter YamlNodeWriter::write()
{
	return YamlNodeWriter(_root, _node.append_child());
}

YamlNodeWriter YamlNodeWriter::operator[](ryml::csubstr key)
{
	return YamlNodeWriter(_root, _node.append_child({ryml::KEY, key}));
}

YamlNodeWriter YamlNodeWriter::writeBase64(ryml::csubstr key, char* data, size_t size)
{
	//return YamlNodeWriter(_root, _node.append_child());
	return YamlNodeWriter(_root, _node.append_child({ryml::KEY, key}) << c4::fmt::base64(ryml::csubstr(data, size)));
}

void YamlNodeWriter::setAsMap()
{
	_node |= ryml::MAP;
}

void YamlNodeWriter::setAsSeq()
{
	_node |= ryml::SEQ;
}

void YamlNodeWriter::setFlowStyle()
{
	_node |= ryml::FLOW_SL;
}

void YamlNodeWriter::setBlockStyle()
{
	_node |= ryml::BLOCK;
}

void YamlNodeWriter::setAsQuoted()
{
	_node |= ryml::VAL_DQUO;
}

void YamlNodeWriter::unsetAsMap()
{
	_node.tree()->_rem_flags(_node.id(), ryml::MAP);
}

void YamlNodeWriter::unsetAsSeq()
{
	_node.tree()->_rem_flags(_node.id(), ryml::SEQ);
}

ryml::csubstr YamlNodeWriter::saveString(const std::string& string)
{
	return _node.tree()->to_arena(string);
}

YamlString YamlNodeWriter::emit()
{
	return YamlString(ryml::emitrs_yaml<std::string>(_node));
}

YamlRootNodeWriter::YamlRootNodeWriter() : YamlNodeWriter(this, _node), _tree(new ryml::Tree()), _parser(nullptr), _eventHandler(nullptr)
{
	_node = _tree->rootref();
}

YamlRootNodeWriter::YamlRootNodeWriter(size_t bufferCapacity) : YamlNodeWriter(this, _node), _tree(new ryml::Tree(0, bufferCapacity)), _parser(nullptr), _eventHandler(nullptr)
{
	_node = _tree->rootref();
}

YamlRootNodeWriter::~YamlRootNodeWriter()
{
	delete _tree;
}

YamlNodeWriter YamlRootNodeWriter::sansRoot()
{
	return YamlNodeWriter(this, _node);
}

} // namespace YAML end

} // namespace OpenXcom end

std::size_t std::hash<ryml::csubstr>::operator()(const ryml::csubstr& k) const
{
#ifdef _MSC_VER
	return _Hash_array_representation(k.str, k.len);
#else
	return std::hash<std::string_view>{}(std::string_view(k.str, k.len));
#endif
};

namespace std
{

// Deserializing "" should succeed when the output type is std::string
bool read(ryml::ConstNodeRef const& n, std::string* str)
{
	if (n.val().len > 0)
		ryml::from_chars(n.val(), str);
	else if (str->size() != 0)
		str->clear();		
	return true;
}

}

namespace c4::yml
{
// Serializing bool should output the string version instead of 0 and 1
void write(ryml::NodeRef* n, bool const& v)
{
	n->set_val_serialized(c4::fmt::boolalpha(v));
}
}
