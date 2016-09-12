#include <syscfg.h> // Path is project specific.
#include "xml/XMLLoader2.h"

#if platBuild != TEUL_SERVER
#include "ui/uiMain.h" // for ui fio callbacks
#endif

#if platOS!=OS_WIN32
#define stricmp strcasecmp
#endif

const char* xmlTrimString(const char* value)
{
	static char buffer[MAX_XML_TRIM_SIZE+1];
	static char* trimmed;

	memset(buffer, 0, MAX_XML_TRIM_SIZE+1);

	if(value && strlen(value))
		strncpy(buffer, value, MAX_XML_TRIM_SIZE);

	trimmed = buffer;

	//return rmlead(rmtrail(trimmed));
	return trimmed;
}

bool xmlGetNodeValue(xmlNode* node, XML_DATA_TYPE type, void* value, const char* attribute)
{
	static char buffer[4096+1];
	xmlChar* data = (attribute) ? xmlGetProp(node, (xmlChar*)attribute):
								  xmlNodeGetContent(node);

	if(data)
	{
		switch(type)
		{
			case XML_DATA_BOOL:
				*(bool*)value = (!stricmp((const char*)data, "true")) ? true: false;
			break;
			case XML_DATA_SINT:
				*(int*)value = atoi((const char*)data);
			break;
			case XML_DATA_UINT:
				*(unsigned int*)value = (unsigned int)atoi((const char*)data);
			break;
			case XML_DATA_LONG:
				*(long*)value = atol((const char*)data);
			break;
			case XML_DATA_FLOAT:
				*(float*)value = (float)atof((const char*)data);
			break;
			case XML_DATA_DOUBLE:
				*(double*)value = atof((const char*)data);
			break;
			case XML_DATA_STRING:
				buffer[0] = 0;
				strncpy(buffer, (const char*)data, 255); // why is this only 255, the buffer can hold up to 4096 characters
				*(const char**)value = buffer;
			break;
			default:
				// bad type
			break;
		}

		xmlFree(data);
		return true;
	}

	return false;
}

const char* xmlGetStringFromNode(xmlNode* node, const char* attribute)
{
	const char* str = nullptr;
	return (xmlGetNodeValue(node, XML_DATA_STRING, &str, attribute)) ? str: "";
}

#if platBuild != TEUL_SERVER
// lame match functions are everywhere
static int xmlinputmatch(const char* filename) { return 1; }

void xmlRegisterUIFIO()
{
	xmlRegisterInputCallbacks(xmlinputmatch,
									  (xmlInputOpenCallback)fioCallbacks.openread,
									  (xmlInputReadCallback)fioCallbacks.read,
									  (xmlInputCloseCallback)fioCallbacks.close);
}
#endif
