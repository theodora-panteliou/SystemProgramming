#include "skiplist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void string_delete(void* str){
    free(str); str = NULL;
} 

// gcc -o skip.out testSkiplist.c ../src/skiplist.c ../src/list.c -g -I../include
int main(){ 
    skip_list* l;
    l = skiplist_init(8, NULL);
    char* str = malloc(sizeof(char)*4);
    char* str2;
    strcpy(str,"hi");
    skiplist_insert(l, "5", str);
    skiplist_print(l);
    printf("\nsaerching for key 5 \n");
    str2 = skiplist_search(l, "5");
    if (str2 == NULL){
        printf("\nkey not in skiplist\n");
    } else    printf("%s\n", str2);

    skiplist_insert(l, "6", str);
    skiplist_insert(l, "10", str);
    skiplist_insert(l, "39", str);
    skiplist_insert(l, "25", str);
    skiplist_insert(l, "9", str);
    skiplist_insert(l, "13", str);
    skiplist_insert(l, "1", str);
    skiplist_insert(l, "4", str);
    skiplist_insert(l, "8", str);
    
    skiplist_print(l);
    printf("\nsaerching for key 2 \n");
    str2 = skiplist_search(l, "2");
    if (str2 == NULL){
        printf("\nkey not in skiplist\n");
    } else    printf("\n%s\n", str2);

    printf("\nsaerching for key 8\n");
    str2 = skiplist_search(l, "8");
    printf("%s\n", str2);
    skiplist_print(l);
    printf("\nsaerching for key 5 \n");
    str2 = skiplist_search(l, "5");
    printf("\nsaerching for key 1 \n");
    str2 = skiplist_search(l, "1");
    printf("\nsaerching for key 22 \n");
    str2 = skiplist_search(l, "22");
    printf("\nsaerching for key 18 \n");
    str2 = skiplist_search(l, "18");
    printf("\nsaerching for key 13 \n");
    str2 = skiplist_search(l, "13");

    // printf("\nremove 5\n");
    // skiplist_remove(l, "5");
    printf("\nremove 13\n");
    skiplist_remove(l, "13");
    printf("\nremove 1\n");
    skiplist_remove(l, "1");
    printf("\nremove 9\n");
    skiplist_remove(l, "9");
    printf("\nremove 8\n");
    skiplist_remove(l, "8");
    printf("\nremove 6\n");
    skiplist_remove(l, "6");
    printf("\nremove 10\n");
    skiplist_remove(l, "10");
    printf("\nremove 4\n");
    skiplist_remove(l, "4");
    printf("\nremove 25\n");
    skiplist_remove(l, "25");
    skiplist_print(l);
    skiplist_delete(l);
    free(str);

    l = skiplist_init(8, string_delete);
    char* str5 = malloc(sizeof(char)*4);
    strcpy(str5,"hi");
    skiplist_insert(l, "2", str5);
    skiplist_print(l);
    skiplist_remove(l, "2");
    skiplist_print(l);
    skiplist_delete(l);
}