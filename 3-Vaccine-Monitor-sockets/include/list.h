#include <stdbool.h>
/*ordered list with unique keys
nodes have pointers which point to lower levels
to be used for skip lists*/

typedef struct list_node list_node;

list_node* list_init(void* lower);
void* list_insert_after(char* key, void* lower_item, void* prev_node); /*inserts after node prev and returns the address of the new node, so that it can be used to link nodes between levels*/
void* list_remove(void* l, char* key, void (*destroy_fun)()); /* removes item with given key (if it exists) and returns a pointer to the lower level from which the remove from the next level should start. */

void* list_search(void* l, char* key, void** trace); /*searches list after current node l and returns a pointer to the lower level from which the search to the lower level should start*/
void* list_print(list_node* l); /* prints a list and if returns a pointer to the sentinel of the lower level */
void* list_delete(list_node* l, void (*destroy_fun)()); /* deletes a list and if desttroy_fun!=NULL the lower level. returns a pointer to the sentinel of the lower level */
bool list_iskey(void* node, char* key); /* returns if that key is node's key*/

void* list_getNext(void* node); /* returns next node in list */
void* list_down(void* node); /* returns the lower level of that node */
bool list_isempty(void* node); /* checks if list id empty */