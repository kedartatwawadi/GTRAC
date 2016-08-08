#include"hash_table.h"

// ***************************************************************
// Default Constructor
// ***************************************************************

hash_table::hash_table()
{
    hash_len = 0;
    ht_factor = 0;
    hash_step = 0;
}

// ***************************************************************
// Constructor
// ***************************************************************
hash_table::hash_table(int num_files_t, int file_size_t, int hash_len_t, int ht_factor_t, int hash_step)
{
    initialize_ht(num_files_t, file_size_t, hash_len_t, ht_factor_t, hash_step);
}

// ***************************************************************
// Initialize the hash table
// ***************************************************************
void hash_table::initialize_ht(int num_files_t, int file_size_t, int hash_len_t, int ht_factor_t, int hash_step_t)
{
    num_files = num_files_t;
    file_size = file_size_t;
    hash_len = hash_len_t;
    ht_factor = ht_factor_t;
    hash_step = hash_step_t;
    
    prepare_ht();
}

// ***************************************************************
// Prepare the hash tables
// ***************************************************************
void hash_table::prepare_ht(void)
{
	// set the appropriate numbet of hashtable slots

	for(ht_slot_size_exp = 5; num_files * ht_factor > (1u << ht_slot_size_exp); ht_slot_size_exp++);

	ht_slot_size      = 1u << ht_slot_size_exp;
	ht_slot_size_mask = ht_slot_size-1; // the mask is used later to find the mod with respect to this value .. this is a power of 2 
	                                    // so this works so awesomely! 
	
	ht_slots	      = (unsigned long) (file_size / hash_step + 1);
	ht_size           = ht_slot_size * ht_slots;
	
	// Need to understand this, but not very improtant
	cout << "HT size: " << ht_size / (1<<20)*sizeof(short) << "MB\n";

	ht = new file_id_t[ht_size];
	fill(ht, ht+ht_size, -1);

	// set counter of entries with hash value = 0
	// The hash table construction is kind of okay as of this point.
	// but not clear why this ht_zeros has been defined.
	// It is just one value per hash_table ( as in one value per hash table which belongs to ht1 or ht2)
	ht_zeros = new file_id_t[ht_slots];
	fill(ht_zeros, ht_zeros+ht_slots, 0);
}


// ***************************************************************
// Hash function
// ***************************************************************
inline unsigned long hash_table::hash_fun(unsigned char *p, int pos)
{
	unsigned long res = 0;
	
	for(int i = 0; i < hash_len; ++i)
		res = res * 65531u + p[pos+i] * 29u;
		
	return res & ht_slot_size_mask;
}


// ***************************************************************
//
// ***************************************************************
void hash_table::insert_into_ht(file_id_t file_id, unsigned char *p)
{

	unsigned int n_slot = 0;
	unsigned long long off = 0;
	
	for(int i = 0; i < file_size - hash_len + 1; i += hash_step, ++n_slot, off += ht_slot_size)
	{
		unsigned long h = hash_fun(p, i);

		if(h <= ht_zeros[n_slot])		// insert counter of entries with hash_value = 0
			h = ht_zeros[n_slot]++;
	
		// If the bucket is already filled, put it into the next bucket
		while(ht[off+h] != -1)
			h = (h+1) & ht_slot_size_mask;

		ht[off+h] = file_id;
	}
}

// ***************************************************************
// Find the best match
// ***************************************************************
pair<int, int> hash_table::find_match_ht(unsigned char *p, int pos, file_id_t current_file_id, RSDic* phraseEnd, vector<unsigned char*>* data_ptr )
{
    vector<unsigned char*> data = *data_ptr;
	int best_id  = -1;
	int best_len = -1;
			
	// round off pos to the nearest starting point of 
	int pos_norm = ((pos + hash_step - 1) / hash_step)* hash_step;
	
	if(pos_norm + hash_len > file_size)
		return make_pair(-1, -1);
		
	unsigned long h = hash_fun(p, pos_norm);
	unsigned long long off = (pos_norm / hash_step) << (ht_slot_size_exp);
	
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
// Getters
// ***************************************************************
int hash_table::get_hash_len()
{
    return hash_len;
}

int hash_table::get_hash_step()
{
    return hash_step;
}
