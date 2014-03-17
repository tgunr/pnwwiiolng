%{
#include <cstdio>
#include <iostream>
#include <string>
#include "omg.yy.h"

const char* g_currentFile = nullptr;
extern int yylineno;

void yyerror(const char* s)
{
	fprintf(stderr, "%s:%u: %s\n", g_currentFile, yylineno, s);
}
void yyerrorDetail(const char* s, const char* text)
{
	fprintf(stderr, "%s:%u: %s: %s\n", g_currentFile, yylineno, s, text);
}

void omg_type_decl(const char* str)
{
	printf("TYPE %s\n", str);
}

void omg_namespace(const char* str)
{
	printf("NAMESPACE %s\n", str);
}

void omg_start_message(const char* str)
{
	std::cout << "START MESSAGE " << str << "\n";
}

void omg_end_message(const char* str)
{
	std::cout << "END MESSAGE " << str << "\n";
}

void omg_start_field(const char* str)
{
	std::cout << "START FIELD " << str << "\n";
}

void omg_end_field(const char* str)
{
	std::cout << "END FIELD " << str << "\n";
}

void omg_hoist_as(const char* str)
{
	std::cout << "HOIST_AS " << str << "\n";
}

void omg_add_code(const char* str)
{
	std::cout << "code:\n" << str << "\n";
}
%}

%union {
	int			iVal;
	double		dVal;	
	const char*	sVal;
}

// declarations
%token			TKW_TYPE TKW_NAMESPACE
// statement keywords
%token			TKW_MESSAGE TKW_GROUP
// member options
%token			TKW_HOIST_AS
// C++ keywords
%token			TKW_CONST TKW_VOID TKW_CPP_SCOPE

%token <sVal>	TIDENTIFIER TCODE_FRAG
%token <iVal>	TINTEGER
%token <dVal>	TDOUBLE

%%

file			:	declarations messages
				|	messages
				;

declarations	:	declaration
				|	declarations declaration
				;

declaration		:	TKW_TYPE TIDENTIFIER { omg_type_decl($2); } ';'
				|	TKW_NAMESPACE TIDENTIFIER { omg_namespace($2); } ';'
				;

messages		:	message
				|	messages message
				;

message			:	TKW_MESSAGE TIDENTIFIER
						{ omg_start_message($2); } 
						'{' messagebody '}'
						{ omg_end_message($2); }
						';'
				;
				
messagebody		:	/* empty */
				|	messagestmts 
				;
				
messagestmts	:	messagestmt
				|	messagestmts messagestmt
				;

messagestmt		:	group
				|	func
				|	field
				;

group			:	TKW_GROUP '{' fields '}' ';'
				;
				
fields			:	field
				|	fields field
				;
				
field			:	qualified_type TIDENTIFIER { omg_start_field($2); } fieldopts { omg_end_field($2); } ';'
				;

fieldopts		:	/* empty */
				|	fieldopts fieldopt
				;

fieldopt		:	TKW_HOIST_AS TIDENTIFIER '(' ')' { omg_hoist_as($2); }
				;

func			:	TCODE_FRAG { omg_add_code($1); } ;
				
qualified_type	:	TIDENTIFIER
				|	qualified_type TKW_CPP_SCOPE TIDENTIFIER
				;

%%

int main(int argc, const char** argv)
{
	if ( argc <= 1 ) {
		g_currentFile = "stdin";
		yyparse();
	} else {
		for (int i = 1; i < argc; ++i) {
			g_currentFile = argv[i];
			FILE* f = fopen(g_currentFile, "r");
			if (!f) {
				perror(g_currentFile);
				return 1;
			}
			yyrestart(f);
			yyparse();
			fclose(f);
		}
	}
}
