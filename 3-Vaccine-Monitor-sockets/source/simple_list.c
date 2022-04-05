#include "simple_list.h"

#include <stdio.h>
#include <stdlib.h>

simple_list* simple_list_init(void (*delete_item_f)(void*)){
    simple_list* info = malloc(sizeof(simple_list));
    info->delete_item = delete_item_f;
    info->start = NULL;
    return info;
}

void simple_list_add(simple_list* l, void* item){
    simple_list_node* new_node = malloc(sizeof(simple_list_node));
    new_node->item = item;
    new_node->next = l->start;
    l->start = new_node;
}

void simple_list_print(simple_list* l,  void (*print)(void*)){
    if (print == NULL) return; /* nothing to print */
    simple_list_node* temp = l->start;
    while (temp!=NULL){
        print(temp->item);
        temp = temp->next;
    }
}

void simple_list_delete(simple_list* l){
    simple_list_node* temp = l->start, *to_del;
    while (temp!=NULL){
        to_del=temp;
        temp = temp->next;
        if (l->delete_item!=NULL){
            l->delete_item(to_del->item);
        }
        free(to_del);
    }
    free(l);
}