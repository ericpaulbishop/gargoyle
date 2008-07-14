/*
 *  Copyright © 2008 by Eric Bishop <eric@gargoyle-router.com>
 * 
 *  This file is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PRIO_QUEUE_H
#define PRIO_QUEUE_H

#include <string.h>
#include <stdlib.h>

typedef struct
{
	int priority;
	char* id;
	
	void* next_node;
	void* previous_node;
	
	void* data;
	
} priority_queue_node;

typedef priority_queue_node** priority_queue;

void insert_priority_node(priority_queue_node** node_list, char* id, int priority, void* data);
void set_node_priority(priority_queue_node** node_list, char* id, int new_priority);
priority_queue_node* remove_priority_node(priority_queue_node** node_list, char* id);
priority_queue_node* get_priority_node(priority_queue_node** node_list, char* id);

priority_queue_node* get_first_in_priority_queue(priority_queue_node** node_list);
priority_queue_node* pop_priority_queue(priority_queue_node** node_list);

priority_queue initialize_priority_queue(void);
void free_empty_priority_queue(priority_queue node_list);

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

#endif //PRIO_QUEUE_H
