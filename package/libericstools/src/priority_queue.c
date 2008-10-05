/*
 * Copyright © 2008 by Eric Bishop <eric@gargoyle-router.com>
 * 
 * This work ‘as-is’ we provide.
 * No warranty, express or implied.
 * We’ve done our best,
 * to debug and test.
 * Liability for damages denied.
 *
 * Permission is granted hereby,
 * to copy, share, and modify.
 * Use as is fit,
 * free or for profit.
 * On this notice these rights rely.
 *
 *
 *
 *  Note that unlike other portions of Gargoyle this code
 *  does not fall under the GPL, but the rather whimsical
 *  'Poetic License' above.
 *
 *  Basically, this library contains a bunch of utilities
 *  that I find useful.  I'm sure other libraries exist
 *  that are just as good or better, but I like these tools 
 *  because I personally wrote them, so I know their quirks.
 *  (i.e. I know where the bodies are buried).  I want to 
 *  make sure that I can re-use these utilities for whatever
 *  code I may want to write in the future be it
 *  proprietary or open-source, so I've put them under
 *  a very, very permissive license.
 *
 *  If you find this code useful, use it.  If not, don't.
 *  I really don't care.
 *
 */

#include "erics_tools.h"

priority_queue initialize_priority_queue(void)
{
	priority_queue node_list =  (priority_queue_node**)malloc(sizeof(priority_queue_node*));
	*node_list = NULL;
	return node_list;
}
void free_empty_priority_queue(priority_queue node_list)
{
	if(*node_list == NULL)
	{
		free(node_list);
	}
}

void insert_priority_node(priority_queue_node** node_list, char* id, int priority, void* data)
{
	priority_queue_node* new_node = malloc(sizeof(priority_queue_node));
	new_node->id = id;
	new_node->priority = priority;
	new_node->data = data;
	if(*node_list == NULL)
	{
		new_node->previous_node = NULL;
		new_node->next_node = NULL;
		*node_list = new_node;
	}
	else
	{
		priority_queue_node* test_node = *node_list;
		while(test_node->priority < priority && test_node->next_node != NULL)
		{
			test_node = (priority_queue_node*)test_node->next_node;
		}
		if(test_node->priority < priority) //node goes at end of list
		{
			new_node->next_node = NULL;
			new_node->previous_node = (void *)test_node;
			test_node->next_node = (void *)new_node;
		}
		else //node goes at beginning or middle of list
		{
			new_node->next_node = (void*)test_node;
			new_node->previous_node = (void*)test_node->previous_node;
			test_node->previous_node = (void*)new_node;
			if(new_node->previous_node != NULL) //node goes in middle of list
			{
				priority_queue_node* prev = (priority_queue_node*)new_node->previous_node;
				prev->next_node = (void*)new_node;
			}
			else //node goes at beginning of list
			{
				*node_list = new_node;
			}
		}

	}

}

void set_node_priority(priority_queue_node** node_list, char* id, int new_priority)
{
	
	//remove node from current position
	priority_queue_node* removed_node = remove_priority_node(node_list, id);
	if(removed_node != NULL)
	{
		// use insert function to properly position node with new priority	
		insert_priority_node(node_list, removed_node->id, new_priority, removed_node->data);
			
		// free test node, since insert function dynamically allocates a new node
		// we deliberately pass same id/data pointers from old node to insert
		// function so the only thing we need to free (that no longer is used)
		//  is the old node itself
		free(removed_node);
	}

}

priority_queue_node* remove_priority_node(priority_queue_node** node_list, char* id)
{

	priority_queue_node* remove_priority_node = get_priority_node(node_list, id);
	if(remove_priority_node != NULL)
	{
		//remove node from current position
		if(remove_priority_node->next_node != NULL)
		{
			priority_queue_node* next = (priority_queue_node*)remove_priority_node->next_node;
			next->previous_node = remove_priority_node->previous_node;
		}
		if(remove_priority_node->previous_node != NULL)
		{
			priority_queue_node* prev = (priority_queue_node*)remove_priority_node->previous_node;
			prev->next_node = remove_priority_node->next_node;
		}
		else
		{
			*node_list = (priority_queue_node*)remove_priority_node->next_node;
		}
	}
	
	return remove_priority_node;
}

priority_queue_node* get_priority_node(priority_queue_node** node_list, char* id)
{
	priority_queue_node* test_node = NULL;
	if(*node_list != NULL)
	{
		//find node
		test_node = *node_list;
		while(strcmp(id, test_node->id) != 0 && test_node->next_node != NULL)
		{
			test_node = (priority_queue_node*)test_node->next_node;
		}
		if(strcmp(id, test_node->id) != 0)
		{
			test_node = NULL;
		}
	}
	return test_node;
}
priority_queue_node* get_first_in_priority_queue(priority_queue_node** node_list)
{
	priority_queue_node* first_node = NULL;
	if(node_list != NULL)
	{
		first_node =  *node_list;
	}
	return first_node;
}
priority_queue_node* pop_priority_queue(priority_queue_node** node_list)
{
	priority_queue_node* first_node = NULL;
	if(node_list != NULL)
	{
		if(*node_list != NULL)
		{
			first_node =  remove_priority_node(node_list, (*node_list)->id);
		}
	}
	return first_node;
}

