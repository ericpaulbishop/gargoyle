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


#ifndef STRING_MAP_H
#define STRING_MAP_H

#include <stdlib.h>
#include <string.h>


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

}string_map;

string_map* initialize_map(void);
void* set_map_element(string_map* map, const char* key, char* value);
void* get_map_element(string_map* map, const char* key);
void* remove_map_element(string_map* map, const char* key);
char** get_map_keys(string_map* map);
void get_node_keys(map_node* node, char** key_list, int* next_key_index);

unsigned long sdbm_string_hash(const char *key);


#endif
