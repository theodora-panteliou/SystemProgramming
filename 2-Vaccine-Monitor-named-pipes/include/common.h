#include "bloomfilter.h"
#include <signal.h>
// #include <unistd.h>

void send_message(int writefd, char* message, int message_size, int buffer_size);
void receive_message(int readfd, char** message, int* size, int buffer_size);
int is_end(char* str);

/* this function converts a bloom filter to serial so that we can send it through a pipe */
void serialize_bloomfilter(char* dest, bloom_filter* source, int bloom_size);
bloom_filter* deserialize(char* source, int bloom_size);
