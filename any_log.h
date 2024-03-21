// any_log
//
// A single header library that provides a simple and somewhat opinionated
// interface for logging and structured logging.
//
// To use this library you should choose a suitable file to put the
// implementation and define ANY_LOG_IMPLEMENT. For example
//
//    #define ANY_LOG_IMPLEMENT
//    #include "any_log.h"
//
// Additionally, you can customize the library behavior by defining certain
// macros in the file where you put the implementation. You can see which are
// supported by reading the code guarded by ANY_LOG_IMPLEMENT.
//
// This library is licensed under the terms of the MIT license.
// A copy of the license is included at the end of this file.
//

#ifndef ANY_LOG_INCLUDE
#define ANY_LOG_INCLUDE

// These values represent the decreasing urgency of a log invocation.
//
// panic: indicates a fatal error and using it will result in
//        the program termination (see any_log_panic)
//
// error: indicates a (non-fatal) error
//
// warn: indicates a warning
//
// info: indicates an information (potentially useful to the user)
//
// debug: indicates debugging information
//
// trace: indicates verbose debugging information and can be completely
//        disabled by defining ANY_LOG_NO_TRACE before including
//
// NOTE: The value ANY_LOG_ALL is not an actual level and it is used as
//       a sentinel to indicate the last value of any_log_level_t
//
typedef enum {
    ANY_LOG_PANIC,
    ANY_LOG_ERROR,
    ANY_LOG_WARN,
    ANY_LOG_INFO,
    ANY_LOG_DEBUG,
    ANY_LOG_TRACE,
    ANY_LOG_ALL,
} any_log_level_t;

// The value of ANY_LOG_MODULE is used to indicate the current module.
// By default it is defined as __FILE__, so that it will coincide with the
// source file path (relative to the compiler cwd).
//
// You can customize ANY_LOG_MODULE before including the header by simply
// defining it. For example
//
//    #define ANY_LOG_MODULE "my-library"
//    #include "any_log.h"
//
#ifndef ANY_LOG_MODULE
#define ANY_LOG_MODULE __FILE__
#endif

// C99 and later define the __func__ variable
#ifndef ANY_LOG_FUNC
#define ANY_LOG_FUNC __func__
#endif

// log_panic is implemented with the function any_log_panic, which takes
// some extra parameters compared with the other log levels. This way we can
// include as many information as possible for identifying fatal errors.
//
// You can change the format string and exit function in the implementation
// (see ANY_LOG_EXIT, ANY_LOG_PANIC_BEFORE and ANY_LOG_PANIC_AFTER).
//
// NOTE: log_panic will always terminate the program and should be used only
//       for non recoverable situations! For normal errors just use log_error
//
#define log_panic(...) any_log_panic(__FILE__, __LINE__, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)

// log_[level] provide normal printf style logging.
//
// The logs will be filtered according to the global log level. See any_log_level.
//
// You should invoke log_[level] with a format string and any number of
// matched arguments. For example
//
//    log_error("This is an error");
//    log_debug("The X is %d (padding %d)", X, 10);
//
// log_trace and log_debug can be disabled completely (to avoid their overhead
// in release/optimized builds) by defining ANY_LOG_NO_TRACE and ANY_LOG_NO_DEBUG
// respectively. As this will work only if they are defined before every header
// include, it is recommended to define this from the compiler.
//
#define log_error(...) any_log_format(ANY_LOG_ERROR, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)
#define log_warn(...)  any_log_format(ANY_LOG_WARN, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)
#define log_info(...)  any_log_format(ANY_LOG_INFO, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)

#ifdef ANY_LOG_NO_DEBUG
#define log_debug(...)
#else
#define log_debug(...) any_log_format(ANY_LOG_DEBUG, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)
#endif

#ifdef ANY_LOG_NO_TRACE
#define log_trace(...)
#else
#define log_trace(...) any_log_format(ANY_LOG_TRACE, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)
#endif

// log_value_[level] provide structured logging.
//
// The logs will be filtered according to the global log level. See any_log_level.
//
// You should always pass a message string (printf style specifiers are ignored)
// and some key-value pairs.
//
// The key are simply strings. It is advised to pass only literals for security.
//
// The value can be of type int, unsigned int, pointer (void *), double and
// string (char *).
//
// The value type is specified by a type specifier at the start of the key
// string and should be like so
//
//    key = (type_specifier ANY_LOG_VALUE_TYPE_SEP)? ...
//
// By default ANY_LOG_VALUE_TYPE_SEP is the character ':'.
//
// The possible type_specifiers are
// d (or i): int
// x (or u): unsigned int
// p: void *
// f: double
// s: char *
//
// If no type specifier is given the function will assume the type given
// by ANY_LOG_VALUE_DEFAULT_TYPE (by default string).
//
// For example
//
//    log_value_info("Created graphical context",
//                   "d:width", width,
//                   "d:height", height,
//                   "p:window", window_handle,
//                   "f:scale", scale_factor_dpi,
//                   "appname", "nice app");
//
// In the implementation you can customize the format of every key-value pair
// and of the message. This is useful if you want to adhere to a structured
// logging format like JSON. For example
//
//    #define ANY_LOG_IMPLEMENT
//    #define ANY_LOG_VALUE_BEFORE(level, module, func, message)
//    "{\"module\": \"%s\", \"function\": \"%s\", \"level\": \"%s\", \"message\": \"%s\"",
//    module, func, any_log_level_strings[level], message
//
//    #define ANY_LOG_VALUE_INT(key, value) "\"%s\": %d", key, value
//    #define ANY_LOG_VALUE_HEX(key, value) "\"%s\": %u", key, value
//    #define ANY_LOG_VALUE_PTR(key, value) "\"%s\": \"%p\"", key, value
//    #define ANY_LOG_VALUE_DOUBLE(key, value) "\"%s\": %lf", key, value
//    #define ANY_LOG_VALUE_STRING(key, value) "\"%s \": \"%s\"", key, value
//    #define ANY_LOG_VALUE_AFTER(level, module, func, message) "}\n"
//    #include "any_log.h"
//
// As with log_trace and log_debug, log_value_trace and log_value_debug can be
// disabled by defining ANY_LOG_NO_TRACE and ANY_LOG_NO_DEBUG respectively.
//
#define log_value_error(...) any_log_value(ANY_LOG_ERROR, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__, (char *)NULL)
#define log_value_warn(...)  any_log_value(ANY_LOG_WARN, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__, (char *)NULL)
#define log_value_info(...)  any_log_value(ANY_LOG_INFO, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__, (char *)NULL)
#define log_value_debug(...) any_log_value(ANY_LOG_DEBUG, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__, (char *)NULL)

#ifdef ANY_LOG_NO_DEBUG
#define log_value_debug(...)
#else
#define log_value_debug(...) any_log_value(ANY_LOG_DEBUG, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__, (char *)NULL)
#endif

#ifdef ANY_LOG_NO_TRACE
#define log_value_trace(...)
#else
#define log_value_trace(...) any_log_value(ANY_LOG_TRACE, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__, (char *)NULL)
#endif

#ifdef __GNUC__
#define ANY_LOG_ATTRIBUTE(...) __attribute__((__VA_ARGS__))
#else
#define ANY_LOG_ATTRIBUTE(...)
#endif

#ifdef __cpluscplus
extern "C" {
#endif

// All log functions will ignore the message if the level is below the
// threshold specified in any_log_level.
//
// To modify the log level you can assign a any_log_level_t to this global.
//
// By default it has value ANY_LOG_LEVEL_DEFAULT (see implementation).
//
extern any_log_level_t any_log_level;

// An array containing the strings corresponding to the log levels.
//
// Can be modified in the implementation by defining the macros [level]_STRING.
//
// The functions any_log_level_to_string and any_log_level_from_string are
// provided for easy conversion.
//
extern const char *any_log_level_strings[ANY_LOG_ALL];

ANY_LOG_ATTRIBUTE(pure)
const char *any_log_level_to_string(any_log_level_t level);

ANY_LOG_ATTRIBUTE(pure)
any_log_level_t any_log_level_from_string(const char *string);

// Colors
extern const char **any_log_colors;

extern const char *any_log_colors_default[ANY_LOG_ALL + 3];

extern const char *any_log_colors_disabled[ANY_LOG_ALL + 3];

// NOTE: You should never call the functions below directly!
//       See the above explanations on how to use logging.

ANY_LOG_ATTRIBUTE(format(printf, 4, 5))
ANY_LOG_ATTRIBUTE(nonnull(4))
void any_log_format(any_log_level_t level, const char *module,
                    const char *func, const char *format, ...);

ANY_LOG_ATTRIBUTE(nonnull(4))
void any_log_value(any_log_level_t level, const char *module,
                   const char *func, const char *message, ...);

ANY_LOG_ATTRIBUTE(noreturn)
ANY_LOG_ATTRIBUTE(format(printf, 5, 6))
ANY_LOG_ATTRIBUTE(nonnull(1, 4))
void any_log_panic(const char *file, int line, const char *module,
                   const char *func, const char *format, ...);

#ifdef __cpluscplus
}
#endif

#endif

#ifdef ANY_LOG_IMPLEMENT

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The default value for any_log_level
#ifndef ANY_LOG_LEVEL_DEFAULT
#define ANY_LOG_LEVEL_DEFAULT ANY_LOG_INFO
#endif

any_log_level_t any_log_level = ANY_LOG_LEVEL_DEFAULT;

// Log level strings
#ifndef ANY_LOG_PANIC_STRING
#define ANY_LOG_PANIC_STRING "panic"
#endif
#ifndef ANY_LOG_ERROR_STRING
#define ANY_LOG_ERROR_STRING "error"
#endif
#ifndef ANY_LOG_WARN_STRING
#define ANY_LOG_WARN_STRING "warn"
#endif
#ifndef ANY_LOG_INFO_STRING
#define ANY_LOG_INFO_STRING "info"
#endif
#ifndef ANY_LOG_DEBUG_STRING
#define ANY_LOG_DEBUG_STRING "debug"
#endif
#ifndef ANY_LOG_TRACE_STRING
#define ANY_LOG_TRACE_STRING "trace"
#endif

const char *any_log_level_strings[ANY_LOG_ALL] = {
    ANY_LOG_PANIC_STRING,
    ANY_LOG_ERROR_STRING,
    ANY_LOG_WARN_STRING,
    ANY_LOG_INFO_STRING ,
    ANY_LOG_DEBUG_STRING,
    ANY_LOG_TRACE_STRING,
};

const char *any_log_level_to_string(any_log_level_t level)
{
    return level >= ANY_LOG_PANIC && level <= ANY_LOG_TRACE
        ? any_log_level_strings[level] : "";
}

any_log_level_t any_log_level_from_string(const char *string)
{
    for (int level = ANY_LOG_PANIC; level < ANY_LOG_ALL; level++) {
        if (strcmp(any_log_level_strings[level], string) == 0)
            return (any_log_level_t)level;
    }

    return ANY_LOG_ALL;
}

const char **any_log_colors = any_log_colors_default;

// Log colors indexed by log level, with the addition of special colors
// for func, module and reset sequence.
//
#ifndef ANY_LOG_PANIC_COLOR
#define ANY_LOG_PANIC_COLOR "\x1b[1;91m"
#endif
#ifndef ANY_LOG_ERROR_COLOR
#define ANY_LOG_ERROR_COLOR "\x1b[31m"
#endif
#ifndef ANY_LOG_WARN_COLOR
#define ANY_LOG_WARN_COLOR "\x1b[1;33m"
#endif
#ifndef ANY_LOG_INFO_COLOR
#define ANY_LOG_INFO_COLOR "\x1b[1;96m"
#endif
#ifndef ANY_LOG_DEBUG_COLOR
#define ANY_LOG_DEBUG_COLOR "\x1b[1;37m"
#endif
#ifndef ANY_LOG_TRACE_COLOR
#define ANY_LOG_TRACE_COLOR "\x1b[1;90m"
#endif
#ifndef ANY_LOG_RESET_COLOR
#define ANY_LOG_RESET_COLOR "\x1b[0m"
#endif
#ifndef ANY_LOG_MODULE_COLOR
#define ANY_LOG_MODULE_COLOR ""
#endif
#ifndef ANY_LOG_FUNC_COLOR
#define ANY_LOG_FUNC_COLOR "\x1b[1m"
#endif

const char *any_log_colors_default[ANY_LOG_ALL + 3] = {
    ANY_LOG_PANIC_COLOR,
    ANY_LOG_ERROR_COLOR,
    ANY_LOG_WARN_COLOR,
    ANY_LOG_INFO_COLOR ,
    ANY_LOG_DEBUG_COLOR,
    ANY_LOG_TRACE_COLOR,
    ANY_LOG_RESET_COLOR,
    ANY_LOG_MODULE_COLOR,
    ANY_LOG_FUNC_COLOR
};

const char *any_log_colors_disabled[ANY_LOG_ALL + 3] = {
    "", "", "", "", "", "", "",
};

// Format for any_log_format (used at the start)
#ifndef ANY_LOG_FORMAT_BEFORE
#define ANY_LOG_FORMAT_BEFORE(level, module, func) \
    "[%s%s%s %s%s%s] %s%s%s: ", any_log_colors[ANY_LOG_ALL + 1], module, any_log_colors[ANY_LOG_ALL], any_log_colors[ANY_LOG_ALL + 2], \
    func, any_log_colors[ANY_LOG_ALL], any_log_colors[level], any_log_level_strings[level], any_log_colors[ANY_LOG_ALL]
#endif

// Format for any_log_format (used at the end)
#ifndef ANY_LOG_FORMAT_AFTER
#define ANY_LOG_FORMAT_AFTER(level, module, func) "\n"
#endif

void any_log_format(any_log_level_t level, const char *module,
                    const char *func, const char *format, ...)
{
    if (level > any_log_level)
        return;

    fprintf(stdout, ANY_LOG_FORMAT_BEFORE(level, module, func));

    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);

    fprintf(stdout, ANY_LOG_FORMAT_AFTER(level, module, func));

    // NOTE: Suppress compiler warning if the user customizes the format string
    //       and doesn't use these values in it
    (void)module;
    (void)func;
}

// Format for any_log_value (used at the start)
#ifndef ANY_LOG_VALUE_BEFORE
#define ANY_LOG_VALUE_BEFORE(level, module, func) \
    "[%s%s%s %s%s%s] %s%s%s: %s", any_log_colors[ANY_LOG_ALL + 1], module, any_log_colors[ANY_LOG_ALL], any_log_colors[ANY_LOG_ALL + 2], \
    func, any_log_colors[ANY_LOG_ALL], any_log_colors[level], any_log_level_strings[level], any_log_colors[ANY_LOG_ALL], message
#endif

// Format for any_log_value (used at the end)
#ifndef ANY_LOG_VALUE_AFTER
#define ANY_LOG_VALUE_AFTER(level, module, func, message) "]\n"
#endif

// This is used in the parsing of the type specifier from the key
//
// NOTE: It must be a character
#ifndef ANY_LOG_VALUE_TYPE_SEP
#define ANY_LOG_VALUE_TYPE_SEP ':'
#endif

// Format for pairs with an int value
#ifndef ANY_LOG_VALUE_INT
#define ANY_LOG_VALUE_INT(key, value) "%s=%d", key, value
#endif

// Format for pairs with an unsinged int value (hex by default)
#ifndef ANY_LOG_VALUE_HEX
#define ANY_LOG_VALUE_HEX(key, value) "%s=%#x", key, value
#endif

// Format for pairs with a pointer value
#ifndef ANY_LOG_VALUE_PTR
#define ANY_LOG_VALUE_PTR(key, value) "%s=%p", key, value
#endif

// Format for pairs with a double value
#ifndef ANY_LOG_VALUE_DOUBLE
#define ANY_LOG_VALUE_DOUBLE(key, value) "%s=%lf", key, value
#endif

// Format for pairs with a string value
#ifndef ANY_LOG_VALUE_STRING
#define ANY_LOG_VALUE_STRING(key, value) "%s=\"%s\"", key, value
#endif

// The default is to use string
#ifndef ANY_LOG_VALUE_DEFAULT
#define ANY_LOG_VALUE_DEFAULT(key, value) ANY_LOG_VALUE_STRING(key, value)
#define ANY_LOG_VALUE_DEFAULT_TYPE char *
#endif

// This is used as a separator between different pairs
#ifndef ANY_LOG_VALUE_PAIR_SEP
#define ANY_LOG_VALUE_PAIR_SEP ", "
#endif

void any_log_value(any_log_level_t level, const char *module,
                   const char *func, const char *message, ...)
{
    if (level > any_log_level)
        return;

    fprintf(stdout, ANY_LOG_VALUE_BEFORE(level, module, func, message));

    va_list args;
    va_start(args, message);

    // NOTE: This function should be called with at least one pair
    char *key = va_arg(args, char *);

    while (key != NULL) {
        if (key[0] != '\0' && key[1] == ANY_LOG_VALUE_TYPE_SEP) {
            key += 2;
            switch (key[-2]) {
                case 'd':
                case 'i':
                    fprintf(stdout, ANY_LOG_VALUE_INT(key, va_arg(args, int)));
                    break;

                case 'x':
                case 'u':
                    fprintf(stdout, ANY_LOG_VALUE_HEX(key, va_arg(args, unsigned int)));
                    break;

                case 'p':
                    fprintf(stdout, ANY_LOG_VALUE_PTR(key, va_arg(args, void *)));
                    break;

                case 'f':
                    fprintf(stdout, ANY_LOG_VALUE_DOUBLE(key, va_arg(args, double)));
                    break;

                case 's':
                    fprintf(stdout, ANY_LOG_VALUE_STRING(key, va_arg(args, char *)));
                    break;

                default:
                    goto tdefault;
            }
        } else {
tdefault:
            fprintf(stdout, ANY_LOG_VALUE_DEFAULT(key, va_arg(args, ANY_LOG_VALUE_DEFAULT_TYPE)));
        }

        key = va_arg(args, char *);
        if (key == NULL)
            break;

        fprintf(stdout, ANY_LOG_VALUE_PAIR_SEP);
    }

    va_end(args);
    fprintf(stdout, ANY_LOG_VALUE_AFTER(level, module, func, message));

    (void)module;
    (void)func;
    (void)message;
}

// Using log_panic results in a call to any_log_panic, which should terminate
// the program. The value of ANY_LOG_EXIT is used to specify an action to
// take at the end of the aforementioned function.
// By default it is abort
//
// NOTE: This function should never return!
//
#ifndef ANY_LOG_EXIT
#define ANY_LOG_EXIT(file, line, module, func) abort()
#endif

// Format for any_log_panic (used at the start)
#ifndef ANY_LOG_PANIC_BEFORE
#define ANY_LOG_PANIC_BEFORE(file, line, module, func) \
    "[%s%s%s %s%s%s] %s%s%s: ", any_log_colors[ANY_LOG_ALL + 1], module, any_log_colors[ANY_LOG_ALL], any_log_colors[ANY_LOG_ALL + 2], \
    func, any_log_colors[ANY_LOG_ALL], any_log_colors[ANY_LOG_PANIC], any_log_level_strings[ANY_LOG_PANIC], any_log_colors[ANY_LOG_ALL]
#endif

// Format for any_log_panic (used at the start)
#ifndef ANY_LOG_PANIC_AFTER
#define ANY_LOG_PANIC_AFTER(file, line, module, func) \
    "\n%spanic was invoked from%s %s:%d (%s%s%s)\n", any_log_colors[ANY_LOG_PANIC], any_log_colors[ANY_LOG_ALL], \
    file, line, any_log_colors[ANY_LOG_ALL + 1], module, any_log_colors[ANY_LOG_ALL]
#endif

// NOTE: This function *exceptionally* gets more location information
//       because we want to be specific at least for fatal errors
//
void any_log_panic(const char *file, int line, const char *module,
                   const char *func, const char *format, ...)
{
    fprintf(stdout, ANY_LOG_PANIC_BEFORE(file, line, module, func));

    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);

    fprintf(stdout, ANY_LOG_PANIC_AFTER(file, line, module, func));

    (void)module;
    (void)func;
    (void)file;
    (void)line;

    ANY_LOG_EXIT(file, line, module, func);

    // In a way or another, this function shall not return
    abort();
}

#endif

// MIT License
//
// Copyright (c) 2024 Federico Angelilli
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
