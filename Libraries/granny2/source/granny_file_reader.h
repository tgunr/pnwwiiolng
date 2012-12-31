#if !defined(GRANNY_FILE_READER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_file_reader.h $
// $DateTime: 2009/06/23 13:30:11 $
// $Change: 22123 $
// $Revision: #9 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(FileReaderGroup);

struct file_reader;

EXPAPI typedef file_reader *open_file_reader_callback(char const *FileName);
EXPAPI typedef void close_file_reader_callback(file_reader *Reader);
EXPAPI typedef int32x read_at_most_callback(file_reader* Reader,
                                            int32x FilePosition,
                                            int32x UInt8Count,
                                            void *Buffer);

EXPAPI typedef bool get_reader_size_callback(file_reader* Reader, int32x* NumBytes);


EXPTYPE_EPHEMERAL struct file_reader
{
    close_file_reader_callback* CloseFileReaderCallback;
    read_at_most_callback* ReadAtMostCallback;
    get_reader_size_callback* GetReaderSizeCallback;
};

EXPAPI GS_SAFE bool InitializeFileReader(close_file_reader_callback *CloseFileReaderCallback,
                                         read_at_most_callback *ReadAtMostCallback,
                                         get_reader_size_callback* GetReaderSizeCallback,
                                         file_reader &Reader);

EXPAPI GS_PARAM file_reader *CreatePlatformFileReader(char const *FileNameToOpen);

#define OpenFileReader(FileName) (*OpenFileReaderCallback)(FileName)
#define CloseFileReader(Reader) do { if(Reader) {(*((Reader)->CloseFileReaderCallback))(Reader);} } while (false) EXPMACRO
#define ReadAtMost(Reader, Pos, Count, Buffer) (*((Reader)->ReadAtMostCallback))(Reader, Pos, Count, Buffer) EXPMACRO
#define ReadExactly(Reader, Pos, Count, Buffer) ((*((Reader)->ReadAtMostCallback))(Reader, Pos, Count, Buffer) == Count) EXPMACRO
#define GetReaderSize(Reader, SizeVar)          ((*((Reader)->GetReaderSizeCallback))(Reader, SizeVar)) EXPMACRO

// TODO: Access to some sort of list of open file handles?

extern open_file_reader_callback *OpenFileReaderCallback;

EXPAPI GS_READ open_file_reader_callback *GetDefaultFileReaderOpenCallback(void);
EXPAPI GS_MODIFY void SetDefaultFileReaderOpenCallback(open_file_reader_callback *OpenFileReaderCallback);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_FILE_READER_H
#endif
