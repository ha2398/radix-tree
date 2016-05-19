/*
 * radix_test.c
 */

#include "radix_tree.h"
#include <stdio.h>
#include <stdlib.h>

/* Allocates memory for an unsigned long item */
void *allocate_item(unsigned long item)
{
	unsigned long *new = (unsigned long *)malloc(sizeof(unsigned long));

	if(!new) {
		perror("Malloc failed.\n");
		return NULL;
	} else {
		return (void *)new;
	}
}

int main(int argc, char **argv)
{
	struct radix_tree myTree;

	radix_tree_init(&myTree, 64, 8);

	void *item = radix_tree_find(&myTree, 123130384);

	if(!item)
		printf("Find for 123130384 returned NULL.\n");

	item = radix_tree_find_alloc(&myTree, 123130384, allocate_item);

	if(item)
		printf("Find alloc for 123130384 returned valid pointer.\n");

	void *item2 = radix_tree_find(&myTree, 123130384);

	if(item == item2) {
		printf("Find for 123130384 returned same pointer as");
		printf(" previously.\n");
	}

	return 0;
}