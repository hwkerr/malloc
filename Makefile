
CC=clang
CFLAGS=-Wall -g

SUBMIT=libmyalloc.so
BINS=libmyalloc.so myalloc
BIN2=libmyalloc-fast.so

all: $(SUBMIT)
fast: $(BIN2)

libmyalloc.so:  allocator.c
	$(CC) $(CFLAGS) -fPIC -shared allocator.c -o libmyalloc.so

libmyalloc-fast.so:
	$(CC) -O2 -DNDEBUG -Wall -fPIC -shared allocator.c -o libmyalloc.so

myalloc: shimsetter.c
	$(CC) $(CFLAGS) -o myalloc shimsetter.c

test:  test.c
	$(CC) $(CFLAGS) -o test test.c

clean:
	rm $(BINS)
