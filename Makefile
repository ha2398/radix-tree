# Radix Tree Makefile

CC = gcc

INCLUDES = -I.

# Compilation options:
CFLAGS = -g -Wall $(INCLUDES)

# Linking options:
LDFLAGS = -g -L.

# Libraries
LDLIBS = -lradixtree -lm

all: libradixtree.a radix_test

libradixtree.a: radix_tree.o
	ar rc libradixtree.a radix_tree.o
	ranlib libradixtree.a

radix_tree.o: radix_tree.c radix_tree.h

radix_test.o: radix_test.c radix_tree.h

radix_test: radix_test.o

.PHONY: clean
clean:
	rm -f *.o a.out radix_test