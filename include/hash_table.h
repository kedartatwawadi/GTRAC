#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

#define HASH_LEN		    11
#define HT_FACTOR			2
#define MIN_MATCH_LEN		5
#define MAX_MATCH_LEN_EXP	11
#define MAX_MATCH_LEN		((1 << MAX_MATCH_LEN_EXP) + MIN_MATCH_LEN - 1)
#define HASH_STEP1			4
#define HASH_STEP2			(MIN_MATCH_LEN - HASH_LEN2 + 1)


using namespace std;
using namespace rsdic;
typedef short file_id_t;

class hash_table {
public:
    hash_table();
    hash_table();
    void initialize_ht(int hash_len, int ht_factor, int hash_step);
private:



}
