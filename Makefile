# Radix Tree Makefile

CC = gcc

INCLUDES = -I.

# Compilation options:
CFLAGS = -g -Wall $(INCLUDES)

# Linking options:
LDFLAGS = -g -L.

# Libraries
LDLIBS = -lradixtree -pthread

all: radix_test

radix_test: radix_test.o libradixtree.a

radix_test.o: radix_test.c radix_tree.h

libradixtree.a: radix_tree.o
	ar rc libradixtree.a radix_tree.o
	ranlib libradixtree.a

radix_tree.o: radix_tree.c radix_tree.h

.PHONY: clean
clean:
	rm -rf *.o .out *.a radix_test graph.* test_files

# Test parameters (default)

# Type of graph to be generated through tests.
GRAPH = 1

# Maximum number of tracked bits and radix to use in order to generate
# random trees in the tests.
RANGE = 8

# Number of keys to be inserted in the trees for each test instance.
KEYS = 10000

# Number of test instances per program execution.
TESTS = 1

# Maximum Number of threads to perform the tests.
THREADS = 4

.PHONY: test_and_plot
test_and_plot:
	./test_and_plot.sh $(GRAPH) $(RANGE) $(KEYS) $(TESTS) $(THREADS)
