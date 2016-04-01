#include <stdio.h>
#include <string.h>
#include <string>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <algorithm>
#include <iostream>
#include <map>
#include <thread>
#include <list>
#include <utility>
#include <thread>

#include "port.h"
#include "qsmodel.h"
#include "rangecod.h"

#define RC_SHIFT			15

#define RC_SCALE			(1u << 12)

//#define TURN_OFF_SNP
//#define TURN_OFF_INS
//#define TURN_OFF_DEL
//#define TURN_OFF_SV

using namespace std;

const int RC_CTX_POS = 8;
const int SIGMA      = 5;
const int N_TYPES    = 6;
const int N_POS      = 5;
const int N_LEN_DEL  = 3;
const int N_LEN_INS  = 3;

const int T_SNP      = 0;
const int T_INS1     = 1;
const int T_INS_LONG = 2;
const int T_DEL1     = 3;
const int T_DEL_LONG = 4;
const int T_SV       = 5;

rangecoder rc;
qsmodel 
	qsm_pos[N_POS],
	qsm_type[N_TYPES], 
	qsm_lens_del[N_LEN_DEL],
	qsm_lens_ins[N_LEN_INS],
	qsm_lens_sv[2],
	qsm_literals;

FILE *rc_file;		// output file (range coder compressed)

int dna_coding[256];
char *dna_uncoding = "ACGTNURYMKWSBDHV";

int ctx_type;

bool compress_mode;
string input_name, output_name;

// ***************************************************************
//bool read_file(string input_name);
void close_files(void);

bool compress(void);
void decompress(void);

void init_rc_models(int mode);
void delete_rc_models(void);

void prepare_DNA_coding();
int to_int(char c);
int from_int(int x);

// ***************************************************************
// Range coder interals
// ***************************************************************

// ***************************************************************
void save_byte(int x)
{
	putc(x, rc_file);
}

// ***************************************************************
int read_byte()
{
	int a = getc(rc_file);
	return a;
}

// ***************************************************************
void prepare_DNA_coding()
{
	fill(dna_coding, dna_coding+256, -1);
	
	for(int i = 0; i < strlen(dna_uncoding); ++i)
		dna_coding[dna_uncoding[i]] = i;
}

// ***************************************************************
int to_int(char c)
{
	int r = dna_coding[c];
	
	if(r < 0)
	{
		cout << "Wrong letter: " << int(c) << "\n";
		exit(0);
	}
	
	return r;	
}

// ***************************************************************
int from_int(int x)
{
	return dna_uncoding[x];
}


// ***************************************************************
// Initialize range coder models
void init_rc_models(int mode)
{
	// Delta positions
	for(int i = 0; i < N_POS; ++i)
		initqsmodel(&qsm_pos[i], 256, RC_SHIFT, RC_SCALE, NULL, mode);

	// Types
	for(int i = 0; i < N_TYPES; ++i)
		initqsmodel(&qsm_type[i], N_TYPES, RC_SHIFT, RC_SCALE, NULL, mode);

	// Literals
	initqsmodel(&qsm_literals, 16, RC_SHIFT, RC_SCALE, NULL, mode);
	
	// Lenghts
	for(int i = 0; i < N_LEN_DEL; ++i)
		initqsmodel(&qsm_lens_del[i], 256, RC_SHIFT, RC_SCALE, NULL, mode);
	for(int i = 0; i < N_LEN_INS; ++i)
		initqsmodel(&qsm_lens_ins[i], 256, RC_SHIFT, RC_SCALE, NULL, mode);

	for(int i = 0; i < 2; ++i)
		initqsmodel(&qsm_lens_sv[i], 256, RC_SHIFT, RC_SCALE, NULL, mode);
}

// ***************************************************************
// Delete range coder models
void delete_rc_models(void)
{
	// Delta positions
	for(int i = 0; i < N_POS; ++i)
		deleteqsmodel(&qsm_pos[i]);

	// Types
	for(int i = 0; i < N_TYPES; ++i)
		deleteqsmodel(&qsm_type[i]);

	// Literals
	deleteqsmodel(&qsm_literals);
	
	// Lenghts
	for(int i = 0; i < N_LEN_DEL; ++i)
		deleteqsmodel(&qsm_lens_del[i]);
	for(int i = 0; i < N_LEN_INS; ++i)
		deleteqsmodel(&qsm_lens_ins[i]);

	for(int i = 0; i < 2; ++i)
		deleteqsmodel(&qsm_lens_sv[i]);
}

// ***************************************************************
// Prepare output files
bool prepare_files(string output_name)
{
	if(compress_mode)
		rc_file = fopen(output_name.c_str(), "wb");
	else
		rc_file = fopen(input_name.c_str(), "rb");

	return rc_file;
}

// ***************************************************************
bool compress(void)
{
	int syfreq, ltfreq;

	prepare_DNA_coding();
	init_rc_models(1);
	start_encoding(&rc, 0, 0);
	
	FILE *in = fopen(input_name.c_str(), "rb");
	if(!in)
	{
		cout << "No file: " << input_name << "\n";
		return NULL;
	}

	char *line = new char[1 << 20];
	int id, pos;
	char type[16];
	string s_type;
	int delta = 0;
	int last_pos = 0;

	ctx_type = 0;
	
	while(!feof(in))
	{
		if(fscanf(in, "%d%d%s", &id, &pos, type) == EOF)
			break;
		s_type = string(type);
		delta = pos - last_pos;
		last_pos = pos;
		
		if(id % 100000 == 0)
			cout << id << " " << pos << "\n";
			
#ifdef TURN_OFF_SNP
		if(s_type == "SNP")
		{
			fgets(line, 1 << 20, in);
			continue;
		}
#endif
#ifdef TURN_OFF_INS
		if(s_type == "INS")
		{
			fgets(line, 1 << 20, in);
			continue;
		}
#endif
#ifdef TURN_OFF_DEL
		if(s_type == "DEL")
		{
			fgets(line, 1 << 20, in);
			continue;
		}
#endif
#ifdef TURN_OFF_SV
		if(s_type == "SV")
		{
			fgets(line, 1 << 20, in);
			continue;
		}
#endif
			
		// Store delta pos
		if(delta < 252)	// small deltas
		{
			qsgetfreq(&qsm_pos[0], delta, &syfreq, &ltfreq);
			encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
			qsupdate(&qsm_pos[0], delta);
		}
		else 			// large deltas
		{
			int delta_bytes;
			if(delta < (256u << 8))
			{
				qsgetfreq(&qsm_pos[0], 252, &syfreq, &ltfreq);
				encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
				qsupdate(&qsm_pos[0], 252);
				delta_bytes = 2;					
			}
			else if(delta < (256u << 16))
			{
				qsgetfreq(&qsm_pos[0], 253, &syfreq, &ltfreq);
				encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
				qsupdate(&qsm_pos[0], 253);
				delta_bytes = 3;					
			}
			else 
			{
				qsgetfreq(&qsm_pos[0], 254, &syfreq, &ltfreq);
				encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
				qsupdate(&qsm_pos[0], 254);
				delta_bytes = 4;
			}
			
			for(int i = 0; i < delta_bytes; ++i)
			{
				qsgetfreq(&qsm_pos[i+1], delta & 0xFF, &syfreq, &ltfreq);
				encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
				qsupdate(&qsm_pos[i+1], delta & 0xFF);
				delta >>= 8;
			}
		}

		// Encode data
		if(s_type == "SNP")
		{
			qsgetfreq(&qsm_type[ctx_type], T_SNP, &syfreq, &ltfreq);
			encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
			qsupdate(&qsm_type[ctx_type], T_SNP);

			fscanf(in, "%s", line);
			qsgetfreq(&qsm_literals, to_int(line[0]), &syfreq, &ltfreq);
			encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
			qsupdate(&qsm_literals, to_int(line[0]));
			
			ctx_type = T_SNP;
		}
		else if(s_type == "INS")
		{
			fscanf(in, "%s", line);
			int len = strlen(line);
			
			if(len == 1)		// 1-symbol insertion
			{
				qsgetfreq(&qsm_type[ctx_type], T_INS1, &syfreq, &ltfreq);
				encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
				qsupdate(&qsm_type[ctx_type], T_INS1);

				qsgetfreq(&qsm_literals, to_int(line[0]), &syfreq, &ltfreq);
				encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
				qsupdate(&qsm_literals, to_int(line[0]));

				ctx_type = T_INS1;
			}
			else				// longer insertion
			{
				qsgetfreq(&qsm_type[ctx_type], T_INS_LONG, &syfreq, &ltfreq);
				encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
				qsupdate(&qsm_type[ctx_type], T_INS_LONG);
			
				if(len < 255)
				{
					qsgetfreq(&qsm_lens_ins[0], len, &syfreq, &ltfreq);
					encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
					qsupdate(&qsm_lens_ins[0], len);
				}		
				else
				{
					cout << "Very long insertion\n";
					exit(0);
				}

				for(int i = 0; i < len; ++i)
				{
					qsgetfreq(&qsm_literals, to_int(line[i]), &syfreq, &ltfreq);
					encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
					qsupdate(&qsm_literals, to_int(line[i]));
				}

				ctx_type = T_INS_LONG;
			}
		}
		else if(s_type == "DEL")
		{
			int len;
			fscanf(in, "%d", &len);
			
			if(len == 1)			// 1-symbol deletion
			{
				qsgetfreq(&qsm_type[ctx_type], T_DEL1, &syfreq, &ltfreq);
				encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
				qsupdate(&qsm_type[ctx_type], T_DEL1);
				ctx_type = T_DEL1;
			}
			else
			{
				qsgetfreq(&qsm_type[ctx_type], T_DEL_LONG, &syfreq, &ltfreq);
				encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
				qsupdate(&qsm_type[ctx_type], T_DEL_LONG);
			
				if(len < 255)
				{
					qsgetfreq(&qsm_lens_del[0], len, &syfreq, &ltfreq);
					encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
					qsupdate(&qsm_lens_del[0], len);
				}		
				else
				{
					cout << "Very long deletion\n";
					exit(0);
				}
				ctx_type = T_DEL_LONG;			
			}
		}
		else if(s_type == "SV")
		{
			int len_del;
			fscanf(in, "%d%s", &len_del, line);
			int len_ins = strlen(line);
			
			qsgetfreq(&qsm_type[ctx_type], T_SV, &syfreq, &ltfreq);
			encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
			qsupdate(&qsm_type[ctx_type], T_SV);

			for(int i = 0; i < 3; ++i)
			{
				qsgetfreq(&qsm_lens_sv[0], len_del & 0xFF, &syfreq, &ltfreq);
				encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
				qsupdate(&qsm_lens_sv[0], len_del & 0xFF);
				len_del >>= 8;
			}		
		
			if(line[0] == '-')
				len_ins = 0;
				
			int len = len_ins;
			for(int i = 0; i < 3; ++i)
			{
				qsgetfreq(&qsm_lens_sv[1], len & 0xFF, &syfreq, &ltfreq);
				encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
				qsupdate(&qsm_lens_sv[1], len & 0xFF);
				len >>= 8;
			}		

			for(int i = 0; i < len_ins; ++i)
			{
				qsgetfreq(&qsm_literals, to_int(line[i]), &syfreq, &ltfreq);
				encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
				qsupdate(&qsm_literals, to_int(line[i]));
			}

			ctx_type = T_SV;
		}
		else		
		{
			cout << "Wrong type: " << s_type << "\n";
			exit(0);
		}
	}
	
	delete[] line;
	
	fclose(in);
	
	// EOF
	qsgetfreq(&qsm_pos[0], 255, &syfreq, &ltfreq);
	encode_shift(&rc, syfreq, ltfreq, RC_SHIFT);
	qsupdate(&qsm_pos[0], 255);
	
	done_encoding(&rc);
	delete_rc_models();

	return true;
}

// ***************************************************************
void decompress(void)
{
	int syfreq, ltfreq;

	init_rc_models(0);				// decompression mode (0)
	start_decoding(&rc);

	FILE *out = fopen(output_name.c_str(), "wb");
	if(!out)
	{
		cout << "Cannot create output file\n";
		exit(0);
	}
	
	char *line = new char[1 << 20];
	int id, pos;
	char type[16];
	string s_type;
	int delta = 0;
	int last_pos = 0;
	int t_type = 0;

	ctx_type = 0;
	id = 0;

	while(true)
	{
		// Decode delta
		if(++id % 100000 == 0)
			cout << id << "\n";

		ltfreq = decode_culshift(&rc, RC_SHIFT);
		delta = qsgetsym(&qsm_pos[0], ltfreq);
		qsgetfreq(&qsm_pos[0], delta, &syfreq, &ltfreq);
		decode_update(&rc, syfreq, ltfreq, 1u << RC_SHIFT);
		qsupdate(&qsm_pos[0], delta);
		
		if(delta == 255)		// EOF
			break;
		// Decode delta pos
		if(delta < 252)	// small deltas
		{
			;	// do nothing - delta uncoded
		}
		else 			// large deltas
		{
			int delta_bytes = delta - 250;
			delta = 0;
			int x;
			
			for(int i = 0; i < delta_bytes; ++i)
			{
				ltfreq = decode_culshift(&rc, RC_SHIFT);
				x = qsgetsym(&qsm_pos[i+1], ltfreq);
				qsgetfreq(&qsm_pos[i+1], x, &syfreq, &ltfreq);
				decode_update(&rc, syfreq, ltfreq, 1u << RC_SHIFT);
				qsupdate(&qsm_pos[i+1], x);

				delta += x << (i*8);
			}
		}
		pos = last_pos + delta;
		last_pos = pos;
		
		fprintf(out, "%d %d", id, pos);

		// Decode type
		ltfreq = decode_culshift(&rc, RC_SHIFT);
		t_type = qsgetsym(&qsm_type[ctx_type], ltfreq);
		qsgetfreq(&qsm_type[ctx_type], t_type, &syfreq, &ltfreq);
		decode_update(&rc, syfreq, ltfreq, 1u << RC_SHIFT);
		qsupdate(&qsm_type[ctx_type], t_type);
		
		if(t_type == T_SNP)
		{
			ltfreq = decode_culshift(&rc, RC_SHIFT);
			int c = qsgetsym(&qsm_literals, ltfreq);
			qsgetfreq(&qsm_literals, c, &syfreq, &ltfreq);
			decode_update(&rc, syfreq, ltfreq, 1u << RC_SHIFT);
			qsupdate(&qsm_literals, c);

			fprintf(out, " SNP %c\n", from_int(c));
		}
		else if(t_type == T_DEL1)
		{
			fprintf(out, " DEL 1\n");
		}
		else if(t_type == T_DEL_LONG)
		{
			ltfreq = decode_culshift(&rc, RC_SHIFT);
			int len = qsgetsym(&qsm_lens_del[0], ltfreq);
			qsgetfreq(&qsm_lens_del[0], len, &syfreq, &ltfreq);
			decode_update(&rc, syfreq, ltfreq, 1u << RC_SHIFT);
			qsupdate(&qsm_lens_del[0], len);
			
			fprintf(out, " DEL %d\n", len);
		}		
		else if(t_type == T_INS1)
		{
			ltfreq = decode_culshift(&rc, RC_SHIFT);
			int c = qsgetsym(&qsm_literals, ltfreq);
			qsgetfreq(&qsm_literals, c, &syfreq, &ltfreq);
			decode_update(&rc, syfreq, ltfreq, 1u << RC_SHIFT);
			qsupdate(&qsm_literals, c);

			fprintf(out, " INS %c\n", from_int(c));
		}
		else if(t_type == T_INS_LONG)
		{
			ltfreq = decode_culshift(&rc, RC_SHIFT);
			int len = qsgetsym(&qsm_lens_ins[0], ltfreq);
			qsgetfreq(&qsm_lens_ins[0], len, &syfreq, &ltfreq);
			decode_update(&rc, syfreq, ltfreq, 1u << RC_SHIFT);
			qsupdate(&qsm_lens_ins[0], len);

			fprintf(out, " INS ");
			for(int i = 0; i < len; ++i)
			{
				ltfreq = decode_culshift(&rc, RC_SHIFT);
				int c = qsgetsym(&qsm_literals, ltfreq);
				qsgetfreq(&qsm_literals, c, &syfreq, &ltfreq);
				decode_update(&rc, syfreq, ltfreq, 1u << RC_SHIFT);
				qsupdate(&qsm_literals, c);
				putc(from_int(c), out);
			}
			fprintf(out, "\n");
		}
		else if(t_type == T_SV)
		{
			int len_del = 0;
			int len_ins = 0;

			for(int i = 0; i < 3; ++i)
			{
				ltfreq = decode_culshift(&rc, RC_SHIFT);
				int len = qsgetsym(&qsm_lens_sv[0], ltfreq);
				qsgetfreq(&qsm_lens_sv[0], len, &syfreq, &ltfreq);
				decode_update(&rc, syfreq, ltfreq, 1u << RC_SHIFT);
				qsupdate(&qsm_lens_sv[0], len);
				len_del += len << (i*8);
			}

			fprintf(out, " SV %d ", len_del);
			
			for(int i = 0; i < 3; ++i)
			{
				ltfreq = decode_culshift(&rc, RC_SHIFT);
				int len = qsgetsym(&qsm_lens_sv[1], ltfreq);
				qsgetfreq(&qsm_lens_sv[1], len, &syfreq, &ltfreq);
				decode_update(&rc, syfreq, ltfreq, 1u << RC_SHIFT);
				qsupdate(&qsm_lens_sv[1], len);
				len_ins += len << (i*8);
			}
		
			if(len_ins == 0)
				fprintf(out, "-\n");
			else
			{
				for(int i = 0; i < len_ins; ++i)
				{
					ltfreq = decode_culshift(&rc, RC_SHIFT);
					int c = qsgetsym(&qsm_literals, ltfreq);
					qsgetfreq(&qsm_literals, c, &syfreq, &ltfreq);
					decode_update(&rc, syfreq, ltfreq, 1u << RC_SHIFT);
					qsupdate(&qsm_literals, c);
					putc(from_int(c), out);
				}
				fprintf(out, "\n");
			}
		}
	
		ctx_type = t_type;
}
	
	done_decoding(&rc);
	
	fclose(out);
}

// ***************************************************************
// Check parameters, input files etc.
bool check_data(int argc, char *argv[])
{
	if(argc < 4)
	{
		cout << "Usage: tgc_db <mode> <input_name> <output_name>\n";
		cout << "  mod         - c (compression) or d (decompression)\n";
		cout << "  input_name  - name of the input file\n";
		cout << "  output_name - name of the output file\n";
		cout << "Examples:\n";
		cout << "  tgc c chr1.vcldb chr.tgc_db\n";
		cout << "  tgc d chr1.tgc_db chr1.vcldb_ori\n";
		return false;
	}

	if(strcmp(argv[1], "c") == 0)		// compression
		compress_mode = true;
	else 
		compress_mode = false;

	input_name  = string(argv[2]);
	output_name = string(argv[3]);
	
	return true;
}

// ***************************************************************
// Close output files
void close_files(void)
{
	if(rc_file)
		fclose(rc_file);
}

// ***************************************************************
// ***************************************************************
int main(int argc, char *argv[])
{
	clock_t t1 = clock();

	if(!check_data(argc, argv))
		return 0;
		
	prepare_files(output_name);
	
	if(compress_mode)
	{
		cout << "Compressing...\n";
		compress();
	}
	else
	{
		cout << "Decompressing...\n";
		decompress();
	}
	close_files();
	
	cout << "\nTime: " << (double) (clock() - t1) / CLOCKS_PER_SEC << "\n";
	
	return 0;
}