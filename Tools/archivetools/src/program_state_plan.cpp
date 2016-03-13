//
//  program_state_plan.cpp
//  playbuilder
//
//  Created by Gwynne Raskind on 12/17/15.
//  Copyright © 2015 Playnet, Inc. All rights reserved.
//

#include "informational.hpp"
#include "program_state.hpp"

// Utility
char program_state::band::band_edge(size_t which, size_t num_bands)
{
    return 'A' + static_cast<char>(static_cast<float>(which) / static_cast<float>(num_bands) * 26.0);
}

program_state::band::band(uint8_t which, uint8_t split, const vector<string> &possible_entries)
    : first(band_edge(which, split)), last(band_edge(which + 1, split) - 1), paths()
{
    for (const string &path : possible_entries) {
        char c = toupper(file_basename(path)[0]);
        
        if ((c >= first && c <= last) || (last == 'Z' && !isalpha(c))) {
            paths.push_back(path);
        }
    }
}

// Data paths
void program_state::read_data_paths(void)
{
    for (const string &path : recursive_paths_at_path(data_directory)) {
        string ext = strtoupper(file_path_extension(path));
        
        if (std::find(whitelist.begin(), whitelist.end(), ext) != whitelist.end()) {
            data_files_by_extension[ext].push_back(path);
        }
    }
    
    INFO("Raw data path list (%lu entries):", data_files_by_extension.size());//\n%s", paths.size(), combine(paths, "\n").c_str());
    INFO("Extension-to-path map (%lu extensions):", data_files_by_extension.size());
        //\n%s", map_combine(data_files_by_extension, "\n", ":\n    ", "\n    ").c_str());
}

// Archive plan
vector<program_state::band> program_state::band_by_alpha_split(uint8_t alpha_split, const vector<string> &possible_files)
{
    vector<band> bands;
    
    for (uint8_t i = 0; i < alpha_split; ++i) {
        bands.push_back(band(i, alpha_split, possible_files));
    }
    return bands;
}

vector<program_state::chunk> program_state::chunk_by_max_size(size_t max_size, const vector<program_state::band> &bands)
{
    vector<chunk> chunks;
    
    for (const band &b : bands) {
        if (b.paths.empty()) { continue; }
        
        chunk current_chunk = { chunks.size(), 0, {} };
        
        for (const string &path : b.paths) {
            size_t filesize = file_size_on_disk(path);
            
            if (filesize > max_size) {
                die("File %s too big for archive max size %lu.\n", path.c_str(), max_size);
            }
            else if (current_chunk.total_size + filesize > max_size) {
                chunks.push_back(current_chunk);
                current_chunk = { chunks.size(), filesize, { path } };
            } else {
                current_chunk.total_size += filesize;
                current_chunk.paths.push_back(path);
            }
        }
        chunks.push_back(current_chunk);
    }
    return chunks;
}

void program_state::make_archive_plan(void)
{
    uint32_t next_auto_number = 1;
    unordered_map<string, vector<string>> remaining_files = data_files_by_extension;

    for (const archive_entry &entry : entries) {
        vector<string> possible_files;
        
        INFO("Starting next entry.");
        show_parsed_entry(entry, true);
        
        for (const pair<string, vector<string>> &item : remaining_files) {
            if (entry.extensions.size() == 0 || std::find(entry.extensions.begin(), entry.extensions.end(), item.first) != entry.extensions.end()) {
                possible_files.insert(possible_files.end(), item.second.begin(), item.second.end());
            }
        }
        std::sort(possible_files.begin(), possible_files.end(), [&] (const string &a, const string &b) { return file_basename(a) < file_basename(b); });
        
        vector<band> bands = band_by_alpha_split(entry.alpha_split, possible_files);
        vector<chunk> chunks = chunk_by_max_size(entry.max_size, bands);
        
        for (const chunk &c : chunks) {
            plans.push_back(archive_plan{ next_auto_number++, c.paths });
        }

        INFO("%lu possible files for entry", possible_files.size());
        INFO("Entry bands with alpha split %hhu:\n", entry.alpha_split);
        for (const band &b : bands) { INFO("  %c-%c: %lu paths", b.first, b.last, b.paths.size()); }
        INFO("Entry chunks with max size %llu:\n", entry.max_size == SIZE_MAX ? 0 : entry.max_size);
        for (const chunk &c : chunks) { INFO("  %2lu: %lu paths, totalling %lu bytes", c.n, c.paths.size(), c.total_size); }
        INFO("Allocated %lu archives to this entry:", chunks.size());
        for (size_t i = plans.size() - 1; i > plans.size() - chunks.size() - 1; --i) {
            INFO("  %02u: %lu files", plans[i].number, plans[i].paths.size());
        }
        INFO(" ");
    }
    INFO("Final plan has %lu archives", plans.size());
}
