/*
 * Copyright © 2008 by Eric Bishop <eric@gargoyle-router.com>
 * 
 * This work 'as-is' we provide.
 * No warranty, express or implied.
 * We've done our best,
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

#ifndef ERICS_TOOLS_H
#define ERICS_TOOLS_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>


/* tree_map structs / prototypes */
typedef struct long_tree_map_node
{
	unsigned long key;
	void* value;
	
	signed char balance; 
	struct long_tree_map_node* left;
	struct long_tree_map_node* right;
} long_map_node;

typedef struct 
{
	long_map_node* root;
	unsigned long num_elements;

}long_map;

typedef struct
{
	long_map lm;
	unsigned char store_keys;
	unsigned long num_elements;

}string_map;



/* long map functions */
extern long_map* initialize_long_map(void);
extern void* set_long_map_element(long_map* map, unsigned long key, void* value);
extern void* get_long_map_element(long_map* map, unsigned long key);
extern void* remove_long_map_element(long_map* map, unsigned long key);
extern unsigned long* get_sorted_long_map_keys(long_map* map, unsigned long* keys_returned);
extern void** get_sorted_long_map_values(long_map* map, unsigned long* values_returned);
extern void** destroy_long_map(long_map* map, int destruction_type);

/* string map functions */
extern string_map* initialize_string_map(unsigned char store_keys);
extern void* set_string_map_element(string_map* map, const char* key, void* value);
extern void* get_string_map_element(string_map* map, const char* key);
extern void* remove_string_map_element(string_map* map, const char* key);
extern char** get_string_map_keys(string_map* map); 
extern void** get_string_map_values(string_map* map, unsigned long* values_returned);
extern void** destroy_string_map(string_map* map, int destruction_type);

/*
 * three different ways to deal with values when map is destroyed
 */
#define DESTROY_MAP_RETURN_VALUES	1
#define DESTROY_MAP_FREE_VALUES		2
#define DESTROY_MAP_IGNORE_VALUES 	3

/* 
 * for convenience & backwards compatibility alias _string_map_ functions to 
 *  _map_ functions since string map is used more often than long map
 */
#define initialize_map		initialize_string_map
#define set_map_element		set_string_map_element
#define get_map_element		get_string_map_element
#define remove_map_element	remove_string_map_element
#define get_map_keys		get_string_map_keys
#define get_map_values		get_string_map_values
#define destroy_map		destroy_string_map


/* priority_queue structs / prototypes */

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


/* string_util structs / prototypes */

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

#endif /* ERICS_TOOLS_H */
