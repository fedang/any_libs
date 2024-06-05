#ifndef ANY_SEXP_INCLUDE
#define ANY_SEXP_INCLUDE

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
	ANY_SEXP_TAG_ERROR  = 0xf,
	ANY_SEXP_TAG_NIL    = 0,
	ANY_SEXP_TAG_CONS   = 1 << 0,
	ANY_SEXP_TAG_SYMBOL = 1 << 1,
	ANY_SEXP_TAG_QUOTE  = 1 << 2,
} any_sexp_tag_t;

#ifdef ANY_SEXP_NO_BOXING

typedef struct any_sexp {
	any_sexp_tag_t tag;
	union {
		struct any_sexp_cons *cons;
		char *symbol;
		struct any_sexp *quote;
	};
} any_sexp_t;

#define ANY_SEXP_NIL ((any_sexp_t){ ANY_SEXP_TAG_NIL })
#define ANY_SEXP_ERROR ((any_sexp_t){ ANY_SEXP_TAG_ERROR })

#define ANY_SEXP_GET_TAG(sexp) ((sexp).tag)

#else
typedef void *any_sexp_t;

#define ANY_SEXP_NIL (any_sexp_t)NULL
#define ANY_SEXP_ERROR (any_sexp_t)UINTPTR_MAX

#define ANY_SEXP_UNTAG(sexp) (any_sexp_t)((uintptr_t)(sexp) & 0x0fffffffffffffff)
#define ANY_SEXP_TAG(sexp, tag) (any_sexp_t)((uintptr_t)(sexp) | ((uintptr_t)tag << 60))
#define ANY_SEXP_GET_TAG(sexp) (((uintptr_t)(sexp) >> 60) & 0xf)
#endif

#define ANY_SEXP_IS_TAG(sexp, tag) (ANY_SEXP_GET_TAG(sexp) == (tag))
#define ANY_SEXP_IS_ERROR(sexp) (ANY_SEXP_IS_TAG(sexp, ANY_SEXP_TAG_ERROR))
#define ANY_SEXP_IS_NIL(sexp) (ANY_SEXP_IS_TAG(sexp, ANY_SEXP_TAG_NIL))
#define ANY_SEXP_IS_CONS(sexp) (ANY_SEXP_IS_TAG(sexp, ANY_SEXP_TAG_CONS))
#define ANY_SEXP_IS_QUOTE(sexp) (ANY_SEXP_IS_TAG(sexp, ANY_SEXP_TAG_QUOTE))

typedef struct any_sexp_cons {
	any_sexp_t car;
	any_sexp_t cdr;
} any_sexp_cons_t;

typedef struct {
	const char *source;
	size_t length;
	size_t cursor;
} any_sexp_parser_t;

void any_sexp_parser_init(any_sexp_parser_t *parser, const char *source, size_t length);

bool any_sexp_parser_eof(any_sexp_parser_t *parser);

any_sexp_t any_sexp_parser_next(any_sexp_parser_t *parser);

any_sexp_t any_sexp_cons(any_sexp_t car, any_sexp_t cdr);

any_sexp_t any_sexp_car(any_sexp_t sexp);

any_sexp_t any_sexp_cdr(any_sexp_t sexp);

any_sexp_t any_sexp_reverse(any_sexp_t sexp);

void any_sexp_print(any_sexp_t sexp);

void any_sexp_free(any_sexp_t sexp);

#endif

#ifdef ANY_SEXP_IMPLEMENT

#include <ctype.h>

#ifndef ANY_SEXP_MALLOC
#include <stdlib.h>
#define ANY_SEXP_MALLOC malloc
#define ANY_SEXP_FREE free
#endif

void any_sexp_parser_init(any_sexp_parser_t *parser, const char *source, size_t length)
{
	parser->source = source;
	parser->length = length;
	parser->cursor = 0;
}

bool any_sexp_parser_eof(any_sexp_parser_t *parser)
{
	return parser->cursor >= parser->length;
}

void any_sexp_skip(any_sexp_parser_t *parser)
{
	while (!any_sexp_parser_eof(parser)) {
#ifndef ANY_SEXP_NO_COMMENT
		if (parser->source[parser->cursor] == ';') {
			while (!any_sexp_parser_eof(parser) && parser->source[parser->cursor] != '\n')
				parser->cursor++;
		}
#endif

		if (!isspace(parser->source[parser->cursor]))
			return;

		parser->cursor++;
	}
}

// Extended alphabetic characters
// ! $ % & * + - . / : < = > ? @ ^ _ ~
//
static inline bool any_sexp_issym(char c)
{
	switch (c) {
		case '!': case '$': case '%':
		case '&': case '*': case '+':
		case '-': case '.': case '/':
		case ':': case '<': case '=':
		case '>': case '?': case '@':
		case '^': case '_': case '~':

		case '|': // TODO: Multiword symbols (CL)
			return true;

		default:
			return isalnum(c);
	}
}

static any_sexp_t any_sexp_parser_symbol(any_sexp_parser_t *parser)
{
	size_t cursor = parser->cursor;

	while (!any_sexp_parser_eof(parser) && any_sexp_issym(parser->source[parser->cursor]))
		parser->cursor++;

	size_t length = parser->cursor - cursor;
	char *sym = ANY_SEXP_MALLOC(length + 1);
	memcpy(sym, parser->source + cursor, length);
	sym[length] = '\0';

#ifndef ANY_SEXP_NO_BOXING
	return ANY_SEXP_TAG(sym, ANY_SEXP_TAG_SYMBOL);
#else
	any_sexp_t sexp = {
		.tag = ANY_SEXP_TAG_SYMBOL,
		.symbol = sym,
	};
	return sexp;
#endif
}

static any_sexp_t any_sexp_parser_quote(any_sexp_parser_t *parser)
{
	// TODO: Maybe this is not the most efficient way...

	parser->cursor++;
	any_sexp_t *quote = ANY_SEXP_MALLOC(sizeof(any_sexp_t));
	if (quote == NULL) return ANY_SEXP_ERROR;
	*quote = any_sexp_parser_next(parser);

#ifndef ANY_SEXP_NO_BOXING
	return (any_sexp_t )ANY_SEXP_TAG(quote, ANY_SEXP_TAG_QUOTE);
#else
	any_sexp_t sexp = {
		.tag = ANY_SEXP_TAG_QUOTE,
		.quote = quote,
	};
	return sexp;
#endif
}

any_sexp_t any_sexp_parser_next(any_sexp_parser_t *parser)
{
	any_sexp_skip(parser);

	if (any_sexp_parser_eof(parser))
		return ANY_SEXP_ERROR;

	if (any_sexp_issym(parser->source[parser->cursor]))
		return any_sexp_parser_symbol(parser);

	if (parser->source[parser->cursor] == '\'')
		return any_sexp_parser_quote(parser);

	if (parser->source[parser->cursor] != '(')
		return ANY_SEXP_ERROR;

	parser->cursor++;
	any_sexp_t sexp = ANY_SEXP_NIL;

	any_sexp_skip(parser);
	while (!any_sexp_parser_eof(parser) && parser->source[parser->cursor] != ')') {

		any_sexp_t sub = any_sexp_parser_next(parser);
		sexp = any_sexp_cons(sub, sexp); // reversed

		if (ANY_SEXP_IS_ERROR(sub) || ANY_SEXP_IS_ERROR(sexp)) {
			any_sexp_free(sexp);
			return ANY_SEXP_ERROR;
		}

		any_sexp_skip(parser);
	}

	parser->cursor++;
	return any_sexp_reverse(sexp);
}

any_sexp_t any_sexp_cons(any_sexp_t car, any_sexp_t cdr)
{
	any_sexp_cons_t *cons = ANY_SEXP_MALLOC(sizeof(any_sexp_cons_t));
	if (cons == NULL) return ANY_SEXP_ERROR;

	cons->car = car;
	cons->cdr = cdr;

#ifndef ANY_SEXP_NO_BOXING
	return ANY_SEXP_TAG(cons, ANY_SEXP_TAG_CONS);
#else
	any_sexp_t sexp = {
		.tag = ANY_SEXP_TAG_CONS,
		.cons = cons,
	};
	return sexp;
#endif
}

any_sexp_t any_sexp_car(any_sexp_t sexp)
{
	if (!ANY_SEXP_IS_CONS(sexp))
		return ANY_SEXP_ERROR;

#ifndef ANY_SEXP_NO_BOXING
	any_sexp_cons_t *cons = (any_sexp_cons_t *)ANY_SEXP_UNTAG(sexp);
	return cons->car;
#else
	return sexp.cons->car;
#endif
}

any_sexp_t any_sexp_cdr(any_sexp_t sexp)
{
	if (!ANY_SEXP_IS_CONS(sexp))
		return ANY_SEXP_ERROR;

#ifndef ANY_SEXP_NO_BOXING
	any_sexp_cons_t *cons = (any_sexp_cons_t *)ANY_SEXP_UNTAG(sexp);
	return cons->cdr;
#else
	return sexp.cons->cdr;
#endif
}

any_sexp_t any_sexp_reverse(any_sexp_t sexp)
{
	if (ANY_SEXP_IS_NIL(sexp))
		return sexp;

	if (!ANY_SEXP_IS_CONS(sexp))
		return ANY_SEXP_ERROR;

#ifndef ANY_SEXP_NO_BOXING
	any_sexp_cons_t *cons = (any_sexp_cons_t *)ANY_SEXP_UNTAG(sexp);
	any_sexp_t prev = ANY_SEXP_NIL, next = ANY_SEXP_NIL;

	while (cons != NULL) {
		if (!ANY_SEXP_IS_NIL(cons) && !ANY_SEXP_IS_CONS(cons))
			return ANY_SEXP_ERROR;

		next = (any_sexp_cons_t *)ANY_SEXP_UNTAG(cons->cdr);
		cons->cdr = prev;
		prev = ANY_SEXP_TAG(cons, ANY_SEXP_TAG_CONS);
		cons = next;
	}
#else
	any_sexp_t cons = sexp, prev = ANY_SEXP_NIL, next = ANY_SEXP_NIL;

	while (cons.cons != NULL) {
		if (!ANY_SEXP_IS_NIL(cons) && !ANY_SEXP_IS_CONS(cons))
			return ANY_SEXP_ERROR;

		memcpy(&next, &cons.cons->cdr, sizeof(any_sexp_t));
		memcpy(&cons.cons->cdr, &prev, sizeof(any_sexp_t));
		memcpy(&prev, &cons, sizeof(any_sexp_t));
		memcpy(&cons, &next, sizeof(any_sexp_t));
	}
#endif

	return prev;
}

void any_sexp_print(any_sexp_t sexp)
{
	switch (ANY_SEXP_GET_TAG(sexp)) {
		case ANY_SEXP_TAG_ERROR:
			printf("<error>");
			break;

		case ANY_SEXP_TAG_NIL:
			printf("()");
			break;

		case ANY_SEXP_TAG_CONS:
			putchar('(');
			while (!ANY_SEXP_IS_NIL(sexp) && !ANY_SEXP_IS_ERROR(sexp)) {
				any_sexp_print(any_sexp_car(sexp));
				sexp = any_sexp_cdr(sexp);
				if (!ANY_SEXP_IS_NIL(sexp)) putchar(' ');
			}
			putchar(')');
			break;

		case ANY_SEXP_TAG_SYMBOL:
#ifndef ANY_SEXP_NO_BOXING
			printf("%s", (char *)ANY_SEXP_UNTAG(sexp));
#else
			printf("%s", sexp.symbol);
#endif
			break;

		case ANY_SEXP_TAG_QUOTE:
			putchar('\'');
#ifndef ANY_SEXP_NO_BOXING
			any_sexp_print(*(any_sexp_t *)ANY_SEXP_UNTAG(sexp));
#else
			any_sexp_print(*sexp.quote);
#endif
			break;
	}
}

void any_sexp_free(any_sexp_t sexp)
{
	switch (ANY_SEXP_GET_TAG(sexp)) {
		case ANY_SEXP_TAG_NIL:
		case ANY_SEXP_TAG_ERROR:
			break;

		case ANY_SEXP_TAG_CONS:
			any_sexp_free(any_sexp_car(sexp));
			any_sexp_free(any_sexp_cdr(sexp));
#ifndef ANY_SEXP_NO_BOXING
			ANY_SEXP_FREE(ANY_SEXP_UNTAG(sexp));
#endif
			break;

		case ANY_SEXP_TAG_SYMBOL:
#ifndef ANY_SEXP_NO_BOXING
			ANY_SEXP_FREE(ANY_SEXP_UNTAG(sexp));
#else
			ANY_SEXP_FREE(sexp.symbol);
#endif
			break;

		case ANY_SEXP_TAG_QUOTE:
#ifndef ANY_SEXP_NO_BOXING
			any_sexp_free(*(any_sexp_t *)ANY_SEXP_UNTAG(sexp));
			ANY_SEXP_FREE(ANY_SEXP_UNTAG(sexp));
#else
			any_sexp_free(*sexp.quote);
#endif
			break;
	}
}

#endif
