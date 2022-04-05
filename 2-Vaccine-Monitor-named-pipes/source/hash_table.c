#include "hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "hash_i.h"

#define MAX_LOAD_FACTOR 1.7

typedef struct HashT_entry{
    char* key;
    void* item;
    struct HashT_entry* next;
} HashT_entry;

struct HashT{
    unsigned int nbuckets; /*number of buckets*/
    unsigned int nitems; /*number of items stored in hash table*/
    HashT_entry** table; /*pointer to array of hash table entries*/
    void (*destroy_item)(void *); /* function that destroys hash table's entries*/
};

/* declarations for some functions that will be used inside this file */
int HashT_HashFunction(char* key); /* returns a positive hash value for this key */
HashT_entry* create_entry(char* key, void* item);
void HashT_resize(HashT* hash_table); /* duplicates the buckets and rearranges items for extendible hashing */

int HashT_HashFunction(char* key){
    int hash_value = hash_i((unsigned char *)key, 1);
    return abs(hash_value);
}

HashT_entry* create_entry(char* key, void* item){
    HashT_entry* new_entry = malloc(sizeof(HashT_entry));
    new_entry->key = key;
    new_entry->item = item;
    new_entry->next = NULL;
    return new_entry;
}

HashT_entry* HashT_getEntry(HashT* hash_table, char* key){
    int hash_value = HashT_HashFunction(key)%(hash_table->nbuckets);
    HashT_entry* curr = (hash_table->table)[hash_value];
    if (curr != NULL) {
        do {
            if (strcmp(curr->key, key) == 0) {
                return curr;
            }
            curr = curr->next;
        } while (curr != NULL);
    }
    return  NULL;
}

void HashT_resize(HashT* hash_table){
    int new_nbuckets = hash_table->nbuckets*2;
    HashT_entry** new_table = malloc(sizeof(HashT_entry)*new_nbuckets);
    int new_index;
    HashT_entry* curr = NULL, *prev = NULL;
    for (int i=0; i < hash_table->nbuckets; i++){ /*for every bucket in the old hash table*/

        /*initialize the new_table's buckets (to which old bucket's element will move) with NULL*/
        new_table[i] = NULL;
        new_table[i+hash_table->nbuckets] = NULL;

        if ((hash_table->table)[i] != NULL){ /*if this old bucket isn't empty*/
            /*take each item and rehash it into the new hash table*/
            curr = (hash_table->table)[i];
            prev = NULL;
            do {
                new_index = (HashT_HashFunction(curr->key))%new_nbuckets;
                /*insert to new hash_table[new_index]*/
                if (new_table[new_index] != NULL){ 
                    HashT_entry* curr2 = new_table[new_index], *prev2 = NULL;
                    
                    do {
                        prev2 = curr2;
                        curr2 = curr2->next;
                    } while (curr2!= NULL);
                    prev2->next = create_entry(curr->key, curr->item); /*curr is the pointer to the old bucket's entry*/
                }
                else {
                    new_table[new_index] = create_entry(curr->key, curr->item); /*curr is the pointer to the old bucket's entry*/
                }

                /*move to the next item in this bucket*/
                prev = curr;
                curr = curr->next;
                free(prev);
            } while (curr!= NULL);
            
        }
    } 
    /**/
    hash_table->nbuckets= new_nbuckets;
    free(hash_table->table); /* delete old hash table*/
    hash_table->table = new_table;
    
}

HashT* HashT_init(unsigned int nbuckets, void (*destroy_item)(void *)){
    HashT* hash_table = malloc(sizeof(HashT));
    unsigned int v = nbuckets;
    /* this calculates the next higher power of 2 for nbuckets */
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    /**/
    hash_table->nbuckets = v;
    hash_table->table = malloc(sizeof(HashT_entry)*nbuckets);
    for (int i=0; i < hash_table->nbuckets; i++){
        (hash_table->table)[i] = NULL;
    }
    hash_table->nitems = 0;
    hash_table->destroy_item = destroy_item;
    return hash_table;
}

bool HashT_insert(HashT* hash_table, char* key, void* item){
    if (hash_table == NULL) return NULL;

    int hash_value = HashT_HashFunction(key)%(hash_table->nbuckets);

    if ((hash_table->table)[hash_value] != NULL){ 
        HashT_entry* curr = (hash_table->table)[hash_value], *prev = NULL;
        /* parse that list */
        do {
            if (strcmp(curr->key, key) == 0){
                return false;
            }
            prev = curr;
            curr = curr->next;
        } while (curr!= NULL);
        /* append item at the end of the list */
        prev->next = create_entry(key, item);
    }
    else { /* if bucket is NULL create the first node in the bucket */
        (hash_table->table)[hash_value] = create_entry(key, item);
    }
    (hash_table->nitems)++;
    if ( (float)(hash_table->nitems)/(float)(hash_table->nbuckets) > MAX_LOAD_FACTOR){ /* check load factor for resizing */
        HashT_resize(hash_table);
    }
    return true;
}

void HashT_remove(HashT* hash_table, char* key){
    /* find bucket */
    int hash_value = HashT_HashFunction(key)%(hash_table->nbuckets);
    HashT_entry* curr = (hash_table->table)[hash_value];
    HashT_entry* prev = NULL;
    /* if key is at the start of the list */
    if (curr != NULL && strcmp(curr->key, key) == 0){
        (hash_table->table)[hash_value] = curr->next;
        hash_table->destroy_item(curr->item);
        free(curr);
        curr = NULL;
        return;
    }
    /* else */
    while (curr != NULL && strcmp(curr->key, key) != 0){
        prev = curr;
        curr = curr->next;
    }
    prev->next = curr->next;
    free(curr);
    curr = NULL;
}

void HashT_delete(HashT* hash_table){
    for (int i = 0; i < hash_table->nbuckets; i++){ /* for every bucket */
        if ((hash_table->table)[i] != NULL){
            HashT_entry* temp;
            while ((hash_table->table)[i]) { /* delete list */
                temp = (hash_table->table)[i];
                (hash_table->table)[i] = ((hash_table->table)[i])->next;
                if ((hash_table->destroy_item)!=NULL){
                    (hash_table->destroy_item)(temp->item);
                }
                free(temp);
                temp = NULL;
            }
        }
    }
    free(hash_table->table);
    hash_table->table = NULL;
    free(hash_table);
    hash_table = NULL;
}

bool HashT_exists(HashT* hash_table, char* key) {
    HashT_entry* entry = HashT_getEntry(hash_table, key);
    if (entry == NULL){
        return false;
    } else {
        return true;
    }
}


void* HashT_get(HashT* hash_table, char* key){
    HashT_entry* entry = HashT_getEntry(hash_table, key);
    if (entry == NULL) {
        return NULL;
    } else {
        return entry->item;
    }
}

void HashT_print(HashT* hash_table, void (*print)(void*)) {
    for (int i = 0; i < hash_table->nbuckets; i++){
        if ((hash_table->table)[i] == NULL){
            printf("Index %d empty\n", i);
        }
        else {
            HashT_entry* curr = (hash_table->table)[i];
            printf("Index %d\n", i);
            while (curr!=NULL) {
                printf("\tkey:%s item:", curr->key);
                if (print != NULL){
                    print(curr->item);
                }
                printf("\n");
                curr = curr->next;
            }
        }
    }
    return;
}

void* HashT_getNextEntry(HashT* hash_table) {
    /* static variables keep their values between calls and initialize if hash_table == NULL*/
    static int i;
    static HashT_entry* curr;
    static HashT* static_ht;
    static bool ret; /* return flag */
    
    if (hash_table != NULL) {  /*reset*/
        i=0;
        curr = NULL;
        static_ht = hash_table;
        ret = true;
    }
    /* for each bucket parse the list and return one item at a time */
    while (i < static_ht->nbuckets){ 
        if ((static_ht->table)[i] != NULL){
            if (curr == NULL){
                curr = (static_ht->table)[i];
            }
            while (curr!=NULL) {
                if (ret == true){
                    ret = false;
                    return curr->item;
                }
                ret = true;
                curr = curr->next;
            }
        }
        i++;
    }
    return NULL;
}

void HashT_stats(HashT* hash_table){
    printf("Number of buckets is: %d\number of items is: %d\nCurrent load factor is: %f\n", hash_table->nbuckets, hash_table->nitems, (float)(hash_table->nitems)/(float)(hash_table->nbuckets));
}