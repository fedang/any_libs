#ifndef ANY_LOG_INCLUDE
#define ANY_LOG_INCLUDE

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

void any_log_exit(const char *module, const char *func);

void any_log_format(any_log_level_t level, const char *module,
                    const char *func, const char *format, ...);


#endif

#ifdef ANY_LOG_IMPLEMENT

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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

static const char *log_level_strings[ANY_LOG_ALL] = {
    ANY_LOG_PANIC_STRING,
    ANY_LOG_ERROR_STRING,
    ANY_LOG_WARN_STRING,
    ANY_LOG_INFO_STRING ,
    ANY_LOG_DEBUG_STRING,
    ANY_LOG_TRACE_STRING,
};

// Using log_panic results in a call to any_log_exit, which should terminate
// the program. The value of ANY_LOG_PANIC is used to specify what action
// to take in any_log_exit.
// By default it is abort
//
// NOTE: That any_log_exit should never return!
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
#define ANY_LOG_FORMAT_BEFORE(level, module, func, level_strings) \
    "[%s %s] %s: ", module, func, level_strings[level]
#endif

#ifndef ANY_LOG_FORMAT_AFTER
#define ANY_LOG_FORMAT_AFTER(level, module, func, levels) "\n"
#endif

void any_log_format(any_log_level_t level, const char *module,
                    const char *func, const char *format, ...)
{
	fprintf(stdout,
            ANY_LOG_FORMAT_BEFORE(level, module, func, log_level_strings));

    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);

	fprintf(stdout,
            ANY_LOG_FORMAT_AFTER(level, module, func, log_level_strings));
}

#endif

// vim: ts=4 sw=4 et
