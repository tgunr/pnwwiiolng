#include "phui-precompiled-header.h"

#include <string>
#include <map>

#include "phui/Message.h"

using namespace std;
using namespace phui;

/*************************************************************************************************
	MessageData
*************************************************************************************************/

struct MessageData
{
	long type;
	void* data;
	long size;
	
	MessageData(long inType, const void* inData, long inSize);
	MessageData(const MessageData&);
	~MessageData();

	void SetData(long inType, const void* inData, long inSize);
	void FreeData();
};

MessageData::MessageData(long inType, const void* inData, long inSize) : type(0), data(0), size(0)
{
	SetData(inType,inData,inSize);
}

MessageData::MessageData(const MessageData& orig) : type(0), data(0), size(0)
{
	SetData(orig.type, orig.data, orig.size);
}

MessageData::~MessageData()
{
	FreeData();
}

void MessageData::SetData(long inType, const void* inData, long inSize)
{
	FreeData();
	
	type = inType;
	size = inSize;
	
	switch(inType)
	{
		case Message::MESSAGE_TYPE:
			data = new Message(*(Message*)inData);
		break;
#ifdef USE_SCALAR		
		case Message::SCALAR_TYPE:
			data = new Scalar(*(Scalar*)inData);
		break;
#endif		
		default:
			data = malloc(inSize);
			memcpy(data,inData,inSize);
		break;
	}
}

void MessageData::FreeData()
{
	if(!data) return;
	
	switch(type)
	{
		case Message::MESSAGE_TYPE:
			delete (Message*)data;
		break;
#ifdef USE_SCALAR
		case Message::SCALAR_TYPE:
			delete (Scalar*)data;
		break;
#endif				
		default:
			free(data);
		break;
	}
	data = 0;
}

typedef multimap<string,MessageData*> MessageHash;

/*************************************************************************************************
	MessageImp
*************************************************************************************************/

struct Message::MessageImp 
{	
	MessageHash hash;
};

/*************************************************************************************************
	Message
*************************************************************************************************/

Message::Message(long w) : what(w),  mImp(0)
{
}

Message::Message(): what(0), mImp(0)
{
}

Message::Message(const Message& orig): what(0), mImp(0)
{
	*this = orig;	
}

Message::~Message()
{
	MakeEmpty();
}

Message& Message::operator = (const Message& orig)
{
	if(this == &orig) return *this;
	
	MakeEmpty();
	
	what = orig.what;
	
	if(orig.mImp)
	{
		mImp = new MessageImp;
		MessageHash::const_iterator it = orig.mImp->hash.begin();
		for(; it !=  orig.mImp->hash.end(); ++it)
		{
			MessageData* data = new MessageData(*it->second);
			
			MessageHash::value_type item(it->first, data);
			mImp->hash.insert(item);
		}
	}
	return *this;
}

/*************************************************************************************************
	AddScalar
*************************************************************************************************/
#ifdef USE_SCALAR
long Message::AddScalar(const char* name, Scalar const& val)
{
	return AddData(name, SCALAR_TYPE, &val, sizeof(val));
}
#endif
/*************************************************************************************************
	AddBool
*************************************************************************************************/

long Message::AddBool(const char* name, bool val)
{
	return AddData(name, BOOL_TYPE, &val, sizeof(val));
}

/*************************************************************************************************
	AddChar
*************************************************************************************************/

long Message::AddChar(const char* name, char val)
{
	return AddData(name, CHAR_TYPE, &val, sizeof(val));
}

/*************************************************************************************************
	AddShort
*************************************************************************************************/

long Message::AddShort(const char* name, short val)
{
	return AddData(name, SHORT_TYPE, &val, sizeof(val));
}

/*************************************************************************************************
	AddLong
*************************************************************************************************/

long Message::AddLong(const char* name, long val)
{
	return AddData(name, LONG_TYPE, &val, sizeof(val));
}

/*************************************************************************************************
	AddFloat
*************************************************************************************************/

long Message::AddFloat(const char* name, float val)
{
	return AddData(name, FLOAT_TYPE, &val, sizeof(val));
}

/*************************************************************************************************
	AddDouble
*************************************************************************************************/

long Message::AddDouble(const char* name, double val)
{
	return AddData(name, DOUBLE_TYPE, &val, sizeof(val));
}

/*************************************************************************************************
	AddString
*************************************************************************************************/

long Message::AddString(const char* name, const char* str)
{
	return AddData(name, STRING_TYPE, str, (long)(strlen(str) + 1));
}

/*************************************************************************************************
	AddWideString
*************************************************************************************************/

long Message::AddWideString(const char* name, const wchar_t* str)
{
	return AddData(name, WIDE_STRING_TYPE, str, (long)(wcslen(str) + 1));
}

/*************************************************************************************************
	AddPointer
*************************************************************************************************/

long Message::AddPointer(const char* name, const void* ptr)
{
	return AddData(name,POINTER_TYPE, &ptr, sizeof(ptr));
}

/*************************************************************************************************
	AddMessage
*************************************************************************************************/

long Message::AddMessage(const char* name, const Message* msg)
{
	return AddData(name,MESSAGE_TYPE, msg, sizeof(msg));
}

/*************************************************************************************************
	AddData
*************************************************************************************************/

long Message::AddData(const char* name, long type, const void* data, long size)
{
	const void* d;
	long sz;
	long err = FindData(name,type,&d,&sz,0);
	
	if(err == OK || err == NAME_NOT_FOUND)
	{
		if(!mImp) mImp = new MessageImp;
		
		MessageData* msgData = new MessageData(type, data, size);
		
		string str(name);
		MessageHash::value_type item(str, msgData);
		mImp->hash.insert(item);
		
		err = OK;
	}

	return err;
}

/*************************************************************************************************
	FindScalar
*************************************************************************************************/
#ifdef USE_SCALAR
long Message::FindScalar(const char* name, Scalar* val, long index) const
{
	Scalar* origVal;
	long size;
	long err = FindData(name, SCALAR_TYPE, (const void**)&origVal, &size, index);

	if(!err)
		*val = *origVal;
	
	return err;
}
#endif
/*************************************************************************************************
	FindBool
*************************************************************************************************/

long Message::FindBool(const char* name, bool* val, long index) const
{
	const void* data;
	long size;
	long err = FindData(name, BOOL_TYPE, &data, &size, index);

	if(!err)
		*val = *(bool*)data;
	
	return err;
}


/*************************************************************************************************
	FindChar
*************************************************************************************************/

long Message::FindChar(const char* name, char* val, long index) const
{
	const void* data;
	long size;
	
	long err = FindData(name, CHAR_TYPE, &data, &size, index);
	if(!err)
		*val = *(char*)data;
	
	return err;
}

/*************************************************************************************************
	FindShort
*************************************************************************************************/

long Message::FindShort(const char* name, short* val,long index) const
{
	const void* data;
	long size;
	
	long err = FindData(name, SHORT_TYPE, &data, &size, index);
	if(!err)
		*val = *(short*)data;
	
	return err;
}

/*************************************************************************************************
	FindLong
*************************************************************************************************/

long Message::FindLong(const char* name, long* val, long index) const
{
	const void* data;
	long size;
	
	long err = FindData(name, LONG_TYPE, &data, &size, index);
	if(!err)
		*val = *(long*)data;
	
	return err;
}

/*************************************************************************************************
	FindFloat
*************************************************************************************************/

long Message::FindFloat(const char* name, float* val, long index) const
{
	const void* data;
	long size;
	
	long err = FindData(name, FLOAT_TYPE, &data, &size, index);
	if(!err)
		*val = *(float*)data;
	
	return err;
}

/*************************************************************************************************
	FindDouble
*************************************************************************************************/

long Message::FindDouble(const char* name, double* val, long index) const
{
	const void* data;
	long size;
	
	long err = FindData(name, DOUBLE_TYPE, &data, &size, index);
	if(!err)
		*val = *(double*)data;
	
	return err;
}

/*************************************************************************************************
	FindString
*************************************************************************************************/

long Message::FindString(const char* name, const char** str, long index) const
{
	long size;	
	return FindData(name, STRING_TYPE, (const void**)str, &size, index);
}

/*************************************************************************************************
	FindWideString
*************************************************************************************************/

long Message::FindWideString(const char* name, const wchar_t** str, long index) const
{
	long size;	
	return FindData(name, WIDE_STRING_TYPE, (const void**)str, &size, index);
}

/*************************************************************************************************
	FindPointer
*************************************************************************************************/

long Message::FindPointer(const char* name, void** ptr, long index) const
{
	const void* data;
	long size;	
	
	long err = FindData(name, POINTER_TYPE, &data, &size, index);
	if(!err)
		*ptr = *(void**)data;
		
	return err;
}

/*************************************************************************************************
	FindMessage
*************************************************************************************************/

long Message::FindMessage(const char* name, Message* msg, long index) const
{
	long size;	
	
	Message* origMsg;
	long err = FindData(name, MESSAGE_TYPE, (const void**)&origMsg, &size, index);
	
	if(!err)
		*msg = *origMsg;
	
	return err;
}

/*************************************************************************************************
	FindData
*************************************************************************************************/

long Message::FindData(const char* name, long type, const void **data, long* size, long index) const
{
	*size = 0;
	if(!mImp) return NAME_NOT_FOUND;
	
	long err = OK;
	string str(name);
		
	MessageHash::const_iterator it = mImp->hash.find(str);
	if(it != mImp->hash.end())
	{
		while(it != mImp->hash.begin())
		{
			--it;
			if(it->first != str)
			{
				++it;
				break;
			}
		}
	
		for(int i = 0; i < index && it != mImp->hash.end(); i++)
			++it;
	
		if(it != mImp->hash.end() && it->first == str)
		{		
			if(it->second->type == type)
			{
				*data = it->second->data;
				*size = it->second->size;
			}else
				err = BAD_TYPE;
		}else
			err = BAD_INDEX;
	}else
		err = NAME_NOT_FOUND;
	
	return err;
}

/*************************************************************************************************
	FindData
*************************************************************************************************/

Message::KeyList Message::FindKeys(const char* preName) const
{
	Message::KeyList list;
	
	if(!mImp)
		return list;
	
	MessageHash::const_iterator it = mImp->hash.begin();

	for(; it != mImp->hash.end(); ++it)
	{
		const char* key = it->first.c_str();
		const char* found = strstr(key, preName);
		
		if(found == key) // the preName is at the beginning of the string
			list.push_back(std::string(key));
	}
	
	return list;
}


/*************************************************************************************************
	MakeEmpty
*************************************************************************************************/

void Message::MakeEmpty()
{
	if(!mImp) return;
	
	MessageHash::iterator it = mImp->hash.begin();
	for(; it != mImp->hash.end(); ++it)
		delete it->second;
	
	mImp->hash.erase(mImp->hash.begin(), mImp->hash.end());
	
	delete mImp;
	mImp = NULL;
}

/*************************************************************************************************
	IsEmpty
*************************************************************************************************/

bool Message::IsEmpty()
{
	return (mImp == NULL || mImp->hash.size() == 0);
}


/*************************************************************************************************
	GetInfo
*************************************************************************************************/

long Message::GetInfo(const char* name, long* typeFound, long* countFound) const
{
	*typeFound = 0;
	*countFound = 0;
	
	if(!mImp) return NAME_NOT_FOUND;
	
	long err = OK;
	string str(name);
	
	MessageHash::const_iterator it = mImp->hash.find(str);
	if(it != mImp->hash.end())
	{
		*typeFound = it->second->type;
		/*for(*countFound = 0; it != mImp->hash.end() && it->first == str;  ++(*countFound))
			++it;*/
		*countFound = 0;
		while(it != mImp->hash.begin())
		{
			if(it->first != str) break;  
			++(*countFound);
			--it;
		}
	
	}else
		err = NAME_NOT_FOUND;
	
	return err;
}

/*************************************************************************************************
	GetInfo
*************************************************************************************************/

long Message::GetInfo(long type, long index, const char** nameFound, long* typeFound, long* countFound) const
{
	*typeFound = 0;
	*countFound = 0;
	*nameFound = 0;
	
	if(!mImp) return NAME_NOT_FOUND;

	long err = OK;

	MessageHash::const_iterator it = mImp->hash.begin();
	if(type != ANY_TYPE)
	{
		while(it != mImp->hash.end())
		{
			if(it->second->type == type)  break;
			 ++it;
		}
	}
	
	for(int i = 0; i < index && it != mImp->hash.end();)
	{
		if(type == ANY_TYPE || it->second->type == type) i++;
		
		string str(it->first);

		while(it != mImp->hash.end())
		{
			if(it->first != str) break;
			++it;
		}
		if(it == mImp->hash.end()) break;
	}

	if(type != ANY_TYPE)
	{
		while(it != mImp->hash.end())
		{
			if(it->second->type == type)  break;
			 ++it;
		}
	}
	
	if(it != mImp->hash.end() && ((type == ANY_TYPE) || it->second->type == type))
	{
		*nameFound = it->first.c_str();
		*typeFound = it->second->type;
		
		string str(it->first);
		for(*countFound = 0; it != mImp->hash.end(); ++it)
		{
			if(it->first == str) 
				++(*countFound);
			else
				break;
		}
	}else
		err = BAD_INDEX;
	
	return err;
}

/*************************************************************************************************
	ReplaceString
*************************************************************************************************/

long Message::ReplaceString(const char *name, const char *str, long index)
{
	return ReplaceData(name,STRING_TYPE, str, (long)(strlen(str) + 1), index);
}

/*************************************************************************************************
	ReplaceWideString
*************************************************************************************************/

long Message::ReplaceWideString(const char *name, const wchar_t *str, long index)
{
	return ReplaceData(name,WIDE_STRING_TYPE, str, (long)(wcslen(str) + 1), index);
}

/*************************************************************************************************
	ReplaceChar
*************************************************************************************************/

long Message::ReplaceChar(const char *name, char val, long index )
{
	return ReplaceData(name, CHAR_TYPE, &val, sizeof(val), index);
}

/*************************************************************************************************
	ReplaceShort
*************************************************************************************************/

long Message::ReplaceShort(const char *name, short val, long index )
{
	return ReplaceData(name, SHORT_TYPE, &val, sizeof(val),index);
}

/*************************************************************************************************
	ReplaceLong
*************************************************************************************************/

long Message::ReplaceLong(const char *name, long val, long index )
{
	return ReplaceData(name, LONG_TYPE, &val, sizeof(val),index);
}

/*************************************************************************************************
	ReplaceBool
*************************************************************************************************/

long Message::ReplaceBool(const char *name, bool val, long index )
{
	return ReplaceData(name, BOOL_TYPE, &val, sizeof(val),index);
}

/*************************************************************************************************
	ReplaceFloat
*************************************************************************************************/

long Message::ReplaceFloat(const char *name, float val, long index )
{
	return ReplaceData(name, FLOAT_TYPE, &val, sizeof(val),index);
}

/*************************************************************************************************
	ReplaceDouble
*************************************************************************************************/

long Message::ReplaceDouble(const char *name, double val, long index )
{
	return ReplaceData(name, DOUBLE_TYPE, &val, sizeof(val),index);
}

/*************************************************************************************************
	ReplacePointer
*************************************************************************************************/

long Message::ReplacePointer(const char *name, const void *ptr, long index )
{
	return ReplaceData(name, POINTER_TYPE, &ptr, sizeof(ptr),index);
}

/*************************************************************************************************
	ReplaceMessage
*************************************************************************************************/

long Message::ReplaceMessage(const char *name, const Message *msg, long index )
{
	return ReplaceData(name, MESSAGE_TYPE, msg, sizeof(msg),index);
}

/*************************************************************************************************
	ReplaceMessage
*************************************************************************************************/
#ifdef USE_SCALAR
long Message::ReplaceScalar(const char* name, Scalar& val, long index)
{
	return ReplaceData(name, SCALAR_TYPE, &val, sizeof(val),index);
}
#endif
/*************************************************************************************************
	ReplaceData
*************************************************************************************************/

long Message::ReplaceData(const char *name, long type,
					const void *data, long size, long index )
{
	if(!mImp) return NAME_NOT_FOUND;

	long err = OK;
	string str(name);
	
	MessageHash::const_iterator it = mImp->hash.find(str);
	if(it != mImp->hash.end())
	{
		while(it != mImp->hash.begin())
		{
			--it;
			if(it->first != str)
			{
				++it;
				break;
			}
		}
		
		for(int i = 0; i < index && it != mImp->hash.end(); ++i)
			++it;
			
		if(it != mImp->hash.end() && it->first == str)
		{		
			if(it->second->type == type)
			{
				it->second->SetData(type,data,size);
			}else
				err = BAD_TYPE;
		}else
			err = BAD_INDEX;
	}else
		err = NAME_NOT_FOUND;
	
	return err;
}

/*************************************************************************************************
	Rename
*************************************************************************************************/

long Message::RemoveData(const char *name, long index )
{
	if(!mImp) return NAME_NOT_FOUND;

	long err = OK;
	string str(name);
	
	MessageHash::iterator it = mImp->hash.find(str);
	if(it != mImp->hash.end())
	{
		while(it != mImp->hash.begin())
		{
			--it;
			if(it->first != str)
			{
				++it;
				break;
			}
		}
	
		for(int i = 0; i < index && it != mImp->hash.end(); ++i)
			++it;
	
		if(it != mImp->hash.end() && it->first == str)
		{		
			delete it->second;
			mImp->hash.erase(it);
		}else
			err = BAD_INDEX;
	}else
		err = NAME_NOT_FOUND;
	
	return err;
}

/*************************************************************************************************
	Rename
*************************************************************************************************/

long Message::RemoveName(const char *name)
{
	if(!mImp) return NAME_NOT_FOUND;

	long err = OK;
	string str(name);
	
	MessageHash::iterator it = mImp->hash.find(str);
	if(it != mImp->hash.end())
	{
		MessageHash::iterator start = it;
		while(it != mImp->hash.end() && it->first == str)
		{
			delete it->second;
			++it;
		}

		mImp->hash.erase(start,it);
		
	/*	MessageHash::iterator end = it;
		while(it != mImp->hash.begin())
		{	
			--it;
			if( it->first != str)
			{
				++it;
				break;
			}
			delete it->second;
		}

		mImp->hash.erase(it,++end);*/
		
	}else
		err = NAME_NOT_FOUND;
	
	return err;
}

static int tab_level = 0;
static void tab();

/*************************************************************************************************
	Print
*************************************************************************************************/

void Message::Print() const
{
	long type, count;
	const char* key;
	
	tab_level++;
	
	tab();
	printf("what: %d\n", (int)what);
	
	for(int i = 0;; i++)
	{
		if(GetInfo(ANY_TYPE, i, &key, &type, &count) != OK) break;
		
		
		for(int j = 0; j < count; j++)
		{
			tab();
			printf("\"%s\": ", key);

			switch(type)
			{
				case MESSAGE_TYPE:
				{
					Message subMsg;
					FindMessage(key,&subMsg,j);
					subMsg.Print();
				}
				break;
				
				case BOOL_TYPE:
				{
					bool b;
					FindBool(key,&b,j);
					printf("%s\n",(b) ? "true" : "false");
				}
				break;
				
				case CHAR_TYPE:
				{
					char c;
					FindChar(key,&c,j);
					printf("%c\n", c);
				}
				break;
				
				case SHORT_TYPE:
				{
					short s;
					FindShort(key,&s,j);
					printf("%hd\n",s);
				}
				break;
				
				case LONG_TYPE:
				{
					long l;
					FindLong(key,&l,j);
					printf("%d\n",(int)l);
				}
				break;
				
				case FLOAT_TYPE:
				{
					float f;
					FindFloat(key,&f,j);
					printf("%f\n",f);
				}
				break;
				
				case DOUBLE_TYPE:
				{
					double d;
					FindDouble(key,&d,j);
					printf("%f\n",d);
				}
				break;
				
				case POINTER_TYPE:
				{	
					long ptr;
					FindPointer(key,(void**)&ptr,j);
					printf("0x%x\n",(int)ptr);
				}
				break;
				
				case STRING_TYPE:
				{
					const char* cstr = 0;
					FindString(key,&cstr,j);
					printf("\"%s\"\n",cstr);
				}
				break;
				
				default:
				{
					const void *data;
					long size;
					FindData(key, type, &data, &size, j);
					for(int k = 0; k < size; k++)
					{
						long byte = ((char*)data)[k];
						printf("0x%x ", (int)byte);
					}
					printf("\n");
				}
				break;
			}
		}
	}
	
	tab_level--;
}

/*************************************************************************************************
	tab
*************************************************************************************************/

static void tab()
{
	for(int i = 0; i < tab_level; i++)
		putchar('\t');
}

long Message::ReplaceOrAddString(const char *name, const char *str)
{
	if(ReplaceString(name,str) == phui::Message::NAME_NOT_FOUND)
		return AddString(name,str);
	
	return Message::OK;
}

long Message::ReplaceOrAddWideString(const char *name, const wchar_t *str)
{
	if(ReplaceWideString(name,str) == phui::Message::NAME_NOT_FOUND)
		return AddWideString(name,str);
	
	return Message::OK;
}

long Message::ReplaceOrAddChar(const char *name, char val)
{
	if(ReplaceChar(name,val) == phui::Message::NAME_NOT_FOUND)
		return AddChar(name,val);
	
	return Message::OK;
}

long Message::ReplaceOrAddShort(const char *name, short val)
{
	if(ReplaceShort(name,val) == phui::Message::NAME_NOT_FOUND)
		return AddShort(name,val);
	
	return Message::OK;
}

long Message::ReplaceOrAddLong(const char *name, long val)
{
	if(ReplaceLong(name,val) == phui::Message::NAME_NOT_FOUND)
		return AddLong(name,val);
	
	return Message::OK;
}

long Message::ReplaceOrAddBool(const char *name, bool val)
{
	if(ReplaceBool(name,val) == phui::Message::NAME_NOT_FOUND)
		return AddBool(name,val);
	
	return Message::OK;
}

long Message::ReplaceOrAddFloat(const char *name, float val)
{
	if(ReplaceFloat(name,val) == phui::Message::NAME_NOT_FOUND)
		return AddFloat(name,val);
	
	return Message::OK;
}

long Message::ReplaceOrAddDouble(const char *name, double val)
{
	if(ReplaceDouble(name,val) == phui::Message::NAME_NOT_FOUND)
		return AddDouble(name,val);
	
	return Message::OK;
}

long Message::ReplaceOrAddPointer(const char *name, const void *ptr)
{
	if(ReplacePointer(name,ptr) == phui::Message::NAME_NOT_FOUND)
		return AddPointer(name,ptr);
	
	return Message::OK;
}

long Message::ReplaceOrAddMessage(const char *name, const Message *msg)
{
	if(ReplaceMessage(name,msg) == phui::Message::NAME_NOT_FOUND)
		return AddMessage(name,msg);
	
	return Message::OK;
}

long Message::ReplaceOrAddScalar(const char* name, Scalar& val)
{
	if(ReplaceScalar(name,val) == phui::Message::NAME_NOT_FOUND)
		return AddScalar(name,val);
	
	return Message::OK;
}