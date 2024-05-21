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
#define ANY_INI_BUFFER_SIZE 512
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


#ifndef ANY_INI_MALLOC
#include <stdlib.h>
#define ANY_INI_MALLOC malloc
#define ANY_INI_REALLOC realloc
#endif

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
#ifdef ANY_INI_NO_MULTILINE
        memcpy(string, start, length);
#else
        size_t i, size = length;
        for (i = 0, length = 0; i < size; i++) {
            if (start[i] == '\\') {
                if (i + 1 < size && start[i + 1] == '\n')
                    i++;
                else if (i + 2 < size && start[i + 1] == '\r' && start[i + 2] == '\\')
                    i += 2;
                continue;
            }
            string[length++] = start[i];
        }
#endif
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
#ifndef ANY_INI_NO_MULTILINE
            if ((cursor > 2 && source[cursor - 1] == '\r' && source[cursor - 2] == '\\') ||
                (cursor > 1 && source[cursor - 1] == '\\'))
                return true;
#endif
            return false;

#ifndef ANY_INI_NO_INLINE_COMMENT
        case ANY_INI_DELIM_COMMENT:
#ifdef ANY_INI_DELIM_COMMENT2
        case ANY_INI_DELIM_COMMENT2:
#endif
            if (cursor != 0 && isspace(source[cursor - 1]))
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

static void any_ini_stream_skip_line(any_ini_stream_t *ini)
{
    while (true) {
        switch (ini->buffer[ini->cursor]) {
            case '\n':
                return;

            case '\0':
                any_ini_stream_read(ini);
                break;

            default:
                ini->cursor++;
                break;
        }
    }
}

static void any_ini_stream_skip(any_ini_stream_t *ini, bool comment)
{
    while (!ini->eof) {
        switch (ini->buffer[ini->cursor]) {
            case ' ':
            case '\t':
            case '\v':
            case '\r':
                break;

            case '\0':
                any_ini_stream_read(ini);
                continue;

            case ANY_INI_DELIM_COMMENT:
#ifdef ANY_INI_DELIM_COMMENT2
            case ANY_INI_DELIM_COMMENT2:
#endif
                if (!comment)
                    return;

                any_ini_stream_skip_line(ini);
                // fallthrough

            // Discard the current line
            case '\n':
                ini->line++;
                any_ini_stream_read(ini);
                continue;

            default:
                return;
        }
        ini->cursor++;
    }
}

static char *any_ini_stream_until(any_ini_stream_t *ini, size_t start, char c)
{
    char *tmp, *value = NULL;
    size_t size = 0;
    char prev = '\0';

    bool done = false;
    while (!ini->eof && !done) {
        switch (ini->buffer[ini->cursor]) {
            case '\0':
                tmp = ANY_INI_REALLOC(value, size + ini->cursor - start);
                if (!tmp) return value;

                memcpy(tmp + size, ini->buffer + start, ini->cursor - start);
                size += ini->cursor - start;
                value = tmp;

                any_ini_stream_read(ini);
                start = 0;
                break;

            // Stop at line boundaries
            case '\n':
                done = true;
                break;

#ifndef ANY_INI_NO_INLINE_COMMENT
            case ANY_INI_DELIM_COMMENT:
#ifdef ANY_INI_DELIM_COMMENT2
            case ANY_INI_DELIM_COMMENT2:
#endif
                if (isspace(prev)) {
                    done = true;
                    break;
                }
                // fallthrough
#endif
            default:
                if (ini->buffer[ini->cursor] == c) {
                    done = true;
                    break;
                }

                prev = ini->buffer[ini->cursor];
                ini->cursor++;
                break;

        }
    }

    tmp = ANY_INI_REALLOC(value, size + ini->cursor - start);
    if (!tmp) return value;

    memcpy(tmp + size, ini->buffer + start, ini->cursor - start);
    size = any_ini_trim(tmp, 0, size + ini->cursor - start);
    tmp[size] = '\0';
    return tmp;
}

void any_ini_stream_init(any_ini_stream_t *ini, any_ini_stream_read_t read, void *stream)
{
    ini->line = 1;
    ini->read = read;
    ini->stream = stream;

    // Init buffer
    memset(ini->buffer, 0, ANY_INI_BUFFER_SIZE);
    any_ini_stream_read(ini);
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
    any_ini_stream_skip(ini, true);

    if (ini->eof || ini->buffer[ini->cursor] != ANY_INI_SECTION_START)
        return NULL;

    // Skip padding
    ini->cursor++;
    any_ini_stream_skip(ini, false);

    char *section = any_ini_stream_until(ini, ini->cursor, ANY_INI_SECTION_END);
    any_ini_stream_skip_line(ini);
    return section;
}

char *any_ini_stream_next_key(any_ini_stream_t *ini)
{
    any_ini_stream_skip(ini, true);

    if (ini->eof || ini->buffer[ini->cursor] == ANY_INI_SECTION_START
                 || ini->buffer[ini->cursor] == ANY_INI_DELIM_PAIR)
        return NULL;

    return any_ini_stream_until(ini, ini->cursor, ANY_INI_DELIM_PAIR);
}

char *any_ini_stream_next_value(any_ini_stream_t *ini)
{
    if (ini->eof || ini->buffer[ini->cursor] != ANY_INI_DELIM_PAIR)
        return NULL;

    // Skip padding
    ini->cursor++;
    any_ini_stream_skip(ini, false);
    return any_ini_stream_until(ini, ini->cursor, '\n');
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
