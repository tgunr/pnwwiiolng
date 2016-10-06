#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>

#include <stdarg.h>
#include <vector>
#include <set>
#include <utility>
#include <algorithm>
#include <string>

#if defined(_MSC_VER)
#define strdup(...) _strdup(__VA_ARGS__)
#define PRSizeT "u"
#else
#include <unistd.h>
#define PRSizeT "zu"
#endif


class TypeDef
{
	std::string					m_name;
	std::string					m_declType;
	size_t						m_wireBits;
	bool						m_passByRef;
	std::string					m_include;

public:
	TypeDef(std::string&& name, std::string&& declType, size_t wireBits, bool passByRef, std::string&& include)
		: m_name(std::move(name))
		, m_declType(std::move(declType))
		, m_wireBits(wireBits)
		, m_passByRef(passByRef)
		, m_include(std::move(include))
	{
	}

	const std::string& Name() const { return m_name; }
	const std::string& DeclType() const { return m_declType.empty() ? Name() : m_declType; }
	size_t WireBits() const { return m_wireBits; }
	bool PassByRef() const { return m_passByRef; }
	const std::string& Include() const { return m_include; }
};

/* Client type enumerations */
enum
{
	CL_FRONTEND,
	CL_SERVER,
};

/* Message Attribute enumerations */
enum
{
	MA_TOCLIENT,
	MA_TOHOST,
	MA_GMONLY,
	MA_INTERNAL,
};

extern const char* g_currentFile;
extern int yylineno;

// Functions common to omggen.cpp and the .y file.
void yyerror(const char* s, ...);
[[noreturn]] void yyfatal(const char* s, ...);
void yymissing(const char* what, const char* type, const char* name);
void omg_namespace(const char* name);
void omg_client(unsigned long clientType);
void omg_enable_debug();
void omg_start_y(int value);
void omg_type_decl(TypeDef&& typeDef);
void omg_start_message(const char* name);
void omg_message_attr(unsigned long);
void omg_end_message();
void omg_start_group(const char* name);
void omg_end_group();
void omg_start_field(const char* type, const char* name);
void omg_add_field_option(const char* optName, const char* optValue);
void omg_field_dimension(const char* dim);
void omg_field_bits(const char*);
void omg_end_field();
void omg_add_code(const char* code);
void omg_start_if(const char* condition);
void omg_end_if();
void omg_start_array(std::string structName, std::string typedefName, std::string memberName, std::string dimension);
void omg_end_array();
void omg_using(std::string filename);
void omg_eof();
void omg_add_include(const char* filename);
void omg_start_enum(const char* enumName);
void omg_end_enum();
void omg_add_enum_value(const char* name);

const char* omg_current_node_name();

extern long long leakCount;
