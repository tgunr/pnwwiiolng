/* Flex file */

%{
#include <string>
#include <utility>
#include "omg.tab.h"

extern YYSTYPE yylval;

extern void yyerror(const char* message);
extern void yyerrorDetail(const char* message, const char* text);

extern int yylineno;
extern const char* g_currentFile;
std::string g_funcBody;

void add_code(const char* line)
{
	char annotation[256];
	sprintf(annotation, "#line %u \"%s\"\n", yylineno, g_currentFile);
	g_funcBody += annotation;
	g_funcBody += line;
	g_funcBody += '\n';
}
%}

%option				noyywrap
%option				yylineno
%option				stack
%option				nounistd
%option				never-interactive

%x ML_CMT
%x CODE_FRAG

%%

	/* {{ ... }} code fragments */
"{{"[ ]*				{ g_funcBody = ""; yy_push_state(CODE_FRAG); }
<CODE_FRAG>"}}"			{ yy_pop_state(); yylval.sVal = g_funcBody.c_str(); return TCODE_FRAG; }
<CODE_FRAG>.*?/"}}"		{ add_code(yytext); }
<CODE_FRAG>.*?			{ add_code(yytext); }
<CODE_FRAG>\n

	/* multi-line comments */
<INITIAL,ML_CMT>"/*"	yy_push_state(ML_CMT);
<ML_CMT>"*/"			yy_pop_state();
<ML_CMT>\n
<ML_CMT>.?

	/* single-line comments */
"//".?

	/* declaration keywords */
TYPE				return TKW_TYPE;
NAMESPACE			return TKW_NAMESPACE;

	/* statement keywords */
MESSAGE				return TKW_MESSAGE;
GROUP				return TKW_GROUP;

	/* field definition options */
HOIST[ \t]+AS		return TKW_HOIST_AS;

	/* C++ */
const				return TKW_CONST;
void				return TKW_VOID;
::					return TKW_CPP_SCOPE;

	/* general types */
[A-Za-z][A-Za-z0-9_]*			{ yylval.sVal = strdup(yytext); return TIDENTIFIER; }
[0-9]+\.[0-9]+		return TDOUBLE;
[0-9]+				return TINTEGER;

[{};()]				{ return yytext[0]; }

	/* misc */
[ \t]				/* skip whitespace */
\n

.					{ yyerrorDetail("Unexpected string", yytext); exit(1); }


