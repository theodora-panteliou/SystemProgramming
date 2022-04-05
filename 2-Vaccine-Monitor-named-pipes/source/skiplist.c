#include "skiplist.h"
#include <stdio.h>
#include "list.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

struct skip_list
{
    void* start; /*this is the highest head node of all levels-lists*/
    int num_levels; /*actual number of levels in the skip list*/
    int max_levels; /*upper bound of levels*/
    void (*destroy)(void *); /*function to destroy the information level*/
};

int random_level(skip_list* sl){
    int i = 0;
    while (rand()%100 >= 50 && i<sl->max_levels){ /*flip a coin - 50/50 chances that the key appears to a higher level*/
        i++;
    }
    return i;
}

/*level 0 of skip list is an ordered simply linked list*/
skip_list* skiplist_init(int levels, void (*destroy_fun)(void *)){
    srand(time(NULL));

    skip_list* new_skip_list = malloc(sizeof(skip_list)); /*allocate memory*/

    /*initialize an empty list for level 0*/
    new_skip_list->start = list_init(NULL); /*make start be a list that has no lower level*/

    new_skip_list->num_levels = 1; /*at first there is only level 0 list*/
    new_skip_list->max_levels = levels;
    new_skip_list->destroy = destroy_fun;
    return new_skip_list;
}

void skiplist_insert(skip_list* sl, char* key, void* item){
    /* find the level/height of the new node*/
    int height = random_level(sl);
    if (height + 1 > sl->num_levels){ /*if the height is <k> then the number of levels that the new node will appear to is <k+1>*/
        for (int i=0; i < height-sl->num_levels + 1; i++){ /*make new levels until we reach height*/
            sl->start = list_init(sl->start); /*make a new list for each level above the first list. Each new list has as lower list the previous list*/
        }
        sl->num_levels = height + 1;  
    }

    /*search and mark the nodes after which we should insert the new node */
    void* level; /*keeps the current node for the search*/
    void** trace = malloc(sizeof(void *)*sl->num_levels); /*trace is the node after which we should insert the key in each list*/

    level = sl->start; /*start from the highest list*/
    for (int i=0; i < sl->num_levels; i++){ 
        trace[i] = NULL; /*initalize trace*/
    }
    for (int i = sl->num_levels; i >=1 ; i--){ /*high to low*/
        level = list_search(level, key, &trace[i-1]); /*in: higher level (i), out: lower level (i-1). trace is the node with key smaller than input key. It is (i-1) bc we need num_levels nodes - one for each list*/
    }
    
    /*insert node after marked nodes for given height*/
    void* address_of_node;
    address_of_node = list_insert_after(key, item, (trace[0])); /*insert to level 0 and keep the address to link with higher levels*/
    for (int i=1; i <= height; i++){
        address_of_node = list_insert_after(key, address_of_node, trace[i]);
    }
    free(trace);
}

void* skiplist_search(skip_list* sl, char* key){
    /* search starts from highest level to lowest level */
    if (sl != NULL){
        void* level_node = sl->start;
        void* trace = NULL;
        for (int i=0; i < sl->num_levels; i++){
            level_node = list_search(level_node, key, &trace);
        }
        if (list_iskey(trace, key) == true){
            return  level_node;/*return lowest level which is the answer, if it is the node with given key*/
        } else return NULL;
    } else return NULL;
}

void skiplist_remove(skip_list* sl, char* key){
    /* deletes item with given key */
    void* level_node = sl->start;
    if (sl->destroy == NULL){ 
        /* remove from every list */
        for (int i=0; i < sl->num_levels; i++){
            level_node = list_remove(level_node, key, NULL);
        }
    }
    else { /* if we have to call destroy, then remove the high levels as regular and for the base level call remove with destroy, so that it destroyes the information node */
        for (int i=0; i < sl->num_levels-1; i++){
            level_node = list_remove(level_node, key, NULL);
        }
        list_remove(level_node, key, sl->destroy);
    }

    /* delete any empty lists, but don't delete level 0 list even if it is empty */
    void* temp;
    while (list_isempty(sl->start) == true && list_down(sl->start) != NULL){
        temp = sl->start;
        sl->start = list_down(sl->start);
        list_delete(temp, NULL); /* we don't need the destroy function because we are deleting an empty list */
        sl->num_levels--;
    }
}


void skiplist_delete(void* skipl){
    if (skipl != NULL){
        /*delete each list and free allocated space*/
        skip_list* sl=skipl;
        list_node* l = sl->start;
        while (l != NULL){
            l = list_delete(l, sl->destroy);
        }
        free(sl);
        sl =NULL;
    }
}


void skiplist_print(skip_list* sl){
    void* level = sl->start;
    for (int i=0; i < sl->num_levels; i++){
        printf("level %d skiplist print level %p\n", abs(sl->num_levels-i-1), level);
        level = list_print(level);
    }
}

void* skiplist_getNextEntry(skip_list* sl){
    static void* node;
    if (sl != NULL) { /*if sl is not NULL then it is a new skip list*/
        /*go to the lowest level list*/
        node = sl->start;
        while ( list_down(node) != NULL){
            node = list_down(node);
        }
    }
    /*now return the next item in the skip list*/
    node = list_getNext(node);
    if (node != NULL){
        return list_down(node);
    } else return NULL;    
}
