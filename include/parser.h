#ifndef PARSER_H_
#define PARSER_H_

#include <iostream>     // std::cout, std::ostream, std::ios
#include <fstream> 
#include <stdio.h>
#include <string>
#include <iterator>
#include <vector>
#include <thread>
#include "rsdic/RSDic.hpp"


#define HASH_TABLE_VER_1    1
#define HASH_TABLE_VER_2    2
#define HASH_LEN1			11
#define HASH_LEN2			2
#define HT_FACTOR			2
#define MIN_MATCH_LEN		5
#define MAX_MATCH_LEN_EXP	11
#define MAX_MATCH_LEN		((1 << MAX_MATCH_LEN_EXP) + MIN_MATCH_LEN - 1)
#define HASH_STEP1			4
#define HASH_STEP2			(MIN_MATCH_LEN - HASH_LEN2 + 1)

using namespace std;
using namespace rsdic;
typedef short file_id_t;

class parser {
public:
    parser();
    parser(int file_size_t, int num_files_t);
	void initialize_parser(int file_size_t, int num_files_t);
    void prepare_ht(void);
    void parse_file(unsigned char *d, file_id_t current_file_id);
	void insert_into_ht(file_id_t file_id, unsigned char *p, int ver);
	pair<int, int> find_match_ht(unsigned char *p, int pos, int ver, file_id_t current_file_id, RSDic* phraseEnd );  
    pair<int, int> find_match(unsigned char *p, int pos, file_id_t current_file_id, RSDic* phraseEnd );

private:
    int file_size;
    int num_files;
	vector<unsigned char*> data;
	

	// hash table stuff
	// file_id_t is a short typedef
	file_id_t *ht1, *ht2;   // Hash Tables
	file_id_t *ht_zeros1, *ht_zeros2; // I dont know what is this.

	// unsigned int no_files;							// no. of files to compress
	unsigned long long ht_size1, ht_size2;			// size of hash table
	unsigned long ht_slots1, ht_slots2;				// no. of slots in HT
	unsigned long ht_slot_size_exp;					// size of hast table for each location
	unsigned long ht_slot_size_mask;				// size of hast table for each location
	unsigned long ht_slot_size;						// size of slot of hash table
	unsigned long hash_fun(unsigned char *p, int pos, int ver);

};

#endif /* PARSER_H_ */

