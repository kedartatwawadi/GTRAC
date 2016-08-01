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
	bool check_data(char* path);
	// vector<unsigned char*> get_data_vector;

private:
	int num_files;
	vector<string> file_names;
	int file_size;

	// vector<unsigned char*> data;
};

#endif /* INPUT_DATA_H_ */