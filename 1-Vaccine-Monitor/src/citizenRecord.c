#include "citizenRecord.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

citizenRecord* citizenRec_construnct(char** array, HashT* countries){
    /*read one record and make a struct*/
    citizenRecord* rec = malloc(sizeof(citizenRecord));
    rec->citizenID = malloc(sizeof(char)*(strlen(array[0])+1));
    strcpy(rec->citizenID, array[0]);

    rec->firstName = malloc(sizeof(char)*(strlen(array[1])+1));
    strcpy(rec->firstName, array[1]);

    rec->lastName = malloc(sizeof(char)*(strlen(array[2])+1));
    strcpy(rec->lastName, array[2]);
    
    char *c;
    c = HashT_get(countries, array[3]);
    if (c == NULL){ /*if that country doesn't exist*/
        c = malloc(sizeof(char)*(strlen(array[3])+1));
        strcpy(c, array[3]);
        HashT_insert(countries, c, c);
    }
    rec->country = c;

    rec->age = atoi(array[4]);

    return rec;
}

void citizenRec_delete(void* record) {
    citizenRecord* rec = record;
    free(rec->citizenID);
    rec->citizenID = NULL;
    free(rec->firstName);
    rec->firstName = NULL;
    free(rec->lastName);
    rec->lastName = NULL;
    rec->country = NULL;
    free(rec);
    rec = NULL;
}

void citizenRec_print(citizenRecord* rec){
    printf("%s %s %s %d %s\n", rec->citizenID, rec->firstName, rec->lastName, rec->age, rec->country);
}

void date_citizen_delete(date_citizen* d) {
    free(d->dateVaccinated);
    d->dateVaccinated = NULL;
    d->citizen = NULL;
    free(d);
    d = NULL;
}

void date_citizen_print(date_citizen *d){
    printf("date %s", d->dateVaccinated);
    citizenRec_print(d->citizen);
}