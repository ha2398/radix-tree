/*
 * radix_test.c
 */

#include "radix_tree.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef cpu_relax
#define cpu_relax() asm volatile("rep; nop" ::: "memory")
#endif

#ifndef ACCESS_ONCE
#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#endif

#define BILLION 1E9

/* Default parameter values */
#define BITS 16
#define RADIX 4
#define KEYS 30000
#define LOOKUPS 60000
#define THREADS 4

#define OPTSTRING "b:r:k:l:p:i:"

/*
 * Global Variables
 */
static short err_flag;
static void **items; /* items[key] = lookup for key */
static unsigned long **keys; /* stores randomly generated keys */
static pthread_t *threads;
static unsigned long n_lookups; /* number of lookups per thread */
static unsigned long n_threads; /* number of threads to use for lookups */
static struct radix_tree myTree;

static struct radix_tree_desc *impl_desc;

static unsigned long n_ready_threads;
static bool test_start;

/* Implementations' Descriptors */

struct radix_tree_desc *descs[] = {
        [0] = &sequential_desc,
        [1] = &lock_level_desc,
        [2] = &lock_node_desc,
        [3] = &lock_subtree_desc,
        [4] = &lockless_desc
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

static void *thread_find(void *thread_arg)
{
	int *id = thread_arg;

	int i, j;

	i = *id;
	void *temp; /* result of tree lookups */

	__sync_fetch_and_add(&n_ready_threads, 1);

	while (!ACCESS_ONCE(test_start))
		cpu_relax();

	for (j = 0; j < n_lookups; j++) {
		temp = impl_desc->find(&myTree, keys[i][j]);

		if (items[keys[i][j]] != temp) {
			err_flag = 2;
			break;
		}
	}

	return NULL;
}

static void print_usage(char *file_name)
{
	fprintf(stderr, "Usage:\t%s [-b bits] [-r radix] [-k keys]"
		"[-l lookups] [-p threads] [-i implementation]\n", file_name);
}

static struct radix_tree_desc *radix_tree_desc_find(const char *name)
{
	int i;	
	unsigned long array_size = ARRAY_SIZE(descs);

	for (i = 0; i < array_size; i++) {
		struct radix_tree_desc *desc = descs[i];

		if (!strcmp(desc->name, name))
			return desc;
	}

	return NULL;
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
	double lookup_time = 0;
	struct timespec start, end;

	unsigned int bits; /* number of tracked bits */
	unsigned int radix;
	unsigned long n_keys; /* number of intended inserted keys */
	unsigned long inserted_keys; /* number of effectively inserted key */
	unsigned long lookups_range; /* max key to be looked up on the tree */

	int i, k, opt;
	int *ids;
	void *temp; /* result of tree lookups */

	extern char *optarg;

	/* Default parameter values */

	bits = BITS;
	radix = RADIX;
	n_keys = KEYS;
	n_lookups = LOOKUPS;
	n_threads = THREADS;
	impl_desc = descs[0];

	opt = getopt(argc, argv, OPTSTRING);
	while (opt != -1) {
		switch (opt) {
		case 'b':
			bits = atoi(optarg);
			break;
		case 'r':
			radix = atoi(optarg);
			break;
		case 'k':
			n_keys = atoi(optarg);
			lookups_range = n_keys;
			break;
		case 'l':
			n_lookups = atoi(optarg);
			break;
		case 'p':
			n_threads = atoi(optarg);
			break;
		case 'i':
			impl_desc = radix_tree_desc_find(optarg);
			if (impl_desc == NULL) {
				fprintf(stderr, "fatal: implementation "
					"'%s' not found\n", optarg);
				print_usage(argv[0]);
				return -1;
			}
			break;
		case '?':
		case ':':
			print_usage(argv[0]);
			return -1;
		}

		opt = getopt(argc, argv, OPTSTRING);
	}

	lookups_range = (1L << bits) - 1;

	inserted_keys = (n_keys > lookups_range) ? lookups_range : n_keys;

	threads = malloc(sizeof(*threads) * n_threads);
	if (!threads)
		die_with_error("failed to allocate threads array.\n");

	keys = malloc(sizeof(**keys) * n_threads);
	if (!keys)
		die_with_error("failed to allocate keys array.\n");

	for (i = 0; i < n_threads; i++) {
		keys[i] = malloc(sizeof(*keys) * n_lookups);

		if (!keys[i])
			die_with_error("failed to allocate keys array.\n");
	}

	srand(0);

	/* Test */

	n_ready_threads = 0;
	test_start = false;

	fprintf(stderr, "TESTING TREE: BITS = %d, RADIX = %d\n",
		bits, radix);

	items = calloc(lookups_range, sizeof(*items));

	if (!items)
		die_with_error("failed to allocate items array.\n");

	impl_desc->init(&myTree, bits, radix);

	/* testing find_alloc */
	for (i = 0; i < inserted_keys; i++) {
		temp = impl_desc->find_alloc(&myTree, i, create);

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

		return err_flag;
	}

	/* generates random keys to lookup */
	for (i = 0; i < n_threads; i++)
		for (k = 0; k < n_lookups; k++)
			keys[i][k] = rand() % lookups_range;

	ids = malloc(sizeof(*ids) * n_threads);
	for (i = 0; i < n_threads; i++)
		ids[i] = i;

	/* testing find */
	for (i = 0; i < n_threads; i++)
		pthread_create(&threads[i], NULL, thread_find,
			&ids[i]);

	while (ACCESS_ONCE(n_ready_threads) != n_threads)
		cpu_relax();

	test_start = true;
	clock_gettime(CLOCK_REALTIME, &start);

	for (i = 0; i < n_threads; i++)
		pthread_join(threads[i], NULL);

	clock_gettime(CLOCK_REALTIME, &end);

	lookup_time += end.tv_sec - start.tv_sec +
		(end.tv_nsec - start.tv_nsec) / BILLION;

	if (err_flag) {
		fprintf(stderr, "\n[Error number %d detected]\n",
			err_flag);

		return err_flag;
	}

	printf("%f", lookup_time);

	return 0;
}
