# GTRAC: GenoType Random Access Compressor
GTRAC is a formulation for compression of a collection of genomic variants. It achieves random access over the compressed dataset.

## Getting Started
### Setup
1. **Clone the repo**: The github repository contains all the tested code
2. **Install rsdic**: Rsdic is a succinct bitvector library which we use. Use the following link to install the rsdic library.
3. **Test Script**: As genomic data is generally very huge, we have provided some smaller sample data, which we use for testing. Run the test script to check if everything is all right.

### Data
To emulate the experiments conducted in the paper. You can run the following scripts to download the relevant data:
1. **H.sapiens dataset**: 
2. **A.Thaliana dataset**:

### Usage
The following files in the github repo have the listed functionality: 

The compression usage is as follows:
The decompression usage is as follows:
1. Row-wise decopression
2. Column-wise decompression

## Additional Implementation Details
We here describe the implementation details which are very specific but are important to understand the source code.
### Compression
### Decompression

## Complete Results
The paper included partial results for the experiments which we conducted. We here include the complete results for completeness.

