CC=g++
CFLAGS  = -O3 -m64 -pthread -std=c++11
LFLAGS  = -O3 -m64 -pthread -std=c++11
OMPFLAGS = -fopenmp

all: VCF2VCFmin VCF2VDBV-d VCF2VDBV-h VDBV2VCFmin-d VDBV2VCFmin-h tgc_3 tgc_5 tgc_single_file tgc_db tgc_decomp_3 tgc_decomp_5.1 tgc_decomp_single_file

VCF2VCFmin: VCF2VCFmin.cpp
	$(CC) $(CFLAGS) VCF2VCFmin.cpp -o $@  

VCF2VDBV-d: VCF2VDBV-d.cpp      
	$(CC) $(CFLAGS) $(OMPFLAGS) VCF2VDBV-d.cpp -o $@  

VCF2VDBV-h: VCF2VDBV-h.cpp        
	$(CC) $(CFLAGS) $(OMPFLAGS) VCF2VDBV-h.cpp -o $@ 

VDBV2VCFmin-d: VDBV2VCFmin-d.cpp        
	$(CC) $(CFLAGS) VDBV2VCFmin-d.cpp -o $@ 

VDBV2VCFmin-h: VDBV2VCFmin-h.cpp               
	$(CC) $(CFLAGS) VDBV2VCFmin-h.cpp -o $@ 

.cpp.o:
	$(CC) $(CFLAGS) $(OMPFLAGS) -c $<

tgc_3: tgc_kedar_2.1.3.o
	$(CC) $(LFLAGS) -o $@ tgc_kedar_2.1.3.o libRSDic_kedi.a

tgc_5: tgc_kedar_2.1.5.o
	$(CC) $(LFLAGS) -o $@ tgc_kedar_2.1.5.o libRSDic_kedi.a

tgc_single_file: tgc_kedar_2.1.5_single_file.o
	$(CC) $(LFLAGS) -o $@ tgc_kedar_2.1.5_single_file.o libRSDic_kedi.a

tgc_db: tgc_db.o rangecod.o qsmodel.o 
	$(CC) $(LFLAGS) -o $@ tgc_db.o rangecod.o qsmodel.o

tgc_decomp_3: tgc_kedar_decompress_2.1.3.1.o libRSDic_kedi.a
	$(CC) $(LFLAGS) $(OMPFLAGS) -o $@ tgc_kedar_decompress_2.1.3.1.o libRSDic_kedi.a

tgc_decomp_5.1: tgc_kedar_decompress_2.1.5.1.o libRSDic_kedi.a
	$(CC) $(LFLAGS) $(OMPFLAGS) -o $@ tgc_kedar_decompress_2.1.5.1.o libRSDic_kedi.a

tgc_decomp_single_file: tgc_kedar_decompress_2.1.5.1_single_file.o libRSDic_kedi.a
	$(CC) $(LFLAGS) $(OMPFLAGS) -o $@ tgc_kedar_decompress_2.1.5.1_single_file.o libRSDic_kedi.a

clean:
	-rm VCF2VCFmin
	-rm VCF2VDBV-d
	-rm VCF2VDBV-h
	-rm VDBV2VCFmin-d
	-rm VDBV2VCFmin-h
	-rm *.o
	-rm tgc_3
	-rm tgc_5
	-rm tgc_db
	-rm tgc_decomp_3
	-rm tgc_decomp_*
	-rm tgc_single_file
