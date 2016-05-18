/*
 * radix_tree.h
 */

#ifndef _RADIX_TREE_H_
#define _RADIX_TREE_H_

struct radix_node {
	void *slots[0];
};

struct radix_tree {
        struct radix_node *node;
        int radix; /* each node will have 2**radix slots */
        int max_height; /* depends on the number of bits and the radix */
};

// Initiaizes @tree according to the number of @bits and specified @radix
void radix_tree_init(struct radix_tree *tree, int bits, int radix);

// Inserts @item associated with @index into the @tree 
int radix_tree_insert(struct radix_tree *tree,
		      unsigned long index, void *item);

// @TODO
void *radix_tree_find_alloc(struct radix_tree *tree, unsigned long index,
                            void *(*create)(unsigned long),
                            void *(*delete)(void *));

// @TODO
void *radix_tree_find(struct radix_tree *tree, unsigned long index);

/*
 * Note: You probably want to implement radix_tree_find as
 * radix_tree_find_alloc(tree, index, NULL, NULL).
 */

#endif