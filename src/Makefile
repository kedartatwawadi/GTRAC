CC=g++
CFLAGS  = -O3 -m64 -pthread -std=c++11
LFLAGS  = -O3 -m64 -pthread -std=c++11
OMPFLAGS = -fopenmp

all: gtrac_comp gtrac_decomp


.cpp.o:
	$(CC) $(CFLAGS) $(OMPFLAGS) -c $<

gtrac_comp: gtrac_compressor_v1.o
	$(CC) $(LFLAGS) -o $@ gtrac_compressor_v1.o -lRSDic

gtrac_decomp: gtrac_decompressor_v1.o
	$(CC) $(LFLAGS) $(OMPFLAGS) -o $@ gtrac_decompressor_v1.o -lRSDic

clean:
	-rm *.o
	-rm gtrac_comp
	-rm gtrac_decomp
