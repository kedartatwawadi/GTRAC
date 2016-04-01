CC=g++
CFLAGS  = -O3 -m64 -pthread -std=c++11
LFLAGS  = -O3 -m64 -pthread -std=c++11
OMPFLAGS = -fopenmp

all: VCF2VCFmin VCF2VDBV-d VCF2VDBV-h VDBV2VCFmin-d VDBV2VCFmin-h gtrac_comp gtrac_decomp

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

gtrac_comp: gtrac_compressor_v1.o
	$(CC) $(LFLAGS) -o $@ gtrac_compressor_v1.o -lRSDic

tgc_db: tgc_db.o rangecod.o qsmodel.o 
	$(CC) $(LFLAGS) -o $@ tgc_db.o rangecod.o qsmodel.o

gtrac_decomp: gtrac_decompressor_v1.o
	$(CC) $(LFLAGS) $(OMPFLAGS) -o $@ gtrac_decompressor_v1.o -lRSDic

clean:
	-rm VCF2VCFmin
	-rm VCF2VDBV-d
	-rm VCF2VDBV-h
	-rm VDBV2VCFmin-d
	-rm VDBV2VCFmin-h
	-rm *.o
	-rm gtrac_comp
	-rm gtrac_decomp
	-rm tgc_db
