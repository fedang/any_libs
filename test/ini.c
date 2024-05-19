#include <string.h>
#include <stdio.h>

#define ANY_INI_IMPLEMENT
#define ANY_INI_DELIM_COMMENT2 '#'
#include "any_ini.h"

int main()
{
	const char *src =
		/* 1*/ "ciao = 10\n"
		/* 2*/ "global = yes\n"
		/* 3*/ "   complex  name with space   = value  with   space  \n\n"
		/* 5*/ "[sus]\n"
		/* 6*/ "nice = 1\n"
		/* 7*/ ";comment\n\n"
		/* 9*/ "another=10;x\n"
		/*10*/ "true=1   ;xx\n"
		/*11*/ " # comment 2 ;\n\n"
		/*13*/ "try = catch 123 bool\n"
		/*14*/ " k e y = value pair!   ; comment\n";

	any_ini_t ini;
	any_ini_init(&ini, src, strlen(src));

	char *section = "";
	do {
		printf("%ld: SECTION \"%s\"\n", ini.line, section);

		char *key, *value;
		while ((key = any_ini_next_key(&ini)) != NULL) {
			value = any_ini_next_value(&ini);
			printf("%ld: \"%s\" = \"%s\"\n", ini.line, key, value);
		}
	} while ((section = any_ini_next_section(&ini)) != NULL);

	return 0;
}
