# RadixTree

Radix Tree implementation in C

1) The implementation follows this [article](https://lwn.net/Articles/175432/).

2) It supports lookups and lookups with insertion.

3) There is no support to deleting an item in the tree.

4) It does not support keys mapped to same nodes.

## Testing

### Arguments

The test program receives as arguments the following values:

master:	RANGE KEYS TESTS

other branches:	RANGE KEYS TESTS THREADS

RANGE -> Maximum number of tracked bits and radix to use in order to generate random trees in the tests.

KEYS -> Number of keys to be inserted in the trees for each test instance.

TESTS -> Number of test instances per program execution.

THREADS -> Maximum Number of threads to perform the tests. (For branches with support to parallel lookups)

### Structure

The test program asserts the Radix Tree implementation's correctness by keeping track of the pointers returned by the functions **radix_tree_find** and **radix_tree_find_alloc**. 
Initially, there is an array with all NULL entries, where each entry correspond to one possible key to be generated (the size of this array comes from RANGE, since the highest key is the highest number we can represent with RANGE bits.)
Then the main loop in the program will repeat the following steps:

	1)	Create a random *radix_tree*
	2) 	Create a random array of keys to be inserted into the tree.
	3)	Insert all keys into the tree using **radix_tree_find_alloc**.
	4)	Recreate the keys array with new random keys.
	5)	Lookup all these new keys using **radix_tree_find**.
	6)	Delete the tree.

This loop is repeated **TESTS** number of times.

### Types of errors

The test can detect two kinds of errors:

	1) Error in **radix_tree_find_alloc**

		i) Key is already in the tree, but the function returns an address which differs from the actual key's location.

		ii) Key is not present in the tree, but the function returns NULL.

	2) Error in **radix_tree_find**

		i) The function returns an address which differs from the key's location in the tree (which can be NULL, if the keys has not been inserted).

### Output

The test file has as main goal asserting the correctness of the implementation. However, in order to compare the branches and their performances, it produces as output the total time spent during the lookups. For that, a variable of type clock_t (time.h) is created and it marks the beginning and end of the lookup section of the code. The time spent in this section is added and the final value is printed in *stdout*.

### Test script

In order to make the testing process even easier and more automated, a shell script was created. The file **test_and_plot.sh** obtain the output for all branches and generates graphs to compare. For this, it uses Gnuplot utility.

#### Structure

The structure of the script is as follows:

	1) Step through GitHub branches and obtain the executable test files
	2) Step through the executable test files
	3) Run each of them with different parameter values.
	4) Obtain the running time for each execution
	5) Store this data in an output file
	6) Plot a graph for each output file

#### Types of Graphs

Currently, these are the graphs supported by the script:

	1) Number of Threads x Running Time (s)

	2) Number of Threads x Throughput (Lookups/Execution Time)

#### Graphs and Branches' Performance

The very first observation to make regarding the performance of the branches, is that they all suffer from a decrease in performance when compared to master, which is sequential and therefore makes no use of the **pthread** library. This happens due to the cost of creating threads (**pthread_create**) and waiting for them to finish their work (**pthread_join**).

Analyzing the running time, we see that two of the threads have a very similar (and bad) performace, **p_lock_level** and **p_lock_node**. The branch **p_lock_level** works with mutexes that lock the current level on which they are working in the tree. This is expected to be a low performance branch because, usually, the radix trees will not have a big height. The maximum height of the tree will then **limit** the number of threads than can be traversing the tree concurrently.

For **p_lock_node**, the problem is that it has to acquire and release a mutex for every single node it traverses. The cost for doing this is very big, since the tree may have up to sum{from 1 to maximum height} of (1 ^ number of slots per node).

Among the branches that provide synchronization through mutexes, the one with best performance is **p_lock_subtree**. This branch will acquire a lock for the subtree (of root node) about to be traversed. This protocol acquires way fewer mutexes than **p_lock_node**.
