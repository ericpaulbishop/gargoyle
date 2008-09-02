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

#include "erics_tools.h"



string_map* initialize_map(unsigned char store_keys)
{
	string_map* map = (string_map*)malloc(sizeof(string_map));
	map->root = NULL;
	map->num_elements = 0;

	map->store_keys = store_keys;
	return map;
}



///note: returned keys are dynamically allocated, you need to free them!
char** get_map_keys(string_map* map)
{
	char** key_list = (char**)malloc((1+map->num_elements)*sizeof(char*));

	int next_key_index = 0;
	if(map->store_keys > 0)
	{
		get_node_keys(map->root, key_list, &next_key_index);
	}
	key_list[next_key_index] = NULL;
	return key_list;
}

void get_node_keys(map_node* node, char** key_list, int* next_key_index)
{
	if(node != NULL)
	{
		get_node_keys(node->left, key_list, next_key_index);
		
		key_list[ *next_key_index ] = strdup(node->key_str);
		(*next_key_index)++;
		
		get_node_keys(node->right, key_list, next_key_index);
	}
}

//if replacement performed, returns replaced value, otherwise null
void* set_map_element(string_map* map, const char* key, char* value)
{
	unsigned long hashed_key = sdbm_string_hash(key);


	map_node* new_node = (map_node*)malloc(sizeof(map_node));
	new_node->value = value;
	new_node->hashed_key = hashed_key;
	new_node->left = NULL;
	new_node->right = NULL;
	if(map->store_keys > 0)
	{
		new_node->key_str = strdup(key);
	}
	else 
	{
		new_node->key_str = NULL;
	}

	void* old_value = NULL;
	if(map->root == NULL)
	{
		map->root = new_node;	
	}
	else
	{
		map_node* parent_node = map->root;
		map_node* next_node;
		while( hashed_key != parent_node->hashed_key && (next_node = (map_node *)(hashed_key < parent_node->hashed_key ? parent_node->left : parent_node->right))  != NULL)
		{
			parent_node = next_node;
		}
		if(hashed_key == parent_node->hashed_key)
		{
			old_value = parent_node->value;
			parent_node->value = value;
			if(map->store_keys > 0)
			{	
				free(parent_node->key_str);
				parent_node->key_str = new_node->key_str;
			}
			free(new_node);
		}
		else if(hashed_key < parent_node->hashed_key)
		{
			parent_node->left = (void*)new_node;
		}
		else
		{
			parent_node->right = (void*)new_node;
		}
	}

	if(old_value == NULL)
	{
		map->num_elements = map->num_elements + 1;
	}

	return old_value;
}


void* get_map_element(string_map* map, const char* key)
{
	unsigned long hashed_key = sdbm_string_hash(key);
	void* value = NULL;

	if(map->root != NULL)
	{
		map_node* parent_node = map->root;
		map_node* next_node;	
		while( hashed_key != parent_node->hashed_key && (next_node = (map_node *)(hashed_key < parent_node->hashed_key ? parent_node->left : parent_node->right))  != NULL)
		{
			parent_node = next_node;
		}
		if(parent_node->hashed_key == hashed_key)
		{
			value = parent_node->value;
		}
	}
	return value;
}

void* remove_map_element(string_map* map, const char* key)
{
	unsigned long hashed_key = sdbm_string_hash(key);
	void* value = NULL;
	int value_found = 0; //necessary to tell if we find node, in case we find node, but value is NULL

	map_node* root_node = map->root;	
	if(root_node != NULL)
	{
		map_node* remove_parent = root_node;
		map_node* remove_node = (map_node *)(hashed_key < remove_parent->hashed_key ? remove_parent->left : remove_parent->right);
		map_node* next_node;
		if(remove_node != NULL && hashed_key != remove_parent->hashed_key)
		{
			while( hashed_key != remove_node->hashed_key && (next_node = (map_node *)(hashed_key < remove_node->hashed_key ? remove_node->left : remove_node->right))  != NULL)
			{
				remove_parent = remove_node;
				remove_node = next_node;
			}
		}
		else
		{
			remove_node = remove_parent;
		}
		if(hashed_key == remove_node->hashed_key)
		{
			map_node* replacement;
			if(remove_node->left == NULL && remove_node->right == NULL)
			{
				replacement = NULL;
			}
			else 
			{
				if(remove_node->left != NULL && remove_node->right != NULL)
				{
					//find smallest node on right (large) side of tree
					map_node* replacement_parent = (map_node*)remove_node->right;
					replacement = (map_node*)replacement_parent->left;
					if(replacement != NULL)
					{
						map_node* replacement_next;
						while((replacement_next = (map_node *)replacement->left)  != NULL)
						{
							replacement_parent = replacement;
							replacement = replacement_next;
						}
						replacement_parent->left = replacement->right;
					}
					else
					{
						replacement = replacement_parent;
						remove_node->right = replacement->right;
					}
					replacement->left = remove_node->left;
					replacement->right = remove_node->right;
				}
				else
				{
					replacement = (map_node*)(remove_node->left == NULL ? remove_node->right : remove_node->left);
				}
			}
			if(remove_node == remove_parent)
			{
				map->root = replacement;
			}
			else
			{
				remove_parent->left = remove_node == remove_parent->left ? replacement : remove_parent->left;
				remove_parent->right = remove_node == remove_parent->right ? replacement : remove_parent->right;
			}
			value_found = 1;
			value = remove_node->value;
			if(map->store_keys)
			{
				free(remove_node->key_str);
			}
			free(remove_node);
		}
	}

	if(value_found > 0)
	{
		map->num_elements = map->num_elements - 1;
	}

	return value;
}


/***************************************************************************
 * This algorithm was created for the sdbm database library (a public-domain 
 * reimplementation of ndbm) and seems to work relatively well in 
 * scrambling bits
 *
 * Because this binary tree implementation does not include any code to keep
 * the tree balanced the original hash algorithm has been modified so that if the 
 * keys are entered in alphabetical (or reverse alphabetical) order, the
 * tree will not necessarily be lopsided. 
 *
 * This code was derived from code found at:
 * http://www.cse.yorku.ca/~oz/hash.html
 ***************************************************************************/
unsigned long sdbm_string_hash(const char *key)
{
	unsigned long hashed_key = 0;

	int index = 0;
	unsigned int nextch;
	while(key[index] != '\0')
	{
		nextch = key[index];
		hashed_key = nextch + (hashed_key << 6) + (hashed_key << 16) - hashed_key;
		index++;
	}
	while(index >= 0)
	{
		nextch = key[index];
		hashed_key = nextch + (hashed_key << 6) + (hashed_key << 16) - hashed_key;
		index--;
	}
	
	return hashed_key;
}

