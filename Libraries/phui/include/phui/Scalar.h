#ifndef PHUI_SCALAR_H
#define PHUI_SCALAR_H

#include <memory>
#include <string>
#include <wchar.h>

namespace phui
{
/************************************************************************************************
	Scalar
*************************************************************************************************/
/// Scary experiments in C++...
class Scalar
{
public:
	Scalar();
	~Scalar();
	Scalar(const Scalar&);
	Scalar(bool);
	Scalar(char);
	Scalar(short);
	Scalar(long);
	Scalar(int);
	Scalar(unsigned char);
	Scalar(unsigned short);
	Scalar(unsigned long);
	Scalar(unsigned int);
	Scalar(float);
	Scalar(double);
	Scalar(void*);
	Scalar(const char*);
	Scalar(const wchar_t*);
	
enum TypeT { UndefT, BoolT, CharT, ShortT, LongT, FloatT, DoubleT, PointerT, StringT, WideStringT };
	
	TypeT Type() const { return _type; }
	void Convert(TypeT);
	
	bool Bool() const;
	char Char() const;
	short Short() const;
	long Long() const;
	int Int() const;
	char UnsignedChar() const   { return (unsigned char)Char(); } 
	short UnsignedShort() const { return (unsigned short)Short(); }
	long UnsignedLong() const   { return (unsigned long)Long(); }
	int UnsignedInt() const		{ return (unsigned int)Long(); }
	float Float() const;
	double Double() const;
	void* Pointer() const;
	const char* String() const;
	const wchar_t* WideString() const;
	
	operator bool() const   { return Bool(); }
	operator char() const   { return Char(); }
	operator short() const  { return Short(); }
	operator long() const   { return Long(); }
	operator int() const	{ return (int)Long(); }
	operator unsigned char() const  { return (unsigned char)Char(); }
	operator unsigned short() const { return (unsigned short)Short(); }
	operator unsigned long() const  { return (unsigned long)Long(); }
	operator unsigned int() const   { return (unsigned int)Long(); }
	operator float() const		{ return Float(); }
	operator double() const		{ return Double(); }
	operator void*() const		{ return Pointer(); }
	operator const char*() const		{ return String(); }
	operator const wchar_t*() const		{ return WideString(); }
	
	Scalar& Set(bool);
	Scalar& Set(char);
	Scalar& Set(short);
	Scalar& Set(long);
	Scalar& Set(int);
	Scalar& Set(float);
	Scalar& Set(double);
	Scalar& Set(const void*);
	Scalar& Set(const char*);
	Scalar& Set(const wchar_t*);

	Scalar& operator=(const Scalar& v);
	Scalar& operator=(bool b)	{ return Set(b); }
	Scalar& operator=(char c)	{ return Set(c); }
	Scalar& operator=(short s)   { return Set(s); }
	Scalar& operator=(long l)	{ return Set(l); }
	Scalar& operator=(int i)		{ return Set((long)i); }
	Scalar& operator=(unsigned char c)	{ return Set((char)c); }
	Scalar& operator=(unsigned short s)  { return Set((short)s); }
	Scalar& operator=(unsigned long l)	{ return Set((long)l); }
	Scalar& operator=(unsigned int i)	{ return Set((long)i); }	
	Scalar& operator=(float f)   { return Set(f); }
	Scalar& operator=(double d)  { return Set(d); }
	Scalar& operator=(const void* p)		{ return Set(p); }
	Scalar& operator=(const char* str)		{ return Set(str); }
	Scalar& operator=(const wchar_t* wstr)  { return Set(wstr); }

	void ToString(const char* format, ...) const;
	void ToString(const wchar_t* format, ...) const;
	void Reset();
	
private:
union value_union
{
	bool b;
	char c;
	short s;
	long l;
	float f;
	double d;
	const void* p;
};
	TypeT _type;
	mutable value_union _value;
	mutable std::string _str;
	mutable std::wstring _wstr;
};

}
#endif // PHUI_VALUE_H
