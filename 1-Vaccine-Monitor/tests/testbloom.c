#include <stdio.h>
#include <stdlib.h>
#include "bloomfilter.h"
#include <string.h>
// gcc -o test testbloom.c bloomfilter.c hash_i.c -g
int main(){
    bloom_filter* bl = bloom_init(65, 3);
    char* str = malloc(strlen("dora")+1);
    strcpy(str, "dora");
    
    bloom_insert(bl, str);
    free(str);

    str = malloc(strlen("giorgos")+1);
    strcpy(str, "giorgos");
    bloom_insert(bl, str);
    free(str);

    str = malloc(strlen("dora")+1);
    strcpy(str, "dora");
    printf("dora exists %d\n",bloom_exists(bl, str));
    free(str);

    str = malloc(strlen("giorgos")+1);
    strcpy(str, "giorgos");
    printf("giorgos exists %d\n",bloom_exists(bl, str));
    free(str);

    str = malloc(strlen("geia")+1);
    strcpy(str, "geia");
    printf("geia exists %d\n",bloom_exists(bl, str));
    free(str);
    bloom_delete(bl);

    return 0;
}