#! /bin/bash

set -e

echo -e '\n\e[31;1m* Compiling\e[m'
gcc -Wall -O3 -pthread main.c -o hw2
gcc -Wall -O3 std.c -o hw2_std

echo -e '\n\e[31;1m* Generating unsorted data\e[m'
dd if=/dev/urandom of=unsorted bs=4MB count=1 iflag=fullblock

echo -e '\n\e[31;1m* Printing unsorted data\e[m'
od -A x -t u4 unsorted > temp
head temp
echo '......'
tail temp

echo -e '\n\e[31;1m* Sorting data\e[m'
time ./hw2

echo -e '\n\e[31;1m* Sorting data (reference program)\e[m'
time ./hw2_std

echo -e '\n\e[31;1m* Printing sorted data\e[m'
od -A x -t u4 sorted > temp
head temp
echo '......'
tail temp
rm temp

echo -e '\n\e[31;1m* Comparing result (SHA256 should match)\e[m'
sha256sum sorted sorted_std
