#include "hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// gcc -o hash.out testhash.c hash_table.c -g
int main(){ 
    char* str = malloc(sizeof(char)*4);
    char* str2;
    strcpy(str,"hi");

    HashT* ht;
    ht = HashT_init(2, NULL);
    HashT_insert(ht, 4, str);
    HashT_insert(ht, 3, str);
    HashT_insert(ht, 5, str);
    HashT_print(ht,NULL);
    HashT_insert(ht, 7, str);
    HashT_insert(ht, 200, str);
    HashT_insert(ht, 128, str);
    HashT_insert(ht, 87, str);
    HashT_insert(ht, 11, str);
    HashT_insert(ht, 27, str);
    if (HashT_exists(ht, 9)){
        printf("key 9 is in hash_table\n");
    }

    if (HashT_exists(ht, 128)){
        printf("key 128 is in hash_table\n");
    }
    HashT_print(ht,NULL);
    HashT_insert(ht, 15, str);
    HashT_insert(ht, 21, str);
    HashT_insert(ht, 188, str);
    HashT_insert(ht, 67, str);
    HashT_insert(ht, 89, str);
    HashT_insert(ht, 89, str);
    
    if (HashT_exists(ht, 344)){
        printf("key 344 is in hash_table\n");
    }
    if (HashT_exists(ht, 15)){
        printf("key 15 is in hash_table\n");
    }
    if (HashT_exists(ht, 5)){
        printf("key 5 is in hash_table\n");
    }
    if (HashT_exists(ht, 11)){
        printf("key 11 is in hash_table\n");
    }
    HashT_print(ht,NULL);
    HashT_delete(ht);
    free(str);
}