//
//  utility.cpp
//  playbuilder
//
//  Created by Gwynne Raskind on 12/15/15.
//  Copyright © 2015 Playnet, Inc. All rights reserved.
//

#include "utility.hpp"
#include "informational.hpp"
#include <dirent.h>

// Options
option_result _read_opt_internal(int &argind, int argc, char **argv, const std::vector<option_spec *> &opts, std::string &error_message)
{
    std::tuple<option_result, int, std::string *, std::string> result;
    
    if (argind >= argc) {
        error_message = "no more arguments";
        return option_result::end_args;
    }
    
    std::string arg = argv[argind++];
    
    for (option_spec *opt : opts) {
        if (arg.length() < 1) {
            continue;
        }
        if (arg[0] != '-') {
            error_message = arg;
            return option_result::not_opt;
        }
            
        if (opt->takes_param && arg.find("--" + opt->long_arg + "=") == 0) {
            opt->stored_value = arg.substr(2 + opt->long_arg.length() + 1);
            return option_result::found_opt;
        } else if ((opt->long_arg != "" && arg == "--" + opt->long_arg) || (opt->short_arg != 0 && arg == "-" + std::string(1, opt->short_arg))) {
            if (opt->takes_param) {
                if (argind >= argc) {
                    error_message = "--" + opt->long_arg + ": missing parameter";
                    return option_result::no_param;
                }
                opt->stored_value = argv[argind++];
                return option_result::found_opt;
            } else {
                opt->stored_value = "yes";
                return option_result::found_opt;
            }
        } else if (arg == "--") {
            error_message = "argument separator found";
            return option_result::end_args;
        }
    }
    error_message = "Unrecognized option: " + arg;
    return option_result::bad_opt;
}

// Strings
bool isdigits(const std::string &s)
{
    for (const char &c : s) {
        if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}

std::string strtoupper(const std::string &s)
{
    std::string result = s;
    
    std::for_each(result.begin(), result.end(), [](decltype(result)::value_type &c) { c = toupper(c); });
    return result;
}

std::string replace(const std::string &haystack, const std::string &needle, const std::string &replacement)
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

std::vector<std::string> split(const std::string &str, const std::string &delimiter, size_t max_split)
{
    if (str == "") { return {}; }
    
    std::vector<std::string> strings;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos && strings.size() < (max_split == 0 ? SIZE_MAX : max_split - 1))
    {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + 1;
    }
    strings.push_back(str.substr(prev));
    return strings;
}

std::string combine(const std::vector<std::string> &items, const std::string &delimiter)
{
    std::string result;
    
    for (const auto &item : items) {
        result += item + delimiter;
    }
    return result.substr(0, result.length() - delimiter.length());
}

bool has_prefix(const std::string &str, const std::string &prefix)
{
    return str.compare(0, prefix.size(), prefix) == 0;
}

std::string repeat(const std::string &str, size_t n)
{
    std::string result;
    for (size_t i = 0; i < n; ++i) { result += str; }
    return result;
}

printf_function(1,0) std::string vbprintf(printf_format_arg(1) const char * const format, va_list args)
{
    char *s = nullptr;

    vasprintf(&s, format, args);
    return std::string(std::unique_ptr<char, std::decay_t<decltype(free)>>(s, free).get());
}

printf_function(1,2) std::string bprintf(printf_format_arg(0) const char * const format, ...)
{
    va_list args;
    std::string r;
    
    va_start(args, format);
    r = vbprintf(format, args);
    va_end(args);
    return r;
}

// Filesystem
template <typename T> std::make_signed_t<T> signed_val(T val) { return static_cast<std::make_signed_t<T>>(val); }

static auto _file_basename_indexes(const std::string &path)
{
    // Very strangely, this works even when there are only separators or only
    // not-separators in the entire string, or even an empty string! This is due
    // specifically to npos being -1 and substr() being okay with count>size().
    // This is vulnerable to an integer underflow exploit if you manage to pass
    // in a string that is (or thinks it is) near 2GB in length.
    //
    // +--------+----+----+----+----+----+------+------+------+------+------+
    // |  path  | ns | s  |s+1 |ns-s|d-ns| bsub | base | dsb1 | dsb2 | dir  |
    // |01234567+----+----+----+----+----+------+------+------+------+------+
    // |        | -1 | -1 |  0 |  0 | -1 |      | .    |      |      | .    |
    // |abc     |  2 | -1 |  0 |  3 |  2 | abc  | abc  | abc  | abc  | .    |
    // |/abc    |  3 |  0 |  1 |  3 | -1 | abc  | abc  |      |      | /    |
    // |abc/    |  2 | -1 |  0 |  3 |  2 | abc  | abc  | abc/ | abc  | .    |
    // |a/b     |  2 |  1 |  2 |  1 |  0 | b    | b    | a    | a    | a    |
    // |/a/b    |  3 |  2 |  3 |  1 |  1 | b    | b    | /a   | /a   | /a   |
    // |/a/b/   |  3 |  2 |  3 |  1 |  1 | b    | b    | /a   | /a   | /a   |
    // |//a/b   |  4 |  3 |  4 |  1 |  2 | b    | b    | //a  | //a  | //a  |
    // |//a//b  |  5 |  4 |  5 |  1 |  2 | b    | b    | //a/ | //a  | //a  |
    // |//a//b//|  5 |  4 |  5 |  1 |  2 | b    | b    | //a/ | //a  | //a  |
	// |//a/b// |  4 |  3 |  4 |  1 |  2 | b    | b    | //a  | //a  | //a  |
    // |/       | -1 |  0 |  1 | -1 | -1 | /    | /    |      |      | /    |
    // |///     | -1 |  2 |  3 | -3 | -1 | /    | /    | //   |      | /    |
    // +--------+----+----+----+----+----+------+------+------+------+------+
    auto not_sep = path.find_last_not_of(DIRECTORY_SEPARATOR),
             sep = path.find_last_of(DIRECTORY_SEPARATOR, not_sep);
    return std::make_pair( sep + 1, not_sep - sep );
}

std::string file_basename(const std::string &path)
{
    auto indexes = _file_basename_indexes(path);
    return path == "" ? "." : path.substr(indexes.first, indexes.second);
}

std::string file_dirname(const std::string &path)
{
    auto indexes = _file_basename_indexes(path);
    return (indexes.first == 0 ? "." : ( // empty, relative, or one-component path
            signed_val(indexes.second) < 0 || indexes.first == 1 ? "/" : ( // only separators or one-component absolute path
            path.substr(0, indexes.first - 1).substr(0, path.find_last_not_of(DIRECTORY_SEPARATOR, indexes.first - 1) + 1)
           )));
}

static std::string file_extension(const std::string &name)
{
    // asdf   substr(-1 + 1 = 0) ==
    // asdf.  substr( 4 + 1 = 5) ==
    // asdf.a substr( 4 + 1 = 5) == a
    // .asdf  substr( 0 + 1 = 1) == asdf
    // .      substr( 0 + 1 = 1) ==
    return name.substr(name.find_last_of('.') + 1 ?: name.length());
}

std::string file_path_extension(const std::string &name)
{
    return file_extension(file_basename(name));
}

#include <sys/stat.h>

size_t file_size_on_disk(const std::string &path)
{
    struct stat buf;
    
    if (stat(path.c_str(), &buf) < 0) {
        return SIZE_MAX;
    }
    return static_cast<size_t>(buf.st_size);
}

struct direntries
{
    template <class _Selector, class _Comparer>
    direntries(const std::string &path, _Selector selector, _Comparer comparer)
        : entries(nullptr), nentries(scandir(path.c_str(), &entries, selector, comparer))
    {
    }
    
    ~direntries()
    {
        while (--nentries > -1) {
            ::free(entries[nentries]);
        }
        ::free(entries);
    }
    
    bool error() const
    {
        return nentries < 0;
    }
    
    size_t size() const
    {
        return static_cast<size_t>(std::max(0, nentries));
    }
    
    auto begin() const
    {
        return entries;
    }
    
    auto end() const
    {
        return begin() + size();
    }
    
    private:
        dirent **entries;
        int nentries;
};

static int seekdir(const std::string &path, std::unordered_set<std::string> &full_paths)
{
    auto result = direntries(path, [] (auto ent) -> int {
        return ent->d_name[0] != '.' && (ent->d_type & (DT_DIR | DT_REG));
    }, nullptr);
    
    for (auto d : result) {
        if (d->d_type == DT_DIR && seekdir(path + DIRECTORY_SEPARATOR + d->d_name, full_paths) < 0) {
            return -1;
        }
        else if (d->d_type == DT_REG) {
            full_paths.insert(path + DIRECTORY_SEPARATOR + d->d_name);
        }
    }
	return result.error() ? -1 : 0;
}

std::unordered_set<std::string> recursive_paths_at_path(const std::string &path)
{
    std::unordered_set<std::string> results;
    
    if (seekdir(path, results) < 0) {
        // oh well
        return decltype(results)();
    }
    return results;
}
