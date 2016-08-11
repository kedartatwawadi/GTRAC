

#include <iostream>     // std::cout, std::ostream, std::ios
#include <utility>
#include "input_data.h"
#include "compressor.h"

using namespace std;

void help_msg()
{
    cout << "Usage: gtrac_comp output_file_prefix [list_file_name]\n";
    cout << "  output_file_prefix - Prefix to store the compressed files\n";
    cout << "  list_file_name     - name of the file with the list of files to compress\n";
}

// ***************************************************************
// Main program
// ***************************************************************
int main(int argc, char *argv[])
{
	clock_t t1 = clock();

	if((argc < 2))
	{
	    help_msg();
        return false;
	}

	// initialize the input_data class object
	input_data gtrac_input(argv[2]);
    input_data* gtrac_input_ptr = &gtrac_input;

    string output_prefix(argv[1]);
	compressor gtrac_comp(gtrac_input_ptr, output_prefix);

	cout << "Compressing...\n";
	gtrac_comp.compress();
	
	cout << "\nTime: " << (double) (clock() - t1) / CLOCKS_PER_SEC << "\n";
	
	return 0;
}
