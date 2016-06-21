/*
 * radix_tree_p_lock_node.c
 */

#include <pthread.h>
#include "radix_tree_pln.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#endif

/*
 * Global variables 
 */

/* Prints an error @message and stops execution */
static void die_with_error(char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

void radix_tree_init(struct radix_tree *tree, int bits, int radix)
{
	if (radix < 1) {
		perror("Invalid radix\n");
		return;
	}

	if (bits < 1) {
		perror("Invalid number of bits\n");
		return;
	}

	unsigned long n_slots = 1L << radix;

	tree->radix = radix;
	tree->max_height = DIV_ROUND_UP(bits, radix);

	tree->node = calloc(sizeof(struct radix_node) +
		     		  (n_slots * sizeof(void *)), 1);

	if (!tree->node)
		die_with_error("failed to create new node.\n");
	else
		pthread_mutex_init(&(tree->node->lock), NULL);
}

/* Finds the appropriate slot to follow in the tree */
static int find_slot_index(unsigned long key, int levels_left, int radix)
{
	return (int) (key >> (levels_left * radix) & ((1 << radix) - 1));
}

void *radix_tree_find_alloc(struct radix_tree *tree, unsigned long key,
			    void *(*create)(unsigned long))
{
	int current_level = 0;
	int levels_left = tree->max_height - 1;
	int radix = tree->radix;
	int n_slots = 1 << radix;
	int index;
	pthread_mutex_t *lock;

	struct radix_node *current_node = tree->node;
	void **next_slot = NULL;

	while (levels_left) {
		index = find_slot_index(key, levels_left, radix);

		lock = &(current_node->lock);
		pthread_mutex_lock(lock);

		next_slot = &current_node->slots[index];

		if (*next_slot) {
			current_node = *next_slot;
		} else if (create) {
			*next_slot = calloc(sizeof(struct radix_node) +
		     		  	   (n_slots * sizeof(void *)), 1);

			if (!*next_slot) {
				die_with_error("failed to create new node.\n");
			} else {
				current_node = *next_slot;
				pthread_mutex_init(&(current_node->lock),
						   NULL);
			}
		} else {
			pthread_mutex_unlock(lock);
			return NULL;
		}

		pthread_mutex_unlock(lock);

		current_level++;
		levels_left--;
	}

	index = find_slot_index(key, levels_left, radix);

	lock = &(current_node->lock);
	pthread_mutex_lock(lock);

	next_slot = &current_node->slots[index];

	if (!(*next_slot) && create)
		*next_slot = create(key);

	pthread_mutex_unlock(lock);

	return *next_slot;
}

void *radix_tree_find(struct radix_tree *tree, unsigned long key)
{
	return radix_tree_find_alloc(tree, key, NULL);
}

static void radix_tree_delete_node(struct radix_node *node, int n_slots,
				   int levels_left)
{
	int i;
	struct radix_node *next_node = NULL;

	if (levels_left) {
		for (i = 0; i < n_slots; i++) {
			next_node = node->slots[i];

			if (next_node) {
				radix_tree_delete_node(next_node, n_slots,
						       levels_left - 1);

				pthread_mutex_destroy(&(next_node->lock));
				free(next_node);
			}
		}
	} else {
		for (i = 0; i < n_slots; i++) {
			if (node->slots[i])
				free(node->slots[i]);
		}
	}
}

void radix_tree_delete(struct radix_tree *tree)
{
	int n_slots = 1 << tree->radix;

	radix_tree_delete_node(tree->node, n_slots, tree->max_height - 1);
	free(tree->node);
}