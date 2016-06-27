/*
 * radix_tree.h
 */

#include <pthread.h>

#ifndef _RADIX_TREE_H_
#define _RADIX_TREE_H_

struct radix_node {
	pthread_mutex_t lock;
	void *slots[0];
};

struct radix_tree {
	struct radix_node *node;
	int radix; /* each node will have 2**radix slots */
	int max_height; /* depends on the number of bits and the radix */
};

/*
 * Implementations Descriptors
 */
struct radix_tree_desc {
	char *name;
        void (*init)(struct radix_tree *, int, int);
        void *(*find_alloc)(struct radix_tree *, unsigned long,
        	void *(*)(unsigned long));
        void *(*find)(struct radix_tree *, unsigned long);
        void (*tree_delete)(struct radix_tree *);
};

struct radix_tree_desc sequential_desc;
struct radix_tree_desc lockless_desc;
struct radix_tree_desc lock_subtree_desc;
struct radix_tree_desc lock_level_desc;
struct radix_tree_desc lock_node_desc;

/* Initiaizes @tree according to the number of @bits and specified @radix */
static void radix_tree_init(struct radix_tree *tree, int bits, int radix);

/*
 * If there is an item associated with @key in the tree, returns it.
 * Otherwise, calls "create" function to allocate it if "create" is not NULL.
 */

static void *radix_tree_find_alloc(struct radix_tree *tree, unsigned long index,
				void *(*create)(unsigned long));

/*
 * Works just like radix_tree_find_alloc but does not allocate memory for
 * new item if it is not found in the tree.
 */

static void *radix_tree_find(struct radix_tree *tree, unsigned long key);

/*
 * Deletes a tree and its contents by deallocating its memory
 */

static void radix_tree_delete(struct radix_tree *tree);

#endif
