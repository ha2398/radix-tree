/*
 * radix_tree.c
 */

#include "radix_tree.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#endif

/* Prints an error @message and stops execution */
void die_with_error(char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

/* Initializes a @node with the appropriate @n_slots number of slots */
void init_node(struct radix_node **node, int n_slots)
{
	*node = calloc(n_slots, sizeof(void *));
	if (!(*node))
		die_with_error("calloc failed.\n");
}

void radix_tree_init(struct radix_tree *tree, int bits, int radix)
{
	int n_slots = 1 << radix;

	tree->radix = radix;
	tree->max_height = DIV_ROUND_UP(bits, radix);

	init_node(&tree->node, n_slots);
}

/* Finds the appropriate slot to follow in the tree */
int find_slot_index(unsigned long key, int levels_left, int radix)
{
	return (int) (key >> (levels_left * radix) & ((1 << radix) - 1));
}

void *radix_tree_find_alloc(struct radix_tree *tree, unsigned long key,
			    void *(*create)(unsigned long))
{
	int levels_left = tree->max_height - 1;
	int radix = tree->radix;
	int n_slots = 1 << radix;
	int index;

	struct radix_node *current_node = tree->node;
	void **next_slot = NULL;

	while (levels_left) {
		index = find_slot_index(key, levels_left, radix);
		next_slot = &current_node->slots[index];

		if (*next_slot) {
			current_node = *next_slot;
		} else if (create) {
			init_node(next_slot, n_slots);	
			current_node = *next_slot;
		} else {
			return NULL;
		}

		levels_left--;
	}

	index = find_slot_index(key, levels_left, radix);
	next_slot = &current_node->slots[index];

	if (*next_slot) {
		return *next_slot;
	} else if (create) {
		*next_slot = create(key);
		return *next_slot;
	} else {
		return NULL;
	}
}	

void *radix_tree_find(struct radix_tree *tree, unsigned long key)
{
	return radix_tree_find_alloc(tree, key, NULL);
}