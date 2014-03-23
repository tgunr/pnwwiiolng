#include <syscfg.h>
// Object Model Generator v2

#include <cstdio>
#include <iostream>
#include <string>
#include <cstdarg>
#include <set>

#include "omg.h"
#include "omg.yy.h"
#include "omg.tab.h"

static bool s_debug = false;
static std::string s_namespace = "";
static std::string s_namespaceQual = "";

const char* g_currentFile = nullptr;

enum { MAX_MESSAGE_GROUPS = 32, MAX_GROUP_FIELDS = 32 };

typedef std::set<std::string> TypeMap;
static TypeMap s_typeMap {
	"UINT8", "SINT8",
	"UINT16", "SINT16",
	"UINT32", "SINT32",
	"UINT64", "SINT64",
	"FLOAT", "DOUBLE",
	"FULL_OBJ_ID",
	"WEAPON_SPEC",
	"WEAPON_SERIAL",
	"RANK_NO",
	"ChatHandle",
	"Teul::NetTimeMS",
	"TeulTimeMS",
	"TeulDeltaMS",
	"PermissionMask",
	"playerid_t",
	"sortieid_t",
	"instanceid_t",
};

static unsigned int s_msgX;					// X of messages defined in this file.
static unsigned int s_msgY;					// next msgY to assign.

struct Node
{
	enum class Type
	{
		Message
	,	Group
	,	Field
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
		m_memberName[2] = tolower(m_memberName[2]);
	}

	void Emit() const override;
};

struct Field : public Node
{
	const char*			m_typeName;		// Variable type
	std::string			m_memberName;
	size_t				m_size;			// bits, length, etc.
	const char*			m_defaultValue;

	Field(const char* type, const char* name, Node* parent)
		: Node(Node::Type::Field, name, parent) 
		, m_typeName(type)
		, m_memberName(m_name)
		, m_size(0)
		, m_defaultValue("")
	{
		m_memberName[2] = tolower(m_memberName[2]);
	}

	void Emit() const override;
};


static Message* s_firstMessage;
static Message* s_lastMessage;
static Node*	s_currentNode;

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

void yyaddcode(const char* yytext, std::string& into)
{
	const bool isNewLine = (into.empty() || (into.back() == '\n'));
	if (isNewLine) {
		if (*yytext == '\n')
			return;
		char annotation[1024];
		sprintf(annotation, "#line %u \"%s\"\n", yylineno, g_currentFile);
		into += annotation;
	}
	into += *yytext;
}

//////////////////////////////////////////////////////////////////////

void omg_enable_debug()
{
	s_debug = true;
	annotate("Debugging output enabled");
}

void omg_type_decl(const char* name)
{
	auto result = s_typeMap.insert(name);
	if (result.second)
		annotate("Added new type: %s", name);
	else
		annotate("Redefinition of type %s", name);
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
	if (s_currentNode != s_lastMessage)
		yyfatal("Internal error: Group declaration that is not directly below MESSAGE", name);

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
		s_lastMessage->AddExtraMember("TeulTimeMS", "lastModified", "0ULL");

	annotate("Adding group '%s'", name);
	s_currentNode = group;
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

	Field* field = new Field(type, name, s_currentNode);

	annotate("Adding field %s %s;", type, name);
	s_currentNode = field;
}

void omg_end_field()
{
	annotate("End of field");
	s_currentNode = s_currentNode->m_parent;
}

void omg_add_code(const char* code)
{
	annotate("adding code");
	switch (s_currentNode->m_type) {
		case Node::Type::Message:
			static_cast<Message*>(s_currentNode)->m_code += code;
			break;
		case Node::Type::Group:
			static_cast<Group*>(s_currentNode)->m_extraCode += code;
			break;
	}
}

//////////////////////////////////////////////////////////////////////

void Message::AddExtraMember(const char* type, const char* name, const char* defaultValue)
{
	// extra members are added in reverse order
	Field* newField = new Field("TeulTimeMS", "lastModified", nullptr);
	newField->m_defaultValue = defaultValue;
	newField->m_next = m_extraMembers;
	m_extraMembers = newField;
}

//////////////////////////////////////////////////////////////////////

#define HDR(...) fprintf(stdout, __VA_ARGS__)
#define SRC(...) fprintf(stderr, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////

void Message::Emit() const
{
	HDR(
		"struct %s\n"
		"{\n"
		, m_name.c_str()
		);

	HDR("\tenum { MessageY = %u };\n", m_y);

	EmitConstructor();
	EmitPacker();

	EmitExtraMembers();
	EmitPublicMembers();

	HDR("};\n"
		"\n"
		);
}

void Message::EmitConstructor() const
{
	HDR("\tpublic: %s()\n", m_name.c_str());
	char joiner = ':';
	for (Field* field = m_extraMembers; field; field = static_cast<Field*>(field->m_next)) {
		HDR("\t\t%c m_%s(%s)\n", joiner, field->m_name.c_str(), field->m_defaultValue);
		joiner = ',';
	}
	for (Node* node = m_child; node; node = node->m_next) {
		switch (node->m_type) {
			case Node::Type::Group:
				HDR("\t\t%c %s(%s)\n", joiner, static_cast<Group*>(node)->m_memberName.c_str(), "m_lastModified");
				joiner = ',';
				break;
			case Node::Type::Field:
				HDR("\t\t%c %s(%s)\n", joiner, static_cast<Field*>(node)->m_memberName.c_str(), static_cast<Field*>(node)->m_defaultValue);
				joiner = ',';
				break;
			default:
				break;
		}
	}
	HDR("\t{\n"
		"\t}\n"
		"\n"
		);
}

void Message::EmitPacker() const
{
	const char* fingerprint;
	if (IsDeltad()) {
		EmitGroupPackers();
		fingerprint = "packer(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack)";
	} else {
		fingerprint = "packer(LOGICAL mode, ACTION_OBJECT* const a, SINT32 size)";
	}
	if (m_child == nullptr) {
		HDR("\tpublic: bool %s { return true; }\n", fingerprint);
		return;
	}
	HDR("\tpublic: bool %s;\n", fingerprint);
	SRC("bool %s%s::%s\n", s_namespaceQual.c_str(), m_name.c_str(), fingerprint);
	SRC("{\n");
	for (Node* node = m_child; node; node = node->m_next) {
		if (node->m_type != Node::Type::Field)
			continue;
		Field* field = static_cast<Field*>(node);
		SRC("\ta->packer(mode, m_%s, %" PRSizeT ");\n", field->m_name.c_str(), field->m_size);
	}
	if (m_numGroups != 0)
		SRC("\treturn (mode == TEUL_PACK) ? _pack(a, size, lastPack) : _unpack(a, size, lastPack);\n");
	else
		SRC("\treturn (a->offset >= 0);\n");
	SRC("}\n"
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

	HDR(
		"\tpublic: const TeulTimeMS& lastModified() const { return m_lastModified; }\n"
		"\tprivate: bool _unpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack);\n"
		"\tprivate: bool _pack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack) const;\n"
	);

	// unpack delta function
	SRC(
		"bool %s%s::_unpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack)\n"
		"{\n"
		"\t%s groupPresence(0);\n"
		"\ta->packer(mode, groupPresence, %" PRSizeT ");\n"
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
		SRC("\tif (groupPresence & (1 << %" PRSizeT "))\n"
			"\t\t%s.packer(TEUL_UNPACK, a, lastPack);\n"
			, groupNo++
			, group->m_memberName.c_str()
			);
	}
	SRC(
		"\treturn true;\n"
		"}\n"
		"\n"
		);

	// pack delta function
	SRC(
		"bool %s%s::_pack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack) const\n"
		"{\n"
		"\t%s groupPresence(0);\n"
		"\tSINT32 groupPresenceOffset = a->offset;\n"
		"\ta->packer(mode, groupPresence, %" PRSizeT ");\n"
		, s_namespaceQual.c_str(), m_name.c_str()
		, groupPresenceType
		, groupPresenceBits
		);
	groupNo = 0;
	for (Node* node = m_child; node; node = node->m_next) {
		if (node->m_type != Node::Type::Group)
			continue;
		Group* group = static_cast<Group*>(node);
		SRC("\tif (%s.packer(TEUL_PACK, a, lastPack))\n"
			"\t\tgroupPresence |= (1 << %" PRSizeT ");\n"
			, group->m_memberName.c_str()
			, groupNo++
			);
	}
	SRC(
		"\tif (groupPresence == 0)\n"
		"\t\treturn false;\n"
		"\ta->offset = teulPack%s(mode, a->packed, groupPresenceOffset, &groupPresenceOffset, %" PRSizeT ");\n"
		"\treturn true;\n"
		"}\n"
		"\n"
		, groupPresenceType
		, groupPresenceBits
		);
}

static void _generateOutput()
{
	///TODO: Generate the output code.
	HDR("#pragma once\n");
	HDR("// Automatically generated by omggen from %s - Manual edits will be lost.\n", g_currentFile);
	if (s_namespace.empty() == false)
		HDR("namespace %s\n{\n\n", s_namespace.c_str());

	SRC("#include <syscfg.h> // path is project dependent\n");
	SRC("// Automatically generated by omggen from %s - Manual edits will be lost.\n", g_currentFile);

	for (const Message* message = s_firstMessage; message; message = static_cast<const Message*>(message->m_next)) {
		message->Emit();
	}

	if (s_namespace.empty() == false)
		HDR("\n} // namespace %s\n", s_namespace.c_str());
}

void Message::EmitExtraMembers() const
{
	if (!m_extraMembers)
		return;
	HDR("\tprotected:\n");
	for (Field* field = m_extraMembers; field; field = static_cast<Field*>(field->m_next)) {
		field->Emit();
	}
	HDR("\n");
}

void Message::EmitPublicMembers() const
{
	if (!m_child)
		return;
	HDR("\tpublic:\n");
	for (Node* node = m_child; node; node = node->m_next) {
		node->Emit();
	}
	HDR("\n");
}

void Group::Emit() const
{
	std::string fqn(s_namespaceQual);
	if (m_parent) {
		fqn += m_parent->m_name;
		fqn += "::";
	}
	fqn += m_name;

	HDR(
		"\tstruct %s : public WWII::VarGroup\n"
		"\t{\n"
		"\t\t%s(TeulTimeMS& parent)\n"
		"\t\t\t: VarGroup(parent)\n"
		, m_name.c_str()
		, m_name.c_str()
		);
	for (Field* child = static_cast<Field*>(m_child); child; child = static_cast<Field*>(child->m_next)) {
		HDR("\t\t\t, %s(this)\n", child->m_name.c_str());
	}
	HDR("\t\t{}\n");

	HDR("\t\t%s& operator = (const %s& rhs);\n", m_name.c_str(), m_name.c_str());
	SRC(
		"%s& %s::operator = (const %s& rhs)\n"
		"{\n"
		"\tm_lastModified = rhs.m_lastModified;\n"
		, fqn.c_str()
		, fqn.c_str()
		, fqn.c_str()
		);
	for (Field* child = static_cast<Field*>(m_child); child; child = static_cast<Field*>(child->m_next)) {
		SRC("\t%s.operator=(rhs.%s);\n", child->m_name.c_str(), child->m_name.c_str());
	}
	SRC(
		"\treturn *this;\n"
		"}\n"
		"\n"
		);

	HDR("\t\tvoid groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes) override;\n");
	SRC(
		"void %s::groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)\n"
		"{\n"
		, fqn.c_str()
		);
	size_t fieldNo = 0;
	for (Field* child = static_cast<Field*>(m_child); child; child = static_cast<Field*>(child->m_next)) {
		SRC("\t%s.packer(mode, a, lastPack, changes, %" PRSizeT ");\n", child->m_name.c_str(), fieldNo++);
	}
	SRC("}\n"
		"\n");

	for (Field* child = static_cast<Field*>(m_child); child; child = static_cast<Field*>(child->m_next)) {
		HDR("\t\tWWII::SyncVar<%s, %" PRSizeT "> %s;\n", child->m_typeName, child->m_name.c_str(), child->m_size);
	}

	if (m_extraCode.empty() == false) {
		HDR("\n");
		HDR("%s", m_extraCode.c_str());
	}

	HDR("\t} %s;\n", m_memberName.c_str());
	HDR("\tconst %s& %s() const { return %s; }\n", m_name.c_str(), m_memberName.c_str() + 2, m_memberName.c_str());
	HDR("\t%s& %s() { return %s; }\n", m_name.c_str(), m_memberName.c_str() + 2, m_memberName.c_str());
	HDR("\n");
}

void Field::Emit() const
{
	HDR("\t\t%s\t%s;\n", m_typeName, m_memberName.c_str());
}

int main(int argc, const char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
		return 1;
	}

	g_currentFile = argv[1];
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

	_generateOutput();
}
