#!/bin/sh

# clean all the previous builds
make clean
# make the orignal epanet
make

# make the epanet engine file
make -f Makefile_wateropt
