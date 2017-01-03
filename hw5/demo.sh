#! /bin/bash

set -e

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

echo -e '\n\e[31;1m* Compiling\e[m'
make

echo -e '\n\e[31;1m* Inspecting kernel module\e[m'
modinfo mypipe.ko

echo -e '\n\e[31;1m* Loading kernel module (mypipe related log should appear in dmesg output)\e[m'
rmmod mypipe 2>/dev/null || :
insmod ./mypipe.ko
dmesg --color=always | tail

echo -e '\n\e[31;1m* Generating random data\e[m'
dd if=/dev/urandom of=test256M bs=256M count=1 iflag=fullblock

echo -e '\n\e[31;1m* Sending data through /dev/mypipe\e[m'
dd if=test256M of=/dev/mypipe bs=256M count=1 iflag=fullblock &
dd if=/dev/mypipe of=recv256M bs=256M count=1 iflag=fullblock
wait

echo -e '\n\e[31;1m* Comparing recived file (SHA256 should match)\e[m'
sha256sum test256M recv256M

echo -e '\n\e[31;1m* Unloading kernel module (mypipe related log should appear in dmesg output)\e[m'
rmmod mypipe
dmesg --color=always | tail
