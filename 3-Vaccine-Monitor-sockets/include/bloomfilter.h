#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include <stdbool.h>

struct bloom_filter{
    int m_bits;
    int k_hash;
    char* array; /*array of chars. char is 1 byte*/
};

/*
           Bloom Filter's array 
bytes:      0        1        2      
        |________|________|________|
bits:    01234567 01234567 01234567         
*/

typedef struct bloom_filter bloom_filter;

bloom_filter* bloom_init(int m_bits, int k_hash); /*initialize a bloom filter with m bits and k hash functions*/
void bloom_insert(bloom_filter* bf, char* item); /*insert item in bloom filter bf*/
bool bloom_exists(bloom_filter* bf, char* item); /*check if item exists in bloom filter*/
void bloom_delete(void* bf); /*delete bloom filter bf*/
void bloom_add(bloom_filter* bf, bloom_filter* new_bf); /* adds new_bf to bf using bitwise OR */
unsigned int compute_size(bloom_filter* bf);
void bloom_print(bloom_filter* bf);

#endif /*BLOOMFILTER_H*/