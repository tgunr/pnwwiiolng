//
//  program_state_config.cpp
//  playbuilder
//
//  Created by Gwynne Raskind on 12/17/15.
//  Copyright © 2015 Playnet, Inc. All rights reserved.
//

#include "informational.hpp"
#include "program_state.hpp"

static vector<string> builtin_whitelist = {
    "DDS",          "GR2",          "AIM",          "AIR",          "ATLAS",            "BELT",         "BMP",
    "CAB",          "CFML",         "CG",           "CMP",          "COMPRESS",         "CONTROL",      "CSV",
    "DAT",          "DDS",          "DLL",          "DTD",          "ENG",              "EVT",          "EZ",
    "FNT",          "FRAG",         "GR2",          "GS",           "HDR",              "HIT",          "INI",
    "JMM",          "JS",           "LOADOUT",      "LOC",          "LUA",              "MASS",         "MDL",
    "OLD",          "PCX",          "POL",          "POS",          "SDM",              "SPT",          "STA",
    "STALL",        "STATE",        "STRINGS",      "TGA",          "TTF",              "TURRET",       "TXT",
    "UIXML",        "UV",           "VEH",          "VERT",         "WAV",              "XML",          "XSOUND",
    "XY",           ""/*none*/,
};

// Whitelist
void program_state::consolidate_whitelist(bool use_builtin, const string &extra_entries)
{
    if (use_builtin) {
        INFO("Using builtin whitelist.");
        whitelist.insert(builtin_whitelist.begin(), builtin_whitelist.end());
    }
    for (const string &entry : split(extra_entries, "/")) {
        whitelist.insert(strtoupper(entry));
    }
    show_initial_state(extra_entries, use_builtin);
}

// Format string
uint32_t program_state::parse_archive_specifier(const string &specifier)
{
    if (specifier.length() < 1) { die_usage("Archive specifier must be provided."); }
    if (specifier != "*" && !isdigits(specifier)) { die_usage("Archive specifier %s must be a number or '*'.", specifier.c_str()); }
    if (specifier == "*") {
        INFO("Parsed archive specifier *");
        return 0;
    } else {
        int32_t numeric_value = stoi(specifier);
        
        if (numeric_value <= 0) { die_usage("Archive specifier %s must be greater than zero.", specifier.c_str()); }
        INFO("Parsed archive specifier %u", static_cast<uint32_t>(numeric_value));
        return static_cast<uint32_t>(numeric_value);
    }
}

uint8_t program_state::parse_alpha_split_specifier(const string &specifier)
{
    if (!isdigits(specifier.substr(1))) { die_usage("Data specifier %s must provide a numeric alpha-split value.", specifier.c_str()); }
    
    int32_t numeric_value = stoi(specifier.substr(1));
    
    if (numeric_value <= 1) { die_usage("Data specifier %s must provide an alpha-split number greater than one.", specifier.c_str()); }
    if (numeric_value > 26) { die_usage("Data specifier %s must provide an alpha-split number no greater than 26.", specifier.c_str()); }
    
    INFO("Parsed alpha-split number %hhu", static_cast<uint8_t>(numeric_value));
    return static_cast<uint8_t>(numeric_value);
}

uint64_t program_state::parse_file_max_specifier(const string &specifier)
{
    uint64_t multiplier = 1;
    string only_digits = specifier.substr(1);
    
    if (!isdigits(only_digits)) {
        // Might have a size multiplier
        if (only_digits.back() == 'K')          { multiplier = 1024; }
        else if (only_digits.back() == 'M')     { multiplier = 1048576; }
        else if (only_digits.back() == 'G')     { multiplier = 1073741824; }
        else {
            die_usage("Data specifier %s must provide a numeric file-max value followed by nothing, K, M, or G.", specifier.c_str());
        }
        if (!isdigits(only_digits.substr(0, only_digits.size() - 1))) {
            die_usage("Data specifier %s must provide a numeric file-max value before the multiplier.", specifier.c_str());
        }
        only_digits = only_digits.substr(0, only_digits.size() - 1);
    }
    
    int64_t num_value = stoll(only_digits);
    
    if (num_value <= 0) { die_usage("Data specifier %s must provide a file-max value greater than zero.", specifier.c_str()); }
    num_value *= multiplier;
    info(debug, "Parsed file-max %lldB (x %c)", num_value, (multiplier == 1 ? 'B' : (multiplier == 1024 ? 'K' : (multiplier == 1048576 ? 'M' : 'G'))));
    return static_cast<uint64_t>(num_value);
}

program_state::archive_entry program_state::parse_data_specifiers(uint32_t archive_number, const string &specifier_list)
{
    if (specifier_list.length() < 1) { die_usage("Data specifiers must be provided."); }

    archive_entry new_entry = { .first_number = archive_number, .extensions = {}, .alpha_split = 1, .max_size = SIZE_MAX };
    bool saw_plus = false;
    auto specifiers = split(specifier_list, ",", 0);
    
    if (specifiers.size() < 1) { die_usage("Data specifiers must be provided."); }
    
    for (auto data_spec : specifiers) {
        if (data_spec.length() > 1 && data_spec[0] == '%') {
            new_entry.alpha_split = parse_alpha_split_specifier(data_spec);
        } else if (data_spec.length() > 1 && data_spec[0] == '<') {
            new_entry.max_size = parse_file_max_specifier(data_spec);
        } else if (data_spec.length() > 0 && data_spec[0] == '+') {
            if (new_entry.extensions.size() > 0) {
                die_usage("It is an error to use the + specifier with other file extensions in the same archive.");
            }
            saw_plus = true;
            INFO("Marked current entry with '+' specifier.");
            break;
        } else {
            // An extension of some kind
            if (saw_plus) { die_usage("It is an error to use the + specifier with other file extensions in the same archive."); }
            if (data_spec.find_first_of("/\\") != string::npos) {
                die_usage("/ and \\ can not appear in file extensions like %s", data_spec.c_str());
            }
            if (whitelist.find(strtoupper(data_spec)) == whitelist.end()) {
                NOTE("Skipping extension %s, not in the whitelist.", data_spec.c_str());
            }
            INFO("Adding extension %s to entry.", strtoupper(data_spec).c_str());
            new_entry.extensions.push_back(strtoupper(data_spec));
        }
    }
    INFO("Finished parsing data specifier.");
    return new_entry;
}

void program_state::parse_format_string(void)
{
    // 1:WAV;2:SDM;3:BMP;4:MDL,%3;*:DDS,<250M;*:+
    // 8:DDS,<150M;5:MDL,%3;1:WAV;2:SDM;3:BMP;4:+
    if (format_string == "") { die_usage("Format string can't be empty."); }
    
    for (const string &item_spec : split(format_string, ";")) {
        INFO("Processing specification %s", item_spec.c_str());

        auto parts = split(item_spec, ":", 2);
        
        if (parts.size() != 2) { die_usage("Format specification %s must contain a colon.", item_spec.c_str()); }
        entries.push_back(parse_data_specifiers(parse_archive_specifier(parts[0]), parts[1]));
    }
    
    for (const archive_entry &entry : entries) {
        if (count_if(entries.begin(), entries.end(), [&] (const archive_entry &ent) {
            return ent.first_number != 0 && ent.first_number == entry.first_number;
        }) > 1) {
            die_usage("Archive number %u can't appear explicitly more than once.", entry.first_number);
        }
    }
    
    // desired_extensions = reduce<decltype(desired_extensions)>(entries, [] (const auto &e, const auto &v) { return append(v, e.extensions); });
    show_parsed_entry_list();
}

void dump_whitelist(FILE *stream)
{
                                                                                            // 80 columns |
    fprintf(stream, "File extensions whitelisted by default:                                              \n");
    fprintf(stream, "    ");
    for (decltype(builtin_whitelist)::size_type i = 0; i < builtin_whitelist.size(); ++i) {
        if (builtin_whitelist[i] == "") {
            fprintf(stream, "%-9s", "<no ext>");
        } else {
            fprintf(stream, ".%-8s", strtoupper(builtin_whitelist[i]).c_str());
        }
        fprintf(stream, "%s", ((i + 1) % 7) == 0 ? (i == builtin_whitelist.size() - 1 ? "\n" : "\n    ") : " ");
    }
    if ((builtin_whitelist.size() % 7) != 0) {
        fprintf(stream, "\n");
    }
}

