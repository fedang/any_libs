#include <stdio.h>
#include <string.h>

#define ANY_SEXP_IMPLEMENT
#include "any_sexp.h"

int main()
{
	const char *s = "(a b c (sub list) ())\n"
		 			"(another lispy thingy)\n"
					"() id ciao 20 a1020|x|3a\n"
					";comm\n3433 ;s\n"
					"'symbol 'another 'a\n"
					"'(a b c) '('a) ''x\n";

	any_sexp_parser_t parser;
	any_sexp_parser_init(&parser, s, strlen(s));

	any_sexp_t *sexp;
	while ((sexp = any_sexp_parser_next(&parser)) != ANY_SEXP_ERROR) {
		any_sexp_print(sexp);
		putchar('\n');
		any_sexp_free(sexp);
	}

	return 0;
}
