#ifndef ANY_JSON_INCLUDE
#define ANY_JSON_INCLUDE

#include <stdbool.h>

typedef int (*any_json_write_fputs_t)(const char *string, void *stream);

typedef struct {
    void *stream;
    any_json_write_fputs_t fputs;
    int indent;
    bool first;
    bool pretty;
} any_json_write_t;

void any_json_write_init(any_json_write_t *json, void *stream,
                         any_json_write_fputs_t fputs, bool pretty);

bool any_json_write_open_object(any_json_write_t *json);

bool any_json_write_close_object(any_json_write_t *json);

bool any_json_write_open_array(any_json_write_t *json);

bool any_json_write_close_array(any_json_write_t *json);

bool any_json_write_member(any_json_write_t *json, const char *key);

bool any_json_write_string(any_json_write_t *json, const char *string);

bool any_json_write_number(any_json_write_t *json, double value);

bool any_json_write_bool(any_json_write_t *json, bool value);

bool any_json_write_null(any_json_write_t *json);

#endif

#ifdef ANY_JSON_IMPLEMENT

#include <string.h>
#include <ctype.h>

#ifndef ANY_JSON_INDENT
#define ANY_JSON_INDENT 4
#endif

static void any_json_write_indent(any_json_write_t *json, bool newline)
{
    if (json->pretty) {
        if (newline)
            json->fputs("\n", json->stream);

        for (int i = 0; i < json->indent; i++)
            json->fputs(" ", json->stream);
    }
}

void any_json_write_init(any_json_write_t *json, void *stream,
                         any_json_write_fputs_t fputs, bool pretty)
{
    memset(json, 0, sizeof(any_json_write_t));
    json->stream = stream;
    json->fputs = fputs;
    json->pretty = pretty;
}

bool any_json_write_open_object(any_json_write_t *json)
{
    json->indent += ANY_JSON_INDENT;
    json->first = true;
    json->fputs("{", json->stream);
}

bool any_json_write_close_object(any_json_write_t *json)
{
    json->indent -= ANY_JSON_INDENT;
    json->first = false;
    any_json_write_indent(json, true);
    json->fputs("}", json->stream);
}

bool any_json_write_open_array(any_json_write_t *json)
{
    json->indent += ANY_JSON_INDENT;
    json->first = true;
    json->fputs("[", json->stream);
}

bool any_json_write_close_array(any_json_write_t *json)
{
    json->indent -= ANY_JSON_INDENT;
    json->first = false;
    any_json_write_indent(json, true);
    json->fputs("]", json->stream);
}

bool any_json_write_member(any_json_write_t *json, const char *key)
{
    if (!json->first)
        json->fputs(",", json->stream);

    any_json_write_indent(json, true);
    json->first = false;

    if (key != NULL) {
        any_json_write_string(json, key);
        json->fputs(json->pretty ? ": " : ":", json->stream);
    }
}

#ifndef ANY_JSON_BUFFER_SIZE
#define ANY_JSON_BUFFER_SIZE 512
#endif

bool any_json_write_string(any_json_write_t *json, const char *string)
{
    json->fputs("\"", json->stream);

    char buffer[ANY_JSON_BUFFER_SIZE] = { 0 };
    size_t t = 0;

    for (size_t i = 0; string[i] != '\0'; ++i) {
        if (t + 6 >= sizeof(buffer)) {
            buffer[t] = '\0';
            json->fputs(buffer, json->stream);
            t = 0;
        }

        if (iscntrl(string[i])) {
            buffer[t++] = '\\';

            switch (string[i]) {
                case '\b':
                    buffer[t++] = 'b';
                    break;
                case '\f':
                    buffer[t++] = 'f';
                    break;
                case '\n':
                    buffer[t++] = 'n';
                    break;
                case '\r':
                    buffer[t++] = 'r';
                    break;
                case '\t':
                    buffer[t++] = 't';
                    break;
                default:
                    t += sprintf(buffer + t, "u%04x", string[i]);
                    break;
            }
        } else {
            if (string[i] == '"' || string[i] == '\\' || string[i] == '/')
                buffer[t++] = '\\';

            buffer[t++] = string[i];
        }
    }

    buffer[t] = '\0';
    json->fputs(buffer, json->stream);

    json->fputs("\"", json->stream);
}

#ifndef ANY_JSON_NUMBER_FORMAT
#define ANY_JSON_NUMBER_FORMAT "%.17g"
#endif

bool any_json_write_number(any_json_write_t *json, double value)
{
    char buffer[ANY_JSON_BUFFER_SIZE] = { 0 };
    snprintf(buffer, sizeof(buffer), ANY_JSON_NUMBER_FORMAT, value);
    json->fputs(buffer, json->stream);
}

bool any_json_write_bool(any_json_write_t *json, bool value)
{
    json->fputs(value ? "true" : "false", json->stream);
}

bool any_json_write_null(any_json_write_t *json)
{
    json->fputs("null", json->stream);
}

#endif

// MIT License
//
// Copyright (c) 2025 Federico Angelilli
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
