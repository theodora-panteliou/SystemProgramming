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

#include <assert.h>

#include "functions.h"
#include "common.h"
#include "stats.h"
#include "simple_list.h"

#define PATH_BUFSIZE 128

#define READ 0
#define WRITE 1

#define PERMS 0666
#define PATH "/tmp/"

/*color*/
#define HMAG "\e[0;95m"
#define reset "\e[0m"
#define MAG "\e[0;35m"

/* flags for signal handling */
static volatile sig_atomic_t sig_intquit;
static volatile sig_atomic_t sig_usr1;
static volatile sig_atomic_t sig_chld;

/* signal handling functions */
void sighandler_intquit(int signo) { sig_intquit = signo; }
void sighandler_usr1(int signo) { sig_usr1 = signo; }
void sighandler_chld(int signo) { sig_chld = signo; }

/* counters for log_file */
static int accepted = 0, rejected = 0;

/* function that writes to log_file.xxx */
static void make_logfile(struct dirent** namelist, int n);

void replace_monitor(pid_t dead_pid, int num_monitors, pid_t* children_pids, int** monitor_fd, int buffer_size,
 int size_of_bloom, int num_subdirs, struct dirent** namelist, char* input_dir, HashT* viruses, HashT* bloom_filters);

/* ./travelMonitor –m numMonitors -b bufferSize -s sizeOfBloom -i input_dir */
int main(int argc, char *argv[]) {
    /* -------------------------- signal handling ---------------------------*/
    static struct sigaction intquit={0}, usr1={0}, chld={0};

    intquit.sa_handler = sighandler_intquit;
    usr1.sa_handler = sighandler_usr1;
    chld.sa_handler = sighandler_chld;
    
    /* Block all signals during signal handling */
    sigfillset(&(intquit.sa_mask));
    sigfillset(&(usr1.sa_mask));
    sigfillset(&(chld.sa_mask));

    sigaction(SIGINT, &intquit, NULL);
    sigaction(SIGQUIT, &intquit, NULL);
    sigaction(SIGUSR1, &usr1, NULL);
    sigaction(SIGCHLD, &chld, NULL);

    /*------------------------- command line input ------------------------*/
    char* input_dir = NULL;
    int size_of_bloom, num_monitors, buffer_size;
    if (argc!=9){
        printf("Wrong number of arguments.\n");
        exit(0);
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-m") == 0) { 
            if (argv[i+1] == NULL){
                printf("Invalid number of monitors.\n");
                return 1;
            }
            num_monitors = atoi(argv[i+1]);
            if (num_monitors <= 0) {
                printf("Invalid number of monitors.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-b") == 0) { 
            if (argv[i+1] == NULL){
                printf("Invalid size of buffer.\n");
                return 1;
            }
            buffer_size = atoi(argv[i+1]);
            if (buffer_size <= 0) {
                printf("Invalid size of buffer.\n");
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
        }
    }
    printf("travelMonitor id:%d Command line arguments num monitors:%d, bloom size to bits:%d, input_dir:%s, buffer_size:%d\n", getpid(), num_monitors, size_of_bloom, input_dir, buffer_size);

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

    pid_t children_pids[num_monitors];
    pid_t pid;
    /* for each monitor create a pair of named pipes, one write and one read*/
    /* then fork exec a monitor and write through pipe the buffer size */
    int** monitor_fd;
    monitor_fd = malloc(sizeof(int*)*2);
    for(int i=0; i<2; i++)
        monitor_fd[i] = malloc(sizeof(int)*num_monitors);
    char read_fifo[64], write_fifo[64]; 

    for (int i=0; i<num_monitors; i++){
        /* make names for fifos and fifos */
        sprintf(read_fifo, "%s%s%d", PATH, "fifo_read", i); 
        sprintf(write_fifo, "%s%s%d", PATH, "fifo_write", i);
        // printf("travelMonitor %s, %s\n", read_fifo, write_fifo);
        if ((mkfifo(read_fifo, PERMS) < 0) && (errno != EEXIST)){
            perror("can't create fifo");
        }
        if ((mkfifo(write_fifo, PERMS) < 0) && (errno != EEXIST)){
            unlink(read_fifo);
            perror("can't create fifo");
        }
        
        /* start a child process */
        int dir_len;
        
        char path[PATH_BUFSIZE];
        getcwd(path, PATH_BUFSIZE);
        dir_len = strlen(path);

        strcpy(path + dir_len, "/build/monitor");
        pid = fork();
        if (pid < 0) {
            perror("fork");
        }
        else if (pid == 0) {
            /* We're in the child process, exec monitor */
            execl(path, "monitor", read_fifo, write_fifo, (char*)0);
            perror("execl");
            _exit(127);
        }
        
        /* only the parent process reaches this */
        children_pids[i] = pid;
        if ( (monitor_fd[READ][i] = open(read_fifo, O_RDONLY|O_TRUNC)) < 0){
            perror("travelMonitor: can't open read fifo");
        }
        
        if ( (monitor_fd[WRITE][i] = open(write_fifo, O_WRONLY|O_TRUNC)) < 0){
            perror("travelMonitor: can't open write fifo");
        }

        /* first message to each monitor is buffer size and it is sizeof(int) */
        if (write(monitor_fd[WRITE][i], &buffer_size, sizeof(int))!=sizeof(int)){
            perror("Monitor write buff size");
            exit(0);
        }
        /* then send bloom size */
        send_message(monitor_fd[WRITE][i%num_monitors], (char *)(&size_of_bloom), sizeof(int), buffer_size);
    }
    /* Children are made and have received buff size and bloomsize */

    /*--------------------------Sending directories to monitors-----------------------*/
    
    /* parse the list of names and send one name to each monitor at a time (Round Robin) */
    /* the funtion I will be using to match a directory name with a monitor is { number_in_namelist_for_subdirs%num_monitors = monitor_that_controls_that_subdir } */
    int loop_counter = 0;
    for (int i=0; i<num_subdirs-2; i++){ /* for each subdir */
        int message_size=strlen(input_dir)+strlen(namelist[i]->d_name)+2;
        char message[strlen(input_dir)+strlen(namelist[i]->d_name)+2];
        strcpy(message, input_dir);
        strcat(message, "/");
        strcat(message, namelist[i]->d_name);
        loop_counter++;
        send_message(monitor_fd[WRITE][i%num_monitors], message, message_size, buffer_size);
    }
    // printf("loops %d\n", loop_counter);

    /* send END so the monitors are done waiting for directories */
    for (int i=0; i<num_monitors; i++){
        char message[3];
        memset(message, 0, sizeof(message));
        memcpy(message, "END", 3);
        send_message(monitor_fd[WRITE][i], message, 3, buffer_size);
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
            FD_SET(monitor_fd[READ][i], &fds);
        }
        int maxfd = monitor_fd[READ][num_monitors-1];

        /* I am using select() call to read whichever bloom filter comes first */
        int retval = select(maxfd+1, &fds, NULL, NULL, NULL);
        if (retval == -1){
            perror("select()");
        }
        else if (retval){

            /* find which named pipe is ready */
            for (int i=0; i<num_monitors; i++){
                if (FD_ISSET(monitor_fd[READ][i], &fds)) { 
                    /* Found ready fifo */
                    char* complete_message;
                    int message_size;
                    receive_message(monitor_fd[READ][i], &complete_message, &message_size, buffer_size);
                    
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
                    bloom_filter* bloom=deserialize(complete_message+size_of_str, message_size-size_of_str-2*sizeof(int));
                    // bloom_print(bloom);
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
        /* check for signals */
        if (sig_intquit) break;
        if (sig_chld) {
            pid_t pid;
            int status;
            for (;;) { /* this catches many dead children */
                pid = waitpid(-1, &status, WNOHANG);
                if (pid <= 0) {
                    break;
                }
                else {
                    printf("Child %d exited with status %d\n", pid, status);
                    replace_monitor(pid, num_monitors, children_pids, monitor_fd, buffer_size, size_of_bloom, num_subdirs, namelist, input_dir, viruses, bloom_filters);
                }
            }
            sig_chld=0;
            continue;
        }
        
        fflush(stdin);
        printf(MAG"\ninput:\n"reset); /*\e[38;5;214m*/
        if (fgets(line, linesize, stdin) == NULL){
            continue;
        }
        // kill(getpid(), SIGINT);
        // printf("%s", line);
        /**/
        int par_count = input_read(line, &command, parameter, array_size);
        if (command == NULL){
            printf("Invalid input\n");
        }
        /*cases*/
        else if (strcmp(command, "/exit") == 0){ /* /exit */

            for (int i=0; i<num_monitors; i++){
                kill(children_pids[i], SIGKILL);
            }
            make_logfile(namelist, num_subdirs-2);
            /* wait children */
            int status;
            for (int i=0; i<num_monitors; i++){
                pid = wait(&status);
                printf("Child terminated: PID = %d, exit code = %d\n", pid, status >> 8);
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

                        send_message(monitor_fd[WRITE][countryFrom_monitor], message, size, buffer_size);
                        
                        /* receive answer */
                        char* answer;
                        int size_answer;
                        receive_message(monitor_fd[READ][countryFrom_monitor], &answer, &size_answer, buffer_size);
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
                        send_message(monitor_fd[WRITE][countryTo_monitor], "ACCEPTED", strlen("ACCEPTED")+1,  buffer_size);
                    else
                        send_message(monitor_fd[WRITE][countryTo_monitor], "REJECTED", strlen("REJECTED")+1,  buffer_size);


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
                /* inform only the monitor that has that country thrpugh signal SIGUSR1 */
                for (int i=0; i<num_subdirs; i++){
                    if (strcmp(namelist[i]->d_name, parameter[0])==0){
                        // printf("Sending SIGUSR1 to %d\n", children_pids[i%num_monitors]);
                        kill(children_pids[i%num_monitors], SIGUSR1);

                        /* receive updated bloom filters */
                        int count_loops=0;
                        while (1){
                            count_loops++;
                            char* complete_message;
                            int message_size;
                            receive_message(monitor_fd[READ][i%num_monitors], &complete_message, &message_size, buffer_size);

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
                    send_message(monitor_fd[WRITE][i], message, strlen(command)+1+strlen(parameter[0])+1, buffer_size);
                }
                struct timeval tv;
                tv.tv_usec = 0;
                tv.tv_sec = 5;
                /* prepare for select */
                fd_set fds;
                FD_ZERO(&fds); /* init fd_set */
                for (int i=0; i<num_monitors; i++){
                    FD_SET(monitor_fd[READ][i], &fds);
                }
                int maxfd = monitor_fd[READ][num_monitors-1];

                /* I am using select() call to read whichever bloom filter comes first */
                int retval = select(maxfd+1, &fds, NULL, NULL, &tv);
                if (retval == -1){
                    perror("select()");
                }
                else if (retval){
                    for (int i=0; i<num_monitors; i++){
                        if (FD_ISSET(monitor_fd[READ][i], &fds)) { /* find which fifo is ready */
                            /* read and print answer */
                            char *complete_message;
                            int size;
                            receive_message(monitor_fd[READ][i], &complete_message, &size, buffer_size);
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

    if (sig_intquit){
        for (int i=0; i<num_monitors; i++){
            kill(children_pids[i], SIGKILL);
        }
        make_logfile(namelist, num_subdirs-2);
        /* wait children */
        int status;
        for (int i=0; i<num_monitors; i++){
            pid = wait(&status);
            printf("Child terminated: PID = %d, exit code = %d\n", pid, status >> 8);
        }
        sig_intquit = 0;
    }

    destroy_stats();
    free(line);
    free(parameter);

    /* free list of subdirs */
    while (num_subdirs--) {
        // printf("%s\n", old_namelist[num_subdirs]->d_name);
        free(old_namelist[num_subdirs]);
    }
    free(old_namelist);

    HashT_delete(bloom_filters);
    HashT_delete(viruses);
    
    for (int i=0; i<num_monitors; i++){
        close(monitor_fd[READ][i]);
        close(monitor_fd[WRITE][i]);
    }
    for (int i=0; i<2; i++)
        free(monitor_fd[i]);
    free(monitor_fd);
    return 0;
}

void replace_monitor(pid_t dead_pid, int num_monitors, pid_t* children_pids, int** monitor_fd, int buffer_size, int size_of_bloom, int num_subdirs, struct dirent** namelist, char* input_dir, HashT* viruses, HashT* bloom_filters){
    /* new monitor will be replaced in place of the dead monitor */
    /* find the position (i) of the old monitor through the previous children_pids table, and keep the position */
    /* later we will replace the position (i) of children_pids and monitor_fd */

    printf("Replacing proccess with pid %d\n", dead_pid);
    int i;
    for (i=0; i<num_monitors; i++){
        if (children_pids[i] == dead_pid){
            break;
        }
    }
    close(monitor_fd[READ][i]);
    close(monitor_fd[WRITE][i]);
    char read_fifo[64], write_fifo[64];
    sprintf(read_fifo, "%s%s%d", PATH, "fifo_read", i); 
    sprintf(write_fifo, "%s%s%d", PATH, "fifo_write", i);
    // printf("travelMonitor %s, %s\n", read_fifo, write_fifo);
    int dir_len;
    
    char path[PATH_BUFSIZE];
    getcwd(path, PATH_BUFSIZE);
    dir_len = strlen(path);

    strcpy(path + dir_len, "/build/monitor");
    // printf("%s\n", path);
    pid_t pid = fork(); 
    children_pids[i] = pid; /* replace pid in table */
    if (pid < 0) {
        perror("fork");
    }
    else if (pid == 0) {
        execl(path, "monitor", read_fifo, write_fifo, (char*)0);
        perror("execl");
        _exit(127);
    }
    // printf("fork done. I'm parent\n");
    
    /* only the parent process reaches this */
    if ( (monitor_fd[READ][i] = open(read_fifo, O_RDONLY|O_TRUNC)) < 0){
        perror("server: can't open read fifo");
    }
    
    if ( (monitor_fd[WRITE][i] = open(write_fifo, O_WRONLY|O_TRUNC)) < 0){
        perror("server: can't open write fifo");
    }
    /* first message to each monitor is buffer size and it is sizeof(int) */ 
    // printf("buf size=%s\n", buffer_size_str);
    if (write(monitor_fd[WRITE][i], &buffer_size, sizeof(int))!=sizeof(int)){
        perror("Monitor write buff size");
        exit(0);
    }
    /* then send bloom size */
    send_message(monitor_fd[WRITE][i], (char *)(&size_of_bloom), sizeof(int), buffer_size);

    int loop_counter = 0;
    for (int j=i; j<num_subdirs-2; j+=num_monitors){ /* for each subdir */
        int message_size=strlen(input_dir)+strlen(namelist[j]->d_name)+2;
        char message[strlen(input_dir)+strlen(namelist[j]->d_name)+2];
        strcpy(message, input_dir);
        strcat(message, "/");
        strcat(message, namelist[j]->d_name);
        loop_counter++;
        send_message(monitor_fd[WRITE][i], message, message_size, buffer_size); 
    }
    // printf("loops %d\n", loop_counter);
    
    char message[3];
    memset(message, 0, sizeof(message));
    memcpy(message, "END", 3);
    send_message(monitor_fd[WRITE][i], message, 3, buffer_size);

    /* receive updated bloom filters */
    int count_loops=0;
    while (1){
        count_loops++;
        char* complete_message;
        int message_size;
        receive_message(monitor_fd[READ][i], &complete_message, &message_size, buffer_size);

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
