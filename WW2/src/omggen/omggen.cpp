// Object Model Generator v2

#include "omggen.h"
#include "omggen.yy.h"
#include "omggen.tab.h"

#include <stack>
#include <string>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef min
#undef max
#endif

#if defined(_MSC_VER)
# define NORETURN_FN __declspec(noreturn)
#else
# define NORETURN_FN __attribute__ ((noreturn))
#endif


long long leakCount = 0;
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
static TypeMap s_typeMap = {
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
	{ "bool", "", false, "" },
};

static unsigned int s_msgX;					// X of messages defined in this file.
static unsigned int s_msgY;					// next msgY to assign.

static const char s_indents[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
static const size_t MAX_INDENTS = sizeof(s_indents) -1;

struct Using
{
	std::string			m_filename;
	FILE*				m_fp;
	int					m_lineNo;

	Using(const char* filename, FILE* fp, int lineNo)
		: m_filename(filename), m_fp(fp), m_lineNo(lineNo)
	{ }
};
std::stack<Using>		s_usings;

struct Node
{
	enum class Type
	{
		Message
	,	Group
	,	Field
	,	Condition
	,	Array
	,	Enumeration
	,	EnumValue
	};

	Node*				m_next;
	Node*				m_parent;
	Node*				m_child;
	Node*				m_lastChild;

	Type				m_type;
	std::string			m_name;
	mutable std::string	m_qualifiedName;

	Node(Type type, std::string&& name, Node* parent)
		: m_next(nullptr)
		, m_parent(parent)
		, m_child(nullptr)
		, m_lastChild(nullptr)
		, m_type(type)
		, m_name(std::move(name))
		, m_qualifiedName("")
	{
		if (m_parent) {
			if (m_parent->m_lastChild)
				m_parent->m_lastChild->m_next = this;
			else
				m_parent->m_child = this;
			m_parent->m_lastChild = this;
		}
	}

	Node(Type type, const char* name, Node* parent)
		: Node(type, std::string(name), parent)
	{}

	virtual void Emit(size_t /*indentLevel*/) const
	{
		yyerror("Internal Error: Reached Node->Emit();\n");
		exit(9);
	}

	virtual void EmitPack(size_t /*indentLevel*/) const
	{
	}

	virtual void EmitUnpack(size_t /*indentLevel*/) const
	{
	}

	bool Contains(const char* name) const
	{
		for (const Node* child = m_child; child; child = child->m_next) {
			if (child->m_name.compare(name) == 0)
				return true;
		}
		return false;
	}

	bool IsOrIsChildOf(Type type) const
	{
		if (m_type == type)
			return true;
		if (m_parent == nullptr)
			return false;
		return m_parent->IsOrIsChildOf(type);
	}

	const std::string& GetQualifiedName() const
	{
		if (m_qualifiedName.empty()) {
			m_qualifiedName = (m_parent) ? m_parent->GetQualifiedName() : s_namespace;
			if (!m_name.empty() && m_type != Node::Type::Condition) {
				m_qualifiedName += "::";
				m_qualifiedName += m_name;
			}
		}
		return m_qualifiedName;
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

	void Emit(size_t /*indentLevel*/) const override;
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

	void Emit(size_t /*indentLevel*/) const override;
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

	void Emit(size_t indentLevel) const override;
	void EmitPack(size_t indentLevel) const override;
	void EmitUnpack(size_t indentLevel) const override;
};

struct Conditional : public Node
{
	std::string			m_condition;

	Conditional(const char* keyword, const char* condition, Node* parent)
		: Node(Node::Type::Condition, keyword, parent)
		, m_condition(condition)
	{
	}

	void Emit(size_t indentLevel) const override;
	void EmitPack(size_t indentLevel) const override;
	void EmitUnpack(size_t indentLevel) const override;
};

struct Array : public Node
{
	std::string			m_memberName;
	std::string			m_typedef;
	std::string			m_dimension;

	Array(std::string&& structName, std::string&& typedefName, std::string&& memberName, std::string&& dimension, Node* parent)
		: Node(Node::Type::Array, std::move(structName), parent)
		, m_memberName(std::move(memberName))
		, m_typedef(std::move(typedefName))
		, m_dimension(std::move(dimension))
	{
	}

	void Emit(size_t indentLevel) const override;
	void EmitPack(size_t indentLevel) const override;
	void EmitUnpack(size_t indentLevel) const override;

	uint16_t GetPackBits() const;
};

struct Enumeration : public Node
{
	uint32_t	m_count;

	Enumeration(std::string&& enumName, Node* parent)
		: Node(Node::Type::Enumeration, std::move(enumName), parent)
		, m_count(0)
	{}

	void Emit(size_t indentLevel) const override;
};

struct EnumValue : public Node
{
	uint32_t	m_value;

	EnumValue(std::string&& name, uint32_t value, Node* parent)
		: Node(Node::Type::EnumValue, std::move(name), parent)
		, m_value(value)
	{
	}

	void Emit(size_t indentLevel) const override;
};

static Message* s_firstMessage;
static Message* s_lastMessage;
static Node*	s_currentNode;

std::vector<Enumeration*> s_enumerations;

//////////////////////////////////////////////////////////////////////

#define HXX(...) fprintf(s_hxx, __VA_ARGS__)
#define CXX(...) fprintf(s_cxx, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////

static void _reportLocation(FILE* fp)
{
#if defined(WIN32)
	char fullPath[1024];
	GetFullPathNameA(g_currentFile, sizeof(fullPath), fullPath, nullptr);
	fprintf(fp, "%s(%u): ", fullPath, yylineno);
#else
	fprintf(fp, "%s:%u: ", g_currentFile, yylineno);
#endif
}

template<typename... Args>
void annotate(const char* fmt, Args... args)
{
	if (s_debug) {
		_reportLocation(stdout);
		fprintf(stdout, fmt, std::forward<Args>(args)...);
		fprintf(stdout, "\n");
	}
}

void yyerror(const char *s, ...)
{
	_reportLocation(stderr);
	fprintf(stderr, "error O101: ");
	va_list ap;
	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

void yyfatal(const char* s, ...)
{
	_reportLocation(stderr);
	fprintf(stderr, "error F101: ");
	va_list ap;
	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
	fprintf(stderr, "\n");
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

static void _endOfNode(Node::Type expectedType)
{
	if (s_currentNode == nullptr) {
		yyerror("INTERNAL ERROR: currentNode is null");
		exit(1);
	}
	if (s_currentNode->m_type != expectedType) {
		yyerror("INTERNAL ERROR: node %s is not of expected type (%d != %d)", s_currentNode->m_name.c_str(), s_currentNode->m_type, expectedType);
		exit(1);
	}
	
	s_currentNode = s_currentNode->m_parent;
}

static void _checkName(const char* name)
{
	if (s_currentNode && (s_currentNode->m_name.compare(name) == 0 || s_currentNode->Contains(name)))
		yyfatal("Redefinition of '%s' in this scope", name);
	if (_findTypeDef(name))
		yyfatal("Invalid use of type name '%s' as identifier", name);
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
	annotate("end message '%s'", s_lastMessage->m_name.c_str());
	_endOfNode(Node::Type::Message);
}

void omg_start_group(const char* name)
{
	if (s_currentNode->IsOrIsChildOf(Node::Type::Group))
		yyfatal("GROUPs may only occur at the top level of a message definition: %s", name);

	if (islower(name[0]))
		yyfatal("GROUP names must start with a capital letter ('%s')", name);

	_checkName(name);

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
	annotate("end group %s", s_currentNode->m_name.c_str());

	if (s_currentNode->m_child == nullptr)
		yyfatal("'GROUP %s' has empty definition.", s_currentNode->m_name.c_str());

	_endOfNode(Node::Type::Group);
}

void omg_start_field(const char* type, const char* name)
{
	_checkName(name);

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
	annotate("end field %s", s_currentNode->m_name.c_str());
	_endOfNode(Node::Type::Field);
}

void omg_start_if(const char* condition)
{
	if (s_currentNode->m_type == Node::Type::Group) {
		yyerror("Can't use IF inside of a GROUP definition.");
		exit(1);
	}
	annotate("if (%s) ...", condition);
	Conditional* cond = new Conditional("if", condition, s_currentNode);
	s_currentNode = cond;
}

void omg_end_if()
{
	annotate("endif()");
	_endOfNode(Node::Type::Condition);
}

void omg_start_array(std::string structName, std::string typedefName, std::string memberName, std::string dimension)
{
	if (s_currentNode->IsOrIsChildOf(Node::Type::Group))
		yyfatal("ARRAY %s: cannot be use ARRAY inside of a GROUP definition", structName.c_str());

	if (dimension[0] == '-')
		yyfatal("Illegal ARRAY dimension: %s", dimension.c_str());
	if (dimension[0] == '-' || isdigit(dimension[0])) {
		long i = std::stol(dimension);
		if (i <= 0)
			yyfatal("Illegal ARRAY dimension: %s", dimension.c_str());
		if (i >= (1 << 24))
			yyfatal("ARRAY dimension exceeds 2^24: %s", dimension.c_str());
	}

	if (typedefName.empty()) {
		typedefName = structName;
		typedefName.push_back('s');
	}

	if (memberName.empty())
		memberName = typedefName;
	memberName[0] = tolower(memberName[0]);

	annotate("array struct %s typedef %s member %s [%s]", structName.c_str(), typedefName.c_str(), memberName.c_str(), dimension.c_str());

	_checkName(structName.c_str());
	_checkName(typedefName.c_str());
	_checkName(memberName.c_str());

	Array* arr = new Array(std::move(structName), std::move(typedefName), std::move(memberName), std::move(dimension), s_currentNode);
	s_currentNode = arr;

	s_includes.insert("include/inplace_vector.h");
}

void omg_end_array()
{
	annotate("end array %s", s_currentNode->m_name.c_str());
	_endOfNode(Node::Type::Array);
}

void omg_add_code(const char* code)
{
	annotate("adding code");
	// trim off leading blank lines.
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

const char* omg_current_node_name()
{
	return s_currentNode->m_name.c_str();
}

void omg_using(const char* filename)
{
	FILE* fp = fopen(filename, "r");
	if (!fp) {
		yyfatal("Unable to open USING file \"%s\".\n", filename);
		return;
	}
	s_usings.top().m_lineNo = yylineno;
	yypush_buffer_state(yy_create_buffer(fp, YY_BUF_SIZE));
	s_usings.emplace(filename, fp, 0);
	g_currentFile = s_usings.top().m_filename.c_str();
	yylineno = 1;
}

void omg_eof()
{
	if (s_usings.size() > 1) {
		fclose(s_usings.top().m_fp);
		s_usings.pop();
	}
	if (!s_usings.empty()) {
		g_currentFile = s_usings.top().m_filename.c_str();
		yylineno = s_usings.top().m_lineNo;
	}
	yypop_buffer_state();
}

void omg_add_include(const char* filename)
{
	s_includes.insert(filename);
}

void omg_start_enum(const char* enumName)
{
	annotate("Adding enum %s", enumName);

	assert(!s_currentNode || !s_currentNode->IsOrIsChildOf(Node::Type::Enumeration));

	_checkName(enumName);

	if (islower(enumName[0]))
		yyfatal("ENUM names must start with a capital letter ('%s')", enumName);

	Enumeration* enumeration = new Enumeration(enumName, s_currentNode);
	s_enumerations.push_back(enumeration);
	s_currentNode = enumeration;
}

void omg_end_enum()
{
	annotate("end enum %s", s_currentNode->m_name.c_str());

	if (s_currentNode->m_child == nullptr)
		yyfatal("'ENUM %s' has empty definition.", s_currentNode->m_name.c_str());

	_endOfNode(Node::Type::Enumeration);
}

void omg_add_enum_value(const char* name)
{
	if (islower(name[0]) && name[0] != '_')
		yyfatal("ENUM %s value '%s' must start with a capital letter or an underscore.", s_currentNode->m_name.c_str(), name);

	_checkName(name);

	Enumeration* enumeration = static_cast<Enumeration*>(s_currentNode);
	assert(enumeration != nullptr);
	EnumValue* value = new EnumValue(name, enumeration->m_count++, s_currentNode);

	annotate("Adding ENUM %s value %d", s_currentNode->m_name.c_str(), name, value->m_value);
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

void Message::Emit(size_t indentLevel) const
{
	assert(indentLevel == 1);

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
	HXX(
		"\t%s()\n"
		, m_name.c_str()
		);
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
		HXX("\tbool packer(LOGICAL /*mode*/, ACTION_OBJECT* /*a*/, SINT32 /*size*/) { return true; }\n");
		return;
	}

	const char* fingerprint;
	const char* param3;
	if (IsDeltad()) {
		EmitGroupPackers();
		fingerprint = "ACTION_OBJECT* const a, const TeulTimeMS& lastPack";
		param3 = "lastPack";
	} else {
		fingerprint = "ACTION_OBJECT* const a, SINT32 size";
		param3 = "size";
	}
	HXX("\tbool packer(LOGICAL mode, %s) { return (mode == TEUL_PACK) ? pack(a, %s) : unpack(a, %s); }\n", fingerprint, param3, param3);
	HXX("\tbool pack(%s) const;\n", fingerprint);
	HXX("\tbool unpack(%s);\n", fingerprint);

	CXX("bool %s::pack(%s) const\n", GetQualifiedName().c_str(), fingerprint);
	CXX("{\n");
	for (Node* node = m_child; node; node = node->m_next) {
		node->EmitPack(1);
	}
	if (m_numGroups != 0)
		CXX("\treturn groupPack(a, lastPack) && (a->offset >= 0);\n");
	else
		CXX("\treturn (a->offset >= 0);\n");
	CXX(
		"}\n"
		"\n"
		);

	CXX("bool %s::unpack(%s)\n", GetQualifiedName().c_str(), fingerprint);
	CXX("{\n");
	for (Node* node = m_child; node; node = node->m_next) {
		node->EmitUnpack(1);
	}
	if (m_numGroups != 0)
		CXX("\treturn groupUnpack(a, lastPack) && (a->offset >= 0);\n");
	else
		CXX("\treturn (a->offset >= 0);\n");
	CXX(
		"}\n"
		"\n"
		);

}

void Message::EmitGroupPackers() const
{
	size_t groupNo;
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
		"\tenum { GroupPresenceBits = %u };\n"
		"\tconst TeulTimeMS& lastModified() const { return m_lastModified; }\n"
		"\tbool groupPack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack) const;\n"
		"\tbool groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack);\n"
		, groupPresenceBits
	);

	// group pack delta function
	CXX(
		"bool %s::groupPack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack) const\n"
		"{\n"
		"\t%s groupPresence(0);\n"
		"\tSINT32 groupPresenceOffset = a->offset;\n"
		"\ta->pack(groupPresence, %" PRSizeT ");\n"
		, GetQualifiedName().c_str()
		, groupPresenceType
		, groupPresenceBits
		);
	groupNo = 0;
	for (Node* node = m_child; node; node = node->m_next) {
		if (node->m_type != Node::Type::Group)
			continue;
		Group* group = static_cast<Group*>(node);
		CXX("\tif (%s.pack(a, lastPack))\n"
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

	// unpack delta function
	CXX(
		"bool %s::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack)\n"
		"{\n"
		"\t%s groupPresence(0);\n"
		"\ta->unpack(groupPresence, %" PRSizeT ");\n"
		"\tif (PN_LIKELY(groupPresence != 0)) {\n"
		, GetQualifiedName().c_str()
		, groupPresenceType
		, groupPresenceBits
		);
	groupNo = 0;
	for (Node* node = m_child; node; node = node->m_next) {
		if (node->m_type != Node::Type::Group)
			continue;
		Group* group = static_cast<Group*>(node);
		CXX("\t\tif (groupPresence & (1 << %" PRSizeT "))\n"
			"\t\t\t%s.unpack(a, lastPack);\n"
			, groupNo++
			, group->m_memberName.c_str()
			);
	}
	CXX(
		"\t\treturn (a->offset >= 0);\n"
		"\t}\n"
		"\treturn false;\n"
		"}\n"
		"\n"
		);
}

void Message::EmitExtraMembers() const
{
	if (!m_extraMembers)
		return;
	for (Field* field = m_extraMembers; field; field = static_cast<Field*>(field->m_next)) {
		field->Emit(1);
	}
	HXX("\n");
}

void Message::EmitPublicMembers() const
{
	if (!m_child)
		return;
	for (Node* node = m_child; node; node = node->m_next) {
		node->Emit(1);
	}
}

//////////////////////////////////////////////////////////////////////

void Group::Emit(size_t indentLevel) const
{
	HXX("%sstruct %s : public WWII::VarGroup\n", _getIndents(indentLevel), m_name.c_str());
	HXX("%s{\n", _getIndents(indentLevel));
	HXX("%s%s(TeulTimeMS& parent)\n", _getIndents(indentLevel + 1), m_name.c_str());
	HXX("%s: VarGroup(parent)\n", _getIndents(indentLevel + 2));
	for (const Node* child = m_child; child; child = child->m_next) {
		HXX("%s, %s(this)\n", _getIndents(indentLevel + 2), child->m_name.c_str());
	}
	HXX("%s{}\n", _getIndents(indentLevel + 1));

	HXX("%s%s& operator = (const %s& rhs);\n", _getIndents(indentLevel + 1), m_name.c_str(), m_name.c_str());

	CXX(
		"%s& %s::operator = (const %s& rhs)\n"
		"{\n"
		"\tm_lastModified = rhs.m_lastModified;\n"
		, GetQualifiedName().c_str()
		, GetQualifiedName().c_str()
		, GetQualifiedName().c_str()
		);
	for (const Node* child = m_child; child; child = child->m_next) {
		CXX("\t%s.operator=(rhs.%s);\n", child->m_name.c_str(), child->m_name.c_str());
	}
	CXX(
		"\treturn *this;\n"
		"}\n"
		"\n"
		);

	HXX("%svoid groupPack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes) const override;\n", _getIndents(indentLevel + 1));
	CXX(
		"void %s::groupPack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes) const\n"
		"{\n"
		, GetQualifiedName().c_str()
		);
	size_t fieldNo = 0;
	for (const Node* child = m_child; child; child = child->m_next) {
		CXX("\t%s.pack(a, lastPack, changes, %" PRSizeT ");\n", child->m_name.c_str(), fieldNo++);
	}
	CXX("}\n"
		"\n");

	HXX("%svoid groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes) override;\n", _getIndents(indentLevel + 1));
	CXX(
		"void %s::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)\n"
		"{\n"
		, GetQualifiedName().c_str()
		);
	fieldNo = 0;
	for (const Node* child = m_child; child; child = child->m_next) {
		CXX("\t%s.unpack(a, lastPack, changes, %" PRSizeT ");\n", child->m_name.c_str(), fieldNo++);
	}
	CXX("}\n"
		"\n");

	for (const Node* child = m_child; child; child = child->m_next) {
		if (child->m_type != Node::Type::Field) {
			yyerror("INTERNAL ERROR: Group contains non-field node");
			exit(1);
		}
		const Field* const field = static_cast<const Field*>(child);
		HXX("%sWWII::SyncVar<%s, %" PRSizeT "> %s;\n", _getIndents(indentLevel + 1), field->m_typeDef->DeclType().c_str(), field->m_size, field->m_name.c_str());
	}

	if (m_extraCode.empty() == false) {
		HXX("\n");
		HXX("%s", m_extraCode.c_str());
	}

	HXX("%s} %s;\n", _getIndents(indentLevel), m_memberName.c_str());
	HXX("%sconst %s& %s() const { return %s; }\n", _getIndents(indentLevel), m_name.c_str(), m_memberName.c_str() + 2, m_memberName.c_str());
	HXX("%s%s& %s() { return %s; }\n", _getIndents(indentLevel), m_name.c_str(), m_memberName.c_str() + 2, m_memberName.c_str());
	HXX("\n");
}

//////////////////////////////////////////////////////////////////////

void Field::Emit(size_t indentLevel) const
{
	HXX("%s%s\t%s;\n", _getIndents(indentLevel), m_typeDef->DeclType().c_str(), m_name.c_str());
}

void Field::EmitPack(size_t indentLevel) const
{
	CXX("%sa->pack(%s, %" PRSizeT ");\n", _getIndents(indentLevel), m_name.c_str(), m_size);
}

void Field::EmitUnpack(size_t indentLevel) const
{
	CXX("%sa->unpack(%s, %" PRSizeT ");\n", _getIndents(indentLevel), m_name.c_str(), m_size);
}

//////////////////////////////////////////////////////////////////////

void Conditional::Emit(size_t indentLevel) const
{
	for (Node* node = m_child; node; node = node->m_next) {
		node->Emit(indentLevel);
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

void Conditional::EmitUnpack(size_t indentLevel) const
{
	CXX("%s%s (%s) {\n", _getIndents(indentLevel), m_name.c_str(), m_condition.c_str());
	for (Node* child = m_child; child; child = child->m_next) {
		child->EmitUnpack(indentLevel + 1);
	}
	CXX("%s}\n", _getIndents(indentLevel));
}

//////////////////////////////////////////////////////////////////////

void Array::Emit(size_t indentLevel) const
{
	const char* indent = _getIndents(indentLevel);
	HXX("%sstruct %s\n", indent, m_name.c_str());
	HXX("%s{\n", indent);
	for (Node* child = m_child; child; child = child->m_next) {
		child->Emit(indentLevel + 1);
	}
	HXX("%s\tbool pack(ACTION_OBJECT* const a, SINT32) const;\n", indent);
	HXX("%s\tbool unpack(ACTION_OBJECT* const a, SINT32);\n", indent);
	HXX("%s};\n", indent);
	HXX("%stypedef WWII::inplace_vector<%s, %s> %s;\n", indent, m_name.c_str(), m_dimension.c_str(), m_typedef.c_str());
	HXX("%s%s %s;\n", indent, m_typedef.c_str(), m_memberName.c_str());

	CXX("bool %s::pack(ACTION_OBJECT* const a, SINT32) const\n", GetQualifiedName().c_str());
	CXX("{\n");
	for (Node* node = m_child; node; node = node->m_next) {
		node->EmitPack(1);
	}
	CXX("\treturn (a->offset >= 0);\n");
	CXX("}\n");
	CXX("\n");

	CXX("bool %s::unpack(ACTION_OBJECT* const a, SINT32)\n", GetQualifiedName().c_str());
	CXX("{\n");
	for (Node* node = m_child; node; node = node->m_next) {
		node->EmitUnpack(1);
	}
	CXX("\treturn (a->offset >= 0);\n");
	CXX("}\n");
	CXX("\n");
}

uint16_t Array::GetPackBits() const
{
	if (isdigit(m_dimension[0])) {
		long value = std::stol(m_dimension);
		if (value < (1 << 8))
			return 8;
		else if (value < (1 << 16))
			return 16;
		else if (value < (1 << 24))
			return 24;
		else
			yyfatal("ARRAY dimension too large: %s", m_dimension.c_str());
	}
	return 16;
}

void Array::EmitPack(size_t indentLevel) const
{
	const char* indent = _getIndents(indentLevel);
	uint16_t packBits = GetPackBits();
	std::string countVar = m_name + "_count";
	std::string iterVar = m_name + "_idx";

	CXX("%sABORT_IF(%s.size() >= %s);\n", indent, m_memberName.c_str(), m_dimension.c_str());
	CXX("%sUINT16 %s(%s.size());\n", indent, countVar.c_str(), m_memberName.c_str());
	CXX("%sa->pack(%s, %u);\n", indent, countVar.c_str(), packBits);

	CXX("%sfor (UINT32 %s = 0; %s < %s; ++%s) {\n", indent, iterVar.c_str(), iterVar.c_str(), countVar.c_str(), iterVar.c_str());
	CXX("%s\t%s[%s].pack(a, 0);\n", indent, m_memberName.c_str(), iterVar.c_str());
	CXX("%s}\n", indent);
}

void Array::EmitUnpack(size_t indentLevel) const
{
	const char* indent = _getIndents(indentLevel);
	uint16_t packBits = GetPackBits();
	std::string countVar = m_name + "_count";
	std::string iterVar = m_name + "_idx";

	CXX("%sUINT16 %s(0);\n", indent, countVar.c_str());
	CXX("%sa->unpack(%s, %u);\n", indent, countVar.c_str(), packBits);
	CXX("%sif (%s > %s) { a->offset = -EINVAL; return false; }\n", indent, countVar.c_str(), m_dimension.c_str());
	CXX("%s%s.resize(%s);\n", indent, m_memberName.c_str(), countVar.c_str());

	CXX("%sfor (UINT32 %s = 0; %s < %s; ++%s) {\n", indent, iterVar.c_str(), iterVar.c_str(), countVar.c_str(), iterVar.c_str());
	CXX("%s\t%s[%s].unpack(a, 0);\n", indent, m_memberName.c_str(), iterVar.c_str());
	CXX("%s}\n", indent);
}

//////////////////////////////////////////////////////////////////////

void Enumeration::Emit(size_t indentLevel) const
{
	const char* indent = _getIndents(indentLevel);
	HXX("%senum class %s\n", indent, m_name.c_str());
	HXX("%s{\n", indent);
	for (Node* child = m_child; child; child = child->m_next) {
		child->Emit(indentLevel + 1);
	}
	HXX("%s\tMax%s\n", indent, m_name.c_str());
	HXX("%s};\n\n", indent);
}

//////////////////////////////////////////////////////////////////////

void EnumValue::Emit(size_t indentLevel) const
{
	HXX("%s%s,\n", _getIndents(indentLevel), m_name.c_str());
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

	if (s_enumerations.empty() == false) {
		HXX("// Enumerations\n\n");
		for (auto enumPtr : s_enumerations) {
			enumPtr->Emit(0);
		}
	}
	for (const Message* message = s_firstMessage; message; message = static_cast<const Message*>(message->m_next)) {
		message->Emit(1);
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

NORETURN_FN void usage()
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
	s_usings.emplace(g_currentFile, f, 0);
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
