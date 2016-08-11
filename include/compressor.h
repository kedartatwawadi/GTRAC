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
#include "parser.h"

#define phraseEndDir "compressed_files/phrase_end"
#define phraseLiteralDir "compressed_files/phrase_C"
#define phraseSourceSizeDir "compressed_files/phrase_s"
#define phraseParmsDir "compressed_files/phrase_params"
#define referenceFileDir "compressed_files/reference_file"

using namespace std;
using namespace rsdic;


class compressor {
public: 
	// ***************************************************************
	compressor();
	compressor(input_data* input_info, string output_name);
	void prepare_compressor(input_data* gtrac_input, string output_name); 
	bool prepare_files();
	void compress(void);
	unsigned char* read_file(string &name);
	void output_all_succint_bv_files();
	void output_bv_files(RSDic* succint_bv_dict, string write_dir);
	void compress_file(unsigned char * d, file_id_t file_id);

	void createBitVector(bool* phrase, int file_id );
	void createLiteralBitVector(vector<bool> phrase_literal, int file_id );
	void createSourceSizeBitVector(vector<bool> phrase_source_size, int file_id );

    void output_reference_file();


private:
	RSDicBuilder bvb;
	RSDic* phraseEnd;
	RSDic* phraseLiteral;
	RSDic* phraseSourceSize;
    parser gtrac_parser;

	int pos;
	int cur_id_file;
	string output_prefix;
	input_data gtrac_input;
};

#endif /* COMPRESSOR_H_ */




