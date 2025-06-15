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


C4_NORETURN static void RootReader_error(const char* msg, size_t len, ryml::Location loc, void* this_)
{
	const auto& globalCallback = ryml::get_callbacks();
	globalCallback.m_error(msg, len, loc, globalCallback.m_user_data);
	C4_UNREACHABLE();
}

static ryml::Callbacks callbacksForRootReader(const YamlRootNodeReader* root)
{
	return ryml::Callbacks(const_cast<YamlRootNodeReader*>(root), s_allocate, s_free, RootReader_error);
}

static const YamlRootNodeReader* getRootReaderData(const ryml::Callbacks& callbacks)
{
	return callbacks.m_error == RootReader_error ? static_cast<const YamlRootNodeReader*>(callbacks.m_user_data) : nullptr;
}


C4_NORETURN static void RootWriter_error(const char* msg, size_t len, ryml::Location loc, void* this_)
{
	const auto& globalCallback = ryml::get_callbacks();
	globalCallback.m_error(msg, len, loc, globalCallback.m_user_data);
	C4_UNREACHABLE();
}

static ryml::Callbacks callbacksForRootWriter(const YamlRootNodeWriter* root)
{
	return ryml::Callbacks(const_cast<YamlRootNodeWriter*>(root), s_allocate, s_free, RootWriter_error);
}

static const YamlRootNodeWriter* getRootWriterData(const ryml::Callbacks& callbacks)
{
	return callbacks.m_error == RootWriter_error ?  static_cast<const YamlRootNodeWriter*>(callbacks.m_user_data) : nullptr;
}

////////////////////////////////////////////////////////////
//					YamlNodeReader
////////////////////////////////////////////////////////////


YamlNodeReader::YamlNodeReader()
	: _node(ryml::ConstNodeRef(nullptr, ryml::NONE)), _nextChildId(ryml::NONE)
{
}

YamlNodeReader::YamlNodeReader(const ryml::ConstNodeRef& node)
	: _node(node), _nextChildId(ryml::NONE)
{

}

YamlNodeReader::YamlNodeReader(const ryml::ConstNodeRef& node, bool useIndex) : YamlNodeReader( node)
{
	if (!useIndex || _node.invalid())
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
	return YamlNodeReader(_node, true);
}

std::string_view YamlNodeReader::key() const
{
	if (_node.invalid())
		throw Exception("Tried getting key of an invalid node!");
	const auto& key = _node.tree()->get(_node.m_id)->m_key.scalar;
	return std::string_view(key.str, key.len);
}

std::string_view YamlNodeReader::val() const
{
	if (_node.invalid())
		throw Exception("Tried getting value of an invalid node!");
	const auto& val = _node.tree()->get(_node.m_id)->m_val.scalar;
	return std::string_view(val.str, val.len);
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
	if (_node.invalid())
		return 0;
	return _index ? _index->size() : _node.num_children();
}

ryml::ConstNodeRef YamlNodeReader::getChildNode(const ryml::csubstr& key) const
{
	if (_node.invalid())
		return ryml::ConstNodeRef(_node.tree(), ryml::NONE);
	if (!_index)
	{
		if (!_node.is_map())
			return ryml::ConstNodeRef(_node.tree(), ryml::NONE);
		return findChildNode(key);
	}
	if (const auto& keyNodeIdPair = _index->find(key); keyNodeIdPair != _index->end())
		return _node.tree()->cref(keyNodeIdPair->second);
	return ryml::ConstNodeRef(_node.tree(), ryml::NONE);
}

ryml::ConstNodeRef YamlNodeReader::findChildNode(const ryml::csubstr& key) const
{
	ryml::id_type firstChildId = _node.m_tree->get(_node.m_id)->m_first_child;
	if (firstChildId == ryml::NONE)
		return ryml::ConstNodeRef(_node.m_tree, ryml::NONE);
	if (_nextChildId == ryml::NONE)
	{ // do a normal search
		for (ryml::id_type i = firstChildId; i != ryml::NONE;)
		{
			const ryml::NodeData* data = _node.m_tree->_p(i);
			if (data->m_key.scalar == key)
			{
				_nextChildId = data->m_next_sibling;
				return ryml::ConstNodeRef(_node.m_tree, i);
			}
			i = data->m_next_sibling;
		}
		return ryml::ConstNodeRef(_node.m_tree, ryml::NONE);
	}
	// search from saved iterator to the last child
	for (ryml::id_type i = _nextChildId; i != ryml::NONE;)
	{
		const ryml::NodeData* data = _node.m_tree->_p(i);
		if (data->m_key.scalar == key)
		{
			_nextChildId = data->m_next_sibling;
			return ryml::ConstNodeRef(_node.m_tree, i);
		}
		i = data->m_next_sibling;
	}
	// search from the first child to the saved iterator
	for (ryml::id_type i = firstChildId; i != _nextChildId;)
	{
		const ryml::NodeData* data = _node.m_tree->_p(i);
		if (data->m_key.scalar == key)
		{
			_nextChildId = data->m_next_sibling;
			return ryml::ConstNodeRef(_node.m_tree, i);
		}
		i = data->m_next_sibling;
	}
	return ryml::ConstNodeRef(_node.m_tree, ryml::NONE);
}

std::vector<YamlNodeReader> YamlNodeReader::children() const
{
	std::vector<YamlNodeReader> children;
	if (_node.invalid())
		return children;
	children.reserve(_node.num_children());
	for (const ryml::ConstNodeRef child : _node.cchildren())
		children.emplace_back(child);
	return children;
}

bool YamlNodeReader::isValid() const
{
	return !_node.invalid();
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
	if (_node.invalid() || !_node.has_val_tag())
		return false;
	return ryml::to_tag(_node.val_tag()) == tag;
}

bool YamlNodeReader::hasValTag(const std::string& tagName) const
{
	if (_node.invalid() || !_node.has_val_tag())
		return false;
	return _node.val_tag() == tagName;
}

std::string YamlNodeReader::getValTag() const
{
	if (_node.invalid() || !_node.has_val_tag())
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
	writer._tree->duplicate_children(_node.tree(), _node.id(), writer._node.id(), ryml::NONE);
	if (defaultValuesReader)
		for (const ryml::ConstNodeRef child : defaultValuesReader._node.cchildren())
			if (writer._node.find_child(child.key()).invalid())
				writer._tree.get()->duplicate(child.tree(), child.id(), writer._node.id(), writer._node.last_child().id());
	return writer.emit();
}

ryml::Location YamlNodeReader::getLocationInFile() const
{
	auto root = _node.tree() ? getRootReaderData(_node.tree()->callbacks()) : nullptr;
	return root ? root->getLocationInFile(_node) : ryml::Location{};
}

YamlNodeReader YamlNodeReader::operator[](ryml::csubstr key) const
{
	return YamlNodeReader(getChildNode(key));
}

YamlNodeReader YamlNodeReader::operator[](size_t pos) const
{
	if (_node.invalid())
		return YamlNodeReader(ryml::ConstNodeRef(_node.tree(), ryml::NONE));
	return YamlNodeReader(_node.child(pos));
}

YamlNodeReader::operator bool() const
{
	return !_node.invalid();
}


void YamlNodeReader::throwTypeError(const ryml::ConstNodeRef& node, const ryml::cspan<char>& type) const
{
	auto name = ryml::csubstr(type.data(), type.size() - (type.back() == 0));
	if (node.readable())
	{
		ryml::Location loc = YamlNodeReader{node}.getLocationInFile();
		throw Exception(c4::formatrs<std::string>("Could not deserialize value to type <{}>! {} at line {}:{}", name, loc.name, loc.line, loc.col));
	}
	else
	{
		throw Exception(c4::formatrs<std::string>("Could not deserialize value to type <{}>!", name));
	}
}

void YamlNodeReader::throwNodeError(const std::string& what) const
{
	if (_node.readable())
	{
		ryml::Location loc = getLocationInFile();
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


YamlRootNodeReader::YamlRootNodeReader(const std::string& fullFilePath, bool onlyInfoHeader, bool resolveReferences) : YamlNodeReader(), _tree(new ryml::Tree(callbacksForRootReader(this)))
{
	RawData data = onlyInfoHeader ? CrossPlatform::getYamlSaveHeaderRaw(fullFilePath) : CrossPlatform::readFileRaw(fullFilePath);
	ryml::csubstr str = ryml::csubstr((char*)data.data(), data.size());
	if (onlyInfoHeader)
		str = ryml::csubstr((char*)data.data(), str.find("\n---") + 1);
	Parse(str, fullFilePath, true, resolveReferences);
}

YamlRootNodeReader::YamlRootNodeReader(const RawData& data, const std::string& fileNameForError, bool resolveReferences) : YamlNodeReader(), _tree(new ryml::Tree(callbacksForRootReader(this)))
{
	Parse(ryml::csubstr((char*)data.data(), data.size()), fileNameForError, true, resolveReferences);
}

YamlRootNodeReader::YamlRootNodeReader(const YamlString& yamlString, std::string description, bool resolveReferences) : YamlNodeReader(), _tree(new ryml::Tree(callbacksForRootReader(this)))
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
	// if the yaml file is an empty document, it's parsed into a single node with type NOTYPE
	if (_tree->size() == 1 && _node.type() == ryml::NOTYPE)
		_node = ryml::ConstNodeRef(_tree.get(), ryml::NONE); // treat it as an invalid node
}

YamlNodeReader YamlRootNodeReader::toBase() const
{
	return YamlNodeReader(_node);
}

ryml::Location YamlRootNodeReader::getLocationInFile(const ryml::ConstNodeRef& node) const
{
	if (_parser)
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


YamlNodeWriter::YamlNodeWriter(ryml::NodeRef node) : _node(node)
{

}

YamlNodeReader YamlNodeWriter::toReader()
{
	return YamlNodeReader(_node);
}

YamlNodeWriter YamlNodeWriter::write()
{
	return YamlNodeWriter(_node.append_child());
}

YamlNodeWriter YamlNodeWriter::operator[](ryml::csubstr key)
{
	return YamlNodeWriter(_node.append_child({ryml::KEY, key}));
}

YamlNodeWriter YamlNodeWriter::writeBase64(ryml::csubstr key, char* data, size_t size)
{
	return YamlNodeWriter(_node.append_child({ryml::KEY, key}) << c4::fmt::base64(ryml::csubstr(data, size)));
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

bool isPrintable(uint8_t c)
{
	// TAB, LF, CR, and x20-x7E range; Any 2+ byte UTF-8 characters are assumed to be printable
	return c == 0x09 || c == 0x0A || c == 0x0D || (c >= 0x20 && c <= 0x7E) || c >= 0x80;
}

void YamlNodeWriter::setAsQuotedAndEscaped()
{
	ryml::csubstr scalar = _node.val();
	size_t pos, last = 0;
	for (pos = 0; pos < scalar.len; pos++)
		if (!isPrintable(scalar[pos]))
			break;
	if (pos == scalar.len) // didn't find a non-printable ASCII character;
	{
		_node |= ryml::VAL_DQUO;
		return;
	}
	// manually create a double-quoted scalar; escape the non-printable AND special characters
	_node |= ryml::VAL_PLAIN;
	const char hexDigits[] = "0123456789ABCDEF";
	std::string newScalar;
	newScalar.reserve(scalar.len + 2 + 3); // 2 for quotes and 3 for one hex escape
	newScalar.push_back('\"');
	for (pos = 0; pos < scalar.len; pos++)
	{
		char toEscape = 0;
		switch (scalar[pos])
		{
		case '"': toEscape = '\"'; break;
		case '\\': toEscape = '\\'; break;
		case '\n': toEscape = 'n'; break;
		case '\r': toEscape = 'r'; break;
		case '\b': toEscape = 'b'; break;
		default: if (!isPrintable(scalar[pos])) toEscape = 'x'; break;
		}
		if (toEscape != 0)
		{
			newScalar.append(&scalar[last], pos - last); // copy from the last escaped to the current position
			newScalar.push_back('\\');
			newScalar.push_back(toEscape);
			if (toEscape == 'x')
			{
				newScalar.push_back(hexDigits[(scalar[pos] >> 4) & 0xF]);
				newScalar.push_back(hexDigits[scalar[pos] & 0xF]);
			}
			last = pos + 1;
		}
	}
	if (pos != last)
		newScalar.append(&scalar[last], pos - last); // copy from the last escaped to the end of string
	newScalar.push_back('\"');
	_node.set_val(_node.tree()->to_arena(newScalar));
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


YamlRootNodeWriter::YamlRootNodeWriter() : YamlNodeWriter(ryml::NodeRef{}), _tree(new ryml::Tree(callbacksForRootWriter(this)))
{
	_node = _tree->rootref();
}

YamlRootNodeWriter::YamlRootNodeWriter(size_t bufferCapacity) : YamlNodeWriter(ryml::NodeRef{}), _tree(new ryml::Tree(0, bufferCapacity, callbacksForRootWriter(this)))
{
	_node = _tree->rootref();
}

YamlNodeWriter YamlRootNodeWriter::toBase()
{
	return YamlNodeWriter(_node);
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


#ifndef NDEBUG

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


	{
		auto reader = createRootReader("foo: 1");
		OpenXcom::NullableValue<int> p;

		assert_noexcept(reader.tryRead("foo", p));
		assert(p.isValue());
		assert(p.getValueOrDefualt() == 1);
	}

	{
		auto reader = createRootReader("foo: -42");
		OpenXcom::NullableValue<int> p;

		assert_noexcept(reader.tryRead("foo", p));
		assert(p.isValue());
		assert(p.getValueOrDefualt() == -42);
	}

	{
		auto reader = createRootReader("foo: ~");
		OpenXcom::NullableValue<int> p;

		assert_noexcept(reader.tryRead("foo", p));
		assert(p.isNull());
		assert(p.getValueOrDefualt() == 0);
	}

	{
		auto reader = createRootReader("foo: []");
		OpenXcom::NullableValue<int> p;

		assert_exception(reader.tryRead("foo", p));
	}


	{
		auto reader = createRootReader("foo: true");
		OpenXcom::NullableValue<bool> p;

		assert_noexcept(reader.tryRead("foo", p));
		assert(p.isValue());
		assert(p.getValueOrDefualt() == true);
	}


	{
		auto reader = createRootReader("foo: false");
		OpenXcom::NullableValue<bool> p;

		assert_noexcept(reader.tryRead("foo", p));
		assert(p.isValue());
		assert(p.getValueOrDefualt() == false);
	}

	{
		auto reader = createRootReader("foo: ~");
		OpenXcom::NullableValue<bool> p;

		assert_noexcept(reader.tryRead("foo", p));
		assert(p.isNull());
		assert(p.getValueOrDefualt() == false);
	}

	{
		auto reader = createRootReader("foo: []");
		OpenXcom::NullableValue<bool> p;

		assert_exception(reader.tryRead("foo", p));
	}


	return 0;
})();

#endif
