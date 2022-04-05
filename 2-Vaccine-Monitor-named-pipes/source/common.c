#include "common.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>


void send(int writefd, char* message, int message_size, int buffer_size, char* buff){
    int bytes_written = 0;
    int bytes;

    while (message_size - bytes_written > buffer_size){
        memmove(buff, &(message[bytes_written]), buffer_size);
        if ((bytes = write(writefd, buff, buffer_size))<0){
            perror("write");
            exit(0);
        }
        bytes_written+= bytes;
    }
    if (message_size - bytes_written>0){ /* if there are bytes left */
        memset(buff, 0, buffer_size); /* in cases that the buffer isn't full I fill the unused bytes with nulls */

        memmove(buff, &(message[bytes_written]), message_size - bytes_written);
        if ((bytes = write(writefd, buff, message_size - bytes_written))<0){
            perror("write");
            exit(0);
        }
        bytes_written+= bytes;
    }
    // assert(bytes_written==message_size);
}

void send_message(int writefd, char* message, int message_size, int buffer_size){

    char* buff=malloc(buffer_size);

    /* send size */
    send(writefd, (char *)(&message_size), sizeof(int), buffer_size, buff);

    /* send actual message */
    send(writefd, message, message_size, buffer_size, buff);
    
    free(buff);
}

void receive_message(int readfd, char** message, int* size, int buffer_size){
       
    char* buff=malloc(buffer_size);
    int bytes_read=0;
    int bytes;
    /* read message size */
    int message_size=0;
    char *size_sent=malloc(sizeof(int));
    bytes_read = 0;
    while (sizeof(int) - bytes_read> buffer_size && bytes_read>=0){
        if ((bytes = read(readfd, buff, buffer_size))<0){
            perror("read");
            return;
        }
        memmove(&(size_sent[bytes_read]), buff, buffer_size);
        bytes_read+=bytes;
    }
    if (sizeof(int) - bytes_read>0 && bytes_read>=0){
        memset(buff, 0, buffer_size);
        if ((bytes = read(readfd, buff, sizeof(int) - bytes_read))<0){
            perror("read2");
            return;
        }
        memmove(&(size_sent[bytes_read]), buff, sizeof(int) - bytes_read);
        bytes_read+=bytes;
    }
    // printf("bytres read %d\n", bytes_read);
    // assert(bytes_read == sizeof(int));
    message_size = *(int *)size_sent;
    free(size_sent);
    // printf("mess size is %d\n", message_size);
    char* complete_message=malloc(message_size*sizeof(char));
    bytes_read = 0;
    while (message_size-bytes_read>buffer_size){
        if ((bytes = read(readfd, buff, buffer_size))<0){
            perror("read3");
        }
        memmove(&(complete_message[bytes_read]), buff, buffer_size);
        bytes_read+=bytes;
    }
    /* these bytes will not be read with buffer_size */
    if (message_size-bytes_read>0){
        memset(buff, 0, buffer_size);
        if ((bytes = read(readfd, buff, message_size-bytes_read))<0){
            perror("read4");
        }
        memmove(&(complete_message[bytes_read]), buff, message_size-bytes_read);
        bytes_read+=bytes;
    }
    // assert(bytes_read==message_size);
    *message = complete_message;
    *size = message_size;
    free(buff);
}

int is_end(char* str){
    if (memcmp(str, "END", 3)==0){
        return 1;
    }
    else return 0;
}

void serialize_bloomfilter(char* dest, bloom_filter* source, int bloom_size){ /* this function converts a bloom filter to serial so that we can send it through a pipe */
    memmove( &(((char*)(dest))[0]), &(source->m_bits), sizeof(int));
    memmove( &(((char*)(dest))[sizeof(int)]), &(source->k_hash), sizeof(int));
    memmove( &(dest[2*sizeof(int)]), source->array, bloom_size);
}

bloom_filter* deserialize(char* source, int bloom_size){
    bloom_filter* bf=malloc(sizeof(bloom_filter));
    memmove(&(bf->m_bits), (char*)source, sizeof(int));
    memmove(&(bf->k_hash), &(((char*)(source))[sizeof(int)]), sizeof(int));

    bf->array = malloc(bloom_size);
    memmove(bf->array, &(((char*)(source))[2*sizeof(int)]), bloom_size);
    return bf;
}