#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

#include <iostream>     // std::cout, std::ostream, std::ios
#include <fstream> 
#include <stdio.h>
#include <string>
#include <iterator>
#include <vector>
#include "rsdic/RSDic.hpp"

using namespace std;
using namespace rsdic;
typedef short file_id_t;

class hash_table {
public:
    hash_table();
    hash_table(int num_files, int file_size, int hash_len, int ht_factor, int hash_step);
    void initialize_ht(int num_files, int file_size, int hash_len, int ht_factor, int hash_step);
	void prepare_ht();
    void insert_into_ht(file_id_t file_id, unsigned char *p);
	pair<int, int> find_match_ht(unsigned char *p, int pos, file_id_t current_file_id, RSDic* phraseEnd, vector<unsigned char*>* data_ptr );  
    
    int get_hash_len();
    int get_hash_step();

private:
    int file_size;
    int num_files;

    unsigned int hash_len;
    unsigned int ht_factor;
    unsigned int hash_step;

	unsigned long long ht_size;			// size of hash table
	unsigned long ht_slots;				// no. of slots in HT
	unsigned long ht_slot_size_exp;		// size of hast table for each location
	unsigned long ht_slot_size_mask;	// size of hast table for each location
	unsigned long ht_slot_size;			// size of slot of hash table

    file_id_t* ht;
    file_id_t* ht_zeros;

    unsigned long hash_fun(unsigned char *p, int pos);

};

#endif /* HASH_TABLE_H_ */
