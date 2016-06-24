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

/* Initiaizes @tree according to the number of @bits and specified @radix */
void radix_tree_init(struct radix_tree *tree, int bits, int radix);

/*
 * If there is an item associated with @key in the tree, returns it.
 * Otherwise, calls "create" function to allocate it if "create" is not NULL.
 */
void *radix_tree_find_alloc(struct radix_tree *tree, unsigned long index,
				void *(*create)(unsigned long));

/*
 * Works just like radix_tree_find_alloc but does not allocate memory for
 * new item if it is not found in the tree.
 */
void *radix_tree_find(struct radix_tree *tree, unsigned long key);

/*
 * Deletes a tree and its contents by deallocating its memory
 */
void radix_tree_delete(struct radix_tree *tree);

#endif
