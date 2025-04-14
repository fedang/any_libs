#define _POSIX_SOURCE

#include <stdbool.h>
#include <stdint.h>

#if __GNUC__

#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <unistd.h>

void print_backtrace(FILE *stream)
{
    int nptrs;
    void *buffer[100];

    nptrs = backtrace(buffer, 100);
    fprintf(stream, "\nbacktrace() returned %d addresses\n", nptrs);
    backtrace_symbols_fd(buffer, nptrs, fileno(stream));
}

#define ANY_LOG_PANIC_AFTER(stream, file, line, context, module, func) \
    do { \
        print_backtrace(stream); \
        fprintf(stream, "%spanic was invoked from%s %s:%d (module %s%s%s)\n", \
                ANY_LOG_COLOR_GET(ANY_LOG_PANIC), ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET), file, line,  \
                ANY_LOG_COLOR_GET(ANY_LOG_COLOR_MODULE), module, ANY_LOG_COLOR_GET(ANY_LOG_COLOR_RESET)); \
    } while (false)

#endif

//#define ANY_LOG_LOCKING
#define ANY_LOG_IMPLEMENT
//#define ANY_LOG_MODULE "test"

#define STR2(x) #x
#define STR(x) STR2(x)
#define ANY_LOG_SOURCE __FILE__ ":" STR(__LINE__)

// Print in a JSON like way

#define ANY_LOG_VALUE_BEFORE(stream, level, context, module, func, message) \
    fprintf(stream, "{\"module\": \"%s\", \"function\": \"%s\", \"level\": \"%s\", \"message\": \"%s\", ", \
            module, func, any_log_level_strings[level], message)

#define ANY_LOG_VALUE_BOOL(stream, key, value) \
    fprintf(stream, "\"%s\": %s", key, value ? "true" : "false")

#define ANY_LOG_VALUE_INT(stream, key, value) \
    fprintf(stream, "\"%s\": %d", key, value)

#define ANY_LOG_VALUE_HEX(stream, key, value) \
    fprintf(stream, "\"%s\": %u", key, value)

#define ANY_LOG_VALUE_LONG(stream, key, value) \
    fprintf(stream, "\"%s\": %ld", key, value)

#define ANY_LOG_VALUE_PTR(stream, key, value) \
    do { \
        if (value == NULL) fprintf(stream, "\"%s\": none", key); \
        else fprintf(stream, "\"%s\": %lu", key, (uintptr_t)value); \
    } while (false)

#define ANY_LOG_VALUE_DOUBLE(stream, key, value) \
    fprintf(stream, "\"%s\": %lf", key, value)

#define ANY_LOG_VALUE_STRING(stream, key, value) \
    fprintf(stream, "\"%s \": \"%s\"", key, value)

#define ANY_LOG_VALUE_GENERIC(stream, key, value, formatter) \
    do { \
        fprintf(stream, "\"%s\": \"", key); \
        formatter(stream, value); \
        fprintf(stream, "\""); \
    } while (false)

#define ANY_LOG_VALUE_AFTER(stream, level, context, module, func, message) \
    fprintf(stream, "}\n")

#include "any_log.h"


struct pair {
    const char *s1, *s2;
};

void pairs_format(FILE *stream, struct pair *pairs)
{
    fprintf(stream, "[");
    while (pairs->s1 && pairs->s2) {
        fprintf(stream, "%s -> %s", pairs->s1, pairs->s2);
        pairs++;
        if (pairs->s1 && pairs->s2)
            fprintf(stream, ", ");
    }
    fprintf(stream, "]");
}

void adios(int x)
{
    if (x == 0) {
        log_panic("Adios");
    }

    adios(x - 1);
}

int main()
{
    any_log_init(ANY_LOG_TRACE, stdout);

    FILE *streams[ANY_LOG_ALL] = {
        stderr, //ANY_LOG_PANIC
        stderr, //ANY_LOG_ERROR
        stdout, //ANY_LOG_WARN
        stdout, //ANY_LOG_INFO
        stdout, //ANY_LOG_DEBUG
        stdout, //ANY_LOG_TRACE
    };
    memcpy(any_log_streams, streams, sizeof(streams));

    // Test any_log_level_to_string
    log_trace("ANY_LOG_PANIC = %s", any_log_level_to_string(ANY_LOG_PANIC));
    log_trace("ANY_LOG_ERROR = %s", any_log_level_to_string(ANY_LOG_ERROR));
    log_trace("ANY_LOG_WARN = %s", any_log_level_to_string(ANY_LOG_WARN));
    log_trace("ANY_LOG_INFO = %s", any_log_level_to_string(ANY_LOG_INFO));
    log_trace("ANY_LOG_DEBUG = %s", any_log_level_to_string(ANY_LOG_DEBUG));
    log_trace("ANY_LOG_TRACE = %s", any_log_level_to_string(ANY_LOG_TRACE));
    log_trace("ANY_LOG_ALL = %s", any_log_level_to_string(ANY_LOG_ALL));

    // Test any_log_level_from_string
    log_trace("ANY_LOG_PANIC = %d = %d", ANY_LOG_PANIC,
            any_log_level_from_string(ANY_LOG_PANIC_STRING));

    log_trace("ANY_LOG_ERROR = %d = %d", ANY_LOG_ERROR,
            any_log_level_from_string(ANY_LOG_ERROR_STRING));

    log_trace("ANY_LOG_WARN = %d = %d", ANY_LOG_WARN,
            any_log_level_from_string(ANY_LOG_WARN_STRING));

    log_trace("ANY_LOG_INFO = %d = %d", ANY_LOG_INFO,
            any_log_level_from_string(ANY_LOG_INFO_STRING));

    log_trace("ANY_LOG_DEBUG = %d = %d", ANY_LOG_DEBUG,
            any_log_level_from_string(ANY_LOG_DEBUG_STRING));

    log_trace("ANY_LOG_TRACE = %d = %d", ANY_LOG_TRACE,
            any_log_level_from_string(ANY_LOG_TRACE_STRING));

    // Test any_log_value

    log_value_warn("Hello",
            "this is a", "string");

    log_value_info("I'll try",
            "d:this is ", 10,
            "f:dbl", 20.3333,
            "p:a", NULL);

    struct pair pairs[] = {
        { "v", "v2" },
        { "p", "xp" },
        { "23", "42" },
        { NULL, NULL },
    };

    log_value_info("Created graphical context",
                   "d:width", 100,
                   "d:height", 200,
                   "p:window", NULL,
                   "f:scale", 1.23,
                   "b:hidden", true,
                   "p:ptr", malloc(0),
                   "g:pairs", ANY_LOG_FORMATTER(pairs_format), pairs,
                   "appname", "nice app");

    // Test any_log_format

    log_trace("Hello");
    log_debug("Hello");
    log_info("Hello");
    log_warn("Hello");
    log_error("Hello");

    adios(10);

    return 0;
}
