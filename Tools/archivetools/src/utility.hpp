//
//  utility.hpp
//  playbuilder
//
//  Created by Gwynne Raskind on 12/15/15.
//  Copyright © 2015 Playnet, Inc. All rights reserved.
//

#ifndef utility_hpp
#define utility_hpp

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

// Compiler feature and version checks. Clang is assumed to exist if __has_feature() works.
#ifndef __has_feature
#define __has_feature(...) 0
#endif

#ifndef __has_extension
#define __has_extension(...) 0
#endif

#ifndef __has_feature_or_ext
#define __has_feature_or_ext(...) __has_feature(__VA_ARGS__) || __has_extension(__VA_ARGS__)
#endif

#ifndef __check_GCC_version
#define __check_GCC_version(major, minor) (__GNUC__ > major || (__GNUC__ == major && __GNUC_MINOR__ >= minor))
#endif

#ifndef __check_MSVC_version
#define __check_MSVC_version(major) (_MSC_VER >= major ## 00)
#endif

// Compiler-specific feature sets
#if __has_feature_or_ext(cxx_attributes) || __check_GCC_version(4, 9) || __check_MSVC_version(19) // Clang, GCC 4.9+, VS 2015+

#define noreturn_function [[noreturn]]
#define printf_function(last_arg, start_of_varargs) [[gnu::format(printf,last_arg,start_of_varargs)]]
#define printf_format_arg(args_start)

#elif __check_GCC_version(4, 9) // GCC 4.9+, don't support older

#define noreturn_function __attribute__((noreturn))
#define printf_function(last_arg, start_of_varargs) __attribute__((format(printf,last_arg,start_of_varargs)))
#define printf_format_arg(args_start)

#elif __check_MSVC_version(16) // VS 2010+

#include <sal.h>
#define noreturn_function __declspec(noreturn)
#define printf_function(last_arg, start_of_varargs)
#define printf_format_arg(args_start) _Printf_format_string_params_(args_start)

#else

#error "Unsupported compiler!"

#endif

// Option parsing
struct option_spec
{
    std::string long_arg; // no leading --
    char        short_arg;
    bool        takes_param; // params are always required
    std::string stored_value;
};

enum class option_result : uint32_t {
    found_opt, // Found a valid option
    end_args,  // Hit end of arguments
    not_opt,   // Found something not an option
    bad_opt,   // Found an invalid option
    no_param,  // Found a valid option w/o its required parameter
};

// The map keys are not interpreted; they're for the caller's reference.
template <typename T>
option_result read_opt(int &argind, int argc, char **argv, std::unordered_map<T, option_spec> &opts, std::string &error_message)
{
    using vosp_t = std::vector<option_spec *>;
    vosp_t opts_;
    for (typename std::remove_reference_t<decltype(opts)>::value_type &spec : opts) { opts_.push_back(std::addressof(spec.second)); }
    option_result _read_opt_internal(int &, int, char **, const vosp_t &, std::string &);
    return _read_opt_internal(argind, argc, argv, opts_, error_message);
}

// String utilities
bool isdigits(const std::string &s);
std::string strtoupper(const std::string &s);
std::string replace(const std::string &haystack, const std::string &needle, const std::string &replacement);
std::vector<std::string> split(const std::string &str, const std::string &delimiter, size_t max_split = 0);
bool has_prefix(const std::string &str, const std::string &prefix);
std::string repeat(const std::string &str, size_t n);

template <typename _InputIterator>
std::string combine(_InputIterator __first, _InputIterator __last, const std::string &delimiter, const std::string &emptyValue = "")
{
    if (__first == __last) { return emptyValue; }
    std::string result;
    for (; __first != __last; ++__first)
        result += *__first + delimiter;
    return result.substr(0, result.length() - delimiter.length());
}

template <typename _Container>
std::string combine(_Container __container, const std::string &delimiter, const std::string &emptyValue = "")
{
    return combine(std::cbegin(__container), std::cend(__container), delimiter, emptyValue);
}

template <typename _InputIterator>
std::string map_combine(_InputIterator __first, _InputIterator __last,
                        const std::string &key_delimiter, const std::string &key_value_separator, const std::string &value_delimiter,
                        const std::string &emptyValue = "")
{
    if (__first == __last) { return emptyValue; }
    std::string result;
    for (; __first != __last; ++__first)
        result += __first->first
                + key_value_separator
                + combine(std::begin(__first->second), std::end(__first->second), value_delimiter)
                + key_delimiter;
    return result.substr(0, result.length() - key_delimiter.length());
}

template <typename _Container>
std::string map_combine(_Container __container,
                        const std::string &key_delimiter, const std::string &key_value_separator, const std::string &value_delimiter,
                        const std::string &emptyValue = "")
{
    return map_combine(std::cbegin(__container), std::cend(__container), key_delimiter, key_value_separator, value_delimiter, emptyValue);
}

printf_function(1,2) std::string  bprintf(printf_format_arg(0) const char * const format, ...);
printf_function(1,0) std::string vbprintf(printf_format_arg(1) const char * const format, va_list args);

// Filesystem utilities
#ifdef _WIN32
constexpr const char DIRECTORY_SEPARATOR = '\\';
#else
constexpr const char DIRECTORY_SEPARATOR = '/';
#endif

std::string file_dirname(const std::string &path);
std::string file_basename(const std::string &path);
std::string file_path_extension(const std::string &name);
size_t file_size_on_disk(const std::string &path);
std::unordered_set<std::string> recursive_paths_at_path(const std::string &path);

// Type utilities
template <typename... Ts>
struct types_exist;

template <typename T>
struct types_exist<T>
{
    constexpr static auto value = std::is_destructible<T>::value;
};

template <typename T, typename... Ts>
struct types_exist<T, Ts...>
{
    constexpr static auto value = types_exist<T>::value && types_exist<Ts...>::value;
};


#ifndef _LIBCPP_NO_RTTI

#include <typeinfo>

template <const char *Sep, typename... Ts>
static const std::string type_names;

template <const char *Sep>
static const std::string type_names<Sep> = "";

template <const char *Sep, typename T>
static const std::string type_names<Sep, T> = typeid(T).name();

template <const char *Sep, typename T, typename... Ts>
static const std::string type_names<Sep, T, Ts...> = type_names<T> + Sep + type_names<Ts...>;

#endif

#include <cxxabi.h>

template <typename T>
struct full_type_info
{
#if _LIBCPP_NO_RTTI
    full_type_info() : name("**no RTTI**"), demangled_name(strdup("**no RTTI**"), std::free) {}
#else
    full_type_info()
        : _name(strdup(typeid(T).name()), std::free),
          _demangled_name(__cxxabiv1::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr), std::free)
    {
    }
#endif
    
    _LIBCPP_INLINE_VISIBILITY const char* raw_name() const _NOEXCEPT { return _name.get(); }
    _LIBCPP_INLINE_VISIBILITY const char* name() const _NOEXCEPT { return _demangled_name.get(); }
    
    protected:
        std::unique_ptr<char, std::decay_t<decltype(std::free)>> _name;
        std::unique_ptr<char, std::decay_t<decltype(std::free)>> _demangled_name;
};


template <typename... Ts>
struct last_type;

template <typename T>
struct last_type<T>
{
    typedef T type;
};

template <typename T, typename... Ts>
struct last_type<T, Ts...> : public last_type<Ts...>
{
    typedef typename last_type<Ts...>::type type;
};

template <typename... Ts>
using last_type_t = typename last_type<Ts...>::type;


template <size_t N, typename... Ts>
struct nth_type;

template <size_t N, typename T, typename... Ts>
struct nth_type<N, T, Ts...>
{
    static_assert(N <= sizeof...(Ts), "Type list index is out of range.");

    typedef typename nth_type<N - 1, T, Ts...>::type type;
};

template <typename T, typename... Ts>
struct nth_type<0, T, Ts...>
{
    typedef T type;
};

template <size_t N, typename... Ts>
using nth_type_t = typename nth_type<N, Ts...>::type;


// String template parameters
template <char... Cs>
struct __tmpl_str
{
    constexpr static size_t Length = sizeof...(Cs);
    constexpr static const char Value[sizeof...(Cs) + 1] = { Cs..., '\0' };
};

template <char... Cs>
constexpr const char __tmpl_str<Cs...>::Value[sizeof...(Cs) + 1];

template <size_t Idx, size_t Len> constexpr char __tmpl_str_char_at(const char (&str)[Len])
{ return str[Idx < Len ? Idx : Len - 1]; }

template <char... Cs> auto __tmpl_str_build(__tmpl_str<Cs...>) -> __tmpl_str<Cs...>;

template <char... Cs, char... J>
auto __tmpl_str_build(__tmpl_str<Cs...>, __tmpl_str<'\0'>, __tmpl_str<J>...) -> __tmpl_str<Cs...>;

template <char C, char... Cs, char... M>
auto __tmpl_str_build(__tmpl_str<Cs...>, __tmpl_str<C>, __tmpl_str<M>...)
    -> decltype(__tmpl_str_build(__tmpl_str<Cs..., C>(), __tmpl_str<M>()...));

template <char... Cs> auto __tmpl_str_make(__tmpl_str<Cs...>) -> decltype(__tmpl_str_build(__tmpl_str<Cs>()...));

#define tmpl_str(s) decltype(__tmpl_str_make(__tmpl_str< \
    __tmpl_str_char_at< 0>(s), __tmpl_str_char_at< 1>(s), __tmpl_str_char_at< 2>(s), __tmpl_str_char_at< 3>(s), \
    __tmpl_str_char_at< 4>(s), __tmpl_str_char_at< 5>(s), __tmpl_str_char_at< 6>(s), __tmpl_str_char_at< 7>(s), \
    __tmpl_str_char_at< 8>(s), __tmpl_str_char_at< 9>(s), __tmpl_str_char_at<10>(s), __tmpl_str_char_at<11>(s), \
    __tmpl_str_char_at<12>(s), __tmpl_str_char_at<13>(s), __tmpl_str_char_at<14>(s), __tmpl_str_char_at<15>(s)  \
>()))

#endif /* utility_hpp */
