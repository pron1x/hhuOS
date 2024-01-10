#!/bin/bash

rm -f "../nvme0.img"
mkfs.fat -F 16 -C "nvme0part.img" 63488
mcopy -s -o -i "nvme0part.img" img/* ::/

fallocate -l 1M "nvme0fill.img"
cat nvme0fill.img nvme0part.img nvme0fill.img > "../nvme0.img"

rm nvme0part.img nvme0fill.img
echo -e "o\\nn\\np\\n1\\n2048\\n131071\\nt\\ne\\nw\\n" | fdisk "../nvme0.img"
