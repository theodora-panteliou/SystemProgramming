#include <stdio.h>
#include <stdlib.h>
#include "hash_i.h"
#include "bloomfilter.h"

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

bloom_filter* bloom_init(int m_bits, int k_hash){ /*m bits, k hash functions*/
    bloom_filter* bf = malloc(sizeof(bloom_filter));
    bf->m_bits = m_bits;
    bf->k_hash = k_hash;

    /* convert bits to bytes */
    int array_size = m_bits/(8*sizeof(char)); 
    if (m_bits%(8*sizeof(char)) > 0){
        array_size += 1;
    }
    /* allocate an array with m bits size */
    bf->array = malloc(array_size*sizeof(char));
    for (int i = 0; i<array_size; i++) { /*initializing array*/
        bf->array[i] = 0;
    }
    return bf;
}

void bloom_insert(bloom_filter* bf, char* item) {
    if (bf != NULL){
        int hash_value;
        for (int k = 0; k < bf->k_hash ; k++) { /* for K hash functions */
            hash_value = hash_i((unsigned char *)item, k) % bf->m_bits; /* hash_value is the i-th bit to be set inside the array */
            
            int array_cell = hash_value/(8*sizeof(char)); /* find the right cell in the array */
            int offset = hash_value%(8*sizeof(char)); /* find the bit inside the cell which is 1 byte */
            bf->array[array_cell] |= 1UL << offset; /* set the bit */
        }
    }
}

bool bloom_exists(bloom_filter* bf, char* item){
    if (bf != NULL){
        int hash_value;
        for (int k = 0; k < bf->k_hash ; k++) { /* for K hash functions */
            hash_value = hash_i((unsigned char *)item, k) % bf->m_bits; /* hash_value is the i-th bit that we need to check inside the array */
            
            int array_cell = hash_value/(8*sizeof(char)); /* find the right cell in the array */
            int offset = hash_value%(8*sizeof(char)); /* find the bit inside the cell which is 1 byte */
            int checking_bit = (bf->array[array_cell] >> offset) & 1U; /* check if that bit is 0 or 1 */
            if (checking_bit == 0) { /* if at least 1 bit of these is not checked then that item is definetly not in the bloom filter*/
                return false;
            }
        }
        return true;
    } else return false;
}

void bloom_delete(void* bloomf) {
    bloom_filter* bf=bloomf;
    if (bf != NULL){
        free(bf->array);
        free(bf);
    }
}