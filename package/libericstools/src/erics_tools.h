/*
 *  Copyright © 2008 by Eric Bishop <eric@gargoyle-router.com>
 * 
 *  This file is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU Lesser General Public License as published by the
 *  Free Software Foundation, either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef ERICS_TOOLS_H
#define ERICS_TOOLS_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

// priority_queue structs / prototypes

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


// string_map structs / prototypes


typedef struct
{
	unsigned long hashed_key;
	void* value;
	char* key_str;

	void* left;
	void* right;
} map_node;

typedef struct 
{
	map_node* root;
	int num_elements;
	unsigned char store_keys;

}string_map;

string_map* initialize_map(unsigned char store_keys);

void* set_map_element(string_map* map, const char* key, char* value);
void* get_map_element(string_map* map, const char* key);
void* remove_map_element(string_map* map, const char* key);
char** get_map_keys(string_map* map);
void get_node_keys(map_node* node, char** key_list, int* next_key_index);

unsigned long sdbm_string_hash(const char *key);

// string_util structs / prototypes

typedef struct 
{
	char* str;
	int terminator;
} dyn_read_t;

char* replace_prefix(char* original, char* old_prefix, char* new_prefix);
char* trim_flanking_whitespace(char* str);
dyn_read_t dynamic_read(FILE* open_file, char* terminators, int num_terminators);
char* read_entire_file(FILE* in, int read_block_size);
char* dynamic_strcat(int num_strs, ...);
int safe_strcmp(const char* str1, const char* str2);
char** split_on_separators(char* line, char* separators, int num_separators, int max_pieces, int include_remainder_at_max);
void to_lowercase(char* str);
void to_uppercase(char* str);

#endif //ERICS_TOOLS_H
