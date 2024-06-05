#include <stdio.h>
#include <string.h>

#define ANY_SEXP_IMPLEMENT
#define ANY_SEXP_NO_BOXING
#include "any_sexp.h"

int main()
{
	const char *s = "(a b c (sub list) ())\n"
		 			"(another lispy thingy)\n"
					"() id ciao 20 a1020|x|3a\n"
					";comm\n3433 ;s\n"
					"'symbol 'another 'a\n"
					"'(a b c) '('a) ''x\n"
					"\"string\" (\"a\" \"b\")\n";

	any_sexp_string_stream_t stream;
	any_sexp_parser_t parser;
	any_sexp_parser_init_string(&parser, &stream, s, strlen(s));

	any_sexp_t sexp = any_sexp_parser_next(&parser);
	while (!ANY_SEXP_IS_ERROR(sexp)) {
		any_sexp_print(sexp);
		putchar('\n');
		any_sexp_free(sexp);
		sexp = any_sexp_parser_next(&parser);
	}

	//printf("%zu\n", sizeof(any_sexp_t));

	return 0;
}
