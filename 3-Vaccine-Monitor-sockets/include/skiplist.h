#include <stdbool.h>

typedef struct skip_list skip_list;

skip_list* skiplist_init(int max_levels, void (*destroy_fun)()); /* initialize a skip list with a name, max levels and a function (or NULL) to destroy the items below level 0 (NULL=don't destroy items)*/
void skiplist_insert(skip_list* sl, char* key, void* item); /* insert to skip list item with key "key" */
void* skiplist_search(skip_list* sl, char* key); /* search and return item with key "key" */

void skiplist_remove(skip_list* sl, char* key); /* remove item with key "key" */
void skiplist_delete(void* sl);

void skiplist_print(skip_list* sl);

void* skiplist_getNextEntry(skip_list* sl); /* first call with a pointer to a skip list and then call with NULL, to get one by one the items in the skip list */