#!/bin/bash

# Run the regen-fsdata.sh script
./regen-fsdata.sh

# Change directory to the build directory
cd build

# Run cmake to generate build files
cmake ..

# Run make to build the project
make
