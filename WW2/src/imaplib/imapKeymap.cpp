#include "syscfg.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "imapInternals.h"
#include "cfml.h"

/************************************************************************************************
	imapKeymap
*************************************************************************************************/

extern imap inputMap;

static void imapKeymapAddEntry(imapKeymapRef ref, imap_keymap_entry* entry);

static void cfmlStartElementHandler(void *userdata, const xmlChar *name, const xmlChar **atts);
static void cfmlEndElementHandler(void *userdata, const xmlChar *name);
static void cfmlDataHandler(void *userdata, const xmlChar *s, int len);

static void cfmlWriteButtonEntry(cfmlWriterRef cfmlRef, imap_keymap_entry* entry);
static void cfmlWriteAxisEntry(cfmlWriterRef cfmlRef, imap_keymap_entry* entry);
static void cfmlWriteAxisRelEntry(cfmlWriterRef cfmlRef, imap_keymap_entry* entry);
static void cfmlWriteAxisAbsEntry(cfmlWriterRef cfmlRef, imap_keymap_entry* entry);

static int keymap_read(void* userdata, int numbytes, void* buf);
static unsigned int str_to_bit(const char* bitstr);
static int istrcmp(const char* str1, const char* str2);

/************************************************************************************************
	imapKeymapAlloc
*************************************************************************************************/

imapKeymapRef imapKeymapAlloc()
{
	imapKeymapRef ref = (imapKeymapRef)calloc(1,sizeof(imap_keymap_list));

	return ref;
}

/************************************************************************************************
	imapKeymapCopy
*************************************************************************************************/

int imapKeymapCopy(imapKeymapRef dst, const imapKeymapRef src)
{
	imap_keymap_entry* entry = src->keymapEntries;
	imapKeymapRemoveAll(dst);

	while(entry)
	{
		int err = imapKeymapSet(dst, entry->functionCode, entry->idx,&entry->keymap);
			
		if(err != IMAP_NO_ERR) return err;
					
		entry = entry->next;
	}
	
	return 0;
}

/************************************************************************************************
	imapKeymapFree
*************************************************************************************************/

void imapKeymapFree(imapKeymapRef ref)
{
	imapKeymapRemoveAll(ref);
	free(ref);
}

/************************************************************************************************
	imapKeymapSet
*************************************************************************************************/

int imapKeymapSet(imapKeymapRef ref, unsigned int functionCode, unsigned int idx, const imap_keymap* keymap)
{
	imap_keymap_entry* entry = (imap_keymap_entry*)calloc(1, sizeof(imap_keymap_entry));
	if(entry == NULL) return IMAP_MEMORY_ERR;
	
	entry->functionCode = functionCode;
	entry->idx = idx;
	entry->keymap = *keymap;

	imapKeymapRemove(ref, functionCode, idx, entry->keymap.keymapType);
	imapKeymapAddEntry(ref, entry);
	
	return IMAP_NO_ERR;
}

/************************************************************************************************
	imapKeymapGet
*************************************************************************************************/

int imapKeymapGet(const imapKeymapRef ref, unsigned int functionCode, unsigned int idx,  int keymapType, imap_keymap* keymap)
{
	imap_keymap_entry* entry;
		
	entry = ref->keymapEntries;
	
	while(entry)
	{	
		if(entry->functionCode == functionCode && entry->idx == idx && entry->keymap.keymapType == keymapType)
		{
			if(keymap)
				*keymap = entry->keymap;
			
			return IMAP_NO_ERR;
		}
		entry = entry->next;
	}
		
	return IMAP_KEYMAP_NOT_FOUND_ERR;
}

/************************************************************************************************
	imapKeymapAdd
*************************************************************************************************/

int imapKeymapAdd(imapKeymapRef ref, unsigned int functionCode, unsigned int* idx, const imap_keymap* keymap)
{
	imap_keymap_entry* entry;
		
	entry = ref->keymapEntries;
	
	while(entry)
	{	
		if(entry->functionCode > functionCode) break;

		if(entry->functionCode == functionCode && entry->keymap.keymapType == keymap->keymapType)
		{
			if(entry->idx == *idx) (*idx)++;
		}
		
		entry = entry->next;
	}
	
	return imapKeymapSet(ref, functionCode, *idx, keymap);
}


/************************************************************************************************
	imapKeymapRemove
*************************************************************************************************/

int imapKeymapRemove(imapKeymapRef ref, unsigned int functionCode, unsigned int idx,  int keymapType)
{
	imap_keymap_entry* entry;
	imap_keymap_entry* remove;
		
	entry = ref->keymapEntries;
	
	while(entry)
	{	
		if(entry->functionCode == functionCode && entry->idx == idx && entry->keymap.keymapType == keymapType)
		{
			if(entry == ref->keymapEntries)
				ref->keymapEntries = entry->next;
			if(entry->prev)
				entry->prev->next = entry->next;
			if(entry->next)
				entry->next->prev = entry->prev;
			
			remove = entry;
			entry = entry->next;
			
			free(remove);
					
			//break; // let it run because some default keymaps have multiple entries for the same function
		}
		else
		{
			entry = entry->next;
		}
	}
	
	return IMAP_NO_ERR;
}

/************************************************************************************************
	imapKeymapRemoveAll
*************************************************************************************************/

void imapKeymapRemoveAll(imapKeymapRef ref)
{
	imap_keymap_entry* entry = ref->keymapEntries;
	imap_keymap_entry* next;
	
	while(entry)
	{
		next = entry->next;
		free(entry);		
		entry = next;
	}
	ref->keymapEntries = NULL;
}

/************************************************************************************************
	imapKeymapSave
*************************************************************************************************/

int imapKeymapSave(const imapKeymapRef ref, void* userdata)
{	
	imap_keymap_entry* entry;
	imap_function_info funcInfo;
	cfmlWriterRef cfmlRef = cfmlWriteAlloc(userdata, inputMap.write);
								
	if(cfmlRef == NULL) return IMAP_MEMORY_ERR;
	
	cfmlWriteXMLHeaders(cfmlRef);
	cfmlWriteControlSetBegin(cfmlRef, NULL, CFML_ENGLISH_LANGUAGE, CFML_US_KEYBOARD);
	
	entry = ref->keymapEntries;

	while(entry)
	{
		int edit = (entry->idx < IMAP_NOEDIT_ID); // by convention
		char functionName[MAX_PARSE_STR_LEN+1];
		int sameFunction = entry->prev && (entry->prev->functionCode == entry->functionCode);
		
		if(!sameFunction)
		{
			// This is to keep entries with the same function code in the same control tag.
			// Although this is not necessary, it makes the CFML somewhat "cleaner".
			// This works because the entries are sorted by function code.
			if(entry->prev) cfmlWriteControlEnd(cfmlRef);

			inputMap.functionInfo(IMAP_HAVE_CODE,  CFML_ENGLISH_LANGUAGE, &entry->functionCode, 
									functionName, MAX_PARSE_STR_LEN, &funcInfo);
			cfmlWriteControlBegin(cfmlRef, functionName);
		}
		
		switch(entry->keymap.keymapType)
		{
			case IMAP_BUTTON_KEYMAP:
				cfmlWriteButtonEntry(cfmlRef, entry);
			break;
			
			case IMAP_AXIS_KEYMAP:
				cfmlWriteAxisEntry(cfmlRef, entry);
			break;
			
			case IMAP_AXIS_REL_KEYMAP:
				cfmlWriteAxisRelEntry(cfmlRef, entry);
			break;
			
			case IMAP_AXIS_ABS_KEYMAP:
				cfmlWriteAxisAbsEntry(cfmlRef, entry);
			break;
		}
				
		entry = entry->next;
	}
	
	if(ref->keymapEntries) cfmlWriteControlEnd(cfmlRef);
	
	cfmlWriteControlSetEnd(cfmlRef);
	cfmlWriteFree(cfmlRef);
	
	return IMAP_NO_ERR;
}

/************************************************************************************************
	imapKeymapLoad
*************************************************************************************************/

int imapKeymapLoad(imapKeymapRef ref, void* userdata)
{	
	cfmlParserRef cfmlParser;
	parser_data parseData;

	imapKeymapRemoveAll(ref);
	
	memset(&parseData, 0x0, sizeof(parseData));
	parseData.userdata = userdata;
	parseData.ref = ref;
	
	cfmlParser = cfmlNewParser(&parseData, keymap_read, cfmlStartElementHandler,
											cfmlEndElementHandler, cfmlDataHandler);
	cfmlParse(cfmlParser);
	cfmlParserFree(cfmlParser);
	
	return IMAP_NO_ERR;
}

/************************************************************************************************
	imapKeymapDump
*************************************************************************************************/

void imapKeymapDump(const imapKeymapRef ref)
{
	imap_keymap_entry* entry = ref->keymapEntries;
	int i;
	char name[MAX_PARSE_STR_LEN+1];
	imap_function_info funcInfo;
	imap_iac_info iacInfo;
	
	printf( "KeymapDump\n{\n");

	while(entry)
	{
		inputMap.functionInfo(IMAP_HAVE_CODE,  CFML_ENGLISH_LANGUAGE, &entry->functionCode, name, MAX_PARSE_STR_LEN, &funcInfo);
		printf( "\tfunctionCode: 0x%x, \"%s\"\n", entry->functionCode, name);
		printf( "\tid: 0x%x\n", entry->idx);
		printf( "\tkeymapType: %d\n", entry->keymap.keymapType);
		printf( "\t{\n");
		
		switch(entry->keymap.keymapType)
		{
			case IMAP_AXIS_ABS_KEYMAP:
				printf( "\t\tpercent: %f, onrelease: %d, releasePercent: %f\n", 
						entry->keymap.axisAbs.percent, entry->keymap.axisAbs.onRelease, entry->keymap.axisAbs.releasePercent);
			break;
			
			case IMAP_AXIS_REL_KEYMAP:
				printf( "\t\ttype: %d, rate: %f\n", entry->keymap.axisRel.axisMapType, entry->keymap.axisRel.rate);
			break;
		}
		
		for(i = 0; i < entry->keymap.numKeys; i++)
		{
			switch(entry->keymap.keymapType)
			{
				case IMAP_BUTTON_KEYMAP:
				{
					inputMap.iacInfo(IMAP_HAVE_CODE, CFML_ENGLISH_LANGUAGE, CFML_US_KEYBOARD, &entry->keymap.btn.iac[i], name, MAX_PARSE_STR_LEN, 
										&iacInfo);
					printf( "\t\tiac: 0x%x, \"%s\"\n", entry->keymap.btn.iac[i], name);
				}
				break;
		
				case IMAP_AXIS_KEYMAP:
				{
					inputMap.iacInfo(IMAP_HAVE_CODE, CFML_ENGLISH_LANGUAGE, CFML_US_KEYBOARD, &entry->keymap.axis.iac, name, MAX_PARSE_STR_LEN, 
										&iacInfo);
					printf( "\t\tiac: 0x%x, \"%s\", axisRange=%d invert=%d emulateJoystick=%d\n", entry->keymap.axis.iac, name,
							entry->keymap.axis.range, entry->keymap.axis.invert, entry->keymap.axis.emulateJoystick);
				}	
				break;
			
				case IMAP_AXIS_REL_KEYMAP:
					inputMap.iacInfo(IMAP_HAVE_CODE, CFML_ENGLISH_LANGUAGE, CFML_US_KEYBOARD, &entry->keymap.axisRel.iac[i], name, MAX_PARSE_STR_LEN, 
										&iacInfo);
					printf( "\t\tiac: 0x%x, \"%s\"\n", entry->keymap.axisRel.iac[i], name);					
				
				break;
			
				case IMAP_AXIS_ABS_KEYMAP:
					inputMap.iacInfo(IMAP_HAVE_CODE, CFML_ENGLISH_LANGUAGE, CFML_US_KEYBOARD, &entry->keymap.axisAbs.iac[i], name, MAX_PARSE_STR_LEN, 
										&iacInfo);
					printf( "\t\tiac: 0x%x, \"%s\"\n", entry->keymap.axisAbs.iac[i], name);					
				break;
			}
		}
		printf( "\t}\n");
		entry = entry->next;	
	}
	printf( "}\n");
}

/************************************************************************************************
	imapKeymapAddEntry
*************************************************************************************************/
// Adds entry to keymap. Keeps entries sorted by function code, keymap type, and idx.

static void imapKeymapAddEntry(imapKeymapRef ref, imap_keymap_entry* entry)
{
	imap_keymap_entry* head = ref->keymapEntries;
	imap_keymap_entry* prev = NULL;
	imap_keymap_entry* cur  = head;
	
	while(cur) // Multi-field sorted insert, ahh!
	{
		if(cur->functionCode > entry->functionCode ) break;
		
		if(cur->functionCode == entry->functionCode)
		{
			while(cur && cur->functionCode == entry->functionCode)
			{
				if(cur->keymap.keymapType > entry->keymap.keymapType) break;
				
				if(cur->keymap.keymapType == entry->keymap.keymapType)
				{
					while(cur && cur->functionCode == entry->functionCode && cur->keymap.keymapType == entry->keymap.keymapType)
					{
						if(cur->idx >= entry->idx) break;
						
						prev = cur;
						cur = cur->next;
					}				
					break;
				}	
				prev = cur;
				cur = cur->next;
			}
			break;
		}		
		prev = cur;
		cur = cur->next;
	}
	
	if(prev) prev->next = entry;
	else head = entry;
	
	if(cur) cur->prev = entry;
	
	entry->prev = prev;
	entry->next = cur;
	
	ref->keymapEntries = head;
}


/************************************************************************************************
	cfmlStartElementHandler
*************************************************************************************************/

static void cfmlStartElementHandler(void *userdata, const xmlChar *name_, const xmlChar **atts)
{
	parser_data* parserData = (parser_data*)userdata;
	const xmlChar** pair = atts;
    const char *name = reinterpret_cast<const char *>(name_);
	const char *key, *value;
	imap_keymap_entry* entry;
	parser_entry_data* entryData;
	int idx, elementIdx, cfmlIdx = 0;
	int halfaxis = 0, emulateJoystick = 0, noedit = 0, deviceIdx = 1, invert = 0, onRelease = 0;
	float keyAxisValue = 0.0f, releasePercent = 0.0f;
	char language[MAX_PARSE_STR_LEN+1], keyboard[MAX_PARSE_STR_LEN+1], per[MAX_PARSE_STR_LEN+1];
	
	language[0] = keyboard[0] = per[0] = parserData->data[0] = '\0';

	// tag attributes
	while(pair && *pair)
	{
		key = reinterpret_cast<const char *>(*pair++);
		value = reinterpret_cast<const char *>(*pair++);
	 	
		if(!istrcmp(key, CFML_CONTROLSET_LANGUAGE)) 		// LANGUAGE
		{
			strncpy(language, value, MAX_PARSE_STR_LEN);
		}else if(!istrcmp(key, CFML_CONTROLSET_KEYBOARD)) 	// KEYBOARD
		{	
			strncpy(keyboard, value, MAX_PARSE_STR_LEN);			
		}else if(!istrcmp(key, CFML_EDIT)) 					// EDIT
		{
			if(atoi(value) == 0) noedit = 1;
		}else if(!istrcmp(key, CFML_JOYNUMBER)) 			// STICK
		{
			deviceIdx = atoi(value);
		}else if(!istrcmp(key, CFML_HALFAXIS)) 				// HALFAXIS
		{
			if(value[0] == '+') halfaxis = 1;
			else if(value[0] == '-') halfaxis = -1;
		}else if(!istrcmp(key, CFML_INVERT)) 				// INVERT
		{
			invert = atoi(value);
		}else if(!istrcmp(key, CFML_ONRELEASE))				// ONRELEASE
		{
			onRelease = 1;
			releasePercent = (float)atoi(value) / 100.0f;			
		}else if(!istrcmp(key, CFML_VALUE))		 			// VALUE
		{
			keyAxisValue = (float)atoi(value) / 100.0f;
		}else if(!istrcmp(key, CFML_INDEX))					// INDEX
		{
			cfmlIdx = atoi(value);
		}else if(!istrcmp(key, CFML_PER))					// PER
		{
			strncpy(per, value, MAX_PARSE_STR_LEN);			// FUNCTION
		}else if(!istrcmp(key, CFML_FUNCTION))
		{
			imap_function_info funcInfo;
			// get function code from function name
			inputMap.functionInfo(IMAP_HAVE_NAME,  parserData->language, &parserData->functionCode, 
									(char*)value, MAX_PARSE_STR_LEN, &funcInfo); 
		}else if(!istrcmp(key, CFML_EMULATEJOYSTICK))
		{
			emulateJoystick = atoi(value);
		}
	}
	
	// <CONTROLSET>
	if(!istrcmp(name, CFML_CONTROLSET_TAG))
	{
		strncpy(parserData->language, language, MAX_PARSE_STR_LEN);
		strncpy(parserData->keyboard, keyboard, MAX_PARSE_STR_LEN);
	}
	//  <COMBO> 
	else if(!istrcmp(name, CFML_COMBO_TAG))
	{
		// increment the entry count and set the combo flag
		if(parserData->numEntries < MAX_PARSE_ENTRIES)
		{
			idx = parserData->numEntries++;
			entryData 	= &parserData->entryData[idx];
			
			entryData->combo = 1;	
			entryData->noedit = noedit;
			entryData->idx = cfmlIdx;
		}
	}
	// <KEY> <JOYBUTTON> <MOUSEBUTTON>
	else if(!istrcmp(name, CFML_KEY_TAG) 		||
	   		!istrcmp(name, CFML_JOYBUTTON_TAG) 	|| !istrcmp(name, CFML_MOUSEBUTTON_TAG))
	{
		idx = (parserData->numEntries > 0) ? parserData->numEntries - 1 : 0;

		// if it's not a combo  we need to increment the entry count
		if(!parserData->entryData[idx].combo)
		{
			if(parserData->numEntries < MAX_PARSE_ENTRIES)
				idx = parserData->numEntries++;
		}
		
		entry 		= &parserData->keymapEntries[idx];
		entryData 	= &parserData->entryData[idx];
		
		if(entryData->combo)
		{
			noedit = entryData->noedit;
			cfmlIdx = entryData->idx;
		}
		
		if(entryData->keyAxisType)
		{
			entry->keymap.keymapType = entryData->keyAxisType;
			if(entry->keymap.keymapType == IMAP_AXIS_ABS_KEYMAP)
			{
				entry->keymap.axisAbs.percent = entryData->keyAxisValue;
				entry->keymap.axisAbs.onRelease = entryData->onRelease;
				entry->keymap.axisAbs.releasePercent = entryData->releasePercent;
			}else if(entry->keymap.keymapType == IMAP_AXIS_REL_KEYMAP)
			{
				entry->keymap.axisRel.axisMapType = entryData->keyDeltaType;
				entry->keymap.axisRel.rate = entryData->keyAxisValue;
			}
		}else
		{
			entry->keymap.keymapType 		= IMAP_BUTTON_KEYMAP;
		}
		
		entry->idx 				= noedit ?  IMAP_NOEDIT_ID : cfmlIdx;
		
		if(entry->keymap.numKeys < MAX_PARSE_ELEMENTS)
		{
			elementIdx = entry->keymap.numKeys++;
			
			entryData->deviceIdx[elementIdx] = deviceIdx;
			
			if(!istrcmp(name, CFML_KEY_TAG))
				entryData->deviceType[elementIdx] = IMAP_IAC_IS_KEYBOARD;
			else if(!istrcmp(name, CFML_JOYBUTTON_TAG))
				entryData->deviceType[elementIdx] = IMAP_IAC_IS_JOYSTICK;
			else 
				entryData->deviceType[elementIdx] = IMAP_IAC_IS_MOUSE;
		}

	}
	// <JOYAXIS> <MOUSEAXIS>
	else if(!istrcmp(name, CFML_MOUSEAXIS_TAG) || !istrcmp(name, CFML_JOYAXIS_TAG))
	{
		if(parserData->numEntries < MAX_PARSE_ENTRIES)
			parserData->numEntries++;
		
		idx = parserData->numEntries - 1;
		
		entry 		= &parserData->keymapEntries[idx];
		entryData 	= &parserData->entryData[idx];
		
		entry->keymap.keymapType 		= IMAP_AXIS_KEYMAP;
		entryData->deviceIdx[0]	= deviceIdx;
		entry->idx 				= noedit ?  IMAP_NOEDIT_ID : cfmlIdx;
		entry->keymap.numKeys = 1;

		entry->keymap.axis.invert			= invert;
		entry->keymap.axis.range			= halfaxis;
		entry->keymap.axis.emulateJoystick 	= emulateJoystick;
			
		if(!istrcmp(name, CFML_MOUSEAXIS_TAG))
			entryData->deviceType[0] = IMAP_IAC_IS_MOUSE;
		else
			entryData->deviceType[0] = IMAP_IAC_IS_JOYSTICK;
	}
	// <KEYDELTA>
	else if(!istrcmp(name, CFML_KEYDELTA_TAG))
	{
		// increment the entry count and set the keyaxis type
		if(parserData->numEntries < MAX_PARSE_ENTRIES)
		{
			idx = parserData->numEntries++;
			
			entryData = &parserData->entryData[idx];
			
			entryData->combo = 1;	
			entryData->noedit = noedit;
			entryData->idx = cfmlIdx;
			
			entryData->keyAxisType = IMAP_AXIS_REL_KEYMAP;
			entryData->keyDeltaType =  (!istrcmp(CFML_KEYPRESS, per)) ? IMAP_AXIS_PER_STROKE : IMAP_AXIS_PER_SEC;
			entryData->keyAxisValue = keyAxisValue;
		}
	}
	// <KEYABSOLUTE>
	else if(!istrcmp(name, CFML_KEYABS_TAG))
	{
		// increment the entry count and set the keyaxis type
		if(parserData->numEntries < MAX_PARSE_ENTRIES)
		{
			idx = parserData->numEntries++;
			
			entryData = &parserData->entryData[idx];
			
			entryData->combo = 1;	
			entryData->noedit = noedit;
			entryData->idx = cfmlIdx;
			
			entryData->onRelease = onRelease;
			entryData->releasePercent = releasePercent;
			
			entryData->keyAxisType = IMAP_AXIS_ABS_KEYMAP;	
			entryData->keyAxisValue = keyAxisValue;
		}
	}
}

/************************************************************************************************
	cfmlEndElementHandler
*************************************************************************************************/

static void cfmlEndElementHandler(void *userdata, const xmlChar *name_)
{
    const char *name = reinterpret_cast<const char *>(name_);
	parser_data* parserData = (parser_data*)userdata;
	
	// </CONTROL>
	if(!istrcmp(name, CFML_CONTROL_TAG)) 	
	{	
		int i;

		for(i = 0; i < parserData->numEntries; i++)
		{
			imap_keymap_entry* entry = &parserData->keymapEntries[i];
			entry->functionCode = parserData->functionCode;
			if(entry->functionCode != 0)
			{
				imapKeymapAdd(parserData->ref, entry->functionCode, &entry->idx, &entry->keymap);
			}
		}
		// re-init parser for next control tag
		parserData->data[0] = '\0';
		parserData->functionCode = parserData->numEntries = 0;
		memset(parserData->entryData, 0x0, sizeof(parserData->entryData));
		memset(parserData->keymapEntries, 0x0, sizeof(parserData->keymapEntries));
		
	}
	// </FUNCTION>
	 else if(!istrcmp(name, CFML_FUNCTION_TAG))
	{
		imap_function_info funcInfo;
		// get function code from function name
		inputMap.functionInfo(IMAP_HAVE_NAME,  parserData->language, &parserData->functionCode, 
								parserData->data, MAX_PARSE_STR_LEN, &funcInfo); 
	
	}// </KEY> </JOYBUTTON> </MOUSEBUTTON>
	else if(!istrcmp(name, CFML_KEY_TAG) ||	
			 !istrcmp(name, CFML_JOYBUTTON_TAG) || !istrcmp(name, CFML_MOUSEBUTTON_TAG)) 
	{
		imap_iac_info iacInfo;
		imap_keymap_entry* entry = NULL;
		parser_entry_data* entryData = NULL;
		
		int idx = parserData->numEntries - 1;		
		if(parserData->numEntries)
		{
			 entry = &parserData->keymapEntries[idx];
			 entryData = &parserData->entryData[idx];
		}	
		
		if(entry && entry->keymap.numKeys)
		{
			int elementIdx = entry->keymap.numKeys - 1;
	
			iacInfo.deviceIdx = parserData->entryData[idx].deviceIdx[elementIdx];
			iacInfo.isAxis = 0;
			iacInfo.deviceType = parserData->entryData[idx].deviceType[elementIdx];		
			
			switch(entry->keymap.keymapType)
			{
				case IMAP_BUTTON_KEYMAP:			
					inputMap.iacInfo(IMAP_HAVE_NAME, parserData->language, parserData->keyboard, &entry->keymap.btn.iac[elementIdx], 
										parserData->data, MAX_PARSE_STR_LEN, &iacInfo);	
				break;
				
				case IMAP_AXIS_ABS_KEYMAP:
					inputMap.iacInfo(IMAP_HAVE_NAME, parserData->language, parserData->keyboard, &entry->keymap.axisAbs.iac[elementIdx], 
										parserData->data, MAX_PARSE_STR_LEN, &iacInfo);	
			
				break;
				
				case IMAP_AXIS_REL_KEYMAP:
					inputMap.iacInfo(IMAP_HAVE_NAME, parserData->language, parserData->keyboard, &entry->keymap.axisRel.iac[elementIdx], 
										parserData->data, MAX_PARSE_STR_LEN, &iacInfo);
				break;
			}
		}
	}// </COMBO>
	else if(!istrcmp(name, CFML_COMBO_TAG)) 
	{
		if(parserData->numEntries > 0 && parserData->numEntries <= MAX_PARSE_ENTRIES)
			parserData->entryData[parserData->numEntries - 1].combo = 0;
	}
	// </MOUSEAXIS> </JOYAXIS>
	else if(!istrcmp(name, CFML_MOUSEAXIS_TAG) || !istrcmp(name,CFML_JOYAXIS_TAG)) 
	{
		imap_iac_info iacInfo;
		imap_keymap_entry* entry = NULL;
		imap_axis_keymap* keymap = NULL;
		parser_entry_data* entryData = NULL;
		int idx = parserData->numEntries - 1;
	
		if(parserData->numEntries)
		{
			entry = &parserData->keymapEntries[idx];
			entryData = &parserData->entryData[idx];
		}
			 
		if(entry && entry->keymap.numKeys && entry->keymap.keymapType == IMAP_AXIS_KEYMAP)
		{
			keymap = &entry->keymap.axis;
			
			if(keymap)
			{
				iacInfo.deviceIdx = parserData->entryData[idx].deviceIdx[0];
				iacInfo.isAxis = 1;
				iacInfo.deviceType = parserData->entryData[idx].deviceType[0];

				inputMap.iacInfo(IMAP_HAVE_NAME, parserData->language, parserData->keyboard, &keymap->iac, 
									parserData->data, MAX_PARSE_STR_LEN, &iacInfo);	
			}
		}
		
	}	// </CFML_KEYDELTA_TAG> </CFML_KEYABS_TAG>
	else if(!istrcmp(name, CFML_KEYDELTA_TAG) || !istrcmp(name, CFML_KEYABS_TAG))
	{
		if(parserData->numEntries > 0 && parserData->numEntries <= MAX_PARSE_ENTRIES)
		{
			parserData->entryData[parserData->numEntries - 1].combo = 0;
			parserData->entryData[parserData->numEntries - 1].keyAxisType = 0;
		}
	}
}

/************************************************************************************************
	cfmlDataHandler
*************************************************************************************************/

static void cfmlDataHandler(void *userdata, const xmlChar *s, int len)
{
	parser_data* parserData = (parser_data*)userdata;
	int dlen = strlen(parserData->data);
	
	if(dlen + len > MAX_PARSE_STR_LEN) len = MAX_PARSE_STR_LEN - dlen;
	
	memcpy(parserData->data + dlen, s, len);
	parserData->data[dlen + len] = '\0';
}

/************************************************************************************************
	cfmlWriteButtonEntry
*************************************************************************************************/

static void cfmlWriteButtonEntry(cfmlWriterRef cfmlRef, imap_keymap_entry* entry)
{
	imap_button_keymap* keymap = &entry->keymap.btn;
	
	char iacName[MAX_PARSE_STR_LEN+1];
	int edit = (entry->idx < IMAP_NOEDIT_ID); // by convention
	int i;
	imap_iac_info iacInfo;
	int idx = (edit) ? (int)entry->idx : -1;
	
	if(entry->keymap.numKeys > 1)
	{
		cfmlWriteComboBegin(cfmlRef, edit, idx);
		edit = 1;
		idx = -1;
	}
	
	for(i = 0; i < entry->keymap.numKeys; i++)
	{
		inputMap.iacInfo(IMAP_HAVE_CODE, CFML_ENGLISH_LANGUAGE, CFML_US_KEYBOARD, &keymap->iac[i], 
							iacName, MAX_PARSE_STR_LEN, &iacInfo);

		switch(iacInfo.deviceType)
		{
			case IMAP_IAC_IS_KEYBOARD:
				cfmlWriteKey(cfmlRef, edit, idx, iacName);
			break;
			
			case IMAP_IAC_IS_MOUSE:	
				cfmlWriteMouseButton(cfmlRef, edit, idx, iacName);
			break;
			
			case IMAP_IAC_IS_JOYSTICK:
				cfmlWriteJoyButton(cfmlRef, edit, idx, iacInfo.deviceIdx, iacName);
			break;
		}
	}
	
	if(entry->keymap.numKeys > 1)
		cfmlWriteComboEnd(cfmlRef);
}

/************************************************************************************************
	cfmlWriteAxisEntry
*************************************************************************************************/

static void cfmlWriteAxisEntry(cfmlWriterRef cfmlRef, imap_keymap_entry* entry)
{
	imap_axis_keymap* keymap = &entry->keymap.axis;
	
	char iacName[MAX_PARSE_STR_LEN+1];
	int edit = (entry->idx < IMAP_NOEDIT_ID); // by convention
	imap_iac_info iacInfo;
	int idx = (edit) ? (int)entry->idx : -1;
	
	inputMap.iacInfo(IMAP_HAVE_CODE, CFML_ENGLISH_LANGUAGE, CFML_US_KEYBOARD, &keymap->iac, iacName, MAX_PARSE_STR_LEN,
						&iacInfo);

	if(keymap->iac == IMAP_IAC_NOKEY) iacInfo.deviceType = IMAP_IAC_IS_JOYSTICK;
	
	switch(iacInfo.deviceType)
	{							
		case IMAP_IAC_IS_MOUSE:
			cfmlWriteMouseAxis(cfmlRef, edit, idx, keymap->range, 
								keymap->invert, iacName,  keymap->emulateJoystick);
		break;
		
		case IMAP_IAC_IS_JOYSTICK:
			cfmlWriteJoyAxis(cfmlRef, edit, idx, iacInfo.deviceIdx, keymap->range, 
								keymap->invert, iacName);
		break;
	}
}

/************************************************************************************************
	cfmlWriteAxisRelEntry
*************************************************************************************************/

static void cfmlWriteAxisRelEntry(cfmlWriterRef cfmlRef, imap_keymap_entry* entry)
{
	imap_axis_rel_keymap* keymap = &entry->keymap.axisRel;
	
	char iacName[MAX_PARSE_STR_LEN+1];
	int edit = (entry->idx < IMAP_NOEDIT_ID); // by convention
	int i;
	imap_iac_info iacInfo;
	int idx = (edit) ? (int)entry->idx : -1;
	
	const char* mapType = (keymap->axisMapType == IMAP_AXIS_PER_STROKE) ? CFML_KEYPRESS : CFML_SEC;
	float value = keymap->rate * 100.0f; //(int)(keymap->rate * 100.0f + ((keymap->rate > 0) ? 1 : -1) * 0.5f);
	
	cfmlWriteKeyDeltaBegin(cfmlRef, edit, idx, value, mapType);
	
	edit = 1;
	idx = -1;
	
	for(i = 0; i < entry->keymap.numKeys; i++)
	{
		inputMap.iacInfo(IMAP_HAVE_CODE, CFML_ENGLISH_LANGUAGE, CFML_US_KEYBOARD, &keymap->iac[i], 
							iacName, MAX_PARSE_STR_LEN, &iacInfo);

		switch(iacInfo.deviceType)
		{
			case IMAP_IAC_IS_KEYBOARD:
				cfmlWriteKey(cfmlRef, edit, idx, iacName);
			break;
			
			case IMAP_IAC_IS_MOUSE:	
				cfmlWriteMouseButton(cfmlRef, edit, idx, iacName);
			break;
			
			case IMAP_IAC_IS_JOYSTICK:
				cfmlWriteJoyButton(cfmlRef, edit, idx, iacInfo.deviceIdx, iacName);
			break;
		}
	}
	
	cfmlWriteKeyDeltaEnd(cfmlRef);
}

/************************************************************************************************
	cfmlWriteAxisAbsEntry
*************************************************************************************************/

static void cfmlWriteAxisAbsEntry(cfmlWriterRef cfmlRef, imap_keymap_entry* entry)
{
	imap_axis_abs_keymap* keymap = &entry->keymap.axisAbs;

	char iacName[MAX_PARSE_STR_LEN+1];
	int edit = (entry->idx < IMAP_NOEDIT_ID); // by convention
	int i;
	imap_iac_info iacInfo;
	int idx = (edit) ? (int)entry->idx : -1;
	
	float value = keymap->percent * 100.0f; //(int)(keymap->percent * 100.0f + 0.5f);
	float releaseValue = keymap->releasePercent * 100.0f; //(int)(keymap->releasePercent * 100.0f + 0.5f);
	
	cfmlWriteKeyAbsoluteBegin(cfmlRef, edit, idx, value, keymap->onRelease, releaseValue);
	
	edit = 1;
	idx = -1;
	
	for(i = 0; i < entry->keymap.numKeys; i++)
	{
		inputMap.iacInfo(IMAP_HAVE_CODE, CFML_ENGLISH_LANGUAGE, CFML_US_KEYBOARD, &keymap->iac[i], 
							iacName, MAX_PARSE_STR_LEN, &iacInfo);

		switch(iacInfo.deviceType)
		{
			case IMAP_IAC_IS_KEYBOARD:
				cfmlWriteKey(cfmlRef, edit, idx, iacName);
			break;
			
			case IMAP_IAC_IS_MOUSE:	
				cfmlWriteMouseButton(cfmlRef, edit, idx, iacName);
			break;
			
			case IMAP_IAC_IS_JOYSTICK:
				cfmlWriteJoyButton(cfmlRef, edit, idx, iacInfo.deviceIdx, iacName);
			break;
		}
	}
	
	cfmlWriteKeyAbsoluteEnd(cfmlRef);
}

/************************************************************************************************
	keymap_read
*************************************************************************************************/

static int keymap_read(void* userdata, int numbytes, void* buf)
{
	parser_data* parserData = (parser_data*)userdata;
	return inputMap.read(parserData->userdata, numbytes, buf);
}

/************************************************************************************************
	str_to_bit
*************************************************************************************************/

static unsigned int str_to_bit(const char* bitstr)
{
	int i;
	int len = strlen(bitstr);
	unsigned int mask = 0x0;
	unsigned int bit = 0x0;
	
	if(len > 32) len = 32;
	
	for(i = 0; i < len; i++)
	{
		bit = (bitstr[i] == '0') ? 0 : 1;
		mask |= (bit << i);
	}
	
	return mask;
}

/************************************************************************************************
	istrcmp
*************************************************************************************************/

static int istrcmp(const char* str1, const char* str2)
{
	const char * p1 = str1;
	const char * p2 = str2;
	char c1, c2;
	
	for(;;)
	{
		c1 = *p1++; 
		c2 = *p2++;
		c1 = tolower(c1);
		c2 = tolower(c2);
		
		if(c1 != c2) break;
		if (!c1)
			return(0);
	}
	return(c1 - c2);
}
