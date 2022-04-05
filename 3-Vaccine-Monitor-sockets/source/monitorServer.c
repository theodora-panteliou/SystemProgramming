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

#include "common.h"
#include "functions.h"

#include <sys/wait.h>	     /* sockets */
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <netdb.h>	         /* gethostbyaddr */

#include <arpa/inet.h> /* for hton * */

#include <pthread.h>
#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))

/* counters for log_file */
int accepted = 0, rejected = 0;
/* function that writes to log_file.xxx */
static void make_logfile(char** directories, int num_directories);
void send_bloomfilters(int writefd, HashT* VaccineTable, int bloom_size, int buffer_size);

/* struct with the common variables that need to be passed in threads */
struct structures {
    HashT* VaccineTable;
    HashT* citizens;
    HashT* countries;
    int size_of_bloom;
    HashT* updated;
};

/* cyclic buffer implementation based on class examples */
/* cyclic buffer buffer */
typedef struct {
	char** data;
    int buffer_size;
	int start;
	int end;
	int count;
} buffer_t;

pthread_mutex_t mtx;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;
buffer_t buffer;

char end_flag = 0; /* this flag will be set to 1 when the main process will join the threads */

void initialize(buffer_t * buffer, int buffer_size) {
    buffer->data = malloc(sizeof(char*)*buffer_size);
    buffer->buffer_size = buffer_size;
	buffer->start = 0;
	buffer->end = -1;
	buffer->count = 0;
}

void put_to_buffer(buffer_t * buffer, char* data) {
	pthread_mutex_lock(&mtx);
	while (buffer->count >= buffer->buffer_size) { /* wait */
		// printf(">> Found Buffer Full \n");
		pthread_cond_wait(&cond_nonfull, &mtx);
	}

	buffer->end = (buffer->end + 1) % buffer->buffer_size;
	buffer->data[buffer->end] = data;
	buffer->count++;
	pthread_mutex_unlock(&mtx);
}

char* get_from_buffer(buffer_t * buffer) {
	char* data = NULL;
	pthread_mutex_lock(&mtx);
	while (buffer->count <= 0) {
		// printf("%ld >> Found Buffer Empty \n", pthread_self());
		pthread_cond_wait(&cond_nonempty, &mtx);
        // printf("%ld woke up\n", pthread_self());
        if (end_flag){ 
	        pthread_mutex_unlock(&mtx);
            return NULL;
        }
    }
	data = buffer->data[buffer->start];
	buffer->start = (buffer->start + 1) % buffer->buffer_size;
	buffer->count--;
	pthread_mutex_unlock(&mtx);
	return data;
}

void producer(char* path) {
    put_to_buffer(&buffer, path);
    // printf("producer: %s\n", path);
    pthread_cond_signal(&cond_nonempty);
    usleep(0);
}

void* consumer(void * ptr) {
    struct structures * structures = (struct structures *)ptr;
    int rec_counter =0;
	while (1) {
        char* path = get_from_buffer(&buffer);
        if (path == NULL) break;
		// printf("%ld consumer: %s %d\n", pthread_self(), path, buffer.count);
        // printf("consumer: %s\n", path);
		pthread_cond_signal(&cond_nonfull);
        /* do */
        FILE *file_ptr;
        file_ptr = fopen(path, "r");
        if (file_ptr == NULL){
            printf("%ld Unable to read file %s\n",pthread_self() , path);
            continue;
        }

        rec_counter += update_structures(file_ptr, structures->VaccineTable, structures->citizens, structures->countries, structures->size_of_bloom, structures->updated);

        /*done reading records from file*/
        fclose(file_ptr);
        file_ptr = NULL;
        /**/
		usleep(0);
    }
    printf("%ld thread exiting. Recs read: %d\n", pthread_self(), rec_counter);
	pthread_exit(0);
}

/* monitorServer -p port -t numThreads -b socketBufferSize -c cyclicBufferSize -s sizeOfBloom path1 path2 ... pathn */
int main(int argc, char** argv){
    /* Read command line arguments */
    int port, socket_buffer_size, num_threads, cyclic_buffer_size, size_of_bloom;
    for (int i = 1; i<11; i+=2){
        if (strcmp(argv[i], "-b") == 0) { 
            if (argv[i+1] == NULL){
                printf("Invalid socket buffer size.\n");
                return 1;
            }
            socket_buffer_size = atoi(argv[i+1]);
            if (socket_buffer_size <= 0) {
                printf("Invalid socket buffer size.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-c") == 0) {
            if (argv[i+1] == NULL){
                printf("Invalid cyclic buffer size.\n");
                return 1;
            }
            cyclic_buffer_size = atoi(argv[i+1]); 
            if (cyclic_buffer_size <= 0) {
                printf("Invalid cyclic buffer size.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-s") == 0) { /* bloom size in bytes */
            if (argv[i+1] == NULL){
                printf("Invalid bloom size.\n");
                return 1;
            }
            size_of_bloom = atoi(argv[i+1]); /* convert to bits */
            if (size_of_bloom <= 0) {
                printf("Invalid bloom size.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-t") == 0) { 
            if (argv[i+1] == NULL){
                printf("Invalid number of threads.\n");
                return 1;
            }
            num_threads = atoi(argv[i+1]);
            if (num_threads <= 0) {
                printf("Invalid number of threads.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-p") == 0) {
            if (argv[i+1] == NULL){
                printf("Invalid port.\n");
                return 1;
            }
            port = atoi(argv[i+1]); 
            if (port <= 0) {
                printf("Invalid port.\n");
                return 1;
            }
        }
    }
    int num_directories = argc-11;
    char** directories = malloc(sizeof(char *)*(num_directories)); //buffer
    // printf("argc %d\n", argc);
    for (int i = 11; i<argc; i++){
        directories[i-11] = malloc(strlen(argv[i])+1);
        strcpy(directories[i-11], argv[i]);
        // printf("%s\n", argv[i]);
    }
    printf("Monitor id:%d Command line arguments socketBufferSize:%d cyclicBufferSize:%d sizeOfBloom:%d numThreads:%d\n", getpid(), socket_buffer_size, cyclic_buffer_size, size_of_bloom, num_threads);

    /* Code from class example to create a server socket *****************************/
    int sock, newsock;
    struct sockaddr_in server, client;
    socklen_t clientlen = sizeof(client);

    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    /* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");
    server.sin_family = AF_INET;       /* Internet domain */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);      /* The given port */
    /* Bind socket to address */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        herror("setsockopt");

    if (bind(sock, serverptr, sizeof(server)) < 0)
        perror_exit("bind");
    /* Listen for connections */
    if (listen(sock, 1) < 0) perror_exit("listen");
    printf("Listening for connections to port %d\n", port);
    /* accept and wait for connection */
    if ((newsock = accept(sock, clientptr, &clientlen)) < 0) perror_exit("accept");
    printf("Connected\n");
    /**************************************************/

    close(sock);

    /*---------------------------Reading files and filling structures with threads ------------------------------*/

    /*initilize structs that will be used to store data*/
    HashT* citizens = HashT_init(1, citizenRec_delete); /*hash table which consists of citizen records*/
    HashT* VaccineTable = HashT_init(3, VirusEntry_delete); /*each entry consists of two skip lists for viruses with vaccinated citizens and unvaccinated citizens*/
    HashT* countries = HashT_init(3, string_delete); /*consists of countries. citizen->country points here*/

    /* Create threads */
    /* keep thread ids in an array */
    pthread_t *tids;
    if ((tids = malloc(num_threads * sizeof(pthread_t))) == NULL) {
        perror_exit("malloc");
    }
    	
	initialize(&buffer, cyclic_buffer_size);
	pthread_mutex_init(&mtx, 0);

    struct structures* structures = malloc(sizeof(struct structures));
    structures->VaccineTable = VaccineTable;
    structures->citizens = citizens;
    structures->countries = countries;
    structures->size_of_bloom = size_of_bloom;
    structures->updated = NULL;
    
    int err;
    for (int i=0 ; i<num_threads ; i++) {
        if ((err = pthread_create(tids+i, NULL, consumer, structures))) {
            /* Create a thread */
            perror2("pthread_create", err); 
            exit(1); 
        } 
    }

    /* hash table for read files. 
    This hash table contains all the files that are already read by the monitor so that we can quickly check if a file is new or not.
    Not sorted by country. */
    HashT* read_files = HashT_init(8, string_delete);

    struct dirent **namelist;
    int n;

    for (int j=0; j<num_directories; j++){
        n = scandir(directories[j], &namelist, 0, NULL);
        for (int i=0; i<n; i++){
            if (strcmp(namelist[i]->d_name,".")==0 || strcmp(namelist[i]->d_name,"..")==0){
                continue;
            }
            // printf("%s %s\n", directories[j], namelist[i]->d_name);
            char* path=malloc( strlen(directories[j]) + 1 + strlen(namelist[i]->d_name) + 1 );
            strcpy(path, directories[j]);
            strcat(path, "/");
            strcat(path, namelist[i]->d_name);
            /* add file to read_files */
            HashT_insert(read_files, path, path);
            /* put new file to buffer */
            producer(path);
        }
        while (n--) {
            free(namelist[n]);
        }
        free(namelist);
    }
    // printf("records read %d\n", rec_counter);


    /*send bloom filters to travelMonitor*/
    send_bloomfilters(newsock, VaccineTable, size_of_bloom, socket_buffer_size);

    /* send END */
    char message[3];
    memset(message, 0, 3);
    memcpy(message, "END", 3);
    send_message(newsock, message, 3, socket_buffer_size);

    
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
        receive_message(newsock, &message, &size, socket_buffer_size);
        
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
                send_message(newsock, "NO", 2, socket_buffer_size);
            }
            else {
                date_citizen* rec = skiplist_search(virus_entry->vaccinated_persons, parameter[0]);
                if (rec != NULL) {
                    int size = 3+1+11; /* YES+" "+date */
                    char message[size];
                    sprintf(message, "YES %s", rec->dateVaccinated);
                    // printf("Sending %s\n", message);
                    send_message(newsock, message, size, socket_buffer_size);
                }
                else {
                    // printf("Sending %s\n", message);
                    send_message(newsock, "NO", 2, socket_buffer_size);
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
                send_message(newsock, to_send, size_to_send, socket_buffer_size);
                free(to_send);
            } 
        }
        else if(strcmp(command, "/addVaccinationRecords")==0){ /* /addVaccinationRecords */
            HashT* updated_entries = HashT_init(1, NULL);
            structures->updated = updated_entries;
            for (int j=0; j<num_directories; j++){
                n = scandir(directories[j], &namelist, 0, NULL);
                for (int i=0; i<n; i++){
                    if (strcmp(namelist[i]->d_name,".")==0 || strcmp(namelist[i]->d_name,"..")==0){
                        continue;
                    }
                    
                    char* path=malloc( strlen(directories[j]) + 1 + strlen(namelist[i]->d_name) + 1 );
                    strcpy(path, directories[j]);
                    strcat(path, "/");
                    strcat(path, namelist[i]->d_name);

                    if (HashT_exists(read_files, path)){
                        free(path);
                        continue;
                    }
                    // printf("Found file that didn't exist\n");
                    /* add file to read_files */
                    HashT_insert(read_files, path, path);
                    
                    /* put new file to buffer */
                    producer(path);
                }
                while (n--) {
                    free(namelist[n]);
                }
                free(namelist);
            }
            send_bloomfilters(newsock, updated_entries, size_of_bloom, socket_buffer_size);
            HashT_delete(updated_entries);
            /* send END */
            char message3[3];
            memset(message3, 0, 3);
            memcpy(message3, "END", 3);
            send_message(newsock, message3, 3, socket_buffer_size);
        }
        else if(strcmp(command, "/exit")==0){ /* /exit */
            free(message);
            input_init(command, parameter);
            break;
        }
        free(message);
        input_init(command, parameter);
    }
        
    free(parameter);

    /*---------------------------------Exiting -------------------------------------*/
    /* terminating threads */
    // while (buffer.count > 0) sleep(1);
    end_flag = 1;
    pthread_cond_broadcast(&cond_nonempty);

    for (int i=0 ; i<num_threads ; i++) {
        if ((err = pthread_join(*(tids+i), NULL))) {
            /* Wait for thread termination */
            perror2("pthread_join", err); 
            exit(1); 
        }
    }
    printf("monitorServer: %d all %d threads have terminated\n", getpid(), num_threads);
	pthread_cond_destroy(&cond_nonempty);
	pthread_cond_destroy(&cond_nonfull);
	pthread_mutex_destroy(&mtx);
    free(tids);
    /**/

    make_logfile(directories, num_directories);

    for (int i=0; i<argc-11; i++){
        free(directories[i]);
    }
    free(directories);
    close(newsock);

    HashT_delete(read_files);

    HashT_delete(citizens); 
    HashT_delete(VaccineTable);
    HashT_delete(countries);

    free(structures);

    free(buffer.data);
    return 0;
}

extern pthread_mutex_t vaccinetable_mtx; /* This mutex is defined in functions.c and it is used for access in the vaccine table.
                                            Here it is used in send_bloomfilters to safely access the vaccine table. */

void send_bloomfilters(int writefd, HashT* VaccineTable, int bloom_size, int buffer_size){
    /* send updated bloomfilters travelMonitor*/
    pthread_mutex_lock(&vaccinetable_mtx);
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
    pthread_mutex_unlock(&vaccinetable_mtx);
}

static void make_logfile(char** directories, int num_directories){
    FILE *file_ptr;
    char name[64];
    sprintf(name, "log_file.%d", getpid());
    file_ptr = fopen(name, "wb"); /* mode write only, create if not exists, truncate*/
    if (file_ptr == NULL){
        printf("Unable to open file\n");
    }
    for (int i=0; i<num_directories; i++){
        char* temp = malloc(strlen(directories[i])+1);
        strcpy(temp, directories[i]);
        char* token = strtok(temp, "/");
        token = strtok(NULL, "/");
        fprintf(file_ptr, "%s\n", token);
        free(temp);
    }
    fprintf(file_ptr, "TOTAL REQUESTS %d\nACCEPTED %d\nREJECTED %d\n",accepted+rejected, accepted, rejected);
    fclose(file_ptr);
}
