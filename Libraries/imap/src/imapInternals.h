#pragma once

#include "imap.h"

/************************************************************************************************
	imap internal data structures. Nothing outside of libimap sees this.
*************************************************************************************************/
// CFML limits
#define MAX_PARSE_STR_LEN 	127 // max length of any string in cfml file
#define MAX_PARSE_ENTRIES 	64 	// max number of tags within a control tag
#define MAX_PARSE_ELEMENTS 	64 	// max number tags within a combo or keyaxis

struct imap_keymap_entry
{
	struct imap_keymap_entry* prev;
	struct imap_keymap_entry* next;

	unsigned int idx;
	unsigned int functionCode;

	imap_keymap keymap;
};

struct imap_keymap_list
{
	imap_keymap_entry* keymapEntries; // sorted list of keymap entries (by functionCode, keymapType and idx)
};

// processInputs
struct imap_key_state
{
	struct imap_key_state* next;
	
	unsigned int iac;
	imap_iac_info iacInfo;
	
	int state;
	int count;

	// $#@!'ing hacks
	int gotinput;
	int emulateKeyUp;
	int executingCombo;
	
};

struct imap_func_axis_state
{
	struct imap_func_axis_state* next;
	
	unsigned int functionCode;	
	int param;
	
};

struct imap_calibration
{
	struct imap_calibration* next;
	
	unsigned int iac;
	
	int scaleType;
	int numParams;
	float* params;
	
	int axisMap[IMAP_MAX_VALUE+1];
};

struct imap_compiled_keymap_entry
{
	struct imap_compiled_keymap_entry* next;
	struct imap_compiled_keymap_entry* prev;
	
	unsigned int idx;

	unsigned int functionCode;
	int param;
	int executing;
	int lastExecuteTime;
	imapControlCallback controlCallback;
	
	imap_keymap keymap;

	imap_key_state* keyState[IMAP_MAX_COMBO_KEYS];
	imap_func_axis_state* funcAxisState;
	imap_function_info funcInfo;
	imap_calibration* calibration;
	
	int execute;
	int disabled;
};

// imap
struct imap_function
{	
	struct imap_function* prev;
	struct imap_function* next;
	
	unsigned int functionCode;
	int isAxis;
	
	int removable; // don't really like this, for compat with old imap.
	imapControlCallback callback;
	
};

struct imap
{
	imap_function* functionTable;
	imapKeymapRef keymaps[IMAP_MAX_KEYMAPS];

	imap_compiled_keymap_entry* compiledKeymap;
	imap_key_state* keyState; // sorted by iac
	imap_func_axis_state* funcAxisState; // keep track of axis state across multiple entries for same axis
	imap_calibration* calibration; // calibration info for axes
	
	imapReadCallback read;
	imapWriteCallback write;
	imapIACInfoCallback iacInfo;
	imapFunctionInfoCallback functionInfo;

	unsigned buttonInputEnabled:1;
	unsigned axisInputEnabled:1;
	unsigned compile:1;
	unsigned updateControlCallbacks:1;
	
	int inited;
};

// parser
struct parser_entry_data
{
	int combo;
	int keyAxisType;
	int keyDeltaType;
	float keyAxisValue;
	int onRelease;
	float releasePercent;
	
	int noedit;
 	int idx;
	int deviceIdx[MAX_PARSE_ELEMENTS];
	int deviceType[MAX_PARSE_ELEMENTS];
};

struct parser_data
{
	imapKeymapRef ref;
	void* userdata; // userdata passed to keymap load
	
	int err;
	
	char language[MAX_PARSE_STR_LEN+1];
	char keyboard[MAX_PARSE_STR_LEN+1];
	
	char data[MAX_PARSE_STR_LEN+1];
	
	unsigned int functionCode;

	int numEntries;
	parser_entry_data entryData[MAX_PARSE_ENTRIES];
	imap_keymap_entry keymapEntries[MAX_PARSE_ENTRIES];
		
};
