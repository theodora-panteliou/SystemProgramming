#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/select.h>
#include <signal.h>

#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <netdb.h>	         /* gethostbyaddr */

#include <assert.h>

#include <netinet/in.h> /* temp */
#include <arpa/inet.h>

#include "functions.h"
#include "common.h"
#include "stats.h"
#include "simple_list.h"

#define PATH_BUFSIZE 128

/*color*/
#define reset "\e[0m"
#define MAG "\e[0;35m"

/* counters for log_file */
static int accepted = 0, rejected = 0;

/* function that writes to log_file.xxx */
static void make_logfile(struct dirent** namelist, int n);

/* ./travelMonitorClient –m numMonitors -b socketBufferSize -c cyclicBufferSize -s sizeOfBloom -i input_dir -t numThreads */
int main(int argc, char *argv[]) {
    /*------------------------- command line input ------------------------*/
    char* input_dir = NULL;
    int size_of_bloom, num_monitors, cyclic_buffer_size, socket_buffer_size, num_threads;
    if (argc!=13){
        printf("Wrong number of arguments.\n");
        exit(0);
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-m") == 0) { /* number of monitors */
            if (argv[i+1] == NULL){
                printf("Invalid number of monitors.\n");
                return 1;
            }
            num_monitors = atoi(argv[i+1]);
            if (num_monitors <= 0) {
                printf("Invalid number of monitors.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-b") == 0) { /* socket buffer size */
            if (argv[i+1] == NULL){
                printf("Invalid socket buffer size.\n");
                return 1;
            }
            socket_buffer_size = atoi(argv[i+1]);
            if (socket_buffer_size <= 0) {
                printf("Invalid socket buffer size.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-c") == 0) { /* cyclic buffer size */
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
            size_of_bloom = atoi(argv[i+1])*8; /* convert to bits */
            if (size_of_bloom <= 0) {
                printf("Invalid bloom size.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-i") == 0) { /* input directory */
            if (argv[i+1] == NULL){
                printf("Invalid directory name.\n");
                return 1;
            }
            input_dir = argv[i+1];
        } else if (strcmp(argv[i], "-t") == 0) { /* number of threads for each monitor */
            if (argv[i+1] == NULL){
                printf("Invalid number of threads.\n");
                return 1;
            }
            num_threads = atoi(argv[i+1]);
            if (num_threads <= 0) {
                printf("Invalid number of threads.\n");
                return 1;
            }
        }
    }
    // printf("travelMonitor id:%d Command line arguments numMonitors:%d socketBufferSize:%d cyclicBufferSize:%d sizeOfBloom:%d input_dir:%s numThreads:%d\n", getpid(), num_monitors, socket_buffer_size, cyclic_buffer_size, size_of_bloom, input_dir, num_threads);

    /*------------------- Reading input_dir -----------------------*/
    struct dirent **namelist;
    int num_subdirs;
    num_subdirs = scandir(input_dir, &namelist, 0, alphasort);
    if (num_subdirs < 0) {
        perror("scandir");
        exit(1);
    }
    
    /* ignore .. and . subdirectories */
    struct dirent **old_namelist;
    old_namelist = namelist;
    namelist+=2;
    
    /*------------------- Creating children ----------------------------*/
    if (num_monitors>num_subdirs-2){ /* if monitors are less than subdirs (excluding . and ..) avoid creating inactive monitors */
        num_monitors=num_subdirs-2;
        printf("There are only %d subdirectories. To not have inactive monitors only %d monitor will be created\n", num_subdirs-2, num_monitors);
    }

    pid_t pid;
    /* for each monitor create a socket */
    /* then fork exec a monitor*/
    int* monitor_fd;
    monitor_fd = malloc(sizeof(int)*num_monitors); 
    
    int port=7000; 

    int num_paths = (num_subdirs-2)/num_monitors; /* calculate minimum number of paths for every child/monitor */
    int res = (num_subdirs-2)%num_monitors; /* res is the number of monitors that will have num_paths+1 paths */
    
    for (int i=0; i<num_monitors; i++){
        
        /* find paths for the files that will be sent through the exec call */
        int i_num_paths = num_paths; /* num of paths for i monitor */
        if (i<res) i_num_paths++; 

        char** paths;
        paths = malloc(i_num_paths*sizeof(char*)); /* declare an array of i_num_path strings */

        int pos = 0;
        for (int j=i; j<num_subdirs-2; j+=num_monitors){ /* choose subdirectory names for i monitor using round robin */
            paths[pos] = malloc(strlen(input_dir)+1+strlen(namelist[j]->d_name)+1);
            strcpy(paths[pos], input_dir);
            strcat(paths[pos], "/");
            strcat(paths[pos], namelist[j]->d_name);
            pos++;  
        }
        // printf("i:%d i_num_paths:%d res:%d num subdirs:%d\n", i, i_num_paths, res, num_subdirs);
        
        /* format of the exec call is: 
        monitorServer -p port -t numThreads -b socketBufferSize -c cyclicBufferSize -s sizeOfBloom path1 path2 ... pathn */
        char** arguments;
        arguments=malloc((11+i_num_paths+1)*sizeof(char*));
        int k=0;
        arguments[k] = "monitorServer"; /* name of executable */

        arguments[++k] = "-p";
        arguments[++k] = malloc(8);
        sprintf(arguments[k], "%d", port+i); /* port number */

        arguments[++k] = "-t";
        arguments[++k] = malloc(8);
        sprintf(arguments[k], "%d", num_threads); /* num threads */

        arguments[++k] = "-b";
        arguments[++k] = malloc(8);
        sprintf(arguments[k], "%d", socket_buffer_size); /* socket buffer size */

        arguments[++k] = "-c";
        arguments[++k] = malloc(8);
        sprintf(arguments[k], "%d", cyclic_buffer_size); /* cyclic buffer size */

        arguments[++k] = "-s";
        arguments[++k] = malloc(8);
        sprintf(arguments[k], "%d", size_of_bloom); /* bloom size */
        k++;
        for (int j=0; j<i_num_paths; j++){ /* paths to countries' subdirectories */
            arguments[j+k] = paths[j];
        }
        arguments[k+i_num_paths] = NULL; /* last pointer must be NULL according to execv */
        // printf("k=%d, i_num_paths=%d\n", k, i_num_paths);

        /* start a child process */
        int dir_len;
        
        char path[PATH_BUFSIZE];
        getcwd(path, PATH_BUFSIZE);
        dir_len = strlen(path);

        strcpy(path + dir_len, "/build/monitorServer");
        pid = fork();
        if (pid < 0) {
            perror("fork");
        }
        else if (pid == 0) {
            /* We're in the child process */
            execv(path, arguments);
            perror("execv");
            _exit(127);
        }
        
        /* only the parent process reaches this */

        /* Create socket */
        int sock;
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            perror_exit("socket");
        /* Connect */

        /* From class examples ******************************************/
        /* Find server address */
        struct sockaddr_in server;
        struct sockaddr *serverptr = (struct sockaddr*)&server;
        struct hostent *rem;
        char hostname[50];
        gethostname(hostname, 50);
        if ((rem = gethostbyname(hostname)) == NULL) {	
            herror("gethostbyname"); exit(1);
        }
        char  symbolicip[50];

        struct  in_addr **addr_list = (struct in_addr **) rem->h_addr_list;
        // for(i = 0; addr_list[i] != NULL; i++) {
            strcpy(symbolicip , inet_ntoa(*addr_list[0]) );
            printf("%s resolved to %s \n",rem->h_name,symbolicip);
        // }
        /*Convert port number to integer*/
        server.sin_family = AF_INET;       /* Internet domain */
        memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
        server.sin_port = htons(port+i);         /* Server port */
        /* Initiate connection */
        while (connect(sock, serverptr, sizeof(server)) < 0);
        printf("Connecting to %s port %d\n", hostname, port+i);
        /********************************************************/

        monitor_fd[i] = sock;

        for (int j=0; j<pos; j++){
            free(paths[j]);
        }
        free(paths);
        for (int j=2; j<k; j+=2){
            free(arguments[j]);
        }
        free(arguments);
    }
   
    /*-------------------------Reading bloomfilters from children-----------------------*/

    /* In this hash table there will be inserted bloom filters with key the virus name they refer to.
    If a new bloom filter for an existing key is going to be inserted then it will be added to the 
    previous one with bitwise OR, thanks to the structure of the bloom filters */
    HashT* bloom_filters = HashT_init(4, bloom_delete);
    HashT* viruses = HashT_init(4, string_delete);
    int counter=0;

    fd_set fds;
    /* this loop stops when it receives num_monitor ENDs */
    /* each monitor when it's done sending bloom filters it sends END */
    int end_number=0;
    while (1){
        counter++;

        FD_ZERO(&fds); /* reset fd_set for select */
        
        for (int i=0; i<num_monitors; i++){
            FD_SET(monitor_fd[i], &fds);
        }
        int maxfd = monitor_fd[num_monitors-1];

        /* I am using select() call to read whichever bloom filter comes first */
        int retval = select(maxfd+1, &fds, NULL, NULL, NULL);
        if (retval == -1){
            perror("select()");
        }
        else if (retval){

            /* find ready socket */
            for (int i=0; i<num_monitors; i++){
                if (FD_ISSET(monitor_fd[i], &fds)) { 
                    /* Found ready fifo */
                    char* complete_message;
                    int message_size;
                    receive_message(monitor_fd[i], &complete_message, &message_size, socket_buffer_size);
                    
                    if (is_end(complete_message)) {
                        end_number++;
                        free(complete_message);
                        if (end_number == num_monitors){ /* If we received END from every monitor break the loop */
                            break;
                        }
                        continue;
                    }

                    /* now that we got the complete message from monitor we have to extract the name and the rest of it is the bloom filter */
                    /* the name stops with null so the strlen(message)+1 will be the name and the rest the bloom filter */
                    int size_of_str = strlen(complete_message)+1;
                    char *virus_name=malloc(size_of_str);
                    memcpy(virus_name, complete_message, size_of_str);
                    // printf("complete message size %d virus name %s, %d\n",  size_of_str, virus_name, message_size- size_of_str);
                    // printf("bloom size for deserialize %ld\n", message_size-size_of_str-2*sizeof(int));
                    bloom_filter* bloom=deserialize(complete_message+size_of_str, message_size-size_of_str-2*sizeof(int));

                    free(complete_message);
                    bloom_filter* temp_bloom = HashT_get(bloom_filters, virus_name);
                    if (temp_bloom == NULL){ /* if bloom filter for that virus doesn't exist insert it */
                        HashT_insert(viruses, virus_name, virus_name);
                        HashT_insert(bloom_filters, virus_name, bloom);
                    }
                    else {
                        bloom_add(temp_bloom, bloom);
                        bloom_delete(bloom);
                        free(virus_name);
                    }

                }
            }
            if (end_number == num_monitors){
                break;
            }
        }
        // printf("counter %d\n", counter);
    }

    /*--------------------------- Commands -------------------------------*/

    /* reaching here we have received a message from every monitor (END) that indicates that a monitor is done and ready to accept requests */
    /* now travelMonitor is ready to accept commands */

    /* variables to read commands and/or parameters */
    char* command;
    char** parameter;
    int array_size = 6; /* is an upper limit to parameters in commands */
    parameter = malloc(sizeof(char*)*array_size); /*array of strings*/

    /* buffer for input */
    size_t linesize = 110;
    char* line;
    line = malloc(linesize*sizeof(char));

    init_stats(); /* start count stats */

    while (1){
        printf(MAG"\ninput:\n"reset);
        if (fgets(line, linesize, stdin) == NULL){
            continue;
        }
        // printf("%s", line);
        /**/
        int par_count = input_read(line, &command, parameter, array_size);
        if (command == NULL){
            printf("Invalid input\n");
        }
        /*cases*/
        else if (strcmp(command, "/exit") == 0){ /* /exit */
            /* send exit to all monitors and break */
            for (int i=0; i<num_monitors; i++){
                send_message(monitor_fd[i], command, strlen(command)+1, socket_buffer_size);
            }
            input_init(command, parameter);
            break;
        }
        else if (strcmp(command, "/travelRequest")==0){ /* /travelRequest citizenID date countryFrom countryTo virusName */
            /*  */
            if (par_count != 5 || !is_date(parameter[1]) || !is_name(parameter[2]) || !is_name(parameter[3]) || !is_virusname(parameter[4])){
                printf("Syntax is /travelRequest citizenID date countryFrom countryTo virusName. Try again.\n");
                input_init(command, parameter);
                continue;
            }

            /* find countryFrom and countryTo */
            int countryTo_monitor = -1, countryFrom_monitor = -1;
            for (int i=0; i<num_subdirs-2; i++){
                if (strcmp(namelist[i]->d_name, parameter[2])==0)
                    countryFrom_monitor = i%num_monitors;
                else if (strcmp(namelist[i]->d_name, parameter[3])==0)
                    countryTo_monitor = i%num_monitors;
            }

            if (countryTo_monitor == -1 || countryFrom_monitor == -1){
                printf("CountryTo and/or countryFrom doesn't exist.\n");
            }
            else {
                /*checking bloom filter*/
                bloom_filter* bf = HashT_get(bloom_filters, parameter[4]); /*find bloom filter with key "virusName"*/
                if (bf != NULL){
                    bool accepted_flag;
                    if (bloom_exists(bf, parameter[0]) == false) { /*check if "citizenID" exists in bloom filter*/
                        printf("REQUEST REJECTED – YOU ARE NOT VACCINATED\n");
                        accepted_flag = false;
                    }
                    else {
                                
                        /* send a message that contains only the command, citizenID, countryFrom and virusName to the countryFrom monitor */
                        int size=strlen(command)+1+strlen(parameter[0])+1+strlen(parameter[2])+1+strlen(parameter[4])+1;
                        char message[size];
                        strcpy(message, command); strcat(message, " ");
                        strcat(message, parameter[0]); strcat(message, " ");
                        strcat(message, parameter[2]); strcat(message, " ");
                        strcat(message, parameter[4]);

                        send_message(monitor_fd[countryFrom_monitor], message, size, socket_buffer_size);
                        
                        /* receive answer */
                        char* answer;
                        int size_answer;
                        receive_message(monitor_fd[countryFrom_monitor], &answer, &size_answer, socket_buffer_size);
                        // printf("travel monitor received answer %s, size %d\n", answer, size_answer);

                        if (strncmp(answer, "NO", 2) == 0){
                            printf("REQUEST REJECTED – YOU ARE NOT VACCINATED\n");
                            accepted_flag = false;
                        }
                        else if (strncmp(answer, "YES", 3) == 0){
                            char *temp_answer=malloc(strlen(answer)+1); /* copy answer for strtok */
                            strcpy(temp_answer, answer);

                            /* get date */
                            strtok(temp_answer, " ");
                            char* date;
                            date = strtok(NULL, " ");

                            char date_six_months_before[11];
                            get_date_six_months_before(date_six_months_before, parameter[1]);

                            if (date_between(date, date_six_months_before, parameter[1])){
                                printf("REQUEST ACCEPTED – HAPPY TRAVELS\n");
                                accepted_flag = true;
                            }
                            else {
                                printf("REQUEST REJECTED – YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n");
                                accepted_flag = false;
                            }

                            free(temp_answer);
                        }
                        else {
                            fprintf(stdout, "Wrong message from pipe. Expected 'YES date' or 'NO'\n"); /* should never get here */
                            exit(1);
                        }
                        free(answer);
                    }   

                    update_stats(accepted_flag, parameter[1], parameter[3], parameter[4]);


                    /* send ACCEPTED or REJECTED to monitor to count requests */
                    if (accepted_flag)
                        send_message(monitor_fd[countryTo_monitor], "ACCEPTED", strlen("ACCEPTED")+1,  socket_buffer_size);
                    else
                        send_message(monitor_fd[countryTo_monitor], "REJECTED", strlen("REJECTED")+1,  socket_buffer_size);


                    /* update travelMonitor's counters */
                    if (accepted_flag)
                        accepted++;
                    else
                        rejected++;
                }
                else{
                    printf("Virus doesn't exist.\n");
                }
            }
        }
        else if(strcmp(command, "/travelStats")==0){ /* /travelStats virusName date1 date2 [country] */

            if ((par_count!=3 && par_count!=4) || !is_virusname(parameter[0]) || !is_date(parameter[1]) || !is_date(parameter[2]) || (par_count==4 && !is_name(parameter[3])) ){
                printf("Syntax is /travelStats virusName date1 date2 [country]. Try again.\n");
            } 
            else {

                print_stats(parameter[0], parameter[1], parameter[2], parameter[3]);

            }
        }
        else if(strcmp(command, "/addVaccinationRecords")==0){ /* /addVaccinationRecords country */

            if (par_count!=1 || !is_name(parameter[0])){
                printf("Syntax is /addVaccinationRecords country. Try again.\n");
            } 
            else {
                /* inform only the monitor that has that country through signal SIGUSR1 */
                for (int i=0; i<num_subdirs; i++){
                    if (strcmp(namelist[i]->d_name, parameter[0])==0){
                        /* send command /addVaccinationRecords to the monitor that has the country */
                        send_message(monitor_fd[i%num_monitors], command, strlen(command)+1, socket_buffer_size);
                        /* receive updated bloom filters */
                        int count_loops=0;
                        while (1){
                            count_loops++;
                            char* complete_message;
                            int message_size;
                            receive_message(monitor_fd[i%num_monitors], &complete_message, &message_size, socket_buffer_size);

                            if (is_end(complete_message)) {
                                free(complete_message);
                                break;
                            }
                            /* now that we got the complete message from monitor we have to extract the name and the rest of it is the bloom filter */
                            /* the name stops with null so the strlen(message)+1 will be the name and the rest the bloom filter */
                            int size_of_str = strlen(complete_message)+1;
                            char *virus_name=malloc(size_of_str);
                            memcpy(virus_name, complete_message, size_of_str);
                            // printf("complete message size %d virus name %s, %d\n",  size_of_str, virus_name, message_size- size_of_str);
                            bloom_filter* bloom=deserialize(complete_message+size_of_str, message_size-size_of_str-2*sizeof(int));
                            
                            free(complete_message);
                            bloom_filter* temp_bloom = HashT_get(bloom_filters, virus_name);
                            if (temp_bloom == NULL){ /* if bloom filter for that virus doesn't exist insert it */
                                HashT_insert(viruses, virus_name, virus_name);
                                HashT_insert(bloom_filters, virus_name, bloom);
                            }
                            else {
                                bloom_add(temp_bloom, bloom);
                                bloom_delete(bloom);
                                free(virus_name);
                            }
                        }
                        // printf("received %d bloom filters\n", count_loops-1);
                        break;
                    }
                }
            }
        }
        else if(strcmp(command, "/searchVaccinationStatus")==0){ /* /searchVaccinationStatus citizenID */
            if (par_count!=1){
                printf("Syntax is /searchVaccinationStatus citizenID. Try again.\n");
            } 
            else {
                /* forward the command to all monitors. Only the monitor that has that citizenID will answer */
                for (int i=0; i<num_monitors; i++){
                    char message[strlen(command)+1+strlen(parameter[0])+1];
                    strcpy(message, command);
                    strcat(message, " ");
                    strcat(message, parameter[0]);
                    // printf("travelMonitor sending message: %s\n", message);
                    send_message(monitor_fd[i], message, strlen(command)+1+strlen(parameter[0])+1, socket_buffer_size);
                }
                struct timeval tv;
                tv.tv_usec = 0;
                tv.tv_sec = 5;
                /* prepare for select */
                fd_set fds;
                FD_ZERO(&fds); /* init fd_set */
                for (int i=0; i<num_monitors; i++){
                    FD_SET(monitor_fd[i], &fds);
                }
                int maxfd = monitor_fd[num_monitors-1];

                /* I am using select() call to read whichever bloom filter comes first */
                int retval = select(maxfd+1, &fds, NULL, NULL, &tv);
                if (retval == -1){
                    perror("select()");
                }
                else if (retval){
                    for (int i=0; i<num_monitors; i++){
                        if (FD_ISSET(monitor_fd[i], &fds)) { /* find which socket is ready */
                            /* read and print answer */
                            char *complete_message;
                            int size;
                            receive_message(monitor_fd[i], &complete_message, &size, socket_buffer_size);
                            printf("%s %s", parameter[0], complete_message);
                            free(complete_message);
                        }
                    }
                }
            }           
        }
        else {
            printf("Invalid command. Try again.\n");
        }
        input_init(command, parameter);
    }

    destroy_stats();
    free(line);
    free(parameter);

    /*---------------Exiting---------------------*/
    make_logfile(namelist, num_subdirs-2);
    int status;
	for (;;) {
		pid = wait(&status);
		if (pid < 0) {
			if (errno == ECHILD) {
				printf("All children have exited\n");
				break;
			}
			else {
				perror("Could not wait");
			}
			}
		else {
			printf("Child %d exited with status %d\n", pid, status);
		}
	}
    for (int i=0; i<num_monitors; i++){
        close(monitor_fd[i]);
    }
    /* free list of subdirs */
    while (num_subdirs--) {
        // printf("%s\n", old_namelist[num_subdirs]->d_name);
        free(old_namelist[num_subdirs]);
    }
    free(old_namelist);

    HashT_delete(bloom_filters);
    HashT_delete(viruses);
    
    free(monitor_fd);
    return 0;
}

static void make_logfile(struct dirent** namelist, int n){
    FILE *file_ptr;
    char name[64];
    sprintf(name, "log_file.%d", getpid());
    file_ptr = fopen(name, "wb"); /* mode write only, create if not exists, truncate*/
    if (file_ptr == NULL){
        printf("Unable to open file\n");
    }
    for (int i=0; i<n; i++){
        fprintf(file_ptr, "%s\n", namelist[i]->d_name);
    }
    fprintf(file_ptr, "TOTAL REQUESTS %d\nACCEPTED %d\nREJECTED %d\n",accepted+rejected, accepted, rejected);
    fclose(file_ptr);
}
