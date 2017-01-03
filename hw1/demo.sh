#! /bin/bash

set -e

echo -e '\n\e[31;1m* Compiling\e[m'
gcc -Wall -O3 -pthread main.c -o hw1

echo -e '\n\e[31;1m* Generating sample data\e[m'
echo -e '1 1 10\n2 5 2\n3 6 3' > data
cat data

echo -e '\n\e[31;1m* Simulating sample case (teller_num = 2)\e[m'
./hw1 2 < data
