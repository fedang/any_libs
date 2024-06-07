#include <stdio.h>
#include <string.h>

#define ANY_SEXP_IMPLEMENT
//#define ANY_SEXP_NO_BOXING
//#define ANY_SEXP_NO_STRING
#include "any_sexp.h"

int main()
{
    const char *s = "(a b c (sub list) ())\n"
                    "(another lispy thingy)\n"
                    "() id ciao 20 a1020|x|3a\n"
                    ";comm\n3433 ;s\n"
                    "'symbol 'another 'a\n"
                    "'(a b c z) '('a) ''x  \"escape \\\"inside the string\"\n"
                    "\"string very long sus\" (\"a\" \"b\")\n"
                    "10 -20 30 41 -22345 123456789 '1\n";

    any_sexp_reader_t reader;
    any_sexp_reader_string_t string;
    any_sexp_reader_string_init(&reader, &string, s, strlen(s));

    any_sexp_t sexp = any_sexp_read(&reader);
    while (!ANY_SEXP_IS_ERROR(sexp)) {
        //printf("  %d   ", any_sexp_print(sexp));
        any_sexp_print(sexp);
        putchar('\n');
        any_sexp_free_list(sexp);
        sexp = any_sexp_read(&reader);
    }

    any_sexp_t pair = any_sexp_cons(any_sexp_number(1), any_sexp_number(2));
    any_sexp_print(pair);
    any_sexp_free_list(pair);

    //printf("%zu\n", sizeof(any_sexp_t));

    return 0;
}
