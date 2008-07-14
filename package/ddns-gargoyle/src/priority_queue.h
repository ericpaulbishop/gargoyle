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


#endif //PRIO_QUEUE_H
