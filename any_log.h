#ifndef ANY_LOG_INCLUDE
#define ANY_LOG_INCLUDE

typedef enum {
    LOG_PANIC,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
    LOG_TRACE,
    LOG_ALL,
} log_level_t;

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
        log_generic(LOG_PANIC, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__); \
        log_exit(ANY_LOG_MODULE, ANY_LOG_FUNC); \
    } while (0)

#define log_error(...) \
    log_generic(LOG_ERROR, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)

#define log_warn(...)  \
    log_generic(LOG_WARN, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)

#define log_info(...)  \
    log_generic(LOG_INFO, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)

#define log_debug(...) \
    log_generic(LOG_DEBUG, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)

#ifdef ANY_LOG_NO_TRACE
#define log_trace(...)
#else
#define log_trace(...) \
    log_generic(LOG_TRACE, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)
#endif

void log_exit(const char *module, const char *func);

void log_generic(log_level_t level, const char *module,
                 const char *func, const char *format, ...);


#endif

#ifdef ANY_LOG_IMPLEMENT

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// Using log_panic results in a call to log_exit, which should terminate
// the program. The value of ANY_LOG_PANIC is used to specify what action
// to take in log_exit.
// By default it is abort
//
// NOTE: That log_exit should never return!
//
#ifndef ANY_LOG_PANIC
#define ANY_LOG_PANIC(module, func) abort()
#endif

// NOTE: This function should be called solely by the macro log_panic
void log_exit(const char *module, const char *func)
{
    ANY_LOG_PANIC(module, func);
}

#ifndef ANY_LOG_FORMAT_BEFORE
#define ANY_LOG_FORMAT_BEFORE(level, module, func, levels) \
    "[%s %s] %s: ", module, func, levels[level]
#endif

#ifndef ANY_LOG_FORMAT_AFTER
#define ANY_LOG_FORMAT_AFTER(level, module, func, levels) "\n"
#endif

void log_generic(log_level_t level, const char *module,
		         const char *func, const char *format, ...)
{
	const char *log_levels[] = {
		"panic", "error", "warn",
		"info", "debug", "trace",
	};

	fprintf(stdout, ANY_LOG_FORMAT_BEFORE(level, module, func, log_levels));

    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);

	fprintf(stdout, ANY_LOG_FORMAT_AFTER(level, module, func, log_levels));
}

#endif

// vim: ts=4 sw=4 et
