#ifndef ANY_CHECK_INCLUDE
#define ANY_CHECK_INCLUDE

#ifndef ANY_CHECK_PANIC
#ifdef log_panic
#define ANY_CHECK_PANIC log_panic
#else
#endif
#include <stdlib.h>
#include <stdio.h>
#define ANY_CHECK_PANIC(pred, ...) \
	do { \
		fprintf(stderr, "Check failed at %s:%d: %s\n", __FILE__, __LINE__, pred); \
		abort(); \
	} while (0)
#endif

#define ANY_CHECK_STRING(...) # __VA_ARGS__

#define check_or(pred, ...) \
	do { \
		if (!(pred)) { \
			__VA_ARGS__ ; \
		} \
	} while (0)

#define check_panic(pred, ...) check_or(pred, ANY_CHECK_PANIC(ANY_CHECK_STRING(pred), __VA_ARGS__)

#ifdef ANY_CHECK_DEBUG
#define check_debug_or check_or
#define check_debug_panic check_panic
#else
#define check_debug_or(...)
#define check_debug_panic(...)
#endif

#endif
