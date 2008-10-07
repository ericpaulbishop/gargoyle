/*  weburl --	A netfilter module to match URLs in HTTP requests 
 *  		This module can match using string match or regular expressions
 *  		Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
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

#if __KERNEL__
	#define malloc(foo)	kmalloc(foo,GFP_ATOMIC)
	#define free(foo)	kfree(foo)
	#define printf(format,args...)	printk(format,##args)

	/* kernel strdup */
	static inline char *kernel_strdup(const char *str);
	static inline char *kernel_strdup(const char *str)
	{
		char *tmp;
		long int s;
		s=strlen(str) + 1;
		tmp = kmalloc(s, GFP_ATOMIC);
		if (tmp)
		{
			memcpy(tmp, str, s);
		}
		return tmp;
	}
	#define strdup kernel_strdup

#endif


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
long_map* initialize_long_map(void);
void* set_long_map_element(long_map* map, unsigned long key, void* value);
void* get_long_map_element(long_map* map, unsigned long key);
void* remove_long_map_element(long_map* map, unsigned long key);
unsigned long* get_sorted_long_map_keys(long_map* map, unsigned long* keys_returned);
void** get_sorted_long_map_values(long_map* map, unsigned long* values_returned);
void** destroy_long_map(long_map* map, int destruction_type);

/* string map functions */
string_map* initialize_string_map(unsigned char store_keys);
void* set_string_map_element(string_map* map, const char* key, void* value);
void* get_string_map_element(string_map* map, const char* key);
void* remove_string_map_element(string_map* map, const char* key);
char** get_string_map_keys(string_map* map); 
void** get_string_map_values(string_map* map, unsigned long* values_returned);
void** destroy_string_map(string_map* map, int destruction_type);

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


/* internal utility structures/ functions */
typedef struct l_node
{
	long_map_node** node_ptr;
	signed char direction;
	struct l_node* previous;
} list_node;

void get_sorted_node_keys(long_map_node* node, unsigned long* key_list, unsigned long* next_key_index, int depth);
void get_sorted_node_values(long_map_node* node, void** value_list, unsigned long* next_value_index, int depth);
signed char rebalance (long_map_node** n, signed char direction, signed char update_op);
void rotate_right (long_map_node** parent);
void rotate_left (long_map_node** parent);

/* internal for string map */
typedef struct 
{
	char* key;
	void* value;
} string_map_key_value;
unsigned long sdbm_string_hash(const char *key);




/***************************************************
 * For testing only
 ***************************************************/
/*
void print_list(list_node *l);

void print_list(list_node *l)
{
	if(l != NULL)
	{
		printf(" list key = %ld, dir=%d, \n", (*(l->node_ptr))->key, l->direction);
		print_list(l->previous);
	}
}
*/
/******************************************************
 * End testing Code
 *******************************************************/




/***************************************************
 * string_map function definitions
 ***************************************************/

string_map* initialize_string_map(unsigned char store_keys)
{
	string_map* map = (string_map*)malloc(sizeof(string_map));
	map->store_keys = store_keys;
	map->lm.root = NULL;
	map->lm.num_elements = 0;
	map->num_elements = map->lm.num_elements;
	
	return map;
}

void* set_string_map_element(string_map* map, const char* key, void* value)
{
	unsigned long hashed_key = sdbm_string_hash(key);
	void* return_value;
	if(map->store_keys)
	{
		string_map_key_value* kv = (string_map_key_value*)malloc(sizeof(string_map_key_value));
		kv->key = strdup(key);
		kv->value = value;
		return_value = set_long_map_element(  &(map->lm), hashed_key, kv);
		if(return_value != NULL)
		{
			string_map_key_value* r = (string_map_key_value*)return_value;
			return_value = r->value;
			free(r->key);
			free(r);
		}
	}
	else
	{
		return_value = set_long_map_element( &(map->lm), hashed_key, value);
	}
	map->num_elements = map->lm.num_elements;
	return return_value;
}
void* get_string_map_element(string_map* map, const char* key)
{
	unsigned long hashed_key = sdbm_string_hash(key);
	void* return_value;
	return_value =  get_long_map_element( &(map->lm), hashed_key);
	if(return_value != NULL && map->store_keys)
	{
		string_map_key_value* r = (string_map_key_value*)return_value;
		return_value = r->value;
	}
	map->num_elements = map->lm.num_elements;
	return return_value;
}
void* remove_string_map_element(string_map* map, const char* key)
{
	unsigned long hashed_key = sdbm_string_hash(key);
	void* return_value;
	return_value =  remove_long_map_element( &(map->lm), hashed_key);
	
	if(return_value != NULL && map->store_keys)
	{
		string_map_key_value* r = (string_map_key_value*)return_value;
		return_value = r->value;
		free(r->key);
		free(r);
	}
	map->num_elements = map->lm.num_elements;
	return return_value;
}

char** get_string_map_keys(string_map* map)
{
	char** str_keys;
	str_keys = (char**)malloc((map->num_elements+1)*sizeof(char*));
	str_keys[0] = NULL;
	if(map->store_keys && map->num_elements > 0)
	{
		unsigned long list_length;
		void** long_values = get_sorted_long_map_values( &(map->lm),  &list_length);
		unsigned long key_index;
		for(key_index = 0; key_index < list_length; key_index++)
		{
			str_keys[key_index] = strdup( ((string_map_key_value*)(long_values[key_index]))->key);
		}
		str_keys[list_length] = NULL;
		free(long_values);
	}
	return str_keys;
}

void** get_string_map_values(string_map* map, unsigned long* values_returned)
{
	void** values;
	unsigned long list_length;
	string_map_key_value** long_values;
	unsigned long value_index;
	if(map->store_keys >0)
	{
		values = (void**)malloc((map->num_elements+1)*sizeof(void*));
		long_values = (string_map_key_value**)get_sorted_long_map_values ( &(map->lm), &list_length );
		for(value_index=0; value_index < list_length; value_index++)
		{
			values[value_index] = (long_values[value_index])->value;
		}
		free(long_values);
	}
	else
	{
		values = get_sorted_long_map_values ( &(map->lm), &list_length );

	}
	return values;
}



void** destroy_string_map(string_map* map, int destruction_type)
{
	void** return_values = NULL;
	char** key_list = get_string_map_keys(map);
	unsigned long key_index = 0;

	if(destruction_type == DESTROY_MAP_RETURN_VALUES)
	{
		return_values = (void**)malloc((1+map->num_elements)*sizeof(void*));
		return_values[ map->num_elements ] = NULL;
	}
	while(map->num_elements > 0)
	{
		void* removed_value = remove_string_map_element(map, key_list[key_index]);
		if(destruction_type == DESTROY_MAP_RETURN_VALUES)
		{
			return_values[key_index] = removed_value;
		}
		if(destruction_type == DESTROY_MAP_FREE_VALUES)
		{
			free(removed_value);
		}

		free(key_list[key_index]);
		key_index++;
	}
	free(key_list);
	free(map);

	return return_values;
}



/***************************************************
 * long_map function definitions
 ***************************************************/

long_map* initialize_long_map(void)
{
	long_map* map = (long_map*)malloc(sizeof(long_map));
	map->root = NULL;
	map->num_elements = 0;

	return map;
}


/* if replacement performed, returns replaced value, otherwise null */
void* set_long_map_element(long_map* map, unsigned long key, void* value)
{
	list_node* parent_list = NULL;
	void* old_value = NULL;
	int old_value_found = 0;

	long_map_node* parent_node;
	long_map_node* next_node;
	list_node* next_parent;
	list_node* previous_parent;
	signed char new_balance;


	long_map_node* new_node = (long_map_node*)malloc(sizeof(long_map_node));
	new_node->value = value;
	new_node->key = key;
	new_node->left = NULL;
	new_node->right = NULL;
	new_node->balance = 0;

	

	if(map->root == NULL)
	{
		map->root = new_node;	
	}
	else
	{
		parent_node = map->root;
			
		next_parent = (list_node*)malloc(sizeof(list_node));
		next_parent->node_ptr =  &(map->root);
		next_parent->previous = parent_list;
		parent_list = next_parent;	
			
		while( key != parent_node->key && (next_node = (key < parent_node->key ? parent_node->left : parent_node->right) )  != NULL)
		{
			next_parent = (list_node*)malloc(sizeof(list_node));
			next_parent->node_ptr = key < parent_node->key ? &(parent_node->left) : &(parent_node->right);
			next_parent->previous = parent_list;
			next_parent->previous->direction = key < parent_node->key ? -1 : 1;
			parent_list = next_parent;

			parent_node = next_node;
		}
		
		
		if(key == parent_node->key)
		{
			old_value = parent_node->value;
			old_value_found = 1;
			parent_node->value = value;
			free(new_node);
			/* we merely replaced a node, no need to rebalance */
		}
		else
		{	
			if(key < parent_node->key)
			{
				parent_node->left = (void*)new_node;
				parent_list->direction = -1;
			}
			else
			{
				parent_node->right = (void*)new_node;
				parent_list->direction = 1;
			}
			
			
			/* we inserted a node, rebalance */
			previous_parent = parent_list;
			new_balance  = 1; /* initial value is not used, but must not be 0 for initial loop condition */
			
			
			while(previous_parent != NULL && new_balance != 0)
			{
				new_balance = rebalance(previous_parent->node_ptr, previous_parent->direction, 1);
				previous_parent = previous_parent->previous;
			}
		}
	}

	while(parent_list != NULL)
	{
		previous_parent = parent_list;
		parent_list = previous_parent->previous;
		free(previous_parent);
	}

	if(old_value_found == 0)
	{
		map->num_elements = map->num_elements + 1;
	}

	return old_value;
}


void* get_long_map_element(long_map* map, unsigned long key)
{
	void* value = NULL;

	if(map->root != NULL)
	{
		long_map_node* parent_node = map->root;
		long_map_node* next_node;	
		while( key != parent_node->key && (next_node = (long_map_node *)(key < parent_node->key ? parent_node->left : parent_node->right))  != NULL)
		{
			parent_node = next_node;
		}
		if(parent_node->key == key)
		{
			value = parent_node->value;
		}
	}
	return value;
}

void* remove_long_map_element(long_map* map, unsigned long key)
{

	void* value = NULL;
	
	long_map_node* root_node = map->root;	
	list_node* parent_list = NULL;


	long_map_node* remove_parent;
	long_map_node* remove_node;
	long_map_node* next_node;

	long_map_node* replacement;
	long_map_node* replacement_parent;
	long_map_node* replacement_next;

	list_node* next_parent;
	list_node* previous_parent;
	list_node* replacement_list_node;


	signed char new_balance;



	if(root_node != NULL)
	{
		remove_parent = root_node;
		remove_node = key < remove_parent->key ? remove_parent->left : remove_parent->right;
		
		if(remove_node != NULL && key != remove_parent->key)
		{
			next_parent = (list_node*)malloc(sizeof(list_node));
			next_parent->node_ptr =  &(map->root);
			next_parent->previous = parent_list;
			parent_list = next_parent;	
			while( key != remove_node->key && (next_node = (key < remove_node->key ? remove_node->left : remove_node->right))  != NULL)
			{
				next_parent = (list_node*)malloc(sizeof(list_node));
				next_parent->node_ptr = key < remove_parent->key ? &(remove_parent->left) : &(remove_parent->right);
				next_parent->previous = parent_list;
				next_parent->previous->direction = key < remove_parent->key ? -1 : 1; 
				parent_list = next_parent;
				
				
				remove_parent = remove_node;
				remove_node = next_node;
			}
			parent_list->direction = key < remove_parent-> key ? -1 : 1;
		}
		else
		{
			remove_node = remove_parent;
		}


		if(key == remove_node->key)
		{
			
			/* find replacement for node we are deleting */
			if( remove_node->right == NULL )
			{
				replacement = remove_node->left;
			}
			else if( remove_node->right->left == NULL)
			{

				replacement = remove_node->right;
				replacement->left = remove_node->left;
				replacement->balance = remove_node->balance;

				/* put pointer to replacement node into list for balance update */
				replacement_list_node = (list_node*)malloc(sizeof(list_node));;
				replacement_list_node->previous = parent_list;
				replacement_list_node->direction = 1; /* replacement is from right */
				if(remove_node == remove_parent) /* special case for root node */
				{
					replacement_list_node->node_ptr = &(map->root);
				}
				else
				{
					replacement_list_node->node_ptr = key < remove_parent-> key ? &(remove_parent->left) : &(remove_parent->right);
				}
				parent_list = replacement_list_node;

			}
			else
			{
				/* put pointer to replacement node into list for balance update */
				replacement_list_node = (list_node*)malloc(sizeof(list_node));
				replacement_list_node->previous = parent_list;
				replacement_list_node->direction = 1; /* we always look for replacement on right */
				if(remove_node == remove_parent) /* special case for root node */
				{
					replacement_list_node->node_ptr = &(map->root);
				}
				else
				{
					replacement_list_node->node_ptr = key < remove_parent-> key ? &(remove_parent->left) : &(remove_parent->right);
				}

				parent_list = replacement_list_node;
				

				/*
				 * put pointer to replacement node->right into list for balance update
				 * this node will have to be updated with the proper pointer
				 * after we have identified the replacement
				 */
				replacement_list_node = (list_node*)malloc(sizeof(list_node));
				replacement_list_node->previous = parent_list;
				replacement_list_node->direction = -1; /* we always look for replacement to left of this node */
				parent_list = replacement_list_node;
				
				/* find smallest node on right (large) side of tree */
				replacement_parent = remove_node->right;
				replacement = replacement_parent->left;
				
				while((replacement_next = replacement->left)  != NULL)
				{
					next_parent = (list_node*)malloc(sizeof(list_node));
					next_parent->node_ptr = &(replacement_parent->left);
					next_parent->previous = parent_list;
					next_parent->direction = -1; /* we always go left */
					parent_list = next_parent;

					replacement_parent = replacement;
					replacement = replacement_next;

				}

				replacement_parent->left = replacement->right;
				
				replacement->left = remove_node->left;
				replacement->right = remove_node->right;
				replacement->balance = remove_node->balance;
				replacement_list_node->node_ptr = &(replacement->right);
			}
			
			/* insert replacement at proper location in tree */
			if(remove_node == remove_parent)
			{
				map->root = replacement;
			}
			else
			{
				remove_parent->left = remove_node == remove_parent->left ? replacement : remove_parent->left;
				remove_parent->right = remove_node == remove_parent->right ? replacement : remove_parent->right;
			}
		

			/* rebalance tree */
			previous_parent = parent_list;
			new_balance = 0;
			while(previous_parent != NULL && new_balance == 0)
			{
				new_balance = rebalance(previous_parent->node_ptr, previous_parent->direction, -1);
				previous_parent = previous_parent->previous;
			}
			
			


			/* 
			 * since we found a value to remove, decrease number of elements in map
			 *  set return value to the deleted node's value and free the node
			 */
			map->num_elements = map->num_elements - 1;
			value = remove_node->value;
			free(remove_node);
		}
	}


	while(parent_list != NULL)
	{
		previous_parent = parent_list;
		parent_list = previous_parent->previous;
		free(previous_parent);
	}
	
	return value;
}


/* note: returned keys are dynamically allocated, you need to free them! */
unsigned long* get_sorted_long_map_keys(long_map* map, unsigned long* keys_returned)
{
	unsigned long* key_list = (unsigned long*)malloc((map->num_elements)*sizeof(unsigned long));

	unsigned long next_key_index = 0;
	get_sorted_node_keys(map->root, key_list, &next_key_index, 0);
	
	*keys_returned = map->num_elements;

	return key_list;
}


void** get_sorted_long_map_values(long_map* map, unsigned long* values_returned)
{
	void** value_list = (void**)malloc((map->num_elements+1)*sizeof(void*));

	unsigned long next_value_index = 0;
	get_sorted_node_values(map->root, value_list, &next_value_index, 0);
	value_list[map->num_elements] = NULL; /* since we're dealing with pointers make list null terminated */

	*values_returned = map->num_elements;
	return value_list;

}


void** destroy_long_map(long_map* map, int destruction_type)
{
	void** return_values = NULL;
	unsigned long num_keys;
	unsigned long* key_list = get_sorted_long_map_keys(map, &num_keys);
	unsigned long key_index = 0;


	if(destruction_type == DESTROY_MAP_RETURN_VALUES)
	{
		return_values = (void**)malloc((map->num_elements+1)*sizeof(void*));
		return_values[map->num_elements] = NULL;
	}
	while(map->num_elements > 0)
	{
		void* removed_value = remove_long_map_element(map, key_list[key_index]);
		if(destruction_type == DESTROY_MAP_RETURN_VALUES)
		{
			return_values[key_index] = removed_value;
		}
		if(destruction_type == DESTROY_MAP_FREE_VALUES)
		{
			free(removed_value);
		}
		key_index++;
	}
	free(key_list);
	free(map);

	return return_values;
}


/***************************************************
 * internal utility function definitions
 ***************************************************/

void get_sorted_node_keys(long_map_node* node, unsigned long* key_list, unsigned long* next_key_index, int depth)
{
	if(node != NULL)
	{
		get_sorted_node_keys(node->left, key_list, next_key_index, depth+1);
		
		key_list[ *next_key_index ] = node->key;
		(*next_key_index)++;

		get_sorted_node_keys(node->right, key_list, next_key_index, depth+1);
	}
}

void get_sorted_node_values(long_map_node* node, void** value_list, unsigned long* next_value_index, int depth)
{
	if(node != NULL)
	{
		get_sorted_node_values(node->left, value_list, next_value_index, depth+1);
		
		value_list[ *next_value_index ] = node->value;
		(*next_value_index)++;

		get_sorted_node_values(node->right, value_list, next_value_index, depth+1);
	}
}



/*
 * direction = -1 indicates left subtree updated, direction = 1 for right subtree
 * update_op = -1 indicates delete node, update_op = 1 for insert node
 */
signed char rebalance (long_map_node** n, signed char direction, signed char update_op)
{
	/*
	printf( "original: key = %ld, balance = %d, update_op=%d, direction=%d\n", (*n)->key, (*n)->balance, update_op, direction); 
	*/

	(*n)->balance = (*n)->balance + (update_op*direction);
	
	if( (*n)->balance <  -1)
	{
		if((*n)->left->balance < 0)
		{
			rotate_right(n);
			(*n)->right->balance = 0;
			(*n)->balance = 0;
		}
		else if((*n)->left->balance == 0)
		{
			rotate_right(n);
			(*n)->right->balance = -1;
			(*n)->balance = 1;
		}
		else if((*n)->left->balance > 0)
		{
			rotate_left( &((*n)->left) );
			rotate_right(n);
			/*
			if( (*n)->balance < 0 )
			{
				(*n)->left->balance = 0;
				(*n)->right->balance = 1;
			}
			else if( (*n)->balance == 0 )
			{
				(*n)->left->balance = 0;
				(*n)->right->balance = 0;
			}
			else if( (*n)->balance > 0 )
			{
				(*n)->left->balance = -1;
				(*n)->right->balance = 0;
			}
			*/
			(*n)->left->balance  = (*n)->balance > 0 ? -1 : 0;
			(*n)->right->balance = (*n)->balance < 0 ?  1 : 0;
			(*n)->balance = 0;
		}
	}
	if( (*n)->balance >  1)
	{
		if((*n)->right->balance > 0)
		{
			rotate_left(n);
			(*n)->left->balance = 0;
			(*n)->balance = 0;
		}
		else if ((*n)->right->balance == 0)
		{
			rotate_left(n);
			(*n)->left->balance = 1;
			(*n)->balance = -1;
		}
		else if((*n)->right->balance < 0)
		{
			rotate_right( &((*n)->right) );
			rotate_left(n);
			/*
			if( (*n)->balance < 0 )
			{
				(*n)->left->balance = 0;
				(*n)->right->balance = 1;
			}
			else if( (*n)->balance == 0 )
			{
				(*n)->left->balance = 0;
				(*n)->right->balance = 0;
			}
			else if( (*n)->balance > 0 )
			{
				(*n)->left->balance = -1;
				(*n)->right->balance = 0;
			}
			*/
			(*n)->left->balance   = (*n)->balance > 0 ? -1 : 0;
			(*n)->right->balance  = (*n)->balance < 0 ?  1 : 0;
			(*n)->balance = 0;
		}
	}

	/*
	printf( "key = %ld, balance = %d\n", (*n)->key, (*n)->balance);
	*/

	return (*n)->balance;
}


void rotate_right (long_map_node** parent)
{
	long_map_node* old_parent = *parent;
	long_map_node* pivot = old_parent->left;
	old_parent->left = pivot->right;
	pivot->right  = old_parent;
	
	*parent = pivot;
}

void rotate_left (long_map_node** parent)
{
	long_map_node* old_parent = *parent;
	long_map_node* pivot = old_parent->right;
	old_parent->right = pivot->left;
	pivot->left  = old_parent;
	
	*parent = pivot;
}



/***************************************************************************
 * This algorithm was created for the sdbm database library (a public-domain 
 * reimplementation of ndbm) and seems to work relatively well in 
 * scrambling bits
 *
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
	return hashed_key;
}

