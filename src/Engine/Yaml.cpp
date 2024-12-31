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
	throw Exception("Rapidyaml " + std::string{msg, len}); // This function must not return
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
	static YamlErrorHandler errh;
	ryml::set_callbacks(errh.callbacks());
}


////////////////////////////////////////////////////////////
//					YamlNodeReader
////////////////////////////////////////////////////////////


YamlNodeReader::YamlNodeReader()
	: _node(ryml::ConstNodeRef(nullptr, ryml::NONE)), _root(nullptr), _invalid(true)
{
}

YamlNodeReader::YamlNodeReader(const YamlRootNodeReader* root, const ryml::ConstNodeRef& node)
	: _node(node), _root(root), _invalid(node.invalid())
{
}

YamlNodeReader::YamlNodeReader(const YamlRootNodeReader* root, const ryml::ConstNodeRef& node, bool useIndex)
	: _node(node), _root(root), _invalid(node.invalid())
{
	if (_invalid || !useIndex)
		return;
	// build and use an index to avoid [] operator's O(n) complexity
	_index.emplace();

	if (!_node.is_map())
	{
		if (_node.is_seq())
		{
			if (_node.is_stream())
			{
				(*this)[1].throwNodeError("multi-document yaml file with splits '---'");
			}
			else
			{
				throwNodeError("sequence node as map");
			}
		}
		else if (_node.has_val())
		{
			if (_node.val_is_null())
			{
				// it is `~` and we consider this as allowed empty map
			}
			else
			{
				throwNodeError("value node as map");

			}
		}
		else
		{
			assert(false && "unexpected node type for index");
		}
	}

	_index->reserve(_node.num_children());
	for (const ryml::ConstNodeRef childNode : _node.cchildren())
		_index->emplace(childNode.key(), childNode.id());
}

YamlNodeReader YamlNodeReader::useIndex() const
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
	for (const ryml::ConstNodeRef child : _node.cchildren())
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

bool YamlNodeReader::hasValTag(const std::string& tagName) const
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
	return emitDescendants(YamlNodeReader());
}

const YamlString YamlNodeReader::emitDescendants(const YamlNodeReader& defaultValuesReader) const
{
	YAML::YamlRootNodeWriter writer;
	if (isMap())
		writer.setAsMap();
	else if (isSeq())
		writer.setAsSeq();
	else
		return YamlString(std::string());
	writer._tree->duplicate_children(_root->_tree.get(), _node.id(), writer._node.id(), ryml::NONE);
	if (defaultValuesReader)
		for (const ryml::ConstNodeRef child : defaultValuesReader._node.cchildren())
			if (writer._node.find_child(child.key()).invalid())
				writer._tree.get()->duplicate(child.tree(), child.id(), writer._node.id(), writer._node.last_child().id());
	return writer.emit();
}

ryml::Location YamlNodeReader::getLocationInFile() const
{
	return _root->getLocationInFile(_node);
}

YamlNodeReader YamlNodeReader::operator[](ryml::csubstr key) const
{
	return YamlNodeReader(_root, getChildNode(key));
}

YamlNodeReader YamlNodeReader::operator[](size_t pos) const
{
	if (_invalid)
		return YamlNodeReader(_root, ryml::ConstNodeRef(_node.tree(), ryml::NONE));
	return YamlNodeReader(_root, _node.child(pos));
}

YamlNodeReader::operator bool() const
{
	return !_invalid;
}


void YamlNodeReader::throwTypeError(const ryml::ConstNodeRef& node, const ryml::cspan<char>& type) const
{
	auto name = ryml::csubstr(type.data(), type.size() - (type.back() == 0));
	if (_root && _root->_parser && node.readable())
	{
		ryml::Location loc = _root->getLocationInFile(node);
		throw Exception(c4::formatrs<std::string>("Could not deserialize value to type <{}>! {} at line {}:{}", name, loc.name, loc.line, loc.col));
	}
	else
	{
		throw Exception(c4::formatrs<std::string>("Could not deserialize value to type <{}>!", name));
	}
}

void YamlNodeReader::throwNodeError(const std::string& what) const
{
	if (_root && _root->_parser && isValid())
	{
		ryml::Location loc = _root->getLocationInFile(_node);
		throw Exception(c4::formatrs<std::string>("Tried to deserialize {}. {} at line {}:{}", what, loc.name, loc.line, loc.col));
	}
	else
	{
		throw Exception(c4::formatrs<std::string>("Tried to deserialize {}.", what));
	}
}


////////////////////////////////////////////////////////////
//					YamlRootNodeReader
////////////////////////////////////////////////////////////


YamlRootNodeReader::YamlRootNodeReader(const std::string& fullFilePath, bool onlyInfoHeader, bool resolveReferences) : YamlNodeReader(), _tree(new ryml::Tree())
{
	RawData data = onlyInfoHeader ? CrossPlatform::getYamlSaveHeaderRaw(fullFilePath) : CrossPlatform::readFileRaw(fullFilePath);
	ryml::csubstr str = ryml::csubstr((char*)data.data(), data.size());
	if (onlyInfoHeader)
		str = ryml::csubstr((char*)data.data(), str.find("\n---") + 1);
	Parse(str, fullFilePath, true, resolveReferences);
}

YamlRootNodeReader::YamlRootNodeReader(const RawData& data, const std::string& fileNameForError, bool resolveReferences) : YamlNodeReader(), _tree(new ryml::Tree())
{
	Parse(ryml::csubstr((char*)data.data(), data.size()), fileNameForError, true, resolveReferences);
}

YamlRootNodeReader::YamlRootNodeReader(const YamlString& yamlString, std::string description, bool resolveReferences) : YamlNodeReader(), _tree(new ryml::Tree())
{
	Parse(ryml::to_csubstr(yamlString.yaml), std::move(description), false, resolveReferences);
}

void YamlRootNodeReader::Parse(ryml::csubstr yaml, std::string fileNameForError, bool withNodeLocations, bool resoleReferences)
{
	if (yaml.len > 3 && yaml.first(3) == "\xEF\xBB\xBF") // skip UTF-8 BOM
		yaml = yaml.offs(3, 0);

	{
		// find only name of file, not whole path
		size_t pos = fileNameForError.find_last_of('/');
		if (pos != std::string::npos)
			fileNameForError.erase(0, pos + 1);
	}

	_eventHandler.reset(new ryml::EventHandlerTree(_tree->callbacks()));
	_parser.reset(new ryml::Parser(_eventHandler.get(), ryml::ParserOptions().locations(withNodeLocations)));

	_fileName = std::move(fileNameForError);
	_tree->reserve(yaml.len / 16);
	ryml::parse_in_arena(_parser.get(), ryml::to_csubstr(_fileName), yaml, _tree.get());
	if (resoleReferences)
		_tree->resolve();
	_node = _tree->crootref();

	// yaml file that start with "---\n" should not be consider a multi-document if there are no others "---\n"
	if (_node.is_stream())
	{
		if (_node.first_child() == _node.last_child())
		{
			_node = _node.first_child();
		}
	}

	_root = this;
	_invalid = _node.invalid();
}

YamlNodeReader YamlRootNodeReader::toBase() const
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


////////////////////////////////////////////////////////////
//					YamlNodeWriter
////////////////////////////////////////////////////////////


YamlNodeWriter::YamlNodeWriter(const YamlRootNodeWriter* root, ryml::NodeRef node) : _root(root), _node(node)
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

void YamlNodeWriter::setValueNull()
{
	_node.set_val("~");
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


////////////////////////////////////////////////////////////
//					YamlRootNodeWriter
////////////////////////////////////////////////////////////


YamlRootNodeWriter::YamlRootNodeWriter() : YamlNodeWriter(this, {}), _tree(new ryml::Tree())
{
	_node = _tree->rootref();
}

YamlRootNodeWriter::YamlRootNodeWriter(size_t bufferCapacity) : YamlNodeWriter(this, {}), _tree(new ryml::Tree(0, bufferCapacity))
{
	_node = _tree->rootref();
}

YamlNodeWriter YamlRootNodeWriter::toBase()
{
	return YamlNodeWriter(this, _node);
}

} // namespace YAML

} // namespace OpenXcom


////////////////////////////////////////////////////////////
//		Functions for overriding types form other lib
////////////////////////////////////////////////////////////


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


#ifdef OXCE_AUTO_TEST

#include <cassert>

static auto createRootReader(std::string s)
{
	return OpenXcom::YAML::YamlRootNodeReader(OpenXcom::YAML::YamlString{s}, "dummy");
};
template<typename T>
static auto throw_exception(T&& func)
{
	try
	{
		func();
		return false;
	}
	catch (...)
	{
		return true;
	}
};
template<typename T>
static auto no_exception(T&& func)
{
	try
	{
		return func();
	}
	catch (...)
	{
		return false;
	}
};
#define assert_exception(A) assert(throw_exception([&]{ (A); }));
#define assert_noexcept(A) assert(no_exception([&]{ return (A); }));

static auto dummyHackInitYaml = ([]
{
	OpenXcom::YAML::setGlobalErrorHandler(); //hack, main should do it but this code is called before main
	return 0;
})();

static auto dummyTestMultiDocYaml = ([]
{
	{
		auto reader = createRootReader("foo: [1, 2]");

		assert_noexcept(reader["foo"].isValid());
		assert_noexcept(!reader["bar"].isValid());
		assert_noexcept(reader.useIndex()["foo"].isValid());
		assert_noexcept(!reader.useIndex()["bar"].isValid());
	}

	{
		auto reader = createRootReader("---\nfoo: [1, 2]");

		assert_noexcept(reader["foo"].isValid());
		assert_noexcept(!reader["bar"].isValid());
		assert_noexcept(reader.useIndex()["foo"].isValid());
		assert_noexcept(!reader.useIndex()["bar"].isValid());
	}

	{
		auto reader = createRootReader("---\nfoo: [1, 2]\n---\nbar: 3");

		assert_noexcept(!reader["foo"].isValid());
		assert_noexcept(!reader["bar"].isValid());
		assert_exception(reader.useIndex()["foo"].isValid());

		assert_noexcept(reader[0]["foo"].isValid());
		assert_noexcept(reader[1]["bar"].isValid());
	}

	return 0;
})();

static auto dummyIndexing = ([]
{
	{
		auto reader = createRootReader("map: ~");

		assert_noexcept(reader["map"].useIndex().childrenCount() == 0);
	}

	{
		auto reader = createRootReader("map:");

		assert_noexcept(reader["map"].useIndex().childrenCount() == 0);
	}

	{
		auto reader = createRootReader("map: {}");

		assert_noexcept(reader["map"].useIndex().childrenCount() == 0);
	}

	{
		auto reader = createRootReader("map: {foo: 1}");

		assert_noexcept(reader["map"].useIndex().childrenCount() == 1);
	}

	{
		auto reader = createRootReader("map: []");

		assert_exception(reader["map"].useIndex().isMap());
	}

	{
		auto reader = createRootReader("map: [2]");

		assert_exception(reader["map"].useIndex().isMap());
	}

	{
		auto reader = createRootReader("map: 2");

		assert_exception(reader["map"].useIndex().isMap());
	}

	{
		auto reader = createRootReader("map: bar");

		assert_exception(reader["map"].useIndex().isMap());
	}

	return 0;
})();


static auto dummyTestRead = ([]
{


	// pair tests
	{
		auto reader = createRootReader("foo2: [1, 2]");
		std::pair<int, int> p;

		assert_noexcept(!reader.tryRead("foo", p));
	}

	{
		auto reader = createRootReader("foo: [1, 2]");
		std::pair<int, int> p;

		assert_noexcept(reader.tryRead("foo", p));
		assert(p.first == 1);
		assert(p.second == 2);
	}

	{
		auto reader = createRootReader("foo: [1, 2, 3]");
		std::pair<int, int> p;

		assert_exception(reader.tryRead("foo", p));
	}

	{
		auto reader = createRootReader("foo: [1]");
		std::pair<int, int> p;

		assert_exception(reader.tryRead("foo", p));
	}

	{
		auto reader = createRootReader("foo: 2");
		std::pair<int, int> p;

		assert_exception(reader.tryRead("foo", p));
	}


	// tuple tests
	{
		auto reader = createRootReader("foo2: [1, 2]");
		std::tuple<int, int> p;

		assert_noexcept(!reader.tryRead("foo", p));
	}

	{
		auto reader = createRootReader("foo: [1, 2, 32]");
		std::tuple<int, int, int> p;

		assert(reader.tryRead("foo", p));
		assert(std::get<0>(p) == 1);
		assert(std::get<1>(p) == 2);
		assert(std::get<2>(p) == 32);
	}

	{
		auto reader = createRootReader("foo: [1, 2]");
		std::tuple<int, int> p;

		assert_noexcept(reader.tryRead("foo", p));
		assert(std::get<0>(p) == 1);
		assert(std::get<1>(p) == 2);
	}

	{
		auto reader = createRootReader("foo: [13]");
		std::tuple<int> p;

		assert(reader.tryRead("foo", p));
		assert(std::get<0>(p) == 13);
	}

	{
		auto reader = createRootReader("foo: [1]");
		std::tuple<int, int> p;

		assert_exception(reader.tryRead("foo", p));
	}

	{
		auto reader = createRootReader("foo: [1, 2, 3]");
		std::tuple<int, int> p;

		assert_exception(reader.tryRead("foo", p));
	}


	// array
	{
		auto reader = createRootReader("foo2: [1, 2]");
		std::array<int, 2> p;

		assert(!reader.tryRead("foo", p));
	}

	{
		auto reader = createRootReader("foo: [1, 2]");
		std::array<int, 2> p;

		assert_noexcept(reader.tryRead("foo", p));
		assert(std::get<0>(p) == 1);
		assert(std::get<1>(p) == 2);
	}

	{
		auto reader = createRootReader("foo: [1, 2, 42, 13]");
		std::array<int, 4> p;

		assert_noexcept(reader.tryRead("foo", p));
		assert(std::get<0>(p) == 1);
		assert(std::get<1>(p) == 2);
		assert(std::get<2>(p) == 42);
		assert(std::get<3>(p) == 13);
	}

	{
		auto reader = createRootReader("foo: [1, 2, 3]");
		std::array<int, 2> p;

		assert_exception(reader.tryRead("foo", p));
	}

	{
		auto reader = createRootReader("foo: [1]");
		std::array<int, 2> p;

		assert_exception(reader.tryRead("foo", p));
	}


	return 0;
})();

#endif
