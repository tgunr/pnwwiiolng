#ifndef _LOAD_H_
#define _LOAD_H_

#include <string>
#include "Tchar.h"
#include "../src/language/language.h"

enum
{
    TYPE_BOOL,
    TYPE_LONG,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING
};

typedef struct _VALUE
{
    union
    {
        bool b;
        long l;
        float f;
        double d;
    };

    std::wstring str;

    void set(bool value) 
    {
        b = value;
        str.clear();
        str = b ? _T("true") : _T("false");
    }
    
    void set(long value) 
    {
        wchar_t buff[65];
        l = value;
        str.clear();
        swprintf_s(buff, 64, _T("%d"), l);
        str = buff;
    }

    void set(int value) 
    {
        wchar_t buff[65];
        l = value;
        str.clear();
        swprintf_s(buff, 64, _T("%d"), l);
        str = buff;
    }

    void set(float value)
    {
        wchar_t buff[65];
        f = value;
        str.clear();
        swprintf_s(buff, 64, _T("%f"), f);
        str = buff;
    }

    void set(double value) 
    {
        wchar_t buff[65];
        d = value;
        str.clear();
        swprintf_s(buff, 64, _T("%f"), d);
        str = buff;
    }
    
    void set(const char *s) 
    {
        str.clear(); 
        str = utf8_to_utf16(s);
    }

    void set(const wchar_t *s) 
    {
        str.clear(); 
        str = s;
    }

    void get(bool& value)		{ value = b; }
    void get(long& value)		{ value = l; }
    void get(int& value)		{ value = l; }
    void get(float& value)		{ value = f; }
    void get(double& value)		{ value = d; }
    void get(const char **s)	{ *s = (const char*)utf16_to_utf8(str.c_str()); }
	void get(const wchar_t **s) { *s = str.c_str();}
    void get(std::string& s)	{ s = (const char*)utf16_to_utf8(str.c_str()); }
	void get(std::wstring& s)	{ s = str; }
    
    // Return values in string format for output
    const unsigned char *get()	{ return utf16_to_utf8(str.c_str()); }

} VALUE;

typedef struct _WWIIOLXML_
{
    std::string name;
    std::string type;

    VALUE value;
} WWIIOLXML;

bool loadPreferences();

void loadKill();
void loadSave();

VALUE *getValue(const char *key);
int region();

#endif