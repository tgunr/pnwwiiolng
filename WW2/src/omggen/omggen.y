%{
#include "omggen.yy.h"
#include "omggen.h"

#if defined(_MSC_VER)
#pragma warning( disable : 4065 )		// Disable warning C4065: switch statement contains 'default' but no 'case' labels
#endif

%}

%union {
	const char*	sVal;
	char*		str;
	bool		bVal;
}

%destructor { if ($<str>$) { free($<str>$); --leakCount; } } <str>

// declarations
%token			TKW_DEBUG
%token			TKW_NAMESPACE
%token			TKW_TYPE
%token			TKW_AS
%token			TKW_BY_REF
%token			TKW_USING
%token			TKW_INCLUDE

// statement keywords
%token			TKW_MESSAGE TKW_GROUP
%token <sVal>	TKW_IF
%token <sVal>	TKW_ARRAY

// C++ keywords
%token			TKW_CPP_SCOPE

// misc
%token <str>	TIDENTIFIER
%token <str>	TINTEGER
%token <str>	TDOUBLE
%token <str>	QUOTSTRING
%token <sVal>	TCODE_FRAG

%type <str>		include-filename
%type <sVal>	valid-group
%type <sVal>	valid-field
%type <sVal>	typename scoped-identifier
%type <sVal>	typedef-as typedef-inc
%type <bVal>	typedef-ref
%type <sVal>	message-id
%type <str>		array-typename array-typedef array-membername array-dimension

%%

file				:	directives
					;

directives			:	directive ';'
					|	directives directive ';'
					;

directive			:	debug-directive 
					|	namespace-directive
					|	type-directive
					|	using-directive
					|	include-filename { omg_add_include($1); }
					|	message-directive
					;

debug-directive		:	TKW_DEBUG { omg_enable_debug(); }
					;

namespace-directive	:	TKW_NAMESPACE TIDENTIFIER { omg_namespace($2); }
					|	TKW_NAMESPACE { yymissing("identifier", "keyword", "NAMESPACE"); }
					;

type-directive		:	TKW_TYPE TIDENTIFIER typedef-as typedef-ref typedef-inc { omg_type_decl(TypeDef($2, $3, $4, $5)); }
					|	TKW_TYPE scoped-identifier { yyfatal("TYPE declarations must specify a non-qualified typename. Use TYPE .. AS %s instead", $2); }
					;

typedef-as			:	/* empty */ { $$ = strdup(""); }
					|	TKW_AS typename { $$ = strdup($2); }
					;

typedef-ref			:	/* empty */ { $$ = false; }
					|	TKW_BY_REF { $$ = true; }
					;

typedef-inc			:	/* empty */ { $$ = strdup(""); }
					|	TKW_INCLUDE QUOTSTRING { $$ = strdup($2); }
					;

using-directive		:	TKW_USING QUOTSTRING { omg_using($2); }
					;

include-filename	:	TKW_INCLUDE QUOTSTRING { $$ = strdup($2); }
					;

message-directive	:	message-id message-body { omg_end_message(); }
					|	message-id { yymissing("open-brace ('{')", "MESSAGE", $1); }
					|	TKW_MESSAGE { yymissing("identifier", "keyword", "MESSAGE"); }
					;

message-id			:	TKW_MESSAGE TIDENTIFIER	{ omg_start_message($2); $$ = strdup($2); } 
					;

message-body		:	'{' '}'
					|	'{' message-stmts '}'
					;

message-stmts		:	message-stmt
					|	message-stmts message-stmt
					;

message-stmt		:	field
					|	array
					|	group
					|	conditional-block
					|	func
					;

conditional-stmt	:	field
					|	array
					|	conditional-block
					;

array-stmt			:	field
					|	array
					|	conditional-block
					;

group-stmt			:	field
					|	func
					;

group				:	valid-group ';' 
					|	valid-group { yymissing("semicolon", "GROUP", $1); }
					;

valid-group			:	TKW_GROUP TIDENTIFIER { omg_start_group($2); } '{' group-body '}' { omg_end_group(); $$ = $2; }
					|	TKW_GROUP TIDENTIFIER { yymissing("open-brace ('{')", "GROUP", $2); }
					;

group-body			:	group-stmt
					|	group-body group-stmt
					;

field				:	valid-field ';'
					|	valid-field { yyfatal("Missing trailing semicolon after field '%s'", $1); }
					;

valid-field			:	typename TIDENTIFIER { omg_start_field($1, $2); } field-opts { omg_end_field(); $$ = $2; }
					;

field-opts			:	/* empty */
					;

func				:	TCODE_FRAG { omg_add_code($1); }
				
typename			:	TIDENTIFIER { $<sVal>$ = $<str>1; }
					|	scoped-identifier
					;

scoped-identifier	:	TIDENTIFIER TKW_CPP_SCOPE TIDENTIFIER { char* str = (char*)malloc(strlen($1) + 2 + strlen($3) + 1); sprintf(str, "%s::%s", $1, $3); $$ = str; }
					|	scoped-identifier TKW_CPP_SCOPE TIDENTIFIER { char* str = (char*)malloc(strlen($1) + 2 + strlen($3) + 1); sprintf(str, "%s::%s", $1, $3); $$ = str; }
					;

conditional-block	:	TKW_IF { omg_start_if($1); } conditional-body { omg_end_if(); }
					;

conditional-body	:	'{' conditional-stmts '}'
					|	'{' '}' { yyerror("Empty conditional block (IF {})"); exit(1); }
					;

conditional-stmts	:	conditional-stmt
					|	conditional-stmts conditional-stmt
					;

array				:	TKW_ARRAY array-decl array-body { omg_end_array(); }
					;

array-decl			:	array-typename array-typedef array-membername array-dimension { omg_start_array($1, $2, $3, $4); }
					|	TIDENTIFIER { yymissing("Array dimension ('[...]')", "ARRAY", $1); }
					|	{ yyerror("Expected an identifier after '%s' keyword", "ARRAY"); exit(1); }
					;

array-typename		:	TIDENTIFIER { $$ = $1; }
					;

array-typedef		:	/* empty */	{ leakCount++; $$ = strdup(""); }
					|	'(' TIDENTIFIER ')' { $$ = $2; }
					|	'(' TIDENTIFIER { yymissing("close parenthesis", "ARRAY typedef", $2); $$ = nullptr; }
					;

array-membername	:	/* empty */	{ leakCount++; $$ = strdup(""); }
					|	TIDENTIFIER { $$ = $1; }
					;

array-dimension		:	'[' TINTEGER ']' { $$ = $2; }
					|	'[' TIDENTIFIER ']' { $$ = $2; }
					|	'[' ']' { yyfatal("Maximum %s size must be specified", "ARRAY"); $$ = nullptr; }
					;

array-body			:	'{' array-stmts '}'
					|	'{' '}' { yyfatal("Empty array body: %s", omg_current_node_name()); }
					;

array-stmts			:	array-stmt
					|	array-stmts array-stmt
					;