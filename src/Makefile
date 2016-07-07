CC=g++
CFLAGS  = -O3 -m64 -pthread -std=c++11
LFLAGS  = -O3 -m64 -pthread -std=c++11
OMPFLAGS = -fopenmp

all: gtrac_comp gtrac_decomp


.cpp.o:
	$(CC) $(CFLAGS) $(OMPFLAGS) -c $<

gtrac_comp: gtrac_compressor.o
	$(CC) $(LFLAGS) -o $@ gtrac_compressor.o -lRSDic

gtrac_decomp: gtrac_decompressor.o
	$(CC) $(LFLAGS) $(OMPFLAGS) -o $@ gtrac_decompressor.o -lRSDic

clean:
	-rm *.o
	-rm gtrac_comp
	-rm gtrac_decomp
