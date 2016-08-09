#include "decompressor.h"

// ***************************************************************
// Read the Succint BitVectors from the files
// ***************************************************************
RSDic* decompressor::readSuccintBitVectors(string bvDictDir)
{
	//cout << bvDictDir << endl;
	int num_files = gtrac_input.get_num_files();
	
    RSDic* bvDict = new RSDic[num_files];
	filebuf fb;
	string filename = (bvDictDir + ".succint_bv"); 
	fb.open(filename.c_str(),std::ios::in);
	
	for (int i = 0 ; i < num_files ; i++)
	{
		
		istream is(&fb);
		bvDict[i].Load(is);
	}
	fb.close();
	return bvDict;
}



// ***************************************************************
// generic function to read a file
// ***************************************************************
unsigned char* decompressor::read_file(string &name)
{
	FILE *in = fopen(name.c_str(), "r");
	if(!in)
	{
		cout << "No file!: " << name << "\n";
		return NULL;
	}

	// Check size
	fseek(in, 0, SEEK_END);
	int size_of_file = ftell(in);
	fseek(in, 0, SEEK_SET);

	unsigned char *d = new unsigned char[size_of_file];
	int ret = fread(d, 1, size_of_file, in);
	fclose(in);

	return d;
}




// ***************************************************************
// Given the phrase_id, it gives the new character which we added
// ***************************************************************
unsigned char decompressor::getNewCharforPhrase(int file_id, int phrase_id)
{
	vector<string> file_names = gtrac_input.get_file_names();
	
    string filename = file_names[file_id];
	ifstream file(((string)phraseParmsDir + "/" + filename + ".parms"),ios::in | ios::binary);
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
int decompressor::getSourceforPhrase( int file_id, int phrase_id)
{
	vector<string> file_names = gtrac_input.get_file_names();	

    string filename = file_names[file_id];
	ifstream file(((string)phraseParmsDir + +"/"+ filename + ".parms"),ios::in | ios::binary);
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
int decompressor::select(int file_id, int phrase_id)
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
void decompressor::extract(int file_id, int start, int len)
{
	unsigned char* reference_file = gtrac_input.get_reference_file();	
    
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
void decompressor::extract_block(int file_id, int start, int len, int block_id)
{
	unsigned char* reference_file = gtrac_input.get_reference_file();		
    
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
void decompressor::extractColumn(int column_no )
{
	unsigned char* reference_file = gtrac_input.get_reference_file();		

    int num_files = gtrac_input.get_num_files();
	
    column_data.push_back(reference_file[column_no]);
	for( int file_id=1;file_id < num_files ; file_id++)
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
void decompressor::extractLong(int file_id,int start,int len)
{

	int factor = number_of_blocks;
	block_data.resize(factor);
	

	int start_pos = start;
	int end_pos = start+len-1;
	int default_len_block = len/factor;
	int len_block = default_len_block;

	//cout << "Block length: " << default_len_block << endl;

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

		//cout << "Start Pos: " << start_pos << "Block id:" << i << endl;

		
	}	
}

// ***************************************************************
// 
// ***************************************************************
void decompressor::initialize_decompressor(char* path)
{
    gtrac_input.check_data(path); 
	phraseEnd = readSuccintBitVectors(phraseEndDir);
	phraseLiteral = readSuccintBitVectors(phraseLiteralDir);
	phraseSourceSize = readSuccintBitVectors(phraseSourceSizeDir);

}

// ***************************************************************
// 
// ***************************************************************
void decompressor::perform_column_decomp(int column_num)
{
    cout << "Initiating Column Decompression ... \n";
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    extractColumn(column_num);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    cout << "\nTime: " << time_span.count() << "\n";

	copy(column_data.begin(), column_data.end(), std::ostream_iterator<int>(std::cout, " "));
}

// ***************************************************************
// 
// ***************************************************************

void decompressor::perform_row_decomp(int file_id)
{
	vector<string> file_names = gtrac_input.get_file_names();
    int file_size = gtrac_input.get_file_size();

    cout << "Initiating Fast Decompression ... \n";
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    extractLong(file_id, 0, file_size);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    cout << "\nTime: " << time_span.count() << "\n";

    unsigned char temp = 0;
    ofstream output_file((string)resultsDir+"/"+file_names[file_id]+".output", ios::binary);
    for(int j = 0 ; j < number_of_blocks ; j++ )
    {
        //cout << "Block id: " << j << endl;
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

// ***************************************************************
// 
// ***************************************************************
void decompressor::perform_substring_decomp(int file_id, int start, int len)
{
	vector<string> file_names = gtrac_input.get_file_names();
    
    cout << "Initiating Regular Decompression ... \n";
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    extract(file_id, start, len);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    cout << "\nTime: " << time_span.count() << "\n";		

    unsigned char temp = 0;
    ofstream output_file((string)resultsDir+"/"+file_names[file_id]+".output", ios::binary);
    for(int i = 0 ; i < data.size() ; i++ )
    {
        temp = data[i];
        output_file << temp;
    }
    cout << endl;	
    output_file.close();

}

