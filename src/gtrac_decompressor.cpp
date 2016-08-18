#include <iostream>     // std::cout, std::ostream, std::ios
#include <stdlib.h>     /* atoi */
#include "decompressor.h"

using namespace std;

// ***************************************************************
// 
// ***************************************************************
void help_msg()
{
    cout << "Usage: tgc <mode> <output_name> [list_file_name]\n";
    cout << "  mode               - c (column decompress), d (decompress), f(fast row decompress)  \n";
    cout << "  output_prefix      - The output prefix\n";
    cout << "  list_file_name     - name of the file with the list of files to decompress\n";
    cout << "  file_to_decompress - index of the file to decompress\n";
    cout << "  start_index        - start index of the substring to uncompress\n";
    cout << "  len_to_uncompress  - length to uncompress\n";
}

// ***************************************************************
// Compression functions
// ***************************************************************
int main(int argc, char** argv)
{
    int column_num;
    int file_id;

	if((argc < 5))
	{
		help_msg();
        return false;
	}

    decompressor gtrac_decomp;
    gtrac_decomp.initialize_decompressor(argv[3], (string)argv[2]);

    switch ( *argv[1]) {
        case 'c':
            cout << "Column Extraction..." <<endl;
		    column_num = atoi(argv[3]);
		    cout << "Column to extract: " << column_num << endl;
            gtrac_decomp.perform_column_decomp(column_num);
            break;
        case 'f':
            cout << "Fast Row Extraction..." <<endl;
            file_id = atoi(argv[4]);
            cout << "file_id: " << file_id << endl;
            gtrac_decomp.perform_row_decomp(file_id);
            break;
        case 'd':
            cout << "Subtring Extraction..." <<endl;
                
            if((argc < 7))
            {
                help_msg();
                return false;
            }
            
            file_id = atoi(argv[4]);
            int start = atoi(argv[4]);
            int len = atoi(argv[6]);
            cout << "file_id, start, len: " << file_id << ", " << start << ", " << len << endl;
            gtrac_decomp.perform_substring_decomp(file_id, start, len); 
            break;
    }
	return 0;
}
