/*
 * radix_tree.c
 */

#include "radix_tree.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Prints an error @message and stops execution.
void die_with_error(char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

// Initializes a @node with the appropriate @n_slots number of slots.
void init_node(struct radix_node **node, int n_slots)
{
	*node = (struct radix_node*)malloc(sizeof(void *) * n_slots);
	if(!(*node))
		die_with_error("Malloc failed.");

	int i = 0;
	for(; i < n_slots; i++)
		(*node)->slots[i] = NULL;
}

void radix_tree_init(struct radix_tree *tree, int bits, int radix)
{
	tree->radix = radix;
	tree->max_height = bits / radix;

	int slots_len = pow(2, radix);

	init_node(&tree->node, slots_len);
}

// Finds the appropriate slot to follow in the tree.
int find_slot(unsigned long index, int level, int radix)
{	
	unsigned long mask = 0;
	int tmp = radix;
	while(tmp-- != 0)
		mask = (mask << 1) | 1;

	int n_bits = sizeof(mask) * 8;
	int offset = (n_bits - level * radix);

	index = index >> offset;
	mask = mask & index;
	return (int)mask;
}

int radix_tree_insert(struct radix_tree *tree,
		      unsigned long index, void *item)
{
	int current_level = 0;
	int radix = tree->radix;
	int n_slots = pow(2, radix);
	struct radix_node *current_node = tree->node;

	int slot_number;
	while(++current_level != tree->max_height) {	
		slot_number = find_slot(index, current_level, radix);
		if(current_node->slots[slot_number]) {
			current_node = current_node->slots[slot_number];
		} else {

			init_node((struct radix_node **)&current_node->slots[slot_number],
				   n_slots);
		}	
	}

	slot_number = find_slot(index, current_level, radix);
	if(current_node->slots[slot_number]) {
		return 1;
	} else {
		current_node->slots[slot_number] = item;
	}

	return 0;
}

// @TODO
void *radix_tree_find_alloc(struct radix_tree *tree, unsigned long index,
			    void *(*create)(unsigned long),
			    void *(*delete)(void *))
{
	return NULL;
}

// @TODO
void *radix_tree_find(struct radix_tree *tree, unsigned long index)
{
	return NULL;
}