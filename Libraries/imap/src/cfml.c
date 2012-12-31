#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cfml.h"

/************************************************************************************************
	imapcfml
*************************************************************************************************/

#define CFML_BUFFER_SIZE 1024

struct cfml_writer
{
	char buf[CFML_BUFFER_SIZE];
	void* userdata;
	cfmlWriteCallback write;
	int tabs;
};

struct cfml_parser
{
	char buf[CFML_BUFFER_SIZE];
	void* userdata;
	cfmlReadCallback read;
	XML_Parser xmlparser;
};

static void cfmlWriteCommonFields(cfmlWriterRef ref, int edit,  int idx, int* len);
static void cfmlWriteTabs(cfmlWriterRef ref);
static void bit_to_str(char* bitstr, unsigned int stickSetMask);

#ifdef WIN32
#define snprintf _snprintf
#endif

/************************************************************************************************
	cfmlWriteAlloc
*************************************************************************************************/

cfmlWriterRef cfmlWriteAlloc(void* userdata, cfmlWriteCallback write)
{
	cfmlWriterRef ref = (cfmlWriterRef)calloc(1, sizeof(cfml_writer));
	
	if(ref)
	{
		ref->userdata = userdata;
		ref->write = write;
	}
	
	return ref;
}

/************************************************************************************************
	cfmlWriteFree
*************************************************************************************************/
	
void cfmlWriteFree(cfmlWriterRef ref)
{
	if(ref)
	{
		free(ref);
	}
}

/************************************************************************************************
	cfmlWriteXMLHeaders
*************************************************************************************************/

void cfmlWriteXMLHeaders(cfmlWriterRef ref)
{
	int len = 0;
	
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len, "<?xml version='1.0'?>\n");
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len, "<!DOCTYPE controlset SYSTEM \"cfml.dtd\">\n");
	
	ref->write(ref->userdata, len, ref->buf);
}

/************************************************************************************************
	cfmlWriteControlSetBegin
*************************************************************************************************/

void cfmlWriteControlSetBegin(cfmlWriterRef ref, const char* name, 
								const char* language, const char* keyboard)
{
	int len = 0;
	
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len, "<%s", CFML_CONTROLSET_TAG);
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len, " %s=\"%s\"", CFML_CONTROLSET_VERSION, CFML_CUR_VERSION);
	
	if(name)
		len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len, " %s=\"%s\"", CFML_CONTROLSET_NAME, name);

	if(language)
		len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len, " %s=\"%s\"", CFML_CONTROLSET_LANGUAGE, language);

	if(keyboard)
		len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len, " %s=\"%s\"", CFML_CONTROLSET_KEYBOARD, keyboard);

	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len, ">\n");
	
	ref->write(ref->userdata, len, ref->buf);

	ref->tabs++;
}

/************************************************************************************************
	cfmlWriteControlSetEnd
*************************************************************************************************/

void cfmlWriteControlSetEnd(cfmlWriterRef ref)
{
	int len = 0;

	ref->tabs--;
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,"</%s>\n", CFML_CONTROLSET_TAG);
	
	ref->write(ref->userdata, len , ref->buf);
}

/************************************************************************************************
	cfmlWriteControlSetEnd
*************************************************************************************************/

void cfmlWriteControlBegin(cfmlWriterRef ref, const char* functionName)
{
	int len = 0;
	
	cfmlWriteTabs(ref);
	
	len += snprintf(ref->buf + len , CFML_BUFFER_SIZE - len,"<%s %s=\"%s\">\n", CFML_CONTROL_TAG, CFML_FUNCTION, functionName);
	
	ref->write(ref->userdata, len , ref->buf);

	ref->tabs++;
}

/************************************************************************************************
	cfmlWriteControlSetEnd
*************************************************************************************************/

void cfmlWriteControlEnd(cfmlWriterRef ref)
{
	int len = 0;
	
	ref->tabs--;
	cfmlWriteTabs(ref);
	
	len += snprintf(ref->buf + len , CFML_BUFFER_SIZE - len,"</%s>\n", CFML_CONTROL_TAG);
	
	ref->write(ref->userdata, len , ref->buf);
}

/************************************************************************************************
	cfmlWriteComboBegin
*************************************************************************************************/

void cfmlWriteComboBegin(cfmlWriterRef ref, int edit,  int idx)
{
	int len = 0;
	
	cfmlWriteTabs(ref);
	
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,"<%s", CFML_COMBO_TAG);
	cfmlWriteCommonFields(ref, edit,  idx, &len);
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,">\n");
	
	ref->write(ref->userdata, len , ref->buf);
	
	ref->tabs++;
}

/************************************************************************************************
	cfmlWriteComboEnd
*************************************************************************************************/

void cfmlWriteComboEnd(cfmlWriterRef ref)
{
	int len = 0;
		
	ref->tabs--;
	cfmlWriteTabs(ref);

	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,"</%s>\n", CFML_COMBO_TAG);
	
	ref->write(ref->userdata, len , ref->buf);
}

/************************************************************************************************
	cfmlWriteKey
*************************************************************************************************/

void cfmlWriteKey(cfmlWriterRef ref, int edit,  int idx, const char* key)
{
	int len = 0;
		
	cfmlWriteTabs(ref);

	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,"<%s", CFML_KEY_TAG);
	cfmlWriteCommonFields(ref, edit,  idx, &len);
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,">%s</%s>\n", key, CFML_KEY_TAG);
	
	ref->write(ref->userdata, len , ref->buf);
}

/************************************************************************************************
	cfmlWriteJoyButton
*************************************************************************************************/

void cfmlWriteJoyButton(cfmlWriterRef ref, int edit,  int idx, int joynum, const char* button)
{
	int len = 0;
		
	cfmlWriteTabs(ref);

	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,"<%s %s=\"%d\"", CFML_JOYBUTTON_TAG, CFML_JOYNUMBER, joynum);
	cfmlWriteCommonFields(ref, edit,  idx, &len);
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,">%s</%s>\n", button, CFML_JOYBUTTON_TAG);
	
	ref->write(ref->userdata, len , ref->buf);
}

/************************************************************************************************
	cfmlWriteMouseButton
*************************************************************************************************/

void cfmlWriteMouseButton(cfmlWriterRef ref, int edit,  int idx, const char* button)
{
	int len = 0;
		
	cfmlWriteTabs(ref);

	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,"<%s", CFML_MOUSEBUTTON_TAG);
	cfmlWriteCommonFields(ref, edit,  idx, &len);
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,">%s</%s>\n", button, CFML_MOUSEBUTTON_TAG);
	
	ref->write(ref->userdata, len , ref->buf);
}

/************************************************************************************************
	cfmlWriteKeyDeltaBegin
*************************************************************************************************/

void cfmlWriteKeyDeltaBegin(cfmlWriterRef ref, int edit,  int idx, float value, const char* type)
{
	int len = 0;
	
	cfmlWriteTabs(ref);

	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,"<%s %s=\"%.2f\" %s=\"%s\"", 
									CFML_KEYDELTA_TAG, CFML_VALUE, value, CFML_PER, type);
	cfmlWriteCommonFields(ref, edit,  idx, &len);	
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,">\n");

	ref->write(ref->userdata, len , ref->buf);
	ref->tabs++;
}

/************************************************************************************************
	cfmlWriteKeyDeltaEnd
*************************************************************************************************/

void cfmlWriteKeyDeltaEnd(cfmlWriterRef ref)
{
	int len = 0;
	
	ref->tabs--;
	cfmlWriteTabs(ref);

	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,"</%s>\n", CFML_KEYDELTA_TAG);
	
	ref->write(ref->userdata, len , ref->buf);
}

/************************************************************************************************
	cfmlWriteKeyAbsoluteBegin
*************************************************************************************************/

void cfmlWriteKeyAbsoluteBegin(cfmlWriterRef ref, int edit,  int idx, float value, int onRelease, float releaseValue)
{
	int len = 0;
	
	cfmlWriteTabs(ref);

	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,"<%s %s=\"%.2f\"", CFML_KEYABS_TAG, CFML_VALUE, value);

	if(onRelease)
		len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len," %s=\"%.2f\"", CFML_ONRELEASE, releaseValue);

	cfmlWriteCommonFields(ref, edit,  idx, &len);
	
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len, ">\n");

	
	ref->write(ref->userdata, len , ref->buf);

	ref->tabs++;
}


/************************************************************************************************
	cfmlWriteKeyAbsoluteEnd
*************************************************************************************************/

void cfmlWriteKeyAbsoluteEnd(cfmlWriterRef ref)
{
	int len = 0;
	
	ref->tabs--;
	cfmlWriteTabs(ref);

	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,"</%s>\n", CFML_KEYABS_TAG);
	
	ref->write(ref->userdata, len , ref->buf);
}

/************************************************************************************************
	cfmlWriteJoyAxis
*************************************************************************************************/

void cfmlWriteJoyAxis(cfmlWriterRef ref, int edit,  int idx, int joynum, int halfaxis,
						int invert, const char* axis)
{
	int len = 0;
		
	cfmlWriteTabs(ref);

	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,"<%s %s=\"%d\"", CFML_JOYAXIS_TAG, CFML_JOYNUMBER, joynum);
	cfmlWriteCommonFields(ref, edit,  idx, &len);

	if(halfaxis)
		len += snprintf(ref->buf + len,CFML_BUFFER_SIZE - len," %s=\"%s\"", CFML_HALFAXIS, (halfaxis > 0) ? "+" : "-");

	if(invert)
		len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len," %s=\"%d\"", CFML_INVERT, invert);
	
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,">%s</%s>\n", axis, CFML_JOYAXIS_TAG);

	ref->write(ref->userdata, len , ref->buf);
}

/************************************************************************************************
	cfmlWriteMouseAxis
*************************************************************************************************/

void cfmlWriteMouseAxis(cfmlWriterRef ref, int edit,  int idx, int halfaxis,
						int invert, const char* axis, int emulatejoystick)
{
	int len = 0;
		
	cfmlWriteTabs(ref);

	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,"<%s", CFML_MOUSEAXIS_TAG);
	cfmlWriteCommonFields(ref, edit,  idx, &len);
	
	if(halfaxis)
		len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len," %s=\"%s\"", CFML_HALFAXIS, (halfaxis > 0) ? "+" : "-");

	if(invert)
		len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len," %s=\"%d\"", CFML_INVERT, invert);

	if(emulatejoystick)
		len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len," %s=\"%d\"", CFML_EMULATEJOYSTICK, emulatejoystick);
	

	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len,">%s</%s>\n", axis, CFML_MOUSEAXIS_TAG);

	ref->write(ref->userdata, len , ref->buf);
}

/************************************************************************************************
	cfmlNewParser
*************************************************************************************************/

cfmlParserRef cfmlNewParser(void* userdata, cfmlReadCallback read , XML_StartElementHandler start,
							XML_EndElementHandler end, XML_CharacterDataHandler data)
{
 	cfmlParserRef ref = (cfmlParserRef)calloc(1, sizeof(cfml_parser));
 	
 	if(ref)
 	{
	 	ref->userdata = userdata;
 		ref->read = read;
	 	ref->xmlparser = XML_ParserCreate(NULL);

 		XML_SetElementHandler(ref->xmlparser, start, end);
		XML_SetCharacterDataHandler(ref->xmlparser, data);
		XML_SetUserData(ref->xmlparser, userdata);
	}
	
	return ref;
}

/************************************************************************************************
	cfmlParserFree
*************************************************************************************************/

void cfmlParserFree(cfmlParserRef ref)
{
	if(ref)
	{
		XML_ParserFree(ref->xmlparser);
		free(ref);
	}
}

/************************************************************************************************
	cfmlParse
*************************************************************************************************/

int cfmlParse(cfmlParserRef ref)
{
	for(;;)
	{
		int len = ref->read(ref->userdata, CFML_BUFFER_SIZE, ref->buf);
		int isFinal = (len != CFML_BUFFER_SIZE);
		
		XML_Parse(ref->xmlparser, ref->buf, len, isFinal);
		
		if(isFinal) break;
	}
	return 0;
}

/************************************************************************************************
	cfmlWriteCommonFields
*************************************************************************************************/

static void cfmlWriteCommonFields(cfmlWriterRef ref, int edit,  int idx, int* len)
{
	if(!edit)
		*len += snprintf(ref->buf + *len , CFML_BUFFER_SIZE - *len," %s=\"0\"", CFML_EDIT);
	
	if(idx > 0)
		*len += snprintf(ref->buf + *len , CFML_BUFFER_SIZE - *len," %s=\"%d\"", CFML_INDEX, idx);
}

/************************************************************************************************
	cfmlWriteTabs
*************************************************************************************************/

static void cfmlWriteTabs(cfmlWriterRef ref)
{
	char tab[64];
	int i, len = 0;
	
	for(i = 0; i < ref->tabs; i++)
		tab[i] = '\t';
	
	tab[i] = '\0';
	
	len += snprintf(ref->buf + len, CFML_BUFFER_SIZE - len, "%s", tab);
	
	ref->write(ref->userdata, len , ref->buf);
	
	
}

/************************************************************************************************
	bit_to_str
*************************************************************************************************/

static void bit_to_str(char* bitstr, unsigned int stickSetMask)
{
	int i, bit, maxbit = 0;
			
	for(i = 0; i < 32; i++)
	{
		bit = (stickSetMask >> i) & 0x1;
		if(bit) maxbit = i;
		bitstr[i] = (bit) ? '1' : '0';
	}
	
	bitstr[maxbit+1] = '\0';
}