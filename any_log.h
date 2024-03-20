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
//        the program termination (see any_log_exit)
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

#define log_panic(...) \
    do { \
        any_log_format(ANY_LOG_PANIC, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__); \
        any_log_exit(ANY_LOG_MODULE, ANY_LOG_FUNC); \
    } while (0)

#define log_error(...) \
    any_log_format(ANY_LOG_ERROR, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)

#define log_warn(...)  \
    any_log_format(ANY_LOG_WARN, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)

#define log_info(...)  \
    any_log_format(ANY_LOG_INFO, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)

#define log_debug(...) \
    any_log_format(ANY_LOG_DEBUG, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)

#ifdef ANY_LOG_NO_TRACE
#define log_trace(...)
#else
#define log_trace(...) \
    any_log_format(ANY_LOG_TRACE, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)
#endif

#ifdef __GNUC__
#define ANY_LOG_ATTRIBUTE(...) __attribute__((__VA_ARGS__))
#else
#define ANY_LOG_ATTRIBUTE(...)
#endif

extern const char *any_log_level_strings[ANY_LOG_ALL];

ANY_LOG_ATTRIBUTE(pure)
const char *any_log_level_to_string(any_log_level_t level);

ANY_LOG_ATTRIBUTE(pure)
any_log_level_t any_log_level_from_string(const char *string);

ANY_LOG_ATTRIBUTE(noreturn)
void any_log_exit(const char *module, const char *func);

ANY_LOG_ATTRIBUTE(format(printf, 4, 5))
void any_log_format(any_log_level_t level, const char *module,
                    const char *func, const char *format, ...);

#endif

#ifdef ANY_LOG_IMPLEMENT

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    for (any_log_level_t level = ANY_LOG_PANIC; level < ANY_LOG_ALL; level++) {
        if (strcmp(any_log_level_strings[level], string) == 0)
            return level;
    }

    return ANY_LOG_ALL;
}

// Using log_panic results in a call to any_log_exit, which should terminate
// the program. The value of ANY_LOG_PANIC is used to specify what action
// to take in any_log_exit.
// By default it is abort
//
// NOTE: The function any_log_exit should never return!
//
#ifndef ANY_LOG_PANIC
#define ANY_LOG_PANIC(module, func) abort()
#endif

// NOTE: This function should be called solely by the macro log_panic
void any_log_exit(const char *module, const char *func)
{
    ANY_LOG_PANIC(module, func);
}

#ifndef ANY_LOG_FORMAT_BEFORE
#define ANY_LOG_FORMAT_BEFORE(level, module, func) \
    "[%s %s] %s: ", module, func, any_log_level_strings[level]
#endif

#ifndef ANY_LOG_FORMAT_AFTER
#define ANY_LOG_FORMAT_AFTER(level, module, func) "\n"
#endif

void any_log_format(any_log_level_t level, const char *module,
                    const char *func, const char *format, ...)
{
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
