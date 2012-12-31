#include "phui-precompiled-header.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "phui/Scalar.h"
#include "phui/Util.h"

#ifdef _WIN32
#define wcstof(x,y) _wtof(x)
#define snprintf _snprintf
#endif

#define SCALAR_STR_SIZE 50

using namespace phui;

/************************************************************************************************
	Scalar
*************************************************************************************************/

Scalar::Scalar()
{
	_type = UndefT;
	_value.l = 0;
}

Scalar::~Scalar()
{
}

Scalar::Scalar(const Scalar& v)
{
	*this = v;
}

Scalar& Scalar::operator=(const Scalar& v) 
{ 
	// special case of WideStringT and StringT
	if(_type == WideStringT && v._type == StringT)
	{
		Set(v._str.c_str());
	}
	else
	{
		_type = v._type; 
		
		Reset();
		
		switch(_type)
		{ 
			case StringT:
				_str = v._str;
			break;
			case WideStringT:
				_wstr = v._wstr;
			break;
			default:
				_value = v._value;
			break;
		}
	}
	
	return *this; 
}

Scalar::Scalar(bool b)
{
	_value.b = b;
	_type = BoolT;
}

Scalar::Scalar(char c)
{
	_value.c = c;
	_type = CharT;
}

Scalar::Scalar(short s)
{
	_value.s = s;
	_type = ShortT;
}

Scalar::Scalar(long l)
{
	_value.l = l;
	_type = LongT;
}

Scalar::Scalar(int i)
{
	_value.l = (long)i;
	_type = LongT;
}
Scalar::Scalar(unsigned char c)
{
	_value.c = (char)c;
	_type = CharT;
}

Scalar::Scalar(unsigned short s)
{
	_value.s = (short)s;
	_type = ShortT;
}

Scalar::Scalar(unsigned long l)
{
	_value.l = (long)l;
	_type = LongT;
}

Scalar::Scalar(unsigned int i)
{
	_value.l = (long)i;
	_type = LongT;
}

Scalar::Scalar(float f)
{
	_value.f = f;
	_type = FloatT;
}

Scalar::Scalar(double d)
{
	_value.d = d;
	_type = DoubleT;
}

Scalar::Scalar(void* p)
{
	_value.p = p;
	_type = PointerT;
}

Scalar::Scalar(const char* str)
{
	_type = StringT;
	_str = str;
}

Scalar::Scalar(const wchar_t* wstr)
{
	_type = WideStringT;
	_wstr = wstr;
}

/************************************************************************************************
	Convert
*************************************************************************************************/

void Scalar::Convert(TypeT type)
{
	switch(type)
	{
		case UndefT:
		break;
		
		case BoolT:
			_value.b = Bool();
		break;

		case CharT:
			_value.c = Char();
		break;
		
		case ShortT:
			_value.s = Short();
		break;

		case LongT:
			_value.l = Long();
		break;
		
		case FloatT:
			_value.f = Float();
		break;
		
		case DoubleT:
			_value.d = Double();
		break;
		
		case PointerT:
			_value.p = Pointer();
		break;
		
		case StringT:
			String();
		break;
		
		case WideStringT:
			WideString();
		break;
	}
	
	_type = type;
}

/************************************************************************************************
	Get
*************************************************************************************************/

bool Scalar::Bool() const
{
	bool b;
	
	switch(_type)
	{
		case BoolT:
			b = _value.b;
		break;

		case CharT:
			b = (_value.c != 0);
		break;
		
		case ShortT:
			b = (_value.s != 0);
		break;

		case LongT:
			b = (_value.l != 0);
		break;
		
		case FloatT:
			b = (_value.f != 0.0f);
		break;
		
		case DoubleT:
			b = (_value.d != 0.0);
		break;
		
		case PointerT:
			b = (_value.p != NULL);
		break;
		
		case StringT:
			if(!strcmp(_str.c_str(), "false") || !strcmp(_str.c_str(), "0"))
				b = false;
			else
				b = true;
		break;
		
		case WideStringT:
			if(!wcscmp(_wstr.c_str(), L"false") || !wcscmp(_wstr.c_str(), L"0"))
				b = false;
			else
				b = true;
		break;

		case UndefT:
		default:
			b = false;
		break;
	}
	return b;
}

char Scalar::Char() const
{
	char c;
	switch(_type)
	{
		case BoolT:
			c = (_value.b) ? '1' : '0';
		break;

		case CharT:
			c = _value.c;
		break;
		
		case ShortT:
			c = (char)_value.s;
		break;

		case LongT:
			c = (char)_value.l;
		break;
		
		case FloatT:
			c = (char)_value.f;
		break;
		
		case DoubleT:
			c = (char)_value.d;
		break;
		
		case PointerT:
			c = (char)(size_t)_value.p;
		break;
		
		case StringT:
			c = _str.c_str()[0];
		break;
		
		case WideStringT:
			c = (char)_wstr.c_str()[0];
		break;

		case UndefT:
		default:
			c = 0;
		break;
	}
	return c;
}

short Scalar::Short() const
{
	return (short)Long();
}

long Scalar::Long() const
{
	long l;
	switch(_type)
	{
		case BoolT:
			l = (_value.b) ? 1 : 0;
		break;

		case CharT:
			l = (long)_value.c;
		break;
		
		case ShortT:
			l = (long)_value.s;
		break;

		case LongT:
			l = _value.l;
		break;
		
		case FloatT:
			l = (long)_value.f;
		break;
		
		case DoubleT:
			l = (long)_value.d;
		break;
		
		case PointerT:
			l = (long)_value.p;
		break;
		
		case StringT:
			l = (long)atoi(_str.c_str());
		break;
		
		case WideStringT:
			l = (long)wcstol(_wstr.c_str(),NULL,10);
		break;

		default:
		case UndefT:
			l = 0;
		break;
	}
	return l;
}

int Scalar::Int() const
{
	return (int)Long();
}

float Scalar::Float() const
{
	return (float)Double();
}

double Scalar::Double() const
{
	double d;
	switch(_type)
	{
		case BoolT:
			d = (_value.b) ? 1.0 : 0.0;
		break;

		case CharT:
			d = (double)_value.c;
		break;
		
		case ShortT:
			d = (double)_value.s;
		break;

		case LongT:
			d = (double)_value.l;
		break;
		
		case FloatT:
			d = (double)_value.f;
		break;
		
		case DoubleT:
			d = _value.d;
		break;
		
		case PointerT:
			d = (double)(size_t)_value.p;
		break;
		
		case StringT:
			d = (double)atof(_str.c_str());
		break;
		
		case WideStringT:
			d = (double)wcstof(_wstr.c_str(),NULL);
		break;

		default:
		case UndefT:
			d = 0.0;
		break;
	}
	return d;
}

void* Scalar::Pointer() const
{
	return (void*)Long();
}

const char* Scalar::String() const
{
	if(_str.empty())
	{
		switch(_type)
		{
			case BoolT:
				_str = (_value.b) ? "true" : "false";
			break;

			case CharT:
				ToString("%c", _value.c);
			break;
			
			case ShortT:
				ToString("%hd", _value.s);
			break;

			case LongT:
				ToString("%d", (int)_value.l);
			break;
			
			case FloatT:
				ToString("%f", (int)_value.f);
			break;
			
			case DoubleT:
				ToString("%lf", (int)_value.d);
			break;
			
			case PointerT:
				ToString("%d", (int)_value.p);
			break;
			
			case UndefT:
			case StringT:
				_str = "";
			break;
			
			case WideStringT:
				_str = (const char*)utf16_to_utf8(_wstr.c_str());
			break;
		}
	}
	
	return _str.c_str();	
}

const wchar_t* Scalar::WideString() const
{   
	if(_wstr.c_str() == 0)
	{
		switch(_type)
		{
			case BoolT:
				_wstr = (_value.b) ? L"true" : L"false";
			break;

			case CharT:
				ToString(L"%C", _value.c);
			break;
			
			case ShortT:
				ToString(L"%hd", _value.s);
			break;

			case LongT:
				ToString(L"%d", (int)_value.l);
			break;
			
			case FloatT:
				ToString(L"%f", (int)_value.f);
			break;
			
			case DoubleT:
				ToString(L"%lf", (int)_value.d);
			break;
			
			case PointerT:
				ToString(L"%d", (int)_value.p);
			break;
			
			case StringT:
				_wstr = utf8_to_utf16((const unsigned char*)_str.c_str());
			break;
			
			case WideStringT:
			case UndefT:
				_wstr = L"";
			break;
		}
	}
	
	return _wstr.c_str();
}

/************************************************************************************************
	Set
*************************************************************************************************/

Scalar& Scalar::Set(bool b)
{
	Reset();
	
	if(_type == UndefT) _type = BoolT;
		
	switch(_type)
	{
		case UndefT:
		break;
		
		case BoolT:
			_value.b = b;
		break;

		case CharT:
			_value.c = (b) ? '1' : '0';
		break;
		
		case ShortT:
			_value.s = (short)b;
		break;

		case LongT:
			_value.l = (long)b;
		break;
		
		case FloatT:
			_value.f = (float)b;
		break;
		
		case DoubleT:
			_value.d = (double)b;
		break;
		
		case PointerT:
			_value.p = (void*)(size_t)b;
		break;
		
		case StringT:
			_str = (_value.b) ? "true" : "false";
		break;
		
		case WideStringT:
			_wstr = (_value.b) ? L"true" : L"false";
		break;
	}
	
	return *this;
}

Scalar& Scalar::Set(char c)
{
	Reset();
	
	if(_type == UndefT) _type = CharT;
		
	switch(_type)
	{
		case UndefT:
		break;
		
		case BoolT:
			_value.b = (bool)c;
		break;

		case CharT:
			_value.c = c;
		break;
		
		case ShortT:
			_value.s = (short)c;
		break;

		case LongT:
			_value.l = (long)c;
		break;
		
		case FloatT:
			_value.f = (float)c;
		break;
		
		case DoubleT:
			_value.d = (double)c;
		break;
		
		case PointerT:
			_value.p = (void*)(size_t)c;
		break;
		
		case StringT:
			ToString(L"%c", c);
		break;
		
		case WideStringT:
			ToString(L"%C", c);
		break;
	}
	
	return *this;
}

Scalar& Scalar::Set(short s)
{
	Reset();

	if(_type == UndefT) _type = ShortT;

	switch(_type)
	{
		case UndefT:
		break;
		
		case BoolT:
			_value.b = (bool)s;
		break;

		case CharT:
			_value.c = (char)s;
		break;
		
		case ShortT:
			_value.s = (short)s;
		break;

		case LongT:
			_value.l = (long)s;
		break;
		
		case FloatT:
			_value.f = (float)s;
		break;
		
		case DoubleT:
			_value.d = (double)s;
		break;
		
		case PointerT:
			_value.p = (void*)(size_t)s;
		break;
		
		case StringT:
			ToString("%hd", s);
		break;
		
		case WideStringT:
			ToString(L"%hd", s);
		break;
	}
	
	return *this;
}

Scalar& Scalar::Set(long l)
{
	Reset();

	if(_type == UndefT) _type = LongT;

	switch(_type)
	{
		case UndefT:
		break;
		
		case BoolT:
			_value.b = (bool)l;
		break;

		case CharT:
			_value.c = (char)l;
		break;
		
		case ShortT:
			_value.s = (short)l;
		break;

		case LongT:
			_value.l = (long)l;
		break;
		
		case FloatT:
			_value.f = (float)l;
		break;
		
		case DoubleT:
			_value.d = (double)l;
		break;
		
		case PointerT:
			_value.p = (void*)(size_t)l;
		break;
		
		case StringT:
			ToString("%d", (int)l);
		break;
		
		case WideStringT:
			ToString(L"%d", (int)l);
		break;
	}
	
	return *this;
}

Scalar& Scalar::Set(int i)
{
	return Set((long)i);
}

Scalar& Scalar::Set(float f)
{
	Reset();

	if(_type == UndefT) _type = FloatT;

	switch(_type)
	{
		case UndefT:
		break;
		
		case BoolT:
			_value.b = (bool)f;
		break;

		case CharT:
			_value.c = (char)f;
		break;
		
		case ShortT:
			_value.s = (short)f;
		break;

		case LongT:
			_value.l = (long)f;
		break;
		
		case FloatT:
			_value.f = (float)f;
		break;
		
		case DoubleT:
			_value.d = (double)f;
		break;
		
		case PointerT:
			_value.p = (void*)(size_t)f;
		break;
		
		case StringT:
			ToString("%f", f);
		break;
		
		case WideStringT:
			ToString(L"%f", f);
		break;
	}
	
	return *this;
}

Scalar& Scalar::Set(double d)
{
	Reset();

	if(_type == UndefT) _type = DoubleT;

	switch(_type)
	{
		case UndefT:
		break;
		
		case BoolT:
			_value.b = (bool)d;
		break;

		case CharT:
			_value.c = (char)d;
		break;
		
		case ShortT:
			_value.s = (short)d;
		break;

		case LongT:
			_value.l = (long)d;
		break;
		
		case FloatT:
			_value.f = (float)d;
		break;
		
		case DoubleT:
			_value.d = (double)d;
		break;
		
		case PointerT:
			_value.p = (void*)(size_t)d;
		break;
		
		case StringT:
			ToString("%lf", d);
		break;
		
		case WideStringT:
			ToString(L"%lf", d);
		break;
	}
	
	return *this;
}

Scalar& Scalar::Set(const void* p)
{
	Reset();

	if(_type == UndefT) _type = PointerT;

	switch(_type)
	{
		case UndefT:
		break;
		
		case BoolT:
			_value.b = (bool)p;
		break;

		case CharT:
			_value.c = (char)(size_t)p;
		break;
		
		case ShortT:
			_value.s = (short)(size_t)p;
		break;

		case LongT:
			_value.l = (long)p;
		break;
		
		case FloatT:
			_value.f = (float)(size_t)p;
		break;
		
		case DoubleT:
			_value.d = (double)(size_t)p;
		break;
		
		case PointerT:
			_value.p = p;
		break;
		
		case StringT:
			ToString("%d", (int)p);
		break;
		
		case WideStringT:
			ToString(L"%d", (int)p);
		break;
	}
	
	return *this;
}

Scalar& Scalar::Set(const char* str)
{
	Reset();

	if(_type == UndefT) _type = StringT;

	switch(_type)
	{
		case UndefT:
		break;
		
		case BoolT:
			if(!strcmp(str, "false") || !strcmp(str, "0"))
				_value.b = false;
			else
				_value.b = true;
		break;

		case CharT:
			_value.c = str[0];
		break;
		
		case ShortT:
			_value.s = (short)atoi(str);
		break;

		case LongT:
			_value.l = (long)atoi(str);
		break;
		
		case FloatT:
			_value.f = (float)atof(str);
		break;
		
		case DoubleT:
			_value.d = (double)atof(str);
		break;
		
		case PointerT:
			_value.p = (void*)atoi(str);
		break;
		
		case StringT:
		{
			_str = str;
		}
		break;
		
		case WideStringT:
			_wstr = utf8_to_utf16((const unsigned char*)str);
		break;
	}
	
	return *this;
}


Scalar& Scalar::Set(const wchar_t* wstr)
{
	Reset();

	if(_type == UndefT) _type = WideStringT;

	switch(_type)
	{
		case UndefT:
		break;
		
		case BoolT:
			if(!wcscmp(wstr, L"false") || !wcscmp(wstr, L"0"))
				_value.b = false;
			else
				_value.b = true;
		break;

		case CharT:
			_value.c = (char)wstr[0];
		break;
		
		case ShortT:
			_value.s = (short)wcstol(wstr,NULL,10);
		break;

		case LongT:
			_value.l = (long)wcstol(wstr,NULL,10);
		break;
		
		case FloatT:
			_value.f = (float)wcstol(wstr,NULL,10);
		break;
		
		case DoubleT:
			_value.d = (double)wcstol(wstr,NULL,10);
		break;
		
		case PointerT:
			_value.p = (void*)wcstol(wstr,NULL,10);
		break;
		
		case StringT:
			_str = (const char*)utf16_to_utf8(wstr);
		break;
		
		case WideStringT:
			_wstr = wstr;
		break;
	}
	
	return *this;
}

void Scalar::Reset()
{
	_str = "";
	_wstr = L"";
}

inline void Scalar::ToString(const char* format, ...) const
{
	char buffer[SCALAR_STR_SIZE];
	buffer[0] = 0;

	va_list marker;
	va_start(marker, format);

	// since we're passing in a char* string, we'll do our own varargs formatting
	int len = vsnprintf(buffer, SCALAR_STR_SIZE-1, format, marker); 

	va_end(marker);

	_str = buffer;
}

inline void Scalar::ToString(const wchar_t* format, ...) const
{
	wchar_t buffer[SCALAR_STR_SIZE];
	buffer[0] = 0;

	va_list marker;
	va_start(marker, format);

	// since we're passing in a char* string, we'll do our own varargs formatting
	int len = vswprintf(buffer, SCALAR_STR_SIZE-1, format, marker); 

	va_end(marker);

	_wstr = buffer;
}