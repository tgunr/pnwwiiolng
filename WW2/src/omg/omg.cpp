// Object Model Generator v2

#include <cstdio>
#include <iostream>
#include <string>
#include <cstdarg>
#include <set>

#include "omg.yy.h"
#include "omg.tab.h"

static bool s_debug = false;
static std::string s_namespace = "";

const char* g_currentFile = nullptr;

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

struct Node
{
	enum class Type
	{
		Message
	,	Group
	,	Field
	};

	Node*				m_next;
	Node*				m_child;

	Type				m_type;
	std::string			m_name;

	Node(Type type, const char* name)
		: m_next(nullptr), m_child(nullptr)
		, m_type(type)
		, m_name(name)
	{ }
};

struct Message : public Node
{
	const char*			m_x;			// X to use for this.
	int					m_y;			// assign a Y value

	Message(const char* name) : Node(Node::Type::Message, name) {}
};

struct Group : public Node
{
	Group(const char* name) : Node(Node::Type::Group, name) {}
};

struct Field : public Node
{
	const char*			m_typeName;		// Variable type
	size_t				m_size;			// bits, length, etc.

	Field(const char* name) : Node(Node::Type::Field, name) {}
};

static Message* s_firstMessage;
static Message* s_lastMessage;

static unsigned int s_msgY;					// next msgY to assign

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

Message* _findMessageByName(const char* str)
{
	for (Message* message = s_firstMessage; message; message = static_cast<Message*>(message->m_next)) {
		if (message->m_name == str)
			return message;
	}
	return nullptr;
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
}

void omg_start_message(const char* name)
{
	// did we already allocate a message with this name?
	if (_findMessageByName(name) != nullptr) {
		yyerror("duplicate message: %s", name);
		exit(1);
	}

	if (s_msgY == 255) {
		yyerror("reached message Y #255 at message '%s'", name);
		exit(1);
	}

	Message* message = new Message(name);
	message->m_y = s_msgY++;

	if (!s_firstMessage)
		s_firstMessage = message;
	else
		s_lastMessage->m_next = message;
	s_lastMessage = message;

	annotate("Declaring message '%s' as %u", name, s_msgY);
}

void omg_end_message()
{
	annotate("Completed definition of message '%s'", s_lastMessage->m_name.c_str());
}

void omg_start_group(const char* name)
{
	annotate("Starting group '%s'", name);
}

void omg_end_group()
{
	annotate("Ended group");
}

void omg_start_field(const char* type, const char* name)
{
	annotate("Starting field %s %s;", type, name);
}

void omg_end_field()
{
	annotate("End of field");
}

void omg_add_code(const char* code)
{
	annotate("adding code");
}

static void _generateOutput()
{
	///TODO: Generate the output code.
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
