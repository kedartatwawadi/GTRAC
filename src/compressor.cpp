#include "compressor.h"
using namespace std;


// ***************************************************************
// Bit vector creation functions
// ***************************************************************
void compressor::createBitVector(bool* phrase, int file_id, RSDic* succinct_rsdic )
{
	int num_symbols = gtrac_input.get_num_symbols();
	bvb.Clear();	
	for( int j = 0;j < num_symbols; j++)
		bvb.PushBack( phrase[j] );
	bvb.Build(succinct_rsdic[file_id]);
}

void compressor::createBitVector(vector<bool> phrase, int file_id, RSDic* succinct_rsdic )
{
	bvb.Clear();	
	for( int j = 0;j < phrase.size(); j++)
		bvb.PushBack( phrase[j] );
	bvb.Build(succinct_rsdic[file_id]);
}



// ***************************************************************
// Default Constructor
// ***************************************************************
compressor::compressor(){
}

// ***************************************************************
// Constructor
// ***************************************************************
compressor::compressor(input_data* input_info, string output_name){
	prepare_compressor(input_info, output_name);
}

// ***************************************************************
// Prepare compressor
// ***************************************************************
void compressor::prepare_compressor(input_data* input_info, string output_name){
	gtrac_input = *input_info;
	output_prefix = output_name;
	pos = 0;
	bvb.Clear();

	prepare_files();
	int num_files = gtrac_input.get_num_files();
	//int file_size = gtrac_input.get_file_size();
	int num_symbols = gtrac_input.get_num_symbols();
    

    gtrac_parser.initialize_parser(num_symbols, num_files); 
}


// ***************************************************************
// Prepare output files
// ***************************************************************
bool compressor::prepare_files()
{
	int num_files = gtrac_input.get_num_files();
	//int file_size = gtrac_input.get_file_size();
	int num_symbols = gtrac_input.get_num_symbols();


	// Create the vectors to store the phrase endings
	phraseEnd = new RSDic[num_files];
	phraseLiteral = new RSDic[num_files];
	phraseSourceSize = new RSDic[num_files];
    phraseLiteralSize = new RSDic[num_files];

	// Always true for the reference vector, as we are storing it directly.
	for( int j = 0;j < num_symbols; j++)
		bvb.PushBack( true );
	bvb.Build(phraseEnd[0]);
	
	return 0;
}


// ***************************************************************
// I/Ofunctions
// ***************************************************************
symbol_t* compressor::read_file(string &name)
{
	int file_size = gtrac_input.get_file_size();
	int num_symbols = gtrac_input.get_num_symbols();
    int padding = gtrac_input.get_padding_flag();

	FILE *in = fopen(name.c_str(), "r");
	if(!in)
	{
		int a;
		cout << "No file!: " << name << "\n";
		cin >> a;
		return NULL;
	}

	// Check size
	fseek(in, 0, SEEK_END);
	if(ftell(in) != (unsigned int) file_size)
	{
		cout << "File " << name << " is of incompatibile size\n";
		fclose(in);
		return NULL;
	}
	fseek(in, 0, SEEK_SET);

    unsigned char *d = new unsigned char[file_size+padding];
	int ret = fread(d, 1, file_size, in);
	fclose(in);
    if(padding)
        d[file_size+padding] = 0;

    symbol_t* d_symbol_t_ptr = (symbol_t*)d;
	return d_symbol_t_ptr;
}


// ***************************************************************
// Compression functions
// ***************************************************************
void compressor::compress_file(symbol_t * d, file_id_t file_id)
{
	vector<string> file_names = gtrac_input.get_file_names();
	int num_files = gtrac_input.get_num_files();
	//int file_size = gtrac_input.get_file_size();
	int num_symbols = gtrac_input.get_num_symbols();

	cout << "Parsing file id: " << file_id << endl;
	
	string filename = file_names[file_id];
	ofstream out_file(( (string)phraseParmsDir+"/"+ filename + ".parms"), ios::out | ios::binary | ios::trunc );
	

	bool phrase[num_symbols];
	fill_n(phrase, num_symbols, false);
	vector<bool> phrase_literal;
	vector<bool> phrase_source_size;
	vector<bool> phrase_literal_size;


	pos = 0;
	while(pos < num_symbols)
	{
		    pair<int, int> match = gtrac_parser.find_match(d, pos, file_id, phraseEnd);
			pos += match.second;
			// Get the phrase index to be stored.
			int phrase_index = -1;
			if( match.first >= 0 )
			{
				phrase_index = phraseEnd[match.first].Rank(pos,true);
				phrase_literal.push_back(false);
			}
			else
			{
				phrase_literal.push_back(true);
			}
			

			pos++;
			phrase[pos] = true;
			
			/******************************/
			// WRITE SOURCE
			/******************************/
			if(match.first >= 0)
			{
				unsigned short match_file_id = (unsigned short) file_id - (unsigned short) match.first;
				if( match_file_id > 255)
				{
					out_file.write( (char*)&match_file_id, sizeof( unsigned short ) );
					phrase_source_size.push_back(false);
				}
				else
				{
					unsigned char match_file_id_char = (unsigned char) match_file_id;
					out_file.write( (char*)&match_file_id_char, sizeof( unsigned char ) );
					phrase_source_size.push_back(true);
				}	
			};

			/******************************/
			// WRITE NEWCHAR
			/******************************/
            
            // cout << pos << ": " << match.first << ", " << match.second << endl;
            symbol_t match_new_symbol = d[pos];
            if( match_new_symbol > 255 )
            {
			    out_file.write( (char*)&match_new_symbol, sizeof( symbol_t ) );
                phrase_literal_size.push_back(false);
            }
            else
            {
				unsigned char match_new_symbol_char = (unsigned char) match_new_symbol;
				out_file.write( (char*)&match_new_symbol_char, sizeof( unsigned char ) );
				phrase_literal_size.push_back(true);
            }
			pos++; // gear up for the next search;
	}
	
	createBitVector(phrase, file_id, phraseEnd);
	createBitVector(phrase_literal, file_id, phraseLiteral);
    createBitVector(phrase_source_size, file_id, phraseSourceSize);
    createBitVector(phrase_literal_size, file_id, phraseLiteralSize);
    out_file.close();
}



// ***************************************************************
// The actual tgc compression
// ***************************************************************

void compressor::compress(void)
{
	symbol_t *d; // represents the data file
	vector<string> file_names = gtrac_input.get_file_names();
	int num_files = gtrac_input.get_num_files();
	
	for(file_id_t i = 0; i < num_files; ++i) // iterating over files
	{
		// If we are not able to open the file the move ahead to the next one
		// Also checks if the file sizes are consistent
		if((d = read_file(file_names[i])) == NULL)
			continue;

        gtrac_parser.parse_file(d, i);

		// If i = 0 , we are processing the first file, so no hashing here
		// If i != 0, then we are parsing the file with respect to the hash functions and then storing

		// This parsing is what we need to change for LZ-End I think.
		if(i)
			compress_file(d, i);
		else
			;// store_ref_file
	}
	
	output_all_succint_bv_files();
    output_reference_file();
    output_metadata();
}


// ***************************************************************
// Outputs the bv files
// ***************************************************************
void compressor::output_bv_files(RSDic* succint_bv_dict, string write_dir)
{
	int num_files = gtrac_input.get_num_files();

	filebuf fb;
	string filename = (write_dir  + ".succint_bv"); 
	fb.open(filename.c_str(),std::ios::out);
	ostream os(&fb);

	for (int i = 0 ; i < num_files ; i++)
	{
		cout << "wrote file: " <<  i <<  endl;
		succint_bv_dict[i].Save(os);
	}
	fb.close();
}

// ***************************************************************
// Outputs all the relevant succint bv files
// ***************************************************************
void compressor::output_all_succint_bv_files()
{
	output_bv_files(phraseEnd, phraseEndDir);
	output_bv_files(phraseLiteral, phraseLiteralDir);
	output_bv_files(phraseSourceSize, phraseSourceSizeDir);
	output_bv_files(phraseLiteralSize, phraseLiteralSizeDir);
}

void compressor::output_reference_file()
{
    unsigned char* reference_file = gtrac_input.get_reference_file();
    int file_size = gtrac_input.get_file_size();
    ofstream output_file((string)referenceFileDir+".bv", ios::binary);
    for(int i = 0 ; i < file_size ; i++ )
    {
        output_file << reference_file[i];
    }	
    output_file.close();        
}

void compressor::output_metadata()
{
    int num_files = gtrac_input.get_num_files();
	vector<string> file_names = gtrac_input.get_file_names();
    
    ofstream output_file((string)metadataFileDir+".bv", ios::binary);
    for(int i = 0 ; i < num_files ; i++ )
    {
        output_file << file_names[i] << endl;
    }	
    output_file.close();        
}


