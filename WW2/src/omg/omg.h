#pragma once

#include <utility>
#include <string>

class TypeDef
{
	std::string					m_name;
	std::string					m_declType;
	bool						m_passByRef;
	std::string					m_include;

public:
	TypeDef(std::string&& name, std::string&& declType, bool passByRef, std::string&& include)
		: m_name(std::move(name))
		, m_declType(std::move(declType))
		, m_passByRef(passByRef)
		, m_include(include)
	{
	}

	const std::string& Name() const { return m_name; }
	const std::string& DeclType() const { return m_declType.empty() ? Name() : m_declType; }
	bool PassByRef() const { return m_passByRef; }
	const std::string& Include() const { return m_include; }
};

extern const char* g_currentFile;
extern int yylineno;

// Functions common to omg.cpp and the .y file.
void yyerror(const char* s, ...);
void yyfatal(const char* s, const char* cause);
void yymissing(const char* what, const char* type, const char* name);
void omg_enable_debug();
void omg_namespace(const char* name);
void omg_type_decl(TypeDef&& typeDef);
void omg_start_message(const char* name);
void omg_end_message();
void omg_start_group(const char* name);
void omg_end_group();
void omg_start_field(const char* type, const char* name);
void omg_end_field();
void omg_add_code(const char* code);
void omg_start_if(const char* condition);
void omg_end_if();