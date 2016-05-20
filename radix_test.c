/*
 * radix_test.c
 */

#include "radix_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RANGE 8 /* maximum value for bits or radix */
#define KEY_MAX 255 /* maximum possible value for a key */
#define N_KEYS 50
#define N_TESTS 20

/* Allocates memory for an unsigned long @item */
static void *create(unsigned long item)
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
	int i, j;
	int bits;
	int radix;
	int tests = N_TESTS;
	short err_flag = 0;
	struct radix_tree myTree[tests];
	time_t t;
	unsigned long keys[N_KEYS]; /* stores randomly generated keys */
	void **items; /* items[key[x]] = lookup for key[x] */
	void *temp; /* result of tree lookups */

	srand((unsigned int) time(&t));

	items = malloc(KEY_MAX * sizeof(*items));

	/* test loop */
	for (j = 0; j < tests; j++) {
		bits = (rand() % RANGE) + 1;
		radix = (rand() % RANGE) + 1;

		radix_tree_init(&myTree[j], bits, radix);

		printf("TESTING TREE: BITS = %d, RADIX = %d\n", bits, radix);

		for (i = 0; i < KEY_MAX; i++)
			items[i] = NULL;

		for (i = 0; i < N_KEYS; i++)
			keys[i] = rand() % KEY_MAX;

		/* testing find_alloc */
		for (i = 0; i < N_KEYS; i++) {
			temp = radix_tree_find_alloc(&myTree[j], keys[i],
						     create);

			if (items[keys[i]])
				if (temp != items[keys[i]])
					err_flag = 1;
			else if (!temp)
				err_flag = 1;
			else
				items[keys[i]] = temp;
		}

		for (i = 0; i < N_KEYS; i++)
			keys[i] = rand() % KEY_MAX;

		/* testing find */
		for (i = 0; i < N_KEYS; i++) {
			temp = radix_tree_find(&myTree[j], keys[i]);

			if (items[keys[i]] != temp)
				err_flag = 2;
		}

		if (err_flag) {
			printf("\nError number %d detected\n", err_flag);
			return 0;
		}
	}

	return 0;
}
