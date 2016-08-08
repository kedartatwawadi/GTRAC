#include"parser.h"


// ***************************************************************
// Hash table functions
// ***************************************************************
parser::parser(){
    file_size = 0;
    num_files = 0;
}

// ***************************************************************
// Parser constructor
// ***************************************************************
parser::parser(int file_size_t, int num_files_t){
    initialize_parser(file_size_t, num_files_t);
}


// ***************************************************************
// Initialize Parser
// ***************************************************************
void parser::initialize_parser(int file_size_t, int num_files_t){
    file_size = file_size_t;
    num_files = num_files_t;
    ht1.initialize_ht(num_files, file_size, HASH_LEN1, HT_FACTOR, HASH_STEP1);
    ht2.initialize_ht(num_files, file_size, HASH_LEN2, HT_FACTOR, HASH_STEP2);
}


// ***************************************************************
//
// ***************************************************************
pair<int, int> parser::find_match(unsigned char *p, int pos, file_id_t current_file_id, RSDic* phraseEnd)
{
	pair<int, int> match = ht1.find_match_ht(p, pos, current_file_id, phraseEnd, &data);
	if(match.second < HASH_LEN1 + HASH_STEP1 - 1)
		match = ht2.find_match_ht(p, pos, current_file_id, phraseEnd, &data);
    return match;
}



//***************************************************************
// Add stuff to the hash table
//***************************************************************

void parser::parse_file(unsigned char *d, file_id_t current_file_id)
{
    data.push_back(d);
    // data would be a really big vector :-o as it is basically handling the whole data.
    // Here we start two threads to hash the file using two hash tables
    // This syntax is something new to C++ some kind of lambda function stuff.
    // no need to worry about that right now
    thread t1([&]{ht1.insert_into_ht(current_file_id, d);});
    thread t2([&]{ht2.insert_into_ht(current_file_id, d);});

    t1.join();
    t2.join();
}
                                 
