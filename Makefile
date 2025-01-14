
SRCS = $(wildcard test/*.c)
TESTS = $(SRCS:.c=)

.PHONY: all

all: tests

tests: $(TESTS)

%: %.c
	$(CC) -I. $< -o $@ -ggdb -std=c99

clean:
	rm -rf $(TESTS)
