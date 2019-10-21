#ifndef _CFML_H
#define _CFML_H

#include <libxml/parser.h>
#include <libxml/tree.h>

/************************************************************************************************
	imapcfml
*************************************************************************************************/

#define CFML_CUR_VERSION 			"1.1.0"

#define CFML_CONTROLSET_TAG 		"controlset"
#define CFML_CONTROLSET_NAME 		"name"
#define CFML_CONTROLSET_VERSION		"version"
#define CFML_CONTROLSET_LANGUAGE 	"language"
#define CFML_CONTROLSET_KEYBOARD	"keyboard"

#define CFML_EDIT					"edit"
#define CFML_STICKSET				"stickset"
#define CFML_JOYNUMBER				"stick"
#define CFML_HALFAXIS				"halfaxis"
#define CFML_INVERT					"invert"
#define CFML_VALUE					"value"
#define CFML_ONRELEASE				"onrelease"
#define CFML_INDEX					"index"
#define CFML_PER					"per"
#define CFML_KEYPRESS				"keypress"
#define CFML_SEC					"sec"
#define CFML_FUNCTION				"function"
#define CFML_EMULATEJOYSTICK		"emulatejoystick"

#define CFML_CONTROL_TAG 			"control"
#define CFML_FUNCTION_TAG 			"function"
#define CFML_COMBO_TAG				"combo"
#define CFML_KEY_TAG				"key"
#define CFML_MOUSEBUTTON_TAG		"mousebutton"
#define CFML_JOYBUTTON_TAG			"joybutton"
#define CFML_JOYAXIS_TAG			"joyaxis"
#define CFML_MOUSEAXIS_TAG			"mouseaxis"
#define CFML_KEYDELTA_TAG			"keydelta"
#define CFML_KEYABS_TAG				"keyabsolute"

#define CFML_US_KEYBOARD			"us"
#define CFML_IAC_KEYBOARD			"iac"

#define CFML_ENGLISH_LANGUAGE		"english"
#define CFML_CODE_IMAP				"imap"

/************************************************************************************************
	imapcfml
*************************************************************************************************/

struct cfml_writer;
struct cfml_parser;

typedef struct cfml_writer* cfmlWriterRef;
typedef struct cfml_parser* cfmlParserRef;

typedef int (*cfmlWriteCallback)(void* userdata, int numbytes, const void* buf);
typedef int (*cfmlReadCallback)(void* userdata, int numbytes, void* buf);

#ifdef __cplusplus
extern "C" {
#endif

// write
cfmlWriterRef cfmlWriteAlloc(void* userdata, cfmlWriteCallback);
void cfmlWriteFree(cfmlWriterRef);

void cfmlWriteXMLHeaders(cfmlWriterRef);

void cfmlWriteControlSetBegin(cfmlWriterRef, const char* name,
								const char* language, const char* keyboard);
void cfmlWriteControlSetEnd(cfmlWriterRef);

void cfmlWriteControlBegin(cfmlWriterRef, const char* functionName);
void cfmlWriteControlEnd(cfmlWriterRef);

void cfmlWriteComboBegin(cfmlWriterRef, int edit, int idx);
void cfmlWriteComboEnd(cfmlWriterRef);

void cfmlWriteKey(cfmlWriterRef, int edit,  int idx, const char* key);
void cfmlWriteJoyButton(cfmlWriterRef, int edit,  int idx, int joynum, const char* button);
void cfmlWriteMouseButton(cfmlWriterRef, int edit,  int idx, const char* button);

void cfmlWriteKeyDeltaBegin(cfmlWriterRef, int edit,  int idx, float value, const char* type);
void cfmlWriteKeyDeltaEnd(cfmlWriterRef);

void cfmlWriteKeyAbsoluteBegin(cfmlWriterRef, int edit,  int idx,
								float value, int onRelease, float releaseValue);
void cfmlWriteKeyAbsoluteEnd(cfmlWriterRef);

void cfmlWriteJoyAxis(cfmlWriterRef, int edit,  int idx, int joynum, int halfaxis,
						int invert, const char* axis);
void cfmlWriteMouseAxis(cfmlWriterRef, int edit,  int idx, int halfaxis,
						int invert, const char* axis, int emulatejoystick);

// read
cfmlParserRef cfmlNewParser(void* userdata, cfmlReadCallback, startElementSAXFunc, endElementSAXFunc, charactersSAXFunc);
void cfmlParserFree(cfmlParserRef);

int cfmlParse(cfmlParserRef);

#ifdef __cplusplus
}
#endif

#endif // _CFML_H
