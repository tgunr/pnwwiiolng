//
//  program_state_debug.cpp
//  playbuilder
//
//  Created by Gwynne Raskind on 12/17/15.
//  Copyright © 2015 Playnet, Inc. All rights reserved.
//

#include "informational.hpp"
#include "program_state.hpp"

printf_function(2,0) void program_state::NOTEv(printf_format_arg(1) const char * const format, va_list args)
{
    vnote(verbose, format, args);
}

printf_function(2,3) void program_state::NOTE(printf_format_arg(0) const char * const format, ...)
{
    va_list args;
    
    va_start(args, format);
    NOTEv(format, args);
    va_end(args);
}
    
printf_function(2,0) void program_state::INFOv(printf_format_arg(1) const char * const format, va_list args)
{
    vinfo(debug, format, args);
}

printf_function(2,3) void program_state::INFO(printf_format_arg(0) const char * const format, ...)
{
    va_list args;
    
    va_start(args, format);
    INFOv(format, args);
    va_end(args);
}

void program_state::show_initial_state(const string &whitelist_extra, bool use_builtin_whitelist)
{
    NOTE("Data directory: %s", data_directory.c_str());
    NOTE("Output directory: %s", output_directory.c_str());
    NOTE("Format string: %s", format_string.c_str());
    if (whitelist_extra.length() > 0) { NOTE("Extra whitelist entries: %s", whitelist_extra.c_str()); }
    if (!use_builtin_whitelist) { NOTE("Skipping builtin whitelist."); }
    if (verbose) { NOTE("Verbose information on."); }
    if (debug) { NOTE("Debug information on."); }
}
    
void program_state::show_parsed_entry(const program_state::archive_entry &entry, bool is_debug)
{
    if (is_debug) {
        INFO("    # %u alpha-split %hhu max-size %llu extensions %s",
             entry.first_number,
             entry.alpha_split,
             entry.max_size,
             combine(entry.extensions, ",", "<rest>").c_str()
        );
    } else {
        NOTE("    # %u alpha-split %hhu max-size %llu extensions %s",
             entry.first_number,
             entry.alpha_split,
             entry.max_size,
             combine(entry.extensions, ",", "<rest>").c_str()
        );
    }
}

void program_state::show_parsed_entry_list(void)
{
    NOTE("Parsed entry list:");
    for (const archive_entry &entry : entries) { show_parsed_entry(entry); }
}
