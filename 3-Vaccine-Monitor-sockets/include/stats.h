#include <stdbool.h>

void init_stats(); /* initialize to start counting stats */
void update_stats(bool accepted, char* date, char* country, char* virus);
void print_stats(char* virus, char* date1, char* date2, char* country); /* if country is NULL prints summed stats for all countries */
void destroy_stats();