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
#include <list>
#include <utility>
#include <sstream>
#include <ctime>
#include <ratio>
#include <chrono>

#include "port.h"
#include "rsdic/RSDic.hpp"
#include "rsdic/RSDicBuilder.hpp"
#include "omp.h"


// defining macros for file access
#define phraseEndDir "bv/bv_"
#define phraseLiteralDir "bv1/bv1_"
#define phraseSourceSizeDir "bv2/bv2_"
#define phraseNewCharSizeDir "bv3/bv3_"
#define logFolderDir "log_folder_5.1/log_"


using namespace std;
using namespace rsdic;

RSDic* phraseEnd;
RSDic* phraseLiteral;
RSDic* phraseSourceSize;
RSDic* phraseNewCharSize;

int no_files;
int number_of_blocks = 4;
vector<string> file_names;
unsigned short* reference_file;
int file_size;
vector<unsigned short> data;
vector<vector<unsigned short> > block_data;
vector<unsigned short> column_data;



// ***************************************************************
// Read the Succint BitVectors from the files
// ***************************************************************
RSDic* readSuccintBitVectors(string bvDictDir)
{
	cout << bvDictDir << endl;
	RSDic* bvDict = new RSDic[no_files];
	for (int i = 0 ; i < no_files ; i++)
	{
		filebuf fb;
		string filename = file_names[i];
		filename = (bvDictDir + filename + ".bv"); 
		fb.open(filename.c_str(),std::ios::in);
		istream is(&fb);
		bvDict[i].Load(is);
		fb.close();
	}
	return bvDict;
}

// ***************************************************************
// basic function to make note of all the files
// ***************************************************************
void readFileNames(char* listfile_name)
{
	ifstream inf(listfile_name);
	istream_iterator<string> inf_iter(inf);
	file_names.assign(inf_iter, istream_iterator<string>());
	no_files = file_names.size();
}


// ***************************************************************
// generic function to read a file
// ***************************************************************
unsigned short* read_file(string &name)
{
	FILE *in = fopen(name.c_str(), "r");
	if(!in)
	{
		cout << "No file!: " << name << "\n";
		return NULL;
	}

	// Check size
	fseek(in, 0, SEEK_END);
	file_size = ftell(in);
	fseek(in, 0, SEEK_SET);

	// Since we are dealing with shorts now
	file_size = file_size/2;

	unsigned short *d = new unsigned short[file_size];
	fread(d, 1, file_size, in);
	fclose(in);

	return d;
}

// ***************************************************************
// Reads the reference bytevector
// ***************************************************************
void readReferenceVector()
{
	string reference_filename = file_names[0];
	reference_file = read_file(reference_filename);
}

// ***************************************************************
// Given the phrase_id, it gives the new character which we added
// ***************************************************************
unsigned short getNewCharforPhrase(int file_id, int phrase_id)
{
	string filename = file_names[file_id];
	ifstream file((logFolderDir + filename + ".txt"),ios::in | ios::binary);
	int rank_literal = phraseLiteral[file_id].Rank(phrase_id-1,true);
	int rank_newchar_size = phraseNewCharSize[file_id].Rank(phrase_id-1,true);
	int rank_source_size = phraseSourceSize[file_id].Rank(phrase_id-rank_literal-1,true);
	int position = rank_newchar_size*1 + 
				   (phrase_id-1-rank_newchar_size)*2 + 
				   rank_source_size*1 + 
				   (phrase_id-1-rank_literal-rank_source_size)*2;
	

	if( phraseLiteral[file_id].GetBit(phrase_id-1))
	{
		position = position ;
	}
	else
	{
		if( phraseSourceSize[file_id].GetBit(phrase_id-rank_literal-1) )
			position = position + 1;
		else
			position = position + 2;
	}
	
	// cout << "Position: " << position << " ";
	// cout << "rank_literal: " << rank_literal << " ";
	// cout << "rank_source_size: " << rank_source_size << endl;
	file.seekg(position,ios::beg);
	if( phraseNewCharSize[file_id].GetBit(phrase_id-1) )
	{
		unsigned char new_char_byte = 0;
		file.read((char*) &new_char_byte, sizeof(unsigned char));
		file.close();
		return (unsigned short)new_char_byte;
	}
	
	// If a 2-byte source, return the same
	unsigned short new_char = 0;
	file.read((char*) &new_char, sizeof(unsigned short));
    file.close();
    return new_char;
}

// ***************************************************************
// Returns the source file for the given phrase_id
// ***************************************************************
inline pair<int, int> getSourceforPhrase( int file_id, int phrase_id)
{
	string filename = file_names[file_id];
	ifstream file((logFolderDir + filename + ".txt"),ios::in | ios::binary);
	int rank_literal = phraseLiteral[file_id].Rank(phrase_id-1,true);
	int rank_source_size = phraseSourceSize[file_id].Rank(phrase_id-1-rank_literal,true);
	int rank_newchar_size = phraseNewCharSize[file_id].Rank(phrase_id-1,true);
	int position = rank_newchar_size*1 + 
				   (phrase_id-1-rank_newchar_size)*2 + 
				   rank_source_size*1 + 
				   (phrase_id-1-rank_literal-rank_source_size)*2;
	int p = 0;	
	file.seekg(position,ios::beg);

	// If a literal return -1, -1
	if( phraseLiteral[file_id].GetBit(phrase_id-1))
	{
		return make_pair(-1,-1);
	};
	
	// If a 2-byte source, return the same
	if( phraseSourceSize[file_id].GetBit(phrase_id-rank_literal-1) )
	{
		unsigned char f = 0;
		file.read((char*) &f, sizeof(unsigned char));
		file.close();
		//f = file_id - f;
		return make_pair((int)f,(int)p);
	}
	
	// If a 1-byte source, return the same
	unsigned short f = 0;
	file.read((char*) &f, sizeof(unsigned short));
    file.close();
    return make_pair((int)f,(int)p);
}


// ***************************************************************
// Select function. Causes some issues when the argumen is negative,
// so, created a new function.
// ***************************************************************
int select(int file_id, int phrase_id)
{
	int pos;
	if( phrase_id < 0 )
		pos = -1;
	else
		pos = phraseEnd[file_id].Select(phrase_id,true);
	return pos;
}


// ***************************************************************
// Original decompression function
// ***************************************************************
void extract(int file_id, int start, int len)
{
	if( len > 0)
	{
		if(file_id == 0) // if it is the reference file
		{
			for( int i = 0; i < len ; i++ )
			{
				data.push_back(reference_file[i+start]);
				//cout << (int)reference_file[i+start] << endl;
			}
			//cout << "file id is 0" << endl;
			//cout << file_id << " " << start << " " << len  << " :1" << endl;
		}
		else
		{	
			
			int end = start + len - 1;
			int p = phraseEnd[file_id].Rank(end,true);
			//cout << file_id << " " << start << " " << len  << " " << p << " :2" << endl;
			if(phraseEnd[file_id].GetBit(end) == true)
			{
				extract(file_id,start,len-1);
				//cout << file_id << " " << start << " " << len  << " :3" << endl;
				//cout << "end: " << end << endl;
				data.push_back( getNewCharforPhrase(file_id,p+1) );
				// cout <<  (int)getNewCharforPhrase(file_id,p+1) << endl;
			}
			else
			{
				//cout << file_id << " " << start << " " << len  << " " << p << " :5" << endl;

				int pos = select(file_id,p-1) + 1;
				//cout << "Pos: " << pos << endl;
				if( start < pos )
				{
					//cout << "Here";
					extract(file_id, start, pos-start);
					len = end-pos+1;
					start=pos;
					//cout << file_id << " " << start << " " << len  << " " << p << " :6" << endl;
				}
				pair<int, int> source = getSourceforPhrase( file_id, p+1);
				//cout << source.first << " " << source.second << endl;
				extract(source.first, start , len );
			}
		}
	}
}


// ***************************************************************
// Compression functions
// ***************************************************************
void extract_block(int file_id, int start, int len, int block_id)
{
	
	if( len > 0)
	{
		if(file_id == 0) // if it is the reference file
		{
			for( int i = 0; i < len ; i++ )
			{
				block_data[block_id].push_back(reference_file[i+start]);
				//cout << (int)reference_file[i+start] << endl;
			}
			//cout << "file id is 0" << endl;
			//cout << file_id << " " << start << " " << len  << " :1" << endl;
		}
		else
		{	
			
			int end = start + len - 1;
			int p = phraseEnd[file_id].Rank(end,true);
			//cout << file_id << " " << start << " " << len  << " " << p << " :2" << endl;
			if(phraseEnd[file_id].GetBit(end) == true)
			{
				extract_block(file_id,start,len-1,block_id);
				//cout << file_id << " " << start << " " << len  << " :3" << endl;
				//cout << "end: " << end << endl;
				block_data[block_id].push_back( getNewCharforPhrase(file_id,p+1) );
				//cout <<  (int)getNewCharforPhrase(file_id,p+1) << endl;
			}
			else
			{
				//cout << file_id << " " << start << " " << len  << " " << p << " :5" << endl;

				int pos = select(file_id,p-1) + 1;
				//cout << "Pos: " << pos << endl;
				if( start < pos )
				{
					//cout << "Here";
					extract_block(file_id, start, pos-start,block_id);
					len = end-pos+1;
					start=pos;
					//cout << file_id << " " << start << " " << len  << " " << p << " :6" << endl;
				}
				pair<int, int> source = getSourceforPhrase( file_id, p+1);
				//cout << source.first << " " << source.second << endl;
				extract_block(source.first, start , len, block_id);
			}
		}
	}
}


// ***************************************************************
// Compression functions
// ***************************************************************
void extractColumn(int column_no )
{
	column_data.push_back(reference_file[column_no]);
	for( int file_id=1;file_id < no_files ; file_id++)
	{
		//cout << file_id << " " << start << " " << len  << " " << p << " :2" << endl;
		int p = phraseEnd[file_id].Rank(column_no,true);	
		if(phraseEnd[file_id].GetBit(column_no) == true)
		{
			column_data.push_back( getNewCharforPhrase(file_id,p+1) );
		}
		else
		{
			pair<int, int> source = getSourceforPhrase( file_id, p+1);
			column_data.push_back(column_data[source.first]);
		}
	}
}


// ***************************************************************
// Compression functions
// ***************************************************************
void extractLong(int file_id,int start,int len)
{
	int factor = number_of_blocks;
	block_data.resize(factor);
	

	int start_pos = start;
	int end_pos = start+len-1;
	int default_len_block = len/factor;
	int len_block = default_len_block;

	cout << "Block length: " << default_len_block << endl;

	#pragma omp parallel for
	for( int i = 0; i < factor; i++)
	{
		
		// slight change in the len_bloc for the last block
		if( i == (factor-1) )
		{
			extract_block(file_id, default_len_block*i + start, end_pos - default_len_block*i - start + 1, i);
		}
		else
		{
			extract_block(file_id, default_len_block*i + start, default_len_block, i);
		}

		cout << "Start Pos: " << start_pos << "Block id:" << i << endl;

		
	}		
}

// ***************************************************************
// Testing the setup
// ***************************************************************
void setup_testing(int file_id, int start, int len)
{
	cout << "The setup is done" << endl;
	cout << "testing the binary vectors..." << endl;
	int pos = phraseEnd[file_id].Select(5,true);
	cout << "file_id: " << file_id << " phraseEnd position 5: " << pos << endl;

	short temp = 0;
	unsigned char* temp_char = (unsigned char*)&temp;
	for( int i = start ; i < (start+len) ; i++)
	{
		temp = getNewCharforPhrase(file_id, i);
		cout << "NewChar[" << file_id << ", " << i << " ]: " << (unsigned int)temp_char[0] << " " << (unsigned int)temp_char[1] << endl;	
	};

	for( int i = start ; i < (start+len) ; i++)
	{
		pair<int, int> source;
		source = getSourceforPhrase( file_id, i);
		cout << "Sourceid[" << file_id << ", " << i << " ]: " << (int)source.first << endl;	
	};
}

// ***************************************************************
// Compression functions
// ***************************************************************
int main(int argc, char** argv)
{
	using namespace std::chrono;

	int decomp_column = false;
	int fast_decomp = false;
	int test_setup = false;
	int column_no = -1;
	int file_id = -1;
	int start = -1;
	int len = -1;

	
	if((argc < 4) || (argc < 6 && strcmp(argv[1], "d") == 0 ) || (argc < 6 && strcmp(argv[1], "f") == 0))
	{
			cout << "Usage: tgc <mode> <output_name> [list_file_name]\n";
			cout << "  mode               - c (column decompress), d (decompress), f(fast decompress)  \n";
			cout << "  list_file_name     - name of the file with the list of files to compress or decompress\n";
			cout << "  file_to_uncompress - index of the file to uncompress\n";
			cout << "  start_index        - start index of the substring to uncompress\n";
		 	cout << "  len_to_uncompress  - length to uncompress\n";
			return false;
	}

	if(strcmp(argv[1], "c") == false)
	{
		decomp_column = true;
		column_no = atoi(argv[3]);
		cout << "Column to extract: " << column_no << endl;
	}
	else
	{
		file_id = atoi(argv[3]);
		start = atoi(argv[4]);
		len = atoi(argv[5]);
		cout << "file_id: " << file_id << endl;
		cout << "start: " << start << endl;
		cout << "len: " << len << endl;
	}

	if(strcmp(argv[1], "f") == false)
		fast_decomp = true;

	if(strcmp(argv[1], "t") == false)
		test_setup = true;
	

	readFileNames(argv[2]);
	phraseEnd = readSuccintBitVectors(phraseEndDir);
	phraseLiteral = readSuccintBitVectors(phraseLiteralDir);
	phraseSourceSize = readSuccintBitVectors(phraseSourceSizeDir);
	phraseNewCharSize = readSuccintBitVectors(phraseNewCharSizeDir);
	readReferenceVector();

	if( test_setup ) setup_testing(file_id, start, len);
				
	if( decomp_column == true)
	{
		cout << "Initiating Column Decompression ... \n";
		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		extractColumn(column_no);
		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
		cout << "\nTime: " << time_span.count() << "\n";
		
		copy(column_data.begin(), column_data.end(), std::ostream_iterator<int>(std::cout, " "));
	}
	else
	{
		if( fast_decomp)
		{
			cout << "Initiating Fast Decompression ... \n";
			high_resolution_clock::time_point t1 = high_resolution_clock::now();
			extractLong(file_id, 0, file_size);
			high_resolution_clock::time_point t2 = high_resolution_clock::now();
			duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
			cout << "\nTime: " << time_span.count() << "\n";
		}
		else if(!test_setup)
		{
			cout << "Initiating Regular Deocmpression ... \n";
			high_resolution_clock::time_point t1 = high_resolution_clock::now();
			extract(file_id, start, len);
			high_resolution_clock::time_point t2 = high_resolution_clock::now();
			duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
			cout << "\nTime: " << time_span.count() << "\n";		
		}
	}

	
	if(fast_decomp)
	{
		short temp = 0;
		unsigned char* temp_char = (unsigned char*)&temp;
		for(int j = 0 ; j < number_of_blocks ; j++ )
		{
			cout << "Block id: " << j << endl;
			for(int i = 0 ; i < block_data[j].size() ; i++ )
			{
			temp = block_data[j][i];
			cout << (unsigned int)temp_char[0] << " " << (unsigned int)temp_char[1] << " ";
			}
			cout << endl;
		}
		
	}
	else
	{	
		short temp = 0;
		unsigned char* temp_char = (unsigned char*)&temp;
		
		ofstream output_file("output2_" + file_names[file_id], ios::binary);
		for(int i = 0 ; i < data.size() ; i++ )
		{
			temp = data[i];
			cout << (unsigned int)temp_char[0] << " " << (unsigned int)temp_char[1] << " ";
			output_file << (unsigned char)temp_char[0] << (unsigned char)temp_char[1];
		}
		cout << endl;	
		output_file.close();
	}

	// ofstream output_file("substring_file", ios::binary);
	// copy(data.begin(), data.end(), std::ostream_iterator<unsigned char>(output_file));
	// output_file.close();

	return 0;
}
