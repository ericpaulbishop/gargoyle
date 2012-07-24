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

#include "erics_tools.h"
#define malloc safe_malloc
#define strdup safe_strdup

#define FIRST_IS_BIGGER(KEY1,KEY2,KEYLEN1,KEYLEN2) ( KEYLEN1 == KEYLEN2 ? (memcmp(KEY1, KEY2, KEYLEN1) > 0) :  (KEYLEN1 > KEYLEN2) ) 
#define KEYS_MATCH(KEY1,KEY2,KEYLEN1,KEYLEN2) ( KEYLEN1 == KEYLEN2 ? (memcmp(KEY1, KEY2, KEYLEN1) == 0) : 0 )



/* internal utility structures/ functions */
typedef struct stack_node_struct
{
	raw_map_node** node_ptr;
	signed char direction;
	struct stack_node_struct* previous;
} stack_node;

static void apply_to_every_raw_map_node(raw_map_node* node, void (*apply_func)(void* key, size_t key_length, void* value));
static void get_sorted_node_keys(raw_map_node* node, void** key_list, size_t* key_length_list, unsigned long* next_key_index, int depth);
static void get_sorted_node_values(raw_map_node* node, void** value_list, unsigned long* next_value_index, int depth);




static signed char rebalance (raw_map_node** n, signed char direction, signed char update_op);
static void rotate_right (raw_map_node** parent);
static void rotate_left (raw_map_node** parent);


/***************************************************
 * For testing only
 ***************************************************/
/*
void print_list(stack_node *l);

void print_list(stack_node *l)
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
 * raw_map function definitions
 ***************************************************/

raw_map* initialize_raw_map(void)
{
	raw_map* map = (raw_map*)malloc(sizeof(raw_map));
	map->root = NULL;
	map->num_elements = 0;

	return map;
}

void* get_raw_map_element(raw_map* map, void* key, size_t key_length)
{
	void* value = NULL;

	if(map->root != NULL)
	{
		raw_map_node* parent_node = map->root;
		raw_map_node* next_node;	
		while( KEYS_MATCH(key,parent_node->key,key_length,parent_node->key_length) == 0 && (next_node = (raw_map_node *)(  FIRST_IS_BIGGER(key,parent_node->key,key_length,parent_node->key_length) ? parent_node->right : parent_node->left))  != NULL)
		{
			parent_node = next_node;
		}
		if( KEYS_MATCH(key,parent_node->key,key_length,parent_node->key_length) )
		{
			value = parent_node->value;
		}
	}
	return value;
}

void* get_smallest_raw_map_element(raw_map* map, void** smallest_key, size_t* smallest_key_length)
{
	void* value = NULL;
	if(map->root != NULL)
	{
		raw_map_node* next_node = map->root;	
		while( next_node->left != NULL)
		{
			next_node = next_node->left;
		}
		value = next_node->value;
		*smallest_key_length = next_node->key_length;
		*smallest_key = (void*)malloc(*smallest_key_length);
		memcpy(*smallest_key, next_node->key, *smallest_key_length);
	}
	return value;
}

void* get_largest_raw_map_element(raw_map* map, void** largest_key, size_t* largest_key_length)
{
	void* value = NULL;
	if(map->root != NULL)
	{
		raw_map_node* next_node = map->root;	
		while( next_node->right != NULL)
		{
			next_node = next_node->right;
		}
		value = next_node->value;
		*largest_key_length = next_node->key_length;
		*largest_key = (void*)malloc(*largest_key_length);
		memcpy(*largest_key, next_node->key, *largest_key_length);
	}
	return value;
}

void* remove_smallest_raw_map_element(raw_map* map, void** smallest_key, size_t* smallest_key_length)
{
	get_smallest_raw_map_element(map, smallest_key, smallest_key_length);
	void* ret = remove_raw_map_element(map, *smallest_key, *smallest_key_length);
	return ret;
}

void* remove_largest_raw_map_element(raw_map* map, void** largest_key, size_t* largest_key_length)
{
	get_largest_raw_map_element(map, largest_key, largest_key_length);
	return remove_raw_map_element(map, *largest_key, *largest_key_length);
}


/* if replacement performed, returns replaced value, otherwise null */
void* set_raw_map_element(raw_map* map, void* key, size_t key_length, void* value)
{
	stack_node* parent_list = NULL;
	void* old_value = NULL;
	int old_value_found = 0;

	raw_map_node* parent_node;
	raw_map_node* next_node;
	stack_node* next_parent;
	stack_node* previous_parent;
	signed char new_balance;


	raw_map_node* new_node = (raw_map_node*)malloc(sizeof(raw_map_node));
	new_node->value = value;
	new_node->key_length = key_length;
	new_node->key = NULL; /*don't allocate memory for key until we are sure we don't already have a copy of this key */
	new_node->left = NULL;
	new_node->right = NULL;
	new_node->balance = 0;

	if(map->root == NULL)
	{
		void* key_copy = (void*)malloc(key_length);
		memcpy(key_copy, key, key_length);
		new_node->key = key_copy;

		map->root = new_node;	
	}
	else
	{

		parent_node = map->root;
			
		next_parent = (stack_node*)malloc(sizeof(stack_node));
		next_parent->node_ptr =  &(map->root);
		next_parent->previous = parent_list;
		parent_list = next_parent;
		while( KEYS_MATCH(key,parent_node->key,key_length,parent_node->key_length) == 0 && (next_node = (raw_map_node *)(  FIRST_IS_BIGGER(key,parent_node->key,key_length,parent_node->key_length) ? parent_node->right : parent_node->left))  != NULL)
		{
			next_parent = (stack_node*)malloc(sizeof(stack_node));
			next_parent->node_ptr = FIRST_IS_BIGGER(key,parent_node->key,key_length,parent_node->key_length) ? &(parent_node->right) : &(parent_node->left) ;
			next_parent->previous = parent_list;
			next_parent->previous->direction = FIRST_IS_BIGGER(key,parent_node->key,key_length,parent_node->key_length) ? 1 : -1;
			parent_list = next_parent;

			parent_node = next_node;
		}
			

		if(KEYS_MATCH(key,parent_node->key,key_length,parent_node->key_length))
		{
			old_value = parent_node->value;
			old_value_found = 1;
			parent_node->value = value;
			free(new_node);
			/* we merely replaced a node, no need to rebalance */
		}
		else
		{
			void* key_copy = (void*)malloc(key_length);
			memcpy(key_copy, key, key_length);
			new_node->key = key_copy;
			
			/*
			new_node->key = (void*)malloc(key_length);
			memcpy(new_node->key, key, key_length);
			*/
			if(FIRST_IS_BIGGER(key,parent_node->key,key_length,parent_node->key_length) )
			{
				parent_node->right = (void*)new_node;
				parent_list->direction = 1;
			}
			else
			{
				parent_node->left = (void*)new_node;
				parent_list->direction = -1;
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


void* remove_raw_map_element(raw_map* map, void* key, size_t key_length)
{

	void* value = NULL;
	
	raw_map_node* root_node = map->root;	
	stack_node* parent_list = NULL;


	raw_map_node* remove_parent;
	raw_map_node* remove_node;
	raw_map_node* next_node;

	raw_map_node* replacement;
	raw_map_node* replacement_parent;
	raw_map_node* replacement_next;

	stack_node* next_parent;
	stack_node* previous_parent;
	stack_node* replacement_stack_node;


	signed char new_balance;


	if(root_node != NULL)
	{
		remove_parent = root_node;
		remove_node = FIRST_IS_BIGGER(key,remove_parent->key,key_length,remove_parent->key_length) ? remove_parent->right : remove_parent->left;	
		
		if(remove_node != NULL && KEYS_MATCH(key,remove_parent->key,key_length,remove_parent->key_length) == 0)
		{
			next_parent = (stack_node*)malloc(sizeof(stack_node));
			next_parent->node_ptr =  &(map->root);
			next_parent->previous = parent_list;
			parent_list = next_parent;	
			while( KEYS_MATCH(key,remove_node->key,key_length,remove_node->key_length) == 0 && (next_node = (FIRST_IS_BIGGER(key,remove_node->key,key_length,remove_node->key_length) ? remove_node->right : remove_node->left ))  != NULL)
			{


				next_parent = (stack_node*)malloc(sizeof(stack_node));
				next_parent->node_ptr = FIRST_IS_BIGGER(key,remove_parent->key,key_length,remove_parent->key_length) ? &(remove_parent->right) : &(remove_parent->left);
				next_parent->previous = parent_list;
				next_parent->previous->direction = FIRST_IS_BIGGER(key,remove_parent->key,key_length,remove_parent->key_length) ? 1 : -1;				
				parent_list = next_parent;
				
				remove_parent = remove_node;
				remove_node = next_node;
			}
			parent_list->direction = FIRST_IS_BIGGER(key,remove_parent->key,key_length,remove_parent->key_length) ? 1 : -1;

		}
		else
		{
			remove_node = remove_parent;
		}


		if(KEYS_MATCH(key,remove_node->key,key_length,remove_node->key_length) )
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
				replacement_stack_node = (stack_node*)malloc(sizeof(stack_node));;
				replacement_stack_node->previous = parent_list;
				replacement_stack_node->direction = 1; /* replacement is from right */
				if(remove_node == remove_parent) /* special case for root node */
				{
					replacement_stack_node->node_ptr = &(map->root);
				}
				else
				{
					replacement_stack_node->node_ptr =  FIRST_IS_BIGGER(key,remove_parent->key,key_length,remove_parent->key_length) ? &(remove_parent->right) : &(remove_parent->left);
				}
				parent_list = replacement_stack_node;
			}
			else
			{
				/* put pointer to replacement node into list for balance update */
				replacement_stack_node = (stack_node*)malloc(sizeof(stack_node));
				replacement_stack_node->previous = parent_list;
				replacement_stack_node->direction = 1; /* we always look for replacement on right */
				if(remove_node == remove_parent) /* special case for root node */
				{
					replacement_stack_node->node_ptr = &(map->root);
				}
				else
				{
					replacement_stack_node->node_ptr = FIRST_IS_BIGGER(key,remove_parent->key,key_length,remove_parent->key_length) ? &(remove_parent->right) : &(remove_parent->left);
				}

				parent_list = replacement_stack_node;
				

				/*
				 * put pointer to replacement node->right into list for balance update
				 * this node will have to be updated with the proper pointer
				 * after we have identified the replacement
				 */
				replacement_stack_node = (stack_node*)malloc(sizeof(stack_node));
				replacement_stack_node->previous = parent_list;
				replacement_stack_node->direction = -1; /* we always look for replacement to left of this node */
				parent_list = replacement_stack_node;
				
				/* find smallest node on right (large) side of tree */
				replacement_parent = remove_node->right;
				replacement = replacement_parent->left;
				
				while((replacement_next = replacement->left)  != NULL)
				{
					next_parent = (stack_node*)malloc(sizeof(stack_node));
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
				replacement_stack_node->node_ptr = &(replacement->right);
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
			free(remove_node->key);
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


/* note: returned keys and keylengths are dynamically allocated, you need to free them! */
void** get_sorted_raw_map_keys(raw_map* map, size_t** key_lengths, unsigned long* num_keys_returned)
{
	
	unsigned long next_key_index = 0;
	void** key_list = (void**)malloc((map->num_elements)*sizeof(void*));
	*key_lengths = (size_t*)malloc((map->num_elements)*sizeof(size_t));


	get_sorted_node_keys(map->root, key_list, *key_lengths, &next_key_index, 0);
	
	*num_keys_returned = map->num_elements;

	return key_list;
}


void** get_sorted_raw_map_values(raw_map* map, unsigned long* num_values_returned)
{
	void** value_list = (void**)malloc((map->num_elements+1)*sizeof(void*));

	unsigned long next_value_index = 0;
	get_sorted_node_values(map->root, value_list, &next_value_index, 0);
	value_list[map->num_elements] = NULL; /* since we're dealing with pointers make list null terminated */

	*num_values_returned = map->num_elements;
	return value_list;

}


void** destroy_raw_map(raw_map* map, int destruction_type, unsigned long* num_destroyed)
{
	void** return_values = NULL;
	unsigned long return_index = 0;

	*num_destroyed = 0;


	

	if(destruction_type == DESTROY_MODE_RETURN_VALUES)
	{
		return_values = (void**)malloc((map->num_elements+1)*sizeof(void*));
		return_values[map->num_elements] = NULL;
	}
	while(map->num_elements > 0)
	{
		void* smallest_key = NULL;
		size_t smallest_length;


		void* removed_value = remove_smallest_raw_map_element(map, &smallest_key, &smallest_length);

		

		free(smallest_key);

		if(destruction_type == DESTROY_MODE_RETURN_VALUES)
		{
			return_values[return_index] = removed_value;
		}
		if(destruction_type == DESTROY_MODE_FREE_VALUES)
		{
			free(removed_value);
		}
		return_index++;
		*num_destroyed = *num_destroyed + 1;
	}
	free(map);

	return return_values;
}

void apply_to_every_raw_map_value(raw_map* map, void (*apply_func)(void* key, size_t key_length, void* value))
{
	apply_to_every_raw_map_node(map->root, apply_func);
}

/***************************************************
 * internal utility function definitions
 ***************************************************/
static void apply_to_every_raw_map_node(raw_map_node* node, void (*apply_func)(void* key, size_t key_length, void* value))
{
	if(node != NULL)
	{
		apply_to_every_raw_map_node(node->left,  apply_func);
		
		apply_func(node->key, node->key_length, node->value);

		apply_to_every_raw_map_node(node->right, apply_func);
	}
}

static void get_sorted_node_keys(raw_map_node* node, void** key_list, size_t* key_length_list, unsigned long* next_key_index, int depth)
{
	if(node != NULL)
	{
		get_sorted_node_keys(node->left, key_list, key_length_list, next_key_index, depth+1);
		
		key_list[ *next_key_index ] = (void*)malloc(node->key_length);
		memcpy(key_list[ *next_key_index ],    node->key, node->key_length);
		key_length_list [ *next_key_index] = node->key_length;
		(*next_key_index)++;

		get_sorted_node_keys(node->right, key_list, key_length_list, next_key_index, depth+1);
	}
}

static void get_sorted_node_values(raw_map_node* node, void** value_list, unsigned long* next_value_index, int depth)
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
static signed char rebalance (raw_map_node** n, signed char direction, signed char update_op)
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


static void rotate_right (raw_map_node** parent)
{
	raw_map_node* old_parent = *parent;
	raw_map_node* pivot = old_parent->left;
	old_parent->left = pivot->right;
	pivot->right  = old_parent;
	
	*parent = pivot;
}

static void rotate_left (raw_map_node** parent)
{
	raw_map_node* old_parent = *parent;
	raw_map_node* pivot = old_parent->right;
	old_parent->right = pivot->left;
	pivot->left  = old_parent;
	
	*parent = pivot;
}


