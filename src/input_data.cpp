#include "input_data.h"
using namespace std;

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
	int size_of_file = ftell(in);
	fseek(in, 0, SEEK_SET);

	unsigned char *d = new unsigned char[size_of_file];
	fread(d, 1, size_of_file, in);
	fclose(in);

	return d;
}


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
	

	ifstream inf(path);
	istream_iterator<string> inf_iter(inf);
	file_names.assign(inf_iter, istream_iterator<string>());

	num_files = file_names.size();
    
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
	fclose(in);

	string reference_filename = file_names[0];
	reference_file = read_file(reference_filename);
	
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

vector<string> input_data::get_file_names(){
	return file_names;
}

unsigned char* input_data::get_reference_file(){
    return reference_file;
}
