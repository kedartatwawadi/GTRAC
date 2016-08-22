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
#define phraseLiteralDir "compressed_files/phrase_literal"
#define phraseSourceSizeDir "compressed_files/phrase_source_size"
#define phraseLiteralSizeDir "compressed_files/phrase_literal_size"

#define phraseParmsDir "compressed_files/phrase_params"
#define referenceFileDir "compressed_files/reference_file"
#define metadataFileDir "compressed_files/metadata_file"

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
	symbol_t* read_file(string &name);
	void output_all_succint_bv_files();
	void output_bv_files(RSDic* succint_bv_dict, string write_dir);
	void compress_file(symbol_t * d, file_id_t file_id);

	void createBitVector(bool* phrase, int file_id, RSDic* succinct_rsdic );
	void createBitVector(vector<bool> phrase_literal, int file_id, RSDic* succinct_rsdic );

    void output_reference_file();
    void output_metadata();

private:
	RSDicBuilder bvb;
	RSDic* phraseEnd;
	RSDic* phraseLiteral;
	RSDic* phraseSourceSize;
    RSDic* phraseLiteralSize;
    parser gtrac_parser;

	int pos;
	int cur_id_file;
	string output_prefix;
	input_data gtrac_input;

};

#endif /* COMPRESSOR_H_ */




