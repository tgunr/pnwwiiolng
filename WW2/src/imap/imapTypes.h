#pragma once

/************************************************************************************************
	imaptypes
*************************************************************************************************/
/** \file imapTypes.h */

#define IMAP_MAX_COMBO_KEYS 4 		// max number of keys for a combo
#define IMAP_MAX_KEYMAPS 10		  	// idx < for imapSetKeymap
#define IMAP_NOEDIT_ID 0x80000000	// For NOEDIT CFML tag. Keymaps with ids greater than this should not be modified by app.
#define IMAP_IAC_NOKEY 0x0			// Special IAC code for no key
#define IMAP_MAX_VALUE 1000			// imap param values are from -1000 to 1000

/// Keystroke mapping
struct imap_button_keymap
{
	unsigned int iac[IMAP_MAX_COMBO_KEYS];	// Input Action Code from old imap.
};

/// Axis mapping
struct imap_axis_keymap
{
	unsigned int iac;
	signed range:2;				// For half axis mapping, -1 (negative half), 0 (full axis, default), +1 (postive half).
	unsigned invert:1;			// Reverse normal axis behavior.
	unsigned emulateJoystick:1; // For mouse axes.
};

/// Relative Keyboard Axis mapping
struct imap_axis_rel_keymap
{
	unsigned int iac[IMAP_MAX_COMBO_KEYS];
	int axisMapType;	// IMAP_AXIS_PER_STROKE or IMAP_AXIS_PER_SEC
	float rate;	// 0.0 - 1.0 per keypress or second
};

/// Absolute Keyboard Axis mapping
struct imap_axis_abs_keymap
{
	unsigned int iac[IMAP_MAX_COMBO_KEYS];
	float percent;			// 0.0 - 1.0
	unsigned onRelease : 1; // Do we send a value on key up?
	float releasePercent; 	// 0.0 - 1.0
};

struct imap_keymap
{
	int numKeys;
	int keymapType;
	union
	{
		imap_button_keymap 		btn;
		imap_axis_keymap		axis;
		imap_axis_rel_keymap	axisRel;
		imap_axis_abs_keymap	axisAbs;
	};
};

/// Entry for imapProcessInputs queue.
/**
*	The calling application needs to pass a queue of keystrokes and axis states
*	to imapProcessInputs.
*/
// Inherited from old imap code.
struct imap_queue_entry
{
	unsigned int iac;	// App-defined key/axis code.
	int	count;			// For axes, value scaled from to -1000 to 1000.
	int	state;			// For keys, on/off.
};

///
struct imap_iac_info
{
	int deviceIdx;
	int deviceType;
	int isAxis;
	int axisType;
};

///
struct imap_function_info
{
	int isAxis;
	int axisType;
};

/// 3rf parameter passed to imap callbacks
struct imap_callback_ext_data
{
	int axisType;	// type of axis input
	int deltaInput;
	int emulateJoystick;
};

/// Keymap type for imapKeymap(Get/Set)
enum IMAPTYPES
{
	IMAP_BUTTON_KEYMAP = 1,
	IMAP_AXIS_KEYMAP,
	IMAP_AXIS_REL_KEYMAP,
	IMAP_AXIS_ABS_KEYMAP
};

///
enum
{
	IMAP_SYMMETRIC_AXIS = 1,
	IMAP_ASYMMETRIC_AXIS,
	IMAP_DELTA_AXIS
};

/// Keyboard Axis types
enum
{
	IMAP_AXIS_PER_STROKE = 1,	// Axis value incremented per key press.
	IMAP_AXIS_PER_SEC		// Axis value incremented per second key held down.
};

/// Modes for info callbacks
enum
{
	IMAP_HAVE_NAME,
	IMAP_HAVE_CODE
};

/// Type codes returned by imapKeymapInfoCallback
enum
{
	IMAP_IAC_IS_KEYBOARD = 1,
	IMAP_IAC_IS_MOUSE,
	IMAP_IAC_IS_JOYSTICK
};

/// Scaling types for imapCalibrateAxis
enum
{
	IMAP_CALIBRATE_LINEAR = 1,	// takes one param, the scale factor, for mouse axes
	IMAP_CALIBRATE_BEZIER		// takes four params, deadband, x, y, fullband
};

/// Error codes
enum
{
	IMAP_NO_ERR 				=  0,
	IMAP_ERR					= -1,
	IMAP_FUNCTION_NOT_AXIS_ERR	= -2,
	IMAP_FUNCTION_IS_AXIS_ERR	= -3,
	IMAP_CFML_PARSER_ERR		= -4,
	IMAP_MEMORY_ERR				= -5,
	IMAP_KEYMAP_NOT_FOUND_ERR	= -6,
	IMAP_FUNCTION_NOT_FOUND_ERR = -7,
	IMAP_KEYMAP_SAVE_ERR		= -8,
	IMAP_KEYMAP_LOAD_ERR		= -9,
	IMAP_MAX_KEYMAPS_ERR		= -10
};

/// Opaque reference type for Keymap object.
using imapKeymapRef = struct imap_keymap_list*;

///	Callback for imapInstallControlCallback.
// 	Function signature inherited from old imap code for compatability.
using imapControlCallback = unsigned int(*)(int param, unsigned int functionCode, unsigned int callbackinfo);

/// Read callback for imapInstallCallbacks.
using imapReadCallback = int (*)(void* userdata, int numbytes, void* buf);

/// Write callback for imapInstallCallbacks.
using imapWriteCallback = int(*)(void* userdata, int numbytes, const void* buf);

/// IAC info callback for imapInstallCallbacks.
/**  modes - IMAP_HAVE_NAME, IMAP_HAVE_CODE */
using imapIACInfoCallback = int(*)(int mode, const char* language, const char* keyboard, unsigned int* iac,
									char* name, int maxlen, imap_iac_info* info);

/// Function info callback for imapInstallCallbacks.
/** modes - IMAP_HAVE_NAME, IMAP_HAVE_CODE */
using imapFunctionInfoCallback = int(*)(int mode, const char* language, unsigned int* functionCode,
											char* name, int maxlen, imap_function_info* info);
