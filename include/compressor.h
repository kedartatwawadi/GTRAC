#ifndef COMPRESSOR_H_
#define COMPRESSOR_H_

#include <iostream>     // std::cout, std::ostream, std::ios
#include <fstream> 
#include <stdio.h>
#include <string>
#include <iterator>
#include <vector>
#include <thread>
#include <list>
#include <utility>

#include "port.h"
#include "rsdic/RSDic.hpp"
#include "rsdic/RSDicBuilder.hpp"
#include "input_data.h"

#define MIN_MATCH_LEN		5
#define MAX_MATCH_LEN_EXP	11
#define MAX_MATCH_LEN		((1 << MAX_MATCH_LEN_EXP) + MIN_MATCH_LEN - 1)
#define HASH_STEP1			4
#define HASH_STEP2			(MIN_MATCH_LEN - HASH_LEN2 + 1)

#define HASH_LEN1			11
#define HASH_LEN2			2
#define HT_FACTOR			2

#define phraseEndDir "compressed_files/phrase_end"
#define phraseLiteralDir "compressed_files/phrase_C"
#define phraseSourceSizeDir "compressed_files/phrase_s"
#define phraseParmsDir "compressed_files/phrase_params"

using namespace std;
using namespace rsdic;
typedef short file_id_t;


class compressor {
public: 
	// ***************************************************************
	compressor();
	compressor(input_data* input_info, string output_name);
	void prepare_compressor(input_data* gtrac_input, string output_name); 
	bool prepare_files();
	void close_files(void);
	void compress(void);
	unsigned char* read_file(string &name);
	inline pair<int, int> find_match(unsigned char *p, int pos, int ver);
	void output_all_succint_bv_files();
	void output_bv_files(RSDic* succint_bv_dict, string write_dir);

	void prepare_ht(void);
	inline unsigned long hash_fun(unsigned char *p, int pos, int ver);
	void insert_into_ht(file_id_t file_id, unsigned char *p, int ver);
	void parse_file(unsigned char * d, int file_id);


	void createBitVector(bool* phrase, int file_id );
	void createLiteralBitVector(vector<bool> phrase_literal, int file_id );
	void createSourceSizeBitVector(vector<bool> phrase_source_size, int file_id );



private:
	vector<unsigned char*> data;
	RSDicBuilder bvb;
	RSDic* phraseEnd;
	RSDic* phraseLiteral;
	RSDic* phraseSourceSize;

	int pos;
	int cur_id_file;
	string output_prefix;
	input_data gtrac_input;

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
};

#endif /* COMPRESSOR_H_ */




