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

#define phraseEndDir "bv_"
#define phraseLiteralDir "bv1_"
#define phraseSourceSizeDir "bv2_"

using namespace std;
using namespace rsdic;

RSDicBuilder bvb;
RSDic* phraseEnd;
RSDic* phraseLiteral;
RSDic* phraseSourceSize;

int no_files;
int number_of_blocks = 4;
vector<string> file_names;
unsigned char* reference_file;
int file_size;
vector<unsigned char> data;
vector<vector<unsigned char> > block_data;
vector<unsigned char> column_data;

// ***************************************************************
// Read the Succint BitVectors from the files
// ***************************************************************
RSDic* readSuccintBitVectors(string bvDictDir)
{
	cout << bvDictDir << endl;
	RSDic* bvDict = new RSDic[no_files];
	filebuf fb;
	string filename = (bvDictDir + "single_file" + ".bv"); 
	fb.open(filename.c_str(),std::ios::in);
	
	for (int i = 0 ; i < no_files ; i++)
	{
		
		istream is(&fb);
		bvDict[i].Load(is);
	}
	fb.close();
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
unsigned char* read_file(string &name)
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

	unsigned char *d = new unsigned char[file_size];
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
unsigned char getNewCharforPhrase(int file_id, int phrase_id)
{
	string filename = file_names[file_id];
	ifstream file(("log_folder_3/log_" + filename + ".txt"),ios::in | ios::binary);
	int rank_literal = phraseLiteral[file_id].Rank(phrase_id-1,true);
	int rank_source_size = phraseSourceSize[file_id].Rank(phrase_id-rank_literal-1,true);
	int position = (phrase_id-1)*1 + rank_source_size*1 + (phrase_id-1-rank_literal-rank_source_size)*2;
	

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
	
	// cout << "Position: " << position << endl;
	// cout << "rank_literal: " << rank_literal << endl;
	// cout << "rank_source_size: " << rank_source_size << endl;
	file.seekg(position,ios::beg);
	unsigned char phrase_char = (unsigned char)0;
	file.read((char*) &phrase_char, sizeof(unsigned char));
    
    file.close();
    return phrase_char;
}

// ***************************************************************
// Returns the source file for the given phrase_id
// ***************************************************************
int getSourceforPhrase( int file_id, int phrase_id)
{
	string filename = file_names[file_id];
	ifstream file(("log_folder_3/log_" + filename + ".txt"),ios::in | ios::binary);
	int rank_literal = phraseLiteral[file_id].Rank(phrase_id-1,true);
	int rank_source_size = phraseSourceSize[file_id].Rank(phrase_id-1-rank_literal,true);
	int position = (phrase_id-1)*1 + rank_source_size*1 + (phrase_id-1-rank_literal-rank_source_size)*2;
	int p = 0;	
	file.seekg(position,ios::beg);

	// If a literal return -1, -1
	if( phraseLiteral[file_id].GetBit(phrase_id-1))
	{
		return -1;
	};
	
	// If a 1-byte source, return the same
	if( phraseSourceSize[file_id].GetBit(phrase_id-rank_literal-1) )
	{
		unsigned char f = 0;
		file.read((char*) &f, sizeof(unsigned char));
		file.close();
		int source_file_id = (int)file_id - (int)f;
		return source_file_id;
	}
	
	// If a 1-byte source, return the same
	unsigned short f = 0;
	file.read((char*) &f, sizeof(unsigned short));
	int source_file_id = (int)file_id - (int)f;
    file.close();
    return source_file_id;
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
					extract(file_id, start, pos-start);
					len = end-pos+1;
					start=pos;
					//cout << file_id << " " << start << " " << len  << " " << p << " :6" << endl;
				}
				int source_file_id = getSourceforPhrase( file_id, p+1);
				//cout << source.first << " " << source.second << endl;
				extract(source_file_id, start , len );
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
				int source_file_id = getSourceforPhrase( file_id, p+1);
				//cout << source.first << " " << source.second << endl;
				extract_block(source_file_id, start , len, block_id);
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
			int source_file_id = getSourceforPhrase( file_id, p+1);
			column_data.push_back(column_data[source_file_id]);
		}
	}
}


// ***************************************************************
// Compression functions
// ***************************************************************
void extractLong(int file_id,int start,int len)
{
	// int init_p = phraseEnd[file_id].Rank(start,true);
	// int end_p = phraseEnd[file_id].Rank(start+len-1,true);
	// int factor = 4;
	// block_data.resize(factor);
	// data.resize(0);

	// #pragma omp parallel for
	// for( int i = 0; i < factor; i++)
	// {
	// 	int len_block = len/factor;
	// 	//block_data[i].resize(len_block);
	// 	extract_block(file_id, start+i*len_block, len_block,i);
	// 	data.insert(data.end(), block_data[i].begin() , block_data[i].end() );
	// }

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

// // ***************************************************************
// // Testing the setup
// // ***************************************************************
// void setup_testing(int file_id, int start, int len)
// {
// 	cout << "The setup is done" << endl;
// 	cout << "testing the binary vectors..." << endl;
// 	int pos = phraseEnd[file_id].Select(5,true);
// 	cout << "file_id: " << file_id << " phraseEnd position 5: " << pos << endl;

// 	unsigned char temp = 0;
// 	for( int i = start ; i < (start+len) ; i++)
// 	{
// 		temp = getNewCharforPhrase(file_id, i);
// 		cout << "NewChar[" << file_id << ", " << i << " ]: " << (unsigned int)temp << endl;	
// 	};

// 	for( int i = start ; i < (start+len) ; i++)
// 	{
// 		pair<int, int> source;
// 		source = getSourceforPhrase( file_id, i);
// 		cout << "Sourceid[" << file_id << ", " << i << " ]: " << (int)source.first << endl;	
// 	};
// }

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
	readReferenceVector();

	// if( test_setup ) setup_testing(file_id, start, len);
	

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
			cout << "Initiating Fast Deocmpression ... \n";
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


		if(fast_decomp)
		{
			unsigned char temp = 0;
			ofstream output_file("output_3_" + file_names[file_id], ios::binary);
			for(int j = 0 ; j < number_of_blocks ; j++ )
			{
				cout << "Block id: " << j << endl;
				for(int i = 0 ; i < block_data[j].size() ; i++ )
				{
					temp = block_data[j][i];
					output_file << temp;
				}
				//cout << endl;
				// copy(block_data[j].begin(), block_data[j].end(), std::ostream_iterator<unsigned short>(output_file));
			}
			output_file.close();
			
		}
		else
		{	
			unsigned char temp = 0;
			ofstream output_file("output_d3_" + file_names[file_id], ios::binary);
			for(int i = 0 ; i < data.size() ; i++ )
			{
				temp = data[i];
				output_file << temp;
			}
			cout << endl;	
			output_file.close();
		}
	}
	
	/************************** Output the results appropriately ***********************/
	// copy(data.begin(), data.end(), std::ostream_iterator<int>(std::cout, " "));
	// cout << endl;

	// ofstream output_file("output_3_" + file_names[file_id], ios::binary);
	// copy(data.begin(), data.end(), std::ostream_iterator<unsigned char>(output_file));
	// output_file.close();

	

	return 0;
}