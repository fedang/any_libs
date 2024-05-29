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

// String parser (provided by any_ini_t)

typedef struct {
    const char *source;
    size_t length;
    size_t cursor;
    size_t line;
} any_ini_t;

// Initialize the parser with a string.
//
void any_ini_init(any_ini_t *ini, const char *source, size_t length);

// Check if the parser has reached the end of the string.
//
bool any_ini_eof(any_ini_t *ini);

// Get the current line reached by the parser.
//
size_t any_ini_line(any_ini_t *ini);

// Get the next section.
// This function will return NULL if the parser has reached the end.
//
char *any_ini_next_section(any_ini_t *ini);

// Get the next pair key.
// This function will return NULL if the section has ended.
//
char *any_ini_next_key(any_ini_t *ini);

// Get the value for the current pair.
// This function will return NULL if nothing is found.
//
// NOTE: You should always call any_ini_next_key before this function.
//
char *any_ini_next_value(any_ini_t *ini);

// Stream parser (provided by any_ini_stream_t)
//
// Can be disabled by defining ANY_INI_NO_STREAM.

#ifndef ANY_INI_NO_STREAM

// Specify the size of the temporary line buffer used by the stream parser.
//
#ifndef ANY_INI_BUFFER_SIZE
#define ANY_INI_BUFFER_SIZE 512
#endif

// The stream parser can be initialized with a function similar to fgets.
// This function should read up to size chars from stream and stop at newlines.
//
typedef char *(*any_ini_stream_read_t)(char *string, int size, void *stream);

typedef struct {
    char buffer[ANY_INI_BUFFER_SIZE];
    size_t cursor;
    size_t line;
    any_ini_stream_read_t read;
    void *stream;
    bool eof;
} any_ini_stream_t;

// Initialize the parser with a read function and a stream.
//
void any_ini_stream_init(any_ini_stream_t *ini, any_ini_stream_read_t read, void *stream);

// Initialize the parser with a file stream.
//
// NOTE: This is just a shorthand function equivalent to
//
//       any_ini_stream_init(ini, fgets, file);
//
void any_ini_file_init(any_ini_stream_t *ini, FILE *file);

// Check if the parser has reached the end of the stream.
//
bool any_ini_stream_eof(any_ini_stream_t *ini);

// Get the current line reached by the stream parser.
//
size_t any_ini_stream_line(any_ini_stream_t *ini);

// Get the next section from the stream.
// This function will return NULL if the parser has reached the end.
//
char *any_ini_stream_next_section(any_ini_stream_t *ini);

// Get the next pair key from the stream.
// This function will return NULL if the section has ended.
//
char *any_ini_stream_next_key(any_ini_stream_t *ini);

// Get the value for the current pair from the stream.
// This function will return NULL if nothing is found.
//
// NOTE: You should always call any_ini_next_key before this function.
//
char *any_ini_stream_next_value(any_ini_stream_t *ini);

#endif

#endif

#ifdef ANY_INI_IMPLEMENT

#include <string.h>
#include <ctype.h>

// The strings returned from the parser are allocated with ANY_INI_MALLOC.
// You can change allocation strategy by defining this macro in the
// implementation file like so
//
//    #define ANY_INI_IMPLEMENT
//    #define ANY_INI_MALLOC my_malloc
//    #define ANY_INI_REALLOC my_realloc
//    #include "any_ini.h"
//
// ANY_INI_MALLOC is used by the string parser and ANY_INI_REALLOC is used
// by the stream parser. If you defined ANY_INI_NO_STREAM you can ignore
// the former function.
//
#ifndef ANY_INI_MALLOC
#include <stdlib.h>
#define ANY_INI_MALLOC malloc
#define ANY_INI_REALLOC realloc
#endif

// You can define ANY_INI_DELIM_COMMENT to specify which char starts a comment.
// By default it is ';'.
// Additionally you can specify a second character with ANY_INI_DELIM_COMMENT2.
// For example
//
//    #define ANY_INI_DELIM_COMMENT2 '#'
//    #include "any_ini.h"
//
#ifndef ANY_INI_DELIM_COMMENT
#define ANY_INI_DELIM_COMMENT ';'
#endif

// You can define ANY_INI_DELIM_PAIR to specify which char divides a key from
// the value in a pair.
// By default it is '='.
//
#ifndef ANY_INI_DELIM_PAIR
#define ANY_INI_DELIM_PAIR '='
#endif

// You can define ANY_INI_SECTION_START to specify which char starts a section.
// By default it is '['.
//
#ifndef ANY_INI_SECTION_START
#define ANY_INI_SECTION_START '['
#endif

// You can define ANY_INI_SECTION_END to specify which char ends a section.
// By default it is ']'.
//
#ifndef ANY_INI_SECTION_END
#define ANY_INI_SECTION_END ']'
#endif

// The parsers allow values to stretch multiple lines if a ANY_INI_LINE_ESCAPE
// is found just before the newline character.
// By default ANY_INI_LINE_ESCAPE is '\'.
//
// You can define the macro ANY_INI_NO_MULTILINE if you want to disable
// multiline handling.
//
#ifndef ANY_INI_LINE_ESCAPE
#define ANY_INI_LINE_ESCAPE '\\'
#endif

static size_t any_ini_trim(const char *source, size_t start, size_t end)
{
    while (isspace(source[end - 1])) end--;
    return end - start;
}

// If ANY_INI_NO_MULTILINE is defined it copies verbatim the input, otherwise
// it will remove the sequences '\\\r\n' and '\\\n' while copying
static size_t any_ini_copy(char *dest, const char *source, size_t length)
{
#ifdef ANY_INI_NO_MULTILINE
    memcpy(dest, source, length);
    return length;
#else
    size_t i = 0, j = 0;
    while (i < length) {
        if (source[i] == ANY_INI_LINE_ESCAPE) {
            if (i + 1 < length && source[i + 1] == '\n')
                i += 2;
            else if (i + 2 < length && source[i + 1] == '\r' && source[i + 2] == '\n')
                i += 3;
            continue;
        }
        dest[j++] = source[i++];
    }
    return j;
#endif
}

static char *any_ini_slice(const char *start, size_t length)
{
    char *string = ANY_INI_MALLOC(length + 1);
    if (string) {
        length = any_ini_copy(string, start, length);
        string[length] = '\0';
    }
    return string;
}

static void any_ini_skip(any_ini_t *ini)
{
    while (!any_ini_eof(ini)) {
        switch (ini->source[ini->cursor]) {
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
                if (isspace(ini->source[ini->cursor])) break;
                return;
        }
        ini->cursor++;
    }
}

static bool any_ini_skip_pair(any_ini_t *ini, bool key)
{
    switch (ini->source[ini->cursor]) {
        case '\n':
#ifndef ANY_INI_NO_MULTILINE
            if ((ini->cursor > 2 && ini->source[ini->cursor - 1] == '\r' &&
                 ini->source[ini->cursor - 2] == ANY_INI_LINE_ESCAPE) ||
                (ini->cursor > 1 && ini->source[ini->cursor - 1] == ANY_INI_LINE_ESCAPE)) {
                ini->line++;
                return true;
            }
#endif
            return false;

#ifndef ANY_INI_NO_INLINE_COMMENT
        case ANY_INI_DELIM_COMMENT:
#ifdef ANY_INI_DELIM_COMMENT2
        case ANY_INI_DELIM_COMMENT2:
#endif
            if (ini->cursor != 0 && isspace(ini->source[ini->cursor - 1]))
                return false;
            // fallthrough
#endif

        default:
            return !key || ini->source[ini->cursor] != ANY_INI_DELIM_PAIR;
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

size_t any_ini_line(any_ini_t *ini)
{
    return ini->line;
}

char *any_ini_next_section(any_ini_t *ini)
{
    any_ini_skip(ini);

    if (any_ini_eof(ini) || ini->source[ini->cursor] != ANY_INI_SECTION_START)
        return NULL;

    ++ini->cursor;
    while (!any_ini_eof(ini) && isspace(ini->source[ini->cursor]))
        ini->cursor++;
    size_t start = ini->cursor;

    while (!any_ini_eof(ini) && ini->source[ini->cursor] != '\n' && ini->source[ini->cursor] != ANY_INI_SECTION_END)
        ini->cursor++;

    size_t end = ini->cursor;
    while (!any_ini_eof(ini) && ini->source[ini->cursor] != '\n')
        ini->cursor++;

    size_t length = any_ini_trim(ini->source, start, end);
    return any_ini_slice(ini->source + start, length);
}

char *any_ini_next_key(any_ini_t *ini)
{
    any_ini_skip(ini);

    if (any_ini_eof(ini) || ini->source[ini->cursor] == ANY_INI_SECTION_START)
        return NULL;

    size_t start = ini->cursor;
    while (!any_ini_eof(ini) && any_ini_skip_pair(ini, true))
        ini->cursor++;

    size_t length = any_ini_trim(ini->source, start, ini->cursor);
    return any_ini_slice(ini->source + start, length);
}

char *any_ini_next_value(any_ini_t *ini)
{
    if (any_ini_eof(ini) || ini->source[ini->cursor] != ANY_INI_DELIM_PAIR)
        return NULL;

    ++ini->cursor;
    any_ini_skip(ini);

    size_t start = ini->cursor;
    while (!any_ini_eof(ini) && any_ini_skip_pair(ini, false))
        ini->cursor++;

    size_t length = any_ini_trim(ini->source, start, ini->cursor);
    return any_ini_slice(ini->source + start, length);
}

#ifndef ANY_INI_NO_STREAM

static void any_ini_stream_read(any_ini_stream_t *ini)
{
    ini->eof = ini->read(ini->buffer, ANY_INI_BUFFER_SIZE, ini->stream) == NULL;
    ini->cursor = 0;
}

static void any_ini_stream_skip_line(any_ini_stream_t *ini)
{
    while (!ini->eof && ini->buffer[ini->cursor] != '\n') {
        if (ini->buffer[ini->cursor] == '\0')
            any_ini_stream_read(ini);
        else
            ini->cursor++;
    }
}

static void any_ini_stream_skip(any_ini_stream_t *ini, bool comment)
{
    while (!ini->eof) {
        switch (ini->buffer[ini->cursor]) {
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
                if (isspace(ini->buffer[ini->cursor])) break;
                return;
        }
        ini->cursor++;
    }
}


static char *any_ini_stream_until(any_ini_stream_t *ini, size_t start, char c)
{
    char *tmp, *value = NULL;
    size_t size = 0;
    char prev[2] = { 0 };

    bool done = false;
    while (!ini->eof && !done) {
        switch (ini->buffer[ini->cursor]) {
            // Copy current buffer and refill
            case '\0':
                tmp = ANY_INI_REALLOC(value, size + ini->cursor - start);
                if (!tmp) return value;

                size += any_ini_copy(tmp + size, ini->buffer + start, ini->cursor - start);
                value = tmp;

                any_ini_stream_read(ini);
                start = 0;
                break;

            // Stop at line boundaries
            case '\n':
#ifndef ANY_INI_NO_MULTILINE
                if ((prev[0] == '\r' && prev[1] == '\\') || prev[0] == '\\') {
                    ini->cursor++;
                    continue;
                }
#endif
                done = true;
                break;

#ifndef ANY_INI_NO_INLINE_COMMENT
            case ANY_INI_DELIM_COMMENT:
#ifdef ANY_INI_DELIM_COMMENT2
            case ANY_INI_DELIM_COMMENT2:
#endif
                if (isspace(prev[0])) {
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

                prev[1] = prev[0];
                prev[0] = ini->buffer[ini->cursor];
                ini->cursor++;
                break;

        }
    }

    tmp = ANY_INI_REALLOC(value, size + ini->cursor - start);
    if (!tmp) return value;

    size += any_ini_copy(tmp + size, ini->buffer + start, ini->cursor - start);
    size = any_ini_trim(tmp, 0, size);
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

size_t any_ini_stream_line(any_ini_stream_t *ini)
{
    return ini->line;
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
