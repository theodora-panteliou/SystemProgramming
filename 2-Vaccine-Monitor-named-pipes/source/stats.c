#include "stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"
#include "simple_list.h"
#include "functions.h" /* for date_between*/

static HashT* stats = NULL;

typedef struct list_item{
    char* country;
    char date[11];
} list_item;

list_item* list_item_make(char* country, char* date){
    list_item* li = malloc(sizeof(list_item));
    li->country = malloc(strlen(country)+1);
    strcpy(li->country, country);
    strcpy(li->date, date);
    return li;
}

void list_item_delete(void* item){
    free(((list_item*)item)->country);
    free(item);
}

typedef struct stats_entry{
    char* virus;
    simple_list* accepted;
    simple_list* rejected;
} stats_entry;

stats_entry* stats_entry_make(char* virus){
    stats_entry* stat = malloc(sizeof(stats_entry));
    stat->virus = malloc(strlen(virus)+1);
    strcpy(stat->virus, virus);
    stat->accepted = simple_list_init(list_item_delete);
    stat->rejected = simple_list_init(list_item_delete);
    return stat;
}

void stats_entry_delete(void* stat){
    stats_entry* temp = stat;
    free(temp->virus);
    simple_list_delete(temp->accepted);
    simple_list_delete(temp->rejected);
    free(temp);
}

void init_stats(){
    stats = HashT_init(4, stats_entry_delete);
}

void update_stats(bool accepted, char* date, char* country, char* virus){
    list_item* li = list_item_make(country, date);

    stats_entry* stat = HashT_get(stats, virus);
    if (stat == NULL){
        stat = stats_entry_make(virus);
        HashT_insert(stats, stat->virus, stat);
    }
    if (accepted){
        simple_list_add(stat->accepted, li); 
    }
    else {
        simple_list_add(stat->rejected, li);
    }
}

int count_in_list(simple_list* list, char* date1, char* date2, char* country);

void print_stats(char* virus, char* date1, char* date2, char* country){
    /* count stats. If country == NULL print summed stats for all countries*/
    stats_entry* stat = HashT_get(stats, virus);
    int accepted_count = 0;
    int rejected_count = 0;
    if (stat != NULL) {
        rejected_count = count_in_list(stat->rejected, date1, date2, country);
        accepted_count = count_in_list(stat->accepted, date1, date2, country);
    }
    printf("ACCEPTED %d\n", accepted_count);
    printf("REJECTED %d\n", rejected_count);
    printf("TOTAL REQUESTS %d\n", accepted_count+rejected_count);
}

void destroy_stats(){
    HashT_delete(stats);
}

int count_in_list(simple_list* list, char* date1, char* date2, char* country){
    simple_list_node* lnode = list->start;
    int count = 0;
    list_item* item;
    while (lnode!=NULL){
        item = lnode->item;
        if (country!=NULL){
            if (strcmp(item->country, country) == 0){
                if (date_between(item->date, date1, date2)){
                    count++;
                }
            }
        }
        else {
            if (date_between(item->date, date1, date2)){
                count++;
            }
        }
        lnode = lnode->next;
    }
    return count;
}