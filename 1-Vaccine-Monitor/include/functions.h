/* Functions and structs that are used from vaccineMonitor (main) */

#include <stdbool.h>
#include "hash_table.h"
#include "skiplist.h"
#include "bloomfilter.h"
#include "citizenRecord.h"

#define YES 1
#define NO 0 

#define K 16 /*K: number of hash functions for bloom filter*/
#define SKIPMAXLEVEL 30 /*max levels in skip list*/

/* functions for input */
void input_init(char* command, char** parameter);
int input_read(char*  line, char** command, char** parameter, int array_size);
void input_print(char** parameter); 


/*entry for a virus in hash table*/
typedef struct VirusEntry{
    char* name;
    skip_list* vaccinated_persons;
    skip_list* non_vaccinated_persons;
    bloom_filter* bloom_vaccinated_persons;
} VirusEntry;

VirusEntry* VirusEntry_init(char* name, int bloom_size);
void VirusEntry_delete(void* virus_entry);

/* inserts a record containing {id, first name, last name, country, age, [date]} with given key, virus, answer to data structures*/
void insert(HashT* VaccineTable, void* rec, void* key, char* virus, int answer, int bloom_size);

/* used for /populationStatus for all countries */
typedef struct country_counter{
    char* name;
    int vaccinated_counter;
    int non_vaccinated_counter;
    int vaccinated_period_counter;
} country_counter;

void country_print(country_counter* c);
country_counter* country_counter_init(char* name);
void country_counter_delete(void* c);

/* used for /popStatusByAge for all countries */
typedef struct country_counterbyage{
    char* name;
    int vaccinated_counter[4];
    int non_vaccinated_counter[4];
    int vaccinated_period_counter[4];
} country_counterbyage;

country_counterbyage* country_counterbyage_init(char* name);
void country_counterbyage_delete(void* c);
int age_index(int age); /* returns 0 for ages 0-20, 1 for 20-40, 2 for 40-60, 3 for 60+ and -1 for error*/

/*dates*/
bool date_between(char* my_date, char * date1, char* date2);
void get_current_date(char *date);
bool is_date(char* d); /*checks if a date is in format dd-mm-yyyy*/

void string_delete(void* str);

/*format checks*/
bool is_name(char *s);
bool is_virusname(char* s);
bool is_age(char* s);
bool is_answer(char* s);
