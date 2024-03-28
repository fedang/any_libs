
SRCS = $(wildcard test/*.c)
TESTS = $(SRCS:.c=)

all: tests

tests: $(TESTS)

%: %.c
	$(CC) -I. $< -o $@

clean:
	rm -rf $(TESTS)
