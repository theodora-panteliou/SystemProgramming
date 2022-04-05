#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct list_node{
    struct list_node* next;
    void* lower;
    char* key;
};

list_node* create_node(char* key, void* lower_item, list_node* next_node){
    list_node* new_node = malloc(sizeof(list_node));
    new_node->key = key;
    new_node->lower = lower_item;
    new_node->next = next_node;
    return new_node;
}

list_node* list_init(void* lower){
    list_node* l = malloc(sizeof(list_node));
    l->next = NULL;
    l->lower = lower;
    l->key = NULL;
    return l;
}

void* list_insert_after(char* key, void* lower_item, void* prev){
    /* calls create_node and inserts the new node after the node prev
    and returns the address of the new node, so that it can be used to link nodes between levels*/
    return ((list_node *)prev)->next = create_node(key, lower_item, ((list_node *)prev)->next);
}

void* list_search(void* l, char* key, void** trace){
    /* searches the list starting with node l (that doen't have to be the start of the list-it is just the start of search)*/
    /* sets trace to be the pointer to the node with the <=key, so skiplist_insert can insert the new node after that node if it is needed */
    /* returns pointer to the lower level which may be another list node or information node(that is decided by skiplist).*/
    if (l == NULL) {
        return NULL;
    }
    list_node* curr = ((list_node *)l)->next, *prev = ((list_node *)l);
    while (curr!=NULL && strcmp(curr->key, key) < 0) {
        prev = curr;
        curr = curr->next;
    }
    if (curr!=NULL && strcmp(curr->key, key) == 0) {
        *trace = (void*)curr;
        return curr->lower;
    }
    /*case we didn't find the key*/
    *trace = (void*)prev;
    return prev->lower;
}

void* list_remove(void* l, char* key, void (*destroy_fun)()){
    /* deletes the item with given key. If destroy_fun!=NULL also destroys the lower level.
    Similar to search, but it returns the node with key exclusively less than given key, 
    so that in next level's remove there is at least one node before node_to_be_deleted
    so that we can link the node_to_be_deleted's previous node with node_to_be_deleted's next node */
    if (l == NULL) {
        return NULL;
    }
    list_node* curr = ((list_node *)l)->next, *prev = ((list_node *)l);
    while (curr!=NULL && strcmp(curr->key, key) < 0){
        // printf(" %s ", curr->key);
        prev = curr;
        curr = curr->next;
    }

    if (curr!=NULL && strcmp(curr->key,key) == 0) {
        
        prev->next = curr->next;
        if (destroy_fun!=NULL){
            destroy_fun(curr->lower);
        }
        free(curr);
        curr=NULL;
        // printf(" return at %s\n", prev->key);
        return prev->lower;
        
    }
    /*case we didn't find the key*/ 
    // printf(" return at %s\n", prev->key);
    return prev->lower;
    
}

void* list_print(list_node* l){
    list_node* curr = l->next;
    while (curr!=NULL) {
        printf("\tkey: %s ->", curr->key);
        curr = curr->next;
    }
    printf("\n");
    return l->lower;
}

void* list_delete(list_node* l, void (*destroy_fun)()){
    /*l is the sentinel. If l->lower is NULL meaning that there is no lower list, then the lower level contains structs that will be destroyed with destroy_fun*/
    if (l->next != NULL){
        list_node* temp, *curr = l->next;
        while (curr) {
            temp = curr;
            curr = curr->next;
            if (destroy_fun != NULL && l->lower == NULL){
                destroy_fun(temp->lower);
            }
            free(temp);
            temp = NULL;
        }
    }
    void* temp = l->lower; 
    free(l);
    l=NULL;
    return temp;
}

bool list_iskey(void* node, char* key){
    if (((list_node*)node)->key!=NULL && strcmp(((list_node*)node)->key , key) == 0){
        return true;
    } 
    else if (((list_node*)node)->key==NULL && key==NULL)
        return true;
    else 
        return false;
}

void* list_getNext(void* node){
    return ((list_node*)node)->next;
}

void* list_down(void* node){
    return ((list_node*)node)->lower;
}

bool list_isempty(void* node){
    if (((list_node*)node)->next == NULL){
        return true;
    } 
    else return false;
}