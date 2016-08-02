#include <iostream>     // std::cout, std::ostream, std::ios
#include <fstream> 
#include <stdio.h>
#include <string>
#include <utility>
#include "input_data.h"
#include "compressor.h"

using namespace std;

// ***************************************************************
// Main program
// ***************************************************************
int main(int argc, char *argv[])
{
	clock_t t1 = clock();

	if((argc < 2))
	{
		cout << "Usage: gtrac_comp output_file_prefix [list_file_name]\n";
		cout << "  list_file_name - name of the file with the list of files to compress\n";
		return false;
	}

	// initialize the input_data class object
	input_data* gtrac_input;
	gtrac_input->check_data(argv[2]);
	string output_prefix(argv[1]);
	compressor gtrac_comp(gtrac_input, output_prefix);

	cout << "Compressing...\n";
	gtrac_comp.compress();
	
	cout << "\nTime: " << (double) (clock() - t1) / CLOCKS_PER_SEC << "\n";
	
	return 0;
}
