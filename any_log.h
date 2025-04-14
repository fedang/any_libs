// any_log v0.3.2
//
// A single-file library that provides a simple and somewhat opinionated
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

#include <stdio.h>
#include <stdbool.h>

// These values represent the decreasing urgency of a log invocation.
//
// - panic: indicates a fatal error and using it will result in
//          the program termination (see any_log_panic)
//
// - error: indicates a (non-fatal) error
//
// - warn: indicates a warning
//
// - info: indicates an information (potentially useful to the user)
//
// - debug: indicates debugging information
//
// - trace: indicates verbose debugging information and can be completely
//          disabled by defining ANY_LOG_NO_TRACE before including
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

// The value of ANY_LOG_CONTEXT should be used to pass some
// extra information or context to the logging functions.
// By default it is empty.
//
// Before including the header simply define your custom ANY_LOG_CONTEXT.
//
//    #define ANY_LOG_CONTEXT get_thread_name()
//    #include "any_log.h"
//
#ifndef ANY_LOG_CONTEXT
#define ANY_LOG_CONTEXT ""
#endif

// The value of ANY_LOG_MODULE is used to indicate the current module.
// By default it is defined as __FILE__, which should expand to the
// source file path (relative to the compiler cwd).
//
// You can customize ANY_LOG_MODULE before including the header by simply
// defining it. For example
//
//    #define ANY_LOG_MODULE "my-library"
//    #include "any_log.h"
//
// If you want to also include the current line, you could do something
// like the following:
//
//    #define STR2(x) #x
//    #define STR(x) STR2(x)
//    #define ANY_LOG_MODULE __FILE__ ":" STR(__LINE__)
//    #include "any_log.h"
//
#ifndef ANY_LOG_MODULE
#define ANY_LOG_MODULE __FILE__
#endif

// C99 and later defines the __func__ variable to hold the name of
// the current function.
//
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
// This function was made with a different interface compared to the other
// logging functions in order to be marked as *noreturn*.
//
// NOTE: log_panic will always terminate the program and should be used only
//       for non recoverable situations! For normal errors just use log_error.
//
#define log_panic(...) any_log_panic(__FILE__, __LINE__, ANY_LOG_CONTEXT, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)

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
#define log_error(...) any_log_format(ANY_LOG_ERROR, ANY_LOG_CONTEXT, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)
#define log_warn(...)  any_log_format(ANY_LOG_WARN,  ANY_LOG_CONTEXT, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)
#define log_info(...)  any_log_format(ANY_LOG_INFO,  ANY_LOG_CONTEXT, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)

#ifdef ANY_LOG_NO_DEBUG
#define log_debug(...)
#else
#define log_debug(...) any_log_format(ANY_LOG_DEBUG, ANY_LOG_CONTEXT, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)
#endif

#ifdef ANY_LOG_NO_TRACE
#define log_trace(...)
#else
#define log_trace(...) any_log_format(ANY_LOG_TRACE, ANY_LOG_CONTEXT, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__)
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
// type_specifier |           type               | default format
//                |                              |
//       b        | bool (promoted to int)       | "%s", b ? "true" : "false"
//      d, i      | int                          | "%d"
//      x, u      | unsigned int                 | "%#x"
//       l        | long int                     | "%ld"
//       p        | void *                       | "%p"
//       f        | double                       | "%lf"
//       s        | char * (0-terminated)        | "%s"
//
//       g        | any_log_formatter_t (function) + ANY_LOG_VALUE_GENERIC_TYPE
//
// If no type specifier is given the function will assume the type given
// by ANY_LOG_VALUE_DEFAULT_TYPE (by default string).
//
// The 'g' specifier is handled differently than the others. It needs two parameters,
// the first must be a custom formatter function (of type any_log_formatter_t) to
// format the second value of type ANY_LOG_VALUE_GENERIC_TYPE (by default void *).
// By defining ANY_LOG_NO_GENERIC you can disable this custom type specifier.
//
// Example usage of value logging
//
//    log_value_info("Created graphical context",
//                   "d:width", width,
//                   "d:height", height,
//                   "p:window", window_handle,
//                   "f:scale", scale_factor_dpi,
//                   "b:hidden", visibility == HIDDEN,
//                   "g:widgets", ANY_LOG_FORMATTER(widget_format), widgets,
//                   "appname", "nice app");
//
// In the implementation you can customize the format of every key-value pair
// and of the message. This is useful if you want to adhere to a structured
// logging format. An example implementation for JSON follows:
//
//    #define ANY_LOG_IMPLEMENT
//    #define ANY_LOG_NO_GENERIC
//
//    #define ANY_LOG_VALUE_BEFORE(stream, level, context, module, func, message)
//        fprintf(stream, "{\"module\": \"%s\", \"function\": \"%s\", \"level\": \"%s\", \"message\": \"%s\", ",
//                module, func, any_log_level_strings[level], message)
//
//    #define ANY_LOG_VALUE_BOOL(stream, key, value)
//        fprintf(stream, "\"%s\": %s", key, value ? "true" : "false")
//
//    #define ANY_LOG_VALUE_INT(stream, key, value)
//        fprintf(stream, "\"%s\": %d", key, value)
//
//    #define ANY_LOG_VALUE_HEX(stream, key, value)
//        fprintf(stream, "\"%s\": %u", key, value)
//
//    #define ANY_LOG_VALUE_LONG(stream, key, value)
//        fprintf(stream, "\"%s\": %ld", key, value)
//
//    #define ANY_LOG_VALUE_PTR(stream, key, value)
//        do {
//            if (value == NULL) fprintf(stream, "\"%s\": none", key);
//            else fprintf(stream, "\"%s\": %lu", key, (uintptr_t)value);
//        } while (false)
//
//    #define ANY_LOG_VALUE_DOUBLE(stream, key, value)
//        fprintf(stream, "\"%s\": %lf", key, value)
//
//    #define ANY_LOG_VALUE_STRING(stream, key, value)
//        fprintf(stream, "\"%s \": \"%s\"", key, value)
//
//    #define ANY_LOG_VALUE_AFTER(stream, level, context, module, func, message)
//        fprintf(stream, "}\n")
//
//    #include "any_log.h"
//
// As with log_trace and log_debug, log_value_trace and log_value_debug can be
// disabled by defining ANY_LOG_NO_TRACE and ANY_LOG_NO_DEBUG respectively.
//
#define log_value_error(...) any_log_value(ANY_LOG_ERROR, ANY_LOG_CONTEXT, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__, (char *)NULL)
#define log_value_warn(...)  any_log_value(ANY_LOG_WARN,  ANY_LOG_CONTEXT, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__, (char *)NULL)
#define log_value_info(...)  any_log_value(ANY_LOG_INFO,  ANY_LOG_CONTEXT, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__, (char *)NULL)
#define log_value_debug(...) any_log_value(ANY_LOG_DEBUG, ANY_LOG_CONTEXT, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__, (char *)NULL)

#ifdef ANY_LOG_NO_DEBUG
#define log_value_debug(...)
#else
#define log_value_debug(...) any_log_value(ANY_LOG_DEBUG, ANY_LOG_CONTEXT, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__, (char *)NULL)
#endif

#ifdef ANY_LOG_NO_TRACE
#define log_value_trace(...)
#else
#define log_value_trace(...) any_log_value(ANY_LOG_TRACE, ANY_LOG_CONTEXT, ANY_LOG_MODULE, ANY_LOG_FUNC, __VA_ARGS__, (char *)NULL)
#endif

#ifndef ANY_LOG_NO_GENERIC

#ifndef ANY_LOG_VALUE_GENERIC_TYPE
#define ANY_LOG_VALUE_GENERIC_TYPE void *
#endif

// The type of the format functions for custom types
//
typedef void (*any_log_formatter_t)(FILE *stream, ANY_LOG_VALUE_GENERIC_TYPE value);

#define ANY_LOG_FORMATTER(f) ((any_log_formatter_t)(f))

#endif

// In a multithreaded application you may encounter interleaved writes when
// different threads try to log at the same time.
// Stream locking can be used to prevent such problems.
//
// The macro ANY_LOG_FLOCK is called before any use of fprintf and should
// lock the logging stream for the current thread, while ANY_LOG_FUNLOCK
// will be called at the end of the writes to unlock it.
//
// You can define ANY_LOG_LOCKING to automatically define the aforementioned
// macros using flockfile(3) (from POSIX 2001).
//
#ifdef ANY_LOG_LOCKING

#ifndef ANY_LOG_FLOCK
#define ANY_LOG_FLOCK(stream)   flockfile(stream)
#define ANY_LOG_FUNLOCK(stream) funlockfile(stream)
#endif

#else

// Disable file locking if the macros are not defined.
//
#ifndef ANY_LOG_FLOCK
#define ANY_LOG_FLOCK(...)
#define ANY_LOG_FUNLOCK(...)
#endif

#endif

// This is a wrapper for GCC-style attributes, used by the functions below.
//
#ifdef __GNUC__
#define ANY_LOG_ATTRIBUTE(...) __attribute__((__VA_ARGS__))
#else
#define ANY_LOG_ATTRIBUTE(...)
#endif

#ifndef ANY_LOG_NORETURN
#define ANY_LOG_NORETURN ANY_LOG_ATTRIBUTE(noreturn)
#endif

// All log functions will output to the file streams specified by any_log_streams,
// depending on their log level.
//
// You should always initialize these to valid streams (eg in main) before
// invoking any_log macros or functions!
//
extern FILE *any_log_streams[ANY_LOG_ALL];

// All log functions will ignore the message if the level is below the
// threshold specified in any_log_level.
//
// To modify the log level you can assign a any_log_level_t to this global.
//
// By default it has value ANY_LOG_LEVEL_DEFAULT (see implementation).
//
extern any_log_level_t any_log_level;

// This is a simple utility function that sets both any_log_level and
// any_log_stream with a single call.
//
// The streams for all log levels will be set to the provided one.
//
// Call this function before any use of log_* (for example in main) to
// correctly initialize the library!
//
void any_log_init(any_log_level_t level, FILE *stream);

// An array containing the strings corresponding to the log levels.
//
// Can be modified in the implementation by defining the macros ANY_LOG_[level]_STRING.
//
// The functions any_log_level_to_string and any_log_level_from_string are
// provided for easy conversion.
//
extern const char *any_log_level_strings[ANY_LOG_ALL];

ANY_LOG_ATTRIBUTE(pure)
const char *any_log_level_to_string(any_log_level_t level);

ANY_LOG_ATTRIBUTE(pure)
any_log_level_t any_log_level_from_string(const char *string);

#ifndef ANY_LOG_NO_COLOR

// The default format macros for all logging function uses the global
// any_log_color to get the color sequence to use when printing the logs.
//
// By default this global points to any_log_colors_enabled, but you can
// set it to any_log_colors_disabled or to a custom array of your choice.
//
// Your custom color array should have length ANY_LOG_COLOR_ALL.
// Alternatively you can change the default colors by defining
// ANY_LOG_COLOR_[...]_DEFAULT. For example:
//
//    #define ANY_LOG_COLOR_FUNC_DEFAULT "\x1b[1m"
//    #include "any_log.h"
//
// If you changed the default format in the implementation (by redefining
// ANY_LOG_FORMAT_*, ANY_LOG_VALUE_* and ANY_LOG_PANIC_*), these variables
// can be safely ignored and can use whatever method you prefer to get the colors.
//
// In that case you may want to disable colors altogether by defining
// ANY_LOG_NO_COLOR.
//
extern const char **any_log_colors;

typedef enum {
    ANY_LOG_COLOR_PANIC,
    ANY_LOG_COLOR_ERROR,
    ANY_LOG_COLOR_WARN,
    ANY_LOG_COLOR_INFO,
    ANY_LOG_COLOR_DEBUG,
    ANY_LOG_COLOR_TRACE,
    ANY_LOG_COLOR_CONTEXT,
    ANY_LOG_COLOR_MODULE,
    ANY_LOG_COLOR_FUNC,
    ANY_LOG_COLOR_RESET,
    ANY_LOG_COLOR_ALL,
} any_log_color_t;

// Little helper for getting the colors
//
#define ANY_LOG_COLOR_GET(c) any_log_colors[c]

// This array contains the default colors.
//
// See ANY_LOG_[level]_COLOR, ANY_LOG_RESET_COLOR, ANY_LOG_MODULE_COLOR and
// ANY_LOG_FUNC_COLOR in the implementation.
//
extern const char *any_log_colors_enabled[ANY_LOG_COLOR_ALL];

// This array contains empty strings.
//
extern const char *any_log_colors_disabled[ANY_LOG_COLOR_ALL];

#endif

// NOTE: You should never call the functions below directly!
//       See the above explanations on how to use logging.

ANY_LOG_ATTRIBUTE(format(printf, 5, 6))
ANY_LOG_ATTRIBUTE(nonnull(2, 3, 4, 5))
void any_log_format(any_log_level_t level, const char *context,
                    const char *module, const char *func, const char *format, ...);

ANY_LOG_ATTRIBUTE(nonnull(2, 3, 4, 5))
void any_log_value(any_log_level_t level, const char *context,
                   const char *module, const char *func, const char *message, ...);

ANY_LOG_NORETURN
ANY_LOG_ATTRIBUTE(format(printf, 6, 7))
ANY_LOG_ATTRIBUTE(nonnull(1, 3, 4, 5, 6))
void any_log_panic(const char *file, int line, const char *context,
                   const char *module, const char *func, const char *format, ...);

#endif

#ifdef ANY_LOG_IMPLEMENT

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// For the C standard we can't assign stdout or any other streams here,
// since they are not constant.
//
// This variable MUST be properly initialized before doing any logging!
//
FILE *any_log_streams[ANY_LOG_ALL] = { 0 };

// The default value for any_log_level
#ifndef ANY_LOG_LEVEL_DEFAULT
#define ANY_LOG_LEVEL_DEFAULT ANY_LOG_INFO
#endif

any_log_level_t any_log_level = ANY_LOG_LEVEL_DEFAULT;

// Utility function to initialize the library
//
void any_log_init(any_log_level_t level, FILE *stream)
{
    any_log_level = level;
    for (int i = 0; i < ANY_LOG_ALL; i++)
        any_log_streams[i] = stream;
}

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
         ? any_log_level_strings[level] : NULL;
}

any_log_level_t any_log_level_from_string(const char *string)
{
    for (int level = ANY_LOG_PANIC; level < ANY_LOG_ALL; level++) {
        if (strcmp(any_log_level_strings[level], string) == 0)
            return (any_log_level_t)level;
    }

    return ANY_LOG_ALL;
}

#ifndef ANY_LOG_NO_COLOR

// These colors related variables are provided just to provide a uniform
// interface for setting the colors. If you decide to change the default
// log format macros, feel free to ignore all this variables.
//
const char **any_log_colors = any_log_colors_enabled;

// Log colors indexed by log level, with the addition of special colors
// for func, module and reset sequence.
//
#ifndef ANY_LOG_COLOR_PANIC_DEFAULT
#define ANY_LOG_COLOR_PANIC_DEFAULT "\x1b[1;91m"
#endif
#ifndef ANY_LOG_COLOR_ERROR_DEFAULT
#define ANY_LOG_COLOR_ERROR_DEFAULT "\x1b[1;31m"
#endif
#ifndef ANY_LOG_COLOR_WARN_DEFAULT
#define ANY_LOG_COLOR_WARN_DEFAULT "\x1b[1;33m"
#endif
#ifndef ANY_LOG_COLOR_INFO_DEFAULT
#define ANY_LOG_COLOR_INFO_DEFAULT "\x1b[1;96m"
#endif
#ifndef ANY_LOG_COLOR_DEBUG_DEFAULT
#define ANY_LOG_COLOR_DEBUG_DEFAULT "\x1b[1;37m"
#endif
#ifndef ANY_LOG_COLOR_TRACE_DEFAULT
#define ANY_LOG_COLOR_TRACE_DEFAULT "\x1b[1;90m"
#endif
#ifndef ANY_LOG_COLOR_RESET_DEFAULT
#define ANY_LOG_COLOR_RESET_DEFAULT "\x1b[0m"
#endif
#ifndef ANY_LOG_COLOR_CONTEXT_DEFAULT
#define ANY_LOG_COLOR_CONTEXT_DEFAULT ""
#endif
#ifndef ANY_LOG_COLOR_MODULE_DEFAULT
#define ANY_LOG_COLOR_MODULE_DEFAULT ""
#endif
#ifndef ANY_LOG_COLOR_FUNC_DEFAULT
#define ANY_LOG_COLOR_FUNC_DEFAULT "\x1b[1m"
#endif

const char *any_log_colors_enabled[ANY_LOG_COLOR_ALL] = {
    ANY_LOG_COLOR_PANIC_DEFAULT,
    ANY_LOG_COLOR_ERROR_DEFAULT,
    ANY_LOG_COLOR_WARN_DEFAULT,
    ANY_LOG_COLOR_INFO_DEFAULT,
    ANY_LOG_COLOR_DEBUG_DEFAULT,
    ANY_LOG_COLOR_TRACE_DEFAULT,
    ANY_LOG_COLOR_CONTEXT_DEFAULT,
    ANY_LOG_COLOR_MODULE_DEFAULT,
    ANY_LOG_COLOR_FUNC_DEFAULT,
    ANY_LOG_COLOR_RESET_DEFAULT,
};

const char *any_log_colors_disabled[ANY_LOG_COLOR_ALL] = {
    "", "", "", "", "", "", "", "", "", "",
};

#else

// Define this to not break the default format macros
//
#define ANY_LOG_COLOR_GET(c) ""

#endif

// Format for any_log_format (used at the start)
#ifndef ANY_LOG_FORMAT_BEFORE
#define ANY_LOG_FORMAT_BEFORE(stream, level, context, module, func) \
    fprintf(stream, "[%s%s%s%s%s%s%s %s%s%s] %s%s%s: ", \
            ANY_LOG_COLOR_GET(ANY_LOG_COLOR_CONTEXT), context, ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET), *context ? " " : "", \
            ANY_LOG_COLOR_GET(ANY_LOG_COLOR_MODULE), module, ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET), \
            ANY_LOG_COLOR_GET(ANY_LOG_COLOR_FUNC), func, ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET), \
            ANY_LOG_COLOR_GET(level), any_log_level_strings[level], ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET))
#endif

// Format for any_log_format (used at the end)
#ifndef ANY_LOG_FORMAT_AFTER
#define ANY_LOG_FORMAT_AFTER(stream, level, context, module, func) \
    fprintf(stream, "\n")
#endif

void any_log_format(any_log_level_t level, const char *context,
                     const char *module, const char *func, const char *format, ...)
{
    if (level > any_log_level)
        return;

    FILE *stream = any_log_streams[level];
    ANY_LOG_FLOCK(stream);

    ANY_LOG_FORMAT_BEFORE(stream, level, context, module, func);

    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);

    ANY_LOG_FORMAT_AFTER(stream, level, context, module, func);

    ANY_LOG_FUNLOCK(stream);

    // NOTE: Suppress compiler warning if the user customizes the format string
    //       and doesn't use these values in it

    (void)context;
    (void)module;
    (void)func;
}

// This is used in the parsing of the type specifier from the key
//
// NOTE: It must be a character
#ifndef ANY_LOG_VALUE_TYPE_SEP
#define ANY_LOG_VALUE_TYPE_SEP ':'
#endif

// Format for any_log_value (used at the start)
#ifndef ANY_LOG_VALUE_BEFORE
#define ANY_LOG_VALUE_BEFORE(stream, level, context, module, func, message) \
    fprintf(stream, "[%s%s%s%s%s%s%s %s%s%s] %s%s%s: %s [", \
            ANY_LOG_COLOR_GET(ANY_LOG_COLOR_CONTEXT), context, ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET), *context ? " " : "", \
            ANY_LOG_COLOR_GET(ANY_LOG_COLOR_MODULE), module, ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET), \
            ANY_LOG_COLOR_GET(ANY_LOG_COLOR_FUNC), func, ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET), \
            ANY_LOG_COLOR_GET(level), any_log_level_strings[level], ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET), message)
#endif

// Format for any_log_value (used at the end)
#ifndef ANY_LOG_VALUE_AFTER
#define ANY_LOG_VALUE_AFTER(stream, level, context, module, func, message) \
    fprintf(stream, "]\n")
#endif

// Format for pairs with a bool value
//
// NOTE: C automatically promotes boolean types to int
#ifndef ANY_LOG_VALUE_BOOL
#define ANY_LOG_VALUE_BOOL(stream, key, value) \
    fprintf(stream, "%s=%s", key, value ? "true" : "false")
#endif

// Format for pairs with an int value
#ifndef ANY_LOG_VALUE_INT
#define ANY_LOG_VALUE_INT(stream, key, value) \
    fprintf(stream, "%s=%d", key, value)
#endif

// Format for pairs with an unsigned int value (hex by default)
#ifndef ANY_LOG_VALUE_HEX
#define ANY_LOG_VALUE_HEX(stream, key, value) \
    fprintf(stream, "%s=%#x", key, value)
#endif

// Format for pairs with a long int value
#ifndef ANY_LOG_VALUE_LONG
#define ANY_LOG_VALUE_LONG(stream, key, value) \
    fprintf(stream, "%s=%ld", key, value)
#endif

// Format for pairs with a pointer value (void *)
#ifndef ANY_LOG_VALUE_PTR
#define ANY_LOG_VALUE_PTR(stream, key, value) \
    fprintf(stream, "%s=%p", key, value)
#endif

// Format for pairs with a double value
#ifndef ANY_LOG_VALUE_DOUBLE
#define ANY_LOG_VALUE_DOUBLE(stream, key, value) \
    fprintf(stream, "%s=%lf", key, value)
#endif

// Format for pairs with a string value (char *)
#ifndef ANY_LOG_VALUE_STRING
#define ANY_LOG_VALUE_STRING(stream, key, value) \
    fprintf(stream, "%s=\"%s\"", key, value)
#endif

#ifndef ANY_LOG_NO_GENERIC

// Format custom types with the given formatter function
#ifndef ANY_LOG_VALUE_GENERIC
#define ANY_LOG_VALUE_GENERIC(stream, key, value, formatter) \
    do { \
        fprintf(stream, "%s=", key); \
        formatter(stream, value); \
    } while (false)
#endif

#endif

// By default values will be interpreted as strings.
// Define ANY_LOG_VALUE_DEFAULT and ANY_LOG_VALUE_DEFAULT_TYPE to change this.
//
#ifndef ANY_LOG_VALUE_DEFAULT
#define ANY_LOG_VALUE_DEFAULT(stream, key, value) ANY_LOG_VALUE_STRING(stream, key, value)
#define ANY_LOG_VALUE_DEFAULT_TYPE char *
#endif

// This is used as a separator between different pairs
#ifndef ANY_LOG_VALUE_PAIR_SEP
#define ANY_LOG_VALUE_PAIR_SEP ", "
#endif

// NOTE: This function should be called with at least one parameter after message.
//       The log_value_[level] macros automatically add a NULL.
//
void any_log_value(any_log_level_t level, const char *context,
                   const char *module, const char *func, const char *message, ...)
{
    if (level > any_log_level)
        return;

    FILE *stream = any_log_streams[level];
    ANY_LOG_FLOCK(stream);

    ANY_LOG_VALUE_BEFORE(stream, level, context, module, func, message);

    va_list args;
    va_start(args, message);

    char *key = va_arg(args, char *);
    while (key != NULL) {
        if (key[0] != '\0' && key[1] == ANY_LOG_VALUE_TYPE_SEP) {
            key += 2;
            switch (tolower(key[-2])) {
                case 'b': {
                    int value = va_arg(args, int);
                    ANY_LOG_VALUE_BOOL(stream, key, value);
                    break;
                }

                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    ANY_LOG_VALUE_INT(stream, key, value);
                    break;
                }

                case 'x':
                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    ANY_LOG_VALUE_HEX(stream, key, value);
                    break;
                }

                case 'l': {
                    long int value = va_arg(args, long int);
                    ANY_LOG_VALUE_LONG(stream, key, value);
                    break;
                }

                case 'p': {
                    void *value = va_arg(args, void *);
                    ANY_LOG_VALUE_PTR(stream, key, value);
                    break;
                }

                case 'f': {
                    double value = va_arg(args, double);
                    ANY_LOG_VALUE_DOUBLE(stream, key, value);
                    break;
                }

                case 's': {
                    char *value = va_arg(args, char *);
                    ANY_LOG_VALUE_STRING(stream, key, value);
                    break;
                }

#ifndef ANY_LOG_NO_GENERIC
                case 'g': {
                    any_log_formatter_t formatter = va_arg(args, any_log_formatter_t);
                    ANY_LOG_VALUE_GENERIC_TYPE value = va_arg(args, ANY_LOG_VALUE_GENERIC_TYPE);
                    ANY_LOG_VALUE_GENERIC(stream, key, value, formatter);
                    break;
                }
#endif
                default:
                    goto tdefault;
            }
        } else {
tdefault:;
            ANY_LOG_VALUE_DEFAULT_TYPE value = va_arg(args, ANY_LOG_VALUE_DEFAULT_TYPE);
            ANY_LOG_VALUE_DEFAULT(stream, key, value);
        }

        key = va_arg(args, char *);
        if (key == NULL)
            break;

        fprintf(stream, ANY_LOG_VALUE_PAIR_SEP);
    }

    va_end(args);

    ANY_LOG_VALUE_AFTER(stream, level, context, module, func, message);
    ANY_LOG_FUNLOCK(stream);

    (void)context;
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
#define ANY_LOG_EXIT(file, line, context, module, func) abort()
#endif

// Format for any_log_panic (used at the start)
#ifndef ANY_LOG_PANIC_BEFORE
#define ANY_LOG_PANIC_BEFORE(stream, file, line, context, module, func) \
    fprintf(stream, "[%s%s%s%s%s%s%s %s%s%s] %s%s%s: ", \
            ANY_LOG_COLOR_GET(ANY_LOG_COLOR_CONTEXT), context, ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET), *context ? " " : "", \
            ANY_LOG_COLOR_GET(ANY_LOG_COLOR_MODULE), module, ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET), \
            ANY_LOG_COLOR_GET(ANY_LOG_COLOR_FUNC), func, ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET), \
            ANY_LOG_COLOR_GET(ANY_LOG_PANIC), any_log_level_strings[ANY_LOG_PANIC], ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET))
#endif

// Format for any_log_panic (used at the end)
#ifndef ANY_LOG_PANIC_AFTER
#define ANY_LOG_PANIC_AFTER(stream, file, line, context, module, func) \
    fprintf(stream, "\n%spanic was invoked from%s %s:%d (%s%s%s)\n", \
            ANY_LOG_COLOR_GET(ANY_LOG_PANIC), ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET), file, line,  \
            ANY_LOG_COLOR_GET(ANY_LOG_COLOR_MODULE), module, ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET))
#endif

// NOTE: This function *exceptionally* gets more location information
//       because we want to be specific at least for fatal errors
//
void any_log_panic(const char *file, int line, const char *context,
                   const char *module, const char *func, const char *format, ...)
{
    FILE *stream = any_log_streams[ANY_LOG_PANIC];
    ANY_LOG_FLOCK(stream);

    ANY_LOG_PANIC_BEFORE(stream, file, line, context, module, func);

    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);

    ANY_LOG_PANIC_AFTER(stream, file, line, context, module, func);

    ANY_LOG_FUNLOCK(stream);

    (void)file;
    (void)line;
    (void)context;
    (void)module;
    (void)func;

    ANY_LOG_EXIT(file, line, context, module, func);

    // In a way or another, this function shall not return
    abort();
}

#endif

// MIT License
//
// Copyright (c) 2024-2025 Federico Angelilli
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
