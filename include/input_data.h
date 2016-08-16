#ifndef INPUT_DATA_H_
#define INPUT_DATA_H_

#include <iostream>     // std::cout, std::ostream, std::ios
#include <fstream> 
#include <stdio.h>
#include <string>
#include <iterator>
#include <vector>
#include <thread>


using namespace std;

class input_data {
public:
	input_data();
	input_data(char* path);

	int get_num_files();
	vector<string> get_file_names();
	int get_file_size();
	int get_padding_flag();
	int get_num_symbols();	
    bool check_data(char* path);
    bool read_metadata(string metadata_file_name, string ref_file_name);
    unsigned char* get_reference_file();

private:
	int num_files;
	vector<string> file_names;
	int file_size;
	unsigned char* reference_file;

    int padding;
    int num_symbols;

	// vector<unsigned char*> data;
};

#endif /* INPUT_DATA_H_ */
