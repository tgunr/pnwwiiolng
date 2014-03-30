#pragma once

#include <utility>
#include <string>

extern const char* g_currentFile;
extern int yylineno;

// Functions common to omg.cpp and the .y file.
void yyerror(const char* s, ...);
void yyfatal(const char* s, const char* cause);
void yymissing(const char* what, const char* type, const char* name);
void omg_enable_debug();
void omg_namespace(const char* name);
void omg_type_decl(const char* name);
void omg_start_message(const char* name);
void omg_end_message();
void omg_start_group(const char* name);
void omg_end_group();
void omg_start_field(const char* type, const char* name);
void omg_end_field();
void omg_add_code(const char* code);
void omg_start_if(const char* condition);
void omg_end_if();