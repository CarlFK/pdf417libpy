#!/bin/bash

# fixpdf417enc.sh - fix the tarbal - remove binaries, move examples into examples, remove obsolete versions...

set -xe

wget -N http://voxel.dl.sourceforge.net/sourceforge/pdf417encode/pdf417_enc.4.4.tar.gz
rm -rf pdf417_enc.4.4/
tar xvf pdf417_enc.4.4.tar.gz 
cd pdf417_enc.4.4/

# pdf417_dec is in both pdf417_dec and frontend_src.  it is for decoding images of barcodes back to thext, which is not what pdf417_encode is for.  It looks like some useful code, so it should get it's own project.
rm -rf pdf417_dec frontend_src

# the code from pdf417_prep has been merged into pdf417_enc.c (the functions are in different order, and a few lines were changed)
rm -rf pdf417_prep

# remove binaries
rm libpdf417enc.so pdf417_enc scan_data scan_image lib_test lib_test2 scan main.o pdf417_enc.o tests.o 

# output files from test/examples
rm out*

# README is correct, .OLD is not.
rm README.OLD 

# same ast test.c with some typos fixed.
rm tests.n.c 

# might be value in _old, but I don't see it
rm main_old.c

# makes main_old
rm Makefile.old

# lib_test.c has hardcoded datalines, 
# lib_test2.c generates them

# lib_test.c is lib_test_old.c and lib_test_new.c
rm lib_test_old.c lib_test_new.c

mkdir examples/api
mv lib_test.c lib_test2.c examples/api
mv run.scan examples

# mkdir tests
# mv tests.c tests

# all of mods is included in pdf417_enc.c, nothing references this file
rm mods
