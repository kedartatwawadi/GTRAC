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
#include "hash_table.h"

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
    pair<int, int> find_match(unsigned char *p, int pos, file_id_t current_file_id, RSDic* phraseEnd );

private:
    int file_size;
    int num_files;
	vector<unsigned char*> data;
	
    hash_table ht1;
    hash_table ht2;
};

#endif /* PARSER_H_ */

