#include "functions.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

/*color*/
#define HMAG "\e[0;95m"
#define reset "\e[0m"
#define MAG "\e[0;35m"
/*run: ./vaccineMonitor -c citizenRecordsFile –b bloomSize*/
// ./vaccineMonitor -c inputFile.txt -b 100000

int main(int argc, char *argv[]) {
     
    printf("%d\n",date_between("12-2-2002", "11-02-2002", "21-2-2002"));
    /*command line input*/
    char* file_name = NULL;
    int bloom_size;
    
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-c") == 0) { /* input file */
            file_name = argv[i+1];
        } else if (strcmp(argv[i], "-b") == 0) { /* bloom size in bytes */
            if (argv[i+1] == NULL){
                printf("Invalid bloom size.\n");
                return 1;
            }
            bloom_size = atoi(argv[i+1])*8; /* convert to bits */
            if (bloom_size == 0) {
                printf("Invalid bloom size.\n");
                return 1;
            }
        }
    }
    
    /*open file*/
    FILE *file_ptr;
    file_ptr = fopen(file_name, "r");
    if (file_ptr == NULL){
        printf("Unable to read file\n");
        return 1;
    }
    file_name = NULL;

    /*initilize structs that will be used to store data*/
    HashT* citizens = HashT_init(1, citizenRec_delete); /*hash table which consists of citizen records*/
    HashT* VaccineTable = HashT_init(3, VirusEntry_delete); /*each entry consists of two skip lists for viruses with vaccinated citizens and unvaccinated citizens*/
    HashT* countries = HashT_init(3, string_delete); /*consists of countries. citizen->country points here*/

    /* allocate a buffer */
    char *line;
    size_t linesize = 110;
    line = malloc(linesize*sizeof(char));

    citizenRecord* rec, *temp_rec;
    date_citizen* VirusYES_rec;
    VirusEntry* virus_entry;

    /* variables to read commands and/or parameters */
    char* command;
    char** parameter;
    int array_size = 9; /* 9 is an upper limit to parameters in record's file reading and queries */
    parameter = malloc(sizeof(char*)*array_size); /*array of strings*/

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
            else if ( virus_entry!=NULL && skiplist_search(virus_entry->non_vaccinated_persons, parameter[0]) == NULL ){ /* insert case (if citizen is not already registered as non vaccinated) */
                if (temp_rec!=NULL){ /* if citizen already exists */
                    rec = temp_rec; /* keep that record */
                } 
                else { /*if citizen doesn't exist make a record and insert to citizens hash table*/
                    /*construct record*/
                    rec = citizenRec_construnct(parameter, countries);
                    HashT_insert(citizens, rec->citizenID, rec);
                }  
                /* insert record to VaccineTable */
                insert(VaccineTable, rec, rec->citizenID, parameter[5], NO, bloom_size);
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
                insert(VaccineTable, VirusYES_rec, VirusYES_rec->citizen->citizenID, parameter[5], YES, bloom_size);
            }
        }
        input_init(NULL, parameter);
    }
    // printf("%d records read\n", rec_counter);
    
    /*done reading records from file*/
    fclose(file_ptr);
    file_ptr = NULL;

    /* waiting for input from stdin */
    while(1){
        /*read input*/
        fflush(stdin);
        printf(MAG"\ninput:\n"HMAG); /*\e[38;5;214m*/
        getline(&line, &linesize, stdin);
        // printf("%s", line);
        printf(reset);
        /**/
        int par_count = input_read(line, &command, parameter, array_size);
        if (command == NULL){
            printf("Invalid input\n");
            input_init(command, parameter);
        }
        /*cases*/
        else if (strcmp(command, "/exit") == 0){ /* /exit */
            input_init(command, parameter);
            break;
        } 
        else if (strcmp(command, "/vaccineStatusBloom") == 0) { /* /vaccineStatusBloom citizenID virusName */
            /* prints MAYBE if the bloom filter returns false, even if the citizen id does not exist in citizens hash table */
            if (par_count != 2 || !is_virusname(parameter[1])){
                printf("Syntax is /vaccineStatusBloom citizenID virusName. Try again.\n");
            } 
            else {
                /*checking bloom filter*/
                VirusEntry* virus_entry = HashT_get(VaccineTable, parameter[1]);
                if (virus_entry!=NULL){
                    bloom_filter* bl = virus_entry->bloom_vaccinated_persons; /*find bloom filter with key "virusName"*/
                    if (bl != NULL){
                        if (bloom_exists(bl, parameter[0]) == false) { /*check if "citizenID" exists in bloom filter*/
                            printf("NOT VACCINATED\n");
                        } 
                        else {
                            printf("MAYBE\n");
                        }
                    }
                }
                else{
                    printf("Virus doesn't exist.\n");
                }
            }
        } 
        else if (strcmp(command, "/vaccineStatus") == 0 && par_count == 2){ /* /vaccineStatus citizenID virusName */
            /* prints YES and date if citizenID exists in virusNames's skiplist, else prints NO (even if citizenID is not explicitly registered as non-vaccinated)
            If citizenID or virusName doesn't exist it prints an error message*/
            if (!is_virusname(parameter[1])){
                printf("Syntax is /vaccineStatus citizenID virusName or /vaccineStatus citizenID. Try again.\n"); 
            }
            if (HashT_exists(citizens, parameter[0]) == true){
                /*checking skip list*/
                VirusEntry* virus_entry = HashT_get(VaccineTable, parameter[1]);
                if (virus_entry!= NULL){
                    skip_list* sl = virus_entry->vaccinated_persons; /*find vaccinated_persons skip list with key "virusName"*/
                    if (sl != NULL) {
                        date_citizen* rec = skiplist_search(sl, parameter[0]);
                        if (rec != NULL) { /*check if "citizenID" exists in bloom filter*/
                            printf("VACCINATED ON %s\n", rec->dateVaccinated);
                        }
                        else {
                            printf("NOT VACCINATED\n");
                        }
                    } 
                    // skiplist_print(virus_entry->non_vaccinated_persons);
                }
                else {
                    printf("Virus name doesn't exist.\n");
                }
            }
            else {
                printf("Citizen %s is not registered\n", parameter[0]);
            }
        } 
        else if (strcmp(command, "/vaccineStatus") == 0 && par_count != 2){ /* /vaccineStatus citizenID*/
            /* prints every result that exists for this citizenID in vaccinated skiplist or in non-vaccinated skiplist. If a citizenID does not exist for a virus it doesn't print anything
            if a citizenID does not exist at all it prints a message */
            if (parameter[0] == NULL) {
                printf("Syntax is /vaccineStatus citizenID virusName or /vaccineStatus citizenID. Try again.\n");   
            } else if (HashT_exists(citizens, parameter[0]) == false){
                printf("Citizen ID: %s doesn't exist.\n", parameter[0]);
            }
            else {
                VirusEntry* virus_entry = HashT_getNextEntry(VaccineTable);
                skip_list* sl = NULL;
                while ( virus_entry != NULL){ /* for each entry in VaccineTable search citizenID in vaccinated_persons skip list and in non_vaccinated persons skip list */

                    sl = virus_entry->vaccinated_persons; 
                    date_citizen* rec = skiplist_search(sl, parameter[0]);
                    if (rec != NULL) {
                        printf("%s YES %s\n", virus_entry->name, rec->dateVaccinated);
                    }
                    
                    sl = virus_entry->non_vaccinated_persons; 
                    rec = skiplist_search(sl, parameter[0]);
                    if (rec != NULL){
                        printf("%s NO\n", virus_entry->name);
                    }
                    
                    virus_entry = HashT_getNextEntry(NULL);
                } 
            }
        } 
        else if (strcmp(command, "/populationStatus") == 0) { /* /populationStatus [country] virusName date1 date2 */
            bool dates; /*flag for dates*/
            if ((par_count<1 || par_count>4) || is_date(parameter[0])){
                printf("Syntax is /populationStatus [country] virusName [date1 date2]. Try again.\n");  
                input_init(command, parameter);
                continue;
            }
            if (par_count>=2){
                if (is_date(parameter[par_count-1]) && is_date(parameter[par_count-2])) /*if there are dates they should be 2 and at the end of the command*/
                    dates = true;
                else if (!is_date(parameter[par_count-1]) && !is_date(parameter[par_count-2])) 
                    dates = false;
                else { 
                    printf("Syntax is /populationStatus [country] virusName [date1 date2]. Try again.\n");  
                    input_init(command, parameter);
                    continue;
                }
            } else dates = false; /*if there is only one argument it is not date*/

            if ((par_count == 4 && dates == true) || (par_count == 2 && dates == false)){ /* /populationStatus country virusName [date1 date2] */
                if (!is_name(parameter[0]) || !is_virusname(parameter[1])){
                    printf("Syntax is /populationStatus [country] virusName [date1 date2]. Try again.\n");  
                    input_init(command, parameter);
                    continue;
                }
                if (HashT_exists(countries, parameter[0]) == false){
                    printf("Country doesn't exist.\n");
                    input_init(command, parameter);
                    continue;
                }
                int non_vaccinated_counter = 0;
                int vaccinated_counter = 0;
                int vaccinated_period_counter = 0;
                VirusEntry* virus_entry = HashT_get(VaccineTable, parameter[1]);
                if (virus_entry!=NULL){
                    /*counting vaccinated persons for virusName*/
                    skip_list* sl = virus_entry->vaccinated_persons;
                    if (sl!= NULL){
                        VirusYES_rec = skiplist_getNextEntry(sl);
                        while (VirusYES_rec!=NULL){
                            if (strcmp(VirusYES_rec->citizen->country, parameter[0]) == 0 && dates == true){
                                if (date_between(VirusYES_rec->dateVaccinated, parameter[2], parameter[3])){
                                    vaccinated_period_counter++;
                                }
                            }
                            if (strcmp(VirusYES_rec->citizen->country, parameter[0]) == 0) {
                                vaccinated_counter++;
                            }
                            VirusYES_rec = skiplist_getNextEntry(NULL);
                        }
                    }
                    /*counting unvaccinated persons for virusName*/
                    sl = virus_entry->non_vaccinated_persons;
                    if ( sl!= NULL){
                        rec = skiplist_getNextEntry(sl);
                        while (rec!=NULL){
                            if (strcmp(rec->country, parameter[0]) == 0) {
                                non_vaccinated_counter++;
                            }
                            rec = skiplist_getNextEntry(NULL);
                        }
                    }
                    if (dates == false){
                        float percentage;
                        if (non_vaccinated_counter+vaccinated_counter != 0){
                            percentage = ((float)vaccinated_counter/(float)(non_vaccinated_counter+vaccinated_counter))*100;
                        }
                        else if (vaccinated_counter == 0) percentage = 0;
                        else percentage = 100;

                        printf("%d %0.2f%%\n", vaccinated_counter, percentage);
                    }
                    else {
                        float percentage;
                        if (non_vaccinated_counter+vaccinated_counter != 0){
                            percentage = ((float)vaccinated_period_counter/(float)(non_vaccinated_counter+vaccinated_counter))*100;
                        }
                        else if (vaccinated_period_counter == 0) percentage = 0;
                        else percentage = 100;
                        printf("%d %0.2f%%\n", vaccinated_period_counter, percentage);
                    }
                }
                else printf("Virus doesn't exist.\n");
            } 
            else if ((par_count == 3 && dates == true) || (par_count == 1 && dates == false)){ /*for every country*/
                if (!is_virusname(parameter[0])){
                    printf("Syntax is /populationStatus [country] virusName [date1 date2]. Try again.\n");  
                    input_init(command, parameter);
                    continue;
                }
                HashT* countries_count = HashT_init(1, country_counter_delete); /*hash table with entries country_coutner*/
                country_counter* c_counter;
                /*counting vaccinated persons for virusName*/
                VirusEntry* virus_entry = HashT_get(VaccineTable, parameter[0]);
                if (virus_entry!=NULL){
                    skip_list* sl = virus_entry->vaccinated_persons;
                    if (sl != NULL){
                        
                        VirusYES_rec = skiplist_getNextEntry(sl);
                        while (VirusYES_rec!=NULL){
                            c_counter = HashT_get(countries_count, VirusYES_rec->citizen->country);
                            if (c_counter == NULL) {
                                c_counter = country_counter_init(VirusYES_rec->citizen->country);
                                HashT_insert(countries_count, VirusYES_rec->citizen->country, c_counter);
                            }

                            if (dates == true && date_between(VirusYES_rec->dateVaccinated, parameter[1], parameter[2])){
                                c_counter->vaccinated_period_counter++;
                            }
                            c_counter->vaccinated_counter++;
                            VirusYES_rec = skiplist_getNextEntry(NULL);
                        }
                    }
                    /*counting non-vaccinated persons for virusName*/
                    sl = virus_entry->non_vaccinated_persons;
                    if (sl != NULL){
                        rec = skiplist_getNextEntry(sl);
                        while (rec!=NULL){
                            c_counter = HashT_get(countries_count, rec->country);
                            if (c_counter == NULL) {
                                c_counter = country_counter_init(rec->country);
                                HashT_insert(countries_count, rec->country, c_counter);
                            }
                            c_counter->non_vaccinated_counter++;
                            rec = skiplist_getNextEntry(NULL);
                        }
                    }
                    if (dates == false){
                        float percentage;
                        
                        c_counter = HashT_getNextEntry(countries_count);
                        while (c_counter!=NULL){
                            if (c_counter->non_vaccinated_counter+c_counter->vaccinated_counter != 0){
                                percentage = ((float)c_counter->vaccinated_counter/(float)(c_counter->non_vaccinated_counter+c_counter->vaccinated_counter))*100;
                            } 
                            else if (c_counter->vaccinated_counter == 0) percentage = 0;
                            else percentage = 100;
                            printf("%s %d %0.2f%%\n", c_counter->name, c_counter->vaccinated_counter, percentage);
                            c_counter = HashT_getNextEntry(NULL);
                        }
                    } else {
                        float percentage;

                        c_counter = HashT_getNextEntry(countries_count);
                        while (c_counter!=NULL){
                            if (c_counter->non_vaccinated_counter+c_counter->vaccinated_counter != 0){
                                percentage = ((float)c_counter->vaccinated_period_counter/(float)(c_counter->non_vaccinated_counter+c_counter->vaccinated_counter))*100;
                            } 
                            else if (c_counter->vaccinated_period_counter == 0) percentage = 0;
                            else percentage = 100;
                            
                            printf("%s %d %0.2f%%\n", c_counter->name, c_counter->vaccinated_period_counter, percentage);
                            c_counter = HashT_getNextEntry(NULL);
                        }
                    }
                    HashT_delete(countries_count);
                }
                else printf("Virus doesn't exist\n");
            }
            else printf("Syntax is /populationStatus [country] virusName [date1 date2]. Try again.\n");
        } 
        else if (strcmp(command, "/popStatusByAge") == 0) { /* /popStatusByAge [country] virusName date1 date2 */
            bool dates; /*flag for dates*/
            if ((par_count<1 || par_count>4) || is_date(parameter[0])){
                printf("Syntax is /popStatusByAge [country] virusName [date1 date2]. Try again.\n");   
                input_init(command, parameter);
                continue;
            }
            if (par_count>=2){
                if (is_date(parameter[par_count-1]) && is_date(parameter[par_count-2])) /*if there are dates they should be 2 and at the end of the command*/
                    dates = true;
                else if (!is_date(parameter[par_count-1]) && !is_date(parameter[par_count-2])) 
                    dates = false;
                else { 
                    printf("Syntax is /popStatusByAge [country] virusName [date1 date2]. Try again.\n");   
                    input_init(command, parameter);
                    continue;
                }
            } else dates = false; /*if there is only one argument it is not date*/

            if ((par_count == 4 && dates == true) || (par_count == 2 && dates == false)){
               if (!is_name(parameter[0]) || !is_virusname(parameter[1])){
                    printf("Syntax is /popStatusByAge [country] virusName [date1 date2]. Try again.\n");  
                    input_init(command, parameter);
                    continue;
                }
                /* ages: 0:0-20, 1:20-40, 2:40-60, 3:60+ */
                int non_vaccinated_counter[4] = {0,0,0,0};
                int vaccinated_counter[4] = {0,0,0,0};
                int vaccinated_period_counter[4] = {0,0,0,0};
                int index;

                
                VirusEntry* virus_entry = HashT_get(VaccineTable, parameter[1]);
                if (virus_entry!=NULL){
                    /*counting vaccinated persons for virusName*/
                    skip_list* sl = virus_entry->vaccinated_persons;
                    if (sl != NULL){
                        VirusYES_rec = skiplist_getNextEntry(sl);
                        while (VirusYES_rec!=NULL){
                            index = age_index(VirusYES_rec->citizen->age);
                            if (strcmp(VirusYES_rec->citizen->country, parameter[0]) == 0 && dates == true){
                                if (date_between(VirusYES_rec->dateVaccinated, parameter[2], parameter[3])){
                                    vaccinated_period_counter[index]++;
                                }
                            }
                            if (strcmp(VirusYES_rec->citizen->country, parameter[0]) == 0) {
                                vaccinated_counter[index]++;
                            }
                            VirusYES_rec = skiplist_getNextEntry(NULL);
                        }
                    }
                    /*counting vaccinated persons for virusName*/
                    sl = virus_entry->non_vaccinated_persons;
                    if (sl != NULL){
                        rec = skiplist_getNextEntry(sl);
                        while (rec!=NULL){
                            index = age_index(rec->age);
                            if (strcmp(rec->country, parameter[0]) == 0) {
                                non_vaccinated_counter[index]++;
                            }
                            rec = skiplist_getNextEntry(NULL);
                        }
                    }
                    if (dates == false){
                        float percentage;
                        char ages[4][6] = {"0-20", "21-40", "41-60", "60+"};
                        for (int i=0; i<4; i++){
                            if (non_vaccinated_counter[i]+vaccinated_counter[i] != 0){
                                percentage = ((float)vaccinated_counter[i]/(float)(non_vaccinated_counter[i]+vaccinated_counter[i]))*100;
                            }
                            else if (vaccinated_counter[i] == 0) percentage = 0;
                            else percentage = 100;
                            printf("%s %d %f%%\n", ages[i], vaccinated_counter[i], percentage);
                        }
                    } else {
                        float percentage;
                        char ages[4][6] = {"0-20", "21-40", "41-60", "60+"};
                        for (int i=0; i<4; i++){
                            if (non_vaccinated_counter[i]+vaccinated_counter[i] != 0){
                                percentage = ((float)vaccinated_period_counter[i]/(float)(non_vaccinated_counter[i]+vaccinated_counter[i]))*100;
                            }
                            else if (vaccinated_period_counter[i] == 0) percentage = 0;
                            else percentage = 100;
                            
                            printf("%s %d %f%%\n", ages[i], vaccinated_period_counter[i], percentage);
                        }
                    }
                }
                else printf("Virus doesn't exist.\n");
            } else if ((par_count == 3 && dates == true) || (par_count == 1 && dates == false)){ /*for every country*/
                if (!is_virusname(parameter[0])){
                    printf("Syntax is /popStatusByAge [country] virusName [date1 date2]. Try again.\n");  
                    input_init(command, parameter);
                    continue;
                }
                HashT* countries_count = HashT_init(1, country_counterbyage_delete); /*hash table with entries country_coutner*/
                country_counterbyage* c_counter;
                int index;
                
                VirusEntry* virus_entry = HashT_get(VaccineTable, parameter[0]);
                if (virus_entry!=NULL){
                    /*counting vaccinated persons for virusName*/
                    skip_list* sl = virus_entry->vaccinated_persons;
                    if (sl != NULL){
                        VirusYES_rec = skiplist_getNextEntry(sl);
                        while (VirusYES_rec!=NULL){
                            index = age_index(VirusYES_rec->citizen->age);
                            c_counter = HashT_get(countries_count, VirusYES_rec->citizen->country);
                            if (c_counter == NULL) {
                                c_counter = country_counterbyage_init(VirusYES_rec->citizen->country);
                                HashT_insert(countries_count, VirusYES_rec->citizen->country, c_counter);
                            }

                            if (dates == true && date_between(VirusYES_rec->dateVaccinated, parameter[1], parameter[2])){
                                c_counter->vaccinated_period_counter[index]++;
                            }
                            c_counter->vaccinated_counter[index]++;
                            VirusYES_rec = skiplist_getNextEntry(NULL);
                        }
                    }
                    /*counting non vaccinated persons for virusName*/
                     sl = virus_entry->non_vaccinated_persons;
                    if (sl != NULL){
                        rec = skiplist_getNextEntry(sl);
                        while (rec!=NULL){
                            index = age_index(rec->age);
                            c_counter = HashT_get(countries_count, rec->country);
                            if (c_counter == NULL) {
                                c_counter = country_counterbyage_init(rec->country);
                                HashT_insert(countries_count, rec->country, c_counter);
                            }
                            c_counter->non_vaccinated_counter[index]++;
                            rec = skiplist_getNextEntry(NULL);
                        }
                    }
                    if (dates == false){
                        float percentage;
                        char ages[4][6] = {"0-20", "21-40", "41-60", "60+"};
                        
                        c_counter = HashT_getNextEntry(countries_count);
                        while (c_counter!=NULL){
                            printf("%s\n", c_counter->name);
                            for (int i=0; i<4; i++){
                                if (c_counter->non_vaccinated_counter[i]+c_counter->vaccinated_counter[i] != 0){
                                    percentage = ((float)c_counter->vaccinated_counter[i]/(float)(c_counter->non_vaccinated_counter[i]+c_counter->vaccinated_counter[i]))*100;
                                }
                                else if (c_counter->vaccinated_counter[i] == 0) percentage = 0;
                                else percentage = 100;
                                
                                printf("%s %d %f%%\n", ages[i], c_counter->vaccinated_counter[i], percentage);
                            }
                            c_counter = HashT_getNextEntry(NULL);
                        }
                    } else {
                        float percentage;
                        char ages[4][6] = {"0-20", "21-40", "41-60", "60+"};
                        // HashT_print(countries, country_print);
                        c_counter = HashT_getNextEntry(countries_count);
                        while (c_counter!=NULL){
                            printf("%s\n", c_counter->name);
                            for (int i=0; i<4; i++){
                                if (c_counter->non_vaccinated_counter[i] + c_counter->vaccinated_counter[i] != 0){
                                    percentage = ((float)c_counter->vaccinated_period_counter[i]/(float)(c_counter->non_vaccinated_counter[i] + c_counter->vaccinated_counter[i]))*100;
                                }
                                else if (c_counter->vaccinated_period_counter[i] == 0) percentage = 0;
                                else percentage = 100;
                                
                                printf("%s %d %0.2f%%\n", ages[i], c_counter->vaccinated_period_counter[i], percentage);
                            }
                            c_counter = HashT_getNextEntry(NULL);
                        }
                    }
                    HashT_delete(countries_count);
                }
                else printf("Virus doesn't exist.\n");
            } 
            else printf("Syntax is /popStatusByAge [country] virusName date1 date2. Try again.\n");
        } 
        else if (strcmp(command, "/insertCitizenRecord") == 0) { /* /insertCitizenRecord citizenID firstName lastName country age virusName YES/NO [date] */

            if ((par_count!=7 && par_count!=8) || !is_name(parameter[1]) || !is_name(parameter[2]) || !is_name(parameter[3]) || !is_age(parameter[4]) || !is_virusname(parameter[5]) || !is_answer(parameter[6])) {
                printf("Syntax is /insertCitizenRecord citizenID firstName lastName country age virusName YES/NO [date]. Try again.\n");  
            } 
            else if (( (temp_rec = HashT_get(citizens, parameter[0])) != NULL) && 
                    (strcmp(parameter[1], temp_rec->firstName) != 0 || strcmp(parameter[2], temp_rec->lastName) != 0 
                    || strcmp(parameter[3], temp_rec->country) != 0 || atoi(parameter[4]) != temp_rec->age)){       /*if a citizen with the same id exists and if any data is different: error case*/

                printf("ERROR IN RECORD <incosistent answer: citizen id exists with different data> ");
                input_print(parameter);
            }
            else if (strcmp(parameter[6], "NO") == 0 ){
            
                if (parameter[7]!=NULL){ /* NO with date */
                    printf("ERROR IN RECORD <date after NO> ");
                    input_print(parameter);
                }
                else if ( ( (virus_entry = HashT_get(VaccineTable, parameter[5])) != NULL ) && ((VirusYES_rec = skiplist_search(virus_entry->vaccinated_persons, parameter[0])) != NULL) ){ /*citizen already vaccinated: error case*/
                    printf("ERROR: CITIZEN %s ALREADY VACCINATED ON %s\n", parameter[0], VirusYES_rec->dateVaccinated );
                }
                else if ( virus_entry!=NULL && skiplist_search(virus_entry->non_vaccinated_persons, parameter[0]) ==NULL){ /* insert case (if citizen is not already registered as non vaccinated) */
                    /*construct record*/
                    if (temp_rec!=NULL){ /* if citizen already exists */
                        rec = temp_rec; /* keep that record */
                    } 
                    else { /*if citizen doesn't exist make a record and insert to hash table*/
                        /*construct record*/
                        rec = citizenRec_construnct(parameter, countries);
                        HashT_insert(citizens, rec->citizenID, rec);
                    }
                    /* insert to VaccineTable */
                    insert(VaccineTable, rec, rec->citizenID, parameter[5], NO, bloom_size);
                }

            } else if (strcmp(parameter[6], "YES") == 0) {

                if (parameter[7] == NULL || !is_date(parameter[7])) { /* YES without date: error case*/
                    printf("ERROR IN RECORD <no date after YES> ");
                    input_print(parameter);
                }
                else if ( ( (virus_entry = HashT_get(VaccineTable, parameter[5])) != NULL ) && ((VirusYES_rec = skiplist_search(virus_entry->vaccinated_persons, parameter[0])) != NULL) ){ /*citizen already vaccinated: error case*/
                    printf("ERROR: CITIZEN %s ALREADY VACCINATED ON %s\n", parameter[0], VirusYES_rec->dateVaccinated );
                } 
                else { /*insert case*/
                    if (temp_rec!=NULL){ /* if citizen already exists */
                        rec = temp_rec; /* keep that record */
                        if ((virus_entry = HashT_get(VaccineTable, parameter[5])) != NULL && (skiplist_search(virus_entry->non_vaccinated_persons, parameter[0])) != NULL){ /* this case is different from when we insert a record from the file. ...*/ 
                            /*... If the citizen exists in the database as non-vaccinated re-register as vaccinated and remove from non-vaccinated*/
                            skiplist_remove(virus_entry->non_vaccinated_persons, parameter[0]);
                        }
                    } 
                    else { /*if citizen doesn't exist make a record and insert to hash table*/
                        /*construct record*/
                        rec = citizenRec_construnct(parameter, countries);
                        HashT_insert(citizens, rec->citizenID, rec);
                    } 
                    /* item to insert to skip list */
                    VirusYES_rec = malloc(sizeof(date_citizen));
                    VirusYES_rec->citizen = rec;
                    VirusYES_rec->dateVaccinated = malloc(sizeof(char)*(strlen(parameter[7])+1));
                    strcpy(VirusYES_rec->dateVaccinated, parameter[7]); 
                    /* insert to VaccineTable */
                    insert(VaccineTable, VirusYES_rec, VirusYES_rec->citizen->citizenID, parameter[5], YES, bloom_size);
                }

            }

        }
        else if (strcmp(command, "/vaccinateNow") == 0) { /* /vaccinateNow citizenID firstName lastName country age virusName */
            
            if (par_count!=6 || !is_name(parameter[1]) || !is_name(parameter[2]) || !is_name(parameter[3]) || !is_age(parameter[4]) || !is_virusname(parameter[5])){
                printf("Syntax is /vaccinateNow citizenID firstName lastName country age virusName. Try again.\n");  
            } 
            else if (( (temp_rec = HashT_get(citizens, parameter[0])) != NULL) && 
                        (strcmp(parameter[1], temp_rec->firstName) != 0 || strcmp(parameter[2], temp_rec->lastName) != 0 
                        || strcmp(parameter[3], temp_rec->country) != 0 || atoi(parameter[4]) != temp_rec->age)){       /*if a citizen with the same id exists and if any data is different: error case*/

                printf("ERROR IN RECORD <incosistent answer: citizen id exists with different data> ");
                input_print(parameter);
            } 
            else if ( ( (virus_entry = HashT_get(VaccineTable, parameter[5])) != NULL ) && ((VirusYES_rec = skiplist_search(virus_entry->vaccinated_persons, parameter[0])) != NULL) ){ /*citizen already vaccinated: error case*/
                printf("ERROR: CITIZEN %s ALREADY VACCINATED ON %s\n", parameter[0], VirusYES_rec->dateVaccinated );
            } 
            else { /*insert case*/
                if (temp_rec!=NULL){ /* if citizen already exists */
                    rec = temp_rec; /* keep that record */
                    if ((virus_entry = HashT_get(VaccineTable, parameter[5])) != NULL && (skiplist_search(virus_entry->non_vaccinated_persons, parameter[0])) != NULL){ /* this case is different from when we insert a record from the file. ...*/ 
                        /*... If the citizen exists in the database as non-vaccinated re-register as vaccinated and remove from non-vaccinated*/
                        skiplist_remove(virus_entry->non_vaccinated_persons, parameter[0]);
                    }
                } 
                else { /*if citizen doesn't exist make a record and insert to hash table*/
                    /*construct record*/
                    rec = citizenRec_construnct(parameter, countries);
                    HashT_insert(citizens, rec->citizenID, rec);
                } 
                /* item to insert to skip list*/
                VirusYES_rec = malloc(sizeof(date_citizen));
                VirusYES_rec->citizen = rec;
                VirusYES_rec->dateVaccinated = malloc(sizeof(char)*11);
                get_current_date(VirusYES_rec->dateVaccinated);
                /* insert to VaccineTable */
                insert(VaccineTable, VirusYES_rec, VirusYES_rec->citizen->citizenID, parameter[5], YES, bloom_size);
            }
        } 
        else if (strcmp(command, "/list-nonVaccinated-Persons") == 0) { /* /list-nonVaccinated-Persons virusName */

            if (par_count!=1 || !is_virusname(parameter[0])){
                printf("Syntax is /list-nonVaccinated-Persons virusName. Try again.\n");  
            } 
            else {
                VirusEntry* virus_entry = HashT_get(VaccineTable, parameter[0]); /* get virus entry */
                if (virus_entry!=NULL){
                    skip_list* sl = virus_entry->non_vaccinated_persons; /* get non_vacinated_persons list for that virus */

                    /* print every item in skiplist */
                    rec = skiplist_getNextEntry(sl);
                    while (rec!=NULL){ 
                        citizenRec_print(rec);
                        rec = skiplist_getNextEntry(NULL);
                    }
                }
                else printf("Virus name doesn't exist.\n");
            }
        }
        else {
            printf("Invalid command\n");   
        }

        input_init(command, parameter);
    }
    free(parameter);
    free(line);
    // printf("citizens:\n");
    // HashT_stats(citizens);
    // printf("Vaccine table:\n");
    // HashT_stats(VaccineTable);
    // printf("(countries:\n");
    // HashT_stats(countries);

    HashT_delete(citizens); 
    HashT_delete(VaccineTable);
    HashT_delete(countries);
    return 0;
}
