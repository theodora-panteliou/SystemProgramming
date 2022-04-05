#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <fcntl.h>

#include <unistd.h>

#include <assert.h>
#include <dirent.h>
// #include "hash_table.h"

#include "common.h"
#include "functions.h"

/* flags for signal handling */
static volatile sig_atomic_t sig_intquit=0;
static volatile sig_atomic_t sig_usr1=0;

/* signal handling functions */
void sighandler_intquit(int signo) { sig_intquit = signo; }
void sighandler_usr1(int signo) { sig_usr1 = signo; }

/* counters for log_file */
int accepted = 0, rejected = 0;

/* function that writes to log_file.xxx */
static void make_logfile(HashT* directories);

void send_bloomfilters(int writefd, HashT* VaccineTable, int bloom_size, int buffer_size);

int main(int argc, char** argv){
    /* signal handling */
    static struct sigaction intquit={0};
    static struct sigaction usr1={0};

    intquit.sa_handler = sighandler_intquit;
    usr1.sa_handler = sighandler_usr1;

    /* Block all signals during signal handling */
    sigfillset(&(intquit.sa_mask));
    sigfillset(&(usr1.sa_mask));

    sigaction(SIGINT, &intquit, NULL);
    sigaction(SIGQUIT, &intquit, NULL);
    sigaction(SIGUSR1, &usr1, NULL);

    if (argc!= 3){
        printf("Monitor: Wrong number of arguments.\n");
        exit(1);
    }
    char *read_fifo = argv[1], *write_fifo = argv[2]; 
    // printf("Monitor %s, %s\n", read_fifo, write_fifo);

    int writefd, readfd;
    /* Open the fifo that travelMonitor reads from for writing*/
    if ( (writefd = open(read_fifo, O_WRONLY|O_TRUNC))  < 0)  {
        perror("client: can't open write fifo \n");
    }
    /* Open the fifo that travelMonitor writes to for reading*/
    if ( (readfd = open(write_fifo, O_RDONLY|O_TRUNC))  < 0)  {
        perror("client: can't open read fifo \n");
    }
    /* first message to be received is buffer size*/
    int buffer_size=0;
    if (read(readfd, &buffer_size, sizeof(int))<0){
        perror("Monitor buffer size read");
    }
    
    /* receive bloom size */
    int message_size;
    int bloom_size;
    char *bloom_size_ch; 
    receive_message(readfd, &bloom_size_ch, &message_size, buffer_size);
    bloom_size = *((int *)bloom_size_ch);
    printf("monitor %d: buf size=%d bloom size=%d\n", getpid(), buffer_size, bloom_size);
    
    /*----------------------------------Reading subdirectories' names----------------------------*/
    /* next messages are the subdirs that this monitor will handle */
    /* struct for directories */
    HashT* directories = HashT_init(3, string_delete); /* The number of directories is not known so I'm using hash table because it expands dynamically. */

    if (sig_intquit) {
        make_logfile(directories);
        sig_intquit = 0;
    }

    int count_directories=0;
    while (1){

        char *complete_message=NULL;
        receive_message(readfd, &complete_message, &message_size, buffer_size);

        if (is_end(complete_message)) {
            free(complete_message);
            break;
        } 
        else {
            count_directories++;
            char *directory = malloc(sizeof(char)*(message_size));
            strcpy(directory, complete_message);
            HashT_insert(directories, directory, directory);
            // printf("Monitor dire receive message %s, message size %d\n", complete_message, message_size);
        }
        free(complete_message);
    }

    /*---------------------------Reading files and filling structures ------------------------------*/

    /*initilize structs that will be used to store data*/
    HashT* citizens = HashT_init(1, citizenRec_delete); /*hash table which consists of citizen records*/
    HashT* VaccineTable = HashT_init(3, VirusEntry_delete); /*each entry consists of two skip lists for viruses with vaccinated citizens and unvaccinated citizens*/
    HashT* countries = HashT_init(3, string_delete); /*consists of countries. citizen->country points here*/

    /* parse the files */
    char *directory = HashT_getNextEntry(directories);

    /* hash table for read files. 
    This hash table contains all the files that are already read by the monitor so that we can quickly check if a file is new or not.
    Not sorted by country. */
    HashT* read_files = HashT_init(8, string_delete);

    struct dirent **namelist;
    int n;

    int rec_counter=0;
    while (directory!=NULL){
        n = scandir(directory, &namelist, 0, NULL);

        for (int i=0; i<n; i++){
            if (strcmp(namelist[i]->d_name,".")==0 || strcmp(namelist[i]->d_name,"..")==0){
                continue;
            }
            char* name=malloc( strlen(directory) + 1 + strlen(namelist[i]->d_name) + 1 );
            strcpy(name, directory);
            strcat(name, "/");
            strcat(name, namelist[i]->d_name);
            /* add file to read_files */
            HashT_insert(read_files, name, name);
            FILE *file_ptr;
            file_ptr = fopen(name, "r");
            if (file_ptr == NULL){
                printf("Unable to read file\n");
                return 1;
            }
            rec_counter += update_structures(file_ptr, VaccineTable, citizens, countries, bloom_size, NULL);
            /*done reading records from file*/
            fclose(file_ptr);
            file_ptr = NULL;
        }
        while (n--) {
            free(namelist[n]);
        }
        free(namelist);
        directory=HashT_getNextEntry(NULL);
    }
    // printf("records read %d\n", rec_counter);


    /*send bloom filters to travelMonitor*/
    send_bloomfilters(writefd, VaccineTable, bloom_size, buffer_size);

    /* send END */
    char message[3];
    memset(message, 0, 3);
    memcpy(message, "END", 3);
    send_message(writefd, message, 3, buffer_size);

    /*---------------------------Commands---------------------------------*/
    /* variables to read commands and/or parameters */
    char* command;
    char** parameter;
    int array_size = 9; /* 9 is an upper limit to parameters in record's file reading and queries */
    parameter = malloc(sizeof(char*)*array_size); /*array of strings*/

    VirusEntry* virus_entry;
    /* at this point monitor is ready to receive requests */
    while (1){
        // printf("Monitor: waiting for requests\n");
        char *message=NULL;
        int size;
        receive_message(readfd, &message, &size, buffer_size);
        
        if (sig_intquit) {
            make_logfile(directories);
            sig_intquit=0;
            if (message!=NULL) free(message);
            continue;
        }
        if (sig_usr1){ /* this is the only check for SIGUSR1 signal because it is sent only in this block of code from travelMonitor */
            HashT* updated_entries = HashT_init(1, NULL);
            directory = HashT_getNextEntry(directories);
            while (directory!=NULL){
                n = scandir(directory, &namelist, 0, NULL);
                for (int i=0; i<n; i++){
                    if (strcmp(namelist[i]->d_name,".")==0 || strcmp(namelist[i]->d_name,"..")==0){
                        continue;
                    }
                    
                    char* name=malloc( strlen(directory) + 1 + strlen(namelist[i]->d_name) + 1 );
                    strcpy(name, directory);
                    strcat(name, "/");
                    strcat(name, namelist[i]->d_name);

                    if (HashT_exists(read_files, name)){
                        free(name);
                        continue;
                    }
                    // printf("Found file that didn't exist\n");
                    /* add file to read_files */
                    HashT_insert(read_files, name, name);
                    
                    FILE *file_ptr;
                    file_ptr = fopen(name, "r");
                    if (file_ptr == NULL){
                        printf("Unable to read file\n");
                        return 1;
                    }
                    
                    rec_counter += update_structures(file_ptr, VaccineTable, citizens, countries, bloom_size, updated_entries);
                    /*done reading records from file*/
                    fclose(file_ptr);
                    file_ptr = NULL;
                }
                while (n--) {
                    free(namelist[n]);
                }
                free(namelist);
                directory = HashT_getNextEntry(NULL);
            }
            send_bloomfilters(writefd, updated_entries, bloom_size, buffer_size);
            HashT_delete(updated_entries);
            /* send END */
            char message3[3];
            memset(message3, 0, 3);
            memcpy(message3, "END", 3);
            send_message(writefd, message3, 3, buffer_size);
            sig_usr1 = 0;
            /* at this point pipe is empty due to the signal so reset message and continue at the begining of the loop */
            if (message!=NULL) free(message);
            continue;
        }
        
        // printf("Monitor received message %s\n", message);
        
        /* check is message is ACCEPTED or REJECTED to count */
        if (strcmp(message, "ACCEPTED") == 0){
            accepted++;
            free(message);
            continue;
        }
        else if (strcmp(message, "REJECTED") == 0){
            rejected++;
            free(message);
            continue;
        }

        /* else it is a command */
        input_read(message, &command, parameter, array_size);
        
        
        /* options */
        if (strcmp(command, "/travelRequest") == 0){  /* /travelRequest citizenID countryFrom virusName */
            /* find if citizenID is in vaccinated persons skip list for given virus name */
            virus_entry = HashT_get(VaccineTable, parameter[2]);
            if (virus_entry==NULL){ /* virus name doesn't exist so return NO */
                send_message(writefd, "NO", 2, buffer_size);
            }
            else {
                date_citizen* rec = skiplist_search(virus_entry->vaccinated_persons, parameter[0]);
                if (rec != NULL) {
                    int size = 3+1+11; /* YES+" "+date */
                    char message[size];
                    sprintf(message, "YES %s", rec->dateVaccinated);
                    // printf("Sending %s\n", message);
                    send_message(writefd, message, size, buffer_size);
                }
                else {
                    // printf("Sending %s\n", message);
                    send_message(writefd, "NO", 2, buffer_size);
                }
                
            }
        }
        else if(strcmp(command, "/searchVaccinationStatus")==0){ /* /searchVaccinationStatus citizenID */
            citizenRecord* citizen = HashT_get(citizens, parameter[0]);
            if (citizen != NULL) { /* if citizen exists send to monitor its information and all entries for vaccines */
                                    /* else don't send anything and continue */
                // printf("found %d\n", getpid());
                char age[7];
                sprintf(age, "AGE %d", citizen->age);
                int size_to_send = strlen(citizen->firstName)+1+strlen(citizen->lastName)+1+strlen(citizen->country)+1+strlen(age)+1+1;
                char* to_send = malloc(size_to_send);
                sprintf(to_send, "%s %s %s\n%s\n", citizen->firstName, citizen->lastName, citizen->country, age);

                /* find all entries */
                VirusEntry* virus_entry = HashT_getNextEntry(VaccineTable);
                skip_list* sl = NULL;
                while ( virus_entry != NULL){ /* for each entry in VaccineTable search citizenID in vaccinated_persons skip list and in non_vaccinated persons skip list */

                    sl = virus_entry->vaccinated_persons; 
                    date_citizen* rec = skiplist_search(sl, parameter[0]);
                    if (rec != NULL) {
                                                
                        int new_size = strlen(virus_entry->name)+strlen(" VACCINATED ON ")+strlen(rec->dateVaccinated)+1;
                        char new_to_send[new_size];
                        sprintf(new_to_send, "%s VACCINATED ON %s\n", virus_entry->name, rec->dateVaccinated);

                        size_to_send+=new_size;
                        to_send = (char *)realloc(to_send, size_to_send);
                        strcat(to_send, new_to_send);
                    }
                    
                    sl = virus_entry->non_vaccinated_persons; 
                    rec = skiplist_search(sl, parameter[0]);
                    if (rec != NULL){
                        int new_size = strlen(virus_entry->name)+strlen(" NOT YET VACCINATED\n");
                        char new_to_send[new_size];
                        sprintf(new_to_send, "%s NOT YET VACCINATED\n", virus_entry->name);

                        size_to_send+=new_size;
                        to_send = (char *)realloc(to_send, size_to_send);
                        strcat(to_send, new_to_send);
                    }
                    
                    virus_entry = HashT_getNextEntry(NULL);
                }

                /* send complete message */
                send_message(writefd, to_send, size_to_send, buffer_size);
                free(to_send);
            } 
        }
        free(message);
        input_init(command, parameter);
    }

    close(readfd);
    close(writefd);
    
    free(parameter);
    // printf("citizens:\n");
    // HashT_stats(citizens);
    // printf("Vaccine table:\n");
    // HashT_stats(VaccineTable);
    // printf("(countries:\n");
    // HashT_stats(countries);
    HashT_delete(read_files);

    HashT_delete(citizens); 
    HashT_delete(VaccineTable);
    HashT_delete(countries);
    /* Delete the FIFOs, now that we're done.  */
    if ( unlink(read_fifo) < 0) {
        perror("client: can't unlink \n");
    }
    if ( unlink(write_fifo) < 0) {
        perror("client: can't unlink \n");
    }
    free(bloom_size_ch);

    HashT_delete(directories);
    exit(0);
}


static void make_logfile(HashT* directories){
    FILE *file_ptr;
    char name[64];
    sprintf(name, "log_file.%d", getpid());
    file_ptr = fopen(name, "wb"); /* mode write only, create if not exists, truncate*/
    if (file_ptr == NULL){
        printf("Unable to open file\n");
    }
    char *directory = HashT_getNextEntry(directories);
    while (directory != NULL){
        char* temp = malloc(strlen(directory)+1);
        strcpy(temp, directory);
        char* token = strtok(temp, "/");
        token = strtok(NULL, "/");
        fprintf(file_ptr, "%s\n", token);
        free(temp);
        directory = HashT_getNextEntry(NULL);
    }
    fprintf(file_ptr, "TOTAL REQUESTS %d\nACCEPTED %d\nREJECTED %d\n",accepted+rejected, accepted, rejected);
    fclose(file_ptr);
}

void send_bloomfilters(int writefd, HashT* VaccineTable, int bloom_size, int buffer_size){
    /* send updated bloomfilters travelMonitor*/
    VirusEntry* virus_entry;
    virus_entry=HashT_getNextEntry(VaccineTable);
    while (virus_entry!=NULL){
        /* send both bloom filter and name of the virus */ 
        /* we know where the name ends because it is a string that ends with null
        so the rest bytes are the bloom filter */
        int bloom_bytes = bloom_size/8;
        if (bloom_size%8>0){
            bloom_bytes++;
        }
        /* bloom filter contains a pointer to an array so I nedd to serialize the data before sending it */
        char* bloom_to_send=malloc(2*sizeof(int)+bloom_bytes);
        serialize_bloomfilter(bloom_to_send, virus_entry->bloom_vaccinated_persons, bloom_bytes);

        /* first compute the size of the message to be sent. */
        int message_size=2*sizeof(int)+bloom_bytes+strlen(virus_entry->name)+1;
        char* message=malloc(message_size);
        memmove(message, virus_entry->name, strlen(virus_entry->name)+1);
        memmove( &(((char*)(message))[strlen(virus_entry->name)+1]), bloom_to_send, 2*sizeof(int)+bloom_bytes);

        free(bloom_to_send);
        send_message(writefd, message, message_size, buffer_size);
        free(message);
        virus_entry=HashT_getNextEntry(NULL);
    }
}