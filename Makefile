
SRCS = $(wildcard test/*.c)
TESTS = $(SRCS:.c=)

.PHONY: all

all: tests

tests: $(TESTS)

%: %.c
	$(CC) -I. $< -o $@ -ggdb

clean:
	rm -rf $(TESTS)
