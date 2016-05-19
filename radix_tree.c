/*
 * radix_tree.c
 */

#include "radix_tree.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef ROUND_UP
#define ROUND_UP(n,d) (((n) + (d) - 1) & -(d))
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
	*node = (struct radix_node *)calloc(n_slots, sizeof(void *));
	if (!(*node))
		die_with_error("calloc failed.\n");
}

void radix_tree_init(struct radix_tree *tree, int bits, int radix)
{
	tree->radix = radix;
	tree->max_height = ROUND_UP(bits, radix);

	int slots_len = 1 << radix;

	init_node(&tree->node, slots_len);
}

/* Finds the appropriate slot to follow in the tree */
int find_slot(unsigned long index, int level, int radix)
{
	unsigned long mask = 0;
	int tmp = radix;

	while (tmp-- != 0)
		mask = (mask << 1) | 1;

	int n_bits = sizeof(mask) * 8;
	int offset = (n_bits - level * radix);

	index = index >> offset;
	mask = mask & index;
	return (int)mask;
}

void *radix_tree_find_alloc(struct radix_tree *tree, unsigned long index,
			    void *(*create)(unsigned long))
{
	int current_level = 0;
	struct radix_node *curr_node = tree->node;
	int radix = tree->radix;
	int n_slots = 1 << radix;
	int slot;

	while (++current_level != tree->max_height) {
		slot = find_slot(index, current_level, radix);
		if (curr_node->slots[slot]) {
			curr_node = curr_node->slots[slot];
		} else {
			init_node((struct radix_node **)&curr_node->slots[slot],
				   n_slots);
			curr_node = curr_node->slots[slot];
		}
	}

	slot = find_slot(index, current_level, radix);
	if (curr_node->slots[slot]) {
		return curr_node->slots[slot];
	} else if (create) {
		curr_node->slots[slot] = create(index);
		return curr_node->slots[slot];
	} else {
		return NULL;
	}
}

/* @TODO */
void *radix_tree_find(struct radix_tree *tree, unsigned long index)
{
	return radix_tree_find_alloc(tree, index, NULL);
}