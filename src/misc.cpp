#include"misc.h"

inline int pop_count(unsigned int x);

void save_byte(int x);
int read_byte();
// ***************************************************************
void save_byte(int x, )
{
        putc(x, rc_file);
}

// ***************************************************************
int read_byte()
{
        int a = getc(rc_file);
        return a;
}


// **************************************************************
// Calculate number of set bits
inline int pop_count(unsigned int x)
{
        int cnt = 0;

        for(; x; ++cnt)
                x &= x-1;

        return cnt;
}

