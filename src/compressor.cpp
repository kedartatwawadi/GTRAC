#include "compressor.h"
using namespace std;

// ***************************************************************
// Prepare the hash tables
// ***************************************************************
void compressor::prepare_ht(void)
{
	// set the appropriate numbet of hashtable slots
	int file_size = gtrac_input.get_file_size();
	int no_files  = gtrac_input.get_num_files();

	for(ht_slot_size_exp = 5; no_files * HT_FACTOR > (1u << ht_slot_size_exp); ht_slot_size_exp++);

	ht_slot_size      = 1u << ht_slot_size_exp;
	ht_slot_size_mask = ht_slot_size-1; // the mask is used later to find the mod with respect to this value .. this is a power of 2 
	                                    // so this works so awesomely! 
	
	ht_slots1	      = (unsigned long) (file_size / HASH_STEP1 + 1);
	ht_slots2	      = (unsigned long) (file_size / HASH_STEP2 + 1);
	ht_size1          = ht_slot_size * ht_slots1;
	ht_size2          = ht_slot_size * ht_slots2;
	
	// Need to understand this, but not very improtant
	cout << "HT sizes: " << ht_size1 / (1<<20)*sizeof(short) + ht_size2 / (1<<20)*sizeof(short) << "MB\n";

	ht1 = new file_id_t[ht_size1];
	fill(ht1, ht1+ht_size1, -1);
	ht2 = new file_id_t[ht_size2];
	fill(ht2, ht2+ht_size2, -1);

	// set counter of entries with hash value = 0
	// The hash table construction is kind of okay as of this point.
	// but not clear why this ht_zeros has been defined.
	// It is just one value per hash_table ( as in one value per hash table which belongs to ht1 or ht2)
	ht_zeros1 = new file_id_t[ht_slots1];
	fill(ht_zeros1, ht_zeros1+ht_slots1, 0);
	ht_zeros2 = new file_id_t[ht_slots2];
	fill(ht_zeros2, ht_zeros2+ht_slots2, 0);
}




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
// Hash table functions
// ***************************************************************

// ***************************************************************
// Hash function
// ***************************************************************
inline unsigned long compressor::hash_fun(unsigned char *p, int pos, int ver)
{
	unsigned long res = 0;
	
    // ver: corresponds to the type of hash function in use
	if(ver == 1)
		for(int i = 0; i < HASH_LEN1; ++i)
			res = res * 65531u + p[pos+i] * 29u;
	else
		for(int i = 0; i < HASH_LEN2; ++i)
			res = res * 65531u + p[pos+i] * 29u;
		
	return res & ht_slot_size_mask;
}

// ***************************************************************
// Find the best match
// ***************************************************************
inline pair<int, int> compressor::find_match(unsigned char *p, int pos, int ver)
{

	int file_size = gtrac_input.get_file_size();
	
	int best_id  = -1;
	int best_len = -1;
	int hash_step, hash_len;
	//cerr << "find_match" << endl;
	if(ver == 1)
	{
		hash_step = HASH_STEP1;
		hash_len  = HASH_LEN1;
	}
	else
	{
		hash_step = HASH_STEP2;
		hash_len  = HASH_LEN2;
	}
			
	// round off pos to the nearest starting point of 
	int pos_norm = ((pos + hash_step - 1) / hash_step)* hash_step;
	
	if(pos_norm + hash_len > file_size)
		return make_pair(-1, -1);
		
	unsigned long h = hash_fun(p, pos_norm, ver);
	unsigned long long off = (pos_norm / hash_step) << (ht_slot_size_exp);
	short *ht;
	
	if(ver == 1)
		ht = ht1;
	else
		ht = ht2;
	
	while(ht[off+h] != -1)
	{
		int file_id = ht[off+h];
		int i;
		int last_phrase_end_pos = 0;
		
		// since we only want to reference to previously sequenced stuff.
		if(file_id < cur_id_file)
		{ 
			for(i = pos; i < file_size; ++i)
			{
				//cout << "Here" << endl;
				if(p[i] != data[file_id][i])
					break;
				if( phraseEnd[file_id].GetBit(i) )
					last_phrase_end_pos = i;
			}

			if( ( (last_phrase_end_pos - pos) > best_len ) && (last_phrase_end_pos > 0 ) )
			{
				
				best_len = last_phrase_end_pos - pos;
				best_id  = file_id;
			}	
		}
		
		// We go and check all the hashes until we get -1 .
		h = (h+1) & ht_slot_size_mask;
	}
	
	// if smaller than MIN_MATCH_LEN then we store it as a literal rt?
	if(best_len < 2)
		return make_pair(-1, -1);
	
	//cout << "Comparing with matching files:  "<< best_id << "," << pos<< endl;
	return make_pair(best_id, best_len);
}

// ***************************************************************
//
// ***************************************************************
inline pair<int, int> compressor::find_match_final(unsigned char *p, int pos)
{
	pair<int, int> match = find_match(p, pos, 1);
	if(match.second < HASH_LEN1 + HASH_STEP1 - 1)
		match = find_match(p, pos, 2);
    return match;
}

// ***************************************************************
//
// ***************************************************************
void compressor::insert_into_ht(file_id_t file_id, unsigned char *p, int ver)
{
	int file_size = gtrac_input.get_file_size();

	unsigned int n_slot = 0;
	unsigned long long off = 0;
	
	int hash_len, hash_step;
	short *ht, *ht_zeros;
	
	if(ver == 1)
	{
		hash_len  = HASH_LEN1;
		hash_step = HASH_STEP1;
		ht        = ht1;
		ht_zeros  = ht_zeros1;
	}
	else
	{
		hash_len  = HASH_LEN2;
		hash_step = HASH_STEP2;
		ht        = ht2;
		ht_zeros  = ht_zeros2;
	}
	
	for(int i = 0; i < file_size - hash_len + 1; i += hash_step, ++n_slot, off += ht_slot_size)
	{
		unsigned long h = hash_fun(p, i, ver);

		if(h <= ht_zeros[n_slot])		// insert counter of entries with hash_value = 0
			h = ht_zeros[n_slot]++;
	
		// If the bucket is already filled, put it into the next bucket
		while(ht[off+h] != -1)
			h = (h+1) & ht_slot_size_mask;

		ht[off+h] = file_id;
	}
}


// ***************************************************************
// Add stuff to the hash table
// ***************************************************************

void compressor::add_file_to_hash_table(unsigned char *d)
{
	data.push_back(d);
	// data would be a really big vector :-o as it is basically handling the whole data.

	// Here we start two threads to hash the file using two hash tables
	// This syntax is something new to C++ some kind of lambda function stuff.
	// no need to worry about that right now
	thread t1([&]{insert_into_ht(cur_id_file, d, 1);});
	thread t2([&]{insert_into_ht(cur_id_file, d, 2);});

	t1.join();
	t2.join();

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
	cur_id_file = 0;
	bvb.Clear();

	prepare_files();	
	prepare_ht();
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
void compressor::parse_file(unsigned char * d, int file_id)
{
	vector<string> file_names = gtrac_input.get_file_names();
	int num_files = gtrac_input.get_num_files();
	int file_size = gtrac_input.get_file_size();

	cout << "Parsing file id: " << file_id << endl;
	
	string filename = file_names[file_id];
	ofstream out_file(( (string)phraseParmsDir+"/"+ filename + ".parms"), ios::out | ios::binary | ios::trunc );
	

	bool phrase[file_size];
	fill_n(phrase, file_size, false);
	vector<bool> phrase_literal;
	vector<bool> phrase_source_size;

	pos = 0;
	while(pos < file_size)
	{
		    pair<int, int> match = find_match_final(d, pos);
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
	
	for(int i = 0; i < num_files; ++i) // iterating over files
	{
		// If we are not able to open the file the move ahead to the next one
		// Also checks if the file sizes are consistent
		if((d = read_file(file_names[i])) == NULL)
			continue;

        add_file_to_hash_table(d);

		// If i = 0 , we are processing the first file, so no hashing here
		// If i != 0, then we are parsing the file with respect to the hash functions and then storing

		// This parsing is what we need to change for LZ-End I think.
		if(i)
			parse_file(d, i);
		else
			;// store_ref_file


		++cur_id_file;
	}
	
	output_all_succint_bv_files();
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
}

