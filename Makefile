CC = gcc
CFLAGS = -ansi -Wall -Wextra -Werror -pedantic-errors
LDLIBS = -lm

symnmf: symnmf.o
	$(CC) $(CFLAGS) -o symnmf symnmf.o $(LDLIBS)

symnmf.o: symnmf.c symnmf.h
	$(CC) $(CFLAGS) -c symnmf.c

.PHONY: clean
clean:
	rm -f symnmf symnmf.o
