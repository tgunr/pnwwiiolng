#include <syscfg.h>
// Object Model Generator v2

#include <cstdio>
#include <iostream>
#include <string>
#include <cstdarg>
#include <vector>
#include <set>

#include "omg.h"
#include "omg.yy.h"
#include "omg.tab.h"

static const char* s_exeName;
static std::string s_cxxFilename = "";
static std::string s_hxxFilename = "";
static FILE* s_hxx;
static FILE* s_cxx;

static bool s_debug = false;
static std::string s_namespace = "";
static std::string s_namespaceQual = "";
static std::set<std::string> s_includes;

const char* g_currentFile = nullptr;

enum { MAX_MESSAGE_GROUPS = 32, MAX_GROUP_FIELDS = 32 };

typedef std::vector<TypeDef> TypeMap;
static TypeMap s_typeMap {
	{ "UINT8", "", false, "" },
	{ "SINT8", "", false, "" },
	{ "UINT16", "", false, "" },
	{ "SINT16", "", false, "" },
	{ "UINT32", "", false, "" },
	{ "SINT32", "", false, "" },
	{ "UINT64", "", false, "" },
	{ "SINT64", "", false, "" },
	{ "FLOAT", "", false, "" },
	{ "DOUBLE", "", false, "" },
	{ "FULL_OBJ_ID", "", true, "wwii/wwiiFullObjIDPacker.h" },
	{ "WEAPON_SPEC", "", true, "weap/weapSpecPacker.h" },
	{ "WEAPON_SERIAL", "", true, "" },
	{ "RANK_NO", "", false, "" },
	{ "ChatHandle", "", false, "" },
	{ "NetTimeMS", "Teul::NetTimeMS", false, "teul/teulNetTimeMS.h" },
	{ "TeulTimeMS", "", false, "" },
	{ "TeulDeltaMS", "", false, "" },
	{ "PermissionMask", "", false, "" },
	{ "playerid_t", "", false, "" },
	{ "instanceid_t", "", false, "" },
};

static unsigned int s_msgX;					// X of messages defined in this file.
static unsigned int s_msgY;					// next msgY to assign.

static const char s_indents[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
static const size_t MAX_INDENTS = sizeof(s_indents) -1;

struct Node
{
	enum class Type
	{
		Message
	,	Group
	,	Field
	,	Condition
	};

	Node*				m_next;
	Node*				m_parent;
	Node*				m_child;
	Node*				m_lastChild;

	Type				m_type;
	std::string			m_name;

	Node(Type type, const char* name, Node* parent)
		: m_next(nullptr)
		, m_parent(parent)
		, m_child(nullptr)
		, m_lastChild(nullptr)
		, m_type(type)
		, m_name(name)
	{
		if (m_parent) {
			if (m_parent->m_lastChild)
				m_parent->m_lastChild->m_next = this;
			else
				m_parent->m_child = this;
			m_parent->m_lastChild = this;
		}
	}

	virtual void Emit() const
	{
		yyerror("Internal Error: Reached Node->Emit();\n");
		exit(9);
	}

	virtual void EmitPack(size_t /*indent level*/) const
	{
	}

};

struct Message : public Node
{
	unsigned int		m_x;			// X to use for this.
	unsigned int		m_y;			// assign a Y value
	std::string			m_code;			// additional code
	unsigned int		m_numGroups;
	struct Field*		m_extraMembers;	// additional members

	Message(const char* name)
		: Node(Node::Type::Message, name, nullptr)
		, m_x(s_msgX)
		, m_y(0)
		, m_code()
		, m_numGroups(0)
		, m_extraMembers(nullptr)
	{}

	void AddExtraMember(const char* type, const char* name, const char* defaultValue);

	void Emit() const override;
	void EmitConstructor() const;
	void EmitPacker() const;
	void EmitGroupPackers() const;
	void EmitExtraMembers() const;
	void EmitPublicMembers() const;

	bool IsDeltad() const { return m_numGroups > 0; }
};

struct Group : public Node
{
	std::string			m_memberName;
	std::string			m_extraCode;

	Group(const char* name, Node* parent)
		: Node(Node::Type::Group, name, parent)
		, m_memberName("m_" + m_name)
		, m_extraCode("")
	{
		makeMemberName(m_memberName);
	}

	static void makeMemberName(std::string& field)
	{
		for (auto it = field.begin() + 2, end = field.end(); it != end && isupper(*it); ++it) {
			*it = static_cast<char>(tolower(*it));
		}
	}

	void Emit() const override;
};

struct Field : public Node
{
	const TypeDef*		m_typeDef;		// Variable type
	size_t				m_size;			// bits, length, etc.
	const char*			m_defaultValue;

	Field(const TypeDef* typeDef, const char* name, Node* parent)
		: Node(Node::Type::Field, name, parent) 
		, m_typeDef(typeDef)
		, m_size(0)
		, m_defaultValue("")
	{
	}

	void Emit() const override;
	void EmitPack(size_t indent) const override;
};

struct Conditional : public Node
{
	std::string			m_condition;

	Conditional(const char* condition, Node* parent)
		: Node(Node::Type::Condition, "if", parent)
		, m_condition(condition)
	{
	}

	void Emit() const override;
	void EmitPack(size_t indent) const override;
};


static Message* s_firstMessage;
static Message* s_lastMessage;
static Node*	s_currentNode;

//////////////////////////////////////////////////////////////////////

#define HXX(...) fprintf(s_hxx, __VA_ARGS__)
#define CXX(...) fprintf(s_cxx, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////

template<typename... Args>
void annotate(const char* fmt, Args... args)
{
	if (s_debug) {
		fprintf(stdout, "%s:%u: ", g_currentFile, yylineno);
		fprintf(stdout, fmt, std::forward<Args>(args)...);
		fprintf(stdout, "\n");
	}
}

void yyerror(const char *s, ...)
{
	fprintf(stderr, "%s:%u: error: ", g_currentFile, yylineno);
	va_list ap;
	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

void yyfatal(const char* s, const char* reason)
{
	yyerror(s, reason);
	exit(1);
}

void yymissing(const char* what, const char* type, const char* reason)
{
	yyerror("Expected %s after %s '%s'.", what, type, reason);
	exit(1);
}

void yyaddcode(const char* text, std::string& into)
{
	const bool isNewLine = (into.empty() || (into.back() == '\n'));
	if (isNewLine) {
		if (*text == '\n')
			return;
	}
	into += *text;
}

static const TypeDef* _findTypeDef(const char* name)
{
	for (auto& it : s_typeMap) {
		if (it.Name() == name)
			return &it;
	}
	return nullptr;
}

static const char* _getIndents(size_t indentLevel)
{
	return s_indents + (MAX_INDENTS - std::min(indentLevel, MAX_INDENTS));
}

//////////////////////////////////////////////////////////////////////

void omg_enable_debug()
{
	s_debug = true;
	annotate("Debugging output enabled");
}

void omg_type_decl(TypeDef&& typeDef)
{
	if (_findTypeDef(typeDef.Name().c_str()))
		yyfatal("redefinition of %s", typeDef.Name().c_str());
	annotate("Added new type: %s", typeDef.Name().c_str());
	s_typeMap.push_back(std::move(typeDef));
}

void omg_namespace(const char* name)
{
	annotate("Changing namespace from '%s' to '%s'", s_namespace.c_str(), name);
	s_namespace = name;
	s_namespaceQual = name;
	s_namespaceQual += "::";
}

void omg_start_message(const char* name)
{
	// did we already create a message with this name?
	for (Message* message = s_firstMessage; message; message = static_cast<Message*>(message->m_next)) {
		if (message->m_name == name)
			yyfatal("Redefinition of MESSAGE '%s'", name);
	}

	if (s_msgY == 255)
		yyfatal("Exceeded MessageY 255 for message '%s'", name);

	if (islower(name[0]))
		yyfatal("MESSAGE names must start with a capital letter ('%s')", name);

	Message* message = new Message(name);
	message->m_x = s_msgX;
	message->m_y = s_msgY++;

	if (!s_firstMessage)
		s_firstMessage = message;
	else
		s_lastMessage->m_next = message;
	s_lastMessage = message;

	annotate("Declaring message '%s' as %u", name, s_msgY);
	s_currentNode = message;
}

void omg_end_message()
{
	annotate("Completed definition of message '%s'", s_lastMessage->m_name.c_str());
	s_currentNode = s_currentNode->m_parent;
}

void omg_start_group(const char* name)
{
	for (auto node = s_currentNode; node; node = node->m_parent) {
		if (node->m_type == Node::Type::Group) {
			yyfatal("Cannot nest GROUPs, sorry.", name);
		}
	}

	if (islower(name[0]))
		yyfatal("GROUP names must start with a capital letter ('%s')", name);

	for (Node* node = s_currentNode->m_child; node; node = node->m_next) {
		if (node->m_name == name)
			yyfatal("Redefinition of '%s' in this scope", name);
	}

	if (s_lastMessage->m_numGroups >= MAX_MESSAGE_GROUPS) {
		yyerror("MESSAGE '%s' contains more than %u GROUP definitions.", s_lastMessage->m_name.c_str(), MAX_MESSAGE_GROUPS);
		exit(1);
	}

	Group* group = new Group(name, s_currentNode);

	s_lastMessage->m_numGroups++;
	if (s_lastMessage->m_numGroups == 1)
		s_lastMessage->AddExtraMember("TeulTimeMS", "m_lastModified", "0ULL");

	annotate("Adding group '%s'", name);
	s_currentNode = group;
	s_includes.insert("wwii/wwiiSyncVar.h");
}

void omg_end_group()
{
	if (s_currentNode->m_child == nullptr)
		yyfatal("GROUP with empty definition, '%s'", s_currentNode->m_name.c_str());

	annotate("Ended group %s", s_currentNode->m_name.c_str());
	s_currentNode = s_currentNode->m_parent;
}

void omg_start_field(const char* type, const char* name)
{
	for (Node* node = s_currentNode->m_child; node; node = node->m_next) {
		if (node->m_name == name)
			yyfatal("Redefinition of '%s' in this scope", name);
	}

	if (isupper(name[0]))
		yyfatal("Field names must start with a lowercase letter ('%s')", name);

	const TypeDef* typeDef = _findTypeDef(type);
	if (typeDef == nullptr)
		yyfatal("Unrecognized type: %s", type);

	Field* field = new Field(typeDef, name, s_currentNode);

	annotate("Adding field %s %s;", type, name);
	s_currentNode = field;
	if (typeDef->Include().empty() == false)
		s_includes.insert(typeDef->Include());
}

void omg_end_field()
{
	annotate("End of field");
	s_currentNode = s_currentNode->m_parent;
}

void omg_start_if(const char* condition)
{
	annotate("if (%s) ...", condition);
	Conditional* cond = new Conditional(condition, s_currentNode);
	s_currentNode = cond;
}

void omg_end_if()
{
	annotate("endif()");
	s_currentNode = s_currentNode->m_parent;
}

void omg_add_code(const char* code)
{
	annotate("adding code");
	// trim off leading blank lines.
	const char* ptr = code;
	for (const char* ptr = code; *ptr && isspace(*ptr); ++ptr) {
		if (*ptr == '\n')
			code = ptr + 1;
	}
	size_t len = strlen(code);
	while (len > 0 && isspace(code[len - 1]))
		--len;
	switch (s_currentNode->m_type) {
		case Node::Type::Message:
			static_cast<Message*>(s_currentNode)->m_code.append(code, len);;
			break;
		case Node::Type::Group:
			static_cast<Group*>(s_currentNode)->m_extraCode.append(code, len);
			break;
	}
}

//////////////////////////////////////////////////////////////////////

void Message::AddExtraMember(const char* type, const char* name, const char* defaultValue)
{
	const TypeDef* typeDef = _findTypeDef(type);
	if (typeDef == nullptr)
		yyfatal("INTERNAL ERROR: Unknown type '%s'", type);

	// extra members are added in reverse order
	Field* newField = new Field(typeDef, name, nullptr);
	newField->m_defaultValue = defaultValue;
	newField->m_next = m_extraMembers;
	m_extraMembers = newField;
}

//////////////////////////////////////////////////////////////////////

void Message::Emit() const
{
	HXX(
		"struct %s\n"
		"{\n"
		, m_name.c_str()
		);

	HXX("\tenum { MessageY = %u };\n", m_y);

	EmitConstructor();
	EmitPacker();

	EmitExtraMembers();
	EmitPublicMembers();

	if (!m_code.empty()) {
		HXX(m_code.c_str());
		HXX("\n");
	}
	HXX("}; // end message %s\n"
		"\n"
		, m_name.c_str()
		);
}

void Message::EmitConstructor() const
{
	HXX("\tpublic: %s()\n", m_name.c_str());
	char joiner = ':';
	for (Field* field = m_extraMembers; field; field = static_cast<Field*>(field->m_next)) {
		HXX("\t\t%c %s(%s)\n", joiner, field->m_name.c_str(), field->m_defaultValue);
		joiner = ',';
	}
	for (Node* node = m_child; node; node = node->m_next) {
		switch (node->m_type) {
			case Node::Type::Group:
				HXX("\t\t%c %s(%s)\n", joiner, static_cast<Group*>(node)->m_memberName.c_str(), "m_lastModified");
				joiner = ',';
				break;
			case Node::Type::Field:
				HXX("\t\t%c %s(%s)\n", joiner, node->m_name.c_str(), static_cast<Field*>(node)->m_defaultValue);
				joiner = ',';
				break;
			default:
				break;
		}
	}
	HXX("\t{\n"
		"\t}\n"
		"\n"
		);
}

void Message::EmitPacker() const
{
	if (m_child == nullptr) {
		HXX("\tpublic: bool packer(LOGICAL /*mode*/, ACTION_OBJECT* /*a*/, SINT32 /*size*/) { return true; }\n");
		return;
	}
	const char* fingerprint;
	if (IsDeltad()) {
		EmitGroupPackers();
		fingerprint = "packer(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack)";
	} else {
		fingerprint = "packer(LOGICAL mode, ACTION_OBJECT* const a, SINT32 size)";
	}
	HXX("\tpublic: bool %s;\n", fingerprint);
	CXX("bool %s%s::%s\n", s_namespaceQual.c_str(), m_name.c_str(), fingerprint);
	CXX("{\n");
	for (Node* node = m_child; node; node = node->m_next) {
		node->EmitPack(1);
	}
	if (m_numGroups != 0)
		CXX("\treturn (mode == TEUL_PACK) ? _pack(a, lastPack) : _unpack(a, lastPack);\n");
	else
		CXX("\treturn (a->offset >= 0);\n");
	CXX("}\n"
		"\n"
		);
}

void Message::EmitGroupPackers() const
{
	size_t groupPresenceBits = (m_numGroups & ~7) + 8;
	const char* groupPresenceType;
	if (groupPresenceBits <= 8)
		groupPresenceType = "UCHAR";
	else if (groupPresenceBits <= 16)
		groupPresenceType = "UINT16";
	else if (groupPresenceBits <= 32)
		groupPresenceType = "UINT32";
	else if (groupPresenceBits <= 64)
		groupPresenceType = "UINT64";
	else
		yyfatal("Internal Error: %s", "group presence bits > 64");

	HXX(
		"\tpublic: enum { GroupPresenceBits = %u };\n"
		"\tpublic: const TeulTimeMS& lastModified() const { return m_lastModified; }\n"
		"\tprivate: bool _unpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack);\n"
		"\tprivate: bool _pack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack);\n"
		, groupPresenceBits
	);

	// unpack delta function
	CXX(
		"bool %s%s::_unpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack)\n"
		"{\n"
		"\t%s groupPresence(0);\n"
		"\ta->packer(TEUL_UNPACK, groupPresence, %" PRSizeT ");\n"
		"\tif (groupPresence == 0)\n"
		"\t\treturn false;\n"
		, s_namespaceQual.c_str(), m_name.c_str()
		, groupPresenceType
		, groupPresenceBits
		);
	size_t groupNo = 0;
	for (Node* node = m_child; node; node = node->m_next) {
		if (node->m_type != Node::Type::Group)
			continue;
		Group* group = static_cast<Group*>(node);
		CXX("\tif (groupPresence & (1 << %" PRSizeT "))\n"
			"\t\t%s.packer(TEUL_UNPACK, a, lastPack);\n"
			, groupNo++
			, group->m_memberName.c_str()
			);
	}
	CXX(
		"\treturn (a->offset >= 0);\n"
		"}\n"
		"\n"
		);

	// pack delta function
	CXX(
		"bool %s%s::_pack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack)\n"
		"{\n"
		"\t%s groupPresence(0);\n"
		"\tSINT32 groupPresenceOffset = a->offset;\n"
		"\ta->packer(TEUL_PACK, groupPresence, %" PRSizeT ");\n"
		, s_namespaceQual.c_str(), m_name.c_str()
		, groupPresenceType
		, groupPresenceBits
		);
	groupNo = 0;
	for (Node* node = m_child; node; node = node->m_next) {
		if (node->m_type != Node::Type::Group)
			continue;
		Group* group = static_cast<Group*>(node);
		CXX("\tif (%s.packer(TEUL_PACK, a, lastPack))\n"
			"\t\tgroupPresence |= (1 << %" PRSizeT ");\n"
			, group->m_memberName.c_str()
			, groupNo++
			);
	}
	CXX(
		"\tif (groupPresence == 0)\n"
		"\t\treturn false;\n"
		"\ta->offset = teulPack%s(TEUL_PACK, a->packed, groupPresenceOffset, &groupPresence, %" PRSizeT ");\n"
		"\treturn (a->offset >= 0);\n"
		"}\n"
		"\n"
		, groupPresenceType
		, groupPresenceBits
		);
}

void Message::EmitExtraMembers() const
{
	if (!m_extraMembers)
		return;
	HXX("\tprotected:\n");
	for (Field* field = m_extraMembers; field; field = static_cast<Field*>(field->m_next)) {
		field->Emit();
	}
	HXX("\n");
}

void Message::EmitPublicMembers() const
{
	if (!m_child)
		return;
	HXX("\tpublic:\n");
	for (Node* node = m_child; node; node = node->m_next) {
		node->Emit();
	}
}

//////////////////////////////////////////////////////////////////////

void Group::Emit() const
{
	std::string fqn(s_namespaceQual);
	if (m_parent) {
		fqn += m_parent->m_name;
		fqn += "::";
	}
	fqn += m_name;

	HXX(
		"\tstruct %s : public WWII::VarGroup\n"
		"\t{\n"
		"\t\t%s(TeulTimeMS& parent)\n"
		"\t\t\t: VarGroup(parent)\n"
		, m_name.c_str()
		, m_name.c_str()
		);
	for (Field* child = static_cast<Field*>(m_child); child; child = static_cast<Field*>(child->m_next)) {
		HXX("\t\t\t, %s(this)\n", child->m_name.c_str());
	}
	HXX("\t\t{}\n");

	HXX("\t\t%s& operator = (const %s& rhs);\n", m_name.c_str(), m_name.c_str());
	CXX(
		"%s& %s::operator = (const %s& rhs)\n"
		"{\n"
		"\tm_lastModified = rhs.m_lastModified;\n"
		, fqn.c_str()
		, fqn.c_str()
		, fqn.c_str()
		);
	for (Field* child = static_cast<Field*>(m_child); child; child = static_cast<Field*>(child->m_next)) {
		CXX("\t%s.operator=(rhs.%s);\n", child->m_name.c_str(), child->m_name.c_str());
	}
	CXX(
		"\treturn *this;\n"
		"}\n"
		"\n"
		);

	HXX("\t\tvoid groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes) override;\n");
	CXX(
		"void %s::groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)\n"
		"{\n"
		, fqn.c_str()
		);
	size_t fieldNo = 0;
	for (Field* child = static_cast<Field*>(m_child); child; child = static_cast<Field*>(child->m_next)) {
		CXX("\t%s.packer(mode, a, lastPack, changes, %" PRSizeT ");\n", child->m_name.c_str(), fieldNo++);
	}
	CXX("}\n"
		"\n");

	for (Field* child = static_cast<Field*>(m_child); child; child = static_cast<Field*>(child->m_next)) {
		HXX("\t\tWWII::SyncVar<%s, %" PRSizeT "> %s;\n", child->m_typeDef->DeclType().c_str(), child->m_size, child->m_name.c_str());
	}

	if (m_extraCode.empty() == false) {
		HXX("\n");
		HXX("%s", m_extraCode.c_str());
	}

	HXX("\t} %s;\n", m_memberName.c_str());
	HXX("\tconst %s& %s() const { return %s; }\n", m_name.c_str(), m_memberName.c_str() + 2, m_memberName.c_str());
	HXX("\t%s& %s() { return %s; }\n", m_name.c_str(), m_memberName.c_str() + 2, m_memberName.c_str());
	HXX("\n");
}

//////////////////////////////////////////////////////////////////////

void Field::Emit() const
{
	HXX("\t\t%s\t%s;\n", m_typeDef->DeclType().c_str(), m_name.c_str());
}

void Field::EmitPack(size_t indentLevel) const
{
	CXX("%sa->packer(mode, %s, %" PRSizeT ");\n", _getIndents(indentLevel), m_name.c_str(), m_size);
}

//////////////////////////////////////////////////////////////////////

void Conditional::Emit() const
{
	for (Node* node = m_child; node; node = node->m_next) {
		node->Emit();
	}
}

void Conditional::EmitPack(size_t indentLevel) const
{
	CXX("%s%s (%s) {\n", _getIndents(indentLevel), m_name.c_str(), m_condition.c_str());
	for (Node* child = m_child; child; child = child->m_next) {
		child->EmitPack(indentLevel + 1);
	}
	CXX("%s}\n", _getIndents(indentLevel));
}

//////////////////////////////////////////////////////////////////////
// Header file functions

static void _emitHeaderMast()
{
	HXX("#pragma once\n");
	HXX("// @file %s\n", s_hxxFilename.c_str());
	HXX("// @attention Automatically generated FROM %s\n", g_currentFile);
	HXX("// @attention Manual edits will be lost.\n");
	HXX("\n");
}

static void _emitHeaderIncludes()
{
	if (s_includes.empty())
		return;
	for (auto& it : s_includes) {
		HXX("#include \"%s\"\n", it.c_str());
	}
	HXX("\n");
}

static void _emitHeaderNamespaceOpen()
{
	if (!s_namespace.empty()) {
		HXX(
			"namespace %s\n"
			"{\n"
			"\n"
			, s_namespace.c_str()
			);
	}
}

static void _emitHeaderPrologue()
{
	_emitHeaderMast();
	_emitHeaderIncludes();
	_emitHeaderNamespaceOpen();
}

static void _emitHeaderNamespaceClose()
{
	if (!s_namespace.empty()) {
		HXX(
			"\n"
			"} // namespace %s\n"
			, s_namespace.c_str()
			);
	}
}

static void _emitHeaderEpilog()
{
	_emitHeaderNamespaceClose();
	HXX("\n");
}

//////////////////////////////////////////////////////////////////////
// Source file functions

static void _emitSourceMast()
{
	CXX("#include <syscfg.h> // path is project dependent\n");
	CXX("// @file %s\n", s_cxxFilename.c_str());
	CXX("// @attention Automatically generated FROM %s\n", g_currentFile);
	CXX("// @attention Manual edits will be lost.\n");
	CXX("\n");
}

static void _emitSourceIncludes()
{
	CXX("#include \"%s\"\n", s_hxxFilename.c_str());
}

static void _emitSourcePrologue()
{
	_emitSourceMast();
	_emitSourceIncludes();
}

static void _emitSourceEpilog()
{
}

//////////////////////////////////////////////////////////////////////

static void _generateOutput()
{
	_emitHeaderPrologue();
	_emitSourcePrologue();

	for (const Message* message = s_firstMessage; message; message = static_cast<const Message*>(message->m_next)) {
		message->Emit();
	}

	_emitHeaderEpilog();
	_emitSourceEpilog();
}

//////////////////////////////////////////////////////////////////////

static FILE* _openOutputFile(const std::string& filename)
{
	FILE* fp = fopen(filename.c_str(), "w");
	if (!fp) {
		fprintf(stderr, "ERROR: Unable to open '%s' for writing.\n", filename.c_str());
		exit(1);
	}
	return fp;
}

static std::string _makeOutputFilename(const char* suffix)
{
	std::string filename(g_currentFile);
	auto dotPos = filename.rfind('.');
	if (dotPos != std::string::npos)
		filename.resize(dotPos);
	filename.append(suffix);
	if (s_debug)
		printf("# '%s' file: %s\n", suffix, filename.c_str());
	return filename;
}

static void _openOutputFiles()
{
	if (s_cxxFilename.empty())
		s_cxxFilename = _makeOutputFilename(".cxx");
	s_cxx = _openOutputFile(s_cxxFilename);

	if (s_hxxFilename.empty())
		s_hxxFilename = _makeOutputFilename(".hxx");
	s_hxx = _openOutputFile(s_hxxFilename);
}

static bool _getGeneratorFromArg(const char* generator, const char* arg, std::string* filename)
{
	const size_t genLen = strlen(generator);
	if (strncmp(arg, generator, genLen) != 0)
		return false;
	// is there a filename?
	const char* file = arg + genLen + 1;
	if (arg[genLen] != '=' || !*file) {
		fprintf(stderr, "ERROR: Missing filename after '%s' switch\n", arg);
		exit(1);
	}
	if (!filename->empty()) {
		// if they specify the same file twice, fine.
		if (*filename == file)
			return true;
		fprintf(stderr, "ERROR: '%s' filename specified multiple times (%s and %s)\n", generator, filename->c_str(), file);
		exit(1);
	}
	*filename = file;
	return true;
}

void NORETURN_FN usage()
{
	fprintf(stderr, "Usage: [--cxx=file] [--hxx=file] %s <input file>\n", s_exeName);
	exit(1);
}

int main(int argc, const char** argv)
{
	s_exeName = argv[0];
	if (argc < 2) {
		usage();
	}

	for (int i = 1; i < argc; ++i) {
		const char* arg = argv[i];
		if (_getGeneratorFromArg("--cxx", arg, &s_cxxFilename))
			continue;
		if (_getGeneratorFromArg("--hxx", arg, &s_hxxFilename))
			continue;
		if (strcmp(arg, "--debug") == 0) {
			s_debug = true;
			continue;
		}
		if (arg[0] == '-') {
			fprintf(stderr, "ERROR: Unrecognized command line option, '%s'\n", arg);
			return 1;
		}
		if (g_currentFile != nullptr) {
			fprintf(stderr, "ERROR: Multiple file names or space in unquoted filename.: %s and %s\n", g_currentFile, arg);
			return 1;
		}
		g_currentFile = arg;
	}

	if (g_currentFile == nullptr) {
		fprintf(stderr, "ERROR: No input filename specified.\n");
		usage();
	}
	FILE* f = fopen(g_currentFile, "r");
	if (!f) {
		perror(g_currentFile);
		return 1;
	}
	yyrestart(f);
	yyparse();
	fclose(f);

	if (s_firstMessage == nullptr) {
		yyerror("%s: no messages defined.\n", g_currentFile);
		return 2;
	}

	_openOutputFiles();

	_generateOutput();
}
