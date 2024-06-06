#include <stdbool.h>

#define ANY_LOG_IMPLEMENT
#define ANY_LOG_MODULE "test"

#define ANY_LOG_VALUE_BEFORE(level, module, func, message) \
    "{\"module\": \"%s\", \"function\": \"%s\", \"level\": \"%s\", \"message\": \"%s\", ", \
     module, func, any_log_level_strings[level], message

#define ANY_LOG_VALUE_BOOL(key, value) "\"%s\": %s", key, (value ? "true" : "false")
#define ANY_LOG_VALUE_INT(key, value) "\"%s\": %d", key, value
#define ANY_LOG_VALUE_HEX(key, value) "\"%s\": %u", key, value
#define ANY_LOG_VALUE_LONG(key, value) "\"%s\": %ld", key, value
#define ANY_LOG_VALUE_PTR(key, value) "\"%s\": \"%p\"", key, value
#define ANY_LOG_VALUE_DOUBLE(key, value) "\"%s\": %lf", key, value
#define ANY_LOG_VALUE_STRING(key, value) "\"%s \": \"%s\"", key, value
#define ANY_LOG_VALUE_CUSTOM(key, stream, formatter, value) \
    do { \
        fprintf(stream, "\"%s\": ", key); \
        formatter(stream, value); \
    } while (false)
#define ANY_LOG_VALUE_AFTER(level, module, func, message) "}\n"
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

int main()
{
    any_log_init(stdout, ANY_LOG_DEBUG);

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
                   "c:pairs", ANY_LOG_FORMATTER(pairs_format), pairs,
                   "appname", "nice app");

    // Test any_log_format

    log_trace("Hello");
    log_debug("Hello");
    log_info("Hello");
    log_warn("Hello");
    log_error("Hello");
    log_panic("Hello");

    return 0;
}
