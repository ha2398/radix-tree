#ifndef RADIX_TREE_H
#define RADIX_TREE_H

struct radix_node {
	void *slots[0];
};

struct radix_tree {
        struct radix_node *node;
        int radix; /* each node will have 2**radix slots */
        int max_height; /* depends on the number of bits and the radix */
};

/* @bits: the number of bits to track; say, 32 bits */
void radix_tree_init(struct radix_tree *tree, int bits, int radix);

void *radix_tree_find_alloc(struct radix_tree *tree, unsigned long index,
                            void *(*create)(unsigned long),
                            void *(*delete)(void *));

void *radix_tree_find(struct radix_tree *tree, unsigned long index);

/*
 * Note: You probably want to implement radix_tree_find as
 * radix_tree_find_alloc(tree, index, NULL, NULL).
 */

#endif