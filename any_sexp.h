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
} any_sexp_tag_t;

#ifdef ANY_SEXP_NO_BOXING

typedef struct any_sexp {
	any_sexp_tag_t tag;
	union {
		struct any_sexp_cons *cons;
		char *symbol;
	};
} any_sexp_t;

#define ANY_SEXP_ERROR (any_sexp_error())
#define ANY_SEXP_NIL   (any_sexp_nil())

#define ANY_SEXP_GET_TAG(sexp)    ((sexp).tag)
#define ANY_SEXP_GET_CONS(sexp)   ((sexp).cons)
#define ANY_SEXP_GET_SYMBOL(sexp) ((sexp).symbol)

#else

typedef void *any_sexp_t;

#define ANY_SEXP_ERROR (any_sexp_t)UINTPTR_MAX
#define ANY_SEXP_NIL   (any_sexp_t)NULL

#define ANY_SEXP_BITS_SHIFT  (sizeof(uintptr_t) * 8 - 4)
#define ANY_SEXP_UNTAG(sexp) (any_sexp_t)((uintptr_t)(sexp) & (UINTPTR_MAX & ~((uintptr_t)0xf << ANY_SEXP_BITS_SHIFT)))

#define ANY_SEXP_TAG(sexp, tag)   (any_sexp_t)((uintptr_t)(sexp) | ((uintptr_t)tag << ANY_SEXP_BITS_SHIFT))
#define ANY_SEXP_GET_TAG(sexp)    (((uintptr_t)(sexp) >> ANY_SEXP_BITS_SHIFT) & 0xf)
#define ANY_SEXP_GET_CONS(sexp)   ((any_sexp_cons_t *)ANY_SEXP_UNTAG(sexp))
#define ANY_SEXP_GET_SYMBOL(sexp) (((char *)ANY_SEXP_UNTAG(sexp)))

#endif

#define ANY_SEXP_IS_TAG(sexp, tag) (ANY_SEXP_GET_TAG(sexp) == (tag))
#define ANY_SEXP_IS_ERROR(sexp)    (ANY_SEXP_IS_TAG(sexp, ANY_SEXP_TAG_ERROR))
#define ANY_SEXP_IS_NIL(sexp)      (ANY_SEXP_IS_TAG(sexp, ANY_SEXP_TAG_NIL))
#define ANY_SEXP_IS_CONS(sexp)     (ANY_SEXP_IS_TAG(sexp, ANY_SEXP_TAG_CONS))

// NOTE: Use it only after checking the tag!
#define ANY_SEXP_GET_CAR(sexp) (ANY_SEXP_GET_CONS(sexp)->car)
#define ANY_SEXP_GET_CDR(sexp) (ANY_SEXP_GET_CONS(sexp)->cdr)

typedef struct any_sexp_cons {
	any_sexp_t car;
	any_sexp_t cdr;
} any_sexp_cons_t;

#ifndef ANY_SEXP_SYMBOL_MAX
#define ANY_SEXP_SYMBOL_MAX 256
#endif

typedef char (*any_sexp_getchar_t)(void *stream);

typedef struct {
	const char *source;
	size_t length;
	size_t cursor;
} any_sexp_string_stream_t;

char any_sexp_string_getc(any_sexp_string_stream_t *stream);

typedef struct {
	any_sexp_getchar_t getc;
	void *stream;
	char c;
} any_sexp_parser_t;

void any_sexp_parser_init(any_sexp_parser_t *parser, any_sexp_getchar_t getc, void *stream);

void any_sexp_parser_init_string(any_sexp_parser_t *parser, any_sexp_string_stream_t *stream, const char *source, size_t length);

bool any_sexp_parser_eof(any_sexp_parser_t *parser);

any_sexp_t any_sexp_parser_next(any_sexp_parser_t *parser);

any_sexp_t any_sexp_error(void);

any_sexp_t any_sexp_nil(void);

any_sexp_t any_sexp_symbol(char *symbol, size_t length);

any_sexp_t any_sexp_quote(any_sexp_t sexp);

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

#ifndef ANY_SEXP_CHAR_COMMENT
#define ANY_SEXP_CHAR_COMMENT ';'
#endif

#ifndef ANY_SEXP_CHAR_QUOTE
#define ANY_SEXP_CHAR_QUOTE '\''
#endif

#ifndef ANY_SEXP_QUOTE_SYMBOL
#define ANY_SEXP_QUOTE_SYMBOL "quote"
#endif

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

static inline char any_sexp_parser_advance(any_sexp_parser_t *parser)
{
	parser->c = parser->getc(parser->stream);
	return parser->c;
}

static void any_sexp_parser_skip(any_sexp_parser_t *parser)
{
	while (!any_sexp_parser_eof(parser)) {
#ifndef ANY_SEXP_NO_COMMENT
		if (parser->c == ANY_SEXP_CHAR_COMMENT) {
			while (!any_sexp_parser_eof(parser) && parser->c != '\n')
				any_sexp_parser_advance(parser);
		}
#endif
		if (!isspace(parser->c))
			return;

		any_sexp_parser_advance(parser);
	}
}

void any_sexp_parser_init(any_sexp_parser_t *parser, any_sexp_getchar_t getc, void *stream)
{
	parser->getc = getc;
	parser->stream = stream;
	any_sexp_parser_advance(parser);
}

void any_sexp_parser_init_string(any_sexp_parser_t *parser, any_sexp_string_stream_t *stream, const char *source, size_t length)
{
	stream->source = source;
	stream->length = length;
	stream->cursor = 0;
	any_sexp_parser_init(parser, (any_sexp_getchar_t)any_sexp_string_getc, stream);
}

bool any_sexp_parser_eof(any_sexp_parser_t *parser)
{
	return parser->c == EOF;
}

any_sexp_t any_sexp_parser_next(any_sexp_parser_t *parser)
{
	any_sexp_parser_skip(parser);

	// Symbol
	if (any_sexp_issym(parser->c)) {
		char buffer[ANY_SEXP_SYMBOL_MAX + 1];
		size_t length = 0;

		do {
			if (length < ANY_SEXP_SYMBOL_MAX)
				buffer[length++] = parser->c;

			any_sexp_parser_advance(parser);
		} while (any_sexp_issym(parser->c));

		return any_sexp_symbol(buffer, length);
	}

#ifndef ANY_SEXP_NO_QUOTE
	// Quote
	if (parser->c == ANY_SEXP_CHAR_QUOTE) {
		any_sexp_parser_advance(parser);
		return any_sexp_quote(any_sexp_parser_next(parser));
	}
#endif

	// List
	if (parser->c == '(') {
		any_sexp_parser_advance(parser);
		any_sexp_parser_skip(parser);

		any_sexp_t sexp = ANY_SEXP_NIL;
		while (!any_sexp_parser_eof(parser) && parser->c != ')') {
			any_sexp_t sub = any_sexp_parser_next(parser);
			sexp = any_sexp_cons(sub, sexp); // reversed

			if (ANY_SEXP_IS_ERROR(sub) || ANY_SEXP_IS_ERROR(sexp)) {
				any_sexp_free(sexp);
				return ANY_SEXP_ERROR;
			}

			any_sexp_parser_skip(parser);
		}

		any_sexp_parser_advance(parser);
		return any_sexp_reverse(sexp);
	}

	return ANY_SEXP_ERROR;
}

char any_sexp_string_getc(any_sexp_string_stream_t *stream)
{
	if (stream->cursor >= stream->length)
		return EOF;

	return stream->source[stream->cursor++];
}

any_sexp_t any_sexp_error(void)
{
#ifndef ANY_SEXP_NO_BOXING
	return ANY_SEXP_ERROR;
#else
	any_sexp_t sexp = {
		.tag = ANY_SEXP_TAG_ERROR,
	};
	return sexp;
#endif
}

any_sexp_t any_sexp_nil(void)
{
#ifndef ANY_SEXP_NO_BOXING
	return ANY_SEXP_NIL;
#else
	any_sexp_t sexp = {
		.tag = ANY_SEXP_TAG_NIL,
	};
	return sexp;
#endif
}

any_sexp_t any_sexp_symbol(char *symbol, size_t length)
{
	if (symbol == NULL)
		return ANY_SEXP_ERROR;

	char *sym = ANY_SEXP_MALLOC(length + 1);
	if (sym == NULL)
		return ANY_SEXP_ERROR;

	memcpy(sym, symbol, length);
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

any_sexp_t any_sexp_quote(any_sexp_t sexp)
{
	any_sexp_t quote = any_sexp_symbol(ANY_SEXP_QUOTE_SYMBOL, strlen(ANY_SEXP_QUOTE_SYMBOL));
	return any_sexp_cons(quote, any_sexp_cons(sexp, ANY_SEXP_NIL));
}

any_sexp_t any_sexp_cons(any_sexp_t car, any_sexp_t cdr)
{
	any_sexp_cons_t *cons = ANY_SEXP_MALLOC(sizeof(any_sexp_cons_t));
	if (cons == NULL)
		return ANY_SEXP_ERROR;

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
	return ANY_SEXP_GET_CAR(sexp);
}

any_sexp_t any_sexp_cdr(any_sexp_t sexp)
{
	if (!ANY_SEXP_IS_CONS(sexp))
		return ANY_SEXP_ERROR;
	return ANY_SEXP_GET_CDR(sexp);
}

any_sexp_t any_sexp_reverse(any_sexp_t sexp)
{
	if (ANY_SEXP_IS_NIL(sexp))
		return sexp;

	if (!ANY_SEXP_IS_CONS(sexp))
		return ANY_SEXP_ERROR;

	any_sexp_t cons = sexp,
			   prev = ANY_SEXP_NIL,
			   next = ANY_SEXP_NIL;

	while (ANY_SEXP_GET_CONS(cons) != NULL) {
		if (!ANY_SEXP_IS_NIL(cons) && !ANY_SEXP_IS_CONS(cons))
			return ANY_SEXP_ERROR;

		memcpy(&next, &ANY_SEXP_GET_CDR(cons), sizeof(any_sexp_t));
		memcpy(&ANY_SEXP_GET_CDR(cons), &prev, sizeof(any_sexp_t));
		memcpy(&prev, &cons, sizeof(any_sexp_t));
		memcpy(&cons, &next, sizeof(any_sexp_t));
	}

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
			printf("%s", ANY_SEXP_GET_SYMBOL(sexp));
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
			ANY_SEXP_FREE(ANY_SEXP_GET_CONS(sexp));
			break;

		case ANY_SEXP_TAG_SYMBOL:
			ANY_SEXP_FREE(ANY_SEXP_GET_SYMBOL(sexp));
			break;
	}
}

#endif
