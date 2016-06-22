# RadixTree

Radix Tree implementation in C

1) The implementation follows this [article](https://lwn.net/Articles/175432/).

2) It supports lookups and lookups with insertion.

3) There is no support to deleting an item in the tree.

4) It does not support keys mapped to same nodes.

## Testing

### Arguments

The test program receives as arguments the following values:

master:	RANGE KEYS LOOKUPS TESTS

other implementations:	RANGE KEYS LOOKUPS TESTS THREADS

RANGE -> Maximum number of tracked bits and radix to use in order to generate random trees in the tests.

KEYS -> Number of keys to be inserted in the trees for each test instance.

LOOKUPS -> Number of lookups to perform on the tree. The test program will perform all possible lookups on the interval [0, LOOKUPS]

TESTS -> Number of test instances per program execution.

THREADS -> Maximum Number of threads to perform the tests. (For all implementations but master)

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

The very first observation to make regarding the performance of the implementations, is that they all, after a certain number of threads being used, suffer from a decrease in performance when compared to master, which is sequential and therefore makes no use of the **pthread** library. This happens due to the cost of creating threads (**pthread_create**) and waiting for them to finish their work (**pthread_join**). Also, depending on how many keys we want to lookup in the tree, the cost of managing (creating, joining) the threads can be higher than the time to actually perform the lookups.

Analyzing the running time, we see that two of the threads have a very similar performace, **p_lock_level** and **p_lock_node**. The branch **p_lock_level** works with mutexes that lock the current level on which they are working in the tree. This is expected to be a low performance branch because, usually, the radix trees will not have a big height (tracked bits divided by radix). The maximum height of the tree will then **limit** the number of threads than can be traversing the tree concurrently.

Running the test program for **p_lock_level** using perf, with **RANGE = 31, KEYS = 5000000, LOOKUPS = 10000000, TESTS = 1 and THREADS = 32** (all the perf commands below are run with these paremeters) we can see that the main overhead for this branch is the lock of mutexes.

  Overhead       Command       Shared Object                                       Symbol

  ........  ............  ..................  ...........................................
 
    17.35%  p_lock_level  [kernel.kallsyms]   [k] _raw_spin_lock                

    16.75%  p_lock_level  libpthread-2.19.so  [.] pthread_mutex_lock                  

    10.29%  p_lock_level  libpthread-2.19.so  [.] pthread_mutex_unlock                  

     8.92%  p_lock_level  p_lock_level        [.] thread_find        

     5.47%  p_lock_level  libpthread-2.19.so  [.] __lll_lock_wait             

     3.95%  p_lock_level  [kernel.kallsyms]   [k] futex_wait_setup            

     3.20%  p_lock_level  [kernel.kallsyms]   [k] futex_wake            

     2.95%  p_lock_level  p_lock_level        [.] radix_tree_find_alloc

     2.23%  p_lock_level  [kernel.kallsyms]   [k] get_futex_value_locked

     1.95%  p_lock_level  [kernel.kallsyms]   [k] get_futex_key_refs.isra.13

     1.95%  p_lock_level  [kernel.kallsyms]   [k] system_call

     1.90%  p_lock_level  [kernel.kallsyms]   [k] hash_futex

     1.80%  p_lock_level  [kernel.kallsyms]   [k] do_futex

     1.62%  p_lock_level  [kernel.kallsyms]   [k] get_futex_key

     1.53%  p_lock_level  p_lock_level        [.] main 

     1.36%  p_lock_level  [kernel.kallsyms]   [k] system_call_after_swapgs 

     1.35%  p_lock_level  libpthread-2.19.so  [.] __lll_unlock_wake   

     1.08%  p_lock_level  [kernel.kallsyms]   [k] futex_wait                        

For **p_lock_node**, the problem is that it has to acquire and release a mutex for every single node it traverses. The cost for doing this is very expensive, since the tree may have up to sum{from 1 to maximum height} of (1 ^ number of slots per node), which can be a very large number.

  Overhead      Command       Shared Object                                      Symbol

  ........  ...........  ..................  ..........................................

    21.87%  p_lock_node  [kernel.kallsyms]   [k] _raw_spin_lock              

    13.59%  p_lock_node  p_lock_node         [.] thread_find 

    13.48%  p_lock_node  libpthread-2.19.so  [.] pthread_mutex_lock 

     6.64%  p_lock_node  libpthread-2.19.so  [.] pthread_mutex_unlock

     4.44%  p_lock_node  libpthread-2.19.so  [.] __lll_lock_wait  

     4.16%  p_lock_node  [kernel.kallsyms]   [k] futex_wait_setup 

     3.15%  p_lock_node  p_lock_node         [.] radix_tree_find_alloc 

     3.04%  p_lock_node  [kernel.kallsyms]   [k] futex_wake 

     2.05%  p_lock_node  [kernel.kallsyms]   [k] get_futex_value_locked    

     2.03%  p_lock_node  p_lock_node         [.] main       

     1.79%  p_lock_node  [kernel.kallsyms]   [k] get_futex_key_refs.isra.13    

     1.75%  p_lock_node  [kernel.kallsyms]   [k] system_call    

     1.72%  p_lock_node  [kernel.kallsyms]   [k] hash_futex   

     1.48%  p_lock_node  [kernel.kallsyms]   [k] get_futex_key  

     1.27%  p_lock_node  [kernel.kallsyms]   [k] system_call_after_swapgs 

     1.25%  p_lock_node  [kernel.kallsyms]   [k] do_futex   

     1.25%  p_lock_node  p_lock_node         [.] find_slot_index 

     1.16%  p_lock_node  libpthread-2.19.so  [.] __lll_unlock_wake 

     1.07%  p_lock_node  [kernel.kallsyms]   [k] futex_wait                            

Among the implementations that provide synchronization through mutexes, the one with best performance is **p_lock_subtree**. This implementation acquires a lock for the subtree (of root node) about to be traversed. This protocol acquires way fewer mutexes than **p_lock_node**.

  Overhead         Command       Shared Object                                      Symbol

  ........  ..............  ..................  ..........................................
 
    37.47%  p_lock_subtree  p_lock_subtree      [.] thread_find 

    21.47%  p_lock_subtree  libpthread-2.19.so  [.] pthread_mutex_lock   

    13.07%  p_lock_subtree  p_lock_subtree      [.] radix_tree_find_alloc  

     8.15%  p_lock_subtree  libpthread-2.19.so  [.] pthread_mutex_unlock    

     6.33%  p_lock_subtree  p_lock_subtree      [.] main   

     4.85%  p_lock_subtree  p_lock_subtree      [.] find_slot_index  

     1.42%  p_lock_subtree  libc-2.19.so        [.] __random_r  

     1.09%  p_lock_subtree  libc-2.19.so        [.] __random  

                   