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

//typedef struct {
//	FILE *stream;
//} any_ini_stream_t;
//
//void any_ini_stream_init(any_ini_stream_t *ini, const char *source, size_t length);

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

static inline bool any_ini_special(char c)
{
	return c == ANY_INI_DELIM_PAIR
		|| c == ANY_INI_SECTION_START
		|| c == ANY_INI_SECTION_END;
}

static size_t any_ini_trim(any_ini_t *ini, size_t start, size_t end)
{
	while (isspace(ini->source[end - 1]))
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
	while (!any_ini_eof(ini) && ini->source[ini->cursor] != '\n')
		ini->cursor++;

	// NOTE: Does not handle the case where ANY_INI_SECTION_END is not found
	size_t end = ini->cursor;
	while (end > start && ini->source[end] != ANY_INI_SECTION_END)
		end--;

	size_t length = any_ini_trim(ini, start, end);
	return any_ini_copy(ini->source + start, length);
}

char *any_ini_next_key(any_ini_t *ini)
{
	any_ini_skip(ini);

	if (any_ini_eof(ini) || any_ini_special(ini->source[ini->cursor]))
		return NULL;

	size_t start = ini->cursor;
	while (!any_ini_eof(ini)) {
		switch (ini->source[ini->cursor]) {
			case '\n':
			case ANY_INI_DELIM_COMMENT:
#ifdef ANY_INI_DELIM_COMMENT2
			case ANY_INI_DELIM_COMMENT2:
#endif
			case ANY_INI_DELIM_PAIR:
				break;

			default:
				ini->cursor++;
				continue;
		}
		break;
	}

	size_t length = any_ini_trim(ini, start, ini->cursor);
	return any_ini_copy(ini->source + start, length);
}

char *any_ini_next_value(any_ini_t *ini)
{
	if (any_ini_eof(ini) || ini->source[ini->cursor] != ANY_INI_DELIM_PAIR)
		return NULL;

	++ini->cursor;
	any_ini_skip(ini);

	size_t start = ini->cursor;
	while (!any_ini_eof(ini)) {
		switch (ini->source[ini->cursor]) {
			case '\n':
			case ANY_INI_DELIM_COMMENT:
#ifdef ANY_INI_DELIM_COMMENT2
			case ANY_INI_DELIM_COMMENT2:
#endif
				break;

			default:
				ini->cursor++;
				continue;
		}
		break;
	}

	size_t length = any_ini_trim(ini, start, ini->cursor);
	return any_ini_copy(ini->source + start, length);
}

#endif
