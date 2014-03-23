/* Flex file */

%{
#include "omg.tab.h"
#include "omg.h"
#include <string>
#include <utility>

extern YYSTYPE yylval;

static int codeFragStart;
static std::string g_funcBody;

extern void yyerror(const char* fmt, ...);
extern void yyaddcode(const char* text, std::string& into);
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
"{{"[ ]*				{ codeFragStart = yylineno; g_funcBody = ""; yy_push_state(CODE_FRAG); }
<CODE_FRAG><<EOF>>		{ yyerror("unterminated code fragment from line %u\n", codeFragStart); yyterminate(); }
<CODE_FRAG>"}}"			{ yy_pop_state(); yylval.sVal = g_funcBody.c_str(); return TCODE_FRAG; }
<CODE_FRAG>.			{ yyaddcode(yytext, g_funcBody); }
<CODE_FRAG>\n			{ yyaddcode(yytext, g_funcBody); }

	/* multi-line comments */
<INITIAL,ML_CMT>"/*"	yy_push_state(ML_CMT);
<ML_CMT>"*/"			yy_pop_state();
<ML_CMT>\n
<ML_CMT>.?

	/* single-line comments */
"//".*

	/* declaration keywords */
DEBUG					return TKW_DEBUG;
NAMESPACE				return TKW_NAMESPACE;
TYPE					return TKW_TYPE;

	/* statement keywords */
MESSAGE					return TKW_MESSAGE;
GROUP					return TKW_GROUP;

	/* C++ */
::						return TKW_CPP_SCOPE;

	/* general types */
[A-Za-z][A-Za-z0-9_]*	{ yylval.sVal = strdup(yytext); return TIDENTIFIER; }
[0-9]+\.[0-9]+			return TDOUBLE;
[0-9]+					return TINTEGER;

[{};()]					{ return yytext[0]; }

	/* misc */
[ \t]					/* skip whitespace */
\n

.						{ yyerror("Unexpected string '%s'\n", yytext); yyterminate(); }
