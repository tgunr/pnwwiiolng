//
//  informational.cpp
//  playbuilder
//
//  Created by Gwynne Raskind on 12/15/15.
//  Copyright © 2015 Playnet, Inc. All rights reserved.
//

#include "informational.hpp"

// Errors
printf_function(1,0) noreturn_function void vdie(printf_format_arg(1) const char * const message, va_list args)
{
    vfprintf(stderr, message, args);
    exit(EXIT_FAILURE);
}

printf_function(1,2) noreturn_function void die(printf_format_arg(0) const char * const message, ...)
{
    va_list args;
    
    va_start(args, message);
    vdie(message, args);
    va_end(args);
}

printf_function(1,0) noreturn_function void vdie_usage(printf_format_arg(1) const char * const message, va_list args)
{
    print_usage(stderr, bprintf(message, args).c_str());
    exit(EXIT_FAILURE);
}

printf_function(1,2) noreturn_function void die_usage(printf_format_arg(0) const char * const message, ...)
{
    va_list args;
    
    va_start(args, message);
    vdie_usage(message, args);
    va_end(args);
}

// Logging
printf_function(2,0) void vnote(bool verbose, printf_format_arg(1) const char * const format, va_list args)
{
    if (!verbose) {
        return;
    }
    
    auto lines = split(vbprintf(format, args), "\n", 0);
    
    for (const auto &line : lines) {
        if (line.length() < 1 && std::addressof(line) == std::addressof(lines.back())) { // Wild.
            continue;
        }
        fprintf(stderr, "VERBOSE: %s\n", line.c_str());
    }
}

printf_function(2,3) void note(bool verbose, printf_format_arg(0) const char * const format, ...)
{
    va_list args;
    
    va_start(args, format);
    vnote(verbose, format, args);
    va_end(args);
}

// Debug utilities
printf_function(2,0) void vinfo(bool debug, printf_format_arg(1) const char * const format, va_list args)
{
    if (!debug) {
        return;
    }
    
    auto lines = split(vbprintf(format, args), "\n", 0);
    
    for (const auto &line : lines) {
        if (line.length() < 1 && std::addressof(line) == std::addressof(lines.back())) { // Wild.
            continue;
        }
        fprintf(stderr, "DEBUG: %s\n", line.c_str());
    }
}

printf_function(2,3) void info(bool debug, printf_format_arg(0) const char * const format, ...)
{
    va_list args;
    
    va_start(args, format);
    vinfo(debug, format, args);
    va_end(args);
}

// Help
static void print_detailed_usage(FILE *stream)
{
                                                                                            // 80 columns |
    fprintf(stream, "                                                                                     \n");
    fprintf(stream, "Format string syntax:                                                                \n");
    fprintf(stream, "    The format string consists of a series of semicolon-separated archive numbers,   \n");
    fprintf(stream, "    followed by colons, which determine what files from the data directory go into   \n");
    fprintf(stream, "    each archive. The specifiers can be extensions, distribution percentages, file   \n");
    fprintf(stream, "    size limits, or the special specifier '+' (meaning all files not already in      \n");
    fprintf(stream, "    another archive). If no format string is provided, '*:+' is assumed, placing all \n");
    fprintf(stream, "    files into the first, and only, archive. Whitespace in the format string,        \n");
    fprintf(stream, "    including newlines, is ignored. Comments can be embedded using the C /* */       \n");
    fprintf(stream, "    comment syntax. The archive number '*' means 'the next sequential archive        \n");
    fprintf(stream, "    number', and in the case of file size specifiers may refer to mutiple archives.  \n");
    fprintf(stream, "    The next archive number in sequence is always the last archive in the string plus\n");
    fprintf(stream, "    one, even if the previous archive number was also '*'. The '%%' specifier splits  \n");//+1
    fprintf(stream, "    file extensions across the number of archives given by the following number. The \n");
    fprintf(stream, "    '<' specifier splits the provided extensions across multiple archives, based on  \n");
    fprintf(stream, "    the following file size. File sizes may be in bytes, K, M, or G. Archives split  \n");
    fprintf(stream, "    by this specifier will be numbered sequentially, skipping any archive numbers    \n");
    fprintf(stream, "    which appear elsewhere in the format string. The value of '*' is adjusted        \n");
    fprintf(stream, "    accordingly.                                                                     \n");
    fprintf(stream, "Examples:                                                                            \n");
    fprintf(stream, "    1:WAV;2:SDM;3:BMP;4:MDL,%%3;*:DDS,<250M;*:+                                       \n");//+1
    fprintf(stream, "      An older archive configuration. Splits MDLs across #s 4-6 by alphabetical      \n");
    fprintf(stream, "      sorting (A-F in 4, G-R in 5, S-Z and #s in 6), splits DDS files across 7-? by  \n");
    fprintf(stream, "      archive size. Remaining files go in the last archive.                          \n");
    fprintf(stream, "    8:DDS,<150M;5:MDL,%%3;1:WAV;2:SDM;3:BMP;4:+                                       \n");//+1
    fprintf(stream, "      The original archive configuration. Mostly the same as above, but with         \n");
    fprintf(stream, "      different archive numbers and smaller DDS archives.                            \n");
}

void print_usage(FILE *stream, const char *extra_message, bool detailed)
{
                                                                                            // 80 columns |
    if (extra_message) {
        fprintf(stream, "%s\n", extra_message);
    }
    fprintf(stream, "Usage: playbuilder [options] data_directory output_path                              \n");
    fprintf(stream, "                                                                                     \n");
    fprintf(stream, "Recognized options:\n");
    fprintf(stream, "    --format/-f        Provide a format string which tells the builder how many      \n");
    fprintf(stream, "                       archives to create and what to put in them.                   \n");
    fprintf(stream, "    --allow-ext/-a     Add the /-separated file extensions to the built-in whitelist \n");
    fprintf(stream, "                       of file types, e.g. --allow-ext='ABC/DEF'. Extensions are     \n");
    fprintf(stream, "                       never case-sensitive on any platform.                         \n");
    fprintf(stream, "    --no-whitelist/-n  Disable the built-in whitelist of file types. In this mode,   \n");
    fprintf(stream, "                       only files matched by the -a option will be included.         \n");
    fprintf(stream, "    --print-whitelist  Print the built-in whitelist of file types and exit.          \n");
    fprintf(stream, "    --verbose/-v       Enable verbose operation. Prints more information about what's\n");
    fprintf(stream, "                       going on.                                                     \n");
    fprintf(stream, "    --help/-h          Print this usage message, and additionally information about  \n");
    fprintf(stream, "                       the syntax of the format string, and exit.                    \n");
    fprintf(stream, "    --version/-V       Print version of the program and exit.                        \n");
    if (detailed) {
        print_detailed_usage(stream);
    }
}

void print_version(FILE *stream)
{
                                                                                            // 80 columns |
    fprintf(stream, "PlayBuilder version 2.0. Copyright (c) 2015 Playnet, Inc.                            \n");
    fprintf(stream, "Build date: %s at %s\n", __DATE__, __TIME__);
    fprintf(stream, "This is an internal tool. This tool is the intellectual property of Playnet, Inc.    \n");
    fprintf(stream, "Usage or distribution of this tool in any form outside Playnet, Inc. is strictly     \n");
    fprintf(stream, "prohibited.                                                                          \n");
}
