/* Flex file */

%{
#include "omggen.h"
#include "omggen.tab.h"

extern YYSTYPE yylval;

static int codeFragStart;
static std::string g_funcBody;
static std::string g_condition;

extern void yyerror(const char* fmt, ...);
extern void yyaddcode(const char* text, std::string& into);

#ifdef WIN32
#include <io.h>
#endif
%}

%option				noyywrap yylineno stack nounistd never-interactive batch fast

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
NAMESPACE				return TKW_NAMESPACE;
CLIENT					return TKW_CLIENT;
FRONTEND				return TKW_FRONTEND;
SERVER					return TKW_SERVER;
DEBUG					return TKW_DEBUG;
START-Y					return TKW_START_Y;
TYPE					return TKW_TYPE;
AS						return TKW_AS;
BY\ +REF				return TKW_BY_REF;
USING					return TKW_USING;
INCLUDE					return TKW_INCLUDE;

	/* statement keywords */
ENUM					return TKW_ENUM;
MESSAGE					return TKW_MESSAGE;
GROUP					return TKW_GROUP;

	/* field options */
[Bb][Ii][Tt][Ss]		return TKW_FIELD_BITS;

	/* message keywords */
TOCLIENT				return TKW_TOCLIENT;
TOHOST					return TKW_TOHOST;
GMONLY					return TKW_GMONLY;
INTERNAL				return TKW_INTERNAL;
IF\ \(					{ yy_push_state(IF_COND); g_condition = ""; }
<IF_COND>\(				{ yy_push_state(IF_COND); g_condition.push_back('('); }
<IF_COND>\)				{
	yy_pop_state();
	/* A bug in flex will crash if yy_top_state() is called in INITIAL */
	if (yy_start_stack_ptr == 0 || yy_top_state() != IF_COND) {
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
[A-Za-z][A-Za-z0-9_]*		{ yylval.str = strdup(yytext); ++leakCount; return TIDENTIFIER; }
-?[0-9]{21,}\.[0-9]\+		{ yyfatal("Invalid number '%s'", yytext); }
-?[0-9]{21,}				{ yyfatal("Invalid number '%s'", yytext); }
-?[0-9]\+\.[0-9]+\.			{ yyfatal("Invalid floating pointer number '%s'", yytext); }
-?[0-9]{10,}\.[0-9]{10,}	{ yyfatal("Invalid floating pointer number '%s'", yytext); }
-?[0-9]+\.[0-9]+			{ yylval.str = strdup(yytext); ++leakCount; return TDOUBLE; }
-?[0-9]{1,20}				{ yylval.str = strdup(yytext); ++leakCount; return TINTEGER; }

	/* quoted string */
\"(\\\\|\\\"|[^\r\n"])*\"	{ char* str = strdup(yytext + 1); str[strlen(str) - 1] = '\0'; yylval.str = str; ++leakCount; return QUOTSTRING; }

[{};()\[\]=,]				{ return yytext[0]; }

	/* misc */
[ \t]						/* skip whitespace */
\n

.							{ yyfatal("Unexpected string '%s'\n", yytext); }

<<EOF>>						{ omg_eof(); if (!YY_CURRENT_BUFFER) yyterminate(); }
