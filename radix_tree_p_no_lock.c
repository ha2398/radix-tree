/*
 * radix_tree_p_no_lock.c
 */

#include "radix_tree.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#endif

#ifndef ACCESS_ONCE
#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
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
}

/* Finds the appropriate slot to follow in the tree */
static int find_slot_index(unsigned long key, int levels_left, int radix)
{
	return key >> ((levels_left - 1) * radix) & ((1 << radix) - 1);
}

void *radix_tree_find_alloc(struct radix_tree *tree, unsigned long key,
			    void *(*create)(unsigned long))
{
	int levels_left = tree->max_height;
	int radix = tree->radix;
	int n_slots = 1 << radix;
	int index;
	struct radix_node *current_node = tree->node;
	void **next_slot = NULL;
	void *slot;

	while (levels_left) {
		index = find_slot_index(key, levels_left, radix);

		next_slot = &current_node->slots[index];

retry:
		slot = ACCESS_ONCE(*next_slot);

		if (slot) {
			current_node = slot;
		} else if (create) {
			void *new;

			if (levels_left != 1)
				new = calloc(sizeof(struct radix_node) +
					(n_slots * sizeof(void *)), 1);
			else
				new = create(key);

			if (!new)
				die_with_error("failed to create new node.\n");

			if (__sync_bool_compare_and_swap(next_slot,
				NULL, new)) {
				current_node = new;
			} else {
				free(new);
				goto retry;
			}
		} else {
			return NULL;
		}

		levels_left--;
	}

	return current_node;
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
