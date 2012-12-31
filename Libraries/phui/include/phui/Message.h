#pragma once

#define USE_SCALAR

#ifdef USE_SCALAR
#include "phui/Scalar.h"
#endif

#include <vector>

namespace phui
{
/*************************************************************************************************
	Message
*************************************************************************************************/
/// Core Message object.
class Message
{
public:

	typedef std::vector<std::string> KeyList;

	long what;

	Message(long what);
	Message();
	Message(const Message&);
	~Message();

	Message& operator = (const Message&);

	long AddBool(const char* name, bool val);
	long AddChar(const char* name, char val);
	long AddShort(const char* name, short val);
	long AddLong(const char* name, long val);
	long AddFloat(const char* name, float val);
	long AddDouble(const char* name, double val);	
	long AddString(const char* name, const char* str);
	long AddWideString(const char* name, const wchar_t* str);
	long AddPointer(const char* name, const void* ptr);
	long AddMessage(const char* name, const Message*);
	long AddData(const char* name, long type, const void* data, long size);

	long FindBool(const char* name, bool* val, long index = 0) const;
	long FindChar(const char* name, char* val, long index = 0) const;
	long FindShort(const char* name, short* val,long index = 0) const;
	long FindLong(const char* name, long* val, long index = 0) const;
	long FindFloat(const char* name, float* val, long index = 0) const;
	long FindDouble(const char* name, double* val, long index = 0) const;
	long FindString(const char* name, const char** str, long index = 0) const;
	long FindWideString(const char* name, const wchar_t** str, long index = 0) const;
	long FindPointer(const char* name, void** ptr, long index = 0) const;
	long FindMessage(const char* name, Message*, long index = 0) const;
	long FindData(const char* name, long type, const void **data, long* size, long index = 0) const;
	
	Message::KeyList FindKeys(const char* preName) const;
	
	void MakeEmpty();
	bool IsEmpty();

	long GetInfo(const char* name, long* typeFound, long* countFound) const;
	long GetInfo(long type, long index, const char** nameFound, long* typeFound, long* countFound) const; ///< Pass ANY_TYPE to iterate through all data in the messsage.

	long ReplaceString(const char *name, const char *str, long index = 0);
	long ReplaceWideString(const char *name, const wchar_t *str, long index = 0);
	long ReplaceChar(const char *name, char val, long index = 0);
	long ReplaceShort(const char *name, short val, long index = 0);
	long ReplaceLong(const char *name, long val, long index = 0);
	long ReplaceBool(const char *name, bool val, long index = 0);
	long ReplaceFloat(const char *name, float val, long index = 0);
	long ReplaceDouble(const char *name, double val, long index = 0);
	long ReplacePointer(const char *name, const void *ptr, long index = 0);
	long ReplaceMessage(const char *name, const Message *msg, long index = 0);
	long ReplaceData(const char *name, long type, const void *data, long size, long index = 0);

	long ReplaceOrAddString(const char *name, const char *str);
	long ReplaceOrAddWideString(const char *name, const wchar_t *str);
	long ReplaceOrAddChar(const char *name, char val);
	long ReplaceOrAddShort(const char *name, short val);
	long ReplaceOrAddLong(const char *name, long val);
	long ReplaceOrAddBool(const char *name, bool val);
	long ReplaceOrAddFloat(const char *name, float val);
	long ReplaceOrAddDouble(const char *name, double val);
	long ReplaceOrAddPointer(const char *name, const void *ptr);
	long ReplaceOrAddMessage(const char *name, const Message *msg);

	long RemoveData(const char *name, long index = 0);
	long RemoveName(const char *name);

	void Print() const;

#ifdef USE_SCALAR
	long AddScalar(const char* name, Scalar const&);
	long FindScalar(const char* name, Scalar*, long index = 0) const;
	long ReplaceScalar(const char* name, Scalar&, long index = 0);
	long ReplaceOrAddScalar(const char* name, Scalar&);
#endif	

// built-in data types
enum
{
	SCALAR_TYPE		= 'Sclr',
	BOOL_TYPE		= 'Bool',
	CHAR_TYPE		= 'Char',
	SHORT_TYPE		= 'Shrt',
	LONG_TYPE		= 'Long',
	FLOAT_TYPE		= 'Flot',
	DOUBLE_TYPE 	= 'Doub',
	POINTER_TYPE	= 'Ptr ',
	STRING_TYPE 	= 'Str ',
	WIDE_STRING_TYPE= 'WStr',
	MESSAGE_TYPE 	= 'Msg ',
	ANY_TYPE		= 'Any '
};

// Error codes returned by Message
enum
{
	OK = 0,
	NAME_NOT_FOUND,
	BAD_TYPE,
	BAD_INDEX
};

private:
	struct MessageImp;
	MessageImp* mImp;
};

}
