//
//  main.cpp
//  playbuilder
//
//  Created by Gwynne Raskind on 12/15/15.
//  Copyright © 2015 Playnet, Inc. All rights reserved.
//

#include "utility.hpp"
#include "informational.hpp"
#include "program_state.hpp"

using namespace std;

enum class program_option : uint32_t { format, extrawl, wlflag, printwl, verbose, debug, help, version };

int main(int argc, char **argv)
{
    // 1) Options
    int argind = 1;
    unordered_map<program_option, option_spec> opts = {
        { program_option::format,  { "format",          'f', true,  "" } },
        { program_option::extrawl, { "allow-ext",       'a', true,  "" } },
        { program_option::wlflag,  { "no-whitelist",    'n', false, "" } },
        { program_option::printwl, { "print-whitelist",   0, false, "" } },
        { program_option::verbose, { "verbose",         'v', false, "" } },
        { program_option::debug,   { "debug",           'd', false, "" } },
        { program_option::help,    { "help",            'h', false, "" } },
        { program_option::version, { "version",         'V', false, "" } },
    };
    string data_directory = "", output_directory = "", errmsg = "";
    option_result res = option_result::bad_opt;

    while ((res = read_opt(argind, argc, argv, opts, errmsg)) != option_result::end_args) {
        if (res == option_result::no_param || res == option_result::bad_opt) {
            die_usage("%s\n", errmsg.c_str());
        } else if (res == option_result::not_opt) {
            if (data_directory == "") {
                data_directory = errmsg;
            } else if (output_directory == "") {
                output_directory = errmsg;
            } else {
                die_usage("Too many arguments.\n");
            }
        }
    }
    
    if (opts[program_option::help].stored_value == "yes") {
        print_usage(stdout, nullptr, true);
        exit(EXIT_SUCCESS);
    }
    if (opts[program_option::version].stored_value == "yes") {
        print_version(stdout);
        exit(EXIT_SUCCESS);
    }
    
    if (data_directory == "" || output_directory == "") {
        die_usage("Both input data directory and output archive directory are required.\n");
    }

    if (opts[program_option::printwl].stored_value == "yes") {
        dump_whitelist(stdout);
        exit(EXIT_SUCCESS);
    }
    
    // 2) Onward!
    return program_state::process_data(
        data_directory,
        output_directory,
        (opts[program_option::format].stored_value == "" ? "*:+" : opts[program_option::format].stored_value), // format string
        opts[program_option::extrawl].stored_value, // extra whitelist entries
        (opts[program_option::wlflag].stored_value == ""), // use builtin whitelist flag
        (opts[program_option::verbose].stored_value != ""), // verbose flag
        (opts[program_option::debug].stored_value != "") // debug flag
    );
}
