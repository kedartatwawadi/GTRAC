#include "input_data.h"
using namespace std;


// ***************************************************************
// Default Constructor
// ***************************************************************
input_data::input_data(){
	num_files = 0;
	file_size = 0;
}

// ***************************************************************
// Initializes file_size, file_names, num_files
// ***************************************************************
input_data::input_data(char* path){
	check_data(path);
}

// ***************************************************************
// Check parameters, input files etc.
// ***************************************************************
bool input_data::check_data(char* path)
{
    cout << "Input data prefix: " << path << endl;
	ifstream inf(path);
	istream_iterator<string> inf_iter(inf);
	file_names.assign(inf_iter, istream_iterator<string>());

	num_files = file_names.size();
    cout << "num files is: " << num_files << endl;
	if(!num_files)
	{
		cout << "There are no files to process\n";
		return false;
	}

	// Check size of the first file - it is assummed that all files have the same size!
	// TODO: check that the file sizes are all equal
	FILE *in = fopen(file_names[0].c_str(), "rb");
	if(!in)
	{
		cout << "The reference file " << file_names[0] << " does not exist\n";
		return false;
	}

	fseek(in, 0, SEEK_END);
	file_size = (int) ftell(in);
	fseek(in, 0, SEEK_SET);
    

    reference_file = new unsigned char[file_size];
    int ret = fread(reference_file, 1, file_size, in);
    fclose(in);

    num_symbols = file_size/2;
    if( file_size % 2 == 1 )
    {
        padding = 1;
        num_symbols = num_symbols + 1;
    }
	
    cout << "file_size: " << file_size << endl;
    cout << "num_symbols: " << num_symbols << endl;

	return true;
}


// ***************************************************************
// Check parameters, input files etc.
// ***************************************************************
bool input_data::read_metadata(string metadata_file_name, string ref_file_name)
{
    cout << "Metadata file: " << metadata_file_name << endl;
	ifstream inf(metadata_file_name.c_str());
	istream_iterator<string> inf_iter(inf);
	file_names.assign(inf_iter, istream_iterator<string>());

	num_files = file_names.size();
    cout << "num files is: " << num_files << endl;
	if(!num_files)
	{
		cout << "There are no files to process\n";
		return false;
	}

	FILE *in = fopen(ref_file_name.c_str(), "rb");
	if(!in)
	{
		cout << "The reference file does not exist\n";
		return false;
	}

	fseek(in, 0, SEEK_END);
	file_size = (int) ftell(in);
	fseek(in, 0, SEEK_SET);
    

    reference_file = new unsigned char[file_size];
    int ret = fread(reference_file, 1, file_size, in);
    fclose(in);

    num_symbols = file_size/2;
    if( file_size % 2 == 1 )
    {
        padding = 1;
        num_symbols = num_symbols + 1;
    }
	
    cout << "file_size: " << file_size << endl;
    cout << "num_symbols: " << num_symbols << endl;

	return true;
}




// ***************************************************************
// Getters
// ***************************************************************
int input_data::get_file_size(){
	return file_size;
}

int input_data::get_num_files(){
	return num_files;
}

int input_data::get_num_symbols(){
	return num_symbols;
}

int input_data::get_padding_flag(){
	return padding;
}

vector<string> input_data::get_file_names(){
	return file_names;
}

unsigned char* input_data::get_reference_file(){
    return reference_file;
}
