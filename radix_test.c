/*
 * radix_test.c
 */

#include "radix_tree.h"
#include <stdio.h>
#include <stdlib.h>

/* Allocates memory for an unsigned long item */
static void *allocate_item(unsigned long item)
{
	unsigned long *new = malloc(sizeof(unsigned long));
	*new = item;

	if (!new) {
		perror("Malloc failed.\n");
		return NULL;
	} else {
		return (void *)new;
	}
}

int main(int argc, char **argv)
{
	struct radix_tree myTree;

	radix_tree_init(&myTree, 6, 2);

	void *item = radix_tree_find(&myTree, 39);

	printf("%p\n", item);

	item = radix_tree_find_alloc(&myTree, 39, allocate_item);

	printf("%p\n", item);

	void *item2 = radix_tree_find(&myTree, 39);

	printf("%p\n", item2);

	return 0;
}
