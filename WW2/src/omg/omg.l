/* Flex file */

%{
#include "omg.h"
#include "omg.tab.h"
#include <string>
#include <utility>

extern YYSTYPE yylval;

static int codeFragStart;
static std::string g_funcBody;
static std::string g_condition;

extern void yyerror(const char* fmt, ...);
extern void yyaddcode(const char* text, std::string& into);
%}

%option				noyywrap
%option				yylineno
%option				stack
%option				nounistd
%option				never-interactive

%x ML_CMT
%x IF_COND
%x CODE_FRAG

%%

	/* {{ ... }} code fragments */
"{{"[ ]*				{ codeFragStart = yylineno; g_funcBody = ""; yy_push_state(CODE_FRAG); }
<CODE_FRAG><<EOF>>		{ yyerror("unterminated code fragment from line %u\n", codeFragStart); yyterminate(); }
<CODE_FRAG>"}}"			{ yy_pop_state(); yylval.sVal = g_funcBody.c_str(); return TCODE_FRAG; }
<CODE_FRAG>.			{ yyaddcode(yytext, g_funcBody); }
<CODE_FRAG>\n			{ yyaddcode(yytext, g_funcBody); }

	/* multi-line comments */
<INITIAL,IF_COND,ML_CMT>"/*"	yy_push_state(ML_CMT);
<ML_CMT>"*/"			yy_pop_state();
<ML_CMT>\n
<ML_CMT>.?

	/* single-line comments */
"//".*

	/* declaration keywords */
DEBUG					return TKW_DEBUG;
NAMESPACE				return TKW_NAMESPACE;
TYPE					return TKW_TYPE;
AS						return TKW_AS;
BY\ +REF				return TKW_BY_REF;
INCLUDE					return TKW_INCLUDE;

	/* statement keywords */
MESSAGE					return TKW_MESSAGE;
GROUP					return TKW_GROUP;

	/* message keywords */
IF\ \(					{ yy_push_state(IF_COND); g_condition = ""; }
<IF_COND>\(				{ yy_push_state(IF_COND); g_condition.push_back('('); }
<IF_COND>\)				{
	yy_pop_state();
	if (yy_top_state() != IF_COND) {
		while (g_condition.empty() == false && isspace(g_condition.back())) {
			g_condition.resize(g_condition.size() - 1);
		}
		yylval.sVal = g_condition.c_str();
		return TKW_IF;
	}
	g_condition.push_back(')');
}
<IF_COND>\n				{ yyaddcode(yytext, g_condition); }
<IF_COND>.				{ yyaddcode(yytext, g_condition); }
ARRAY					return TKW_ARRAY;

	/* C++ */
::						return TKW_CPP_SCOPE;

	/* general types */
[A-Za-z][A-Za-z0-9_]{64,}	{ yyfatal("Invalid identifier '%s'", yytext); }
[A-Za-z][A-Za-z0-9_]*		{ yylval.sVal = strdup(yytext); return TIDENTIFIER; }
-?[0-9]{21,}\.[0-9]\+		{ yyfatal("Invalid number '%s'", yytext); }
-?[0-9]{21,}				{ yyfatal("Invalid number '%s'", yytext); }
-?[0-9]\+\.[0-9]+\.			{ yyfatal("Invalid floating pointer number '%s'", yytext); }
-?[0-9]{10,}\.[0-9]{10,}	{ yyfatal("Invalid floating pointer number '%s'", yytext); }
-?[0-9]+\.[0-9]+			{ yylval.str = strdup(yytext); ++leakCount; return TDOUBLE; }
-?[0-9]{1,20}				{ yylval.str = strdup(yytext); ++leakCount; return TINTEGER; }

	/* quoted string */
\"[^"+]\"				{ char* str = strdup(yytext + 1); str[strlen(str) - 1] = '\0'; yylval.str = str; ++leakCount; return QUOTSTRING; }

[{};()\[\]]				{ return yytext[0]; }

	/* misc */
[ \t]					/* skip whitespace */
\n

.						{ yyerror("Unexpected string '%s'\n", yytext); yyterminate(); }
