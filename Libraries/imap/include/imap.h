#ifndef _IMAP_H
#define _IMAP_H

#include "imapTypes.h"

/************************************************************************************************
	imap
*************************************************************************************************/
/**	\file imap.h */

#ifdef __cplusplus
extern "C" {
#endif

/** Must be called before using imap. */
int imapInit();

/** Call when finished using imap. */
void imapKill();

/** Installs callbacks needed by imap for I/O and information about IAC and function codes. */
void imapInstallCallbacks(imapReadCallback, imapWriteCallback, imapIACInfoCallback, imapFunctionInfoCallback);

/** Registers a function with imap. */
void imapRegisterFunction(unsigned int functionCode, int isAxis);

/** Installs the function callback for the specified function code. */
void imapInstallControlCallback(unsigned int functionCode, imapControlCallback);

/** 
*	Installs a non-removable function callback for the specified function code. 
*	I really don't like this function but put in for compatability with old imap.
*/
void imapInstallNonRemovableControlCallback(unsigned int functionCode, imapControlCallback);

/**	Removes all removable function callbacks. */
void imapRemoveControlCallbacks();

/** 
*	This function should be called every game loop. Executes the appropriate function callbacks
*	based on the user input. 
*/
void imapProcessInputs(imap_queue_entry*, int numEntries);

/** Enables or disables button input. */
void imapButtonInputState(int enable);

/** Enables or disable axis input. */
void imapAxisInputState(int enable);

/** Centers the specified axis. Useful for mouse axes emulating a joystick. */
void imapCenterAxis(unsigned int iac);

/** Axis calibration. */
void imapCalibrateAxis(unsigned int iac, int scaleType, int numParams, float* params);

/**. Clears all axis calibration. */
void imapClearAxisCalibration();

/**  Sets a keymap for imap to use. Higher indexes override lower ones. Pass nullptr to clear an index.*/
int imapSetKeymap(const imapKeymapRef, int idx);

/** Allocates a new keymap object. */
imapKeymapRef imapKeymapAlloc();
/** Copies a keymap object. */
int imapKeymapCopy(imapKeymapRef, const imapKeymapRef);

/** Fress the keymap object. */
void imapKeymapFree(imapKeymapRef);

/** 
*	Sets (or adds if not already present) the keymap for the specified function. 
* 	idx is the zero-based index of the keymap associated with this function. Since axis
*	functions  can have different types of keymaps, this index is per keymapType.
*	The special idx value IMAP_NO_EDIT is used to specify that the application should not
*	modify any keymaps with indices above this value (determined by the EDIT tag in the CFML file).
*	keymapType specifies the type of keymap
*	(IMAP_BUTTON_KEYMAP, IMAP_AXIS_KEYMAP, IMAP_AXIS_REL_KEYMAP, IMAP_AXIS_ABS_KEYMAP). 
*	And finally keymap is a pointer to a keymap struct
*	(imap_button_keymap, imap_axis_keymap, imap_axis_rel_keymap, imap_axis_abs_keymap).
*/
int imapKeymapSet(imapKeymapRef, unsigned int functionCode, unsigned int idx, const imap_keymap* keymap);

/**
*	Gets the keymap for the specified function.
*/
int imapKeymapGet(const imapKeymapRef, unsigned int functionCode, unsigned int idx,  int keymapType, imap_keymap* keymap);

/** Adds a new keymap and sets the idx to some value >= passed in idx by finding first unused index of the keymap. */
int imapKeymapAdd(imapKeymapRef, unsigned int functionCode, unsigned int* idx, const imap_keymap* keymap);

/** Removes a keymap from the specified function */
int imapKeymapRemove(imapKeymapRef, unsigned int functionCode, unsigned int idx, int keymapType);

/** Removes all keymaps */
void imapKeymapRemoveAll(imapKeymapRef);

/** Saves the keymap in CFML format. userdata is used by the imapWriteCallback function.*/
int imapKeymapSave(const imapKeymapRef, void* userdata);

/** Reads the keymap in CFML format. userdata is used by the imapReadCallback function.*/
int imapKeymapLoad(imapKeymapRef, void* userdata);

/** Debugging function to dump contents of keymap object to stderr. */
void imapKeymapDump(const imapKeymapRef);

/** Debugging function to dump contents of imap object to stderr. */
void imapDump();

/** Gets keymap for function */
int imapGetKeymap(unsigned int functionCode, int index, const imap_keymap** keymap);

/** Enable/disable specific function */
void imapEnableFunction(unsigned int functionCode, int enable);

/** Enable/Disable all button/axis functions */
void imapEnableAllFunctions(int enableKeyFunctions, int enableAxisFunctions);

#ifdef __cplusplus
}
#endif

#endif // _IMAP_H