#include <iostream>     // std::cout, std::ostream, std::ios
#include <fstream> 
#include <stdio.h>
#include <string.h>
#include <iterator>
#include <vector>
#include <iostream>
#include <utility>
#include <sstream>
#include <ctime>
#include <ratio>
#include <chrono>

#include "port.h"
#include "rsdic/RSDic.hpp"
#include "rsdic/RSDicBuilder.hpp"
#include "omp.h"
#include "input_data.h"

#define phraseEndDir "compressed_files/phrase_end"
#define phraseLiteralDir "compressed_files/phrase_literal"
#define phraseSourceSizeDir "compressed_files/phrase_source_size"
#define phraseLiteralSizeDir "compressed_files/phrase_literal_size"

#define phraseParmsDir "compressed_files/phrase_params"
#define referenceFileDir "compressed_files/reference_file"
#define metadataFileDir "compressed_files/metadata_file"

#define resultsDir "output_gtrac"
#define number_of_blocks 4
typedef short symbol_t;

using namespace std;
using namespace rsdic;
using namespace std::chrono;

class decompressor{
public:
    void perform_column_decomp(int column_num);
    void perform_row_decomp(int file_id);
    void perform_substring_decomp(int file_id, int start, int len); 
    void initialize_decompressor(char* path);

private:
    RSDicBuilder bvb;
    RSDic* phraseEnd;
    RSDic* phraseLiteral;
    RSDic* phraseSourceSize;
    RSDic* phraseLiteralSize;


    input_data gtrac_input;
    vector<symbol_t> data;
    vector<vector<symbol_t> > block_data;
    vector<symbol_t> column_data;

    int select(int file_id, int phrase_id);
    void extract(int file_id, int start, int len);
    void extract_block(int file_id, int start, int len, int block_id);
    void extractColumn(int column_no );
    void extractLong(int file_id,int start,int len);

    RSDic* readSuccintBitVectors(string bvDictDir);
    symbol_t getNewCharforPhrase(int file_id, int phrase_id);
    int getSourceforPhrase( int file_id, int phrase_id);
    symbol_t* get_symbol_reference_file();
};


