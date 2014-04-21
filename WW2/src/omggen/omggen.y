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
%type <sVal>	valid-message
%type <sVal>	valid-field
%type <sVal>	qualified-type
%type <typeDef>	type-definition
%type <sVal>	typedef-as
%type <bVal>	typedef-ref
%type <str>		array-typename array-typedef array-membername array-dimension

%%

file				:	optional-directives definitions
					;

optional-directives	:	/* empty */
					|	directives
					;

directives			:	directive
					|	directives directive
					;

directive			:	TKW_DEBUG ';' { omg_enable_debug(); }
					|	TKW_DEBUG { yymissing("semicolon", "keyword", "DEBUG"); }
					|	TKW_NAMESPACE TIDENTIFIER ';' { omg_namespace($2); }
					|	TKW_NAMESPACE TIDENTIFIER { yymissing("semicolon", $2, "NAMESPACE"); }
					|	TKW_NAMESPACE { yymissing("identifier", "keyword", "NAMESPACE"); }
					|	TKW_TYPE type-definition ';'
					|	TKW_TYPE qualified-type { yyfatal("TYPE declarations must specify a non-qualified typename. Use TYPE .. AS %s instead", $2); }
					|	TKW_TYPE { yyfatal("Malformed TYPE definition", ""); }
					|	include-filename { omg_add_include($1); }
					;

type-definition		:	TIDENTIFIER typedef-as typedef-ref include-filename { omg_type_decl(TypeDef($1, $2, $3, $4)); }
					|	TIDENTIFIER typedef-as typedef-ref { omg_type_decl(TypeDef($1, $2, $3, "")); }
					;

typedef-as			:	TKW_AS qualified-type { $$ = $2; }
					;

typedef-ref			:	TKW_BY_REF { $$ = true; }
					;

include-filename	:	TKW_INCLUDE QUOTSTRING { $$ = strdup($2); }
					;

definitions			:	definition
					|	definitions definition
					;

definition			:	message
					;
				
message				:	valid-message ';'
					|	valid-message { yymissing("semicolon", "MESSAGE", $1); }
					;

valid-message		:	TKW_MESSAGE TIDENTIFIER	{ omg_start_message($2); } message-body { omg_end_message(); $$ = $2; }
					|	TKW_MESSAGE TIDENTIFIER	{ yymissing("open-brace ('{')", "MESSAGE", $2); }
					|	TKW_MESSAGE { yymissing("identifier", "keyword", "MESSAGE"); }
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

valid-field			:	qualified-type TIDENTIFIER { omg_start_field($1, $2); } field-opts { omg_end_field(); $$ = $2; }
					;

field-opts			:	/* empty */
					;

func				:	TCODE_FRAG { omg_add_code($1); }
				
qualified-type		:	TIDENTIFIER { $<sVal>$ = $<str>1; }
					|	qualified-type TKW_CPP_SCOPE TIDENTIFIER { char* str = (char*)malloc(strlen($1) + 2 + strlen($3) + 1); sprintf(str, "%s::%s", $1, $3); $$ = str; }
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