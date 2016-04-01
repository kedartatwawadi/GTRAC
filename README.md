# GTRAC
GeneType Random Access Compressor

## Getting Started
### Install
To install the dependencies and the required code:
```bash
# download GTRAC
git clone https://github.com/kedartatwawadi/GTRAC

# Run the install file to download and install rsdic,TGC and GTRAC libraries
./install.sh
```

The shell script clones the following dependencies and installs them:

1. [rsdic](https://github.com/kedartatwawadi/rsdic): The library implementation of succinct bitvectors
2. [TGC](https://github.com/refresh-bio/TGC): The TGC Compressor, which we use for VCF file handling and variant dictionary compression

### Run
You can run an example of the GTRAC compressor using the following command.
```bash
# Run GTRAC for the chromosomes mentioned in config.ini of the 1000GP project
./run -abceik
```
The shell script does the following thigs:

1. Downloads VCF and reference files (for chromosome 22, by default) corresponding to the 1000 Genome Project
2. Uses tools from **TGC** repo to process the VCF files and convert it into the (H,VD) representation
3. Uses GTRAC compressor to compress the biinary matrix H  

### Local Decompression Test
GTRAC supports per-variant extraction (column-wise extraction), and per-haplotype extraction (row-wise extraction) from the compressed variant dataset. You can run the following examples to test these features:
```bash
# Move to the correct directory
cd ../Data/chr22

# single-variant extraction:
# This corresponds to extracting 792-799th variant (the 100th symbol) information at a time. 
../../GTRAC/src/gtrac_decomp c chr22.list 100

# complete haplotype extraction: 
# This corresponds to extracting 1000th haplotype from the compressed dataset. 
../../GTRAC/src/gtrac_decomp f chr22.list 1000

# haplotype sub-sequence extraction: 
# This corresponds to extracting a subsequence of length 1000 starting from 10th symbol of 800th haplotype of the compressed dataset. 
../../GTRAC/src/gtrac_decomp f chr22.list 800 10 1000

```


