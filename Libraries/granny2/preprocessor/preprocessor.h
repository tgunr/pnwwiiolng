#if !defined(PREPROCESSOR_H)
// ========================================================================
// $File: //jeffr/granny/preprocessor/preprocessor.h $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #16 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny.h"
#include <stdio.h>

// ------------------------------------------------------------
// Structures to hold arguments to the commands
struct input_file
{
    granny_file* GrannyFile;
    char*        Filename;   // as passed to the command line, may
                             // contain path elements
};

struct key_value_pair
{
    char* Key;
    char* Value;
};

// ------------------------------------------------------------
// Function signature for processing functions
enum ECommandType
{
    eMultiFile,
    eSingleFile_NoMultiRun,
    eSingleFile_MultiRun,
    eEmptyStart,
};

typedef bool (*CommandMulti)(input_file*     InputFiles,
                             granny_int32x   NumInputFiles,
                             key_value_pair* KeyValuePairs,
                             granny_int32x   NumKeyValuePairs,
                             granny_memory_arena* TempArena);

typedef bool (*CommandSingle)(input_file&     InputFile,
                              key_value_pair* KeyValuePairs,
                              granny_int32x   NumKeyValuePairs,
                              granny_memory_arena* TempArena);

typedef bool (*CommandEmpty)(key_value_pair* KeyValuePairs,
                             granny_int32x   NumKeyValuePairs,
                             granny_memory_arena* TempArena);

// Error output helpers
#define ErrOut(...) fprintf(stderr, __FUNCTION__ ": " __VA_ARGS__)
#define WarnOut(...) fprintf(stdout, __FUNCTION__ "(warning): " __VA_ARGS__)

// ------------------------------------------------------------
// Rename these to prevent collisions when compiling the preprocessor into the unit
// testing framework.
//
#define PushObject(Arena, Type)                         \
    ((Type*)GrannyMemoryArenaPush(Arena, sizeof(Type)))

#define PushArray(Arena, Count, Type)                               \
    ((Type*)GrannyMemoryArenaPush(Arena, sizeof(Type) * (Count)))

#define PushString(Arena, String)                                   \
    GrannyMemoryArenaPushString(Arena, String)


// ------------------------------------------------------------
// Helper structure to register a preprocessor command.  Use as:
//  static CommandRegistrar RegMyCommand(eSingleFile_MultiRun,  /* or eMultiFile */
//                                       MyCommand,
//                                       "CommandString",
//                                       "ShortDocString");
// at /file/ level, where MyCommand is a pointer to your function.
class CommandRegistrar
{
public:

public:
    CommandRegistrar(CommandMulti NewCommand,
                     char const*  CommandName,
                     char const*  ShortDocString);
    CommandRegistrar(CommandEmpty NewCommand,
                     char const*  CommandName,
                     char const*  ShortDocString);
    CommandRegistrar(ECommandType  CommandType,
                     CommandSingle NewCommand,
                     char const*   CommandName,
                     char const*   ShortDocString);

    // disallow standard c++ fns
private:
    CommandRegistrar();
    CommandRegistrar(CommandRegistrar const&);
    void operator=(CommandRegistrar const&);
};


#define RequireKey(keyname, ...)                                        \
    {                                                                   \
        if (!FindFirstValueForKey(KeyValues, NumKeyValues, keyname))    \
        {                                                               \
            ErrOut(__VA_ARGS__);                                        \
            return false;                                               \
        }                                                               \
    } typedef int __require_semicolon


bool SimpleWriteInfo(key_value_pair* KeyValuePairs,
                     granny_int32x   NumKeyValuePairs,
                     granny_file_info* Info);
bool WriteInfoPreserve(key_value_pair* KeyValuePairs,
                       granny_int32x   NumKeyValuePairs,
                       granny_file_info* Info,
                       granny_file* OriginalFile,
                       bool PreservePlatform,
                       bool PreserveObjectSections,
                       bool PreserveSectionFormats);

// Utility function for searching K/V pairs.  Returns NULL for missing
// key/value
char const* FindFirstValueForKey(key_value_pair* KeyValuePairs,
                                 granny_int32x   NumKeyValuePairs,
                                 char const*     Key);




// Macosx has strcasecmp rather than stricmp
#ifdef _MACOSX
#define _stricmp strcasecmp
#endif


#define PREPROCESSOR_H
#endif
