/*
 * radix_test_prl.c
 */
#include <pthread.h>
#include "radix_tree.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BILLION 1E9

/*
 * Global Variables
 */
static short err_flag;
static void **items; /* items[key[x]] = lookup for key[x] */
static unsigned long *keys; /* stores randomly generated keys */
static pthread_t *threads;
static struct arg *t_args;

/*
 * Struct arg defines all data needed by the threads to perform
 * the lookups on the trees.
 */
struct arg {
	struct radix_tree *tree;
	unsigned long start;
	unsigned long end;
};

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

static void *thread_find_alloc(void *thread_arg)
{
	struct arg *t_arg = thread_arg;

	int i;
	void *temp; /* result of tree lookups */

	for (i = t_arg->start; i < t_arg->end; i++) {
		unsigned long key = keys[i];

		temp = radix_tree_find_alloc(t_arg->tree, key, create);

		if (items[key]) {
			if (temp != items[key])
				err_flag = 1;
		} else if (!temp) {
			err_flag = 1;
		} else {
			items[key] = temp;
		}

		if (err_flag)
			break;
	}

	return NULL;
}

static void *thread_find(void *thread_arg)
{
	struct arg *t_arg = thread_arg;

	int i;
	void *temp; /* result of tree lookups */

	for (i = t_arg->start; i < t_arg->end; i++) {
		unsigned long key = keys[i];

		temp = radix_tree_find(t_arg->tree, key);

		if (items[key] != temp) {
			err_flag = 2;
			break;
		}
	}

	return NULL;
}

/* Prints and error message and frees allocated memory */
static void clean(struct radix_tree *t)
{
	if (err_flag)
		fprintf(stderr, "\n[Error number %d detected]\n", err_flag);

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

	int RANGE;
	unsigned long N_KEYS;
	unsigned long N_TESTS;
	unsigned long N_THREADS;

	int i;
	int j;
	int bits;
	int radix;
	struct radix_tree myTree;
	unsigned long key_max; /* maximum possible value for a key */

	if (argc < 5) {
		print_usage(argv[0]);
		return 0;
	} else {
		RANGE = atoi(argv[1]);
		N_KEYS = atoi(argv[2]);
		N_TESTS = atoi(argv[3]);
		N_THREADS = atoi(argv[4]);
	}

	threads = malloc(sizeof(*threads) * N_THREADS);
	t_args = malloc(sizeof(*t_args) * N_THREADS);
	unsigned long thread_loops = N_KEYS / N_THREADS;

	keys = malloc(sizeof(*keys) * N_KEYS);

	if (!keys)
		die_with_error("failed to allocate keys array.\n");

	srand(0);

	/* test loop */
	for (j = 0; j < N_TESTS; j++) {
		bits = (rand() % RANGE) + 1;
		key_max = ((1L << bits) - 1L);
		radix = (rand() % RANGE) + 1;

		fprintf(stderr, "Test %d\t", j);
		fprintf(stderr, "TESTING TREE: BITS = %d, RADIX = %d\n",
			bits, radix);

		items = calloc(key_max, sizeof(*items));

		if (!items)
			die_with_error("failed to allocate items array.\n");

		radix_tree_init(&myTree, bits, radix);

		for (i = 0; i < N_KEYS; i++) /* generates random keys */
			keys[i] = rand() % key_max;

		/* testing find_alloc */
		for (i = 0; i < N_THREADS; i++) {
			t_args[i].tree = &myTree;
			t_args[i].start = i * thread_loops;
			t_args[i].end = t_args[i].start + thread_loops;

			pthread_create(&threads[i], NULL,
				       thread_find_alloc, &t_args[i]);
		}

		for (i = 0; i < N_THREADS; i++)
			pthread_join(threads[i], NULL);

		if (err_flag) {
			clean(&myTree);
			return 0;
		}

		for (i = 0; i < N_KEYS; i++) /* generates random keys */
			keys[i] = rand() % key_max;

		clock_gettime(CLOCK_REALTIME, &start);

		/* testing find */
		for (i = 0; i < N_THREADS; i++) {
			t_args[i].tree = &myTree;
			t_args[i].start = i * thread_loops;
			t_args[i].end = t_args[i].start + thread_loops;

			pthread_create(&threads[i], NULL, thread_find,
				       &t_args[i]);
		}

		for (i = 0; i < N_THREADS; i++)
			pthread_join(threads[i], NULL);

		clock_gettime(CLOCK_REALTIME, &end);

		lookup_time += BILLION * (end.tv_sec - start.tv_sec) +
			end.tv_nsec - start.tv_nsec;

		if (err_flag) {
			clean(&myTree);
			return 0;
		}

		free(items);
		radix_tree_delete(&myTree);
	}

	free(keys);
	free(threads);
	free(t_args);

	printf("%llu\n", (long long unsigned int) lookup_time);

	return 0;
}