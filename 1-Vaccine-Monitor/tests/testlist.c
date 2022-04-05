#include <stdio.h>
#include "list.h"
#include <stdlib.h>

int main(){
    list* l = list_init();
    char* str = malloc(sizeof(char)*4);
    void* trace = NULL, *lower_level = NULL;
    lower_level = list_search(l, 10, &trace);
    list_insert(l, 1, str);
    list_print(l);
    list_insert(l, 3, str);
    list_print(l);
    list_insert(l, 7, str);
    list_print(l);
    list_insert(l, 2, str);
    list_print(l);
    lower_level = list_search(l, 3, &trace);
    printf("lower level %p\n", lower_level);
    list_insert_after(4, str, trace);
    list_print(l);
    list_remove(l, 3);
    list_print(l);
    
    list_delete(l);
    free(str);
    
    return 0;
}