#include <string.h>
#include <stdio.h>

#define ANY_INI_IMPLEMENT
#include "any_ini.h"

int main()
{
	const char *src =
		"ciao = 10\n"
		"global = yes\n"
		"   complex  name with space   = value  with   space  \n"
		"\n[sus]\n"
		"nice = 1\n"
		";comment\n"
		"another=10;x\n"
		"true=1   ;xx\n"
		" # comment 2 ;\n"
		"\ntry = catch 123 bool\n"
		" k e y = value pair!   ; comment\n";

	any_ini_t ini;
	any_ini_init(&ini, src, strlen(src));

	char *section = "", *key, *value;

	do {
		printf("SECTION: %s\n", section);

		while ((key = any_ini_next_key(&ini)) != NULL) {
			value = any_ini_next_value(&ini);
			printf("PAIR: `%s` = `%s`\n", key, value);
		}

	} while ((section = any_ini_next_section(&ini)) != NULL);

	return 0;
}
