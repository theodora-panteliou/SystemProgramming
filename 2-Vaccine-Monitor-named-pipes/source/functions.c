#include "functions.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

void input_init(char* command, char** parameter){
    if (command != NULL){ 
        free(command); 
        command = NULL;
    }
    int i = 0;
    while (parameter[i] != NULL){
        free(parameter[i]);
        parameter[i] = NULL;
        i++;
    }
}

int input_read(char*  line, char** command, char** parameter, int array_size){
    /*read input into an array "parameter"*/
    /*0:citizenID 1:firstName 2:lastName 3:country 4:age 5:virus name 6:answer 7:date*/
    char* str;
    for (int i = 0; i < array_size; i++){ /*init each pointer to string with NULL*/
        parameter[i] = NULL;
    }

    int par_count = 0;
    str = strtok(line, " \n");
    if (strncmp(str, "/", (size_t)1) == 0){
        *command = malloc(strlen(str)+1);
        strcpy(*command, str);
        str = strtok(NULL, " \n");
    }
    else *command = NULL;

    while (str!=NULL){
    
        parameter[par_count] = malloc(strlen(str)+1);
        strcpy(parameter[par_count], str);
        // printf("parametre[%d] = %s\n", par_count, parameter[par_count]);
        par_count++;
        str = strtok(NULL, " \n");
        if (par_count>=9){ /*error: no command with more than 9 arguments*/
            par_count=-1;
            break;
        }
    }
    return par_count;
}

void input_print(char** parameter){
    int i=0;
    while (parameter[i]!=NULL){ 
        printf("%s ", parameter[i]);
        i++;
    }
    printf("\n");
}

bool date_bigger_than(char* my_date, char* date1){
    /*compare*/
    /* copy dates so that we can use strtok on them */
    char temp_date1[11], temp_my_date[11];
    strcpy(temp_date1, date1);
    strcpy(temp_my_date, my_date);

    /* convert dates to integers */
    int day1 = atoi(strtok(temp_date1, "-"));
    int month1 = atoi(strtok(NULL, "-"));
    int year1 = atoi(strtok(NULL, "-"));

    int my_day = atoi(strtok(temp_my_date, "-"));
    int my_month = atoi(strtok(NULL, "-"));
    int my_year = atoi(strtok(NULL, "-"));
    if ( my_year > year1){
        return true;
    }
    else if ( ( my_year == year1) && (my_month > month1)){
        return true;
    }
    else if (( my_year == year1) && (my_month == month1) && (my_day >= day1)) {
        return true;
    }
    return false;
}

bool date_between(char* my_date, char * date1, char* date2){
    if (date_bigger_than(my_date, date1) && date_bigger_than(date2, my_date)) return true;
    else return false;
}


country_counter* country_counter_init(char* name){
    country_counter* c = malloc(sizeof(country_counter)); /* allocate */
    /* initialize counters */
    c->name = name;
    c->vaccinated_counter = 0;
    c->non_vaccinated_counter = 0;
    c->vaccinated_period_counter = 0;
    return c;
}

void country_print(country_counter* c){
    printf("name=%s, vacc=%d, non_vacc=%d, period_vacc=%d\n", c->name, c->vaccinated_counter, c->non_vaccinated_counter, c->vaccinated_period_counter);
}

void country_counter_delete(void* c){
    free(c); c=NULL;
}

country_counterbyage* country_counterbyage_init(char* name){
    country_counterbyage* c = malloc(sizeof(country_counterbyage)); /* allocate */
    c->name = name;
    /* initialize counter */
    for (int i=0;i<4;i++){
        c->vaccinated_counter[i] = 0;
        c->non_vaccinated_counter[i] = 0;
        c->vaccinated_period_counter[i] = 0;
    }
    return c;
}

void country_counterbyage_delete(void* c){
    free(c); c=NULL;
}

int age_index(int age){
    /* ages: 0:0-20, 1:20-40, 2:40-60, 3:60+ */
    if ( age>=0 && age <=20) return 0;
    else if (age>20 && age<=40) return 1;
    else if (age>40 && age <=60) return 2;
    else if (age>60 && age <= 120) return 3;
    else return -1; /*error: invalid age*/
}

void get_current_date(char *date){
    /* get date */
    time_t t;
    time(&t);
    ctime(&t);
    struct tm *local = localtime(&t);
    int day, month, year;
    day = local->tm_mday;            // get day of month (1 to 31)
    month = local->tm_mon + 1;       // get month of year (0 to 11)
    year = local->tm_year + 1900;    // get year since 1900
    /* convert date to a string dd-mm-yyyy */
    if ((day/10 == 0) && (month/10 == 0)){
        sprintf(date, "0%d-0%d-%d", day, month, year);
    }
    else if (day/10 == 0){
        sprintf(date, "0%d-%d-%d", day, month, year);
    }
    else if (month/10 == 0){
        sprintf(date, "%d-0%d-%d", day, month, year);
    }
    else{
        sprintf(date, "%d-%d-%d", day, month, year);
    }
}

void string_delete(void* str){
    free(str); str = NULL;
}    

bool is_date(char* d){
    if (strlen(d)>10){ /* dd-mm-yyyy is max 10 characters*/
        return false;
    }
    char* date= malloc(sizeof(char)*(strlen(d)+1));
    strcpy(date, d);
    char* str;
    str = strtok(date, "-");

    int i = 0;
    int integer;
    bool flag = true;
    while (str!=NULL){
        integer = atoi(str); /* convert to integer */
        if (integer == 0){ /* if atoi returns 0 it means that str is not integer or that it is 0 */
            flag = false;
        } 
        else if (i == 0 && ((integer<1 || integer>30) || strlen(str)>2)){ /*day*/
            flag = false;
        }
        else if (i == 1 && ((integer<1 || integer>12) || strlen(str)>2)){ /*month*/
            flag = false;
        }
        else if (i == 2 && strlen(str)!=4){ /* year */
            flag = false;
        }
        i++;
        str = strtok(NULL, "-");
    }
    if (i!=3) flag = false;
    free(date);
    // printf("return %s\n", flag ? "true" : "false");
    return flag;
}

VirusEntry* insert(HashT* VaccineTable, void* rec, void* key, char* virus, int answer, int bloom_size){
    VirusEntry* virus_entry = HashT_get(VaccineTable, virus);
    if (virus_entry == NULL) { /* if that virus doesn't exist, make an entry and insert it to the system */
        virus_entry=VirusEntry_init(virus, bloom_size);
        HashT_insert(VaccineTable, virus_entry->name, virus_entry);
    }

    if (answer==YES) { /* if a person got vaccinated insert it to vaccinated skiplist and to bloom filter*/
        skiplist_insert(virus_entry->vaccinated_persons, key, rec);
        bloom_insert(virus_entry->bloom_vaccinated_persons, key);
        return virus_entry;
    }
    else{ /* if a person is not vaccinated insert it to non_vaccinated skiplist*/
        skiplist_insert(virus_entry->non_vaccinated_persons, key, rec);
        return NULL;
    }
}

bool is_name(char *str){
    /* a name contains only letters */
    unsigned char c; 
	while ((c = *str++)) { 
		if (!isalpha(c)) {
            return false;
        }
	}
	return true;
}

bool is_virusname(char* str){
    /* a virus name contains only letters, digits and at most one "-" */
    unsigned char c; 
    bool flag = false;
	while ((c = *str++)) { 
		if (!isalpha(c) && c!='-' && !isdigit(c)) {
            return false;
        }
        if (c=='-'){
            if (flag == true){
                return false;
            } 
            else {
                flag = true;
            }
        }
	}
	return true;
}

bool is_age(char* s){
    /* age is an integer between 0 and 120 */
    int i = atoi(s);
    if (i<=0 || i>120) return false;
    else return true;
}

bool is_answer(char* s){
    /* answer is "YES" or "NO" */
    if (strcmp(s, "YES")!=0 && strcmp(s, "NO")!=0) return false;
    else return true;
}

VirusEntry* VirusEntry_init(char* name, int bloom_size){
    /* initialize a virus entry with a name, 2 skiplists and a bloom filter */
    VirusEntry* virus_entry = malloc(sizeof(struct VirusEntry));
    virus_entry->name=malloc(sizeof(char)*(strlen(name)+1));
    strcpy(virus_entry->name, name);
    virus_entry->vaccinated_persons=skiplist_init(SKIPMAXLEVEL, date_citizen_delete);
    virus_entry->non_vaccinated_persons=skiplist_init(SKIPMAXLEVEL, NULL);
    virus_entry->bloom_vaccinated_persons=bloom_init(bloom_size, K);
    return virus_entry;
}

void VirusEntry_delete(void* virus_entry){
    VirusEntry* to_del = virus_entry;
    free(to_del->name);
    skiplist_delete(to_del->vaccinated_persons);
    skiplist_delete(to_del->non_vaccinated_persons);
    bloom_delete(to_del->bloom_vaccinated_persons);
    free(to_del);
}

void get_date_six_months_before(char *dest_date, char *src_date){
    char temp_date[11];
    strcpy(temp_date, src_date);
    /* convert dates to integers */
    int day = atoi(strtok(temp_date, "-"));
    int month = atoi(strtok(NULL, "-"));
    int year = atoi(strtok(NULL, "-"));

    int dest_month = month-6;
    int dest_year = year;
    if (dest_month < 1){
        dest_month=12+dest_month;
        dest_year = year -1;
    }
    if ((day/10 == 0) && (dest_month/10 == 0)){
        sprintf(dest_date, "0%d-0%d-%d", day, dest_month, dest_year);
    }
    else if (day/10 == 0){
        sprintf(dest_date, "0%d-%d-%d", day, dest_month, dest_year);
    }
    else if (dest_month/10 == 0){
        sprintf(dest_date, "%d-0%d-%d", day, dest_month, dest_year);
    }
    else{
        sprintf(dest_date, "%d-%d-%d", day, dest_month, dest_year);
    }
}

int update_structures(FILE* file_ptr, HashT* VaccineTable, HashT* citizens, HashT* countries, int bloom_size, HashT* updated){ /* call with updated = NULL the first time that VaccineTable is empty */    
    /* allocate a buffer */
    char *line;
    size_t linesize = 110;
    line = malloc(linesize*sizeof(char));

    citizenRecord* rec, *temp_rec;
    date_citizen* VirusYES_rec;
    VirusEntry* virus_entry, *updated_entry;

    /* variables to read commands and/or parameters */
    char* command;
    char** parameter;
    int array_size = 9; /* 9 is an upper limit to parameters in record's file reading and queries */
    parameter = malloc(sizeof(char*)*array_size); /*array of strings*/

    /*read the files and initialize structures*/
    int rec_counter = 0; /* counts recs read */

    while (getline(&line, &linesize, file_ptr) != -1) {
        rec_counter++;
        int par_count = input_read(line, &command, parameter, array_size); /*0:citizenID 1:firstName 2:lastName 3:country 4:age 5:virus name 6:answer 7:date*/
        if (par_count<7 || !is_name(parameter[1]) || !is_name(parameter[2]) || !is_name(parameter[3]) || !is_age(parameter[4]) || !is_virusname(parameter[5]) || !is_answer(parameter[6])){ /* checking format*/ 
            printf("ERROR IN RECORD ");
            input_print(parameter);
            input_init(NULL, parameter);
            continue;
        }
        else if (( (temp_rec = HashT_get(citizens, parameter[0])) != NULL) && 
                    (strcmp(parameter[1], temp_rec->firstName) != 0 || strcmp(parameter[2], temp_rec->lastName) != 0 
                    || strcmp(parameter[3], temp_rec->country) != 0 || atoi(parameter[4]) != temp_rec->age)){       /*if a citizen with the same id exists and if any data is different: error case*/

            printf("ERROR IN RECORD ");//<incosistent answer: citizen id exists with different data> 
            input_print(parameter);
        }
        else if (strcmp(parameter[6], "NO") == 0 ){
    
            if (parameter[7]!=NULL){ /* NO with date */
                printf("ERROR IN RECORD "); //<date after NO> 
                input_print(parameter);
            } 
            else if ((virus_entry = HashT_get(VaccineTable, parameter[5])) != NULL && (skiplist_search(virus_entry->vaccinated_persons, parameter[0]) != NULL)){ /*citizen already registered as vaccinated*/
                printf("ERROR IN RECORD "); //<incosistent answer: citizen registered as vaccinated>
                input_print(parameter);
            } 
            else if ( (virus_entry!=NULL && skiplist_search(virus_entry->non_vaccinated_persons, parameter[0]) == NULL) || virus_entry==NULL){ /* insert case (if citizen is not already registered as non vaccinated) */
                if (temp_rec!=NULL){ /* if citizen already exists */
                    rec = temp_rec; /* keep that record */
                } 
                else { /*if citizen doesn't exist make a record and insert to citizens hash table*/
                    /*construct record*/
                    rec = citizenRec_construnct(parameter, countries);
                    HashT_insert(citizens, rec->citizenID, rec);
                }  
                /* insert record to VaccineTable */
                updated_entry = insert(VaccineTable, rec, rec->citizenID, parameter[5], NO, bloom_size);
                if (updated_entry!=NULL) HashT_insert(updated, updated_entry->name, updated_entry); /* insert to updatted if updated!=NULL */     

            }
        } 
        else if (strcmp(parameter[6], "YES") == 0) {
            if (parameter[7] == NULL  || !is_date(parameter[7])) { /* YES without date: error case*/
                printf("ERROR IN RECORD "); // <no date after YES>
                input_print(parameter);
            } 
            else if ((virus_entry = HashT_get(VaccineTable, parameter[5])) != NULL && (skiplist_search(virus_entry->non_vaccinated_persons, parameter[0]) != NULL)){ /*citizen already registered as unvaccinated*/
                printf("ERROR IN RECORD "); // <incosistent answer: citizen registered as unvaccinated>
                input_print(parameter);
            } 
            else if ( virus_entry != NULL && ((VirusYES_rec = skiplist_search(virus_entry->vaccinated_persons, parameter[0])) != NULL) ){ /*citizen already vaccinated: error case*/
                printf("ERROR IN RECORD "); //<incosistent answer: citizen already registered as vaccinated> 
                input_print(parameter);
            } 
            else { /*insert case*/
                if (temp_rec!=NULL){ /* if citizen already exists */
                    rec = temp_rec; /* keep that record */
                } 
                else { /*if citizen doesn't exist make a record and insert to hash table*/
                    /*construct record*/
                    rec = citizenRec_construnct(parameter, countries);
                    HashT_insert(citizens, rec->citizenID, rec);
                } 
                /* item to insert to skip list*/
                VirusYES_rec = malloc(sizeof(date_citizen));
                VirusYES_rec->citizen = rec;
                VirusYES_rec->dateVaccinated = malloc(sizeof(char)*(strlen(parameter[7])+1));
                strcpy(VirusYES_rec->dateVaccinated, parameter[7]); 

                /* insert record to VaccineTable */
                updated_entry = insert(VaccineTable, VirusYES_rec, VirusYES_rec->citizen->citizenID, parameter[5], YES, bloom_size);
                if (updated_entry!=NULL) HashT_insert(updated, updated_entry->name, updated_entry); /* insert to updatted if updated!=NULL */
            }
        }
        input_init(NULL, parameter);
    }
    free(line);
    free(parameter);
    return rec_counter;
}