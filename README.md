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

### Test random access
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

## Implementation Details
We will go into the details of the datasets we used, and also more details regarding the compression and the decompression techniques.

### Datasets
We used two large datasets: 1000 Genome Project Phase 1 *H.Sapiens* dataset and the 1001 Genomes *A.Thaliana* dataset.

1. **H. Sapiens Dataset**: The 1000 Genome Project phase 1, can be downloaded from:

..1. Link to [Reference FASTA Files]( ftp://ftp.ncbi.nlm.nih.gov/genomes/archive/old_genbank/Eukaryotes/vertebrates_mammals/Homo_sapiens/GRCh37/Primary_Assembly/assembled_chromosomes/FASTA/)
..2. Link to [VCF files](ftp://ftp.1000genomes.ebi.ac.uk/vol1/ftp/phase1/analysis_results/integrated_call_sets/)

2. **A.Thaliana dataset**: The 1001 GP dataset can be obtained from 4 different subprojects.

..1. Link to [Reference FASTA files](ftp://ftp.arabidopsis.org//Sequences/whole_chromosomes/)
..2. The VCF files can be downloaded from the 4 different subprojects:

