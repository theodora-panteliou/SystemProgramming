#include <stdio.h>
#include "functions.h"
#include <stdlib.h>
#include <string.h>
// gcc -g -Wall -o test.out test_isdate.c functions.c bloomfilter.c hash_i.c hash_table.c skiplist.c list.c citizenRecord.c skiplistrec.c
int main(){
    // printf("date %d\n", is_date("01-04-2000"));
    // printf("date %d\n", is_date("01-04-200"));
    // printf("date %d\n", is_date("aa-04-2000"));
    // printf("date %d\n", is_date("01-04-a00"));
    // printf("date %d\n", is_date("04-2000"));
    // printf("date %d\n", is_date("01-4-2000"));

    printf("name %d\n", is_name("Hksaudj"));
    printf("name %d\n", is_name("HksdsASdj"));
    printf("name %d\n", is_name("Hksa udj"));
    printf("name %d\n", is_virusname("COVID-19"));
    printf("name %d\n", is_age("12"));

    FILE *file_ptr;
    file_ptr = fopen("dieaseFile.txt", "r");
    if (file_ptr == NULL){
        printf("Unable to read file\n");
        return 1;
    }
    char *line;
    size_t linesize = 110;
    line = malloc(linesize*sizeof(char));
     while (getline(&line, &linesize, file_ptr) != -1)
     {
         line = strtok(line, " \n");
        printf("%s %d\n",line, is_virusname(line));
     }
     

}