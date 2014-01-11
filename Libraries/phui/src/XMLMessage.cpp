#include "phui-precompiled-header.h"

/************************************************************************************************
	XMLMessage
*************************************************************************************************/

#include <stdio.h>
#include <string.h>

#include "libxml/tree.h"

#include "phui/XMLMessage.h"
#include "phui/Message.h"
#include "phui/Util.h"

#ifdef _WIN32
#define snprintf _snprintf
#endif
using namespace phui;

#define MESSAGE_TAG		"message"
#define VALUE_TAG		"value"
#define SCALAR_TAG		"scalar"
#define DATA_TAG		"data"

#define WHAT_ATTRIB		"what"
#define NAME_ATTRIB		"name"
#define TYPE_ATTRIB		"type"
#define VALUE_ATTRIB	"value"

#define BOOL_VALUE		"bool"
#define CHAR_VALUE		"char"
#define SHORT_VALUE		"short"
#define LONG_VALUE		"long"
#define FLOAT_VALUE		"float"
#define DOUBLE_VALUE	"double"
#define STRING_VALUE	"string"
#define POINTER_VALUE	"pointer"
#define WIDE_STRING_VALUE "wstring"

/*
<message what="">
	<value name="" type="" value=""/>
	<scalar name="" type="" value=""/>
	<message name="" what=""> </message>
	<data name=""> </data>
</message>
*/

static int xmlinputmatch(char const *filename){ return 1;}

static void archive_message(const Message* msg, xmlNodePtr node);
static void restore_message(Message* msg, xmlNodePtr node);

static void add_message_node(const Message* msg, xmlNodePtr node, const char* name, int j);
static void add_scalar_node(const Message* msg, xmlNodePtr node, const char* name, int j);
static void add_value_node(const Message* msg, xmlNodePtr node, const char* name, int j, long type);

static void restore_message_node(Message* msg, xmlNodePtr node);
static void restore_scalar_node(Message* msg, xmlNodePtr node);
static void restore_value_node(Message* msg, xmlNodePtr node);

/************************************************************************************************
	XMLMessage
*************************************************************************************************/

XMLMessage::XMLMessage(IOCallbacks const* io) : _io(io)
{
}

XMLMessage::~XMLMessage()
{
}

/************************************************************************************************
	Archive
*************************************************************************************************/

void XMLMessage::Archive(const Message* msg, const char* name)
{
    xmlDocPtr doc = nullptr;    
	xmlNodePtr node = nullptr;

	doc = xmlNewDoc((xmlChar*)"1.0");
    node = xmlNewNode(nullptr, (xmlChar*)"message");
    xmlDocSetRootElement(doc, node);
	
	archive_message(msg, node);

	xmlRegisterOutputCallbacks(xmlinputmatch, 
								(xmlOutputOpenCallback)_io->openwrite, 
								(xmlOutputWriteCallback)_io->write, 
								(xmlOutputCloseCallback)_io->close); 
								
	xmlSaveFormatFileEnc(name, doc, "UTF-8", 1);
  
	xmlFreeDoc(doc);
    xmlCleanupOutputCallbacks();
}

/************************************************************************************************
	Restore
*************************************************************************************************/

void XMLMessage::Restore(Message* msg, const char* name)
{
	msg->MakeEmpty();
	
	xmlRegisterInputCallbacks(xmlinputmatch, 
								(xmlInputOpenCallback)_io->openread, 
								(xmlInputReadCallback)_io->read, 
								(xmlInputCloseCallback)_io->close); 
								
	xmlDocPtr doc = xmlReadFile(name, nullptr, XML_PARSE_DTDLOAD);
	if(doc)
	{
		xmlNodePtr node = xmlDocGetRootElement(doc);

		if(node)
			restore_message(msg, node);
		
		xmlFreeDoc(doc);
	}
	xmlCleanupInputCallbacks();
}

/************************************************************************************************
	archive_message
*************************************************************************************************/

static void archive_message(const Message* msg, xmlNodePtr node)
{
	char str[64];
	snprintf(str, 63, "%d", (int)msg->what);
	xmlNewProp(node, (xmlChar*)WHAT_ATTRIB, (xmlChar*)str);

	const char *name;
	long type;
	long count; 
	
	for(int i = 0; !msg->GetInfo(Message::ANY_TYPE, i, &name, &type, &count); i++)
	{
		for(int j = 0; j < count; j++)
		{
			switch(type)
			{
				case Message::MESSAGE_TYPE:
					add_message_node(msg, node, name, j);
				break;
				case Message::SCALAR_TYPE:
					add_scalar_node(msg, node, name, j);
				break;
				default:
					add_value_node(msg, node, name, j, type);
				break;
			}
		}
	}
}

/************************************************************************************************
	restore_message
*************************************************************************************************/

static void restore_message(Message* msg, xmlNodePtr node)
{
	while(node)
	{
		if(node->type == XML_ELEMENT_NODE) break;
		node = node->next;
	}

	if(nullptr == node) return;
	
	char* what = (char*)xmlGetProp(node, (xmlChar*)WHAT_ATTRIB);
	if(what)
	{
		msg->what = (long)atoi(what);
		xmlFree((xmlChar*)what);
	}
	
	for(xmlNodePtr cur = node->children; cur; cur = cur->next) 
	{
		if(cur->type != XML_ELEMENT_NODE)
			continue;

		if(!strcmp((char*)cur->name,MESSAGE_TAG))
		{
			restore_message_node(msg, cur);
		}else if(!strcmp((char*)cur->name, SCALAR_TAG))
		{
			restore_scalar_node(msg, cur);
		}else if(!strcmp((char*)cur->name, VALUE_TAG))
		{
			restore_value_node(msg, cur);
		}
	}
}

/************************************************************************************************
	add_message_node
*************************************************************************************************/

static void add_message_node(const Message* msg, xmlNodePtr node, const char* name, int j)
{
	Message subMsg;
	msg->FindMessage(name,&subMsg,j);
	
	xmlNodePtr child = xmlNewNode(nullptr, (xmlChar*)MESSAGE_TAG);
	xmlNewProp(child, (xmlChar*)NAME_ATTRIB, (xmlChar*)name);	

	archive_message(&subMsg, child);
	xmlAddChild(node, child);
}

/************************************************************************************************
	add_scalar_node
*************************************************************************************************/

static void add_scalar_node(const Message* msg, xmlNodePtr node, const char* name, int j)
{
	Scalar val;
	msg->FindScalar(name, &val, j);
		
	const char* type = nullptr;
	switch(val.Type())
	{
		case Scalar::UndefT:
		break;
		case Scalar::BoolT:
			type = BOOL_VALUE;
		break;
		case Scalar::CharT:
			type = CHAR_VALUE;
		break; 
		case Scalar::ShortT:
			type = SHORT_VALUE;
		break;
		case Scalar::LongT:
			type = LONG_VALUE;
		break;
		case Scalar::FloatT:
			type = FLOAT_VALUE;
		break;
		case Scalar::DoubleT:
			type = DOUBLE_VALUE;
		break;
		case Scalar::PointerT:
			type = POINTER_VALUE;
		break; 
		case Scalar::StringT:
			type = STRING_VALUE;
		break;
		case Scalar::WideStringT:
		break;
	}

	if(type)
	{
		xmlNodePtr child = xmlNewChild(node, nullptr, (xmlChar*)SCALAR_TAG, nullptr);

		xmlNewProp(child, (xmlChar*)NAME_ATTRIB, (xmlChar*)name);
		xmlNewProp(child, (xmlChar*)TYPE_ATTRIB, (xmlChar*)type);	
		xmlNewProp(child, (xmlChar*)VALUE_ATTRIB, (xmlChar*)val.String());	
	
		xmlAddChild(node, child);
	}
}

/************************************************************************************************
	add_value_node
*************************************************************************************************/

static void add_value_node(const Message* msg, xmlNodePtr node, const char* name, int j, long type)
{
	char val[64];
	const char* valtype = nullptr;
	const char *valptr = val;

	switch(type)
	{
		case Message::BOOL_TYPE:
		{
			bool b = false;
			msg->FindBool(name,&b,j);
			snprintf(val, 63, "%s", b ? "true" : "false");
			valtype = BOOL_VALUE;
		}
		break;
	
		case Message::CHAR_TYPE:
		{
			char c = 0;
			msg->FindChar(name,&c,j);
			snprintf(val, 63, "%c", c);
			valtype = CHAR_VALUE;
		}
		break;
		
		case Message::SHORT_TYPE:
		{
			short s = 0;
			msg->FindShort(name,&s,j);
			snprintf(val, 63, "%hd", s);
			valtype = SHORT_VALUE;
		}
		break;
		
		case Message::LONG_TYPE:
		{
			long l = 0;
			msg->FindLong(name,&l,j);
			snprintf(val, 63, "%d", (int)l);
			valtype = LONG_VALUE;
		}
		break;
		
		case Message::FLOAT_TYPE:
		{
			float f = 0.0;
			msg->FindFloat(name,&f,j);
			snprintf(val, 63, "%f", f);
			valtype = FLOAT_VALUE;
		}
		break;
		
		case Message::DOUBLE_TYPE:
		{
			double d = 0.0;
			msg->FindDouble(name,&d,j);
			snprintf(val, 63, "%g", d);
			valtype = DOUBLE_VALUE;
		}
		break;
		
		case Message::POINTER_TYPE:
		{
			long ptr = 0;
			msg->FindPointer(name,(void**)&ptr,j);
			snprintf(val, 63, "%d", (int)ptr);
			valtype = POINTER_VALUE;
		}
		break;
		
		case Message::STRING_TYPE:
		{
			msg->FindString(name,&valptr,j);
			valtype = STRING_VALUE;
		}
		break;
		
		default:
			valtype = nullptr;
			valptr = nullptr;
		break;
	}
	
	if(valtype && valptr)
	{
		xmlNodePtr childnode =  xmlNewChild(node, nullptr, (xmlChar*)VALUE_TAG, nullptr);

		xmlNewProp(childnode, (xmlChar*)NAME_ATTRIB, (xmlChar*)name);	
		xmlNewProp(childnode, (xmlChar*)TYPE_ATTRIB, (xmlChar*)valtype);	
		xmlNewProp(childnode, (xmlChar*)VALUE_ATTRIB, (xmlChar*)valptr);	

		xmlAddChild(node, childnode);
	}
}

/************************************************************************************************
	restore_message_node
*************************************************************************************************/

static void restore_message_node(Message* msg, xmlNodePtr node)
{
	char* name = (char*)xmlGetProp(node, (xmlChar*)NAME_ATTRIB);			
	if(name)
	{
		Message submsg;
		restore_message(&submsg, node);
		msg->AddMessage(name, &submsg);
		
		xmlFree((xmlChar*)name);
	}
}

/************************************************************************************************
	restore_scalar_node
*************************************************************************************************/

static void restore_scalar_node(Message* msg, xmlNodePtr node)
{
	char* name = (char*)xmlGetProp(node, (xmlChar*)NAME_ATTRIB);			
	if(name)
	{
		char* type = (char*)xmlGetProp(node, (xmlChar*)TYPE_ATTRIB);		
		if(type)
		{
			char* value = (char*)xmlGetProp(node, (xmlChar*)VALUE_ATTRIB);		
			
			if(value)
			{
				Scalar scalar = value;
				
				if(!strcmp(type, BOOL_VALUE))
					scalar.Convert(Scalar::BoolT);
				else if(!strcmp(type, CHAR_VALUE))
					scalar.Convert(Scalar::CharT);
				else if(!strcmp(type, SHORT_VALUE))
					scalar.Convert(Scalar::ShortT);
				else if(!strcmp(type, LONG_VALUE))
					scalar.Convert(Scalar::LongT);
				else if(!strcmp(type, FLOAT_VALUE))
					scalar.Convert(Scalar::FloatT);
				else if(!strcmp(type, DOUBLE_VALUE))
					scalar.Convert(Scalar::DoubleT);
				else if(!strcmp(type, POINTER_VALUE))
					scalar.Convert(Scalar::PointerT);
					
				msg->AddScalar((char*)name, scalar);

				xmlFree((xmlChar*)value);
			}
			
			xmlFree((xmlChar*)type);			
		}
		xmlFree((xmlChar*)name);
	}
}

/************************************************************************************************
	restore_value_node
*************************************************************************************************/

static void restore_value_node(Message* msg, xmlNodePtr node)
{
	char* name = (char*)xmlGetProp(node, (xmlChar*)NAME_ATTRIB);			
	
	if(name)
	{
		char* type = (char*)xmlGetProp(node, (xmlChar*)TYPE_ATTRIB);
		if(type)
		{
			char* value = (char*)xmlGetProp(node, (xmlChar*)VALUE_ATTRIB);
				
			if(value)
			{
				if(!strcmp(type, BOOL_VALUE))
					msg->AddBool(name, !strcmp(value, "true"));
				else if(!strcmp(type, CHAR_VALUE))
					msg->AddChar(name, value[0]);
				else if(!strcmp(type, SHORT_VALUE))
					msg->AddShort(name, (short)atoi(value));
				else if(!strcmp(type, LONG_VALUE))
					msg->AddLong(name, (long)atoi(value));
				else if(!strcmp(type, FLOAT_VALUE))
					msg->AddFloat(name, (float)atof(value));
				else if(!strcmp(type, DOUBLE_VALUE))
					msg->AddDouble(name, (double)atof(value));
				else if(!strcmp(type, STRING_VALUE))
					msg->AddString(name, value);
				else if(!strcmp(type, POINTER_VALUE))
					msg->AddPointer(name, (void*)(size_t)atoi(value));
				xmlFree((xmlChar*)value);
			}
			xmlFree((xmlChar*)type);
		}
		xmlFree((xmlChar*)name);
	}
}


