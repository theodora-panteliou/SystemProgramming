/* Unordered list */
/* Simple version that only has initialize, delete, add and print that is used for keeping stats in stats.c */
/* Each node can hold an item that is void* and its destroy_function is passed in simple_list_init */
#ifndef SIMPLE_LIST_H
#define SIMPLE_LIST_H

typedef struct simple_list{
    void (*delete_item)(void*);
    struct simple_list_node* start;
} simple_list;

typedef struct simple_list_node{
    void* item;
    struct simple_list_node* next;
} simple_list_node;


simple_list* simple_list_init(void (*delete_item_f)(void*)); /* if delete_item_f is NULL it does not destroy the item passed at delete */

void simple_list_add(simple_list* l, void* item);

void simple_list_print(simple_list* l,  void (*print)(void*));

void simple_list_delete(simple_list* l);

#endif