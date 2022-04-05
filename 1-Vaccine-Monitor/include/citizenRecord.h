#ifndef CITIZENRECORD_H
#define CITIZENRECORD_H
#include "hash_table.h"

typedef struct citizenRecord{
    char *citizenID;
    unsigned int age;
    char *firstName, *lastName, *country;
} citizenRecord;

citizenRecord* citizenRec_construnct(char** array, HashT* countries);
void citizenRec_delete(void* rec);
void citizenRec_print(citizenRecord* rec);

typedef struct date_citizen {
    char* dateVaccinated;
    citizenRecord* citizen;
} date_citizen;

void date_citizen_delete(date_citizen* d);
void date_citizen_print(date_citizen *d);

#endif /*CITIZENRECORD_H*/
