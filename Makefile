
SRCS = $(wildcard test/*.c)
TESTS = $(SRCS:.c=)

.PHONY: all

all: tests

tests: $(TESTS)

%: %.c
	$(CC) -I. $< -o $@ -ggdb -std=c99 -pedantic -Wall -Wextra

clean:
	rm -rf $(TESTS)
