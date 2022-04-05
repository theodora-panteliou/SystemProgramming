#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "hash_i.h"
#include "bloomfilter.h"

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

void bloom_add(bloom_filter* bf, bloom_filter* new_bf){
    assert(bf->k_hash == new_bf->k_hash);
    assert(bf->m_bits == new_bf->m_bits);
    int array_size = bf->m_bits/(8*sizeof(char)); 
    if (bf->m_bits%(8*sizeof(char)) > 0){
        array_size += 1;
    }
    for (int i=0; i<array_size; i++){
        bf->array[i] |= new_bf->array[i];
    }
}

unsigned int compute_size(bloom_filter* bf){
    int array_size = bf->m_bits/(8*sizeof(char)); 
    if (bf->m_bits%(8*sizeof(char)) > 0){
        array_size += 1;
    }
    return sizeof(bloom_filter) + array_size*sizeof(char);
}

void bloom_print(bloom_filter* bf){
    printf("mbits %d\n", bf->m_bits);
    printf("khash %d\n", bf->k_hash);
    int array_size = bf->m_bits/(8*sizeof(char)); 
    if (bf->m_bits%(8*sizeof(char)) > 0){
        array_size += 1;
    }
    printf("bloom array\n");
    for (int i=0; i<6; i++){
        printf(" %x ", bf->array[i]);
    }
    printf("\n");
}