/*
 * radix_test.c
 */

#include "radix_tree.h"
#include <stdio.h>
#include <stdlib.h>

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
		die_with_error("malloc failed.\n");
		return NULL;
	} else {
		return (void *)new;
	}
}

static void print_usage(char *file_name)
{
	fprintf(stderr, "Error: Invalid number of arguments\n\n");
	fprintf(stderr, "Usage:\t%s r k t\n", file_name);
	fprintf(stderr, "r: Maximum number of bits and radix\n");
	fprintf(stderr, "k: Number of keys to insert into the trees\n");
	fprintf(stderr, "t: Number of test instances\n");
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
	int RANGE;
	unsigned long N_KEYS;
	unsigned long N_TESTS;

	int i, j;
	int bits;
	int radix;
	short err_flag = 0;
	struct radix_tree myTree;
	unsigned long *keys; /* stores randomly generated keys */
	unsigned long key_max; /* maximum possible value for a key */
	void **items; /* items[key[x]] = lookup for key[x] */
	void *temp; /* result of tree lookups */

	if (argc < 4) {
		print_usage(argv[0]);
		return 0;
	} else {
		RANGE = atoi(argv[1]);
		N_KEYS = atoi(argv[2]);
		N_TESTS = atoi(argv[3]);
	}

	keys = malloc(sizeof(*keys) * N_KEYS);

	if (!keys)
		die_with_error("failed to allocate keys array.\n");

	srand(0);

	/* test loop */
	for (j = 0; j < N_TESTS; j++) {
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
			fprintf(stderr, "\n[Error number %d detected]\n", err_flag);
			free(keys);
			return 0;
		}
	}

	free(keys);
	return 0;
}
