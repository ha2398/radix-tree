# Radix Tree Makefile

CC = gcc

INCLUDES = -I.

# Compilation options:
CFLAGS = -g -Wall -O2 $(INCLUDES)

# Linking options:
LDFLAGS = -g -L.

# Libraries
LDLIBS = -pthread -lrt

# Executable files
EXECS = master p_lockless p_lock_subtree p_lock_level p_lock_node

# File names
MST = master
PLLE = p_lockless
PLS = p_lock_subtree
PLL = p_lock_level
PLN = p_lock_node

all: $(EXECS)

# Master
$(MST): radix_test_seq.o libradixtree_$(MST).a
	$(CC) -o $(MST) $(LDFLAGS) radix_test_seq.o -lradixtree_$(MST) $(LDLIBS)

libradixtree_$(MST).a: radix_tree_$(MST).o
	ar rc libradixtree_$(MST).a radix_tree_$(MST).o
	ranlib libradixtree_$(MST).a

radix_tree_$(MST).o: radix_tree_$(MST).c radix_tree.h
	$(CC) -c $(CFLAGS) radix_tree_$(MST).c

radix_test_seq.o: radix_test_seq.c radix_tree.h
	$(CC) -c $(CFLAGS) radix_test_seq.c

# p_lockless
$(PLLE): radix_test_prl.o libradixtree_$(PLLE).a
	$(CC) -o $(PLLE) $(LDFLAGS) radix_test_prl.o -lradixtree_$(PLLE) $(LDLIBS)

libradixtree_$(PLLE).a: radix_tree_$(PLLE).o
	ar rc libradixtree_$(PLLE).a radix_tree_$(PLLE).o
	ranlib libradixtree_$(PLLE).a

radix_tree_$(PLLE).o: radix_tree_$(PLLE).c radix_tree.h
	$(CC) -c $(CFLAGS) radix_tree_$(PLLE).c

# p_lock_subtree
$(PLS): radix_test_prl.o libradixtree_$(PLS).a
	$(CC) -o $(PLS) $(LDFLAGS) radix_test_prl.o -lradixtree_$(PLS) $(LDLIBS)

libradixtree_$(PLS).a: radix_tree_$(PLS).o
	ar rc libradixtree_$(PLS).a radix_tree_$(PLS).o
	ranlib libradixtree_$(PLS).a

radix_tree_$(PLS).o: radix_tree_$(PLS).c radix_tree.h
	$(CC) -c $(CFLAGS) radix_tree_$(PLS).c

# p_lock_level
$(PLL): radix_test_prl.o libradixtree_$(PLL).a
	$(CC) -o $(PLL) $(LDFLAGS) radix_test_prl.o -lradixtree_$(PLL) $(LDLIBS)

libradixtree_$(PLL).a: radix_tree_$(PLL).o
	ar rc libradixtree_$(PLL).a radix_tree_$(PLL).o
	ranlib libradixtree_$(PLL).a

radix_tree_$(PLL).o: radix_tree_$(PLL).c radix_tree.h
	$(CC) -c $(CFLAGS) radix_tree_$(PLL).c

# p_lock_node
$(PLN): radix_test_prl_pln.o libradixtree_$(PLN).a
	$(CC) -o $(PLN) $(LDFLAGS) radix_test_prl_pln.o -lradixtree_$(PLN) $(LDLIBS)

libradixtree_$(PLN).a: radix_tree_$(PLN).o
	ar rc libradixtree_$(PLN).a radix_tree_$(PLN).o
	ranlib libradixtree_$(PLN).a

radix_tree_$(PLN).o: radix_tree_$(PLN).c radix_tree_pln.h
	$(CC) -c $(CFLAGS) radix_tree_$(PLN).c

radix_test_prl_pln.o: radix_test_prl_pln.c radix_tree_pln.h
	$(CC) -c $(CFLAGS) radix_test_prl_pln.c

radix_test_prl.o: radix_test_prl.c radix_tree.h
	$(CC) -c $(CFLAGS) radix_test_prl.c

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
