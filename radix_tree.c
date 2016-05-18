/*
 * radix_tree.c
 */

#include "radix_tree.h"
#include <math.h>
#include <stdlib.h>

void radix_tree_init(struct radix_tree *tree, int bits, int radix)
{
	tree = (struct radix_tree*)malloc(sizeof(struct radix_tree));

	tree->radix = radix;
	tree->max_height = bits / radix;

	int slots_len = pow(2, radix);

	tree->node = (struct radix_node*)malloc(sizeof(void *) * slots_len);

	int i = 0;
	for(; i < slots_len; i++)
		tree->node->slots[i] = NULL;
}

void *radix_tree_find_alloc(struct radix_tree *tree, unsigned long index,
			    void *(*create)(unsigned long),
			    void *(*delete)(void *))
{
	return NULL;
}

void *radix_tree_find(struct radix_tree *tree, unsigned long index)
{
	return NULL;
}