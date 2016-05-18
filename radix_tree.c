/*
 * radix_tree.c
 */

#include "radix_tree.h"
#include <stdlib.h>

void radix_tree_init(struct radix_tree *tree, int bits, int radix)
{

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