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

##### H. Sapiens Dataset 
The 1000 Genome Project phase 1, can be downloaded from:

1. Link to the Reference files for all the chromosomes:  <ftp://ftp.ncbi.nlm.nih.gov/genomes/archive/old_genbank/Eukaryotes/vertebrates_mammals/Homo_sapiens/GRCh37/Primary_Assembly/assembled_chromosomes/FASTA/>
2. Link to the VCF files for all the chromosomes <ftp://ftp.1000genomes.ebi.ac.uk/vol1/ftp/phase1/analysis_results/integrated_call_sets/>

##### A.Thaliana dataset 
The 1001 GP dataset can be obtained from 4 different subprojects.

1. Link to the Reference FASTA files for all chromosomes : <ftp://ftp.arabidopsis.org//Sequences/whole_chromosomes/>
2. The VCF files can be downloaded from the 4 different subprojects:
  1. Link 1
  2. Link 2
  3. Link 3
  4. Link 4

The datasets are the same as the ones used by [TGC](sun.aei.polsl.pl/tgc/).

### Compression Details
###### Variant Dictionary compression
The variant dictionary represents an indexed list of all the variants present in the dataset. As the variant dictionary typically consists of a small portion (4% to 5% for the 1000GP dataset) of the memory usage of the (H,VD) representation, we concentrate our efforts on the H matrix compression. We use the same compression technique as [TGC](sun.aei.polsl.pl/tgc/) to compress the variant dictionary. The basic idea is to compress each type of variant (SNP, insertion, deletion and SV). For each variant type, the variant positions are differentially encoded ( for eg: distances between consecutive SNPs) are stored. For every SNP, the substituting symbol is stored, for INS the length and the inserted symbols are stored etc. All these values are encoded using a variant of arithmetic coding with appropriate contextual models.
More details can be found in the the [TGC](sun.aei.polsl.pl/tgc/) paper.

###### Binary Matrix Compression
The binary matrix compression implementation is rather slow right now, as we have concentrated on the local decompression speeds, which is generally the more important factor. For the 1000GP dataset, the compression algorithm required 15 to 30 mins for the compression of the variant information corresponding to a chromosome.

We experimented with different values of parameter K for forming the symbol matrix. For K being multiples of 8, it was observed that memory handling is much easier. The random access is also effectively faster due to the byte-aligned memory handling. For K=16, the overall archival memory usage is similar to K=8. K=16 results in faster compression and haplotype-extraction (with an increase of almost a factor of 2), however the single-variant extraction times remain almost the same. This is expected as our haplotype substring extraction algorithm extracts the sub-string a symbol at a time Thus, increase in symbol size results in lesser symbols to be extracted for a sub-string. As K value increases, the memory requirement to represent the mismatching symbol C increases, however the phrase-type parameter representation requires less amount of memory. Overall, both of K=8 and K=16 perform quite well.




