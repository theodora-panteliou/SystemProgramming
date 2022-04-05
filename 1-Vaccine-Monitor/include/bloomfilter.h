#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include <stdbool.h>

typedef struct bloom_filter bloom_filter;

bloom_filter* bloom_init(int m_bits, int k_hash); /*initialize a bloom filter with m bits and k hash functions*/
void bloom_insert(bloom_filter* bf, char* item); /*insert item in bloom filter bf*/
bool bloom_exists(bloom_filter* bf, char* item); /*check if item exists in bloom filter*/
void bloom_delete(void* bf); /*delete bloom filter bf*/

#endif /*BLOOMFILTER_H*/