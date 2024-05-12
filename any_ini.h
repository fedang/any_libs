#ifndef ANY_INI_INCLUDE
#define ANY_INI_INCLUDE

#ifndef ANY_INI_MALLOC
#define ANY_INI_MALLOC malloc
#include <stdlib.h>
#endif

#include <stddef.h>

typedef struct {
	const char *source;
	size_t length;
	size_t cursor;
} any_ini_t;

void any_ini_init(any_ini_t *ini, const char *source, size_t length);

char *any_ini_next_section(any_ini_t *ini);

char *any_ini_next_key(any_ini_t *ini);

char *any_ini_next_value(any_ini_t *ini);

#endif

#ifdef ANY_INI_IMPLEMENT

#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#ifndef ANY_INI_DELIM_COMMENT
#define ANY_INI_DELIM_COMMENT ';'
#endif

#ifndef ANY_INI_DELIM_COMMENT2
#define ANY_INI_DELIM_COMMENT2 '#'
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

void any_ini_init(any_ini_t *ini, const char *source, size_t length)
{
	ini->source = source;
	ini->length = length;
	ini->cursor = 0;
}

static inline bool any_ini_more(any_ini_t *ini)
{
	return ini->cursor < ini->length;
}

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

static void any_ini_skip_comment(any_ini_t *ini)
{
	while (ini->cursor < ini->length) {
		switch (ini->source[ini->cursor]) {
			case ' ':
			case '\t':
			case '\v':
			case '\r':
			case '\n':
				break;

			case ANY_INI_DELIM_COMMENT:
#ifdef ANY_INI_DELIM_COMMENT2
			case ANY_INI_DELIM_COMMENT2:
#endif
				while (any_ini_more(ini) && ini->source[ini->cursor] != '\n')
					ini->cursor++;
				break;

			default:
				return;
		}
		ini->cursor++;
	}
}

char *any_ini_next_section(any_ini_t *ini)
{
	any_ini_skip_comment(ini);

	if (!any_ini_more(ini) || ini->source[ini->cursor] != ANY_INI_SECTION_START)
		return NULL;

	size_t start = ++ini->cursor;

	while (any_ini_more(ini) && ini->source[ini->cursor] != '\n')
		ini->cursor++;

	size_t end = ini->cursor;

	// NOTE: Does not handle the case where ANY_INI_SECTION_END is not found
	while (end > start && ini->source[end] != ANY_INI_SECTION_END)
		end--;

	size_t length = end - start;

	char *section = ANY_INI_MALLOC(length + 1);
	memcpy(section, &ini->source[start], length);
	section[length] = '\0';

	return section;
}

char *any_ini_next_key(any_ini_t *ini)
{
	any_ini_skip_comment(ini);

	if (!any_ini_more(ini) || any_ini_special(ini->source[ini->cursor]))
		return NULL;

	size_t start = ini->cursor;

	while (any_ini_more(ini)) {
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

	size_t end = ini->cursor;
	size_t length = any_ini_trim(ini, start, end);

	char *key = ANY_INI_MALLOC(length + 1);
	memcpy(key, &ini->source[start], length);
	key[length] = '\0';

	return key;
}

char *any_ini_next_value(any_ini_t *ini)
{
	if (!any_ini_more(ini) || ini->source[ini->cursor] != ANY_INI_DELIM_PAIR)
		return NULL;

	++ini->cursor;
	any_ini_skip_comment(ini);

	size_t start = ini->cursor;

	while (any_ini_more(ini)) {
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

	size_t end = ini->cursor;
	size_t length = any_ini_trim(ini, start, end);

	char *value = ANY_INI_MALLOC(length + 1);
	memcpy(value, &ini->source[start], length);
	value[length] = '\0';

	return value;
}

#endif
