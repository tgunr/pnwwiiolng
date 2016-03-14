//
//  informational.hpp
//  playbuilder
//
//  Created by Gwynne Raskind on 12/15/15.
//  Copyright © 2015 Playnet, Inc. All rights reserved.
//

#ifndef informational_hpp
#define informational_hpp

#include "utility.hpp"

// Error utilities
printf_function(1,0) noreturn_function void       vdie(printf_format_arg(1) const char * const message, va_list args);
printf_function(1,0) noreturn_function void vdie_usage(printf_format_arg(1) const char * const message, va_list args);

printf_function(1,2) noreturn_function void        die(printf_format_arg(0) const char * const message, ...);
printf_function(1,2) noreturn_function void  die_usage(printf_format_arg(0) const char * const message, ...);

// Logging utilities
printf_function(2,0) void vnote(bool verbose, printf_format_arg(1) const char * const format, va_list args);
printf_function(2,3) void  note(bool verbose, printf_format_arg(0) const char * const format, ...);

// Debug utilities
printf_function(2,0) void vinfo(bool debug, printf_format_arg(1) const char * const format, va_list args);
printf_function(2,3) void  info(bool debug, printf_format_arg(0) const char * const format, ...);

// Help utilities
void print_usage(FILE *stream, const char *extra_message = nullptr, bool detailed = false);
void print_version(FILE *stream);

#endif /* informational_hpp */
