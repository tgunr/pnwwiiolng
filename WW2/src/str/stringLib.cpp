#include "syscfg.h" // Path is project specific.
// Created: Fri Mar 31 16:26:10 2000
//
// The following file is a trimmed down version of several files
// I got from:
//
// http://chesworth.com/pv/resources/snippets.htm#section1group5
//
// All functions used were declared to be public domain and therefore free
// for my bogarting
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include <ctype.h>
#include "str/stringLib.h"
#include <string.h>

#if platOS != OS_WIN32

//Turns a string into lowercase
char* strlwr(char* string)
{
      char* s;

      if (string)
      {
            for (s = string; *s; ++s)
                  *s = tolower(*s);
      }
      return string;
}

//Turns a string into lowercase
char* strupr(char* string)
{
      char* s;

      if (string)
      {
            for (s = string; *s; ++s)
                  *s = toupper(*s);
      }
      return string;
}
#endif

//Removes leading whitespace from a string
//Removes leading whitespace from a string
char* rmlead(char* str)
{

  if (str)
  {
	while( isspace(*str))
	{
		str++;
	}
  }

  return str;
}

//Removes trailing whitespace from a string
char* rmtrail(char* str)
{
	if(!str || *str == 0)
		return str;

	// Find the last character of the string (not terminator).
	char* endStr = str + strlen(str) - 1;

	// Find the last non-space character but don't overshoot the start of the string.
	char* p = endStr;
	while(p > str && isspace(*p))
		--p;

	// If p was decremented, replace the prior character (a whitespace) with terminator.
	if ( p < endStr )
		*(p+1) = 0;

	return str;
}

char* strescape(char* d,const char* s, char c)
{
	char* curr = d;
	while (*s != '\0')
	{
		if (*s == c)
			*(curr++) = '\\';
		*(curr++) = *(s++);
	}
	*curr = '\0';
	return d;
}

std::string string_replace(const std::string &haystack, const std::string &needle, const std::string &replacement)
{
    size_t pos = 0;
    std::string result = haystack;

    while((pos = result.find(needle, pos)) != std::string::npos)
    {
        result.replace(pos, needle.length(), replacement);
        pos += replacement.length();
    }
    return result;
}

std::vector<std::string> split_string(const std::string &str, const std::string &delimiter, size_t max_split)
{
    std::vector<std::string> strings;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos && strings.size() < (max_split == 0 ? 0 : max_split - 1))
    {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + 1;
    }
    strings.push_back(str.substr(prev));
    return strings;
}


bool has_prefix(const std::string &str, const std::string &prefix)
{
    return str.compare(0, prefix.size(), prefix) == 0;
}

std::string strtolower(const std::string &str)
{
    std::string result = str;
    for (char &c : result) { c = static_cast<char>(tolower(c)); }
    return result;
}

std::string strtoupper(const std::string &str)
{
    std::string result = str;
    for (char &c : result) { c = static_cast<char>(toupper(c)); }
    return result;
}

std::string strtrim(const std::string &str)
{
    size_t snwspos = str.find_first_not_of(" \t\n\v\f\r");

    if (snwspos == std::string::npos) {
        return "";
    }

    size_t enwspos = str.find_last_not_of(" \t\n\v\f\r");

    if (enwspos == std::string::npos) {
        return "";
    }

    return str.substr(snwspos, (enwspos + 1) - snwspos);
}

#if platOS == OS_WIN32
int vasprintf(char **result, const char *fmt, va_list args)
{
    size_t len = static_cast<size_t>(_vscprintf(fmt, args));
    if (len >= INT_MAX) { return -1; }
    *result = reinterpret_cast<char *>(calloc(1, len + 1));
    vsnprintf_s(*result, len + 1, _TRUNCATE, fmt, args);
    return (int)len;
}
#endif

#if platOS == OS_LINUX
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

std::string Base64Encode(void const *buffer, size_t bufferLength)
{
    BIO *base64_filter = BIO_new(BIO_f_base64());
    BIO_set_flags(base64_filter, BIO_FLAGS_BASE64_NO_NL);

    BIO *buffer_bio = BIO_new(BIO_s_mem());

    BIO_push(base64_filter, buffer_bio);
    BIO_write(base64_filter, buffer, bufferLength);
    BIO_flush(base64_filter);

    char *data = nullptr;
    long length = BIO_get_mem_data(buffer_bio, &data);
    std::string result(data, static_cast<size_t>(length));

    BIO_free_all(base64_filter);
    return result;
}

bool Base64Decode(const std::string &input, void *outputBuffer, size_t *ioBufferLength)
{
    BIO *buffer_bio = BIO_new(BIO_s_mem());
    BIO_write(buffer_bio, input.data(), input.length());

    BIO *base64_filter = BIO_new(BIO_f_base64());
    BIO_push(base64_filter, buffer_bio);
    BIO_set_flags(base64_filter, BIO_FLAGS_BASE64_NO_NL);

    int readAmount = BIO_read(base64_filter, outputBuffer, static_cast<int>(*ioBufferLength));

    BIO_free_all(base64_filter);

    if (readAmount > 0) {
        *ioBufferLength = static_cast<size_t>(readAmount);
        return true;
    } else {
        return false;
    }
}

#elif platOS == OS_WIN32

#pragma comment(lib, "Crypt32.lib")

std::string Base64Encode(void const *buffer, size_t bufferLength)
{
    DWORD neededLen = 0;

    if (CryptBinaryToStringA(
            reinterpret_cast<const BYTE *>(buffer), bufferLength,
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
            nullptr, &neededLen
    )) {
        std::string str(neededLen, 0);

        CryptBinaryToStringA(
            reinterpret_cast<const BYTE *>(buffer), bufferLength,
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
            const_cast<LPSTR>(str.data()), &neededLen);
        return str;
    } else {
        return "";
    }
}

bool Base64Decode(const std::string &input, void *outputBuffer, size_t *ioBufferLength)
{
    DWORD localLength = static_cast<DWORD>(*ioBufferLength);
    if (CryptStringToBinaryA(
        input.data(), 0, CRYPT_STRING_BASE64,
        reinterpret_cast<BYTE *>(outputBuffer), &localLength, nullptr, nullptr
    )) {
        *ioBufferLength = static_cast<size_t>(localLength);
        return true;
    }
    return false;
}

#elif platOS == OS_MACOS

// macOS implementation is kept in sysMacCompat.mm because it requires
// Objective-C++ and we can't use any flavor of Objective-C in this file.

#endif
