CC=gcc
CFLAGS=-Wall -Wextra -O3

all: test_parse

test_parse: parse.o test_parse.o
	$(CC) $(CFLAGS) -o test_parse parse.o test_parse.o

parse.o: parse.c parse.h
	$(CC) $(CFLAGS) -c parse.c

test_parse.o: test_parse.c parse.h
	$(CC) $(CFLAGS) -c test_parse.c

clean:
	rm -f *.o test_parse
