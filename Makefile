# Radix Tree Makefile

CC = cgcc

INCLUDES = -I.

# Compilation options:
CFLAGS = -g -Wall $(INCLUDES)

# Linking options:
LDFLAGS = -g -L.

# Libraries
LDLIBS = -lradixtree

all: radix_test

radix_test: radix_test.o libradixtree.a

radix_test.o: radix_test.c radix_tree.h

libradixtree.a: radix_tree.o
	ar rc libradixtree.a radix_tree.o
	ranlib libradixtree.a

radix_tree.o: radix_tree.c radix_tree.h

.PHONY: clean
clean:
	rm -f *.o .out *.a radix_test