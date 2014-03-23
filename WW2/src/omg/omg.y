%{
#include "omg.yy.h"
#include "omg.h"

#if defined(_MSC_VER)
#pragma warning( disable : 4065 )		// Disable warning C4065: switch statement contains 'default' but no 'case' labels
#endif

%}

%union {
	int			iVal;
	double		dVal;	
	const char*	sVal;
}

// declarations
%token			TKW_DEBUG
%token			TKW_NAMESPACE
%token			TKW_TYPE

// statement keywords
%token			TKW_MESSAGE TKW_GROUP

// C++ keywords
%token			TKW_CPP_SCOPE

%token <sVal>	TIDENTIFIER TCODE_FRAG
%token <iVal>	TINTEGER
%token <dVal>	TDOUBLE

%type <sVal>	valid-group
%type <sVal>	valid-message
%type <sVal>	valid-field
%type <sVal>	qualified-type

%%

file			:	directives definitions
				|	definitions
				;

directives		:	directive
				|	directives directive
				;

directive		:	TKW_DEBUG ';' { omg_enable_debug(); }
				|	TKW_DEBUG { yymissing("semicolon", "keyword", "DEBUG"); }
				|	TKW_NAMESPACE TIDENTIFIER ';' { omg_namespace($2); }
				|	TKW_NAMESPACE TIDENTIFIER { yymissing("semicolon", "keyword", "NAMESPACE"); }
				|	TKW_NAMESPACE { yymissing("identifier", "keyword", "NAMESPACE"); }
				|	TKW_TYPE qualified-type ';' { omg_type_decl($2); }
				|	TKW_TYPE qualified-type { yymissing("semicolon", "keyword", "TYPE"); }
				|	TKW_TYPE { yymissing("typename", "keyword", "TYPE"); }
				;

definitions		:	definition
				|	definitions definition
				;

definition		:	message
				;
				
				
message			:	valid-message ';'
				|	valid-message { yymissing("semicolon", "MESSAGE", $1); }
				;

valid-message	:	TKW_MESSAGE TIDENTIFIER	{ omg_start_message($2); } message-body { omg_end_message(); $$ = $2; }
				|	TKW_MESSAGE TIDENTIFIER	{ yymissing("open-brace ('{')", "MESSAGE", $2); }
				|	TKW_MESSAGE { yymissing("identifier", "keyword", "MESSAGE"); }
				;

message-body	:	'{' '}'
				|	'{' message-stmts '}'
				;

message-stmts	:	message-stmt
				|	message-stmts message-stmt
				;

message-stmt	:	group
				|	func
				|	field
				;

group			:	valid-group ';' 
				|	valid-group { yyfatal("Missing semicolon after GROUP '%s'", $1); }
				;

valid-group		:	TKW_GROUP TIDENTIFIER { omg_start_group($2); } '{' fields '}' { omg_end_group(); $$ = $2; }
				|	TKW_GROUP TIDENTIFIER { yymissing("open-brace ('{')", "GROUP", $2); }
				;
				
fields			:	field
				|	fields field
				;
				
field			:	valid-field ';'
				|	valid-field { yyfatal("Missing trailing semicolon after field '%s'", $1); }
				;

valid-field		:	qualified-type TIDENTIFIER { omg_start_field($1, $2); } field-opts { omg_end_field(); $$ = $2; }
				;

field-opts		:	/* empty */
				;

func			:	TCODE_FRAG { omg_add_code($1); } ;
				
qualified-type	:	TIDENTIFIER
				|	qualified-type TKW_CPP_SCOPE TIDENTIFIER
				;
