# Radix Tree Makefile

CC = gcc

# Compilation options:
CFLAGS = -g -Wall

# Linking options:
LDFLAGS = -g

# Libraries
LDLIBS =

radix_test: radix_test.o radix_tree.o

radix_test.o: radix_test.c radix_tree.h

radix_tree.o: radix_tree.c radix_tree.h

.PHONY: clean
clean:
 rm -f *.o a.out core main
 
.PHONY: all
all: clean radix_test