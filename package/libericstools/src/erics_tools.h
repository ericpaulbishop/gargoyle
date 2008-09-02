/*
 *  Copyright © 2008 by Eric Bishop <eric@gargoyle-router.com>
 * 
 *  NOTE THAT UNLIKE OTHER PARTS OF GARGOYLE THIS LIBRARY FALLS UNDER THE LGPL, NOT THE GPL
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

extern void insert_priority_node(priority_queue_node** node_list, char* id, int priority, void* data);
extern void set_node_priority(priority_queue_node** node_list, char* id, int new_priority);
extern priority_queue_node* remove_priority_node(priority_queue_node** node_list, char* id);
extern priority_queue_node* get_priority_node(priority_queue_node** node_list, char* id);

extern priority_queue_node* get_first_in_priority_queue(priority_queue_node** node_list);
extern priority_queue_node* pop_priority_queue(priority_queue_node** node_list);

extern priority_queue initialize_priority_queue(void);
extern void free_empty_priority_queue(priority_queue node_list);


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

extern string_map* initialize_map(unsigned char store_keys);

extern void* set_map_element(string_map* map, const char* key, char* value);
extern void* get_map_element(string_map* map, const char* key);
extern void* remove_map_element(string_map* map, const char* key);
extern char** get_map_keys(string_map* map);
extern void get_node_keys(map_node* node, char** key_list, int* next_key_index);

extern unsigned long sdbm_string_hash(const char *key);

// string_util structs / prototypes

typedef struct 
{
	char* str;
	int terminator;
} dyn_read_t;

extern char* replace_prefix(char* original, char* old_prefix, char* new_prefix);
extern char* trim_flanking_whitespace(char* str);
extern dyn_read_t dynamic_read(FILE* open_file, char* terminators, int num_terminators);
extern char* read_entire_file(FILE* in, int read_block_size);
extern char* dynamic_strcat(int num_strs, ...);
extern int safe_strcmp(const char* str1, const char* str2);
extern char** split_on_separators(char* line, char* separators, int num_separators, int max_pieces, int include_remainder_at_max);
extern void to_lowercase(char* str);
extern void to_uppercase(char* str);

#endif //ERICS_TOOLS_H
