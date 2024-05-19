// any_ini
//
// A single-file library that provides a simple and somewhat opinionated
// interface for parsing ini files, either from a buffer or stream.
//
// To use this library you should choose a suitable file to put the
// implementation and define ANY_INI_IMPLEMENT. For example
//
//    #define ANY_INI_IMPLEMENT
//    #include "any_ini.h"
//
// Additionally, you can customize the library behavior by defining certain
// macros in the file where you put the implementation. You can see which are
// supported by reading the code guarded by ANY_INI_IMPLEMENT.
//
// This library is licensed under the terms of the MIT license.
// A copy of the license is included at the end of this file.
//

#ifndef ANY_INI_INCLUDE
#define ANY_INI_INCLUDE

#ifndef ANY_INI_MALLOC
#include <stdlib.h>
#define ANY_INI_MALLOC malloc
#endif

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    const char *source;
    size_t length;
    size_t cursor;
    size_t line;
} any_ini_t;

void any_ini_init(any_ini_t *ini, const char *source, size_t length);

bool any_ini_eof(any_ini_t *ini);

char *any_ini_next_section(any_ini_t *ini);

char *any_ini_next_key(any_ini_t *ini);

char *any_ini_next_value(any_ini_t *ini);

#ifndef ANY_INI_NO_STREAM

#ifndef ANY_INI_BUFFER_SIZE
#define ANY_INI_BUFFER_SIZE 4096
#endif

typedef char *(*any_ini_stream_read_t)(char *string, int size, void *stream);

typedef struct {
    char buffer[ANY_INI_BUFFER_SIZE];
    size_t cursor;
    size_t line;
    any_ini_stream_read_t read;
    void *stream;
    bool eof;
} any_ini_stream_t;

void any_ini_stream_init(any_ini_stream_t *ini, any_ini_stream_read_t read, void *stream);

void any_ini_file_init(any_ini_stream_t *ini, FILE *file);

bool any_ini_stream_eof(any_ini_stream_t *ini);

char *any_ini_stream_next_section(any_ini_stream_t *ini);

char *any_ini_stream_next_key(any_ini_stream_t *ini);

char *any_ini_stream_next_value(any_ini_stream_t *ini);

#endif

#endif

#ifdef ANY_INI_IMPLEMENT

#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#ifndef ANY_INI_DELIM_COMMENT
#define ANY_INI_DELIM_COMMENT ';'
#endif

#ifndef ANY_INI_DELIM_PAIR
#define ANY_INI_DELIM_PAIR '='
#endif

#ifndef ANY_INI_SECTION_START
#define ANY_INI_SECTION_START '['
#endif

#ifndef ANY_INI_SECTION_END
#define ANY_INI_SECTION_END ']'
#endif

static size_t any_ini_trim(const char *source, size_t start, size_t end)
{
    while (isspace(source[end - 1]))
        end--;

    return end - start;
}

static char *any_ini_copy(const char *start, size_t length)
{
    char *string = ANY_INI_MALLOC(length + 1);
    if (string) {
        memcpy(string, start, length);
        string[length] = '\0';
    }
    return string;
}

static void any_ini_skip(any_ini_t *ini)
{
    while (!any_ini_eof(ini)) {
        switch (ini->source[ini->cursor]) {
            case ' ':
            case '\t':
            case '\v':
            case '\r':
                break;

            case '\n':
                ini->line++;
                break;

            case ANY_INI_DELIM_COMMENT:
#ifdef ANY_INI_DELIM_COMMENT2
            case ANY_INI_DELIM_COMMENT2:
#endif
                while (!any_ini_eof(ini) && ini->source[ini->cursor] != '\n')
                    ini->cursor++;
                continue;

            default:
                return;
        }
        ini->cursor++;
    }
}

static bool any_ini_skip_pair(const char *source, size_t cursor, bool key)
{
    switch (source[cursor]) {
        case '\n':
            return false;

#ifndef ANY_INI_NO_INLINE_COMMENT
        case ANY_INI_DELIM_COMMENT:
#ifdef ANY_INI_DELIM_COMMENT2
        case ANY_INI_DELIM_COMMENT2:
#endif
            if (isspace(source[cursor - 1]))
                return false;
            // fallthrough
#endif

        default:
            return !key || source[cursor] != ANY_INI_DELIM_PAIR;
    }
}

void any_ini_init(any_ini_t *ini, const char *source, size_t length)
{
    ini->source = source;
    ini->length = length;
    ini->cursor = 0;
    ini->line = 1;
}

bool any_ini_eof(any_ini_t *ini)
{
    return ini->cursor >= ini->length;
}

char *any_ini_next_section(any_ini_t *ini)
{
    any_ini_skip(ini);

    if (any_ini_eof(ini) || ini->source[ini->cursor] != ANY_INI_SECTION_START)
        return NULL;

    size_t start = ++ini->cursor;
    while (!any_ini_eof(ini) && ini->source[ini->cursor] != '\n') {
        if (isspace(ini->source[ini->cursor])) ++start;
        ini->cursor++;
    }

    // NOTE: Does not handle the case where ANY_INI_SECTION_END is not found
    size_t end = ini->cursor;
    while (end > start && ini->source[end] != ANY_INI_SECTION_END)
        end--;

    size_t length = any_ini_trim(ini->source, start, end);
    return any_ini_copy(ini->source + start, length);
}

char *any_ini_next_key(any_ini_t *ini)
{
    any_ini_skip(ini);

    if (any_ini_eof(ini) || ini->source[ini->cursor] == ANY_INI_SECTION_START)
        return NULL;

    size_t start = ini->cursor;
    while (!any_ini_eof(ini) && any_ini_skip_pair(ini->source, ini->cursor, true))
        ini->cursor++;

    size_t length = any_ini_trim(ini->source, start, ini->cursor);
    return any_ini_copy(ini->source + start, length);
}

char *any_ini_next_value(any_ini_t *ini)
{
    if (any_ini_eof(ini) || ini->source[ini->cursor] != ANY_INI_DELIM_PAIR)
        return NULL;

    ++ini->cursor;
    any_ini_skip(ini);

    size_t start = ini->cursor;
    while (!any_ini_eof(ini) && any_ini_skip_pair(ini->source, ini->cursor, false))
        ini->cursor++;

    size_t length = any_ini_trim(ini->source, start, ini->cursor);
    return any_ini_copy(ini->source + start, length);
}

#ifndef ANY_INI_NO_STREAM

static void any_ini_stream_read(any_ini_stream_t *ini)
{
    ini->eof = ini->read(ini->buffer, ANY_INI_BUFFER_SIZE, ini->stream) == NULL;
    ini->cursor = 0;
}

static void any_ini_stream_skip(any_ini_stream_t *ini)
{
    while (!ini->eof) {
        switch (ini->buffer[ini->cursor]) {
            case ' ':
            case '\t':
            case '\v':
            case '\r':
                break;

            // Discard the current line
            case '\n':
            case ANY_INI_DELIM_COMMENT:
#ifdef ANY_INI_DELIM_COMMENT2
            case ANY_INI_DELIM_COMMENT2:
#endif
                ini->line++;
                any_ini_stream_read(ini);
                continue;

            default:
                return;
        }
        ini->cursor++;
    }
}

void any_ini_stream_init(any_ini_stream_t *ini, any_ini_stream_read_t read, void *stream)
{
    ini->cursor = 0;
    ini->line = 1;
    ini->read = read;
    ini->stream = stream;
    ini->eof = read(ini->buffer, ANY_INI_BUFFER_SIZE, stream) == NULL;
}

void any_ini_file_init(any_ini_stream_t *ini, FILE *file)
{
    any_ini_stream_init(ini, (any_ini_stream_read_t)fgets, file);
}

bool any_ini_stream_eof(any_ini_stream_t *ini)
{
    return ini->eof;
}

char *any_ini_stream_next_section(any_ini_stream_t *ini)
{
    any_ini_stream_skip(ini);

    if (ini->eof || ini->buffer[ini->cursor] != ANY_INI_SECTION_START)
        return NULL;

    size_t start = ++ini->cursor;
    while (isspace(ini->buffer[ini->cursor])) ++start;

    while (ini->buffer[ini->cursor] != '\n') ini->cursor++;

    // NOTE: Does not handle the case where ANY_INI_SECTION_END is not found
    size_t end = ini->cursor;
    while (end > start && ini->buffer[end] != ANY_INI_SECTION_END)
        end--;

    size_t length = any_ini_trim(ini->buffer, start, end);
    return any_ini_copy(ini->buffer + start, length);
}

char *any_ini_stream_next_key(any_ini_stream_t *ini)
{
    any_ini_stream_skip(ini);

    if (ini->eof || ini->buffer[ini->cursor] == ANY_INI_SECTION_START)
        return NULL;

    size_t start = ini->cursor;
    while (any_ini_skip_pair(ini->buffer, ini->cursor, true))
        ini->cursor++;

    size_t length = any_ini_trim(ini->buffer, start, ini->cursor);
    return any_ini_copy(ini->buffer + start, length);
}

char *any_ini_stream_next_value(any_ini_stream_t *ini)
{
    if (ini->eof || ini->buffer[ini->cursor] != ANY_INI_DELIM_PAIR)
        return NULL;

    ++ini->cursor;
    any_ini_stream_skip(ini);

    size_t start = ini->cursor;
    while (any_ini_skip_pair(ini->buffer, ini->cursor, false))
        ini->cursor++;

    size_t length = any_ini_trim(ini->buffer, start, ini->cursor);
    return any_ini_copy(ini->buffer + start, length);
}

#endif

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
