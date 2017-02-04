//
//  program_state.hpp
//  playbuilder
//
//  Created by Gwynne Raskind on 12/17/15.
//  Copyright © 2015 Playnet, Inc. All rights reserved.
//

#ifndef program_state_h
#define program_state_h

#include "utility.hpp"

using namespace std;

class program_state
{
    struct archive_entry
    {
        uint32_t first_number; // starts at 1; zero means '*'
        vector<string> extensions; // filtered relative to the whitelist; empty means "rest"
        uint8_t alpha_split; // what fraction to split the alphabet into - 1 means don't, 0 means no % specifier
        uint64_t max_size; // bytes, maximum size per archive, < specifier
    };
    struct band
    {
        char first, last;
        vector<string> paths;

        static char band_edge(size_t which, size_t num_bands);
        band(uint8_t which, uint8_t split, const vector<string> &possible_entries);
    };
    struct chunk
    {
        size_t n, total_size;
        vector<string> paths;
    };
    struct archive_plan
    {
        uint32_t number;
        vector<string> paths;
    };
    
    string data_directory, output_directory, format_string;
    unordered_set<string> whitelist, desired_extensions;
    vector<archive_entry> entries;
    unordered_map<string, vector<string>> data_files_by_extension;
    vector<archive_plan> plans;
    bool verbose, debug;
    
    // Debug
    printf_function(2,0) void NOTEv(printf_format_arg(1) const char * const format, va_list args);
    printf_function(2,3) void NOTE (printf_format_arg(0) const char * const format, ...);
    printf_function(2,0) void INFOv(printf_format_arg(1) const char * const format, va_list args);
    printf_function(2,3) void INFO (printf_format_arg(0) const char * const format, ...);
    void show_initial_state(const string &whitelist_extra, bool use_builtin_whitelist);
    void show_parsed_entry(const archive_entry &entry, bool is_debug = false);
    void show_parsed_entry_list(void);
    
    // Config
    void consolidate_whitelist(bool use_builtin, const string &extra_entries);
    uint32_t parse_archive_specifier(const string &specifier);
    uint8_t parse_alpha_split_specifier(const string &specifier);
    uint64_t parse_file_max_specifier(const string &specifier);
    archive_entry parse_data_specifiers(uint32_t archive_number, const string &specifier_list);
    void parse_format_string(void);
    
    // Plan
    void read_data_paths(void);
    vector<band> band_by_alpha_split(uint8_t alpha_split, const vector<string> &possible_files);
    vector<chunk> chunk_by_max_size(size_t max_size, const vector<band> &bands);
    void make_archive_plan(void);
    
    // Build
    void make_single_archive(const archive_plan &plan, const string &archive_name, void *p_);
    void run_plan(void);

    // Interface
    public:
        static int process_data(const string &data_dir, const string &out_dir, const string &fmt_str, const string &whitelist_extra,
                                bool builtin_whitelist, bool v, bool d)
        {
            return program_state(data_dir, out_dir, fmt_str, whitelist_extra, builtin_whitelist, v, d).run();
        }
        
        program_state(const string &data_dir, const string &out_dir, const string &fmt_str, const string &whitelist_extra, bool builtin_whitelist,
                      bool v, bool d)
            : data_directory(data_dir), output_directory(out_dir), format_string(fmt_str), whitelist(), entries(), data_files_by_extension(),
              verbose(v), debug(d)
        {
            consolidate_whitelist(builtin_whitelist, whitelist_extra);
        }
        
        int run(void)
        {
            parse_format_string();
            read_data_paths();
            make_archive_plan();
            run_plan();
            return EXIT_SUCCESS;
        }
};

void dump_whitelist(FILE *stream);

#endif /* program_state_hpp */
