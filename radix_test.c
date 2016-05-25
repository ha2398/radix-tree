/*
 * radix_test.c
 */

#include "radix_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RANGE 16 /* maximum value for bits or radix */
#define N_KEYS 100000
#define N_TESTS 100

/* Prints an error @message and stops execution */
static void die_with_error(char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

/* Allocates memory for an unsigned long @item */
static void *create(unsigned long item)
{
	unsigned long *new = malloc(sizeof(unsigned long));
	*new = item;

	if (!new) {
		perror("malloc failed.\n");
		return NULL;
	} else {
		return (void *)new;
	}
}

/*
 * Test can detect two kinds of errors:
 * 1) Error in find_alloc
 *	i) key is already in the tree, but find_alloc returns an address which
 *	differs from the actual key's location.
 *	ii) key is not present in the tree, but find_alloc returns NULL
 *
 * 2) Error in find
 *	i) find returns an address which differs from the key's location in
 *	the tree (which can be NULL, if the key hasn't been inserted)
 */
int main(int argc, char **argv)
{
	int i, j;
	int bits;
	int radix;
	int tests = N_TESTS;
	short err_flag = 0;
	struct radix_tree myTree;
	time_t t;
	unsigned long *keys; /* stores randomly generated keys */
	unsigned long key_max; /* maximum possible value for a key */
	void **items; /* items[key[x]] = lookup for key[x] */
	void *temp; /* result of tree lookups */

	keys = malloc(sizeof(*keys) * N_KEYS);

	if (!keys)
		die_with_error("failed to allocate keys array.\n");

	srand((unsigned int) time(&t));

	/* test loop */
	for (j = 0; j < tests; j++) {
		bits = (rand() % RANGE) + 1;
		key_max = (unsigned long) ((1L << bits) - 1L);
		radix = (rand() % RANGE) + 1;

		printf("Test %d\t", j);
		printf("TESTING TREE: BITS = %d, RADIX = %d\n", bits, radix);

		items = calloc(key_max, sizeof(*items));

		if (!items)
			die_with_error("failed to allocate items array.\n");

		radix_tree_init(&myTree, bits, radix);

		for (i = 0; i < N_KEYS; i++)
			keys[i] = rand() % key_max;

		/* testing find_alloc */
		for (i = 0; i < N_KEYS; i++) {
			temp = radix_tree_find_alloc(&myTree, keys[i],
						     create);

			if (items[keys[i]]) {
				if (temp != items[keys[i]])
					err_flag = 1;
			} else if (!temp) {
				err_flag = 1;
			} else {
				items[keys[i]] = temp;
			}
		}

		for (i = 0; i < N_KEYS; i++)
			keys[i] = rand() % key_max;

		/* testing find */
		for (i = 0; i < N_KEYS; i++) {
			temp = radix_tree_find(&myTree, keys[i]);

			if (items[keys[i]] != temp)
				err_flag = 2;
		}

		free(items);

		radix_tree_delete(&myTree);

		if (err_flag) {
			printf("\nError number %d detected\n", err_flag);
			free(keys);
			return 0;
		}
	}

	free(keys);
	return 0;
}
