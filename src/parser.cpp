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
    prepare_ht();
}

// ***************************************************************
// Prepare the hash tables
// ***************************************************************
void parser::prepare_ht(void)
{
	// set the appropriate numbet of hashtable slots

	for(ht_slot_size_exp = 5; num_files * HT_FACTOR > (1u << ht_slot_size_exp); ht_slot_size_exp++);

	ht_slot_size      = 1u << ht_slot_size_exp;
	ht_slot_size_mask = ht_slot_size-1; // the mask is used later to find the mod with respect to this value .. this is a power of 2 
	                                    // so this works so awesomely! 
	
	ht_slots1	      = (unsigned long) (file_size / HASH_STEP1 + 1);
	ht_slots2	      = (unsigned long) (file_size / HASH_STEP2 + 1);
	ht_size1          = ht_slot_size * ht_slots1;
	ht_size2          = ht_slot_size * ht_slots2;
	
	// Need to understand this, but not very improtant
	cout << "HT sizes: " << ht_size1 / (1<<20)*sizeof(short) + ht_size2 / (1<<20)*sizeof(short) << "MB\n";

	ht1 = new file_id_t[ht_size1];
	fill(ht1, ht1+ht_size1, -1);
	ht2 = new file_id_t[ht_size2];
	fill(ht2, ht2+ht_size2, -1);

	// set counter of entries with hash value = 0
	// The hash table construction is kind of okay as of this point.
	// but not clear why this ht_zeros has been defined.
	// It is just one value per hash_table ( as in one value per hash table which belongs to ht1 or ht2)
	ht_zeros1 = new file_id_t[ht_slots1];
	fill(ht_zeros1, ht_zeros1+ht_slots1, 0);
	ht_zeros2 = new file_id_t[ht_slots2];
	fill(ht_zeros2, ht_zeros2+ht_slots2, 0);
}


// ***************************************************************
// Hash function
// ***************************************************************
inline unsigned long parser::hash_fun(unsigned char *p, int pos, int ver)
{
	unsigned long res = 0;
	
    // ver: corresponds to the type of hash function in use
	if(ver == 1)
		for(int i = 0; i < HASH_LEN1; ++i)
			res = res * 65531u + p[pos+i] * 29u;
	else
		for(int i = 0; i < HASH_LEN2; ++i)
			res = res * 65531u + p[pos+i] * 29u;
		
	return res & ht_slot_size_mask;
}

// ***************************************************************
// Find the best match
// ***************************************************************
pair<int, int> parser::find_match_ht(unsigned char *p, int pos, int ver, file_id_t current_file_id, RSDic* phraseEnd)
{

	
	int best_id  = -1;
	int best_len = -1;
	int hash_step, hash_len;
	//cerr << "find_match" << endl;
	if(ver == HASH_TABLE_VER_1)
	{
		hash_step = HASH_STEP1;
		hash_len  = HASH_LEN1;
	}
	else
	{
		hash_step = HASH_STEP2;
		hash_len  = HASH_LEN2;
	}
			
	// round off pos to the nearest starting point of 
	int pos_norm = ((pos + hash_step - 1) / hash_step)* hash_step;
	
	if(pos_norm + hash_len > file_size)
		return make_pair(-1, -1);
		
	unsigned long h = hash_fun(p, pos_norm, ver);
	unsigned long long off = (pos_norm / hash_step) << (ht_slot_size_exp);
	short *ht;
	
	if(ver == HASH_TABLE_VER_1)
		ht = ht1;
	else
		ht = ht2;
	
	while(ht[off+h] != -1)
	{
		int file_id = ht[off+h];
		int i;
		int last_phrase_end_pos = 0;
		
		// since we only want to reference to previously sequenced stuff.
		if(file_id < current_file_id)
		{ 
			for(i = pos; i < file_size; ++i)
			{
				//cout << "Here" << endl;
				if(p[i] != data[file_id][i])
					break;
				if( phraseEnd[file_id].GetBit(i) )
					last_phrase_end_pos = i;
			}

			if( ( (last_phrase_end_pos - pos) > best_len ) && (last_phrase_end_pos > 0 ) )
			{
				
				best_len = last_phrase_end_pos - pos;
				best_id  = file_id;
			}	
		}
		
		// We go and check all the hashes until we get -1 .
		h = (h+1) & ht_slot_size_mask;
	}
	
	// if smaller than MIN_MATCH_LEN then we store it as a literal rt?
	if(best_len < 2)
		return make_pair(-1, -1);
	
	//cout << "Comparing with matching files:  "<< best_id << "," << pos<< endl;
	return make_pair(best_id, best_len);
}

// ***************************************************************
//
// ***************************************************************
pair<int, int> parser::find_match(unsigned char *p, int pos, file_id_t current_file_id, RSDic* phraseEnd)
{
	pair<int, int> match = find_match_ht(p, pos, HASH_TABLE_VER_1, current_file_id, phraseEnd);
	if(match.second < HASH_LEN1 + HASH_STEP1 - 1)
		match = find_match_ht(p, pos, HASH_TABLE_VER_2, current_file_id, phraseEnd);
    return match;
}

// ***************************************************************
//
// ***************************************************************
void parser::insert_into_ht(file_id_t file_id, unsigned char *p, int ver)
{

	unsigned int n_slot = 0;
	unsigned long long off = 0;
	
	int hash_len, hash_step;
	short *ht, *ht_zeros;
	
	if(ver == HASH_TABLE_VER_1)
	{
		hash_len  = HASH_LEN1;
		hash_step = HASH_STEP1;
		ht        = ht1;
		ht_zeros  = ht_zeros1;
	}
	else
	{
		hash_len  = HASH_LEN2;
		hash_step = HASH_STEP2;
		ht        = ht2;
		ht_zeros  = ht_zeros2;
	}
	
	for(int i = 0; i < file_size - hash_len + 1; i += hash_step, ++n_slot, off += ht_slot_size)
	{
		unsigned long h = hash_fun(p, i, ver);

		if(h <= ht_zeros[n_slot])		// insert counter of entries with hash_value = 0
			h = ht_zeros[n_slot]++;
	
		// If the bucket is already filled, put it into the next bucket
		while(ht[off+h] != -1)
			h = (h+1) & ht_slot_size_mask;

		ht[off+h] = file_id;
	}
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
    thread t1([&]{insert_into_ht(current_file_id, d, 1);});
    thread t2([&]{insert_into_ht(current_file_id, d, 2);});

    t1.join();
    t2.join();
}
                                 
