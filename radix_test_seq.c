/*
 * radix_test_seq.c
 */

#include "radix_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BILLION 1E9

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
	fprintf(stderr, "Usage:\t%s r k l t\n", file_name);
	fprintf(stderr, "r: Maximum number of bits and radix\n");
	fprintf(stderr, "k: Number of keys to insert into the trees\n");
	fprintf(stderr, "l: Number of lookups to perform\n");
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
	double lookup_time = 0;
	struct timespec start, end;

	int tree_range; /* maximum number of tracked bits and radix */
	unsigned long n_keys; /* number of inserted keys */
	unsigned long n_tests; /* number of test instances per execution */
	unsigned long n_lookups; /* number of lookups per thread */
	unsigned long lookups_range; /* max key to be looked up on the tree */

	int i, j;
	int bits;
	int radix;
	short err_flag = 0;
	struct radix_tree myTree;
	unsigned long *keys; /* stores randomly generated keys */
	void **items; /* items[i] = lookup for i */
	void *temp; /* result of tree lookups */

	if (argc < 5) {
		print_usage(argv[0]);
		return 0;
	}

	tree_range = atoi(argv[1]);
	n_keys = atoi(argv[2]);
	n_lookups = atoi(argv[3]);
	n_tests = atoi(argv[4]);
	lookups_range = n_keys;

	keys = malloc(sizeof(*keys) * n_lookups);

	if (!keys)
		die_with_error("failed to allocate keys array.\n");

	srand(0);

	/* test loop */
	for (j = 0; j < n_tests; j++) {
		bits = (rand() % tree_range) + 1;
		radix = (rand() % tree_range) + 1;

		fprintf(stderr, "Test %d\t", j);
		fprintf(stderr, "TESTING TREE: BITS = %d, RADIX = %d\n",
			bits, radix);

		items = calloc(n_keys, sizeof(*items));

		if (!items)
			die_with_error("failed to allocate items array.\n");

		radix_tree_init(&myTree, bits, radix);

		/* testing find_alloc */
		for (i = 0; i < n_keys; i++) {
			temp = radix_tree_find_alloc(&myTree, i, create);

			if (items[i]) {
				if (temp != items[i]) {
					err_flag = 1;
					break;
				}
			} else if (!temp) {
				err_flag = 1;
				break;
			} else {
				items[i] = temp;
			}
		}

		if (err_flag) {
			fprintf(stderr, "\n[Error number %d detected]\n",
				err_flag);

			return 0;
		}

		/* generates random keys to lookup */
		for (i = 0; i < n_lookups; i++)
			keys[i] = rand() % lookups_range;

		clock_gettime(CLOCK_REALTIME, &start);

		/* testing find */
		for (i = 0; i < n_lookups; i++) {
			temp = radix_tree_find(&myTree, keys[i]);

			if (items[keys[i]] != temp) {
				err_flag = 2;
				break;
			}
		}

		clock_gettime(CLOCK_REALTIME, &end);

		lookup_time += end.tv_sec - start.tv_sec +
			(end.tv_nsec - start.tv_nsec) / BILLION;

		free(items);
		radix_tree_delete(&myTree);

		if (err_flag) {
			fprintf(stderr, "\n[Error number %d detected]\n",
				err_flag);

			return 0;
		}
	}

	printf("%f\n", lookup_time);

	return 0;
}
