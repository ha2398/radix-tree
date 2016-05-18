/*
 * radix_test.c
 */

#include "radix_tree.h"
 #include <stdlib.h>	

int main(int argc, char **argv)
{
	struct radix_tree *myTree = NULL;
	radix_tree_init(myTree, 6, 2);

	return 0;
}