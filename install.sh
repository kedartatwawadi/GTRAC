#!/bin/bash

cd ..
# Clone the install rsdic repo
git clone https://github.com/kedartatwawadi/rsdic
cd rsdic
./waf configure
./waf
sudo ./waf install
cd ..

# Clone and build the TGC repo
git clone https://github.com/refresh-bio/TGC
cd TGC
cd src
make
cd ../..

# Build GTRAC
cd GTRAC
cd src
make
cd ..

# Create Data dir to hold the experiment data
mkdir ../Data



