sources=$(wildcard *.c)

CFLAGS=-Wall -O0 -g --std=c99 -Werror -pedantic
LDFLAGS=

objects=$(sources:.c=.o)

test: $(objects)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(objects): $(wildcard *.h)

.PHONY: clean
clean:
	$(RM) *.o test
