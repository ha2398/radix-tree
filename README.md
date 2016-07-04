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

The main problem with **lock_node** is that it acquires and releases a very large quantity of mutexes (every time a node is being accessed!), which causes significant slowdown in the lookups.

To fix that, we can observe the path that a thread traverses by performing a lookup. It starts on the root node and goes, node by node, until a leaf. This path that all threads traverse during a lookup is a subtree of the radix tree, and all the operations a thread might perform are restrict to this subtree.

Therefore, it suffices to keep thread safety to lock the subtree about to be traversed by a thread at the beginning of all lookups and unlock it when the lookup finishes.

This approach reduces the number of mutexes in comparison to **lock_node**

### lockless

Even though the performance of **lock_subtree** is a great improvement in comparison to the other implementations, we can still observe that the cost of locking and unlocking a mutex cause great slowdown for the implementation.

In order to get rid of this cost, it is necessary to think of ways that replace mutexes but keep the thread safety that mutexes assert.

One of these ways is based on relying on **atomic operations**. Basically, if we use the GCC built-in function for compare and swap, we can allow two threads to traverse the same subtree (and node) of a three in parallel. This kind of parallel traversing only represents a threat to thread safety when one of the threads has to create a new child node for one of the nodes in the tree.

When that happens, it is necessary to make sure only one of the threads is successful in doing so, and the other ones simply return the resulting node of such an operation. 

## Testing

### Arguments

The test program receives as arguments the following values:

./radix_test -b BITS -r RADIX -k KEYS -l LOOKUPS -p THREADS -i IMPLEMENTATION

BITS -> Number of tracked bits for the radix tree.

RADIX -> Radix value for the tree.

KEYS -> Number of keys to be inserted in the tree. The test program will insert all keys on the interval [0, KEYS], respecting the range of the lookups, which is calculated as (2 ^ (BITS) - 1) (maximum key tracked by the tree without key duplication). 

LOOKUPS -> Number of lookups to perform on the tree. The keys to search on the tree are randomly generated.

THREADS -> Number of threads performing lookups concurrently.

IMPLEMENTATION -> Selects which implementation to test, among the ones described above.

### Structure

The test program asserts the Radix Tree implementation's correctness by keeping track of the pointers returned by the functions **radix_tree_find** and **radix_tree_find_alloc**. 
Initially, there is an array with all NULL entries, where each entry correspond to one possible key to be looked up on the tree (the size of this array comes from the number of tracked bits, since the highest key is the highest number we can represent with BITS bits.)
Then the main loop in the program will repeat the following steps:

	1)	Create a radix tree using the values of BITS and RADIX.
	2) 	Insert all keys in the interval [0, KEYS] (or [0, LOOKUPS_RANGE], where LOOKUPS_RANGE = (2 ^ (BITS) - 1), if it is smaller) in the tree
	3)	Generates a random array of size LOOKUPS for each thread.
	4)	Each thread perform lookups on each of these elements.

### Types of errors

The test can detect two kinds of errors:

	1) Error in radix_tree_find_alloc
		i) Key is already in the tree, but the function returns an address which differs from the actual key's location.
		ii) Key is not present in the tree, but the function returns NULL.

	2) Error in radix_tree_find
		i) The function returns an address which differs from the key's location in the tree (which can be NULL, if the keys has not been inserted).

### Output

The test file has as main goal asserting the correctness of the implementation. However, in order to compare the implementations (different parallel approaches and the sequential one) and their performances, it produces as output the total time spent during the lookups. This only happens, of course, if the program does not terminate before due to some error. To calculate the time spent of the lookups, the function **clock_gettime** is used. This value of the time spent is printed in **stdout**.

### Test script

In order to make the testing process even easier and more automated, a script written in **Python** was created. The file **test_and_plot.py** obtain the output for all implementations and generates graphs to compare. For this, it uses the Gnuplot utility.

#### Arguments

The test script receives as arguments the following values:

./radix_test GRAPH=g BITS=b RADIX=r KEYS=k LOOKUPS=l TESTS=t THREADS=p

g -> Type of graph to be generated.
p -> Maximum number of threads to use to perform the tests.

#### Types of Graphs

Currently, these are the graphs supported by the script:

	1) Number of Threads x Running Time
	2) Number of Threads x Throughput (Lookups/Execution Time)

#### Structure

The structure of the script is as follows:

    1) Compile the source code and obtain the executable test file.
    2) Run it for each of the implementations with different parameter values.
    3) Obtain the running time for each execution.
    4) Store this data in an output file.
    5) Plot a graph for each output file.

#### Graphs and Implementations' Performance

The very first observation to make regarding the performance of the implementations, is that performing the same amount of lookups for a test program which makes no use of the pthread library would be faster than using **sequential**. This happens due to the cost of creating threads (**pthread_create**), managing mutexes (**pthread_mutex_lock and pthread_mutex_unlock**) and waiting for them to finish their work (**pthread_join**).

For **sequential**, we have:

**perf record ./radix_test -b20 -r4 -k5000000 -l10000000 -t1 -p32 -isequential**

**perf report --stdio**

 [Overhead]     [Command]         [Shared Object]                                       [Symbol]   
    93.41%  radix_test  [kernel.kallsyms]     [k] _raw_spin_lock                         
     1.54%  radix_test  radix_test            [.] radix_tree_find                        
     1.27%  radix_test  libpthread-2.19.so    [.] pthread_mutex_lock                     
     0.78%  radix_test  [kernel.kallsyms]     [k] system_call_after_swapgs               
     0.67%  radix_test  libpthread-2.19.so    [.] __lll_lock_wait                        
     0.51%  radix_test  radix_test            [.] thread_find                            
     0.41%  radix_test  libpthread-2.19.so    [.] pthread_mutex_unlock                   
     0.33%  radix_test  [kernel.kallsyms]     [k] get_futex_key_refs.isra.13             
     0.22%  radix_test  [kernel.kallsyms]     [k] _raw_spin_unlock_irqrestore            
     0.14%  radix_test  radix_test            [.] radix_tree_find_alloc     

We can see that there is a high thread containment due to the increased number of locking operations. No parallelism is explored here because the threads are not allowed to work at the same time.                     

Analyzing the running time and throughput, we see that the two implementations with worst performance (after sequential), are **lock_level** and **lock_node**. The implementation **lock_level** works with mutexes that lock the current level on which they are working in the tree. This is expected to be a low performance implementation because, usually, the radix trees will not have a large height (tracked bits divided by radix). The maximum height of the tree will then **limit** the number of threads than can traverse the tree concurrently.

Running the test program for **lock_level** using perf, we can see that the main overhead for this implementation is the lock of mutexes.

**perf record ./radix_test -b20 -r4 -k5000000 -l10000000 -t1 -p32 -ilock_level**

**perf report --stdio**

 [Overhead]   [Command]     [Shared Object]       [Symbol]    
    65.03%  radix_test  [kernel.kallsyms]     [k] _raw_spin_lock                         
     6.81%  radix_test  libpthread-2.19.so    [.] pthread_mutex_lock                     
     6.43%  radix_test  [kernel.kallsyms]     [k] _raw_spin_unlock_irqrestore            
     3.89%  radix_test  libpthread-2.19.so    [.] pthread_mutex_unlock                   
     3.66%  radix_test  libpthread-2.19.so    [.] __lll_lock_wait                        
     2.72%  radix_test  [kernel.kallsyms]     [k] finish_task_switch                     
     2.05%  radix_test  [kernel.kallsyms]     [k] system_call_after_swapgs               
     1.23%  radix_test  radix_test            [.] thread_find                            
     1.05%  radix_test  [kernel.kallsyms]     [k] get_futex_key_refs.isra.13    

For **lock_node**, the problem is that it has to acquire and release a mutex for every single node it traverses. The cost for doing this is very expensive, since the tree may have up to **sum{from 1 to maximum height} of (1 ^ number of slots per node)** nodes, which can be a very large number.

**perf record ./radix_test -b20 -r4 -k5000000 -l10000000 -t1 -p32 -ilock_node**

**perf report --stdio**

 [Overhead]     [Command]         [Shared Object]                                       [Symbol]        
    92.65%  radix_test  [kernel.kallsyms]     [k] _raw_spin_lock                         
     3.55%  radix_test  libpthread-2.19.so    [.] pthread_mutex_lock                     
     0.65%  radix_test  libpthread-2.19.so    [.] __lll_lock_wait                        
     0.61%  radix_test  [kernel.kallsyms]     [k] system_call_after_swapgs               
     0.54%  radix_test  libpthread-2.19.so    [.] pthread_mutex_unlock                   
     0.42%  radix_test  radix_test            [.] radix_tree_find                        
     0.38%  radix_test  radix_test            [.] thread_find                            
     0.27%  radix_test  [kernel.kallsyms]     [k] get_futex_key_refs.isra.13             
     0.18%  radix_test  [kernel.kallsyms]     [k] _raw_spin_unlock_irqrestore            
     0.10%  radix_test  [kernel.kallsyms]     [k] _raw_spin_unlock          

Among the implementations that provide synchronization through mutexes, the one with best performance is **lock_subtree**. This implementation acquires a lock for the subtree (of root node) about to be traversed. This protocol acquires way fewer mutexes than **lock_node**. Here the amount of threads allowed to work concurrently is at most the number of slots in one node of the tree.

**perf record ./radix_test -b20 -r4 -k5000000 -l10000000 -t1 -p32 -ilock_subtree**

**perf report --stdio**

 [Overhead]     [Command]       [Shared Object]                                       [Symbol]   
    36.31%  radix_test  libpthread-2.19.so  [.] pthread_mutex_lock                     
    20.85%  radix_test  libpthread-2.19.so  [.] pthread_mutex_unlock                   
     7.66%  radix_test  [kernel.kallsyms]   [k] _raw_spin_lock                         
     6.23%  radix_test  radix_test          [.] thread_find                            
     4.25%  radix_test  radix_test          [.] radix_tree_find_alloc                  
     4.08%  radix_test  libpthread-2.19.so  [.] __lll_lock_wait                        
     3.42%  radix_test  radix_test          [.] radix_tree_find                        
     3.06%  radix_test  [kernel.kallsyms]   [k] _raw_spin_unlock_irqrestore            
     2.92%  radix_test  [kernel.kallsyms]   [k] system_call_after_swapgs               
     1.74%  radix_test  [kernel.kallsyms]   [k] finish_task_switch                     
     1.57%  radix_test  [kernel.kallsyms]   [k] get_futex_key_refs.isra.13             
     1.43%  radix_test  libpthread-2.19.so  [.] __lll_unlock_wake                      
     1.27%  radix_test  libc-2.19.so        [.] __random                               
     0.87%  radix_test  radix_test          [.] main                                   
     0.53%  radix_test  [kernel.kallsyms]   [k] futex_wake                           

Finally, the parallel approach **lockless** gets rid of the use of mutexes (and all the cost that comes with it for locking and unlocking mutexes) by exploring atomic operations, namely the macro [ACCESS_ONCE](https://lwn.net/Articles/508991/) and the GCC built-in function [__sync_bool_compare_and_swap](https://gcc.gnu.org/onlinedocs/gcc-4.4.3/gcc/Atomic-Builtins.html). These operations will allow the code to keep synchronization between threads and do not rely on the use of mutexes.

**perf record ./radix_test -b20 -r4 -k5000000 -l10000000 -t1 -p32 -ilockless**

**perf report --stdio**

 [Overhead]     [Command]      [Shared Object]                                       [Symbol]   
    76.49%  radix_test  radix_test         [.] thread_find                            
     6.10%  radix_test  libc-2.19.so       [.] __random                               
     6.02%  radix_test  radix_test         [.] radix_tree_find_alloc                  
     4.16%  radix_test  radix_test         [.] main                                   
     1.78%  radix_test  libc-2.19.so       [.] __random_r                             
     1.23%  radix_test  radix_test         [.] radix_tree_find                        
     1.07%  radix_test  [kernel.kallsyms]  [k] clear_page_c                           
     1.00%  radix_test  [kernel.kallsyms]  [k] copy_page_rep                          
     0.69%  radix_test  [kernel.kallsyms]  [k] _raw_spin_lock                        

We can see that there is a gain in performance caused by the absence of operations of locking and unlocking mutexes and the implementation spends more time doing the actual work we want to benchmark. Also, we can see that the lookups themselves are very quick compared to the other operations in the loop function (thread_find) each thread executes.

Additionally, we can see in the graphs below the relation that compares all the implementations and can give some insight on how each one stands in comparison to the others.

**Number of Threads x Throughput**

![Graph 1](https://s31.postimg.org/6rob13vy3/graph1.png)
![Graph 2](https://s31.postimg.org/s4mg64nmz/graph2.png)

**Number of Threads x Running Time**

![Graph 3](https://s32.postimg.org/wr8ajitgl/graph3.png)