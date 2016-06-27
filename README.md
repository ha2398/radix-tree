# RadixTree

Radix Tree implementations in C

1) The implementations follow this [article](https://lwn.net/Articles/175432/).

2) It supports lookups and lookups with insertion.

3) There is no support to deleting an item in the tree.

4) It does not support keys mapped to same nodes.

## Implementations

Currently, there are 5 different radix tree implementations in the repository. Each of them has a different approach for lookups and how these lookups are allowed to work concurrently.

### sequential

This implementation only allows the lookups to occur sequentially, that is, once a lookup operation starts, a mutex is locked, forcing the other ones to wait. When this same lookup operation ends, the mutex is released and the next lookup operation is allowed to start.

### lock_level

In order to push the idea of parallel lookups on the tree forward and develop some sort of parallelism, the approach used in **sequential** needs to be replaced. For that, we need to try to identify what is the critical part of the tree being accessed that needs some sort of protection so that no threads modify it concurrently causing errors. The levels of the tree are one of these critical parts: two threads accessing different levels of the tree offer no threat to the safety of the lookups.

Thinking of that, we can lock a mutex for each level being accessed, and unlock it as soon as the thread currently accessing that level moves forward to traverse the next level.

### lock_node

Extending the idea of **lock_level**, we can observe that, at a given moment, a level must be locked (using a mutex) but a single thread accesses more than one node of that level. Hence, there is no need to lock an entire level of the tree, while unlocking a single node suffices.

To allow more threads to perform lookups on the same tree concurrently, we can lock a mutex for each node being accessed, and unlock it as soon as the thread currently accessing that node moves forward to traverse its child node.

This approach maintains thread safety because no two threads will access the same node (and modify it, by adding children to it) at the same time.

### lock_subtree

The main problem with **lock_node** is that it acquires and releases a very large quantity of mutexes (every time a node is being accessed!), which causes signifcant slowdown in the lookups.

To fix that, we can observe the path that a thread traverses by performing a lookup. It starts on the root node and goes, node by node, until a leaf. This path that all threads traverse during a lookup is a subtree of the radix tree, and all the operations a thread might perform are restrict to this subtree.

Therefore, it suffices to keep thread safety to lock the subtree about to be traversed by a thread at the beggining of all lookups and unlock it when the lookup finishes.

This approach reduces the number of mutexes in comparison to **lock_node**

### lockless

Even though the performance of **lock_subtree** is a great improvement in comparison to the other implementations, we can still observe that the cost of locking and unlocking a mutex cause great slowdown for the implementation.

In order to get rid of this cost, it is necessary to think of ways that replace mutexes but keep the thread safety that mutexes assert.

One of these ways is based on relying on **atomic operations**. Basically, if we use the GCC built-in function for compare and swap, we can allow two threads to traverse the same subtree (and node) of a three in parallel. This kind of parallel traversing only represents a threat to thread safety when one of the threads has to create a new child node for one of the nodes in the tree.

When that happens, it is necessary to make sure only one of the threads is successful in doing so, and the other ones simply return the resulting node of such an operation. 

## Testing

### Arguments

The test program receives as arguments the following values:

./test -r RANGE -k KEYS -l LOOKUPS - t TESTS -t THREADS -i IMPLEMENTATION

RANGE -> Maximum number of tracked bits and radix to use in order to generate random trees in the tests.

KEYS -> Number of keys to be inserted in the trees for each test instance. The test program will insert in the trees all keys on the interval [0, KEYS]

LOOKUPS -> Number of lookups to perform on the tree. These numbers are randomly generated.

TESTS -> Number of test instances per program execution.

THREADS -> Maximum Number of threads to perform the tests.

IMPLEMENTATION -> Selects which implementation to test, among the ones described above.

### Structure

The test program asserts the Radix Tree implementation's correctness by keeping track of the pointers returned by the functions **radix_tree_find** and **radix_tree_find_alloc**. 
Initially, there is an array with all NULL entries, where each entry correspond to one possible key to be generated (the size of this array comes from RANGE, since the highest key is the highest number we can represent with RANGE bits.)
Then the main loop in the program will repeat the following steps:

	1)	Create a random radix_tree
	2) 	Insert all keys in the interval [0, KEYS] in the tree.
	3)	Generates a random array of size LOOKUPS for each thread.
	4)	Each thread perform lookups on each of these elements.
	5)	Delete the tree.

This loop is repeated **TESTS** number of times.

### Types of errors

The test can detect two kinds of errors:

	1) Error in **radix_tree_find_alloc**

		i) Key is already in the tree, but the function returns an address which differs from the actual key's location.

		ii) Key is not present in the tree, but the function returns NULL.

	2) Error in **radix_tree_find**

		i) The function returns an address which differs from the key's location in the tree (which can be NULL, if the keys has not been inserted).

### Output

The test file has as main goal asserting the correctness of the implementation. However, in order to compare the implementations (different parallel approaches and the sequential one) and their performances, it produces as output the total time spent during the lookups. For that, the function **clock_gettime** is used. The time spent in the lookup section is added and the final value is printed in *stdout*.

### Test script

In order to make the testing process even easier and more automated, a script written in **Python** was created. The file **test_and_plot.py** obtain the output for all implementations and generates graphs to compare. For this, it uses the Gnuplot utility.

#### Structure

The structure of the script is as follows:

	1) Compile the source code and obtain the executable test files
	2) Step through the executable test files
	3) Run each of them with different parameter values.
	4) Obtain the running time for each execution
	5) Store this data in an output file
	6) Plot a graph for each output file

#### Types of Graphs

Currently, these are the graphs supported by the script:

	1) Number of Threads x Running Time

	2) Number of Threads x Throughput (Lookups/Execution Time)

#### Graphs and Implementations' Performance

The very first observation to make regarding the performance of the implementations, is that performing the same amount of lookups for a test program which makes no use of the pthread library would be faster than using **sequential**. This happens due to the cost of creating threads (**pthread_create**), managing mutexes (**pthread_mutex_lock and pthread_mutex_unlock**) and waiting for them to finish their work (**pthread_join**).

Analyzing the running time, we see that two of the threads have a very similar performance, **lock_level** and **lock_node**. The implementation **lock_level** works with mutexes that lock the current level on which they are working in the tree. This is expected to be a low performance implementation because, usually, the radix trees will not have a big height (tracked bits divided by radix). The maximum height of the tree will then **limit** the number of threads than can be traversing the tree concurrently.

Running the test program for **lock_level** using perf, with **RANGE = 31, KEYS = 5000000, LOOKUPS = 10000000, TESTS = 1 and THREADS = 32** (all the perf commands below are run with these parameters) we can see that the main overhead for this implementation is the lock of mutexes.      

For **lock_node**, the problem is that it has to acquire and release a mutex for every single node it traverses. The cost for doing this is very expensive, since the tree may have up to sum{from 1 to maximum height} of (1 ^ number of slots per node), which can be a very large number.

Among the implementations that provide synchronization through mutexes, the one with best performance is **lock_subtree**. This implementation acquires a lock for the subtree (of root node) about to be traversed. This protocol acquires way fewer mutexes than **lock_node**.

Finally, the parallel approach **lockless** gets rid of the use of mutexes (and all the cost that comes with it for locking and unlocking mutexes) by exploring atomic operations, namely the macro [ACCESS_ONCE](https://lwn.net/Articles/508991/) and the GCC built-in function [__sync_bool_compare_and_swap](https://gcc.gnu.org/onlinedocs/gcc-4.4.3/gcc/Atomic-Builtins.html). These operations will allow the code to keep synchronization between threads and do not rely on the use of mutexes. 

We can see that there is a gain in performance caused by the absence of operations of locking and unlocking mutexes and the implementation spends more time doing the actual work we want to benchmark (thread_find).

Additionally, we can see in the graph below the relation that compares the throughput (number of lookups/total time spent on lookups) for all the implementations.

**Number of Threads x Throughput**

**Number of Threads x Running Time**
