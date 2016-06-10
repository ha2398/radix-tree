# Radix Tree Makefile

CC = gcc

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
	rm -rf *.o .out *.a radix_test graphs
