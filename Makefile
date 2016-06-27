# Radix Tree Makefile

CC = gcc

INCLUDES = -I.

# Compilation options:
CFLAGS = -g -Wall -O2 $(INCLUDES)

# Linking options:
LDFLAGS = -g -L.

# Libraries
LDLIBS = -pthread -lrt

all: radix_tree_test

radix_tree_test: libradixtree.a radix_test.o
	$(CC) -o $(LDFLAGS) radix_test.o -lradixtree $(LDLIBS)

radix_tree_test.o: radix_test.c radix_tree.h
	$(CC) -c $(CFLAGS) radix_test.c

libradixtree.a: lock_level.o lock_node.o lock_subtree.o lockless.o \
	sequential.o
	ar rcs libradixtree.a lock_level.o lock_node.o lock_subtree.o \
		lockless.o sequential.o
	ranlib libradixtree.a

lock_level.o: lock_level.c radix_tree.h
	$(CC) -c $(CFLAGS) lock_level.c

lock_node.o: lock_node.c radix_tree.h
	$(CC) -c $(CFLAGS) lock_node.c

lock_subtree.o: lock_subtree.c \
	radix_tree.h
	$(CC) -c $(CFLAGS) lock_subtree.c

lockless.o: lockless.c radix_tree.h
	$(CC) -c $(CFLAGS) lockless.c

sequential.o: sequential.c radix_tree.h
	$(CC) -c $(CFLAGS) sequential.c

.PHONY: clean
clean:
	rm -rf *.o *.a $(EXECS) graph.* test_files

.PHONY: clear
clear:
	rm -rf *.o *.a graph.*

# Test parameters (default)

# Type of graph to be generated through tests.
GRAPH = 1

# Maximum number of tracked bits and radix to use in order to generate
# random trees in the tests. Also defines the numer of lookups, which is
# 2 ^ RANGE
RANGE = 15

# Number of keys to be inserted in the trees for each test instance.
KEYS = 15000

# Number of lookups to perform on the tree
LOOKUPS = 30000

# Number of test instances per program execution.
TESTS = 1

# Maximum Number of threads to perform the tests.
THREADS = 4

.PHONY: test_and_plot
test_and_plot:
	./test_and_plot.py $(GRAPH) $(RANGE) $(KEYS) $(LOOKUPS) $(TESTS) $(THREADS)
