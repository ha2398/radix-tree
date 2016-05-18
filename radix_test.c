/*
 * radix_test.c
 */

#include "radix_tree.h"
#include <stdio.h>
#include <stdlib.h>	

int main(int argc, char **argv)
{
	struct radix_tree myTree;
	radix_tree_init(&myTree, 64, 8);
	
	int item1 = 413;
	radix_tree_insert(&myTree, 0xABCDEF1234ABCDEF, &item1);

	return 0;
}