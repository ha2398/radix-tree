#include "radix_tree.h"

void radix_tree_init(struct radix_tree *tree, int bits, int radix)
{

}

void *radix_tree_find_alloc(struct radix_tree *tree, unsigned long index,
                            void *(*create)(unsigned long),
                            void *(*delete)(void *))
{

}

void *radix_tree_find(struct radix_tree *tree, unsigned long index)
{
	
}