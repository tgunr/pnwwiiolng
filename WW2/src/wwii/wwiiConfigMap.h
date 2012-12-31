#pragma once

// WWII Config variable types
enum WWII_CONFIG_TYPE
{
	TYPE_UNUSED						//!< Unused variable
,	TYPE_INT						//!< ATOL conversion on parameter
,	TYPE_BOOL						//!< Yes/No flag
,	TYPE_STRING						//!< Its a string
,	TYPE_SINT						//!< Short Int
,	TYPE_BYTE						//!< A byte value
,	TYPE_FLOAT						//!< Float32
,	MAX_WWII_CONFIG_TYPES
} ;

// Structures
struct WWII_CONFIG_MAP
{
	const char*			name ;		// Variable name
	WWII_CONFIG_TYPE	type ;
	void*				data ;
	UINT64				max ;
	const char*			defaultValue ;
	const char*			description ;
} ;

extern WWII_CONFIG_MAP configMap[] ;

