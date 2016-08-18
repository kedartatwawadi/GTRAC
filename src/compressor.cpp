#include "compressor.h"
using namespace std;


// ***************************************************************
// Bit vector creation functions
// ***************************************************************
void compressor::createBitVector(bool* phrase, int file_id )
{
	int file_size = gtrac_input.get_file_size();
	bvb.Clear();	
	for( int j = 0;j < file_size; j++)
		bvb.PushBack( phrase[j] );
	bvb.Build(phraseEnd[file_id]);
}

void compressor::createLiteralBitVector(vector<bool> phrase_literal, int file_id )
{
	bvb.Clear();	
	for( int j = 0;j < phrase_literal.size(); j++)
		bvb.PushBack( phrase_literal[j] );
	bvb.Build(phraseLiteral[file_id]);
}

void compressor::createSourceSizeBitVector(vector<bool> phrase_source_size, int file_id )
{
	bvb.Clear();	
	for( int j = 0;j < phrase_source_size.size(); j++)
		bvb.PushBack( phrase_source_size[j] );
	bvb.Build(phraseSourceSize[file_id]);
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
	int file_size = gtrac_input.get_file_size();
    
    gtrac_parser.initialize_parser( file_size, num_files); 
}


// ***************************************************************
// Prepare output files
// ***************************************************************
bool compressor::prepare_files()
{
	int num_files = gtrac_input.get_num_files();
	int file_size = gtrac_input.get_file_size();

	// Create the vectors to store the phrase endings
	phraseEnd = new RSDic[num_files];
	phraseLiteral = new RSDic[num_files];
	phraseSourceSize = new RSDic[num_files];

	// Always true for the reference vector, as we are storing it directly.
	for( int j = 0;j < file_size; j++)
		bvb.PushBack( true );
	bvb.Build(phraseEnd[0]);
	
	return 0;
}


// ***************************************************************
// I/Ofunctions
// ***************************************************************
unsigned char* compressor::read_file(string &name)
{
	int file_size = gtrac_input.get_file_size();

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

	unsigned char *d = new unsigned char[file_size];
	int ret = fread(d, 1, file_size, in);
	fclose(in);

	return d;
}


// ***************************************************************
// Compression functions
// ***************************************************************
void compressor::compress_file(unsigned char * d, file_id_t file_id)
{
	vector<string> file_names = gtrac_input.get_file_names();
	int num_files = gtrac_input.get_num_files();
	int file_size = gtrac_input.get_file_size();

	cout << "Parsing file id: " << file_id << endl;
	
	string filename = file_names[file_id];
	ofstream out_file(( (string)phraseParmsDir+output_prefix+"."+filename + ".parms"), ios::out | ios::binary | ios::trunc );
	

	bool phrase[file_size];
	fill_n(phrase, file_size, false);
	vector<bool> phrase_literal;
	vector<bool> phrase_source_size;

	pos = 0;
	while(pos < file_size)
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

			out_file.write( (char*)&d[pos], sizeof( char ) );
			pos++; // gear up for the next search;
	}
	
	createBitVector(phrase,file_id);
	createLiteralBitVector(phrase_literal,file_id);
	createSourceSizeBitVector(phrase_source_size,file_id);
	out_file.close();
}



// ***************************************************************
// The actual tgc compression
// ***************************************************************

void compressor::compress(void)
{
	unsigned char *d; // represents the data file
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
void compressor::output_bv_files(RSDic* succint_bv_dict, string file_prefix)
{
	int num_files = gtrac_input.get_num_files();

	filebuf fb;
	string filename = (compressedFilesDir+output_prefix+"."+file_prefix+".succint_bv"); 
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
	output_bv_files(phraseEnd, phraseEndFile);
	output_bv_files(phraseLiteral, phraseLiteralFile);
	output_bv_files(phraseSourceSize, phraseSourceSizeFile);
}

void compressor::output_reference_file()
{
    unsigned char* reference_file = gtrac_input.get_reference_file();
    int file_size = gtrac_input.get_file_size();
    ofstream output_file((string)compressedFilesDir+output_prefix+"."+referenceFile+".bv", ios::binary);
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

    ofstream output_file((string)compressedFilesDir+output_prefix+"."+metadataFile+".bv", ios::binary);

    for(int i = 0 ; i < num_files ; i++ )
    {
        output_file << file_names[i] << endl;
    }	
    output_file.close();        
}


