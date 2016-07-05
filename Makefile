# Radix Tree Makefile

CC = gcc

INCLUDES = -I.

# Compilation options:
CFLAGS = -g -O2 $(INCLUDES)

# Linking options:
LDFLAGS = -g -L.

# Libraries
LDLIBS = -pthread -lrt -lradixtree

# Executable
EXEC = radix_test

# Dependencies
DEPS = radix_tree.h

all: $(EXEC)

$(EXEC): libradixtree.a $(EXEC).o
	$(CC) $(LDFLAGS) $(EXEC).o $(LDLIBS) -o $(EXEC)

libradixtree.a: lock_level.o lock_node.o lock_subtree.o lockless.o \
	sequential.o
	ar rcs libradixtree.a lock_level.o lock_node.o lock_subtree.o \
		lockless.o sequential.o
	ranlib libradixtree.a

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -rf *.o *.a $(EXEC) graph.* test_files

.PHONY: clear
clear:
	rm -rf *.o *.a graph.*

# Test parameters (default)

# Type of graph to be generated through tests.
GRAPH = 1

# Number of tracked bits in the tree
BITS = 16

# Tree radix
RADIX = 4

# Number of keys to be inserted in the trees for each test instance.
KEYS = 30000

# Number of lookups to perform on the tree
LOOKUPS = 60000

# Number of test instances per program execution.
TESTS = 2

# Maximum Number of threads to perform the tests.
THREADS = 4

.PHONY: test_and_plot
test_and_plot:
	./test_and_plot.py $(GRAPH) $(BITS) $(RADIX) $(KEYS) $(LOOKUPS) \
		$(TESTS) $(THREADS)
