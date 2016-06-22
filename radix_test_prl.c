/*
 * radix_test_prl.c
 */
#include <inttypes.h>
#include <pthread.h>
#include "radix_tree.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef short bool;
#define true 1
#define false 0

#ifndef cpu_relax
#define cpu_relax() asm volatile("rep; nop" ::: "memory")
#endif

#ifndef ACCESS_ONCE
#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
#endif

#define BILLION 1E9

/*
 * Global Variables
 */
static short err_flag;
static void **items; /* items[key] = lookup for key */
static unsigned long **keys; /* stores randomly generated keys */
static pthread_t *threads;
static unsigned long N_LOOKUPS; /* number of lookups per thread */
static unsigned long N_THREADS; /* number of threads to use for lookups */
static struct radix_tree myTree;

static unsigned long n_ready_threads;
static bool test_start;


/* Prints an error @message and stops execution */
static void die_with_error(char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

/* Allocates memory for an unsigned long @item */
static void *create(unsigned long item)
{
	unsigned long *new = malloc(sizeof(*new));
	*new = item;

	if (!new) {
		die_with_error("malloc failed.\n");
		return NULL;
	} else {
		return (void *)new;
	}
}

static void *thread_find(void *thread_arg)
{
	int *id = thread_arg;

	int i, j;

	i = *id;
	void *temp; /* result of tree lookups */

	__sync_fetch_and_add(&n_ready_threads, 1);

	while (!ACCESS_ONCE(test_start))
		cpu_relax();

	for (j = 0; j < N_LOOKUPS; j++) {
		temp = radix_tree_find(&myTree, keys[i][j]);

		if (items[keys[i][j]] != temp) {
			err_flag = 2;
			break;
		}
	}

	return NULL;
}

/* Prints and error message and frees allocated memory */
static void clean(struct radix_tree *t)
{
	int i;

	if (err_flag)
		fprintf(stderr, "\n[Error number %d detected]\n", err_flag);

	for (i = 0; i < N_THREADS; i++)
		free(keys[i]);

	free(keys);
	free(items);
	radix_tree_delete(t);
}

static void print_usage(char *file_name)
{
	fprintf(stderr, "Error: Invalid number of arguments\n\n");
	fprintf(stderr, "Usage:\t%s r k t p\n", file_name);
	fprintf(stderr, "r: Maximum number of bits and radix\n");
	fprintf(stderr, "k: Number of keys to insert into the trees\n");
	fprintf(stderr, "l: Number of lookups to perform\n");
	fprintf(stderr, "t: Number of test instances\n");
	fprintf(stderr, "p: Number of threads\n");
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
 *
 *	The program stops execution as soon as any of the two kinds of error
 *	is detected.
 */
int main(int argc, char **argv)
{
	uint64_t lookup_time = 0;
	struct timespec start, end;

	int TREE_RANGE; /* maximum number of tracked bits and radix */
	unsigned long N_KEYS; /* number of inserted keys */
	unsigned long N_TESTS; /* number of test instances per execution */
	unsigned long LOOKUPS_RANGE; /* max key to be looked up on the tree */

	int i, j, k;
	int bits;
	int *ids;
	int radix;
	void *temp; /* result of tree lookups */

	if (argc < 6) {
		print_usage(argv[0]);
		return 0;
	}

	TREE_RANGE = atoi(argv[1]);
	N_KEYS = atoi(argv[2]);
	N_LOOKUPS = atoi(argv[3]);
	N_TESTS = atoi(argv[4]);
	N_THREADS = atoi(argv[5]);
	LOOKUPS_RANGE = N_KEYS;

	threads = malloc(sizeof(*threads) * N_THREADS);
	if (!threads)
		die_with_error("failed to allocate threads array.\n");

	keys = malloc(sizeof(**keys) * N_THREADS);
	if (!keys)
		die_with_error("failed to allocate keys array.\n");

	for (i = 0; i < N_THREADS; i++) {
		keys[i] = malloc(sizeof(*keys) * N_LOOKUPS);

		if (!keys[i])
			die_with_error("failed to allocate keys array.\n");
	}

	srand(0);

	/* test loop */
	for (j = 0; j < N_TESTS; j++) {
		n_ready_threads = 0;
		test_start = false;

		bits = (rand() % TREE_RANGE) + 1;
		radix = (rand() % TREE_RANGE) + 1;

		fprintf(stderr, "Test %d\t", j);
		fprintf(stderr, "TESTING TREE: BITS = %d, RADIX = %d\n",
			bits, radix);

		items = calloc(N_KEYS, sizeof(*items));

		if (!items)
			die_with_error("failed to allocate items array.\n");

		radix_tree_init(&myTree, bits, radix);

		/* testing find_alloc */
		for (i = 0; i < N_KEYS; i++) {
			temp = radix_tree_find_alloc(&myTree, i, create);

			if (items[i]) {
				if (temp != items[i])
					err_flag = 1;
			} else if (!temp) {
				err_flag = 1;
			} else {
				items[i] = temp;
			}
		}

		if (err_flag) {
			clean(&myTree);
			return 0;
		}

		/* generates random keys to lookup */
		for (i = 0; i < N_THREADS; i++)
			for (k = 0; k < N_LOOKUPS; k++)
				keys[i][k] = rand() % LOOKUPS_RANGE;

		ids = malloc(sizeof(*ids) * N_THREADS);
		for (i = 0; i < N_THREADS; i++)
			ids[i] = i;

		/* testing find */
		for (i = 0; i < N_THREADS; i++)
			pthread_create(&threads[i], NULL, thread_find,
				&ids[i]);

		while (ACCESS_ONCE(n_ready_threads) != N_THREADS)
			cpu_relax();

		test_start = true;
		clock_gettime(CLOCK_REALTIME, &start);

		for (i = 0; i < N_THREADS; i++)
			pthread_join(threads[i], NULL);

		clock_gettime(CLOCK_REALTIME, &end);

		lookup_time += BILLION * (end.tv_sec - start.tv_sec) +
			end.tv_nsec - start.tv_nsec;

		free(ids);

		if (err_flag) {
			clean(&myTree);
			return 0;
		}

		free(items);
		radix_tree_delete(&myTree);
	}

	for (i = 0; i < N_THREADS; i++)
		free(keys[i]);

	free(keys);
	free(threads);

	printf("%llu\n", (unsigned long long int) lookup_time);

	return 0;
}
