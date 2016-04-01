#include <iostream>     // std::cout, std::ostream, std::ios
#include <fstream> 
#include <stdio.h>
#include <string.h>
#include <string>
#include <iterator>
#include <vector>
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <thread>
#include <list>
#include <utility>
#include <thread>

#include "port.h"
#include "rsdic/RSDic.hpp"
#include "rsdic/RSDicBuilder.hpp"

//#define TURN_OFF_OFFSETS
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

//typedef int file_id_t;
typedef short file_id_t;


FILE *rc_file;			// output file (range coder compressed)
FILE *desc_file;		// output file (description of compressed data)
FILE *out_file;			// decompressed file

// file_id_t is a short typedef
file_id_t *ht1, *ht2;   // Hash Tables
file_id_t *ht_zeros1, *ht_zeros2; // I dont know what is this.

unsigned int no_files;							// no. of files to compress
unsigned long long ht_size1, ht_size2;			// size of hash table
unsigned long ht_slots1, ht_slots2;				// no. of slots in HT
unsigned long ht_slot_size_exp;					// size of hast table for each location
unsigned long ht_slot_size_mask;				// size of hast table for each location
unsigned long ht_slot_size;						// size of slot of hash table
int file_size;							// size of file

vector<unsigned char*> data;

RSDicBuilder bvb;
RSDic* phraseEnd;
RSDic* phraseLiteral;
RSDic* phraseSourceSize;

vector<string> file_names;
set<string> file_names_decompress;



int pos;
unsigned char ref_literal;
int cur_id_file;


// ***************************************************************
bool check_data(int argc, char *argv[]);
bool prepare_files(string output_name);
void close_files(void);

void prepare_ht(void);
unsigned char *read_file(string &name);
void compress(void);
void parse_file(unsigned char *d);
void store_start(string &name);

inline unsigned long hash_fun(unsigned char *p, int pos, int ver);
void insert_into_ht(file_id_t file_id, unsigned char *p, int ver);
inline pair<int, int> find_match(unsigned char *p, int pos, int ver);
inline int pop_count(unsigned int x);
void output_all_succint_bv_files();

// ***************************************************************
// Range coder-relared functions
// ***************************************************************

// ***************************************************************
void save_byte(int x)
{
	putc(x, rc_file);
}

// ***************************************************************
int read_byte()
{
	int a = getc(rc_file);
	return a;
}

void createBitVector(bool* phrase, int file_id )
{
	bvb.Clear();	
	for( int j = 0;j < file_size; j++)
		bvb.PushBack( phrase[j] );
	bvb.Build(phraseEnd[file_id]);
}


void createLiteralBitVector(vector<bool> phrase_literal, int file_id )
{
	bvb.Clear();	
	for( int j = 0;j < phrase_literal.size(); j++)
		bvb.PushBack( phrase_literal[j] );
	bvb.Build(phraseLiteral[file_id]);
}

void createSourceSizeBitVector(vector<bool> phrase_source_size, int file_id )
{
	bvb.Clear();	
	for( int j = 0;j < phrase_source_size.size(); j++)
		bvb.PushBack( phrase_source_size[j] );
	bvb.Build(phraseSourceSize[file_id]);
}

// ***************************************************************
// Utility functions
// ***************************************************************

// **************************************************************
// Calculate number of set bits
inline int pop_count(unsigned int x)
{
	int cnt = 0;
	
	for(; x; ++cnt)
		x &= x-1;
		
	return cnt;
}


// ***************************************************************
// Hash table functions
// ***************************************************************

// **************************************************************
inline unsigned long hash_fun(unsigned char *p, int pos, int ver)
{
	unsigned long res = 0;
	
	if(ver == 1)
		for(int i = 0; i < HASH_LEN1; ++i)
			res = res * 65531u + p[pos+i] * 29u;
	else
		for(int i = 0; i < HASH_LEN2; ++i)
			res = res * 65531u + p[pos+i] * 29u;
		
	return res & ht_slot_size_mask;
}

// ***************************************************************
inline pair<int, int> find_match(unsigned char *p, int pos, int ver)
{
	
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
void insert_into_ht(file_id_t file_id, unsigned char *p, int ver)
{
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
void prepare_ht(void)
{
	// set the appropriate numbet of hashtable slots
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
// I/Ofunctions
// ***************************************************************

// ***************************************************************
unsigned char* read_file(string &name)
{
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
	fread(d, 1, file_size, in);
	fclose(in);

	return d;
}

// ***************************************************************
// Prepare output files
bool prepare_files(string output_name)
{
	
	rc_file   = fopen((output_name + ".tgc_data").c_str(), "wb");
	desc_file = fopen((output_name + ".tgc_desc").c_str(), "wt");

	fprintf(desc_file, "%d\n", file_size);
	// Create the vectors to store the phrase endings
	
	phraseEnd = new RSDic[no_files];
	phraseLiteral = new RSDic[no_files];
	phraseSourceSize = new RSDic[no_files];

	// Always true for the reference vector, as we are storing it directly.
	for( int j = 0;j < file_size; j++)
		bvb.PushBack( true );
	bvb.Build(phraseEnd[0]);
	
	return rc_file && desc_file;
}

// ***************************************************************
// Close output files
void close_files(void)
{
	if(rc_file)
		fclose(rc_file);
	if(desc_file)
		fclose(desc_file);
	if(out_file)
		fclose(out_file);
}


// ***************************************************************
// Compression functions
// ***************************************************************


// ***************************************************************
void parse_file(unsigned char * d, int file_id)
{
	cout << "Parsing file id: " << file_id << endl;
	int n_files = data.size();
	int best_len;
	int best_id;
	
	string filename = file_names[file_id];
	ofstream out_file(( (string)phraseParmsDir+"/"+ filename + ".parms"), ios::out | ios::binary | ios::trunc );
	

	bool phrase[file_size];// = {false};//(false);
	fill_n(phrase, file_size, false);
	vector<bool> phrase_literal;
	vector<bool> phrase_source_size;


	pos = 0;
	while(pos < file_size)
	{
		pair<int, int> match = find_match(d, pos, 1);
	//	cout << match.first << ", " << match.second << endl;
		if(match.second < HASH_LEN1 + HASH_STEP1 - 1)
			match = find_match(d, pos, 2);
		
		
		// if(match.first < 0)
		// {
		// 	store_literal(d[pos]);
		// 	pos++;
		// }
		// else
		// {
			//cout << "it is a match" << endl;
			//if(match.second > MAX_MATCH_LEN)
			//	match.second = MAX_MATCH_LEN;
	
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
			
			//cerr <<  match.first <<  match.second <<  phrase_index << d[pos] << endl;;
			//fprintf(fileptr, "%d %d %d\n", match.first, phrase_index,d[pos]);
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
		// }
	}
	
	createBitVector(phrase,file_id);
	createLiteralBitVector(phrase_literal,file_id);
	createSourceSizeBitVector(phrase_source_size,file_id);
	out_file.close();
}


// ***************************************************************
// The actual tgc compression
void compress(void)
{
	unsigned char *d; // represents the data file
	
	for(int i = 0; i < file_names.size(); ++i) // iterating over files
	{
		// If we are not able to open the file the move ahead to the next one
		// Also checks if the file sizes are consistent
		if((d = read_file(file_names[i])) == NULL)
			continue;

		// Store d in the data vector. but why?
		data.push_back(d);
		// data would be a really big vector :-o as it is basically handling the whole data.

		// Here we start two threads to hash the file using two hash tables
		// This syntax is something new to C++ some kind of lambda function stuff.
		// no need to worry about that right now
		thread t1([&]{insert_into_ht(cur_id_file, d, 1);});
		thread t2([&]{insert_into_ht(cur_id_file, d, 2);});

		// If i = 0 , we are processing the first file, so no hashing here
		// If i != 0, then we are parsing the file with respect to the hash functions and then storing

		// This parsing is what we need to change for LZ-End I think.
		if(i)
			parse_file(d, i);
		else
			;// store_ref_file


		t1.join();
		t2.join();
		++cur_id_file;
	}
	
	output_all_succint_bv_files();
	
	//filebuf fb;
  	
	// for (int i = 0 ; i < no_files ; i++)
	// {
	// 	filebuf fb;
	// 	string filename = file_names[i];
	// 	filename = ("bv/bv_" + filename + ".bv"); 
	// 	fb.open(filename.c_str(),std::ios::out);
	// 	cout << "wrote file: " <<  i <<  endl;
	// 	ostream os(&fb);
	// 	phraseEnd[i].Save(os);
	// 	fb.close();
	// }


	// for (int i = 0 ; i < no_files ; i++)
	// {
	// 	filebuf fb;
	// 	string filename = file_names[i];
	// 	filename = ("bv1/bv1_" + filename + ".bv"); 
	// 	fb.open(filename.c_str(),std::ios::out);
	// 	cout << "wrote file: " <<  i <<  endl;
	// 	ostream os(&fb);
	// 	phraseLiteral[i].Save(os);
	// 	fb.close();
	// }

	// for (int i = 0 ; i < no_files ; i++)
	// {
	// 	filebuf fb;
	// 	string filename = file_names[i];
		
	// 	filename = ("bv2/bv2_" + filename + ".bv"); 
	// 	fb.open(filename.c_str(),std::ios::out);
	// 	cout << "wrote file: " <<  i <<  endl;
	// 	ostream os(&fb);
	// 	phraseSourceSize[i].Save(os);
	// 	fb.close();
	// }

}


// ***************************************************************
// Outputs the bv files
// ***************************************************************
void output_bv_files(RSDic* succint_bv_dict, string write_dir)
{
	filebuf fb;
	string filename = (write_dir  + ".succint_bv"); 
	fb.open(filename.c_str(),std::ios::out);
	ostream os(&fb);

	for (int i = 0 ; i < no_files ; i++)
	{
		cout << "wrote file: " <<  i <<  endl;
		succint_bv_dict[i].Save(os);
	}
	fb.close();
}

// ***************************************************************
// Outputs all the relevant succint bv files
// ***************************************************************
void output_all_succint_bv_files()
{
	output_bv_files(phraseEnd, phraseEndDir);
	output_bv_files(phraseLiteral, phraseLiteralDir);
	output_bv_files(phraseSourceSize, phraseSourceSizeDir);
}


// ***************************************************************
// Main program
// ***************************************************************

// ***************************************************************
// Check parameters, input files etc.
bool check_data(int argc, char *argv[])
{
	if((argc < 2))
	{
		cout << "Usage: tgc output_file_prefix [list_file_name]\n";
		cout << "  list_file_name - name of the file with the list of files to compress\n";
		return false;
	}

	ifstream inf(argv[2]);
	istream_iterator<string> inf_iter(inf);
	file_names.assign(inf_iter, istream_iterator<string>());

	no_files = file_names.size();
	//no_files = 20;
    
	if(!no_files)
	{
		cout << "There are no files to process\n";
		return false;
	}

	
	// Check size of the first file - it is assummed that all files have the same size!
	FILE *in = fopen(file_names[0].c_str(), "rb");
	if(!in)
	{
		cout << "The reference file " << file_names[0] << " does not exist\n";
		return false;
	}

	fseek(in, 0, SEEK_END);
	file_size = (int) ftell(in);
	fclose(in);
	
	return true;
}

// ***************************************************************
// ***************************************************************
int main(int argc, char *argv[])
{
	clock_t t1 = clock();

	if(!check_data(argc, argv))
		return 0;
		
	if(!prepare_files(string(argv[1])))
		return 0;
	


	cout << "Initializing\n";
	prepare_ht();

	cout << "Compressing...\n";
	compress();

	close_files();
	
	
	cout << "\nTime: " << (double) (clock() - t1) / CLOCKS_PER_SEC << "\n";
	
	return 0;
}
